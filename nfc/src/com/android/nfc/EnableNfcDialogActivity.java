/*
* Copyright (C) 2015 NXP Semiconductors
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

package com.android.nfc;

import java.util.ArrayList;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.ClipData;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.nfc.BeamShareData;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.NfcAdapter;
import android.os.Bundle;
import android.util.Log;
import android.webkit.URLUtil;
import android.widget.Button;
import android.content.DialogInterface;
import com.android.internal.app.AlertActivity;
import com.android.internal.app.AlertController;
import android.view.View.OnClickListener;
import android.nfc.NfcAdapter;
import com.nxp.nfc.NxpConstants;

import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.View.OnClickListener;

import com.android.internal.R;


public class EnableNfcDialogActivity extends AlertActivity implements DialogInterface.OnClickListener {
    static final String TAG ="EnableNfcDialogActivity";
    static final boolean DBG = false;

    Intent mLaunchIntent;
    Button mBt1, mBt2;
    NfcAdapter mNfcAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setTheme(R.style.Theme_DeviceDefault_Light_Dialog_Alert);
        super.onCreate(savedInstanceState);
        mNfcAdapter = NfcAdapter.getDefaultAdapter(this);

        AlertController.AlertParams ap = mAlertParams;

        ap.mMessage = getString(com.android.nfc.R.string.nfc_enabled_dialog);
        ap.mNegativeButtonText = getString(com.android.nfc.R.string.cancel);
        ap.mPositiveButtonText = getString(com.android.nfc.R.string.ok);
        ap.mPositiveButtonListener = this;
        ap.mNegativeButtonListener = this;
        setupAlert();
   }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        Intent enableNfc = new Intent();
        enableNfc.setAction(NxpConstants.ACTION_GSMA_ENABLE_SET_FLAG);
        if(which == -1) {
            mNfcAdapter.enable(); // POSITIVE
            enableNfc.putExtra("ENABLE_STATE", true);
        } else if(which == -2) {  //Nagitive
           enableNfc.putExtra("ENABLE_STATE", false);
        }
        sendBroadcast(enableNfc);
        finish();
    }
}
