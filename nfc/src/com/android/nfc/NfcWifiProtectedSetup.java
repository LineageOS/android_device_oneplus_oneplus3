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
package com.android.nfc;


import android.content.Context;
import android.content.Intent;
import android.net.wifi.WifiConfiguration;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.tech.Ndef;
import android.os.UserHandle;
import android.os.UserManager;
import android.util.Log;

import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;

import java.util.Arrays;
import java.util.BitSet;

public final class NfcWifiProtectedSetup {

    public static final String NFC_TOKEN_MIME_TYPE = "application/vnd.wfa.wsc";

    public static final String EXTRA_WIFI_CONFIG = "com.android.nfc.WIFI_CONFIG_EXTRA";

    /*
     * ID into configuration record for SSID and Network Key in hex.
     * Obtained from WFA Wifi Simple Configuration Technical Specification v2.0.2.1.
     */
    private static final short CREDENTIAL_FIELD_ID = 0x100E;
    private static final short SSID_FIELD_ID = 0x1045;
    private static final short NETWORK_KEY_FIELD_ID = 0x1027;
    private static final short AUTH_TYPE_FIELD_ID = 0x1003;

    private static final short AUTH_TYPE_EXPECTED_SIZE = 2;

    private static final short AUTH_TYPE_OPEN = 0;
    private static final short AUTH_TYPE_WPA_PSK = 0x0002;
    private static final short AUTH_TYPE_WPA_EAP =  0x0008;
    private static final short AUTH_TYPE_WPA2_EAP = 0x0010;
    private static final short AUTH_TYPE_WPA2_PSK = 0x0020;

    private static final int MAX_NETWORK_KEY_SIZE_BYTES = 64;

    private NfcWifiProtectedSetup() {}

    public static boolean tryNfcWifiSetup(Ndef ndef, Context context) {

        if (ndef == null || context == null) {
            return false;
        }

        NdefMessage cachedNdefMessage = ndef.getCachedNdefMessage();
        if (cachedNdefMessage == null) {
            return false;
        }

        final WifiConfiguration wifiConfiguration;
        try {
            wifiConfiguration = parse(cachedNdefMessage);
        } catch (BufferUnderflowException e) {
            // malformed payload
            return false;
        }

        if (wifiConfiguration != null &&!UserManager.get(context).hasUserRestriction(
                UserManager.DISALLOW_CONFIG_WIFI, UserHandle.CURRENT)) {
            Intent configureNetworkIntent = new Intent()
                    .putExtra(EXTRA_WIFI_CONFIG, wifiConfiguration)
                    .setClass(context, ConfirmConnectToWifiNetworkActivity.class)
                    .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);

            context.startActivityAsUser(configureNetworkIntent, UserHandle.CURRENT);
            return true;
        }

        return false;
    }

    private static WifiConfiguration parse(NdefMessage message) {
        NdefRecord[] records = message.getRecords();

        for (NdefRecord record : records) {
            if (new String(record.getType()).equals(NFC_TOKEN_MIME_TYPE)) {
                ByteBuffer payload = ByteBuffer.wrap(record.getPayload());
                while (payload.hasRemaining()) {
                    short fieldId = payload.getShort();
                    short fieldSize = payload.getShort();
                    if (fieldId == CREDENTIAL_FIELD_ID) {
                        return parseCredential(payload, fieldSize);

                    }
                }
            }
        }

        return null;
    }

    private static WifiConfiguration parseCredential(ByteBuffer payload, short size) {
        int startPosition = payload.position();
        WifiConfiguration result = new WifiConfiguration();
        while (payload.position() < startPosition + size) {
            short fieldId = payload.getShort();
            short fieldSize = payload.getShort();

            // sanity check
            if (payload.position() + fieldSize > startPosition + size) {
                return null;
            }

            switch (fieldId) {
                case SSID_FIELD_ID:
                    byte[] ssid = new byte[fieldSize];
                    payload.get(ssid);
                    result.SSID = "\"" + new String(ssid) + "\"";
                    break;
                case NETWORK_KEY_FIELD_ID:
                    if (fieldSize > MAX_NETWORK_KEY_SIZE_BYTES) {
                        return null;
                    }
                    byte[] networkKey = new byte[fieldSize];
                    payload.get(networkKey);
                    result.preSharedKey = "\"" + new String(networkKey) + "\"";
                    break;
                case AUTH_TYPE_FIELD_ID:
                    if (fieldSize != AUTH_TYPE_EXPECTED_SIZE) {
                        // corrupt data
                        return null;
                    }

                    short authType = payload.getShort();
                    populateAllowedKeyManagement(result.allowedKeyManagement, authType);
                    break;
                default:
                    // unknown / unparsed tag
                    payload.position(payload.position() + fieldSize);
                    break;
            }
        }

        if (result.preSharedKey != null && result.SSID != null) {
            return result;
        }

        return null;
    }

    private static void populateAllowedKeyManagement(BitSet allowedKeyManagement, short authType) {
        if (authType == AUTH_TYPE_WPA_PSK || authType == AUTH_TYPE_WPA2_PSK) {
            allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_PSK);
        } else if (authType == AUTH_TYPE_WPA_EAP || authType == AUTH_TYPE_WPA2_EAP) {
            allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_EAP);
        } else if (authType == AUTH_TYPE_OPEN) {
            allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);
        }
    }
}
