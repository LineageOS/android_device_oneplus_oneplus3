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
 * Tests connectivity to a custom SNEP server, using a physical NFC device.
 */
public class SnepValidationClientTests extends AndroidTestCase {
    private static final String TAG = "nfcTest";

    private static final int FRAGMENT_LENGTH = 50;

    public static final String SERVICE_NAME = SnepValidationServerTests.SERVICE_NAME;

    public void setUp() {
        Log.d(TAG, "Waiting for service to restart...");
        try {
            Thread.sleep(8000);
        } catch (InterruptedException e) {
        }
        Log.d(TAG, "Running test.");
    }

    public void testNonFragmented() throws IOException {
        try {
            SnepClient client = getSnepClient();
            NdefMessage msg = getSmallNdef();
            Log.d(TAG, "Connecting to service " + SERVICE_NAME + "...");
            client.connect();
            Log.d(TAG, "Putting ndef message...");
            client.put(msg);

            Log.d(TAG, "Getting ndef message...");
            byte[] responseBytes = client.get(msg).getNdefMessage().toByteArray();
            int i = 0;
            byte[] msgBytes = msg.toByteArray();
            Log.d(TAG, "Done. Checking " + msgBytes.length + " bytes.");
            for (byte b : msgBytes) {
                assertEquals(b, responseBytes[i++]);
            }
            Log.d(TAG, "Closing client.");
            client.close();
        } catch (IOException e) {
            Log.d(TAG, "Test failed.", e);
            throw e;
        }
    }

    public void testFragmented() throws IOException {
        try {
            SnepClient client = getSnepClient();
            NdefMessage msg = getLargeNdef();
            Log.d(TAG, "Connecting to service " + SERVICE_NAME + "...");
            client.connect();
            Log.d(TAG, "Putting ndef message of size " + msg.toByteArray().length + "...");
            client.put(msg);

            Log.d(TAG, "Getting ndef message...");
            byte[] responseBytes = client.get(msg).getNdefMessage().toByteArray();
            int i = 0;
            byte[] msgBytes = msg.toByteArray();
            Log.d(TAG, "Done. Checking " + msgBytes.length + " bytes.");
            for (byte b : msgBytes) {
                assertEquals(b, responseBytes[i++]);
            }
            client.close();
        } catch (IOException e) {
            Log.d(TAG, "Error running fragmented", e);
            throw e;
        }
    }

    public void testMultipleNdef() throws IOException {
        try {
            SnepClient client = getSnepClient();
            Log.d(TAG, "Connecting to service " + SERVICE_NAME + "...");
            client.connect();

            NdefMessage msgA = getSmallNdef();
            NdefMessage msgB = getLargeNdef();

            Log.d(TAG, "Putting ndef message A...");
            client.put(msgA);
            Log.d(TAG, "Putting ndef message B...");
            client.put(msgB);

            byte[] responseBytes;
            byte[] msgBytes;
            int i;

            Log.d(TAG, "Getting ndef message A...");
            responseBytes = client.get(msgA).getNdefMessage().toByteArray();
            i = 0;
            msgBytes = msgA.toByteArray();
            Log.d(TAG, "Done. Checking " + msgBytes.length + " bytes.");
            for (byte b : msgBytes) {
                assertEquals(b, responseBytes[i++]);
            }

            Log.d(TAG, "Getting ndef message B...");
            responseBytes = client.get(msgB).getNdefMessage().toByteArray();
            i = 0;
            msgBytes = msgB.toByteArray();
            Log.d(TAG, "Done. Checking " + msgBytes.length + " bytes.");
            for (byte b : msgBytes) {
                assertEquals(b, responseBytes[i++]);
            }

            Log.d(TAG, "Closing client.");
            client.close();
        } catch (IOException e) {
            Log.d(TAG, "Test failed.", e);
            throw e;
        }
    }

    public void testUnavailable() throws IOException {
        try {
            SnepClient client = getSnepClient();
            NdefMessage msg = getSmallNdef();
            Log.d(TAG, "Connecting to service " + SERVICE_NAME + "...");
            client.connect();

            Log.d(TAG, "Getting ndef message...");
            SnepMessage response = client.get(msg);
            assertEquals(SnepMessage.RESPONSE_NOT_FOUND, response.getField());
            Log.d(TAG, "Closing client.");
            client.close();
        } catch (IOException e) {
            Log.d(TAG, "Test failed.", e);
            throw e;
        }
    }

    public void testUndeliverable() throws IOException {
        try {
            SnepClient client = new SnepClient(SERVICE_NAME, 100, FRAGMENT_LENGTH);
            NdefMessage msg = getLargeNdef();
            Log.d(TAG, "Connecting to service " + SERVICE_NAME + "...");
            client.connect();
            Log.d(TAG, "Putting ndef message of size " + msg.toByteArray().length + "...");
            client.put(msg);

            Log.d(TAG, "Getting ndef message...");
            SnepMessage response = client.get(msg);
            assertEquals(SnepMessage.RESPONSE_EXCESS_DATA, response.getField());
            client.close();
        } catch (IOException e) {
            Log.d(TAG, "Error running fragmented", e);
            throw e;
        }
    }

    private NdefMessage getSmallNdef() {
        NdefRecord rec = new NdefRecord(NdefRecord.TNF_WELL_KNOWN, NdefRecord.RTD_URI,
                new byte[] { 'A' }, "http://android.com".getBytes());
        return new NdefMessage(new NdefRecord[] { rec });
    }

    private NdefMessage getLargeNdef() {
        int size = 500;
        StringBuffer string = new StringBuffer(size);
        for (int i = 0; i < size; i++) {
            string.append('A' + (i % 26));
        }
        NdefRecord rec = new NdefRecord(NdefRecord.TNF_MIME_MEDIA, "text/plain".getBytes(),
                new byte[] { 'B' }, string.toString().getBytes());
        return new NdefMessage(new NdefRecord[] { rec });
    }

    private SnepClient getSnepClient() {
        return new SnepClient(SERVICE_NAME, FRAGMENT_LENGTH);
    }
}
