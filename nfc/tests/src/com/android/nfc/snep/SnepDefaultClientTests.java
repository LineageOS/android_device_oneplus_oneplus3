/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.nfc.snep;

import java.io.IOException;

import com.android.nfc.snep.SnepClient;
import com.android.nfc.snep.SnepMessage;
import com.android.nfc.snep.SnepServer;

import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.test.AndroidTestCase;
import android.util.Log;

import java.lang.StringBuffer;

/**
 * Tests connectivity to a default SNEP server, using a physical NFC device.
 */
public class SnepDefaultClientTests extends AndroidTestCase {
    private static final String TAG = "nfcTest";

    public void setUp() {
        Log.d(TAG, "Waiting for service to restart...");
        try {
            Thread.sleep(6000);
        } catch (InterruptedException e) {}
        Log.d(TAG, "Running test.");
    }

    public void testPutSmallToDefaultServer() throws IOException {
        SnepClient client = new SnepClient();
        client.connect();
        client.put(getSmallNdef());
        client.close();
    }

    public void testPutLargeToDefaultServer() throws IOException {
        SnepClient client = new SnepClient();
        client.connect();
        client.put(getLargeNdef());
        client.close();
    }

    public void testPutTwiceToDefaultServer() throws IOException {
        SnepClient client = new SnepClient();
        client.connect();
        client.put(getSmallNdef());
        client.put(getSmallNdef());
        client.close();
    }

    public void testGetFail() throws IOException {
        SnepClient client = new SnepClient();
        client.connect();
        SnepMessage response = client.get(getSmallNdef());
        assertEquals(SnepMessage.RESPONSE_NOT_IMPLEMENTED, response.getField());
    }

    private NdefMessage getSmallNdef() {
        NdefRecord rec = new NdefRecord(NdefRecord.TNF_ABSOLUTE_URI, NdefRecord.RTD_URI,
                new byte[0], "http://android.com".getBytes());
        return new NdefMessage(new NdefRecord[] { rec });
    }

    private NdefMessage getLargeNdef() {
        int size = 3000;
        StringBuffer string = new StringBuffer(size);
        for (int i = 0; i < size; i++) {
            string.append('A' + (i % 26));
        }
        NdefRecord rec = new NdefRecord(NdefRecord.TNF_MIME_MEDIA, "text/plain".getBytes(),
                new byte[0], string.toString().getBytes());
        return new NdefMessage(new NdefRecord[] { rec });
    }
}
