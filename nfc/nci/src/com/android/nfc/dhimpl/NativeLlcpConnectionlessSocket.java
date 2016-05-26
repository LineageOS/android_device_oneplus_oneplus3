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
import com.android.nfc.LlcpPacket;

import java.io.IOException;

/**
 * LlcpConnectionlessSocket represents a LLCP Connectionless object to be used
 * in a connectionless communication
 */
public class NativeLlcpConnectionlessSocket implements DeviceHost.LlcpConnectionlessSocket {

    private int mHandle;
    private int mSap;
    private int mLinkMiu;

    public NativeLlcpConnectionlessSocket() { }

    public native boolean doSendTo(int sap, byte[] data);

    public native LlcpPacket doReceiveFrom(int linkMiu);

    public native boolean doClose();

    @Override
    public int getLinkMiu(){
        return mLinkMiu;
    }

    @Override
    public int getSap(){
        return mSap;
    }

    @Override
    public void send(int sap, byte[] data) throws IOException {
        if (!doSendTo(sap, data)) {
            throw new IOException();
        }
    }

    @Override
    public LlcpPacket receive() throws IOException {
        LlcpPacket packet = doReceiveFrom(mLinkMiu);
        if (packet == null) {
            throw new IOException();
        }
        return packet;
    }

    public int getHandle(){
        return mHandle;
    }

    @Override
    public void close() throws IOException {
        if (!doClose()) {
            throw new IOException();
        }
    }
}
