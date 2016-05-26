/*
 * Copyright (C) 2014 The Android Open Source Project
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

import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;

public class BeamSendService extends Service implements BeamTransferManager.Callback {
    private static String TAG = "BeamSendService";
    private static boolean DBG = true;

    public static String EXTRA_BEAM_TRANSFER_RECORD
            = "com.android.nfc.beam.EXTRA_BEAM_TRANSFER_RECORD";
    public static final String EXTRA_BEAM_COMPLETE_CALLBACK
            = "com.android.nfc.beam.TRANSFER_COMPLETE_CALLBACK";

    private BeamTransferManager mTransferManager;
    private BeamStatusReceiver mBeamStatusReceiver;
    private boolean mBluetoothEnabledByNfc;
    private Messenger mCompleteCallback;
    private int mStartId;

    private final BluetoothAdapter mBluetoothAdapter;
    private final BroadcastReceiver mBluetoothStateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)) {
                handleBluetoothStateChanged(intent);
            }
        }
    };

    public BeamSendService() {
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    }

    @Override
    public void onCreate() {
       super.onCreate();

        // register BT state receiver
        IntentFilter filter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
        registerReceiver(mBluetoothStateReceiver, filter);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        if (mBeamStatusReceiver != null) {
            unregisterReceiver(mBeamStatusReceiver);
        }
        unregisterReceiver(mBluetoothStateReceiver);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        mStartId = startId;

        BeamTransferRecord transferRecord;
        if (intent == null ||
               (transferRecord = intent.getParcelableExtra(EXTRA_BEAM_TRANSFER_RECORD)) == null) {
            if (DBG) Log.e(TAG, "No transfer record provided. Stopping.");
            stopSelf(startId);
            return START_NOT_STICKY;
        }

        mCompleteCallback = intent.getParcelableExtra(EXTRA_BEAM_COMPLETE_CALLBACK);

        if (doTransfer(transferRecord)) {
            if (DBG) Log.i(TAG, "Starting outgoing Beam transfer");
            return START_STICKY;
        } else {
            invokeCompleteCallback(false);
            stopSelf(startId);
            return START_NOT_STICKY;
        }
    }

    boolean doTransfer(BeamTransferRecord transferRecord) {
        if (createBeamTransferManager(transferRecord)) {
            // register Beam status receiver
            mBeamStatusReceiver = new BeamStatusReceiver(this, mTransferManager);
            registerReceiver(mBeamStatusReceiver, mBeamStatusReceiver.getIntentFilter(),
                    BeamStatusReceiver.BEAM_STATUS_PERMISSION, new Handler());

            if (transferRecord.dataLinkType == BeamTransferRecord.DATA_LINK_TYPE_BLUETOOTH) {
                if (mBluetoothAdapter.isEnabled()) {
                    // Start the transfer
                    mTransferManager.start();
                } else {
                    if (!mBluetoothAdapter.enableNoAutoConnect()) {
                        Log.e(TAG, "Error enabling Bluetooth.");
                        mTransferManager = null;
                        return false;
                    }
                    mBluetoothEnabledByNfc = true;
                    if (DBG) Log.d(TAG, "Queueing out transfer "
                            + Integer.toString(transferRecord.id));
                }
            }
            return true;
        }

        return false;
    }

    boolean createBeamTransferManager(BeamTransferRecord transferRecord) {
        if (mTransferManager != null) {
            return false;
        }

        if (transferRecord.dataLinkType != BeamTransferRecord.DATA_LINK_TYPE_BLUETOOTH) {
            // only support BT
            return false;
        }

        mTransferManager = new BeamTransferManager(this, this, transferRecord, false);
        mTransferManager.updateNotification();
        return true;
    }

    private void handleBluetoothStateChanged(Intent intent) {
        int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE,
                BluetoothAdapter.ERROR);
        if (state == BluetoothAdapter.STATE_ON) {
            if (mTransferManager != null &&
                    mTransferManager.mDataLinkType == BeamTransferRecord.DATA_LINK_TYPE_BLUETOOTH) {
                mTransferManager.start();
            }
       } else if (state == BluetoothAdapter.STATE_OFF) {
            mBluetoothEnabledByNfc = false;
        }
    }

    private void invokeCompleteCallback(boolean success) {
        if (mCompleteCallback != null) {
            try {
                Message msg = Message.obtain(null, BeamManager.MSG_BEAM_COMPLETE);
                msg.arg1 = success ? 1 : 0;
                mCompleteCallback.send(msg);
            } catch (RemoteException e) {
                Log.e(TAG, "failed to invoke Beam complete callback", e);
            }
        }
    }

    @Override
    public void onTransferComplete(BeamTransferManager transfer, boolean success) {
        // Play success sound
        if (!success) {
            if (DBG) Log.d(TAG, "Transfer failed, final state: " +
                    Integer.toString(transfer.mState));
        }

        if (mBluetoothEnabledByNfc) {
            mBluetoothEnabledByNfc = false;
            mBluetoothAdapter.disable();
        }

        invokeCompleteCallback(success);
        stopSelf(mStartId);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
