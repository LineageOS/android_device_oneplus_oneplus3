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

package com.android.nfc.ndefpush;

import com.android.nfc.DeviceHost.LlcpSocket;
import com.android.nfc.LlcpException;
import com.android.nfc.NfcService;

import android.nfc.NdefMessage;
import android.util.Log;

import java.io.IOException;
import java.util.Arrays;

/**
 * Simple client to push the local NDEF message to a server on the remote side of an
 * LLCP connection, using the Android Ndef Push Protocol.
 */
public class NdefPushClient {
    private static final String TAG = "NdefPushClient";
    private static final int MIU = 128;
    private static final boolean DBG = true;

    private static final int DISCONNECTED = 0;
    private static final int CONNECTING = 1;
    private static final int CONNECTED = 2;

    final Object mLock = new Object();
    // Variables below locked on mLock
    private int mState = DISCONNECTED;
    private LlcpSocket mSocket;

    public void connect() throws IOException {
        synchronized (mLock) {
            if (mState != DISCONNECTED) {
                throw new IOException("Socket still in use.");
            }
            mState = CONNECTING;
        }
        NfcService service = NfcService.getInstance();
        LlcpSocket sock = null;
        if (DBG) Log.d(TAG, "about to create socket");
        try {
            sock = service.createLlcpSocket(0, MIU, 1, 1024);
        } catch (LlcpException e) {
            synchronized (mLock) {
                mState = DISCONNECTED;
            }
            throw new IOException("Could not create socket.");
        }
        try {
            if (DBG) Log.d(TAG, "about to connect to service " + NdefPushServer.SERVICE_NAME);
            sock.connectToService(NdefPushServer.SERVICE_NAME);
        } catch (IOException e) {
            if (sock != null) {
                try {
                    sock.close();
                } catch (IOException e2) {

                }
            }
            synchronized (mLock) {
                mState = DISCONNECTED;
            }
            throw new IOException("Could not connect service.");
        }

        synchronized (mLock) {
            mSocket = sock;
            mState = CONNECTED;
        }
    }

    public boolean push(NdefMessage msg) {
        LlcpSocket sock = null;
        synchronized (mLock) {
            if (mState != CONNECTED) {
                Log.e(TAG, "Not connected to NPP.");
                return false;
            }
            sock = mSocket;
        }
        // We only handle a single immediate action for now
        NdefPushProtocol proto = new NdefPushProtocol(msg, NdefPushProtocol.ACTION_IMMEDIATE);
        byte[] buffer = proto.toByteArray();
        int offset = 0;
        int remoteMiu;

        try {
            remoteMiu = sock.getRemoteMiu();
            if (DBG) Log.d(TAG, "about to send a " + buffer.length + " byte message");
            while (offset < buffer.length) {
                int length = Math.min(buffer.length - offset, remoteMiu);
                byte[] tmpBuffer = Arrays.copyOfRange(buffer, offset, offset+length);
                if (DBG) Log.d(TAG, "about to send a " + length + " byte packet");
                sock.send(tmpBuffer);
                offset += length;
            }
            return true;
        } catch (IOException e) {
            Log.e(TAG, "couldn't send tag");
            if (DBG) Log.d(TAG, "exception:", e);
        } finally {
            if (sock != null) {
                try {
                    if (DBG) Log.d(TAG, "about to close");
                    sock.close();
                } catch (IOException e) {
                    // Ignore
                }
            }
        }
        return false;
    }

    public void close() {
        synchronized (mLock) {
            if (mSocket != null) {
                try {
                    if (DBG) Log.d(TAG, "About to close NPP socket.");
                    mSocket.close();
                } catch (IOException e) {
                    // Ignore
                }
                mSocket = null;
            }
            mState = DISCONNECTED;
        }
    }
}
