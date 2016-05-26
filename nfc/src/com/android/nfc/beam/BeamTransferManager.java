/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.nfc.beam;

import com.android.nfc.R;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Notification.Builder;
import android.bluetooth.BluetoothDevice;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.SystemClock;
import android.os.UserHandle;
import android.util.Log;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.Locale;

/**
 * A BeamTransferManager object represents a set of files
 * that were received through NFC connection handover
 * from the same source address.
 *
 * It manages starting, stopping, and processing the transfer, as well
 * as the user visible notification.
 *
 * For Bluetooth, files are received through OPP, and
 * we have no knowledge how many files will be transferred
 * as part of a single transaction.
 * Hence, a transfer has a notion of being "alive": if
 * the last update to a transfer was within WAIT_FOR_NEXT_TRANSFER_MS
 * milliseconds, we consider a new file transfer from the
 * same source address as part of the same transfer.
 * The corresponding URIs will be grouped in a single folder.
 *
 * @hide
 */

public class BeamTransferManager implements Handler.Callback,
        MediaScannerConnection.OnScanCompletedListener {
    interface Callback {

        void onTransferComplete(BeamTransferManager transfer, boolean success);
    };
    static final String TAG = "BeamTransferManager";

    static final Boolean DBG = true;

    // In the states below we still accept new file transfer
    static final int STATE_NEW = 0;
    static final int STATE_IN_PROGRESS = 1;
    static final int STATE_W4_NEXT_TRANSFER = 2;
    // In the states below no new files are accepted.
    static final int STATE_W4_MEDIA_SCANNER = 3;
    static final int STATE_FAILED = 4;
    static final int STATE_SUCCESS = 5;
    static final int STATE_CANCELLED = 6;
    static final int STATE_CANCELLING = 7;
    static final int MSG_NEXT_TRANSFER_TIMER = 0;

    static final int MSG_TRANSFER_TIMEOUT = 1;
    static final int DATA_LINK_TYPE_BLUETOOTH = 1;

    // We need to receive an update within this time period
    // to still consider this transfer to be "alive" (ie
    // a reason to keep the handover transport enabled).
    static final int ALIVE_CHECK_MS = 20000;

    // The amount of time to wait for a new transfer
    // once the current one completes.
    static final int WAIT_FOR_NEXT_TRANSFER_MS = 4000;

    static final String BEAM_DIR = "beam";

    static final String ACTION_WHITELIST_DEVICE =
            "android.btopp.intent.action.WHITELIST_DEVICE";

    static final String ACTION_STOP_BLUETOOTH_TRANSFER =
            "android.btopp.intent.action.STOP_HANDOVER_TRANSFER";

    final boolean mIncoming;  // whether this is an incoming transfer

    final int mTransferId; // Unique ID of this transfer used for notifications
    int mBluetoothTransferId; // ID of this transfer in Bluetooth namespace

    final PendingIntent mCancelIntent;
    final Context mContext;
    final Handler mHandler;
    final NotificationManager mNotificationManager;
    final BluetoothDevice mRemoteDevice;
    final Callback mCallback;
    final boolean mRemoteActivating;

    // Variables below are only accessed on the main thread
    int mState;
    int mCurrentCount;
    int mSuccessCount;
    int mTotalCount;
    int mDataLinkType;
    boolean mCalledBack;
    Long mLastUpdate; // Last time an event occurred for this transfer
    float mProgress; // Progress in range [0..1]
    ArrayList<Uri> mUris; // Received uris from transport
    ArrayList<String> mTransferMimeTypes; // Mime-types received from transport
    Uri[] mOutgoingUris; // URIs to send
    ArrayList<String> mPaths; // Raw paths on the filesystem for Beam-stored files
    HashMap<String, String> mMimeTypes; // Mime-types associated with each path
    HashMap<String, Uri> mMediaUris; // URIs found by the media scanner for each path
    int mUrisScanned;
    Long mStartTime;

    public BeamTransferManager(Context context, Callback callback,
                               BeamTransferRecord pendingTransfer, boolean incoming) {
        mContext = context;
        mCallback = callback;
        mRemoteDevice = pendingTransfer.remoteDevice;
        mIncoming = incoming;
        mTransferId = pendingTransfer.id;
        mBluetoothTransferId = -1;
        mDataLinkType = pendingTransfer.dataLinkType;
        mRemoteActivating = pendingTransfer.remoteActivating;
        mStartTime = 0L;
        // For incoming transfers, count can be set later
        mTotalCount = (pendingTransfer.uris != null) ? pendingTransfer.uris.length : 0;
        mLastUpdate = SystemClock.elapsedRealtime();
        mProgress = 0.0f;
        mState = STATE_NEW;
        mUris = pendingTransfer.uris == null
                ? new ArrayList<Uri>()
                : new ArrayList<Uri>(Arrays.asList(pendingTransfer.uris));
        mTransferMimeTypes = new ArrayList<String>();
        mMimeTypes = new HashMap<String, String>();
        mPaths = new ArrayList<String>();
        mMediaUris = new HashMap<String, Uri>();
        mCancelIntent = buildCancelIntent();
        mUrisScanned = 0;
        mCurrentCount = 0;
        mSuccessCount = 0;
        mOutgoingUris = pendingTransfer.uris;
        mHandler = new Handler(Looper.getMainLooper(), this);
        mHandler.sendEmptyMessageDelayed(MSG_TRANSFER_TIMEOUT, ALIVE_CHECK_MS);
        mNotificationManager = (NotificationManager) mContext.getSystemService(
                Context.NOTIFICATION_SERVICE);
    }

    void whitelistOppDevice(BluetoothDevice device) {
        if (DBG) Log.d(TAG, "Whitelisting " + device + " for BT OPP");
        Intent intent = new Intent(ACTION_WHITELIST_DEVICE);
        intent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
        mContext.sendBroadcastAsUser(intent, UserHandle.CURRENT);
    }

    public void start() {
        if (mStartTime > 0) {
            // already started
            return;
        }

        mStartTime = System.currentTimeMillis();

        if (!mIncoming) {
            if (mDataLinkType == BeamTransferRecord.DATA_LINK_TYPE_BLUETOOTH) {
                new BluetoothOppHandover(mContext, mRemoteDevice, mUris, mRemoteActivating).start();
            }
        }
    }

    public void updateFileProgress(float progress) {
        if (!isRunning()) return; // Ignore when we're no longer running

        mHandler.removeMessages(MSG_NEXT_TRANSFER_TIMER);

        this.mProgress = progress;

        // We're still receiving data from this device - keep it in
        // the whitelist for a while longer
        if (mIncoming && mRemoteDevice != null) whitelistOppDevice(mRemoteDevice);

        updateStateAndNotification(STATE_IN_PROGRESS);
    }

    public synchronized void setBluetoothTransferId(int id) {
        if (mBluetoothTransferId == -1 && id != -1) {
            mBluetoothTransferId = id;
            if (mState == STATE_CANCELLING) {
                sendBluetoothCancelIntentAndUpdateState();
            }
        }
    }

    public void finishTransfer(boolean success, Uri uri, String mimeType) {
        if (!isRunning()) return; // Ignore when we're no longer running

        mCurrentCount++;
        if (success && uri != null) {
            mSuccessCount++;
            if (DBG) Log.d(TAG, "Transfer success, uri " + uri + " mimeType " + mimeType);
            mProgress = 0.0f;
            if (mimeType == null) {
                mimeType = MimeTypeUtil.getMimeTypeForUri(mContext, uri);
            }
            if (mimeType != null) {
                mUris.add(uri);
                mTransferMimeTypes.add(mimeType);
            } else {
                if (DBG) Log.d(TAG, "Could not get mimeType for file.");
            }
        } else {
            Log.e(TAG, "Handover transfer failed");
            // Do wait to see if there's another file coming.
        }
        mHandler.removeMessages(MSG_NEXT_TRANSFER_TIMER);
        if (mCurrentCount == mTotalCount) {
            if (mIncoming) {
                processFiles();
            } else {
                updateStateAndNotification(mSuccessCount > 0 ? STATE_SUCCESS : STATE_FAILED);
            }
        } else {
            mHandler.sendEmptyMessageDelayed(MSG_NEXT_TRANSFER_TIMER, WAIT_FOR_NEXT_TRANSFER_MS);
            updateStateAndNotification(STATE_W4_NEXT_TRANSFER);
        }
    }

    public boolean isRunning() {
        if (mState != STATE_NEW && mState != STATE_IN_PROGRESS && mState != STATE_W4_NEXT_TRANSFER) {
            return false;
        } else {
            return true;
        }
    }

    public void setObjectCount(int objectCount) {
        mTotalCount = objectCount;
    }

    void cancel() {
        if (!isRunning()) return;

        // Delete all files received so far
        for (Uri uri : mUris) {
            File file = new File(uri.getPath());
            if (file.exists()) file.delete();
        }

        if (mBluetoothTransferId != -1) {
            // we know the ID, we can cancel immediately
            sendBluetoothCancelIntentAndUpdateState();
        } else {
            updateStateAndNotification(STATE_CANCELLING);
        }

    }

    private void sendBluetoothCancelIntentAndUpdateState() {
        Intent cancelIntent = new Intent(ACTION_STOP_BLUETOOTH_TRANSFER);
        cancelIntent.putExtra(BeamStatusReceiver.EXTRA_TRANSFER_ID, mBluetoothTransferId);
        mContext.sendBroadcast(cancelIntent);
        updateStateAndNotification(STATE_CANCELLED);
    }

    void updateNotification() {
        Builder notBuilder = new Notification.Builder(mContext);
        notBuilder.setColor(mContext.getResources().getColor(
                com.android.internal.R.color.system_notification_accent_color));
        notBuilder.setWhen(mStartTime);
        notBuilder.setVisibility(Notification.VISIBILITY_PUBLIC);
        String beamString;
       if (mIncoming) {
           beamString = mContext.getString(R.string.beam_progress);
        } else {
            beamString = mContext.getString(R.string.beam_outgoing);
        }
        if (mState == STATE_NEW || mState == STATE_IN_PROGRESS ||
                mState == STATE_W4_NEXT_TRANSFER || mState == STATE_W4_MEDIA_SCANNER) {
            notBuilder.setAutoCancel(false);
            notBuilder.setSmallIcon(mIncoming ? android.R.drawable.stat_sys_download :
                    android.R.drawable.stat_sys_upload);
            notBuilder.setTicker(beamString);
            notBuilder.setContentTitle(beamString);
            notBuilder.addAction(R.drawable.ic_menu_cancel_holo_dark,
                    mContext.getString(R.string.cancel), mCancelIntent);
            float progress = 0;
            if (mTotalCount > 0) {
                float progressUnit = 1.0f / mTotalCount;
                progress = (float) mCurrentCount * progressUnit + mProgress * progressUnit;
            }
            if (mTotalCount > 0 && progress > 0) {
                notBuilder.setProgress(100, (int) (100 * progress), false);
            } else {
                notBuilder.setProgress(100, 0, true);
           }
        } else if (mState == STATE_SUCCESS) {
            notBuilder.setAutoCancel(true);
            notBuilder.setSmallIcon(mIncoming ? android.R.drawable.stat_sys_download_done :
                    android.R.drawable.stat_sys_upload_done);
            notBuilder.setTicker(mContext.getString(R.string.beam_complete));
            notBuilder.setContentTitle(mContext.getString(R.string.beam_complete));

            if (mIncoming) {
                notBuilder.setContentText(mContext.getString(R.string.beam_touch_to_view));
                Intent viewIntent = buildViewIntent();
                PendingIntent contentIntent = PendingIntent.getActivity(
                        mContext, mTransferId, viewIntent, 0, null);

                notBuilder.setContentIntent(contentIntent);
            }
        } else if (mState == STATE_FAILED) {
            notBuilder.setAutoCancel(false);
            notBuilder.setSmallIcon(mIncoming ? android.R.drawable.stat_sys_download_done :
                    android.R.drawable.stat_sys_upload_done);
            notBuilder.setTicker(mContext.getString(R.string.beam_failed));
            notBuilder.setContentTitle(mContext.getString(R.string.beam_failed));
        } else if (mState == STATE_CANCELLED || mState == STATE_CANCELLING) {
            notBuilder.setAutoCancel(false);
            notBuilder.setSmallIcon(mIncoming ? android.R.drawable.stat_sys_download_done :
                    android.R.drawable.stat_sys_upload_done);
            notBuilder.setTicker(mContext.getString(R.string.beam_canceled));
            notBuilder.setContentTitle(mContext.getString(R.string.beam_canceled));
        } else {
            return;
        }

        mNotificationManager.notify(null, mTransferId, notBuilder.build());
    }

    void updateStateAndNotification(int newState) {
        this.mState = newState;
        this.mLastUpdate = SystemClock.elapsedRealtime();

        mHandler.removeMessages(MSG_TRANSFER_TIMEOUT);
        if (isRunning()) {
            // Update timeout timer if we're still running
            mHandler.sendEmptyMessageDelayed(MSG_TRANSFER_TIMEOUT, ALIVE_CHECK_MS);
        }

        updateNotification();

        if ((mState == STATE_SUCCESS || mState == STATE_FAILED || mState == STATE_CANCELLED)
                && !mCalledBack) {
            mCalledBack = true;
            // Notify that we're done with this transfer
            mCallback.onTransferComplete(this, mState == STATE_SUCCESS);
        }
    }

    void processFiles() {
        // Check the amount of files we received in this transfer;
        // If more than one, create a separate directory for it.
        String extRoot = Environment.getExternalStorageDirectory().getPath();
        File beamPath = new File(extRoot + "/" + BEAM_DIR);

        if (!checkMediaStorage(beamPath) || mUris.size() == 0) {
            Log.e(TAG, "Media storage not valid or no uris received.");
            updateStateAndNotification(STATE_FAILED);
            return;
        }

        if (mUris.size() > 1) {
            beamPath = generateMultiplePath(extRoot + "/" + BEAM_DIR + "/");
            if (!beamPath.isDirectory() && !beamPath.mkdir()) {
                Log.e(TAG, "Failed to create multiple path " + beamPath.toString());
                updateStateAndNotification(STATE_FAILED);
                return;
            }
        }

        for (int i = 0; i < mUris.size(); i++) {
            Uri uri = mUris.get(i);
            String mimeType = mTransferMimeTypes.get(i);

           File srcFile = new File(uri.getPath());

            File dstFile = generateUniqueDestination(beamPath.getAbsolutePath(),
                    uri.getLastPathSegment());
            Log.d(TAG, "Renaming from " + srcFile);
            if (!srcFile.renameTo(dstFile)) {
                if (DBG) Log.d(TAG, "Failed to rename from " + srcFile + " to " + dstFile);
                srcFile.delete();
                return;
            } else {
                mPaths.add(dstFile.getAbsolutePath());
                mMimeTypes.put(dstFile.getAbsolutePath(), mimeType);
                if (DBG) Log.d(TAG, "Did successful rename from " + srcFile + " to " + dstFile);
            }
        }

        // We can either add files to the media provider, or provide an ACTION_VIEW
        // intent to the file directly. We base this decision on the mime type
        // of the first file; if it's media the platform can deal with,
        // use the media provider, if it's something else, just launch an ACTION_VIEW
        // on the file.
        String mimeType = mMimeTypes.get(mPaths.get(0));
        if (mimeType.startsWith("image/") || mimeType.startsWith("video/") ||
                mimeType.startsWith("audio/")) {
            String[] arrayPaths = new String[mPaths.size()];
            MediaScannerConnection.scanFile(mContext, mPaths.toArray(arrayPaths), null, this);
            updateStateAndNotification(STATE_W4_MEDIA_SCANNER);
        } else {
            // We're done.
            updateStateAndNotification(STATE_SUCCESS);
        }

    }

    public boolean handleMessage(Message msg) {
        if (msg.what == MSG_NEXT_TRANSFER_TIMER) {
            // We didn't receive a new transfer in time, finalize this one
            if (mIncoming) {
                processFiles();
            } else {
                updateStateAndNotification(mSuccessCount > 0 ? STATE_SUCCESS : STATE_FAILED);
            }
            return true;
        } else if (msg.what == MSG_TRANSFER_TIMEOUT) {
            // No update on this transfer for a while, fail it.
            if (DBG) Log.d(TAG, "Transfer timed out for id: " + Integer.toString(mTransferId));
            updateStateAndNotification(STATE_FAILED);
        }
        return false;
    }

    public synchronized void onScanCompleted(String path, Uri uri) {
        if (DBG) Log.d(TAG, "Scan completed, path " + path + " uri " + uri);
        if (uri != null) {
            mMediaUris.put(path, uri);
        }
        mUrisScanned++;
        if (mUrisScanned == mPaths.size()) {
            // We're done
            updateStateAndNotification(STATE_SUCCESS);
        }
    }


    Intent buildViewIntent() {
        if (mPaths.size() == 0) return null;

        Intent viewIntent = new Intent(Intent.ACTION_VIEW);

        String filePath = mPaths.get(0);
        Uri mediaUri = mMediaUris.get(filePath);
        Uri uri =  mediaUri != null ? mediaUri :
            Uri.parse(ContentResolver.SCHEME_FILE + "://" + filePath);
        viewIntent.setDataAndTypeAndNormalize(uri, mMimeTypes.get(filePath));
        viewIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        return viewIntent;
   }

    PendingIntent buildCancelIntent() {
        Intent intent = new Intent(BeamStatusReceiver.ACTION_CANCEL_HANDOVER_TRANSFER);
        intent.putExtra(BeamStatusReceiver.EXTRA_ADDRESS, mRemoteDevice.getAddress());
        intent.putExtra(BeamStatusReceiver.EXTRA_INCOMING, mIncoming ?
                BeamStatusReceiver.DIRECTION_INCOMING : BeamStatusReceiver.DIRECTION_OUTGOING);
        PendingIntent pi = PendingIntent.getBroadcast(mContext, mTransferId, intent,
                PendingIntent.FLAG_ONE_SHOT);

        return pi;
    }

    static boolean checkMediaStorage(File path) {
        if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
            if (!path.isDirectory() && !path.mkdir()) {
                Log.e(TAG, "Not dir or not mkdir " + path.getAbsolutePath());
                return false;
            }
            return true;
        } else {
            Log.e(TAG, "External storage not mounted, can't store file.");
            return false;
        }
    }

    static File generateUniqueDestination(String path, String fileName) {
        int dotIndex = fileName.lastIndexOf(".");
        String extension = null;
        String fileNameWithoutExtension = null;
        if (dotIndex < 0) {
            extension = "";
            fileNameWithoutExtension = fileName;
        } else {
            extension = fileName.substring(dotIndex);
            fileNameWithoutExtension = fileName.substring(0, dotIndex);
        }
        File dstFile = new File(path + File.separator + fileName);
        int count = 0;
        while (dstFile.exists()) {
            dstFile = new File(path + File.separator + fileNameWithoutExtension + "-" +
                    Integer.toString(count) + extension);
           count++;
        }
        return dstFile;
    }
    static File generateMultiplePath(String beamRoot) {
        // Generate a unique directory with the date
        String format = "yyyy-MM-dd";
        SimpleDateFormat sdf = new SimpleDateFormat(format, Locale.US);
        String newPath = beamRoot + "beam-" + sdf.format(new Date());
        File newFile = new File(newPath);
        int count = 0;
        while (newFile.exists()) {
            newPath = beamRoot + "beam-" + sdf.format(new Date()) + "-" +
                    Integer.toString(count);
            newFile = new File(newPath);
            count++;
        }
        return newFile;
    }
}
