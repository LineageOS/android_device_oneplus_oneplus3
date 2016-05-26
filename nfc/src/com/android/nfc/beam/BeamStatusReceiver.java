package com.android.nfc.beam;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.util.Log;

import java.io.File;

/**
 * @hide
*/
public class BeamStatusReceiver extends BroadcastReceiver {
   private static final boolean DBG = true;
    private static final String TAG = "BeamStatusReceiver";

    private static final String ACTION_HANDOVER_STARTED =
            "android.nfc.handover.intent.action.HANDOVER_STARTED";

    private static final String ACTION_TRANSFER_PROGRESS =
            "android.nfc.handover.intent.action.TRANSFER_PROGRESS";

    private static final String ACTION_TRANSFER_DONE =
            "android.nfc.handover.intent.action.TRANSFER_DONE";

    private static final String EXTRA_HANDOVER_DATA_LINK_TYPE =
            "android.nfc.handover.intent.extra.HANDOVER_DATA_LINK_TYPE";


    private static final String EXTRA_TRANSFER_PROGRESS =
            "android.nfc.handover.intent.extra.TRANSFER_PROGRESS";

    private static final String EXTRA_TRANSFER_URI =
            "android.nfc.handover.intent.extra.TRANSFER_URI";

    private static final String EXTRA_OBJECT_COUNT =
            "android.nfc.handover.intent.extra.OBJECT_COUNT";

    private static final String EXTRA_TRANSFER_STATUS =
            "android.nfc.handover.intent.extra.TRANSFER_STATUS";

    private static final String EXTRA_TRANSFER_MIMETYPE =
            "android.nfc.handover.intent.extra.TRANSFER_MIME_TYPE";

    private static final String ACTION_STOP_BLUETOOTH_TRANSFER =
            "android.btopp.intent.action.STOP_HANDOVER_TRANSFER";

    // FIXME: Needs to stay in sync with com.android.bluetooth.opp.Constants
    private static final int HANDOVER_TRANSFER_STATUS_SUCCESS = 0;
   private static final int HANDOVER_TRANSFER_STATUS_FAILURE = 1;

    // permission needed to be able to receive handover status requests
    public static final String BEAM_STATUS_PERMISSION =
            "android.permission.NFC_HANDOVER_STATUS";

    // Needed to build cancel intent in Beam notification
    public static final String EXTRA_INCOMING =
            "com.android.nfc.handover.extra.INCOMING";

    public static final String EXTRA_TRANSFER_ID =
            "android.nfc.handover.intent.extra.TRANSFER_ID";

    public static final String EXTRA_ADDRESS =
            "android.nfc.handover.intent.extra.ADDRESS";

    public static final String ACTION_CANCEL_HANDOVER_TRANSFER =
            "com.android.nfc.handover.action.CANCEL_HANDOVER_TRANSFER";

    public static final int DIRECTION_INCOMING = 0;
    public static final int DIRECTION_OUTGOING = 1;

    private final Context mContext;
   private final BeamTransferManager mTransferManager;

    BeamStatusReceiver(Context context, BeamTransferManager transferManager) {
        mContext = context;
        mTransferManager = transferManager;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        int dataLinkType = intent.getIntExtra(EXTRA_HANDOVER_DATA_LINK_TYPE,
                BeamTransferManager.DATA_LINK_TYPE_BLUETOOTH);

        if (ACTION_CANCEL_HANDOVER_TRANSFER.equals(action)) {
            if (mTransferManager != null) {
                mTransferManager.cancel();
            }
        } else if (ACTION_TRANSFER_PROGRESS.equals(action) ||
                ACTION_TRANSFER_DONE.equals(action) ||
                ACTION_HANDOVER_STARTED.equals(action)) {
            handleTransferEvent(intent, dataLinkType);
        }
    }

    public IntentFilter getIntentFilter() {
        IntentFilter filter = new IntentFilter(ACTION_TRANSFER_DONE);
        filter.addAction(ACTION_TRANSFER_PROGRESS);
        filter.addAction(ACTION_CANCEL_HANDOVER_TRANSFER);
        filter.addAction(ACTION_HANDOVER_STARTED);
        return filter;
    }

    private void handleTransferEvent(Intent intent, int deviceType) {
        String action = intent.getAction();
        int id = intent.getIntExtra(EXTRA_TRANSFER_ID, -1);

        String sourceAddress = intent.getStringExtra(EXTRA_ADDRESS);

        if (sourceAddress == null) return;

        if (mTransferManager == null) {
            // There is no transfer running for this source address; most likely
            // the transfer was cancelled. We need to tell BT OPP to stop transferring.
            if (id != -1) {
                if (deviceType == BeamTransferManager.DATA_LINK_TYPE_BLUETOOTH) {
                    if (DBG) Log.d(TAG, "Didn't find transfer, stopping");
                    Intent cancelIntent = new Intent(ACTION_STOP_BLUETOOTH_TRANSFER);
                    cancelIntent.putExtra(EXTRA_TRANSFER_ID, id);
                    mContext.sendBroadcast(cancelIntent);
                }
            }
            return;
        }

        mTransferManager.setBluetoothTransferId(id);

        if (action.equals(ACTION_TRANSFER_DONE)) {
            int handoverStatus = intent.getIntExtra(EXTRA_TRANSFER_STATUS,
                    HANDOVER_TRANSFER_STATUS_FAILURE);
            if (handoverStatus == HANDOVER_TRANSFER_STATUS_SUCCESS) {
                String uriString = intent.getStringExtra(EXTRA_TRANSFER_URI);
                String mimeType = intent.getStringExtra(EXTRA_TRANSFER_MIMETYPE);
                Uri uri = Uri.parse(uriString);
                if (uri != null && uri.getScheme() == null) {
                    uri = Uri.fromFile(new File(uri.getPath()));
                }
                mTransferManager.finishTransfer(true, uri, mimeType);
            } else {
                mTransferManager.finishTransfer(false, null, null);
            }
        } else if (action.equals(ACTION_TRANSFER_PROGRESS)) {
            float progress = intent.getFloatExtra(EXTRA_TRANSFER_PROGRESS, 0.0f);
            mTransferManager.updateFileProgress(progress);
        } else if (action.equals(ACTION_HANDOVER_STARTED)) {
            int count = intent.getIntExtra(EXTRA_OBJECT_COUNT, 0);
            if (count > 0) {
                mTransferManager.setObjectCount(count);
            }
        }
    }
}
