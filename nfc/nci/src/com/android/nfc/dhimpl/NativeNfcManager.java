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
package com.android.nfc.dhimpl;

import android.annotation.SdkConstant;
import android.annotation.SdkConstant.SdkConstantType;
import android.content.Context;
import android.content.SharedPreferences;
import android.nfc.ErrorCodes;
import android.nfc.tech.Ndef;
import android.nfc.tech.TagTechnology;
import android.util.Log;
import java.io.File;

import java.util.HashMap;
import java.util.Map;
import com.android.nfc.DeviceHost;
import com.android.nfc.LlcpException;
import com.android.nfc.NfcDiscoveryParameters;

/**
 * Native interface to the NFC Manager functions
 */
public class NativeNfcManager implements DeviceHost {
    private static final String TAG = "NativeNfcManager";
    private static final String PREF_FIRMWARE_MODTIME = "firmware_modtime";
    private static final long FIRMWARE_MODTIME_DEFAULT = -1;
    static final String PREF = "NciDeviceHost";

    static final int DEFAULT_LLCP_MIU = 1980;
    static final int DEFAULT_LLCP_RWSIZE = 2;
    static final int PN547C2_ID = 1;
    static final int PN65T_ID = 2;
    static final int PN548AD_ID = 3;
    static final int PN66T_ID = 4;

    static final String DRIVER_NAME = "android-nci";

    private static final byte[][] EE_WIPE_APDUS = {};

    static {
        System.loadLibrary("nxp_nfc_nci_jni");
    }

    @SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)
    public static final String INTERNAL_TARGET_DESELECTED_ACTION = "com.android.nfc.action.INTERNAL_TARGET_DESELECTED";

    /* Native structure */
    private long mNative;

    private final DeviceHostListener mListener;
    private final Context mContext;
    private Map<String, Integer> mNfcid2ToHandle;
    public NativeNfcManager(Context context, DeviceHostListener listener) {
        mListener = listener;
        initializeNativeStructure();
        mContext = context;
        mNfcid2ToHandle = new HashMap<String, Integer>();
    }

    public native boolean initializeNativeStructure();

    private native boolean doDownload();
    @Override
    public boolean download() {
        return doDownload();
    }

    public native int doGetLastError();

    @Override
    public void checkFirmware() {
        // Check that the NFC controller firmware is up to date.  This
        // ensures that firmware updates are applied in a timely fashion,
        // and makes it much less likely that the user will have to wait
        // for a firmware download when they enable NFC in the settings
        // app.  Firmware download can take some time, so this should be
        // run in a separate thread.

        // check the timestamp of the firmware file
        File firmwareFile;
        int nbRetry = 0;
        try {
        byte[] fwFileName;
        String filePath="/system/vendor/firmware/";

        //Read firmware file name from config file
        fwFileName=getFwFileName();
        if(fwFileName == null)
        {
            Log.d(TAG,"FileName not found");
            int Ver = getChipVer();
            if( Ver == PN547C2_ID || Ver == PN65T_ID )
                filePath=filePath.concat("libpn547_fw.so");
            else if( Ver == PN548AD_ID || Ver == PN66T_ID )
                filePath=filePath.concat("libpn548ad_fw.so");
            else
                filePath=null;
        }
        else
        {
            Log.d(TAG,"Firmware fileName found");
            String fileName = new String(fwFileName);
            filePath=filePath.concat(fileName);
        }
        Log.d(TAG,"Firmware file path=" + filePath);
        firmwareFile = new File(filePath);
        } catch(NullPointerException npe) {
            Log.e(TAG,"path to firmware file was null");
            return;
        }

        long modtime = firmwareFile.lastModified();

        SharedPreferences prefs = mContext.getSharedPreferences(PREF, Context.MODE_PRIVATE);
        long prev_fw_modtime = prefs.getLong(PREF_FIRMWARE_MODTIME, FIRMWARE_MODTIME_DEFAULT);
        Log.d(TAG,"prev modtime: " + prev_fw_modtime);
        Log.d(TAG,"new modtime: " + modtime);
        if (prev_fw_modtime == modtime) {
            return;
        }

        // FW download.
        Log.d(TAG,"Perform Download");
        if(doDownload()) {
            Log.d(TAG,"Download Success");
            // Now that we've finished updating the firmware, save the new modtime.
            prefs.edit().putLong(PREF_FIRMWARE_MODTIME, modtime).apply();
        }
    else {
            Log.d(TAG,"Download Failed");
        }
    }

    private native boolean doInitialize();

    @Override
    public boolean initialize() {
        SharedPreferences prefs = mContext.getSharedPreferences(PREF, Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();

        if (prefs.getBoolean(NativeNfcSecureElement.PREF_SE_WIRED, false)) {
            try {
                Thread.sleep (12000);
                editor.putBoolean(NativeNfcSecureElement.PREF_SE_WIRED, false);
                editor.apply();
            } catch (InterruptedException e) { }
        }

        return doInitialize();
    }

    private native void doEnableDtaMode();

    @Override
    public void enableDtaMode() {
        Log.d(TAG,"enableDtaMode : entry");
        doEnableDtaMode();
    }

    private native void doDisableDtaMode();

    @Override
    public void disableDtaMode() {
        Log.d(TAG,"disableDtaMode : entry");
        doDisableDtaMode();
    }

    private native boolean doDeinitialize();

    @Override
    public boolean deinitialize() {
        SharedPreferences prefs = mContext.getSharedPreferences(PREF, Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();

        editor.putBoolean(NativeNfcSecureElement.PREF_SE_WIRED, false);
        editor.apply();

        return doDeinitialize();
    }

    @Override
    public String getName() {
        return DRIVER_NAME;
    }

    @Override
    public native boolean sendRawFrame(byte[] data);

    @Override
    public boolean routeAid(byte[] aid, int route, int powerState, boolean isprefix) {

        boolean status = true;
        //if(mIsAidFilterSupported) {
            //Prepare a cache of AIDs, and only send when vzwSetFilterList is called.
          //  mAidFilter.addAppAidToCache(aid, route, powerState);
       // } else {

            status = doRouteAid(aid, route, powerState, isprefix);

        //}

        return status;
    }

    public native boolean doRouteAid(byte[] aid, int route, int powerState, boolean isprefix);

    @Override
    public native boolean setDefaultRoute(int defaultRouteEntry, int defaultProtoRouteEntry, int defaultTechRouteEntry);

    @Override
    public boolean unrouteAid(byte[] aid) {
    //    if(mIsAidFilterSupported) {
            //Remove AID entry from cache.
    //         mAidFilter.removeAppAidToCache(aid);
    //    }

        return doUnrouteAid(aid);
    }

    public native boolean doUnrouteAid(byte[] aid);

    @Override
    public boolean routeNfcid2(byte[] nfcid2, byte[] syscode, byte[] optparam) {
        Log.d(TAG,"routeNfcid2 NFCID2 : " + toHexString(nfcid2, 0, nfcid2.length) );

      //  if(mNfcid2ToHandle.get(toHexString(nfcid2, 0, nfcid2.length)) != null) {
       //     unrouteNfcid2(nfcid2);
        //}

        Log.d(TAG,"routeNfcid2 syscode--- : " + toHexString(syscode, 0, syscode.length) );
        int handle = doRouteNfcid2(nfcid2,syscode,optparam);
        if(handle != 0xFF) {
            mNfcid2ToHandle.put(toHexString(nfcid2, 0, nfcid2.length), handle);
            return true;
        } else {
            return false;
        }

    }

    private native int doRouteNfcid2(byte[] nfcid2, byte[] syscode, byte[] optparam);

    @Override
    public boolean unrouteNfcid2(byte[] nfcid2) {
        Log.d(TAG,"unrouteNfcid2 NFCID2 : " + toHexString(nfcid2, 0, nfcid2.length) );
        int handle = mNfcid2ToHandle.get(toHexString(nfcid2, 0, nfcid2.length));
        if(mNfcid2ToHandle.get(toHexString(nfcid2, 0, nfcid2.length)) != null) {
            mNfcid2ToHandle.remove(toHexString(nfcid2, 0, nfcid2.length));
        }
        return doUnRouteNfcid2(nfcid2);
    }

    private native boolean doUnRouteNfcid2(byte[] nfcid2);

    public native boolean clearAidTable();

    @Override
    public native void doSetProvisionMode(boolean provisionMode);

    @Override
    public native int getRemainingAidTableSize();

    @Override
    public native int getAidTableSize();

    @Override
    public native int   getDefaultAidRoute();

    @Override
    public native int   getDefaultDesfireRoute();

    @Override
    public native int   getDefaultMifareCLTRoute();

    @Override
    public native int   getDefaultAidPowerState();

    @Override
    public native int   getDefaultDesfirePowerState();

    @Override
    public native int   getDefaultMifareCLTPowerState();

    @Override
    public native void doSetScreenOrPowerState(int state);

    @Override
    public native void doSetScreenState(int mScreenState);

    @Override
    public native void doEnablep2p(boolean p2pFlag);

    public native boolean doSetRoutingEntry(int type, int value, int route, int power);
    @Override
    public boolean setRoutingEntry(int type, int value, int route, int power) {
        return(doSetRoutingEntry(type, value, route, power));
    }
    public native boolean doClearRoutingEntry(int type );

    @Override
    public boolean clearRoutingEntry( int type ) {
        return(doClearRoutingEntry( type ));
    }

    @Override
    public native void doSetSecureElementListenTechMask(int tech_mask);


    private native void doEnableDiscovery(int techMask,
                                          boolean enableLowPowerPolling,
                                          boolean enableReaderMode,
                                          boolean enableHostRouting,
                                          boolean enableP2p,
                                          boolean restart);

    @Override
    public void enableDiscovery(NfcDiscoveryParameters params, boolean restart) {
        doEnableDiscovery(params.getTechMask(), params.shouldEnableLowPowerDiscovery(),
                params.shouldEnableReaderMode(), params.shouldEnableHostRouting(),
                params.shouldEnableP2p(), restart);
    }

    @Override
    public native void disableDiscovery();
/*
    @Override
    public native void enableRoutingToHost();

    @Override
    public native void disableRoutingToHost();
*/
    @Override
    public native int[] doGetSecureElementList();

    @Override
    public native void doSelectSecureElement(int seID);

    @Override
    public native void doDeselectSecureElement(int seID);

    @Override
    public native void doSetSEPowerOffState(int seID, boolean enable);

    @Override
    public native void setDefaultTechRoute(int seID, int tech_switchon, int tech_switchoff);

    @Override
    public native void setDefaultProtoRoute(int seID, int proto_switchon, int proto_switchoff);

    @Override
    public native int getChipVer();

    @Override
    public native byte[] getFwFileName();

    @Override
    public native int getNfcInitTimeout();

    @Override
    public native int JCOSDownload();

    @Override
    public native void doSetNfcMode(int nfcMode);

    @Override
    public native int GetDefaultSE();

    @Override
    public native boolean isVzwFeatureEnabled();

    @Override
    public native void setEtsiReaederState(int newState);

    @Override
    public native int getEtsiReaederState();

    @Override
    public native void etsiReaderConfig(int eeHandle);

    @Override
    public native void notifyEEReaderEvent(int evt);

    @Override
    public native void etsiInitConfig();

    @Override
    public native void etsiResetReaderConfig();

    @Override
    public native void updateScreenState();

    private native NativeLlcpConnectionlessSocket doCreateLlcpConnectionlessSocket(int nSap,
            String sn);

    @Override
    public LlcpConnectionlessSocket createLlcpConnectionlessSocket(int nSap, String sn)
            throws LlcpException {
        LlcpConnectionlessSocket socket = doCreateLlcpConnectionlessSocket(nSap, sn);
        if (socket != null) {
            return socket;
        } else {
            /* Get Error Status */
            int error = doGetLastError();

            Log.d(TAG, "failed to create llcp socket: " + ErrorCodes.asString(error));

            switch (error) {
                case ErrorCodes.ERROR_BUFFER_TO_SMALL:
                case ErrorCodes.ERROR_INSUFFICIENT_RESOURCES:
                    throw new LlcpException(error);
                default:
                    throw new LlcpException(ErrorCodes.ERROR_SOCKET_CREATION);
            }
        }
    }

    private native NativeLlcpServiceSocket doCreateLlcpServiceSocket(int nSap, String sn, int miu,
            int rw, int linearBufferLength);
    @Override
    public LlcpServerSocket createLlcpServerSocket(int nSap, String sn, int miu,
            int rw, int linearBufferLength) throws LlcpException {
        LlcpServerSocket socket = doCreateLlcpServiceSocket(nSap, sn, miu, rw, linearBufferLength);
        if (socket != null) {
            return socket;
        } else {
            /* Get Error Status */
            int error = doGetLastError();

            Log.d(TAG, "failed to create llcp socket: " + ErrorCodes.asString(error));

            switch (error) {
                case ErrorCodes.ERROR_BUFFER_TO_SMALL:
                case ErrorCodes.ERROR_INSUFFICIENT_RESOURCES:
                    throw new LlcpException(error);
                default:
                    throw new LlcpException(ErrorCodes.ERROR_SOCKET_CREATION);
            }
        }
    }

    private native NativeLlcpSocket doCreateLlcpSocket(int sap, int miu, int rw,
            int linearBufferLength);
    @Override
    public LlcpSocket createLlcpSocket(int sap, int miu, int rw,
            int linearBufferLength) throws LlcpException {
        LlcpSocket socket = doCreateLlcpSocket(sap, miu, rw, linearBufferLength);
        if (socket != null) {
            return socket;
        } else {
            /* Get Error Status */
            int error = doGetLastError();

            Log.d(TAG, "failed to create llcp socket: " + ErrorCodes.asString(error));

            switch (error) {
                case ErrorCodes.ERROR_BUFFER_TO_SMALL:
                case ErrorCodes.ERROR_INSUFFICIENT_RESOURCES:
                    throw new LlcpException(error);
                default:
                    throw new LlcpException(ErrorCodes.ERROR_SOCKET_CREATION);
            }
        }
    }

    @Override
    public native boolean doCheckLlcp();

    @Override
    public native boolean doCheckJcopDlAtBoot();

    @Override
    public native boolean doActivateLlcp();

    private native void doResetTimeouts();

    @Override
    public void resetTimeouts() {
        doResetTimeouts();
    }

    @Override
    public native void doAbort();

    private native boolean doSetTimeout(int tech, int timeout);
    @Override
    public boolean setTimeout(int tech, int timeout) {
        return doSetTimeout(tech, timeout);
    }

    private native int doGetTimeout(int tech);
    @Override
    public int getTimeout(int tech) {
        return doGetTimeout(tech);
    }


    @Override
    public boolean canMakeReadOnly(int ndefType) {
        return (ndefType == Ndef.TYPE_1 || ndefType == Ndef.TYPE_2 ||
                ndefType == Ndef.TYPE_MIFARE_CLASSIC);
    }

    @Override
    public int getMaxTransceiveLength(int technology) {
        switch (technology) {
            case (TagTechnology.NFC_A):
            case (TagTechnology.MIFARE_CLASSIC):
            case (TagTechnology.MIFARE_ULTRALIGHT):
                return 253; // PN544 RF buffer = 255 bytes, subtract two for CRC
            case (TagTechnology.NFC_B):
                /////////////////////////////////////////////////////////////////
                // Broadcom: Since BCM2079x supports this, set NfcB max size.
                //return 0; // PN544 does not support transceive of raw NfcB
                return 253; // PN544 does not support transceive of raw NfcB
            case (TagTechnology.NFC_V):
                return 253; // PN544 RF buffer = 255 bytes, subtract two for CRC
            case (TagTechnology.ISO_DEP):
                /* The maximum length of a normal IsoDep frame consists of:
                 * CLA, INS, P1, P2, LC, LE + 255 payload bytes = 261 bytes
                 * such a frame is supported. Extended length frames however
                 * are not supported.
                 */
                return 0xFEFF; // Will be automatically split in two frames on the RF layer
            case (TagTechnology.NFC_F):
                return 252; // PN544 RF buffer = 255 bytes, subtract one for SoD, two for CRC
            default:
                return 0;
        }

    }

    @Override
    public native int setEmvCoPollProfile(boolean enable, int route);

    private native void doSetP2pInitiatorModes(int modes);
    @Override
    public void setP2pInitiatorModes(int modes) {
        doSetP2pInitiatorModes(modes);
    }

    private native void doSetP2pTargetModes(int modes);
    @Override
    public void setP2pTargetModes(int modes) {
        doSetP2pTargetModes(modes);
    }

    @Override
    public boolean getExtendedLengthApdusSupported() {
        return true;
    }

    @Override
    public byte[][] getWipeApdus() {
        return EE_WIPE_APDUS;
    }

    @Override
    public int getDefaultLlcpMiu() {
        return DEFAULT_LLCP_MIU;
    }

    @Override
    public int getDefaultLlcpRwSize() {
        return DEFAULT_LLCP_RWSIZE;
    }

    private native String doDump();
    @Override
    public String dump() {
        return doDump();
    }

    private native void doEnableScreenOffSuspend();
    @Override
    public boolean enableScreenOffSuspend() {
        doEnableScreenOffSuspend();
        return true;
    }

    private native void doDisableScreenOffSuspend();
    @Override
    public boolean disableScreenOffSuspend() {
        doDisableScreenOffSuspend();
        return true;
    }

    //private native void doEnableReaderMode(int technologies);
    //@Override
    //public boolean enableScreenOffSuspend() {
      //  doEnableScreenOffSuspend();
        //return true;
    //}

    //private native void doDisableScreenOffSuspend();
    //@Override
    //public boolean disableScreenOffSuspend() {
     //   doDisableScreenOffSuspend();
       // return true;
    //}


    private native void doCommitRouting();

    @Override
    public native int doGetSecureElementTechList();

    @Override
    public native int[] doGetActiveSecureElementList();


    public native byte[] doGetSecureElementUid();

    @Override
    public byte[] getSecureElementUid()
    {
        byte[] buff;
        buff = doGetSecureElementUid();
        if(buff==null)
        {
            //Avoiding Null pointer Exception creating new byte array
            buff =  new byte[0];
            Log.d(TAG,"buff : " + buff);
        }
        return buff;
    }
    @Override
    public void commitRouting() {
        doCommitRouting();
    }

    @Override
    public native void doPrbsOn(int prbs, int hw_prbs, int tech, int rate);

    @Override
    public native void doPrbsOff();

    @Override
    public native int SWPSelfTest(int ch);

    @Override
    public native int getFWVersion();

    @Override
    public native void doSetEEPROM(byte[] val);

    @Override
    public native byte[] doGetRouting();

    @Override
    public native int doGetSeInterface(int type);

    /**
     * Notifies Ndef Message (TODO: rename into notifyTargetDiscovered)
     */
    private void notifyNdefMessageListeners(NativeNfcTag tag) {
        mListener.onRemoteEndpointDiscovered(tag);
    }

    /**
     * Notifies transaction
     */
    private void notifyTargetDeselected() {
        mListener.onCardEmulationDeselected();
    }

    /**
     * Notifies transaction
     */
    private void notifyTransactionListeners(byte[] aid, byte[] data, int evtSrc) {
        mListener.onCardEmulationAidSelected(aid,data,evtSrc);
    }

    /**
     * Notifies transaction
     */
    private void notifyConnectivityListeners(int evtSrc) {
        mListener.onConnectivityEvent(evtSrc);
    }

    /**
     * Notifies transaction
     */
    private void notifyEmvcoMultiCardDetectedListeners() {
        mListener.onEmvcoMultiCardDetectedEvent();
    }

    /**
     * Notifies P2P Device detected, to activate LLCP link
     */
    private void notifyLlcpLinkActivation(NativeP2pDevice device) {
        mListener.onLlcpLinkActivated(device);
    }

    /**
     * Notifies P2P Device detected, to activate LLCP link
     */
    private void notifyLlcpLinkDeactivated(NativeP2pDevice device) {
        mListener.onLlcpLinkDeactivated(device);
    }

    /**
     * Notifies first packet received from remote LLCP
     */
    private void notifyLlcpLinkFirstPacketReceived(NativeP2pDevice device) {
        mListener.onLlcpFirstPacketReceived(device);
    }

    private void notifySeFieldActivated() {
        mListener.onRemoteFieldActivated();
    }

    private void notifySeFieldDeactivated() {
        mListener.onRemoteFieldDeactivated();
    }

    /* Reader over SWP listeners*/
    private void notifySWPReaderRequested(boolean istechA, boolean istechB) {
        mListener.onSWPReaderRequestedEvent(istechA, istechB);
    }

    private void notifySWPReaderRequestedFail(int FailureCause) {
        mListener.onSWPReaderRequestedFail(FailureCause);
    }


    private void notifySWPReaderActivated() {
        mListener.onSWPReaderActivatedEvent();
    }


    private void notifyonETSIReaderModeStartConfig(int eeHandle) {
        mListener.onETSIReaderModeStartConfig(eeHandle);
    }

    private void notifyonETSIReaderModeStopConfig(int disc_ntf_timeout) {
        mListener.onETSIReaderModeStopConfig(disc_ntf_timeout);
    }

    private void notifyonETSIReaderModeSwpTimeout(int disc_ntf_timeout) {
        mListener.onETSIReaderModeSwpTimeout(disc_ntf_timeout);
    }

    private void notifySeListenActivated() {
        mListener.onSeListenActivated();
    }

    private void notifySeListenDeactivated() {
        mListener.onSeListenDeactivated();
    }

    private void notifySeApduReceived(byte[] apdu) {
        mListener.onSeApduReceived(apdu);
    }

    private void notifySeEmvCardRemoval() {
        mListener.onSeEmvCardRemoval();
    }

    private void notifySeMifareAccess(byte[] block) {
        mListener.onSeMifareAccess(block);
    }

    private void notifyHostEmuActivated() {
        mListener.onHostCardEmulationActivated();
    }

    private void notifyHostEmuData(byte[] data) {
        mListener.onHostCardEmulationData(data);
    }

    private void notifyHostEmuDeactivated() {
        mListener.onHostCardEmulationDeactivated();
    }

    private void notifyAidRoutingTableFull() {
        mListener.onAidRoutingTableFull();
    }

    private void notifyRfFieldActivated() {
        mListener.onRemoteFieldActivated();
    }

    private void notifyRfFieldDeactivated() {
        mListener.onRemoteFieldDeactivated();
    }

    static String toHexString(byte[] buffer, int offset, int length) {
        final char[] HEX_CHARS = "0123456789abcdef".toCharArray();
        char[] chars = new char[2 * length];
        for (int j = offset; j < offset + length; ++j) {
            chars[2 * (j - offset)] = HEX_CHARS[(buffer[j] & 0xF0) >>> 4];
            chars[2 * (j - offset) + 1] = HEX_CHARS[buffer[j] & 0x0F];
        }
        return new String(chars);
    }

}
