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

package com.android.nfc.beam;

import android.bluetooth.BluetoothDevice;
import android.net.Uri;
import android.os.Parcel;
import android.os.Parcelable;

public class BeamTransferRecord implements Parcelable {

    public static int DATA_LINK_TYPE_BLUETOOTH = 0;

    public int id;
    public boolean remoteActivating;
    public Uri[] uris;
    public int dataLinkType;

    // Data link specific fields
    public BluetoothDevice remoteDevice;


    private BeamTransferRecord(BluetoothDevice remoteDevice,
                               boolean remoteActivating, Uri[] uris) {
        this.id = 0;
        this.remoteDevice = remoteDevice;
        this.remoteActivating = remoteActivating;
        this.uris = uris;

        this.dataLinkType = DATA_LINK_TYPE_BLUETOOTH;
    }

    public static final BeamTransferRecord forBluetoothDevice(
            BluetoothDevice remoteDevice, boolean remoteActivating,
            Uri[] uris) {
       return new BeamTransferRecord(remoteDevice, remoteActivating, uris);
    }

    public static final Parcelable.Creator<BeamTransferRecord> CREATOR
            = new Parcelable.Creator<BeamTransferRecord>() {
        public BeamTransferRecord createFromParcel(Parcel in) {
            int deviceType = in.readInt();

            if (deviceType != DATA_LINK_TYPE_BLUETOOTH) {
                // only support BLUETOOTH
                return null;
            }

           BluetoothDevice remoteDevice = in.readParcelable(getClass().getClassLoader());
            boolean remoteActivating = (in.readInt() == 1);
            int numUris = in.readInt();
            Uri[] uris = null;
            if (numUris > 0) {
                uris = new Uri[numUris];
                in.readTypedArray(uris, Uri.CREATOR);
            }

            return new BeamTransferRecord(remoteDevice,
                    remoteActivating, uris);

        }

        @Override
        public BeamTransferRecord[] newArray(int size) {
            return new BeamTransferRecord[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(dataLinkType);
        dest.writeParcelable(remoteDevice, 0);
        dest.writeInt(remoteActivating ? 1 : 0);
        dest.writeInt(uris != null ? uris.length : 0);
        if (uris != null && uris.length > 0) {
            dest.writeTypedArray(uris, 0);
        }
    }
}
