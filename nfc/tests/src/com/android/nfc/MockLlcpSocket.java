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

package com.android.nfc;

import android.util.Log;

import java.io.IOException;
import java.util.LinkedList;
import java.util.List;

public class MockLlcpSocket implements DeviceHost.LlcpSocket {
    private static final String TAG = "mockLlcpSocket";
    private MockLlcpSocket mPairedSocket;
    private List<byte[]> mReceivedPackets = new LinkedList<byte[]>();
    private boolean mClosed = false;

    @Override
    public void close() throws IOException {
        mClosed = true;
        mPairedSocket.mClosed = true;
    }

    @Override
    public void connectToSap(int sap) throws IOException {
        throw new UnsupportedOperationException("Use MockLlcpSocket.bind(client, server)");
    }

    @Override
    public void send(byte[] data) throws IOException {
        if (mClosed || mPairedSocket == null) {
            throw new IOException("Socket not connected");
        }
        synchronized (mPairedSocket.mReceivedPackets) {
            Log.d(TAG, "sending packet");
            mPairedSocket.mReceivedPackets.add(data);
            mPairedSocket.mReceivedPackets.notify();
        }
    }

    @Override
    public int receive(byte[] receiveBuffer) throws IOException {
        synchronized (mReceivedPackets) {
            while (!mClosed && mReceivedPackets.size() == 0) {
                try {
                    mReceivedPackets.wait(1000);
                } catch (InterruptedException e) {}
            }
            if (mClosed) {
                throw new IOException("Socket closed.");
            }
            byte[] arr = mReceivedPackets.remove(0);
            System.arraycopy(arr, 0, receiveBuffer, 0, arr.length);
            return arr.length;
        }
    }

    public static void bind(MockLlcpSocket client, MockLlcpSocket server) {
        client.mPairedSocket = server;
        server.mPairedSocket = client;
    }

    @Override
    public void connectToService(String serviceName) throws IOException {
        throw new UnsupportedOperationException();
    }

    @Override
    public int getRemoteMiu() {
        throw new UnsupportedOperationException();
    }

    @Override
    public int getRemoteRw() {
        throw new UnsupportedOperationException();
    }

    @Override
    public int getLocalSap() {
        throw new UnsupportedOperationException();
    }

    @Override
    public int getLocalMiu() {
        throw new UnsupportedOperationException();
    }

    @Override
    public int getLocalRw() {
        throw new UnsupportedOperationException();
    }
}