/*
 * Copyright (C) 2010 The Android Open Source Project
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
/******************************************************************************
 *
 *  The original Work has been changed by NXP Semiconductors.
 *
 *  Copyright (C) 2015 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

package com.android.nfc.dhimpl;

import android.content.Context;
import android.content.SharedPreferences;


/**
 * Native interface to the NFC Secure Element functions
 *
 * {@hide}
 */
public class NativeNfcSecureElement {

    static final String PREF_SE_WIRED = "se_wired";

    private final Context mContext;

    SharedPreferences mPrefs;
    SharedPreferences.Editor mPrefsEditor;

    public NativeNfcSecureElement(Context context) {
        mContext = context;

        mPrefs = mContext.getSharedPreferences(NativeNfcManager.PREF, Context.MODE_PRIVATE);
        mPrefsEditor = mPrefs.edit();
    }

    private native int doNativeOpenSecureElementConnection();

    public int doOpenSecureElementConnection() {
        mPrefsEditor.putBoolean(PREF_SE_WIRED, true);
        mPrefsEditor.apply();

        return doNativeOpenSecureElementConnection();
    }

    private native boolean doNativeDisconnectSecureElementConnection(int handle);

    public boolean doDisconnect(int handle) {
        mPrefsEditor.putBoolean(PREF_SE_WIRED, false);
        mPrefsEditor.apply();

        return doNativeDisconnectSecureElementConnection(handle);
    }

    //TODO: Just stub for compilation
    public boolean doReset(int handle) {
        //return false;
        return doNativeResetSecureElement(handle);
    }


    //TODO: Just stub for compilation
    public byte[] doGetAtr (int handle) {

        return doNativeGetAtr(handle);
    }

    private native byte[] doNativeGetAtr(int handle);
    private native boolean doNativeResetSecureElement(int handle);
    public native byte[] doTransceive(int handle, byte[] data);

    public native int[] doGetTechList(int handle);

    public native byte [] doGetUid(int handle);
}
