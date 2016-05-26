/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.nfc.dhimpl;

import com.android.nfc.DeviceHost;

import java.io.IOException;

/**
 * LlcpClientSocket represents a LLCP Connection-Oriented client to be used in a
 * connection-oriented communication
 */
public class NativeLlcpSocket implements DeviceHost.LlcpSocket {
    private int mHandle;
    private int mSap;
    private int mLocalMiu;
    private int mLocalRw;

    public NativeLlcpSocket(){ }

    private native boolean doConnect(int nSap);
    @Override
    public void connectToSap(int sap) throws IOException {
        if (!doConnect(sap)) {
            throw new IOException();
        }
    }

    private native boolean doConnectBy(String sn);
    @Override
    public void connectToService(String serviceName) throws IOException {
        if (!doConnectBy(serviceName)) {
            throw new IOException();
        }
    }

    private native boolean doClose();
    @Override
    public void close() throws IOException {
        if (!doClose()) {
            throw new IOException();
        }
    }

    private native boolean doSend(byte[] data);
    @Override
    public void send(byte[] data) throws IOException {
        if (!doSend(data)) {
            throw new IOException();
        }
    }

    private native int doReceive(byte[] recvBuff);
    @Override
    public int receive(byte[] recvBuff) throws IOException {
        int receiveLength = doReceive(recvBuff);
        if (receiveLength == -1) {
            throw new IOException();
        }
        return receiveLength;
    }

    private native int doGetRemoteSocketMiu();
    @Override
    public int getRemoteMiu() { return doGetRemoteSocketMiu(); }

    private native int doGetRemoteSocketRw();
    @Override
    public int getRemoteRw() { return doGetRemoteSocketRw(); }

    @Override
    public int getLocalSap(){
        return mSap;
    }

    @Override
    public int getLocalMiu(){
        return mLocalMiu;
    }

    @Override
    public int getLocalRw(){
        return mLocalRw;
    }
}
