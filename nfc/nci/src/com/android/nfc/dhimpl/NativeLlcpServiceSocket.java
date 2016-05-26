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
import com.android.nfc.DeviceHost.LlcpSocket;

import java.io.IOException;

/**
 * LlcpServiceSocket represents a LLCP Service to be used in a
 * Connection-oriented communication
 */
public class NativeLlcpServiceSocket implements DeviceHost.LlcpServerSocket {
    private int mHandle;
    private int mLocalMiu;
    private int mLocalRw;
    private int mLocalLinearBufferLength;
    private int mSap;
    private String mServiceName;

    public NativeLlcpServiceSocket(){ }

    private native NativeLlcpSocket doAccept(int miu, int rw, int linearBufferLength);
    @Override
    public LlcpSocket accept() throws IOException {
        LlcpSocket socket = doAccept(mLocalMiu, mLocalRw, mLocalLinearBufferLength);
        if (socket == null) throw new IOException();
        return socket;
    }

    private native boolean doClose();
    @Override
    public void close() throws IOException {
        if (!doClose()) {
            throw new IOException();
        }
    }
}
