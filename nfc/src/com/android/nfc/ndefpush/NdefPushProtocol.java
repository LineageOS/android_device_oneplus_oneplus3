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

package com.android.nfc.ndefpush;

import android.util.Log;

import android.nfc.NdefMessage;
import android.nfc.FormatException;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;

/**
 * Implementation of the NDEF Push Protocol.
 */
public class NdefPushProtocol {
    public static final byte ACTION_IMMEDIATE = (byte) 0x01;
    public static final byte ACTION_BACKGROUND = (byte) 0x02;

    private static final String TAG = "NdefMessageSet";
    private static final byte VERSION = 1;

    private int mNumMessages;
    private byte[] mActions;
    private NdefMessage[] mMessages;

    public NdefPushProtocol(NdefMessage msg, byte action) {
        mNumMessages = 1;
        mActions = new byte[1];
        mActions[0] = action;
        mMessages = new NdefMessage[1];
        mMessages[0] = msg;
    }

    public NdefPushProtocol(byte[] actions, NdefMessage[] messages) {
        if (actions.length != messages.length || actions.length == 0) {
            throw new IllegalArgumentException(
                    "actions and messages must be the same size and non-empty");
        }

        // Keep a copy of these arrays
        int numMessages = actions.length;
        mActions = new byte[numMessages];
        System.arraycopy(actions, 0, mActions, 0, numMessages);
        mMessages = new NdefMessage[numMessages];
        System.arraycopy(messages, 0, mMessages, 0, numMessages);
        mNumMessages = numMessages;
    }

    public NdefPushProtocol(byte[] data) throws FormatException {
        ByteArrayInputStream buffer = new ByteArrayInputStream(data);
        DataInputStream input = new DataInputStream(buffer);

        // Check version of protocol
        byte version;
        try {
            version = input.readByte();
        } catch (java.io.IOException e) {
            Log.w(TAG, "Unable to read version");
            throw new FormatException("Unable to read version");
        }

        if(version != VERSION) {
            Log.w(TAG, "Got version " + version + ",  expected " + VERSION);
            throw new FormatException("Got version " + version + ",  expected " + VERSION);
        }

        // Read numMessages
        try {
            mNumMessages = input.readInt();
        } catch(java.io.IOException e) {
            Log.w(TAG, "Unable to read numMessages");
            throw new FormatException("Error while parsing NdefMessageSet");
        }
        if(mNumMessages == 0) {
            Log.w(TAG, "No NdefMessage inside NdefMessageSet packet");
            throw new FormatException("Error while parsing NdefMessageSet");
        }

        // Read actions and messages
        mActions = new byte[mNumMessages];
        mMessages = new NdefMessage[mNumMessages];
        for(int i = 0; i < mNumMessages; i++) {
            // Read action
            try {
                mActions[i] = input.readByte();
            } catch(java.io.IOException e) {
                Log.w(TAG, "Unable to read action for message " + i);
                throw new FormatException("Error while parsing NdefMessageSet");
            }
            // Read message length
            int length;
            try {
                length = input.readInt();
            } catch(java.io.IOException e) {
                Log.w(TAG, "Unable to read length for message " + i);
                throw new FormatException("Error while parsing NdefMessageSet");
            }
            byte[] bytes = new byte[length];
            // Read message
            int lengthRead;
            try {
                lengthRead = input.read(bytes);
            } catch(java.io.IOException e) {
                Log.w(TAG, "Unable to read bytes for message " + i);
                throw new FormatException("Error while parsing NdefMessageSet");
            }
            if(length != lengthRead) {
                Log.w(TAG, "Read " + lengthRead + " bytes but expected " +
                    length);
                throw new FormatException("Error while parsing NdefMessageSet");
            }
            // Create and store NdefMessage
            try {
                mMessages[i] = new NdefMessage(bytes);
            } catch(FormatException e) {
                throw e;
            }
        }
    }

    public NdefMessage getImmediate() {
        // Find and return the first immediate message
        for(int i = 0; i < mNumMessages; i++) {
            if(mActions[i] == ACTION_IMMEDIATE) {
                return mMessages[i];
            }
        }
        return null;
    }

    public byte[] toByteArray() {
        ByteArrayOutputStream buffer = new ByteArrayOutputStream(1024);
        DataOutputStream output = new DataOutputStream(buffer);

        try {
            output.writeByte(VERSION);
            output.writeInt(mNumMessages);
            for(int i = 0; i < mNumMessages; i++) {
                output.writeByte(mActions[i]);
                byte[] bytes = mMessages[i].toByteArray();
                output.writeInt(bytes.length);
                output.write(bytes);
            }
        } catch(java.io.IOException e) {
            return null;
        }

        return buffer.toByteArray();
    }
}
