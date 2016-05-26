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

import com.android.nfc.DeviceHost.LlcpSocket;
import com.android.nfc.LlcpException;
import com.android.nfc.NfcService;

import android.nfc.NdefMessage;
import android.util.Log;

import java.io.IOException;

public final class SnepClient {
    private static final String TAG = "SnepClient";
    private static final boolean DBG = false;
    private static final int DEFAULT_ACCEPTABLE_LENGTH = 100*1024;
    private static final int DEFAULT_MIU = 128;
    private static final int DEFAULT_RWSIZE = 1;
    SnepMessenger mMessenger = null;
    private final Object mTransmissionLock = new Object();

    private final String mServiceName;
    private final int mPort;
    private int  mState = DISCONNECTED;
    private final int mAcceptableLength;
    private final int mFragmentLength;
    private final int mMiu;
    private final int mRwSize;

    private static final int DISCONNECTED = 0;
    private static final int CONNECTING = 1;
    private static final int CONNECTED = 2;

    public SnepClient() {
        mServiceName = SnepServer.DEFAULT_SERVICE_NAME;
        mPort = SnepServer.DEFAULT_PORT;
        mAcceptableLength = DEFAULT_ACCEPTABLE_LENGTH;
        mFragmentLength = -1;
        mMiu = DEFAULT_MIU;
        mRwSize = DEFAULT_RWSIZE;
    }

    public SnepClient(String serviceName) {
        mServiceName = serviceName;
        mPort = -1;
        mAcceptableLength = DEFAULT_ACCEPTABLE_LENGTH;
        mFragmentLength = -1;
        mMiu = DEFAULT_MIU;
        mRwSize = DEFAULT_RWSIZE;
    }

    public SnepClient(int miu, int rwSize) {
        mServiceName = SnepServer.DEFAULT_SERVICE_NAME;
        mPort = SnepServer.DEFAULT_PORT;
        mAcceptableLength = DEFAULT_ACCEPTABLE_LENGTH;
        mFragmentLength = -1;
        mMiu = miu;
        mRwSize = rwSize;
    }

    SnepClient(String serviceName, int fragmentLength) {
        mServiceName = serviceName;
        mPort = -1;
        mAcceptableLength = DEFAULT_ACCEPTABLE_LENGTH;
        mFragmentLength = fragmentLength;
        mMiu = DEFAULT_MIU;
        mRwSize = DEFAULT_RWSIZE;
    }

    SnepClient(String serviceName, int acceptableLength, int fragmentLength) {
        mServiceName = serviceName;
        mPort = -1;
        mAcceptableLength = acceptableLength;
        mFragmentLength = fragmentLength;
        mMiu = DEFAULT_MIU;
        mRwSize = DEFAULT_RWSIZE;
    }

    public void put(NdefMessage msg) throws IOException {
        SnepMessenger messenger;
        synchronized (this) {
            if (mState != CONNECTED) {
                throw new IOException("Socket not connected.");
            }
            messenger = mMessenger;
        }

        synchronized (mTransmissionLock) {
            try {
                messenger.sendMessage(SnepMessage.getPutRequest(msg));
                messenger.getMessage();
            } catch (SnepException e) {
                throw new IOException(e);
            }
        }
    }

    public SnepMessage get(NdefMessage msg) throws IOException {
        SnepMessenger messenger;
        synchronized (this) {
            if (mState != CONNECTED) {
                throw new IOException("Socket not connected.");
            }
            messenger = mMessenger;
        }

        synchronized (mTransmissionLock) {
            try {
                messenger.sendMessage(SnepMessage.getGetRequest(mAcceptableLength, msg));
                return messenger.getMessage();
            } catch (SnepException e) {
                throw new IOException(e);
            }
        }
    }

    public void connect() throws IOException {
        synchronized (this) {
            if (mState != DISCONNECTED) {
                throw new IOException("Socket already in use.");
            }
            mState = CONNECTING;
        }

        LlcpSocket socket = null;
        SnepMessenger messenger;
        try {
            if (DBG) Log.d(TAG, "about to create socket");
            // Connect to the snep server on the remote side
            socket = NfcService.getInstance().createLlcpSocket(0, mMiu, mRwSize, 1024);
            if (socket == null) {
                throw new IOException("Could not connect to socket.");
            }
            if (mPort == -1) {
                if (DBG) Log.d(TAG, "about to connect to service " + mServiceName);
                socket.connectToService(mServiceName);
            } else {
                if (DBG) Log.d(TAG, "about to connect to port " + mPort);
                socket.connectToSap(mPort);
            }
            int miu = socket.getRemoteMiu();
            int fragmentLength = (mFragmentLength == -1) ?  miu : Math.min(miu, mFragmentLength);
            messenger = new SnepMessenger(true, socket, fragmentLength);
        } catch (LlcpException e) {
            synchronized (this) {
                mState = DISCONNECTED;
            }
            throw new IOException("Could not connect to socket");
        } catch (IOException e) {
            if (socket != null) {
                try {
                    socket.close();
                } catch (IOException e2) {
                }
            }
            synchronized (this) {
                mState = DISCONNECTED;
            }
            throw new IOException("Failed to connect to socket");
        }

        synchronized (this) {
            mMessenger = messenger;
            mState = CONNECTED;
        }
    }

    public void close() {
        synchronized (this) {
            if (mMessenger != null) {
               try {
                   mMessenger.close();
               } catch (IOException e) {
                   // ignore
               } finally {
                   mMessenger = null;
                   mState = DISCONNECTED;
               }
            }
        }
    }
}
