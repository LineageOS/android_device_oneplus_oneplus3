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
package  com.android.nfc.dhimpl;



/**
 * Native interface to the Jcop Applet load applet functions
 *
 * {@hide}
 */
public class NativeNfcAla {
    public native byte[] doLsExecuteScript(String scriptIn, String scriptOut, byte[] data);
    public native byte[] doLsGetVersion();
    public native byte[] doLsGetStatus();
    public native byte[] doLsGetAppletStatus();
    public native int doGetLSConfigVersion();
    public native int doAppletLoadApplet(String choice, byte[] data);
    public native int GetAppletsList(String[] name);
    public native byte[] GetCertificateKey();
}
