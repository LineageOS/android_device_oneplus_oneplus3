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

package com.android.nfc.handover;

import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothDevice;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Resources;
import android.os.Bundle;

import com.android.nfc.R;

public class ConfirmConnectActivity extends Activity {
    BluetoothDevice mDevice;
    AlertDialog mAlert = null;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        AlertDialog.Builder builder = new AlertDialog.Builder(this,
                AlertDialog.THEME_DEVICE_DEFAULT_LIGHT);
        Intent launchIntent = getIntent();
        mDevice = launchIntent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
        if (mDevice == null) finish();
        Resources res = getResources();
        String deviceName = mDevice.getName() != null ? mDevice.getName() : "";
        String confirmString = String.format(res.getString(R.string.confirm_pairing), deviceName);
        builder.setMessage(confirmString)
               .setCancelable(false)
               .setPositiveButton(res.getString(R.string.pair_yes),
                       new DialogInterface.OnClickListener() {
                   public void onClick(DialogInterface dialog, int id) {
                        Intent allowIntent = new Intent(BluetoothPeripheralHandover.ACTION_ALLOW_CONNECT);
                        allowIntent.putExtra(BluetoothDevice.EXTRA_DEVICE, mDevice);
                        sendBroadcast(allowIntent);
                        ConfirmConnectActivity.this.mAlert = null;
                        ConfirmConnectActivity.this.finish();
                   }
               })
               .setNegativeButton(res.getString(R.string.pair_no),
                       new DialogInterface.OnClickListener() {
                   public void onClick(DialogInterface dialog, int id) {
                       Intent denyIntent = new Intent(BluetoothPeripheralHandover.ACTION_DENY_CONNECT);
                       denyIntent.putExtra(BluetoothDevice.EXTRA_DEVICE, mDevice);
                       sendBroadcast(denyIntent);
                       ConfirmConnectActivity.this.mAlert = null;
                       ConfirmConnectActivity.this.finish();
                   }
               });
        mAlert = builder.create();
        mAlert.show();
    }

    @Override
    protected void onStop() {
        super.onStop();
        if (mAlert != null) {
            mAlert.dismiss();
            Intent denyIntent = new Intent(BluetoothPeripheralHandover.ACTION_DENY_CONNECT);
            denyIntent.putExtra(BluetoothDevice.EXTRA_DEVICE, mDevice);
            sendBroadcast(denyIntent);
        }
    }
}
