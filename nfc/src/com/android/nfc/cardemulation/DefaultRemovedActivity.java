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

package com.android.nfc.cardemulation;

import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.provider.Settings;
import com.android.internal.R;

import com.android.internal.app.AlertActivity;
import com.android.internal.app.AlertController;

public class DefaultRemovedActivity extends AlertActivity implements
        DialogInterface.OnClickListener {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        setTheme(R.style.Theme_DeviceDefault_Light_Dialog_Alert);
        super.onCreate(savedInstanceState);

        AlertController.AlertParams ap = mAlertParams;

        ap.mMessage = getString(com.android.nfc.R.string.default_pay_app_removed);
        ap.mNegativeButtonText = getString(R.string.no);
        ap.mPositiveButtonText = getString(R.string.yes);
        ap.mPositiveButtonListener = this;
        setupAlert();
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        // Launch into Settings
        Intent intent = new Intent(Settings.ACTION_NFC_PAYMENT_SETTINGS);
        startActivity(intent);
    }
}
