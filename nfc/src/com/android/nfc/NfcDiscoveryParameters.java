/*
 * Copyright (C) 2014 The Android Open Source Project
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

/**
 * Parameters for enabling NFC tag discovery and polling,
 * and host card emulation.
 */
public final class NfcDiscoveryParameters {

    public static class Builder {

        private NfcDiscoveryParameters mParameters;

        private Builder() {
            mParameters = new NfcDiscoveryParameters();
        }

        public NfcDiscoveryParameters.Builder setTechMask(int techMask) {
            mParameters.mTechMask = techMask;
            return this;
        }

        public NfcDiscoveryParameters.Builder setEnableLowPowerDiscovery(boolean enable) {
            mParameters.mEnableLowPowerDiscovery = enable;
            return this;
        }

        public NfcDiscoveryParameters.Builder setEnableReaderMode(boolean enable) {
            mParameters.mEnableReaderMode = enable;

            if (enable) {
                mParameters.mEnableLowPowerDiscovery = false;
            }

            return this;
        }

        public NfcDiscoveryParameters.Builder setEnableHostRouting(boolean enable) {
            mParameters.mEnableHostRouting = enable;
            return this;
        }

        public NfcDiscoveryParameters.Builder setEnableP2p(boolean enable) {
            mParameters.mEnableP2p = enable;
            return this;
        }

        public NfcDiscoveryParameters build() {
            if (mParameters.mEnableReaderMode &&
                    (mParameters.mEnableLowPowerDiscovery || mParameters.mEnableP2p)) {
                throw new IllegalStateException("Can't enable LPTD/P2P and reader mode " +
                        "simultaneously");
            }
            return mParameters;
        }
    }

    static final int NFC_POLL_DEFAULT = -1;

    // NOTE: when adding a new field, don't forget to update equals() and toString() below
    private int mTechMask = 0;
    private boolean mEnableLowPowerDiscovery = true;
    private boolean mEnableReaderMode = false;
    private boolean mEnableHostRouting = false;
    private boolean mEnableP2p = false;

    public NfcDiscoveryParameters() {}

    public int getTechMask() {
        return mTechMask;
    }

    public boolean shouldEnableLowPowerDiscovery() {
        return mEnableLowPowerDiscovery;
    }

    public boolean shouldEnableReaderMode() {
        return mEnableReaderMode;
    }

    public boolean shouldEnableHostRouting() {
        return mEnableHostRouting;
    }

    public boolean shouldEnableDiscovery() {
        return mTechMask != 0 || mEnableHostRouting;
    }

    public boolean shouldEnableP2p() {
        return mEnableP2p;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == this) {
            return true;
        }

        if ((obj == null) || (obj.getClass() != this.getClass())) {
            return false;
        }
        NfcDiscoveryParameters params = (NfcDiscoveryParameters) obj;
        return mTechMask == params.mTechMask &&
                (mEnableLowPowerDiscovery == params.mEnableLowPowerDiscovery) &&
                (mEnableReaderMode == params.mEnableReaderMode) &&
                (mEnableHostRouting == params.mEnableHostRouting)
                && (mEnableP2p == params.mEnableP2p);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        if (mTechMask == NFC_POLL_DEFAULT) {
            sb.append("mTechMask: default\n");
        } else {
            sb.append("mTechMask: " + Integer.toString(mTechMask) + "\n");
        }
        sb.append("mEnableLPD: " + Boolean.toString(mEnableLowPowerDiscovery) + "\n");
        sb.append("mEnableReader: " + Boolean.toString(mEnableReaderMode) + "\n");
        sb.append("mEnableHostRouting: " + Boolean.toString(mEnableHostRouting) + "\n");
        sb.append("mEnableP2p: " + Boolean.toString(mEnableP2p));
        return sb.toString();
    }

    public static NfcDiscoveryParameters.Builder newBuilder() {
        return new Builder();
    }

    public static NfcDiscoveryParameters getDefaultInstance() {
        return new NfcDiscoveryParameters();
    }

    public static NfcDiscoveryParameters getNfcOffParameters() {
        return new NfcDiscoveryParameters();
    }
}
