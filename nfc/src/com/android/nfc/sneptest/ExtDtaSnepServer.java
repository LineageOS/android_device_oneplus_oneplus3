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
package com.android.nfc.sneptest;

import java.io.IOException;

import android.content.Context;
import android.nfc.NdefMessage;
import android.util.Log;

import com.android.nfc.DtaServiceConnector;
import com.android.nfc.DeviceHost.LlcpServerSocket;
import com.android.nfc.DeviceHost.LlcpSocket;
import com.android.nfc.LlcpException;
import com.android.nfc.NfcService;
import com.android.nfc.snep.SnepException;
import com.android.nfc.snep.SnepMessage;
import com.android.nfc.snep.SnepMessenger;

public final class ExtDtaSnepServer
{
    private static final String TAG = "ExtDtaSnepServer";
    private static final boolean DBG = true;
    public static final int DEFAULT_PORT = 5;
    public static final String EXTENDED_SNEP_DTA_SERVICE_NAME = "urn:nfc:sn:sneptest";
    public static final String DEFAULT_SERVICE_NAME = EXTENDED_SNEP_DTA_SERVICE_NAME;

    final Callback mExtDtaSnepServerCallback;
    final String mDtaServiceName;
    final int mDtaServiceSap;
    final int mDtaFragmentLength;
    final int mDtaMiu;
    final int mDtaRwSize;
    public static Context mContext;
    public static int mTestCaseId;

    /** Protected by 'this', null when stopped, non-null when running */
    ServerThread mServerThread = null;
    boolean mServerRunning = false;
    static DtaServiceConnector dtaServiceConnector;

    public interface Callback
    {
        public SnepMessage doPut(NdefMessage msg);
        public SnepMessage doGet(int acceptableLength, NdefMessage msg);
    }

    // for NFC Forum SNEP DTA
    public ExtDtaSnepServer(String serviceName, int serviceSap, int miu, int rwSize, Callback callback,Context mContext,int testCaseId)
    {
        mExtDtaSnepServerCallback = callback;
        mDtaServiceName = serviceName;
        mDtaServiceSap = serviceSap;
        mDtaFragmentLength = -1; // to get remote MIU
        mDtaMiu = miu;
        mDtaRwSize = rwSize;
        mTestCaseId = testCaseId;
        dtaServiceConnector=new DtaServiceConnector(mContext);
        dtaServiceConnector.bindService();
    }

    /** Connection class, used to handle incoming connections */
    private class ConnectionThread extends Thread
    {
        private final LlcpSocket mSock;
        private final SnepMessenger mMessager;

        ConnectionThread(LlcpSocket socket, int fragmentLength)
        {
            super(TAG);
            mSock = socket;
            mMessager = new SnepMessenger(false, socket, fragmentLength);
        }

        @Override
        public void run() {
            if (DBG) Log.d(TAG, "starting connection thread");
            try {
                boolean running;
                synchronized (ExtDtaSnepServer.this) {
                    running = mServerRunning;
                }

                while (running) {
                    if (!handleRequest(mMessager, mExtDtaSnepServerCallback)) {
                        break;
                    }

                    synchronized (ExtDtaSnepServer.this) {
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

    static boolean handleRequest(SnepMessenger messenger, Callback callback) throws IOException
    {
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

        if (((request.getVersion() & 0xF0) >> 4) != SnepMessage.VERSION_MAJOR)
        {
            messenger.sendMessage(SnepMessage.getMessage(
                    SnepMessage.RESPONSE_UNSUPPORTED_VERSION));
        }
        else if((request.getLength() > SnepMessage.MAL_IUT) ||  //added for TC_S_ACC_BV_05_0&1 and TC_S_ACC_BV_06_0&1
                (request.getLength() == SnepMessage.MAL))
        {
            if (DBG) Log.d(TAG, "Bad requested length");
            messenger.sendMessage(SnepMessage.getMessage(SnepMessage.RESPONSE_REJECT));
        }
        else if (request.getField() == SnepMessage.REQUEST_GET)
        {
            if (DBG) Log.d(TAG, "getting message " + request.toString());
            messenger.sendMessage(callback.doGet(request.getAcceptableLength(),
                    request.getNdefMessage()));
            if(request.getNdefMessage()!=null)
            dtaServiceConnector.sendMessage(request.getNdefMessage().toString());
        }
        else if (request.getField() == SnepMessage.REQUEST_PUT)
        {
            if (DBG) Log.d(TAG, "putting message " + request.toString());
            messenger.sendMessage(callback.doPut(request.getNdefMessage()));
            if(request.getNdefMessage()!=null)
            dtaServiceConnector.sendMessage(request.getNdefMessage().toString());
        }
        else
        {
            if (DBG) Log.d(TAG, "Unknown request (" + request.getField() +")");
            messenger.sendMessage(SnepMessage.getMessage(
                    SnepMessage.RESPONSE_BAD_REQUEST));
        }
        return true;
    }

    /** Server class, used to listen for incoming connection request */
    class ServerThread extends Thread
    {
        private boolean mThreadRunning = true;
        LlcpServerSocket mServerSocket;

        @Override
        public void run()
        {
            boolean threadRunning;
            synchronized (ExtDtaSnepServer.this)
            {
                threadRunning = mThreadRunning;
            }

            while (threadRunning) {
                if (DBG) Log.d(TAG, "about create LLCP service socket");
                try {
                    synchronized (ExtDtaSnepServer.this) {
                        mServerSocket = NfcService.getInstance().createLlcpServerSocket(mDtaServiceSap,
                                mDtaServiceName, mDtaMiu, mDtaRwSize, 1024);
                    }
                    if (mServerSocket == null) {
                        if (DBG) Log.d(TAG, "failed to create LLCP service socket");
                        return;
                    }
                    if (DBG) Log.d(TAG, "created LLCP service socket");
                    synchronized (ExtDtaSnepServer.this) {
                        threadRunning = mThreadRunning;
                    }

                    while (threadRunning) {
                        LlcpServerSocket serverSocket;
                        synchronized (ExtDtaSnepServer.this) {
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
                            int miu = communicationSocket.getRemoteMiu();
                            int fragmentLength = (mDtaFragmentLength == -1) ?
                                    miu : Math.min(miu, mDtaFragmentLength);
                            new ConnectionThread(communicationSocket, fragmentLength).start();
                        }

                        synchronized (ExtDtaSnepServer.this) {
                            threadRunning = mThreadRunning;
                        }
                    }
                    if (DBG) Log.d(TAG, "stop running");
                } catch (LlcpException e) {
                    Log.e(TAG, "llcp error", e);
                } catch (IOException e) {
                    Log.e(TAG, "IO error", e);
                } finally {
                    synchronized (ExtDtaSnepServer.this) {
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

                synchronized (ExtDtaSnepServer.this) {
                    threadRunning = mThreadRunning;
                }
            }
        }

        public void shutdown()
        {
            synchronized (ExtDtaSnepServer.this) {
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

    public void start()
    {
        synchronized (ExtDtaSnepServer.this)
        {
            if (DBG) Log.d(TAG, "start, thread = " + mServerThread);
            if (mServerThread == null) {
                if (DBG) Log.d(TAG, "starting new server thread");
                mServerThread = new ServerThread();
                mServerThread.start();
                mServerRunning = true;
            }
        }
    }

    public void stop()
    {
        synchronized (ExtDtaSnepServer.this)
        {
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
