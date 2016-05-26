/*
* Copyright (C) 2008 The Android Open Source Project
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

import com.android.nfc.NfcService;
import com.android.nfc.handover.HandoverDataParser;

import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.UserHandle;
import android.util.Log;

/**
 * Manager for starting and stopping Beam transfers. Prevents more than one transfer from
 * happening at a time.
 */
public class BeamManager implements Handler.Callback {
    private static final String TAG = "BeamManager";
    private static final boolean DBG = false;

    private static final String ACTION_WHITELIST_DEVICE =
            "android.btopp.intent.action.WHITELIST_DEVICE";
    public static final int MSG_BEAM_COMPLETE = 0;

    private final Object mLock;

    private boolean mBeamInProgress;
    private final Handler mCallback;

    private NfcService mNfcService;

    private static final class Singleton {
        public static final BeamManager INSTANCE = new BeamManager();
    }

    private BeamManager() {
        mLock = new Object();
        mBeamInProgress = false;
        mCallback = new Handler(Looper.getMainLooper(), this);
        mNfcService = NfcService.getInstance();
    }

    public static BeamManager getInstance() {
        return Singleton.INSTANCE;
    }

    public boolean isBeamInProgress() {
        synchronized (mLock) {
            return mBeamInProgress;
        }
    }

    public boolean startBeamReceive(Context context,
                                 HandoverDataParser.BluetoothHandoverData handoverData) {
        synchronized (mLock) {
            if (mBeamInProgress) {
                return false;
            } else {
                mBeamInProgress = true;
            }
        }

        BeamTransferRecord transferRecord =
                BeamTransferRecord.forBluetoothDevice(
                        handoverData.device, handoverData.carrierActivating, null);

        Intent receiveIntent = new Intent(context.getApplicationContext(),
                BeamReceiveService.class);
        receiveIntent.putExtra(BeamReceiveService.EXTRA_BEAM_TRANSFER_RECORD, transferRecord);
        receiveIntent.putExtra(BeamReceiveService.EXTRA_BEAM_COMPLETE_CALLBACK,
                new Messenger(mCallback));
        whitelistOppDevice(context, handoverData.device);
        context.startServiceAsUser(receiveIntent, UserHandle.CURRENT);
        return true;
    }

    public boolean startBeamSend(Context context,
                               HandoverDataParser.BluetoothHandoverData outgoingHandoverData,
                               Uri[] uris, UserHandle userHandle) {
        synchronized (mLock) {
            if (mBeamInProgress) {
                return false;
            } else {
                mBeamInProgress = true;
            }
        }

        BeamTransferRecord transferRecord = BeamTransferRecord.forBluetoothDevice(
                outgoingHandoverData.device, outgoingHandoverData.carrierActivating,
                uris);
        Intent sendIntent = new Intent(context.getApplicationContext(),
                BeamSendService.class);
        sendIntent.putExtra(BeamSendService.EXTRA_BEAM_TRANSFER_RECORD, transferRecord);
        sendIntent.putExtra(BeamSendService.EXTRA_BEAM_COMPLETE_CALLBACK,
                new Messenger(mCallback));
        context.startServiceAsUser(sendIntent, userHandle);
        return true;
    }

    @Override
    public boolean handleMessage(Message msg) {
        if (msg.what == MSG_BEAM_COMPLETE) {
            synchronized (mLock) {
                mBeamInProgress = false;
            }

            boolean success = msg.arg1 == 1;
            if (success) {
                mNfcService.playSound(NfcService.SOUND_END);
            }
            return true;
        }
        return false;
    }

    void whitelistOppDevice(Context context, BluetoothDevice device) {
        if (DBG) Log.d(TAG, "Whitelisting " + device + " for BT OPP");
        Intent intent = new Intent(ACTION_WHITELIST_DEVICE);
        intent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
        context.sendBroadcastAsUser(intent, UserHandle.CURRENT);
    }

}
