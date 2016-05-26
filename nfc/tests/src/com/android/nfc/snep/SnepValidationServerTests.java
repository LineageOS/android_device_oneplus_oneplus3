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
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

/**
 * Uses the Android testing instrumentation framework to run a listening SNEP
 * server.
 */
public class SnepValidationServerTests extends AndroidTestCase implements SnepServer.Callback {
    static final int SERVICE_SAP = 0x11;
    static final String SERVICE_NAME = "urn:nfc:xsn:nfc-forum.org:snep-validation";
    private static final String TAG = "nfcTest";

    private Map<ByteArrayWrapper, NdefMessage> mStoredNdef =
        new HashMap<ByteArrayWrapper, NdefMessage>();
    private static final ByteArrayWrapper DEFAULT_NDEF = new ByteArrayWrapper(new byte[] {});

    public void setUp() {
        Log.d(TAG, "Waiting for service to restart...");
        try {
            Thread.sleep(6000);
        } catch (InterruptedException e) {
        }
        Log.d(TAG, "Running test.");
    }

    public void testStartSnepServer() throws IOException {
        SnepServer server = new SnepServer(SERVICE_NAME, SERVICE_SAP, this);
        server.start();

        try {
            Thread.sleep(24 * 60 * 60 * 1000);
        } catch (InterruptedException e) {

        }
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

    static class ByteArrayWrapper {
        private final byte[] data;

        public ByteArrayWrapper(byte[] data) {
            if (data == null) {
                throw new NullPointerException();
            }
            this.data = data;
        }

        @Override
        public boolean equals(Object other) {
            if (!(other instanceof ByteArrayWrapper)) {
                return false;
            }
            return Arrays.equals(data, ((ByteArrayWrapper) other).data);
        }

        @Override
        public int hashCode() {
            return Arrays.hashCode(data);
        }
    }

    @Override
    public SnepMessage doPut(NdefMessage msg) {
        Log.d(TAG, "doPut()");
        NdefRecord record = msg.getRecords()[0];
        ByteArrayWrapper id = (record.getId().length > 0) ?
                new ByteArrayWrapper(record.getId()) : DEFAULT_NDEF;
        mStoredNdef.put(id, msg);
        return SnepMessage.getMessage(SnepMessage.RESPONSE_SUCCESS);
    }

    @Override
    public SnepMessage doGet(int acceptableLength, NdefMessage msg) {
        Log.d(TAG, "doGet()");
        NdefRecord record = msg.getRecords()[0];
        ByteArrayWrapper id = (record.getId().length > 0) ?
                new ByteArrayWrapper(record.getId()) : DEFAULT_NDEF;
        NdefMessage result = mStoredNdef.get(id);

        if (result == null) {
            return SnepMessage.getMessage(SnepMessage.RESPONSE_NOT_FOUND);
        }
        if (acceptableLength < result.toByteArray().length) {
            return SnepMessage.getMessage(SnepMessage.RESPONSE_EXCESS_DATA);
        }
        return SnepMessage.getSuccessResponse(result);
    }
}
