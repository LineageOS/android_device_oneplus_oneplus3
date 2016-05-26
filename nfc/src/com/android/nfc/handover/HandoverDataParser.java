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

package com.android.nfc.handover;

import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.Random;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.Intent;
import android.nfc.FormatException;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.os.UserHandle;
import android.util.Log;

/**
 * Manages handover of NFC to other technologies.
 */
public class HandoverDataParser {
    private static final String TAG = "NfcHandover";
    private static final boolean DBG = false;

    private static final byte[] TYPE_BT_OOB = "application/vnd.bluetooth.ep.oob"
            .getBytes(Charset.forName("US_ASCII"));
    private static final byte[] TYPE_BLE_OOB = "application/vnd.bluetooth.le.oob"
            .getBytes(Charset.forName("US_ASCII"));

    private static final byte[] TYPE_NOKIA = "nokia.com:bt".getBytes(Charset.forName("US_ASCII"));

    private static final byte[] RTD_COLLISION_RESOLUTION = {0x63, 0x72}; // "cr";

    private static final int CARRIER_POWER_STATE_INACTIVE = 0;
    private static final int CARRIER_POWER_STATE_ACTIVE = 1;
    private static final int CARRIER_POWER_STATE_ACTIVATING = 2;
    private static final int CARRIER_POWER_STATE_UNKNOWN = 3;

    private static final int BT_HANDOVER_TYPE_MAC = 0x1B;
    private static final int BT_HANDOVER_TYPE_LE_ROLE = 0x1C;
    private static final int BT_HANDOVER_TYPE_LONG_LOCAL_NAME = 0x09;
    private static final int BT_HANDOVER_TYPE_SHORT_LOCAL_NAME = 0x08;
    public static final int BT_HANDOVER_LE_ROLE_CENTRAL_ONLY = 0x01;

    private final BluetoothAdapter mBluetoothAdapter;

    private final Object mLock = new Object();
    // Variables below synchronized on mLock

    private String mLocalBluetoothAddress;

    public static class BluetoothHandoverData {
        public boolean valid = false;
        public BluetoothDevice device;
        public String name;
        public boolean carrierActivating = false;
        public int transport = BluetoothDevice.TRANSPORT_AUTO;
    }

    public static class IncomingHandoverData {
        public final NdefMessage handoverSelect;
        public final BluetoothHandoverData handoverData;

        public IncomingHandoverData(NdefMessage handoverSelect,
                                    BluetoothHandoverData handoverData) {
            this.handoverSelect = handoverSelect;
            this.handoverData = handoverData;
        }
    }

    public HandoverDataParser() {
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    }

    static NdefRecord createCollisionRecord() {
        byte[] random = new byte[2];
        new Random().nextBytes(random);
        return new NdefRecord(NdefRecord.TNF_WELL_KNOWN, RTD_COLLISION_RESOLUTION, null, random);
    }

    NdefRecord createBluetoothAlternateCarrierRecord(boolean activating) {
        byte[] payload = new byte[4];
        payload[0] = (byte) (activating ? CARRIER_POWER_STATE_ACTIVATING :
            CARRIER_POWER_STATE_ACTIVE);  // Carrier Power State: Activating or active
        payload[1] = 1;   // length of carrier data reference
        payload[2] = 'b'; // carrier data reference: ID for Bluetooth OOB data record
        payload[3] = 0;  // Auxiliary data reference count
        return new NdefRecord(NdefRecord.TNF_WELL_KNOWN, NdefRecord.RTD_ALTERNATIVE_CARRIER, null,
                payload);
    }

    NdefRecord createBluetoothOobDataRecord() {
        byte[] payload = new byte[8];
       // Note: this field should be little-endian per the BTSSP spec
        // The Android 4.1 implementation used big-endian order here.
        // No single Android implementation has ever interpreted this
        // length field when parsing this record though.
        payload[0] = (byte) (payload.length & 0xFF);
        payload[1] = (byte) ((payload.length >> 8) & 0xFF);

        synchronized (mLock) {
            if (mLocalBluetoothAddress == null) {
                mLocalBluetoothAddress = mBluetoothAdapter.getAddress();
            }

            byte[] addressBytes = addressToReverseBytes(mLocalBluetoothAddress);
            System.arraycopy(addressBytes, 0, payload, 2, 6);
        }

        return new NdefRecord(NdefRecord.TNF_MIME_MEDIA, TYPE_BT_OOB, new byte[]{'b'}, payload);
    }

    public boolean isHandoverSupported() {
        return (mBluetoothAdapter != null);
    }

    public NdefMessage createHandoverRequestMessage() {
        if (mBluetoothAdapter == null) {
            return null;
        }

        NdefRecord[] dataRecords = new NdefRecord[] {
                createBluetoothOobDataRecord()
        };
        return new NdefMessage(
               createHandoverRequestRecord(),
                dataRecords);
   }

    NdefMessage createBluetoothHandoverSelectMessage(boolean activating) {
        return new NdefMessage(createHandoverSelectRecord(
                createBluetoothAlternateCarrierRecord(activating)),
                createBluetoothOobDataRecord());
    }

    NdefRecord createHandoverSelectRecord(NdefRecord alternateCarrier) {
        NdefMessage nestedMessage = new NdefMessage(alternateCarrier);
        byte[] nestedPayload = nestedMessage.toByteArray();

        ByteBuffer payload = ByteBuffer.allocate(nestedPayload.length + 1);
        payload.put((byte)0x12);  // connection handover v1.2
        payload.put(nestedPayload);

        byte[] payloadBytes = new byte[payload.position()];
        payload.position(0);
        payload.get(payloadBytes);
        return new NdefRecord(NdefRecord.TNF_WELL_KNOWN, NdefRecord.RTD_HANDOVER_SELECT, null,
                payloadBytes);
    }

    NdefRecord createHandoverRequestRecord() {
        NdefRecord[] messages = new NdefRecord[] {
                createBluetoothAlternateCarrierRecord(false)
        };

        NdefMessage nestedMessage = new NdefMessage(createCollisionRecord(), messages);

        byte[] nestedPayload = nestedMessage.toByteArray();

        ByteBuffer payload = ByteBuffer.allocate(nestedPayload.length + 1);
        payload.put((byte) 0x12);  // connection handover v1.2
        payload.put(nestedMessage.toByteArray());

        byte[] payloadBytes = new byte[payload.position()];
        payload.position(0);
        payload.get(payloadBytes);
        return new NdefRecord(NdefRecord.TNF_WELL_KNOWN, NdefRecord.RTD_HANDOVER_REQUEST, null,
                payloadBytes);
    }

    /**
     * Returns null if message is not a Handover Request,
     * returns the IncomingHandoverData (Hs + parsed data) if it is.
     */
    public IncomingHandoverData getIncomingHandoverData(NdefMessage handoverRequest) {
        if (handoverRequest == null) return null;
        if (mBluetoothAdapter == null) return null;

        if (DBG) Log.d(TAG, "getIncomingHandoverData():" + handoverRequest.toString());

        NdefRecord handoverRequestRecord = handoverRequest.getRecords()[0];
        if (handoverRequestRecord.getTnf() != NdefRecord.TNF_WELL_KNOWN) {
            return null;
        }

        if (!Arrays.equals(handoverRequestRecord.getType(), NdefRecord.RTD_HANDOVER_REQUEST)) {
            return null;
        }

        // we have a handover request, look for BT OOB record
        BluetoothHandoverData bluetoothData = null;
        for (NdefRecord dataRecord : handoverRequest.getRecords()) {
            if (dataRecord.getTnf() == NdefRecord.TNF_MIME_MEDIA) {
                if (Arrays.equals(dataRecord.getType(), TYPE_BT_OOB)) {
                    bluetoothData = parseBtOob(ByteBuffer.wrap(dataRecord.getPayload()));
                }
            }
        }

        NdefMessage hs = tryBluetoothHandoverRequest(bluetoothData);
        if (hs != null) {
            return new IncomingHandoverData(hs, bluetoothData);
        }

        return null;
    }

    public BluetoothHandoverData getOutgoingHandoverData(NdefMessage handoverSelect) {
        return parseBluetooth(handoverSelect);
    }

    private NdefMessage tryBluetoothHandoverRequest(BluetoothHandoverData bluetoothData) {
        NdefMessage selectMessage = null;
        if (bluetoothData != null) {
            // Note: there could be a race where we conclude
            // that Bluetooth is already enabled, and shortly
            // after the user turns it off. That will cause
            // the transfer to fail, but there's nothing
            // much we can do about it anyway. It shouldn't
            // be common for the user to be changing BT settings
            // while waiting to receive a picture.
            boolean bluetoothActivating = !mBluetoothAdapter.isEnabled();

            // return BT OOB record so they can perform handover
            selectMessage = (createBluetoothHandoverSelectMessage(bluetoothActivating));
            if (DBG) Log.d(TAG, "Waiting for incoming transfer, [" +
                    bluetoothData.device.getAddress() + "]->[" + mLocalBluetoothAddress + "]");
        }

        return selectMessage;
    }



    boolean isCarrierActivating(NdefRecord handoverRec, byte[] carrierId) {
        byte[] payload = handoverRec.getPayload();
        if (payload == null || payload.length <= 1) return false;
        // Skip version
        byte[] payloadNdef = new byte[payload.length - 1];
        System.arraycopy(payload, 1, payloadNdef, 0, payload.length - 1);
        NdefMessage msg;
        try {
            msg = new NdefMessage(payloadNdef);
        } catch (FormatException e) {
            return false;
        }

        for (NdefRecord alt : msg.getRecords()) {
            byte[] acPayload = alt.getPayload();
            if (acPayload != null) {
                ByteBuffer buf = ByteBuffer.wrap(acPayload);
                int cps = buf.get() & 0x03; // Carrier Power State is in lower 2 bits
                int carrierRefLength = buf.get() & 0xFF;
                if (carrierRefLength != carrierId.length) return false;

                byte[] carrierRefId = new byte[carrierRefLength];
                buf.get(carrierRefId);
                if (Arrays.equals(carrierRefId, carrierId)) {
                    // Found match, returning whether power state is activating
                    return (cps == CARRIER_POWER_STATE_ACTIVATING);
                }
            }
        }

        return true;
    }

    BluetoothHandoverData parseBluetoothHandoverSelect(NdefMessage m) {
        // TODO we could parse this a lot more strictly; right now
        // we just search for a BT OOB record, and try to cross-reference
        // the carrier state inside the 'hs' payload.
        for (NdefRecord oob : m.getRecords()) {
            if (oob.getTnf() == NdefRecord.TNF_MIME_MEDIA &&
                    Arrays.equals(oob.getType(), TYPE_BT_OOB)) {
                BluetoothHandoverData data = parseBtOob(ByteBuffer.wrap(oob.getPayload()));
                if (data != null && isCarrierActivating(m.getRecords()[0], oob.getId())) {
                    data.carrierActivating = true;
                }
                return data;
            }

            if (oob.getTnf() == NdefRecord.TNF_MIME_MEDIA &&
                    Arrays.equals(oob.getType(), TYPE_BLE_OOB)) {
               return parseBleOob(ByteBuffer.wrap(oob.getPayload()));
            }
       }

        return null;
    }

    public BluetoothHandoverData parseBluetooth(NdefMessage m) {
        NdefRecord r = m.getRecords()[0];
        short tnf = r.getTnf();
        byte[] type = r.getType();

        // Check for BT OOB record
        if (r.getTnf() == NdefRecord.TNF_MIME_MEDIA && Arrays.equals(r.getType(), TYPE_BT_OOB)) {
            return parseBtOob(ByteBuffer.wrap(r.getPayload()));
        }

        // Check for BLE OOB record
        if (r.getTnf() == NdefRecord.TNF_MIME_MEDIA && Arrays.equals(r.getType(), TYPE_BLE_OOB)) {
            return parseBleOob(ByteBuffer.wrap(r.getPayload()));
        }

       // Check for Handover Select, followed by a BT OOB record
        if (tnf == NdefRecord.TNF_WELL_KNOWN &&
                Arrays.equals(type, NdefRecord.RTD_HANDOVER_SELECT)) {
            return parseBluetoothHandoverSelect(m);
        }

        // Check for Nokia BT record, found on some Nokia BH-505 Headsets
        if (tnf == NdefRecord.TNF_EXTERNAL_TYPE && Arrays.equals(type, TYPE_NOKIA)) {
            return parseNokia(ByteBuffer.wrap(r.getPayload()));
        }

        return null;
    }

    BluetoothHandoverData parseNokia(ByteBuffer payload) {
        BluetoothHandoverData result = new BluetoothHandoverData();
        result.valid = false;

        try {
            payload.position(1);
            byte[] address = new byte[6];
            payload.get(address);
            result.device = mBluetoothAdapter.getRemoteDevice(address);
            result.valid = true;
            payload.position(14);
            int nameLength = payload.get();
            byte[] nameBytes = new byte[nameLength];
            payload.get(nameBytes);
            result.name = new String(nameBytes, Charset.forName("UTF-8"));
        } catch (IllegalArgumentException e) {
            Log.i(TAG, "nokia: invalid BT address");
        } catch (BufferUnderflowException e) {
            Log.i(TAG, "nokia: payload shorter than expected");
        }
        if (result.valid && result.name == null) result.name = "";
       return result;
    }

    BluetoothHandoverData parseBtOob(ByteBuffer payload) {
        BluetoothHandoverData result = new BluetoothHandoverData();
        result.valid = false;

        try {
            payload.position(2); // length
            byte[] address = parseMacFromBluetoothRecord(payload);
            result.device = mBluetoothAdapter.getRemoteDevice(address);
           result.valid = true;

            while (payload.remaining() > 0) {
                byte[] nameBytes;
                int len = payload.get();
                int type = payload.get();
                switch (type) {
                    case BT_HANDOVER_TYPE_SHORT_LOCAL_NAME:
                        nameBytes = new byte[len - 1];
                        payload.get(nameBytes);
                        result.name = new String(nameBytes, Charset.forName("UTF-8"));
                        break;
                    case BT_HANDOVER_TYPE_LONG_LOCAL_NAME:
                        if (result.name != null) break;  // prefer short name
                        nameBytes = new byte[len - 1];
                        payload.get(nameBytes);
                        result.name = new String(nameBytes, Charset.forName("UTF-8"));
                        break;
                    default:
                        payload.position(payload.position() + len - 1);
                        break;
                }
            }
        } catch (IllegalArgumentException e) {
            Log.i(TAG, "BT OOB: invalid BT address");
        } catch (BufferUnderflowException e) {
            Log.i(TAG, "BT OOB: payload shorter than expected");
        }
        if (result.valid && result.name == null) result.name = "";
        return result;
    }

    BluetoothHandoverData parseBleOob(ByteBuffer payload) {
        BluetoothHandoverData result = new BluetoothHandoverData();
        result.valid = false;
        result.transport = BluetoothDevice.TRANSPORT_LE;

        try {

            while (payload.remaining() > 0) {
                byte[] nameBytes;
                int len = payload.get();
                int type = payload.get();
               switch (type) {
                    case BT_HANDOVER_TYPE_MAC: // mac address
                        byte[] address = parseMacFromBluetoothRecord(payload);
                        payload.position(payload.position() + 1); // advance over random byte
                        result.device = mBluetoothAdapter.getRemoteDevice(address);
                        result.valid = true;
                        break;
                    case BT_HANDOVER_TYPE_LE_ROLE:
                        byte role = payload.get();
                        if (role == BT_HANDOVER_LE_ROLE_CENTRAL_ONLY) {
                            // only central role supported, can't pair
                            result.valid = false;
                            return result;
                        }
                        break;
                   case BT_HANDOVER_TYPE_LONG_LOCAL_NAME:
                        nameBytes = new byte[len - 1];
                        payload.get(nameBytes);
                        result.name = new String(nameBytes, Charset.forName("UTF-8"));
                        break;
                    default:
                        payload.position(payload.position() + len - 1);
                        break;
                }
            }
        } catch (IllegalArgumentException e) {
            Log.i(TAG, "BT OOB: invalid BT address");
        } catch (BufferUnderflowException e) {
            Log.i(TAG, "BT OOB: payload shorter than expected");
        }
        if (result.valid && result.name == null) result.name = "";
        return result;
   }

    private byte[] parseMacFromBluetoothRecord(ByteBuffer payload) {
        byte[] address = new byte[6];
        payload.get(address);
        // ByteBuffer.order(LITTLE_ENDIAN) doesn't work for
        // ByteBuffer.get(byte[]), so manually swap order
        for (int i = 0; i < 3; i++) {
           byte temp = address[i];
            address[i] = address[5 - i];
            address[5 - i] = temp;
        }
        return address;
    }

   static byte[] addressToReverseBytes(String address) {
        String[] split = address.split(":");
        byte[] result = new byte[split.length];

        for (int i = 0; i < split.length; i++) {
           // need to parse as int because parseByte() expects a signed byte
            result[split.length - 1 - i] = (byte)Integer.parseInt(split[i], 16);
        }

        return result;
    }
}
