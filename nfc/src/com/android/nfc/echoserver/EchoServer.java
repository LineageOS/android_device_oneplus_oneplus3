/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.nfc.echoserver;

import com.android.nfc.DeviceHost.LlcpConnectionlessSocket;
import com.android.nfc.LlcpException;
import com.android.nfc.DeviceHost.LlcpServerSocket;
import com.android.nfc.DeviceHost.LlcpSocket;
import com.android.nfc.LlcpPacket;
import com.android.nfc.NfcService;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

import java.io.IOException;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * EchoServer is an implementation of the echo server that is used in the
 * nfcpy LLCP test suite. Enabling the EchoServer allows to test Android
 * NFC devices against nfcpy.
 * It has two main features (which run simultaneously):
 * 1) A connection-based server, which has a receive buffer of two
 *    packets. Once a packet is received, a 2-second sleep is initiated.
 *    After these 2 seconds, all packets that are in the receive buffer
 *    are echoed back on the same connection. The connection-based server
 *    does not drop packets, but simply blocks if the queue is full.
 * 2) A connection-less mode, which has a receive buffer of two packets.
 *    On LLCP link activation, we try to receive data on a pre-determined
 *    connection-less SAP. Like the connection-based server, all data in
 *    the buffer is echoed back to the SAP from which the data originated
 *    after a sleep of two seconds.
 *    The main difference is that the connection-less SAP is supposed
 *    to drop packets when the buffer is full.
 *
 *    To use with nfcpy:
 *    - Adapt default_miu (see ECHO_MIU below)
 *    - llcp-test-client.py --mode=target --co-echo=17 --cl-echo=18 -t 1
 *
 *    Modify -t to execute the different tests.
 *
 */
public class EchoServer {
    static boolean DBG = true;

    static final int DEFAULT_CO_SAP = 0x11;
    static final int DEFAULT_CL_SAP = 0x12;

    // Link MIU
    static final int MIU = 128;

    static final String TAG = "EchoServer";
    static final String CONNECTION_SERVICE_NAME = "urn:nfc:sn:co-echo";
    static final String CONNECTIONLESS_SERVICE_NAME = "urn:nfc:sn:cl-echo";

    ServerThread mServerThread;
    ConnectionlessServerThread mConnectionlessServerThread;
    NfcService mService;

    public interface WriteCallback {
        public void write(byte[] data);
    }

    public EchoServer() {
        mService = NfcService.getInstance();
    }

    static class EchoMachine implements Handler.Callback {
        static final int QUEUE_SIZE = 2;
        static final int ECHO_DELAY_IN_MS = 2000;

        /**
         * ECHO_MIU must be set equal to default_miu in nfcpy.
         * The nfcpy echo server is expected to maintain the
         * packet boundaries and sizes of the requests - that is,
         * if the nfcpy client sends a service data unit of 48 bytes
         * in a packet, the echo packet should have a payload of
         * 48 bytes as well. The "problem" is that the current
         * Android LLCP implementation simply pushes all received data
         * in a single large buffer, causing us to loose the packet
         * boundaries, not knowing how much data to put in a single
         * response packet. The ECHO_MIU parameter determines exactly that.
         * We use ECHO_MIU=48 because of a bug in PN544, which does not respect
         * the target length reduction parameter of the p2p protocol.
         */
        static final int ECHO_MIU = 128;

        final BlockingQueue<byte[]> dataQueue;
        final Handler handler;
        final boolean dumpWhenFull;
        final WriteCallback callback;

        // shutdown can be modified from multiple threads, protected by this
        boolean shutdown = false;

        EchoMachine(WriteCallback callback, boolean dumpWhenFull) {
            this.callback = callback;
            this.dumpWhenFull = dumpWhenFull;
            dataQueue = new LinkedBlockingQueue<byte[]>(QUEUE_SIZE);
            handler = new Handler(this);
        }

        public void pushUnit(byte[] unit, int size) {
            if (dumpWhenFull && dataQueue.remainingCapacity() == 0) {
                if (DBG) Log.d(TAG, "Dumping data unit");
            } else {
                try {
                    // Split up the packet in ECHO_MIU size packets
                    int sizeLeft = size;
                    int offset = 0;
                    if (dataQueue.isEmpty()) {
                        // First message: start echo'ing in 2 seconds
                        handler.sendMessageDelayed(handler.obtainMessage(), ECHO_DELAY_IN_MS);
                    }

                    if (sizeLeft == 0) {
                        // Special case: also send a zero-sized data unit
                        dataQueue.put(new byte[] {});
                    }
                    while (sizeLeft > 0) {
                        int minSize = Math.min(size, ECHO_MIU);
                        byte[] data = new byte[minSize];
                        System.arraycopy(unit, offset, data, 0, minSize);
                        dataQueue.put(data);
                        sizeLeft -= minSize;
                        offset += minSize;
                    }
                } catch (InterruptedException e) {
                    // Ignore
                }
            }
        }

        /** Shuts down the EchoMachine. May block until callbacks
         *  in progress are completed.
         */
        public synchronized void shutdown() {
            dataQueue.clear();
            shutdown = true;
        }

        @Override
        public synchronized boolean handleMessage(Message msg) {
            if (shutdown) return true;
            while (!dataQueue.isEmpty()) {
                callback.write(dataQueue.remove());
            }
            return true;
        }
    }

    public class ServerThread extends Thread implements WriteCallback {
        final EchoMachine echoMachine;

        boolean running = true;
        LlcpServerSocket serverSocket;
        LlcpSocket clientSocket;

        public ServerThread() {
            super();
            echoMachine = new EchoMachine(this, false);
        }

        private void handleClient(LlcpSocket socket) {
            boolean connectionBroken = false;
            byte[] dataUnit = new byte[1024];

            // Get raw data from remote server
            while (!connectionBroken) {
                try {
                    int size = socket.receive(dataUnit);
                    if (DBG) Log.d(TAG, "read " + size + " bytes");
                    if (size < 0) {
                        connectionBroken = true;
                        break;
                    } else {
                        echoMachine.pushUnit(dataUnit, size);
                    }
                } catch (IOException e) {
                    // Connection broken
                    connectionBroken = true;
                    if (DBG) Log.d(TAG, "connection broken by IOException", e);
                }
            }
        }

        @Override
        public void run() {
            if (DBG) Log.d(TAG, "about create LLCP service socket");
            try {
                serverSocket = mService.createLlcpServerSocket(DEFAULT_CO_SAP,
                        CONNECTION_SERVICE_NAME, MIU, 1, 1024);
            } catch (LlcpException e) {
                return;
            }
            if (serverSocket == null) {
                if (DBG) Log.d(TAG, "failed to create LLCP service socket");
                return;
            }
            if (DBG) Log.d(TAG, "created LLCP service socket");

            while (running) {

                try {
                    if (DBG) Log.d(TAG, "about to accept");
                    clientSocket = serverSocket.accept();
                    if (DBG) Log.d(TAG, "accept returned " + clientSocket);
                    handleClient(clientSocket);
                } catch (LlcpException e) {
                    Log.e(TAG, "llcp error", e);
                    running = false;
                } catch (IOException e) {
                    Log.e(TAG, "IO error", e);
                    running = false;
                }
            }

            echoMachine.shutdown();

            try {
                clientSocket.close();
            } catch (IOException e) {
                // Ignore
            }
            clientSocket = null;

            try {
                serverSocket.close();
            } catch (IOException e) {
                // Ignore
            }
            serverSocket = null;
        }

        @Override
        public void write(byte[] data) {
            if (clientSocket != null) {
                try {
                    clientSocket.send(data);
                    Log.e(TAG, "Send success!");
                } catch (IOException e) {
                    Log.e(TAG, "Send failed.");
                }
            }
        }

        public void shutdown() {
            running = false;
            if (serverSocket != null) {
                try {
                    serverSocket.close();
                } catch (IOException e) {
                    // ignore
                }
                serverSocket = null;
            }
        }
    }

    public class ConnectionlessServerThread extends Thread implements WriteCallback {
        final EchoMachine echoMachine;

        LlcpConnectionlessSocket socket;
        int mRemoteSap;
        boolean mRunning = true;

        public ConnectionlessServerThread() {
            super();
            echoMachine = new EchoMachine(this, true);
        }

        @Override
        public void run() {
            boolean connectionBroken = false;
            LlcpPacket packet;
            if (DBG) Log.d(TAG, "about create LLCP connectionless socket");
            try {
                socket = mService.createLlcpConnectionLessSocket(
                        DEFAULT_CL_SAP, CONNECTIONLESS_SERVICE_NAME);
                if (socket == null) {
                    if (DBG) Log.d(TAG, "failed to create LLCP connectionless socket");
                    return;
                }

                while (mRunning && !connectionBroken) {
                    try {
                        packet = socket.receive();
                        if (packet == null || packet.getDataBuffer() == null) {
                            break;
                        }
                        byte[] dataUnit = packet.getDataBuffer();
                        int size = dataUnit.length;

                        if (DBG) Log.d(TAG, "read " + packet.getDataBuffer().length + " bytes");
                        if (size < 0) {
                            connectionBroken = true;
                            break;
                        } else {
                            mRemoteSap = packet.getRemoteSap();
                            echoMachine.pushUnit(dataUnit, size);
                        }
                    } catch (IOException e) {
                        // Connection broken
                        connectionBroken = true;
                        if (DBG) Log.d(TAG, "connection broken by IOException", e);
                    }
                }
            } catch (LlcpException e) {
                Log.e(TAG, "llcp error", e);
            } finally {
                echoMachine.shutdown();

                if (socket != null) {
                    try {
                        socket.close();
                    } catch (IOException e) {
                    }
                }
            }

        }

        public void shutdown() {
            mRunning = false;
        }

        @Override
        public void write(byte[] data) {
            try {
                socket.send(mRemoteSap, data);
            } catch (IOException e) {
                if (DBG) Log.d(TAG, "Error writing data.");
            }
        }
    }

    public void onLlcpActivated() {
        synchronized (this) {
            // Connectionless server can only be started once the link is up
            // - otherwise, all calls to receive() on the connectionless socket
            // will fail immediately.
            if (mConnectionlessServerThread == null) {
                mConnectionlessServerThread = new ConnectionlessServerThread();
                mConnectionlessServerThread.start();
            }
        }
    }

    public void onLlcpDeactivated() {
        synchronized (this) {
            if (mConnectionlessServerThread != null) {
                mConnectionlessServerThread.shutdown();
                mConnectionlessServerThread = null;
            }
        }
    }

    /**
     *  Needs to be called on the UI thread
     */
    public void start() {
        synchronized (this) {
            if (mServerThread == null) {
                mServerThread = new ServerThread();
                mServerThread.start();
            }
        }

    }

    public void stop() {
        synchronized (this) {
            if (mServerThread != null) {
                mServerThread.shutdown();
                mServerThread = null;
            }
        }
    }
}
