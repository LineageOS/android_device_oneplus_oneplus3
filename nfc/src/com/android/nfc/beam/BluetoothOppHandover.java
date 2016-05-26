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

import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.util.Log;

import java.util.ArrayList;

public class BluetoothOppHandover implements Handler.Callback {
    static final String TAG = "BluetoothOppHandover";
    static final boolean DBG = true;

    static final int STATE_INIT = 0;
    static final int STATE_TURNING_ON = 1;
    static final int STATE_WAITING = 2; // Need to wait for remote side turning on BT
    static final int STATE_COMPLETE = 3;

    static final int MSG_START_SEND = 0;

    static final int REMOTE_BT_ENABLE_DELAY_MS = 5000;

    static final String ACTION_HANDOVER_SEND =
            "android.nfc.handover.intent.action.HANDOVER_SEND";

    static final String ACTION_HANDOVER_SEND_MULTIPLE =
            "android.nfc.handover.intent.action.HANDOVER_SEND_MULTIPLE";

    final Context mContext;
    final BluetoothDevice mDevice;

    final ArrayList<Uri> mUris;
    final boolean mRemoteActivating;
    final Handler mHandler;
    final Long mCreateTime;

    int mState;

    public BluetoothOppHandover(Context context, BluetoothDevice device, ArrayList<Uri> uris,
            boolean remoteActivating) {
        mContext = context;
        mDevice = device;
        mUris = uris;
        mRemoteActivating = remoteActivating;
        mCreateTime = SystemClock.elapsedRealtime();

        mHandler = new Handler(context.getMainLooper(), this);
        mState = STATE_INIT;
    }

    /**
     * Main entry point. This method is usually called after construction,
     * to begin the BT sequence. Must be called on Main thread.
     */
    public void start() {
        if (mRemoteActivating) {
            Long timeElapsed = SystemClock.elapsedRealtime() - mCreateTime;
            if (timeElapsed < REMOTE_BT_ENABLE_DELAY_MS) {
                mHandler.sendEmptyMessageDelayed(MSG_START_SEND,
                        REMOTE_BT_ENABLE_DELAY_MS - timeElapsed);
            } else {
                // Already waited long enough for BT to come up
                // - start send.
                sendIntent();
            }
       } else {
            // Remote BT enabled already, start send immediately
            sendIntent();
        }
    }

    void complete() {
        mState = STATE_COMPLETE;
    }
    void sendIntent() {
        Intent intent = new Intent();
        intent.setPackage("com.android.bluetooth");
        String mimeType = MimeTypeUtil.getMimeTypeForUri(mContext, mUris.get(0));
        intent.setType(mimeType);
        intent.putExtra(BluetoothDevice.EXTRA_DEVICE, mDevice);
        for (Uri uri : mUris) {
            // TODO we need to transfer our permission grant from NFC
            // to the Bluetooth process. This works, but we don't have
            // a good framework API for revoking permission yet.
            try {
                mContext.grantUriPermission("com.android.bluetooth", uri,
                        Intent.FLAG_GRANT_READ_URI_PERMISSION);
            } catch (SecurityException e) {
                Log.e(TAG, "Failed to transfer permission to Bluetooth process.");
            }
        }
       if (mUris.size() == 1) {
            intent.setAction(ACTION_HANDOVER_SEND);
            intent.putExtra(Intent.EXTRA_STREAM, mUris.get(0));
        } else {
           intent.setAction(ACTION_HANDOVER_SEND_MULTIPLE);
            intent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, mUris);
        }
        if (DBG) Log.d(TAG, "Handing off outging transfer to BT");
        intent.addFlags(Intent.FLAG_RECEIVER_FOREGROUND);
        mContext.sendBroadcast(intent);

        complete();
    }


    @Override
    public boolean handleMessage(Message msg) {
        if (msg.what == MSG_START_SEND) {
            sendIntent();
            return true;
        }
        return false;
    }
}
