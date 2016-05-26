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

import com.android.nfc.MockLlcpSocket;

import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.test.AndroidTestCase;
import android.util.Log;

import java.io.IOException;

/**
 * Tests the SNEP cleint/server interfaces using a mock LLCP socket.
 */
public class SnepBasicTests extends AndroidTestCase {
    private static final String TAG = "snepBasicTests";
    private static final int MIU = 250;
    private static final int ACCEPTABLE_LENGTH = 2*1024;

    public void testGetSmallNdef() throws IOException {
        MockLlcpSocket clientSocket = new MockLlcpSocket();
        MockLlcpSocket serverSocket = new MockLlcpSocket();
        MockLlcpSocket.bind(clientSocket, serverSocket);

        final SnepMessenger client = new SnepMessenger(true, clientSocket, MIU);
        final SnepMessenger server = new SnepMessenger(false, serverSocket, MIU);

        new Thread() {
            @Override
            public void run() {
                try {
                    SnepServer.handleRequest(server, mCallback);
                } catch (Exception e) {
                    Log.e(TAG, "error getting message", e);
                }
            };
        }.start();

        SnepMessage response = null;
        try {
            client.sendMessage(SnepMessage.getGetRequest(ACCEPTABLE_LENGTH, getSmallNdef()));
            response = client.getMessage();
        } catch (SnepException e) {
            throw new IOException("Failed to retrieve SNEP message", e);
        }

        assertNotNull(response);
        assertEquals(SnepMessage.RESPONSE_SUCCESS, response.getField());
    }

    public void testGetLargeNdef() throws IOException {
        MockLlcpSocket clientSocket = new MockLlcpSocket();
        MockLlcpSocket serverSocket = new MockLlcpSocket();
        MockLlcpSocket.bind(clientSocket, serverSocket);

        final SnepMessenger client = new SnepMessenger(true, clientSocket, MIU);
        final SnepMessenger server = new SnepMessenger(false, serverSocket, MIU);

        new Thread() {
            @Override
            public void run() {
                try {
                    SnepServer.handleRequest(server, mCallback);
                } catch (Exception e) {
                    Log.e(TAG, "error getting message", e);
                }
            };
        }.start();

        SnepMessage response = null;
        try {
            client.sendMessage(SnepMessage.getGetRequest(ACCEPTABLE_LENGTH, getNdef(900)));
            response = client.getMessage();
        } catch (SnepException e) {
            throw new IOException("Failed to retrieve SNEP message", e);
        }

        assertNotNull(response);
        assertEquals(SnepMessage.RESPONSE_SUCCESS, response.getField());
    }

    public void testGetExcessiveNdef() throws IOException {
        MockLlcpSocket clientSocket = new MockLlcpSocket();
        MockLlcpSocket serverSocket = new MockLlcpSocket();
        MockLlcpSocket.bind(clientSocket, serverSocket);

        final SnepMessenger client = new SnepMessenger(true, clientSocket, MIU);
        final SnepMessenger server = new SnepMessenger(false, serverSocket, MIU);

        new Thread() {
            @Override
            public void run() {
                try {
                    SnepServer.handleRequest(server, mCallback);
                } catch (Exception e) {
                    Log.e(TAG, "error getting message", e);
                }
            };
        }.start();

        SnepMessage response = null;
        try {
            client.sendMessage(SnepMessage.getGetRequest(10, getSmallNdef()));
            response = client.getMessage();
        } catch (SnepException e) {
            throw new IOException("Failed to retrieve SNEP message", e);
        }

        assertNotNull(response);
        assertEquals(SnepMessage.RESPONSE_EXCESS_DATA, response.getField());
    }

    public void testPutSmallNdef() throws IOException {
        MockLlcpSocket clientSocket = new MockLlcpSocket();
        MockLlcpSocket serverSocket = new MockLlcpSocket();
        MockLlcpSocket.bind(clientSocket, serverSocket);

        final SnepMessenger client = new SnepMessenger(true, clientSocket, MIU);
        final SnepMessenger server = new SnepMessenger(false, serverSocket, MIU);

        new Thread() {
            @Override
            public void run() {
                try {
                    SnepServer.handleRequest(server, mCallback);
                } catch (Exception e) {
                    Log.e(TAG, "error getting message", e);
                }
            };
        }.start();

        SnepMessage response = null;
        try {
            client.sendMessage(SnepMessage.getPutRequest(getSmallNdef()));
            response = client.getMessage();
        } catch (SnepException e) {
            throw new IOException("Failed to retrieve SNEP message", e);
        }

        assertNotNull(response);
        assertEquals(SnepMessage.RESPONSE_SUCCESS, response.getField());
    }

    public void testPutLargeNdef() throws IOException {
        MockLlcpSocket clientSocket = new MockLlcpSocket();
        MockLlcpSocket serverSocket = new MockLlcpSocket();
        MockLlcpSocket.bind(clientSocket, serverSocket);

        final SnepMessenger client = new SnepMessenger(true, clientSocket, MIU);
        final SnepMessenger server = new SnepMessenger(false, serverSocket, MIU);

        new Thread() {
            @Override
            public void run() {
                try {
                    SnepServer.handleRequest(server, mCallback);
                } catch (Exception e) {
                    Log.e(TAG, "error getting message", e);
                }
            };
        }.start();

        SnepMessage response = null;
        try {
            client.sendMessage(SnepMessage.getPutRequest(getNdef(900)));
            response = client.getMessage();
        } catch (SnepException e) {
            throw new IOException("Failed to retrieve SNEP message", e);
        }

        assertNotNull(response);
        assertEquals(SnepMessage.RESPONSE_SUCCESS, response.getField());
    }

    public void testUnsupportedVersion() throws IOException {
        MockLlcpSocket clientSocket = new MockLlcpSocket();
        MockLlcpSocket serverSocket = new MockLlcpSocket();
        MockLlcpSocket.bind(clientSocket, serverSocket);

        final SnepMessenger client = new SnepMessenger(true, clientSocket, MIU);
        final SnepMessenger server = new SnepMessenger(false, serverSocket, MIU);

        new Thread() {
            @Override
            public void run() {
                try {
                    SnepServer.handleRequest(server, mCallback);
                } catch (Exception e) {
                    Log.e(TAG, "error getting message", e);
                }
            };
        }.start();

        SnepMessage response = null;
        try {
            NdefMessage ndef = getSmallNdef();
            SnepMessage request = new SnepMessage(
                    (byte)2, SnepMessage.REQUEST_PUT, ndef.toByteArray().length, 0, ndef);
            client.sendMessage(request);
            response = client.getMessage();
        } catch (SnepException e) {
            throw new IOException("Failed to retrieve SNEP message", e);
        }

        assertNotNull(response);
        assertEquals(SnepMessage.RESPONSE_UNSUPPORTED_VERSION, response.getField());
    }

    public void testDifferentMinorVersion() throws IOException {
        MockLlcpSocket clientSocket = new MockLlcpSocket();
        MockLlcpSocket serverSocket = new MockLlcpSocket();
        MockLlcpSocket.bind(clientSocket, serverSocket);

        final SnepMessenger client = new SnepMessenger(true, clientSocket, MIU);
        final SnepMessenger server = new SnepMessenger(false, serverSocket, MIU);

        new Thread() {
            @Override
            public void run() {
                try {
                    SnepServer.handleRequest(server, mCallback);
                } catch (Exception e) {
                    Log.e(TAG, "error getting message", e);
                }
            };
        }.start();

        byte version = (0xF0 & (SnepMessage.VERSION_MAJOR << 4)) |
                (0x0F & (SnepMessage.VERSION_MINOR + 1));
        SnepMessage response = null;
        try {
            NdefMessage ndef = getSmallNdef();
            SnepMessage request = new SnepMessage(
                    version, SnepMessage.REQUEST_PUT, ndef.toByteArray().length, 0, ndef);
            client.sendMessage(request);
            response = client.getMessage();
        } catch (SnepException e) {
            throw new IOException("Failed to retrieve SNEP message", e);
        }

        assertNotNull(response);
        assertEquals(SnepMessage.RESPONSE_SUCCESS, response.getField());
    }

    NdefMessage getSmallNdef() {
        NdefRecord rec = new NdefRecord(NdefRecord.TNF_ABSOLUTE_URI, NdefRecord.RTD_URI,
                new byte[0], "http://android.com".getBytes());
        return new NdefMessage(new NdefRecord[] { rec });
    }

    NdefMessage getNdef(int size) {
        StringBuffer string = new StringBuffer(size);
        for (int i = 0; i < size; i++) {
            string.append('A' + (i % 26));
        }
        NdefRecord rec = new NdefRecord(NdefRecord.TNF_MIME_MEDIA, "text/plain".getBytes(),
                new byte[0], string.toString().getBytes());
        return new NdefMessage(new NdefRecord[] { rec });
    }

    /**
     * A SNEP Server implementation that accepts PUT requests for all ndef
     * messages and responds to GET requests with acceptable length greater
     * than or equal to 1024.
     */
    final SnepServer.Callback mCallback = new SnepServer.Callback() {
        private static final int GET_LENGTH = 1024;

        @Override
        public SnepMessage doPut(NdefMessage msg) {
            return SnepMessage.getSuccessResponse(null);
        }

        @Override
        public SnepMessage doGet(int acceptableLength, NdefMessage msg) {
            if (GET_LENGTH <= acceptableLength) {
                return SnepMessage.getSuccessResponse(getSmallNdef());
            } else {
                return SnepMessage.getMessage(SnepMessage.RESPONSE_EXCESS_DATA);
            }
        }
    };
}