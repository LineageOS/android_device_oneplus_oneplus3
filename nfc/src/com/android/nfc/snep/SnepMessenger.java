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

import com.android.nfc.DeviceHost.LlcpSocket;
import com.android.nfc.NfcService;
import com.android.nfc.sneptest.DtaSnepClient;
import com.android.nfc.sneptest.ExtDtaSnepServer;

import android.nfc.FormatException;
import android.util.Log;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.util.Arrays;

public class SnepMessenger {
    private static final String TAG = "SnepMessenger";
    private static final boolean DBG = false;
    private static final int HEADER_LENGTH = 6;
    final LlcpSocket mSocket;
    final int mFragmentLength;
    final boolean mIsClient;

    public SnepMessenger(boolean isClient, LlcpSocket socket, int fragmentLength) {
        mSocket = socket;
        mFragmentLength = fragmentLength;
        mIsClient = isClient;
    }

    public void sendMessage(SnepMessage msg) throws IOException {
        byte[] buffer = msg.toByteArray();
        byte remoteContinue;
        if (mIsClient) {
            remoteContinue = SnepMessage.RESPONSE_CONTINUE;
        } else {
            remoteContinue = SnepMessage.REQUEST_CONTINUE;
        }
        if (DBG) Log.d(TAG, "about to send a " + buffer.length + " byte message");

        // Send first fragment
        int length = Math.min(buffer.length, mFragmentLength);
        byte[] tmpBuffer = Arrays.copyOfRange(buffer, 0, length);
        if (DBG) Log.d(TAG, "about to send a " + length + " byte fragment");
        mSocket.send(tmpBuffer);

        if (length == buffer.length) {
            return;
        }

        // Look for Continue or Reject from peer.
        int offset = length;
        byte[] responseBytes = new byte[HEADER_LENGTH];
        mSocket.receive(responseBytes);
        SnepMessage snepResponse;
        try {
            snepResponse = SnepMessage.fromByteArray(responseBytes);
        } catch (FormatException e) {
            throw new IOException("Invalid SNEP message", e);
        }

        if (DBG) Log.d(TAG, "Got response from first fragment: " + snepResponse.getField());
        if (snepResponse.getField() != remoteContinue) {
            throw new IOException("Invalid response from server (" +
                    snepResponse.getField() + ")");
        }
        // Look for wrong/invalid request or response from peer
       if(NfcService.mIsDtaMode) {
            if((mIsClient)&&(DtaSnepClient.mTestCaseId == 6)) {
                length = Math.min(buffer.length - offset, mFragmentLength);
                tmpBuffer = Arrays.copyOfRange(buffer, offset, offset + length);
                if (DBG) Log.d(TAG, "about to send a " + length + " byte fragment");
                mSocket.send(tmpBuffer);
                offset += length;

                mSocket.receive(responseBytes);

                try {
                    snepResponse = SnepMessage.fromByteArray(responseBytes);
                } catch (FormatException e) {
                    throw new IOException("Invalid SNEP message", e);
                }
                if (DBG) Log.d(TAG, "Got response from second fragment: " + snepResponse.getField());
                if (snepResponse.getField() == remoteContinue) {
                    close();
                    return;
                }
            }
        }

        // Send remaining fragments.
        while (offset < buffer.length) {
            length = Math.min(buffer.length - offset, mFragmentLength);
            tmpBuffer = Arrays.copyOfRange(buffer, offset, offset + length);
            if (DBG) Log.d(TAG, "about to send a " + length + " byte fragment");
            mSocket.send(tmpBuffer);

            if(NfcService.mIsDtaMode) {
                if((!mIsClient)&&(ExtDtaSnepServer.mTestCaseId == 0x01)){
                    mSocket.receive(responseBytes);
                    try {
                        snepResponse = SnepMessage.fromByteArray(responseBytes);
                    } catch (FormatException e) {
                        throw new IOException("Invalid SNEP message", e);
                    }
                    if (DBG) Log.d(TAG, "Got continue response after second fragment: and now disconnecting..." + snepResponse.getField());
                    if (snepResponse.getField() == remoteContinue) {
                        close();
                        return;
                    }
                }
            }

            offset += length;
        }
    }

    public SnepMessage getMessage() throws IOException, SnepException {
        ByteArrayOutputStream buffer = new ByteArrayOutputStream(mFragmentLength);
        byte[] partial = new byte[mFragmentLength];
        int size;
        int readSize = 0;
        byte requestVersion = 0;
        byte requestField = 0; // for DTA Mode
        int requestLength = 0;
        boolean doneReading = false;
        byte fieldContinue;
        byte fieldReject;
        if (mIsClient) {
            fieldContinue = SnepMessage.REQUEST_CONTINUE;
            fieldReject = SnepMessage.REQUEST_REJECT;
        } else {
            fieldContinue = SnepMessage.RESPONSE_CONTINUE;
            fieldReject = SnepMessage.RESPONSE_REJECT;
        }

        size = mSocket.receive(partial);
        if (DBG) Log.d(TAG, "read " + size + " bytes");
        if (size < 0) {
            try {
                mSocket.send(SnepMessage.getMessage(fieldReject).toByteArray());
            } catch (IOException e) {
                // Ignore
            }
            throw new IOException("Error reading SNEP message.");
        } else if (size < HEADER_LENGTH) {
            try {
                if((NfcService.mIsDtaMode)&&(mIsClient)){
                    if (DBG) Log.d(TAG, "Invalid header length");
                    close();
                } else {
                    mSocket.send(SnepMessage.getMessage(fieldReject).toByteArray());

                }
                mSocket.send(SnepMessage.getMessage(fieldReject).toByteArray());
            } catch (IOException e) {
                // Ignore
            }
            throw new IOException("Invalid fragment from sender.");
        } else {
            readSize = size - HEADER_LENGTH;
            buffer.write(partial, 0, size);
        }

        DataInputStream dataIn = new DataInputStream(new ByteArrayInputStream(partial));
        requestVersion = dataIn.readByte();
        requestField = dataIn.readByte();
        requestLength = dataIn.readInt();

        if (DBG) Log.d(TAG, "read " + readSize + " of " + requestLength);

        if (((requestVersion & 0xF0) >> 4) != SnepMessage.VERSION_MAJOR) {
            if(NfcService.mIsDtaMode) {
                sendMessage(SnepMessage.getMessage(SnepMessage.RESPONSE_UNSUPPORTED_VERSION));
                close();
            } else {
            if(NfcService.mIsDtaMode) {
                sendMessage(SnepMessage.getMessage(SnepMessage.RESPONSE_UNSUPPORTED_VERSION));
                close();
            } else {
                // Invalid protocol version; treat message as complete.
                return new SnepMessage(requestVersion, requestField, 0, 0, null);
            }
            }

        }

        if(NfcService.mIsDtaMode) {
            if((!mIsClient)&&((requestField == SnepMessage.RESPONSE_CONTINUE)||  // added for TC_S_BIT_B1_01_X
                              (requestField == SnepMessage.RESPONSE_SUCCESS) ||
                              (requestField == SnepMessage.RESPONSE_NOT_FOUND)))
            {
                if (DBG) Log.d(TAG, "errorneous response received, disconnecting client");
                close();
            }
            if((!mIsClient)&&((requestField == SnepMessage.REQUEST_RFU)))
            {
                if (DBG) Log.d(TAG, "unknown request received, disconnecting client");
                sendMessage(SnepMessage.getMessage(SnepMessage.RESPONSE_BAD_REQUEST));
                close();

            }
            if((mIsClient)&&((requestField == SnepMessage.REQUEST_PUT))) // added for TC_C_BIT_BI_01_0
            {
                if (DBG) Log.d(TAG, "errorneous PUT request received, disconnecting from server");
                    close();
            }
            if((mIsClient)&&(requestLength > SnepMessage.MAL_IUT)) // added for TC_C_GET_BV_03
            {
                if (DBG) Log.d(TAG, "responding reject");
                    return new SnepMessage(requestVersion, requestField, requestLength, 0, null);
            }
            if((!mIsClient)&&((requestLength > SnepMessage.MAL_IUT) || (requestLength == SnepMessage.MAL)))  //added for TC_S_ACC_BV_05_0&1 and TC_S_ACC_BV_06_0&1
            {
                if (DBG) Log.d(TAG, "responding reject");
                    return new SnepMessage(requestVersion, requestField, requestLength, 0, null);
            }
        }

        if (requestLength > readSize) {
            if (DBG) Log.d(TAG, "requesting continuation");
            mSocket.send(SnepMessage.getMessage(fieldContinue).toByteArray());
        } else {
            doneReading = true;
        }

        // Remaining fragments
        while (!doneReading) {
            try {
                size = mSocket.receive(partial);
                if (DBG) Log.d(TAG, "read " + size + " bytes");
                if (size < 0) {
                    try {
                        mSocket.send(SnepMessage.getMessage(fieldReject).toByteArray());
                    } catch (IOException e) {
                        // Ignore
                    }
                    throw new IOException();
                } else {
                    readSize += size;
                    buffer.write(partial, 0, size);
                    if (readSize == requestLength) {
                        doneReading = true;
                    }
                }
            } catch (IOException e) {
                try {
                    mSocket.send(SnepMessage.getMessage(fieldReject).toByteArray());
                } catch (IOException e2) {
                    // Ignore
                }
                throw e;
            }
        }

        // Build NDEF message set from the stream
        try {
            return SnepMessage.fromByteArray(buffer.toByteArray());
        } catch (FormatException e) {
            Log.e(TAG, "Badly formatted NDEF message, ignoring", e);
            throw new SnepException(e);
        }
    }

    public void close() throws IOException {
        mSocket.close();
    }
}
