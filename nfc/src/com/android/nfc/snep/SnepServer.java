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
package com.android.nfc.snep;

import com.android.nfc.DeviceHost.LlcpServerSocket;
import com.android.nfc.DeviceHost.LlcpSocket;
import com.android.nfc.LlcpException;
import com.android.nfc.NfcService;

import android.nfc.NdefMessage;
import android.nfc.NfcAdapter;
import android.util.Log;

import java.io.IOException;

/**
 * A simple server that accepts NDEF messages pushed to it over an LLCP connection. Those messages
 * are typically set on the client side by using {@link NfcAdapter#enableForegroundNdefPush}.
 */
public final class SnepServer {
    private static final String TAG = "SnepServer";
    private static final boolean DBG = false;
    private static final int DEFAULT_MIU = 248;
    private static final int DEFAULT_RW_SIZE = 1;

    public static final int DEFAULT_PORT = 4;

    public static final String DEFAULT_SERVICE_NAME = "urn:nfc:sn:snep";

    final Callback mCallback;
    final String mServiceName;
    final int mServiceSap;
    final int mFragmentLength;
    final int mMiu;
    final int mRwSize;

    /** Protected by 'this', null when stopped, non-null when running */
    ServerThread mServerThread = null;
    boolean mServerRunning = false;

    public interface Callback {
        public SnepMessage doPut(NdefMessage msg);
        public SnepMessage doGet(int acceptableLength, NdefMessage msg);
    }

    public SnepServer(Callback callback) {
        mCallback = callback;
        mServiceName = DEFAULT_SERVICE_NAME;
        mServiceSap = DEFAULT_PORT;
        mFragmentLength = -1;
        mMiu = DEFAULT_MIU;
        mRwSize = DEFAULT_RW_SIZE;
    }

    public SnepServer(String serviceName, int serviceSap, Callback callback) {
        mCallback = callback;
        mServiceName = serviceName;
        mServiceSap = serviceSap;
        mFragmentLength = -1;
        mMiu = DEFAULT_MIU;
        mRwSize = DEFAULT_RW_SIZE;
    }

    public SnepServer(Callback callback, int miu, int rwSize) {
        mCallback = callback;
        mServiceName = DEFAULT_SERVICE_NAME;
        mServiceSap = DEFAULT_PORT;
        mFragmentLength = -1;
        mMiu = miu;
        mRwSize = rwSize;
    }

    SnepServer(String serviceName, int serviceSap, int fragmentLength, Callback callback) {
        mCallback = callback;
        mServiceName = serviceName;
        mServiceSap = serviceSap;
        mFragmentLength = fragmentLength;
        mMiu = DEFAULT_MIU;
        mRwSize = DEFAULT_RW_SIZE;
    }

    /** Connection class, used to handle incoming connections */
    private class ConnectionThread extends Thread {
        private final LlcpSocket mSock;
        private final SnepMessenger mMessager;

        ConnectionThread(LlcpSocket socket, int fragmentLength) {
            super(TAG);
            mSock = socket;
            mMessager = new SnepMessenger(false, socket, fragmentLength);
        }

        @Override
        public void run() {
            if (DBG) Log.d(TAG, "starting connection thread");
            try {
                boolean running;
                synchronized (SnepServer.this) {
                    running = mServerRunning;
                }

                while (running) {
                    if (!handleRequest(mMessager, mCallback)) {
                        break;
                    }

                    synchronized (SnepServer.this) {
                        running = mServerRunning;
                    }
                }
            } catch (IOException e) {
                if (DBG) Log.e(TAG, "Closing from IOException");
            } finally {
                try {
                    if (DBG) Log.d(TAG, "about to close");
                    mSock.close();
                } catch (IOException e) {
                    // ignore
                }
            }

            if (DBG) Log.d(TAG, "finished connection thread");
        }
    }

    static boolean handleRequest(SnepMessenger messenger, Callback callback) throws IOException {
        SnepMessage request;
        try {
            request = messenger.getMessage();
        } catch (SnepException e) {
            if (DBG) Log.w(TAG, "Bad snep message", e);
            try {
                messenger.sendMessage(SnepMessage.getMessage(
                    SnepMessage.RESPONSE_BAD_REQUEST));
            } catch (IOException e2) {
                // Ignore
            }
            return false;
        }

        if (((request.getVersion() & 0xF0) >> 4) != SnepMessage.VERSION_MAJOR) {
            messenger.sendMessage(SnepMessage.getMessage(
                    SnepMessage.RESPONSE_UNSUPPORTED_VERSION));
        } else if((NfcService.mIsDtaMode)&&((request.getLength() > SnepMessage.MAL_IUT) ||
                (request.getLength() == SnepMessage.MAL))){
            if (DBG) Log.d(TAG, "Bad requested length");
            messenger.sendMessage(SnepMessage.getMessage(SnepMessage.RESPONSE_REJECT));
        } else if (request.getField() == SnepMessage.REQUEST_GET) {
            messenger.sendMessage(callback.doGet(request.getAcceptableLength(),
                    request.getNdefMessage()));
        } else if (request.getField() == SnepMessage.REQUEST_PUT) {
            if (DBG) Log.d(TAG, "putting message " + request.toString());
            messenger.sendMessage(callback.doPut(request.getNdefMessage()));
        } else {
            if (DBG) Log.d(TAG, "Unknown request (" + request.getField() +")");
            messenger.sendMessage(SnepMessage.getMessage(
                    SnepMessage.RESPONSE_BAD_REQUEST));
        }
        return true;
    }

    /** Server class, used to listen for incoming connection request */
    class ServerThread extends Thread {
        private boolean mThreadRunning = true;
        LlcpServerSocket mServerSocket;

        @Override
        public void run() {
            boolean threadRunning;
            synchronized (SnepServer.this) {
                threadRunning = mThreadRunning;
            }

            while (threadRunning) {
                if (DBG) Log.d(TAG, "about create LLCP service socket");
                try {
                    synchronized (SnepServer.this) {
                        mServerSocket = NfcService.getInstance().createLlcpServerSocket(mServiceSap,
                                mServiceName, mMiu, mRwSize, 1024);
                    }
                    if (mServerSocket == null) {
                        if (DBG) Log.d(TAG, "failed to create LLCP service socket");
                        return;
                    }
                    if (DBG) Log.d(TAG, "created LLCP service socket");
                    synchronized (SnepServer.this) {
                        threadRunning = mThreadRunning;
                    }

                    while (threadRunning) {
                        LlcpServerSocket serverSocket;
                        synchronized (SnepServer.this) {
                            serverSocket = mServerSocket;
                        }

                        if (serverSocket == null) {
                            if (DBG) Log.d(TAG, "Server socket shut down.");
                            return;
                        }
                        if (DBG) Log.d(TAG, "about to accept");
                        LlcpSocket communicationSocket = serverSocket.accept();
                        if (DBG) Log.d(TAG, "accept returned " + communicationSocket);
                        if (communicationSocket != null) {
                            int fragmentLength = (mFragmentLength == -1) ?
                                    mMiu : Math.min(mMiu, mFragmentLength);
                            new ConnectionThread(communicationSocket, fragmentLength).start();
                        }

                        synchronized (SnepServer.this) {
                            threadRunning = mThreadRunning;
                        }
                    }
                    if (DBG) Log.d(TAG, "stop running");
                } catch (LlcpException e) {
                    Log.e(TAG, "llcp error", e);
                } catch (IOException e) {
                    Log.e(TAG, "IO error", e);
                } finally {
                    synchronized (SnepServer.this) {
                        if (mServerSocket != null) {
                            if (DBG) Log.d(TAG, "about to close");
                            try {
                                mServerSocket.close();
                            } catch (IOException e) {
                                // ignore
                            }
                            mServerSocket = null;
                        }
                    }
                }

                synchronized (SnepServer.this) {
                    threadRunning = mThreadRunning;
                }
            }
        }

        public void shutdown() {
            synchronized (SnepServer.this) {
                mThreadRunning = false;
                if (mServerSocket != null) {
                    try {
                        mServerSocket.close();
                    } catch (IOException e) {
                        // ignore
                    }
                    mServerSocket = null;
                }
            }
        }
    }

    public void start() {
        synchronized (SnepServer.this) {
            if (DBG) Log.d(TAG, "start, thread = " + mServerThread);
            if (mServerThread == null) {
                if (DBG) Log.d(TAG, "starting new server thread");
                mServerThread = new ServerThread();
                mServerThread.start();
                mServerRunning = true;
            }
        }
    }

    public void stop() {
        synchronized (SnepServer.this) {
            if (DBG) Log.d(TAG, "stop, thread = " + mServerThread);
            if (mServerThread != null) {
                if (DBG) Log.d(TAG, "shuting down server thread");
                mServerThread.shutdown();
                mServerThread = null;
                mServerRunning = false;
            }
        }
    }
}
