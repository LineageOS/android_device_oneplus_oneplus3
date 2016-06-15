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
package com.android.nfc;

import android.app.ActivityManager;
import android.app.Application;
import android.app.KeyguardManager;
import android.app.PendingIntent;
import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.content.pm.IPackageManager;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.PackageManager;
import android.content.pm.UserInfo;
import android.content.res.Resources.NotFoundException;
import android.media.AudioManager;
import android.media.SoundPool;
import android.net.Uri;
import android.nfc.BeamShareData;
import android.nfc.ErrorCodes;
import android.nfc.FormatException;
import android.nfc.IAppCallback;
import android.nfc.INfcAdapter;
import android.nfc.INfcAdapterExtras;
import android.nfc.INfcCardEmulation;
import android.nfc.INfcTag;
import android.nfc.INfcUnlockHandler;
import android.nfc.NdefMessage;
import android.nfc.NfcAdapter;
import android.nfc.Tag;
import android.nfc.TechListParcel;
import android.nfc.TransceiveResult;
import android.nfc.tech.Ndef;
import android.nfc.tech.TagTechnology;
import android.os.AsyncTask;
import android.os.Binder;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.PowerManager;
import android.os.Process;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings;
import android.util.Log;

import com.android.nfc.DeviceHost.DeviceHostListener;
import com.android.nfc.DeviceHost.LlcpConnectionlessSocket;
import com.android.nfc.DeviceHost.LlcpServerSocket;
import com.android.nfc.DeviceHost.LlcpSocket;
import com.android.nfc.DeviceHost.NfcDepEndpoint;
import com.android.nfc.DeviceHost.TagEndpoint;

import com.android.nfc.dhimpl.NativeNfcSecureElement;
import com.android.nfc.dhimpl.NativeNfcAla;
import java.security.MessageDigest;

import android.widget.Toast;

import com.android.nfc.cardemulation.AidRoutingManager;
import com.android.nfc.cardemulation.CardEmulationManager;
import com.android.nfc.cardemulation.Nfcid2RoutingManager;
import com.android.nfc.cardemulation.Nfcid2RoutingCache;
import com.android.nfc.cardemulation.RegisteredNfcid2Cache;
import com.android.nfc.dhimpl.NativeNfcManager;
import com.android.nfc.handover.HandoverDataParser;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.File;
import java.io.BufferedReader;
import java.io.FileReader;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import android.util.Pair;
import java.util.HashSet;
import java.util.List;
import java.util.concurrent.ExecutionException;

import com.nxp.nfc.INxpNfcAdapter;
import com.nxp.intf.ILoaderService;
import com.nxp.intf.IJcopService;
import com.nxp.intf.INxpExtrasService;
import com.nxp.intf.IeSEClientServicesAdapter;
import com.nxp.nfc.INfcDta;
import com.nxp.nfc.INfcVzw;
import com.nxp.nfc.INxpNfcAdapterExtras;
import com.nxp.nfc.INxpNfcAccessExtras;
import com.nxp.nfc.NxpNfcAdapter;
import com.nxp.nfc.NxpConstants;
import com.vzw.nfc.RouteEntry;
import com.gsma.nfc.internal.NxpNfcController;
import com.nxp.nfc.gsma.internal.INxpNfcController;

public class NfcService implements DeviceHostListener {
    private static final String ACTION_MASTER_CLEAR_NOTIFICATION = "android.intent.action.MASTER_CLEAR_NOTIFICATION";

    static final boolean DBG = true;
    static final String TAG = "NfcService";

    public static final String SERVICE_NAME = "nfc";

    /** Regular NFC permission */
    private static final String NFC_PERM = android.Manifest.permission.NFC;
    private static final String NFC_PERM_ERROR = "NFC permission required";

    public static final String PREF = "NfcServicePrefs";
    public static final String NXP_PREF = "NfcServiceNxpPrefs";

    static final String PREF_NFC_ON = "nfc_on";
    static final boolean NFC_ON_DEFAULT = true;
    static final String PREF_NDEF_PUSH_ON = "ndef_push_on";
    static final boolean NDEF_PUSH_ON_DEFAULT = true;
    static final String PREF_FIRST_BEAM = "first_beam";
    static final String PREF_FIRST_BOOT = "first_boot";
    static final String PREF_AIRPLANE_OVERRIDE = "airplane_override";
    private static final String PREF_SECURE_ELEMENT_ON = "secure_element_on";
    private boolean SECURE_ELEMENT_ON_DEFAULT = false;
    private int SECURE_ELEMENT_ID_DEFAULT = 0;
    private static final String PREF_DEFAULT_ROUTE_ID = "default_route_id";
    private static final String PREF_MIFARE_DESFIRE_PROTO_ROUTE_ID = "mifare_desfire_proto_route";
    private static final String PREF_SET_DEFAULT_ROUTE_ID ="set_default_route";
    private static final String PREF_MIFARE_CLT_ROUTE_ID= "mifare_clt_route";
    private static final String LS_BACKUP_PATH = "/data/nfc/ls_backup.txt";
    private static final String LS_UPDATE_BACKUP_PATH = "/data/nfc/loaderservice_updater.txt";
    private static final String LS_UPDATE_BACKUP_OUT_PATH = "/data/nfc/loaderservice_updater_out.txt";

    private static final String[] path = {"/data/nfc/JcopOs_Update1.apdu",
                                          "/data/nfc/JcopOs_Update2.apdu",
                                          "data/nfc/JcopOs_Update3.apdu"};

    private static final String[] PREF_JCOP_MODTIME = {"jcop file1 modtime",
                                                       "jcop file2 modtime",
                                                       "jcop file3 modtime"};
    private static final long[] JCOP_MODTIME_DEFAULT = {-1,-1,-1};
    private static final long[] JCOP_MODTIME_TEMP = {-1,-1,-1};

    private boolean ETSI_STOP_CONFIG = false;
    private int ROUTE_ID_HOST= 0x00;
    private int ROUTE_ID_SMX= 0x01;
    private int ROUTE_ID_UICC= 0x02;

    private int ROUTE_SWITCH_ON = 0x01;
    private int ROUTE_SWITCH_OFF = 0x02;
    private int ROUTE_BATT_OFF= 0x04;

    private int TECH_TYPE_A= 0x01;
    private int TECH_TYPE_B= 0x02;
    private int TECH_TYPE_F= 0x04;

    //TODO: Refer L_OSP_EXT [PN547C2]
//    private int DEFAULT_ROUTE_ID_DEFAULT = AidRoutingManager.DEFAULT_ROUTE;
    private int DEFAULT_ROUTE_ID_DEFAULT = 0x00;
    static final boolean SE_BROADCASTS_WITH_HCE = true;

    private static final String PREF_SECURE_ELEMENT_ID = "secure_element_id";
    public static final int ROUTE_LOC_MASK=5;
    public static final int TECH_TYPE_MASK=7;

    static final int MSG_NDEF_TAG = 0;
    static final int MSG_CARD_EMULATION = 1;
    static final int MSG_LLCP_LINK_ACTIVATION = 2;
    static final int MSG_LLCP_LINK_DEACTIVATED = 3;
    static final int MSG_TARGET_DESELECTED = 4;
    static final int MSG_MOCK_NDEF = 7;
    static final int MSG_SE_FIELD_ACTIVATED = 8;
    static final int MSG_SE_FIELD_DEACTIVATED = 9;
    static final int MSG_SE_APDU_RECEIVED = 10;
    static final int MSG_SE_EMV_CARD_REMOVAL = 11;
    static final int MSG_SE_MIFARE_ACCESS = 12;
    static final int MSG_SE_LISTEN_ACTIVATED = 13;
    static final int MSG_SE_LISTEN_DEACTIVATED = 14;
    static final int MSG_LLCP_LINK_FIRST_PACKET = 15;
    static final int MSG_ROUTE_AID = 16;
    static final int MSG_UNROUTE_AID = 17;
    static final int MSG_COMMIT_ROUTING = 18;
    static final int MSG_INVOKE_BEAM = 19;

    static final int MSG_SWP_READER_REQUESTED = 20;
    static final int MSG_SWP_READER_ACTIVATED = 21;
    static final int MSG_SWP_READER_DEACTIVATED = 22;
    static final int MSG_CLEAR_ROUTING = 23;
    static final int MSG_SET_SCREEN_STATE = 25;


    static final int MSG_RF_FIELD_ACTIVATED = 26;
    static final int MSG_RF_FIELD_DEACTIVATED = 27;
    static final int MSG_RESUME_POLLING = 28;
    static final int MSG_SWP_READER_REQUESTED_FAIL =29 ;
    static final int MSG_SWP_READER_TAG_PRESENT = 30;
    static final int MSG_SWP_READER_TAG_REMOVE = 31;
    static final int MSG_CONNECTIVITY_EVENT = 40;
    static final int MSG_VZW_ROUTE_AID = 41;
    static final int MSG_VZW_COMMIT_ROUTING = 42;
    static final int MSG_ROUTE_NFCID2 = 43;
    static final int MSG_UNROUTE_NFCID2 = 44;
    static final int MSG_COMMITINF_FELICA_ROUTING = 45;
    static final int MSG_COMMITED_FELICA_ROUTING = 46;
    static final int MSG_EMVCO_MULTI_CARD_DETECTED_EVENT = 47;
    static final int MSG_ETSI_START_CONFIG = 48;
    static final int MSG_ETSI_STOP_CONFIG = 49;
    static final int MSG_ETSI_SWP_TIMEOUT = 50;
    static final int MSG_APPLY_SREEN_STATE = 51;
    static final long MAX_POLLING_PAUSE_TIMEOUT = 40000;
    static final int TASK_ENABLE = 1;
    static final int TASK_DISABLE = 2;
    static final int TASK_BOOT = 3;
    static final int TASK_EE_WIPE = 4;
    static final int MSG_CHANGE_DEFAULT_ROUTE = 52;
    static final int MSG_SE_DELIVER_INTENT = 53;

    // Copied from com.android.nfc_extras to avoid library dependency
    // Must keep in sync with com.android.nfc_extras
    static final int ROUTE_OFF = 1;
    static final int ROUTE_ON_WHEN_SCREEN_ON = 2;

    // Return values from NfcEe.open() - these are 1:1 mapped
    // to the thrown EE_EXCEPTION_ exceptions in nfc-extras.
    static final int EE_ERROR_IO = -1;
    static final int EE_ERROR_ALREADY_OPEN = -2;
    static final int EE_ERROR_INIT = -3;
    static final int EE_ERROR_LISTEN_MODE = -4;
    static final int EE_ERROR_EXT_FIELD = -5;
    static final int EE_ERROR_NFC_DISABLED = -6;

    // Polling technology masks
    static final int NFC_POLL_A = 0x01;
    static final int NFC_POLL_B = 0x02;
    static final int NFC_POLL_F = 0x04;
    static final int NFC_POLL_ISO15693 = 0x08;
    static final int NFC_POLL_B_PRIME = 0x10;
    static final int NFC_POLL_KOVIO = 0x20;

    // minimum screen state that enables NFC polling
    static final int NFC_POLLING_MODE = ScreenStateHelper.SCREEN_STATE_ON_UNLOCKED;

    // Time to wait for NFC controller to initialize before watchdog
    // goes off. This time is chosen large, because firmware download
    // may be a part of initialization.
    static final int INIT_WATCHDOG_MS = 90000;
    static final int INIT_WATCHDOG_LS_MS = 180000;
    // Time to wait for routing to be applied before watchdog
    // goes off
    static final int ROUTING_WATCHDOG_MS = 10000;

    // Amount of time to wait before closing the NFCEE connection
    // in a disable/shutdown scenario.
    static final int WAIT_FOR_NFCEE_OPERATIONS_MS = 5000;
    // Polling interval for waiting on NFCEE operations
    static final int WAIT_FOR_NFCEE_POLL_MS = 100;

    // Default delay used for presence checks
    static final int DEFAULT_PRESENCE_CHECK_DELAY = 125;

    //Delay used for presence checks of NFC_F non-Ndef
    //Make secure communication done or tranceive next request response command
    //to pause timer before presence check command is sent
    static final int NFC_F_TRANSCEIVE_PRESENCE_CHECK_DELAY = 500;

    // The amount of time we wait before manually launching
    // the Beam animation when called through the share menu.
    static final int INVOKE_BEAM_DELAY_MS = 1000;
    // for use with playSound()
    public static final int SOUND_START = 0;
    public static final int SOUND_END = 1;
    public static final int SOUND_ERROR = 2;

    //ETSI Reader Events
    public static final int ETSI_READER_REQUESTED   = 0;
    public static final int ETSI_READER_START       = 1;
    public static final int ETSI_READER_STOP        = 2;

    //ETSI Reader Req States
    public static final int STATE_SE_RDR_MODE_INVALID = 0x00;
    public static final int STATE_SE_RDR_MODE_START_CONFIG = 0x01;
    public static final int STATE_SE_RDR_MODE_START_IN_PROGRESS = 0x02;
    public static final int STATE_SE_RDR_MODE_STARTED = 0x03;
    public static final int STATE_SE_RDR_MODE_ACTIVATED = 0x04;
    public static final int STATE_SE_RDR_MODE_STOP_CONFIG = 0x05;
    public static final int STATE_SE_RDR_MODE_STOP_IN_PROGRESS = 0x06;
    public static final int STATE_SE_RDR_MODE_STOPPED = 0x07;

    public static final String ACTION_RF_FIELD_ON_DETECTED =
        "com.android.nfc_extras.action.RF_FIELD_ON_DETECTED";
    public static final String ACTION_RF_FIELD_OFF_DETECTED =
        "com.android.nfc_extras.action.RF_FIELD_OFF_DETECTED";
    public static final String ACTION_AID_SELECTED =
        "com.android.nfc_extras.action.AID_SELECTED";
    public static final String EXTRA_AID = "com.android.nfc_extras.extra.AID";

    public static final String ACTION_LLCP_UP =
            "com.android.nfc.action.LLCP_UP";

    public static final String ACTION_LLCP_DOWN =
            "com.android.nfc.action.LLCP_DOWN";

    public static final String ACTION_APDU_RECEIVED =
        "com.android.nfc_extras.action.APDU_RECEIVED";
    public static final String EXTRA_APDU_BYTES =
        "com.android.nfc_extras.extra.APDU_BYTES";

    public static final String ACTION_EMV_CARD_REMOVAL =
        "com.android.nfc_extras.action.EMV_CARD_REMOVAL";

    public static final String ACTION_MIFARE_ACCESS_DETECTED =
        "com.android.nfc_extras.action.MIFARE_ACCESS_DETECTED";
    public static final String EXTRA_MIFARE_BLOCK =
        "com.android.nfc_extras.extra.MIFARE_BLOCK";

    public static final String ACTION_SE_LISTEN_ACTIVATED =
            "com.android.nfc_extras.action.SE_LISTEN_ACTIVATED";
    public static final String ACTION_SE_LISTEN_DEACTIVATED =
            "com.android.nfc_extras.action.SE_LISTEN_DEACTIVATED";

    public static final String ACTION_EMVCO_MULTIPLE_CARD_DETECTED =
            "com.nxp.action.EMVCO_MULTIPLE_CARD_DETECTED";

    private static final String PACKAGE_SMART_CARD_SERVICE  = "org.simalliance.openmobileapi.service";
    /**
     * SMART MX ID to be able to select it as the default Secure Element
     */
    public static final int SMART_MX_ID_TYPE = 1;

    /**
     * UICC ID to be able to select it as the default Secure Element
     */
    public static final int UICC_ID_TYPE = 2;

    /**
     * ID to be able to select all Secure Elements
     */
    public static final int ALL_SE_ID_TYPE = 3;

    public static final int PN547C2_ID = 1;
    public static final int PN65T_ID = 2;
    public static final int PN548AD_ID = 3;
    public static final int PN66T_ID = 4;

    public static final int LS_RETRY_CNT = 3;
    public static final int LOADER_SERVICE_VERSION_21 = 0x21;
    private int mSelectedSeId = 0;
    private boolean mNfcSecureElementState;
    private boolean mIsSmartCardServiceSupported = false;
    // Timeout to re-apply routing if a tag was present and we postponed it
    private static final int APPLY_ROUTING_RETRY_TIMEOUT_MS = 5000;

    private final UserManager mUserManager;
    // NFC Execution Environment
    // fields below are protected by this
    private NativeNfcSecureElement mSecureElement;
    private OpenSecureElement mOpenEe;  // null when EE closed
    private final ReaderModeDeathRecipient mReaderModeDeathRecipient =
            new ReaderModeDeathRecipient();
    private final NfcUnlockManager mNfcUnlockManager;

    private int mEeRoutingState;  // contactless interface routing
    private int mLockscreenPollMask;
    List<PackageInfo> mInstalledPackages; // cached version of installed packages
    private NativeNfcAla mNfcAla;

    NfcAccessExtrasService mNfcAccessExtrasService;

    // fields below are used in multiple threads and protected by synchronized(this)
    final HashMap<Integer, Object> mObjectMap = new HashMap<Integer, Object>();
    // mSePackages holds packages that accessed the SE, but only for the owner user,
    // as SE access is not granted for non-owner users.
    HashSet<String> mSePackages = new HashSet<String>();
    int mScreenState;
    boolean mInProvisionMode; // whether we're in setup wizard and enabled NFC provisioning
    boolean mIsNdefPushEnabled;
    boolean mNfcPollingEnabled;  // current Device Host state of NFC-C polling
    boolean mHostRouteEnabled;   // current Device Host state of host-based routing
    boolean mReaderModeEnabled;  // current Device Host state of reader mode
    boolean mNfceeRouteEnabled;  // current Device Host state of NFC-EE routing
    NfcDiscoveryParameters mCurrentDiscoveryParameters =
            NfcDiscoveryParameters.getNfcOffParameters();
    ReaderModeParams mReaderModeParams;

    // mState is protected by this, however it is only modified in onCreate()
    // and the default AsyncTask thread so it is read unprotected from that
    // thread
    int mState;  // one of NfcAdapter.STATE_ON, STATE_TURNING_ON, etc
    boolean mPowerShutDown = false;  // State for power shut down state

    // fields below are final after onCreate()
    Context mContext;
    private DeviceHost mDeviceHost;
    private SharedPreferences mPrefs;
    private SharedPreferences mNxpPrefs;
    private SharedPreferences.Editor mPrefsEditor;
    private SharedPreferences.Editor mNxpPrefsEditor;
    private PowerManager.WakeLock mRoutingWakeLock;
    private PowerManager.WakeLock mEeWakeLock;

    int mStartSound;
    int mEndSound;
    int mErrorSound;
    SoundPool mSoundPool; // playback synchronized on this
    P2pLinkManager mP2pLinkManager;
    TagService mNfcTagService;
    NfcAdapterService mNfcAdapter;
    NfcAdapterExtrasService mExtrasService;
//    CardEmulationService mCardEmulationService;
    NxpNfcAdapterService mNxpNfcAdapter;
    NxpNfcAdapterExtrasService mNxpExtrasService;
    NxpExtrasService mNxpExtras;
    EseClientServicesAdapter mEseClientServicesAdapter;
    boolean mIsAirplaneSensitive;
    boolean mIsAirplaneToggleable;
    boolean mIsDebugBuild;
    boolean mIsHceCapable;
    boolean mPollingPaused;
    boolean mIsRoutingTableDirty;
    boolean mIsFelicaOnHostConfigured;
    boolean mIsFelicaOnHostConfiguring;

    public boolean mIsRouteForced;
    NfceeAccessControl mNfceeAccessControl;
    NfcSccAccessControl mNfcSccAccessControl;
    NfcSeAccessControl mNfcSeAccessControl;
    NfcAlaService mAlaService;
    NfcJcopService mJcopService;
    NfcDtaService mDtaService;
    NfcVzwService mVzwService;
    private NfcDispatcher mNfcDispatcher;
    private PowerManager mPowerManager;
    private KeyguardManager mKeyguard;
    ToastHandler mToastHandler;
    private HandoverDataParser mHandoverDataParser;
    private ContentResolver mContentResolver;
  //  private RegisteredAidCache mAidCache;
    private CardEmulationManager mCardEmulationManager;
    private AidRoutingManager mAidRoutingManager;
    private Nfcid2RoutingManager mNfcid2RoutingManager;
    private ScreenStateHelper mScreenStateHelper;
    private ForegroundUtils mForegroundUtils;
    private boolean mClearNextTapDefault;
    private NxpNfcController mNxpNfcController;

    private int mUserId;
    private static NfcService sService;
    public static boolean mIsDtaMode = false;
    public static boolean mIsShortRecordLayout = false;
    public static boolean fAidTableFull = false;

    //GSMA
    private final Boolean defaultTransactionEventReceptionMode = Boolean.FALSE;
    private final static Boolean multiReceptionMode = Boolean.TRUE;
    private final static Boolean unicastReceptionMode = Boolean.FALSE;
    boolean mIsSentUnicastReception = false;

    public void enforceNfcSeAdminPerm(String pkg) {
        if (pkg == null) {
            throw new SecurityException("caller must pass a package name");
        }
        mContext.enforceCallingOrSelfPermission(NFC_PERM, NFC_PERM_ERROR);
        if (!mNfcSeAccessControl.check(Binder.getCallingUid(), pkg)) {
            throw new SecurityException(NfcSeAccessControl.NFCSE_ACCESS_PATH +
                    " denies NFCSe access to " + pkg);
        }
        if (UserHandle.getCallingUserId() != UserHandle.USER_OWNER) {
            throw new SecurityException("only the owner is allowed to act as SCC admin");
        }
    }
    public void enforceNfceeAdminPerm(String pkg) {
        if (pkg == null) {
            throw new SecurityException("caller must pass a package name");
        }
        NfcPermissions.enforceUserPermissions(mContext);
        if (!mNfceeAccessControl.check(Binder.getCallingUid(), pkg)) {
            throw new SecurityException(NfceeAccessControl.NFCEE_ACCESS_PATH +
                    " denies NFCEE access to " + pkg);
        }
        if (UserHandle.getCallingUserId() != UserHandle.USER_OWNER) {
            throw new SecurityException("only the owner is allowed to call SE APIs");
        }
    }


    /* SCC Access Control */
    public void enforceNfcSccAdminPerm(String pkg) {
        if (pkg == null) {
            throw new SecurityException("caller must pass a package name");
        }
        mContext.enforceCallingOrSelfPermission(NFC_PERM, NFC_PERM_ERROR);
        if (!mNfcSccAccessControl.check(Binder.getCallingUid(), pkg)) {
            throw new SecurityException(NfcSccAccessControl.NFCSCC_ACCESS_PATH +
                    " denies NFCSCC access to " + pkg);
        }
        if (UserHandle.getCallingUserId() != UserHandle.USER_OWNER) {
            throw new SecurityException("only the owner is allowed to act as SCC admin");
        }
    }

    public static NfcService getInstance() {
        return sService;
    }

    @Override
    public void onRemoteEndpointDiscovered(TagEndpoint tag) {
        sendMessage(NfcService.MSG_NDEF_TAG, tag);
    }

    public int getRemainingAidTableSize() {
        return mDeviceHost.getRemainingAidTableSize();
    }

    public int getChipVer() {
        return mDeviceHost.getChipVer();
    }
    /**
     * Notifies Card emulation deselect
     */
    @Override
    public void onCardEmulationDeselected() {
        if (!mIsHceCapable || SE_BROADCASTS_WITH_HCE) {
            sendMessage(NfcService.MSG_TARGET_DESELECTED, null);
        }
    }

    /**
     * Notifies transaction
     */
    @Override
    public void onCardEmulationAidSelected(byte[] aid, byte[] data, int evtSrc) {
        if (!mIsHceCapable || SE_BROADCASTS_WITH_HCE) {
            Pair<byte[], Integer> dataSrc = new Pair<byte[], Integer>(data, evtSrc);
            Pair<byte[], Pair> transactionInfo = new Pair<byte[], Pair>(aid, dataSrc);
            Log.d(TAG, "onCardEmulationAidSelected : Source" + evtSrc);

            sendMessage(NfcService.MSG_CARD_EMULATION, transactionInfo);
        }
    }

    /**
     * Notifies connectivity
     */
    @Override
    public void onConnectivityEvent(int evtSrc) {
        Log.d(TAG, "onConnectivityEvent : Source" + evtSrc);
        sendMessage(NfcService.MSG_CONNECTIVITY_EVENT, evtSrc);
    }

    @Override
    public void onEmvcoMultiCardDetectedEvent() {
        Log.d(TAG, "onEmvcoMultiCardDetectedEvent");
        sendMessage(NfcService.MSG_EMVCO_MULTI_CARD_DETECTED_EVENT,null);
    }

    /**
     * Notifies transaction
     */
    @Override
    public void onHostCardEmulationActivated() {
        if (mCardEmulationManager != null) {
            mCardEmulationManager.onHostCardEmulationActivated();
        }
    }

    @Override
    public void onAidRoutingTableFull() {
        Log.d(TAG, "NxpNci: onAidRoutingTableFull: AID Routing Table is FULL!");
        /*if((ROUTE_ID_HOST != GetDefaultRouteLoc())&&(fAidTableFull == false))
        {
            Log.d(TAG, "NxpNci: onAidRoutingTableFull: Making Default Route to HOST!");
            fAidTableFull = true;
            mHandler.sendEmptyMessage(NfcService.MSG_CHANGE_DEFAULT_ROUTE);
        }*/
        if (mIsHceCapable) {
            mAidRoutingManager.onNfccRoutingTableCleared();
            mCardEmulationManager.onRoutingTableChanged();
        }
    }

    @Override
    public void onHostCardEmulationData(byte[] data) {
        if (mCardEmulationManager != null) {
            mCardEmulationManager.onHostCardEmulationData(data);
        }
    }

    @Override
    public void onHostCardEmulationDeactivated() {
        if (mCardEmulationManager != null) {
            mCardEmulationManager.onHostCardEmulationDeactivated();
        }
    }

    /**
     * Notifies P2P Device detected, to activate LLCP link
     */
    @Override
    public void onLlcpLinkActivated(NfcDepEndpoint device) {
        sendMessage(NfcService.MSG_LLCP_LINK_ACTIVATION, device);
    }

    /**
     * Notifies P2P Device detected, to activate LLCP link
     */
    @Override
    public void onLlcpLinkDeactivated(NfcDepEndpoint device) {
        sendMessage(NfcService.MSG_LLCP_LINK_DEACTIVATED, device);
    }

    /**
     * Notifies P2P Device detected, first packet received over LLCP link
     */
    @Override
    public void onLlcpFirstPacketReceived(NfcDepEndpoint device) {
        sendMessage(NfcService.MSG_LLCP_LINK_FIRST_PACKET, device);
    }

    @Override
    public void onRemoteFieldActivated() {
        if (!mIsHceCapable || SE_BROADCASTS_WITH_HCE) {
            sendMessage(NfcService.MSG_SE_FIELD_ACTIVATED, null);
        }
    }

    @Override
    public void onRemoteFieldDeactivated() {
        if (!mIsHceCapable || SE_BROADCASTS_WITH_HCE) {
            sendMessage(NfcService.MSG_SE_FIELD_DEACTIVATED, null);
        }
    }

    @Override
    public void onSeListenActivated() {
        if (!mIsHceCapable || SE_BROADCASTS_WITH_HCE) {
            sendMessage(NfcService.MSG_SE_LISTEN_ACTIVATED, null);
        }
        if (mIsHceCapable) {
            mCardEmulationManager.onHostCardEmulationActivated();
        }
    }

    @Override
    public void onSeListenDeactivated() {
        if (!mIsHceCapable || SE_BROADCASTS_WITH_HCE) {
            sendMessage(NfcService.MSG_SE_LISTEN_DEACTIVATED, null);
        }
        if( mIsHceCapable) {
            mCardEmulationManager.onHostCardEmulationDeactivated();
        }
    }


    @Override
    public void onSeApduReceived(byte[] apdu) {
        if (!mIsHceCapable || SE_BROADCASTS_WITH_HCE) {
            sendMessage(NfcService.MSG_SE_APDU_RECEIVED, apdu);
        }
    }

    @Override
    public void onSeEmvCardRemoval() {
        if (!mIsHceCapable || SE_BROADCASTS_WITH_HCE) {
            sendMessage(NfcService.MSG_SE_EMV_CARD_REMOVAL, null);
        }
    }

    @Override
    public void onSeMifareAccess(byte[] block) {
        if (!mIsHceCapable || SE_BROADCASTS_WITH_HCE) {
            sendMessage(NfcService.MSG_SE_MIFARE_ACCESS, block);
        }
    }

    @Override
    public void onSWPReaderRequestedEvent(boolean istechA, boolean istechB)
    {
        int size=0;
        ArrayList<Integer> techList = new ArrayList<Integer>();
        if(istechA)
            techList.add(TagTechnology.NFC_A);
        if(istechB)
            techList.add(TagTechnology.NFC_B);

        sendMessage(NfcService.MSG_SWP_READER_REQUESTED , techList);
    }

    @Override
    public void onSWPReaderRequestedFail(int FailCause)
    {
        sendMessage(NfcService.MSG_SWP_READER_REQUESTED_FAIL , FailCause);
    }

    @Override
    public void onSWPReaderActivatedEvent()
    {
        sendMessage(NfcService.MSG_SWP_READER_ACTIVATED, null);
    }

    @Override
    public void onETSIReaderModeStartConfig(int eeHandle)
    {
        ArrayList<Integer> configList = new ArrayList<Integer>();
        configList.add(eeHandle);
        sendMessage(NfcService.MSG_ETSI_START_CONFIG, configList);
    }

    @Override
    public void onETSIReaderModeStopConfig(int disc_ntf_timeout)
    {
        sendMessage(NfcService.MSG_ETSI_STOP_CONFIG, disc_ntf_timeout);
    }

    @Override
    public void onETSIReaderModeSwpTimeout(int disc_ntf_timeout)
    {
        sendMessage(NfcService.MSG_ETSI_SWP_TIMEOUT, disc_ntf_timeout);
    }

    final class ReaderModeParams {
        public int flags;
        public IAppCallback callback;
        public int presenceCheckDelay;
    }

    public NfcService(Application nfcApplication) {
        mUserId = ActivityManager.getCurrentUser();
        mContext = nfcApplication;

        mNfcTagService = new TagService();
        mNfcAdapter = new NfcAdapterService();
        mNxpNfcAdapter = new NxpNfcAdapterService();
        mExtrasService = new NfcAdapterExtrasService();
        mNxpExtrasService = new NxpNfcAdapterExtrasService();
      //  mCardEmulationService = new CardEmulationService();

        Log.i(TAG, "Starting NFC service");

        sService = this;

        mScreenStateHelper = new ScreenStateHelper(mContext);
        mContentResolver = mContext.getContentResolver();
        mDeviceHost = new NativeNfcManager(mContext, this);

        mNfcUnlockManager = NfcUnlockManager.getInstance();

        mHandoverDataParser = new HandoverDataParser();
        boolean isNfcProvisioningEnabled = false;
        try {
            isNfcProvisioningEnabled = mContext.getResources().getBoolean(
                    R.bool.enable_nfc_provisioning);
        } catch (NotFoundException e) {
        }

        if (isNfcProvisioningEnabled) {
            mInProvisionMode = Settings.Secure.getInt(mContentResolver,
                    Settings.Global.DEVICE_PROVISIONED, 0) == 0;
        } else {
            mInProvisionMode = false;
        }

        if(mInProvisionMode)
        {
            /* if device is in provision mode, set this mode at lower layers */
            mDeviceHost.doSetProvisionMode(mInProvisionMode);
        }

        mNfcDispatcher = new NfcDispatcher(mContext, mHandoverDataParser, mInProvisionMode);
        mP2pLinkManager = new P2pLinkManager(mContext, mHandoverDataParser,
                mDeviceHost.getDefaultLlcpMiu(), mDeviceHost.getDefaultLlcpRwSize());

        mSecureElement = new NativeNfcSecureElement(mContext);
        mEeRoutingState = ROUTE_OFF;
        mToastHandler = new ToastHandler(mContext);

        mNfceeAccessControl = new NfceeAccessControl(mContext);
        mNfcSccAccessControl = new NfcSccAccessControl(mContext);
        mNfcSeAccessControl  = new NfcSeAccessControl(mContext);
        mNfcAla = new NativeNfcAla();

        mPrefs = mContext.getSharedPreferences(PREF, Context.MODE_PRIVATE);
        mPrefsEditor = mPrefs.edit();
        mNxpPrefs = mContext.getSharedPreferences(NXP_PREF, Context.MODE_PRIVATE);
        mNxpPrefsEditor = mNxpPrefs.edit();

        mState = NfcAdapter.STATE_OFF;
        mIsNdefPushEnabled = mPrefs.getBoolean(PREF_NDEF_PUSH_ON, NDEF_PUSH_ON_DEFAULT);
        setBeamShareActivityState(mIsNdefPushEnabled);

        mIsDebugBuild = "userdebug".equals(Build.TYPE) || "eng".equals(Build.TYPE);

        mPowerManager = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);

        mRoutingWakeLock = mPowerManager.newWakeLock(
            PowerManager.PARTIAL_WAKE_LOCK, "NfcService:mRoutingWakeLock");
        mEeWakeLock = mPowerManager.newWakeLock(
            PowerManager.PARTIAL_WAKE_LOCK, "NfcService:mEeWakeLock");

        mKeyguard = (KeyguardManager) mContext.getSystemService(Context.KEYGUARD_SERVICE);
        mUserManager = (UserManager) mContext.getSystemService(Context.USER_SERVICE);

        mScreenState = mScreenStateHelper.checkScreenState();

        ServiceManager.addService(SERVICE_NAME, mNfcAdapter);

        // Intents for all users
        IntentFilter filter = new IntentFilter(NativeNfcManager.INTERNAL_TARGET_DESELECTED_ACTION);
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_USER_PRESENT);
        filter.addAction(Intent.ACTION_USER_SWITCHED);
        filter.addAction(Intent.ACTION_SHUTDOWN);
        registerForAirplaneMode(filter);
        mContext.registerReceiverAsUser(mReceiver, UserHandle.ALL, filter, null, null);

        IntentFilter ownerFilter = new IntentFilter(Intent.ACTION_EXTERNAL_APPLICATIONS_AVAILABLE);
        ownerFilter.addAction(Intent.ACTION_EXTERNAL_APPLICATIONS_UNAVAILABLE);
        mContext.registerReceiver(mOwnerReceiver, ownerFilter);

        ownerFilter = new IntentFilter();
        ownerFilter.addAction(Intent.ACTION_PACKAGE_ADDED);
        ownerFilter.addAction(Intent.ACTION_PACKAGE_REMOVED);
        ownerFilter.addDataScheme("package");
        mContext.registerReceiver(mOwnerReceiver, ownerFilter);

        IntentFilter x509CertificateFilter = new IntentFilter();
        x509CertificateFilter.addAction(NxpConstants.ACTION_CHECK_X509_RESULT);
        mContext.registerReceiver(x509CertificateReceiver, x509CertificateFilter);

        IntentFilter enableNfc = new IntentFilter();
        enableNfc.addAction(NxpConstants.ACTION_GSMA_ENABLE_NFC);
        mContext.registerReceiver(mEnableNfc, enableNfc);

        IntentFilter lsFilter = new IntentFilter(NfcAdapter.ACTION_ADAPTER_STATE_CHANGED);
        //mContext.registerReceiver(mAlaReceiver, lsFilter);
        mContext.registerReceiverAsUser(mAlaReceiver, UserHandle.ALL, lsFilter, null, null);

        IntentFilter policyFilter = new IntentFilter(DevicePolicyManager.ACTION_DEVICE_POLICY_MANAGER_STATE_CHANGED);
        mContext.registerReceiverAsUser(mPolicyReceiver, UserHandle.ALL, policyFilter, null, null);

        updatePackageCache();

        PackageManager pm = mContext.getPackageManager();
        mIsHceCapable = pm.hasSystemFeature(PackageManager.FEATURE_NFC_HOST_CARD_EMULATION);
        if (mIsHceCapable) {
            mAidRoutingManager = new AidRoutingManager();
            mNfcid2RoutingManager = new Nfcid2RoutingManager();
            mCardEmulationManager = new CardEmulationManager(mContext, mAidRoutingManager, mNfcid2RoutingManager);
            //mCardEmulationManager = new CardEmulationManager(mContext);
            Log.d("NfcService", "Before mIsHceCapable");
            mNxpNfcController = new NxpNfcController(mContext, mCardEmulationManager);
        }

        mForegroundUtils = ForegroundUtils.getInstance();
        new EnableDisableTask().execute(TASK_BOOT);  // do blocking boot tasks
    }

    void initSoundPool() {
        synchronized(this) {
            if (mSoundPool == null) {
                mSoundPool = new SoundPool(1, AudioManager.STREAM_NOTIFICATION, 0);
                mStartSound = mSoundPool.load(mContext, R.raw.start, 1);
                mEndSound = mSoundPool.load(mContext, R.raw.end, 1);
                mErrorSound = mSoundPool.load(mContext, R.raw.error, 1);
            }
        }
    }

    void releaseSoundPool() {
        synchronized (this) {
            if (mSoundPool != null) {
                mSoundPool.release();
                mSoundPool = null;
            }
        }
    }

    void registerForAirplaneMode(IntentFilter filter) {
        final String airplaneModeRadios = Settings.System.getString(mContentResolver,
                Settings.Global.AIRPLANE_MODE_RADIOS);
        final String toggleableRadios = Settings.System.getString(mContentResolver,
                Settings.Global.AIRPLANE_MODE_TOGGLEABLE_RADIOS);

        mIsAirplaneSensitive = airplaneModeRadios == null ? true :
                airplaneModeRadios.contains(Settings.Global.RADIO_NFC);
        mIsAirplaneToggleable = toggleableRadios == null ? false :
            toggleableRadios.contains(Settings.Global.RADIO_NFC);

        if (mIsAirplaneSensitive) {
            filter.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
        }
    }
    void updatePackageCache() {
        PackageManager pm = mContext.getPackageManager();
        List<PackageInfo> packages = pm.getInstalledPackages(0, UserHandle.USER_OWNER);
        synchronized (this) {
            mInstalledPackages = packages;
        }
    }

    int doOpenSecureElementConnection() {
        mEeWakeLock.acquire();
        try {
            return mSecureElement.doOpenSecureElementConnection();
        } finally {
            mEeWakeLock.release();
        }
    }

    byte[] doTransceive(int handle, byte[] cmd) {
        mEeWakeLock.acquire();
        try {
            return doTransceiveNoLock(handle, cmd);
        } finally {
            mEeWakeLock.release();
        }
    }

    byte[] doTransceiveNoLock(int handle, byte[] cmd) {
        return mSecureElement.doTransceive(handle, cmd);
    }

    void doDisconnect(int handle) {
        mEeWakeLock.acquire();
        try {
            mSecureElement.doDisconnect(handle);
        } finally {
            mEeWakeLock.release();
        }
    }

    boolean doReset(int handle) {
            mEeWakeLock.acquire();
            try {
                return mSecureElement.doReset(handle);
            } finally {
                mEeWakeLock.release();
            }
    }

    public static byte[] CreateSHA(String pkg, int alaVer){
        String TAG = "Utils:CreateSHA";
        StringBuffer sb = new StringBuffer();
        try {
            MessageDigest md;
            if(alaVer == 1)
            md = MessageDigest.getInstance("SHA-256");
            else
            md = MessageDigest.getInstance("SHA-1");

            md.update(pkg.getBytes());

            byte byteData[] = md.digest();
            Log.i(TAG, "byteData len : " + byteData.length);
            /*
            for (int i = 0; i < byteData.length; i++) {
                sb.append(Integer.toString((byteData[i] & 0xff) + 0x100, 16).substring(1));
            }
            //  Log.i(TAG, "sb.toString()" + sb.toString());*/
            return byteData;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    public static String getCallingAppPkg(Context context) {
        String TAG = "getCallingAppPkg";
        ActivityManager am = (ActivityManager) context.getSystemService(context.ACTIVITY_SERVICE);

        // get the info from the currently running task
        List< ActivityManager.RunningTaskInfo > taskInfo = am.getRunningTasks(1);

        Log.d("topActivity", "CURRENT Activity ::"
                + taskInfo.get(0).topActivity.getClassName());
        String s = taskInfo.get(0).topActivity.getClassName();

        ComponentName componentInfo = taskInfo.get(0).topActivity;
        componentInfo.getPackageName();
        Log.i(TAG,"componentInfo.getPackageName()" + componentInfo.getPackageName());
        return componentInfo.getPackageName();
    }

    /**
     * Manages tasks that involve turning on/off the NFC controller.
     * <p/>
     * <p>All work that might turn the NFC adapter on or off must be done
     * through this task, to keep the handling of mState simple.
     * In other words, mState is only modified in these tasks (and we
     * don't need a lock to read it in these tasks).
     * <p/>
     * <p>These tasks are all done on the same AsyncTask background
     * thread, so they are serialized. Each task may temporarily transition
     * mState to STATE_TURNING_OFF or STATE_TURNING_ON, but must exit in
     * either STATE_ON or STATE_OFF. This way each task can be guaranteed
     * of starting in either STATE_OFF or STATE_ON, without needing to hold
     * NfcService.this for the entire task.
     * <p/>
     * <p>AsyncTask's are also implicitly queued. This is useful for corner
     * cases like turning airplane mode on while TASK_ENABLE is in progress.
     * The TASK_DISABLE triggered by airplane mode will be correctly executed
     * immediately after TASK_ENABLE is complete. This seems like the most sane
     * way to deal with these situations.
     * <p/>
     * <p>{@link #TASK_ENABLE} enables the NFC adapter, without changing
     * preferences
     * <p>{@link #TASK_DISABLE} disables the NFC adapter, without changing
     * preferences
     * <p>{@link #TASK_BOOT} does first boot work and may enable NFC
     */
    class EnableDisableTask extends AsyncTask<Integer, Void, Void> {
        @Override
        protected Void doInBackground(Integer... params) {
            // Sanity check mState
            switch (mState) {
                case NfcAdapter.STATE_TURNING_OFF:
                case NfcAdapter.STATE_TURNING_ON:
                    Log.e(TAG, "Processing EnableDisable task " + params[0] + " from bad state " +
                            mState);
                    return null;
            }

            /* AsyncTask sets this thread to THREAD_PRIORITY_BACKGROUND,
             * override with the default. THREAD_PRIORITY_BACKGROUND causes
             * us to service software I2C too slow for firmware download
             * with the NXP PN544.
             * TODO: move this to the DAL I2C layer in libnfc-nxp, since this
             * problem only occurs on I2C platforms using PN544
             */
            Process.setThreadPriority(Process.THREAD_PRIORITY_DEFAULT);

            switch (params[0].intValue()) {
                case TASK_ENABLE:
                    enableInternal();
                    break;
                case TASK_DISABLE:
                    disableInternal();
                    break;
                case TASK_BOOT:
                    Log.d(TAG, "checking on firmware download");
                    boolean airplaneOverride = mPrefs.getBoolean(PREF_AIRPLANE_OVERRIDE, false);
                    if (mPrefs.getBoolean(PREF_NFC_ON, NFC_ON_DEFAULT) &&
                            (!mIsAirplaneSensitive || !isAirplaneModeOn() || airplaneOverride)) {
                        Log.d(TAG, "NFC is on. Doing normal stuff");
                        enableInternal();
                    } else {
                        Log.d(TAG, "NFC is off.  Checking firmware version");
                        mDeviceHost.checkFirmware();
                    }
                    if (mPrefs.getBoolean(PREF_FIRST_BOOT, true)) {
                        Log.i(TAG, "First Boot");
                        mPrefsEditor.putBoolean(PREF_FIRST_BOOT, false);
                        mPrefsEditor.apply();
                    }
                    break;
            }

            // Restore default AsyncTask priority
            Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND);
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {

            if(mState == NfcAdapter.STATE_ON){
                IntentFilter filter = new IntentFilter(NativeNfcManager.INTERNAL_TARGET_DESELECTED_ACTION);
                filter.addAction(Intent.ACTION_SCREEN_OFF);
                filter.addAction(Intent.ACTION_SCREEN_ON);
                filter.addAction(Intent.ACTION_USER_PRESENT);
                filter.addAction(Intent.ACTION_USER_SWITCHED);
                filter.addAction(Intent.ACTION_SHUTDOWN);
                registerForAirplaneMode(filter);
                mContext.registerReceiverAsUser(mReceiver, UserHandle.ALL, filter, null, null);
            }else if (mState == NfcAdapter.STATE_OFF){
                mContext.unregisterReceiver(mReceiver);
                IntentFilter filter = new IntentFilter(NativeNfcManager.INTERNAL_TARGET_DESELECTED_ACTION);
                filter.addAction(Intent.ACTION_USER_SWITCHED);
                filter.addAction(Intent.ACTION_SHUTDOWN);
                registerForAirplaneMode(filter);
                mContext.registerReceiverAsUser(mReceiver, UserHandle.ALL, filter, null, null);
            }


        }

        /**
         * Check the default Secure Element configuration.
         */
        void checkSecureElementConfuration() {

            /* Get SE List */
            int[] Se_list = mDeviceHost.doGetSecureElementList();

            /* Check Secure Element setting */
            int Se_Num=mDeviceHost.GetDefaultSE();
            if(Se_Num != 0)
            {
                SECURE_ELEMENT_ON_DEFAULT=true;
                SECURE_ELEMENT_ID_DEFAULT=Se_Num;
            } else {
                if (Se_list != null) {
                    for (int i = 0; i < Se_list.length; i++) {
                        mDeviceHost.doDeselectSecureElement(Se_list[i]);
                    }
                }
            }

            mNfcSecureElementState =
                    mNxpPrefs.getBoolean(PREF_SECURE_ELEMENT_ON, SECURE_ELEMENT_ON_DEFAULT);

            if (mNfcSecureElementState) {
                int secureElementId =
                        mNxpPrefs.getInt(PREF_SECURE_ELEMENT_ID, SECURE_ELEMENT_ID_DEFAULT);

                if (Se_list != null) {

                    if (secureElementId != ALL_SE_ID_TYPE/* SECURE_ELEMENT_ALL */) {
                        mDeviceHost.doDeselectSecureElement(UICC_ID_TYPE);
                        mDeviceHost.doDeselectSecureElement(SMART_MX_ID_TYPE);

                        for (int i = 0; i < Se_list.length; i++) {

                            if (Se_list[i] == secureElementId) {
                                if (secureElementId == SMART_MX_ID_TYPE) { // SECURE_ELEMENT_SMX_ID
                                    if (Se_list.length > 1) {
                                        if (DBG) {
                                            Log.d(TAG, "Deselect UICC");
                                        }
                                    }
                                    Log.d(TAG, "Select SMX");
                                    mDeviceHost.doSelectSecureElement(secureElementId);
                                    mSelectedSeId = secureElementId;
                                    break;
                                } else if (secureElementId == UICC_ID_TYPE/* SECURE_ELEMENT_UICC_ID */) {
                                    if (Se_list.length > 1) {
                                        if (DBG) {
                                            Log.d(TAG, "Deselect SMX");
                                        }
                                    }
                                    Log.d(TAG, "Select UICC");
                                    mDeviceHost.doSelectSecureElement(secureElementId);
                                    mSelectedSeId = secureElementId;
                                    break;
                                } else if (secureElementId == SECURE_ELEMENT_ID_DEFAULT) {
                                    if (Se_list.length > 1) {
                                        if (DBG) {
                                            Log.d(TAG, "UICC deselected by default");
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if (DBG) {
                            Log.d(TAG, "Select ALL_SE");
                        }

                        if (Se_list.length > 1) {

                            for (int i = 0; i < Se_list.length; i++) {
                                mDeviceHost.doSelectSecureElement(Se_list[i]);
                                try{
                                    //Delay b/w two SE selection.
                                    Thread.sleep(200);
                                } catch(Exception e) {
                                    e.printStackTrace();
                                }
                            }
                            mSelectedSeId = secureElementId;
                        }
                    }
                }
            } else {
                if (Se_list != null && Se_list.length > 0) {
                    if (DBG) {
                        Log.d(TAG, "UICC/eSE deselected by default");
                    }
                    mDeviceHost.doDeselectSecureElement(UICC_ID_TYPE);
                    mDeviceHost.doDeselectSecureElement(SMART_MX_ID_TYPE);
                }
            }
        }

        boolean getJcopOsFileInfo() {
            File jcopOsFile;
            Log.i(TAG, "getJcopOsFileInfo");

            for (int num = 0; num < 3; num++) {
                try{
                    jcopOsFile = new File(path[num]);
                }catch(NullPointerException npe) {
                    Log.e(TAG,"path to jcop os file was null");
                    return false;
                }
                long modtime = jcopOsFile.lastModified();
                SharedPreferences prefs = mContext.getSharedPreferences(PREF,Context.MODE_PRIVATE);
                long prev_modtime = prefs.getLong(PREF_JCOP_MODTIME[num], JCOP_MODTIME_DEFAULT[num]);
                Log.d(TAG,"prev_modtime:" + prev_modtime);
                Log.d(TAG,"new_modtime:" + modtime);
                if(prev_modtime == modtime){
                    return false;
                }
                JCOP_MODTIME_TEMP[num] = modtime;
            }
            return true;
        }

       /* jcop os Download at boot time */
        void jcopOsDownload() {
            int status = ErrorCodes.SUCCESS;
            boolean jcopStatus;
            Log.i(TAG, "Jcop Download starts");

            SharedPreferences prefs = mContext.getSharedPreferences(PREF,Context.MODE_PRIVATE);
            jcopStatus = getJcopOsFileInfo();

            if( jcopStatus == true) {
                Log.i(TAG, "Starting getChipName");
                int Ver = mDeviceHost.getChipVer();
                if(Ver == PN66T_ID || Ver == PN65T_ID) {
                    status = mDeviceHost.JCOSDownload();
                }
                if(status != ErrorCodes.SUCCESS) {
                    Log.i(TAG, "Jcop Download failed");
                }
                else {
                    Log.i(TAG, "Jcop Download success");
                    prefs.edit().putLong(PREF_JCOP_MODTIME[0],JCOP_MODTIME_TEMP[0]).apply();
                    prefs.edit().putLong(PREF_JCOP_MODTIME[1],JCOP_MODTIME_TEMP[1]).apply();
                    prefs.edit().putLong(PREF_JCOP_MODTIME[2],JCOP_MODTIME_TEMP[2]).apply();
                }
            }
        }
        /**
         * Enable NFC adapter functions.
         * Does not toggle preferences.
         */
        boolean enableInternal() {
            if (mState == NfcAdapter.STATE_ON) {
                return true;
            }
            Log.i(TAG, "Enabling NFC");
            updateState(NfcAdapter.STATE_TURNING_ON);
            int timeout = mDeviceHost.getNfcInitTimeout();
            if (timeout < INIT_WATCHDOG_MS)
            {
                timeout = INIT_WATCHDOG_MS;
            }
            Log.i(TAG, "Enabling NFC timeout" +timeout);
            WatchDogThread watchDog = new WatchDogThread("enableInternal", timeout);
            watchDog.start();
            try {
                mRoutingWakeLock.acquire();
                try {
                    if (!mDeviceHost.initialize()) {
                        Log.w(TAG, "Error enabling NFC");
                        updateState(NfcAdapter.STATE_OFF);
                        return false;
                    }
                } finally {
                    mRoutingWakeLock.release();
                }
            } finally {
                watchDog.cancel();
            }
            checkSecureElementConfuration();

            mIsRouteForced = true;
            if (mIsHceCapable) {
                // Generate the initial card emulation routing table
                fAidTableFull = false;
                mCardEmulationManager.onNfcEnabled();
            }
            mIsRouteForced = false;

            synchronized (NfcService.this) {
                mObjectMap.clear();
                mP2pLinkManager.enableDisable(mIsNdefPushEnabled, true);
                updateState(NfcAdapter.STATE_ON);
            }

            synchronized (NfcService.this) {
                if(mDeviceHost.doCheckJcopDlAtBoot()) {
                    /* start jcop download */
                    Log.i(TAG, "Calling Jcop Download");
                    jcopOsDownload();
                }
            }

            initSoundPool();
            /* Start polling loop */
            Log.e(TAG, "applyRouting -3");
            mScreenState = mScreenStateHelper.checkScreenState();
            mDeviceHost.doSetScreenOrPowerState(mScreenState);
            mIsRoutingTableDirty = true;
            applyRouting(true);
            return true;
        }

        /**
         * Disable all NFC adapter functions.
         * Does not toggle preferences.
         */
        boolean disableInternal() {
            if (mState == NfcAdapter.STATE_OFF) {
                return true;
            }
            Log.i(TAG, "Disabling NFC");
            updateState(NfcAdapter.STATE_TURNING_OFF);

            /* Sometimes mDeviceHost.deinitialize() hangs, use a watch-dog.
             * Implemented with a new thread (instead of a Handler or AsyncTask),
             * because the UI Thread and AsyncTask thread-pools can also get hung
             * when the NFC controller stops responding */
            WatchDogThread watchDog = new WatchDogThread("disableInternal", ROUTING_WATCHDOG_MS);
            watchDog.start();

            if (mIsHceCapable) {
                mCardEmulationManager.onNfcDisabled();
            }

            mP2pLinkManager.enableDisable(false, false);

            /* The NFC-EE may still be opened by another process,
             * and a transceive() could still be in progress on
             * another Binder thread.
             * Give it a while to finish existing operations
             * before we close it.
             */
            Long startTime = SystemClock.elapsedRealtime();
            do {
                synchronized (NfcService.this) {
                    if (mOpenEe == null)
                        break;
                }
                try {
                    Thread.sleep(WAIT_FOR_NFCEE_POLL_MS);
                } catch (InterruptedException e) {
                    // Ignore
                }
            } while (SystemClock.elapsedRealtime() - startTime < WAIT_FOR_NFCEE_OPERATIONS_MS);

            synchronized (NfcService.this) {
                if (mOpenEe != null) {
                    try {
                        _nfcEeClose(-1, mOpenEe.binder);
                    } catch (IOException e) { }
                }
            }

            // Stop watchdog if tag present
            // A convenient way to stop the watchdog properly consists of
            // disconnecting the tag. The polling loop shall be stopped before
            // to avoid the tag being discovered again.
            maybeDisconnectTarget();

            mNfcDispatcher.setForegroundDispatch(null, null, null);

            boolean result = mDeviceHost.deinitialize();
            if (DBG) Log.d(TAG, "mDeviceHost.deinitialize() = " + result);

            watchDog.cancel();

            synchronized (NfcService.this) {
                mCurrentDiscoveryParameters = NfcDiscoveryParameters.getNfcOffParameters();
                updateState(NfcAdapter.STATE_OFF);
            }

            releaseSoundPool();

            return result;
        }

        void updateState(int newState) {
            synchronized (NfcService.this) {
                if (newState == mState) {
                    return;
                }
                /*Added for Loader service recover during NFC Off/On*/
                if(newState == NfcAdapter.STATE_ON){
                    NfcAlaService nas = new NfcAlaService();
                    nas.LSReexecute();
                    IntentFilter lsFilter = new IntentFilter(NfcAdapter.ACTION_ADAPTER_STATE_CHANGED);
                    mContext.registerReceiverAsUser(mAlaReceiver, UserHandle.ALL, lsFilter, null, null);
                }
                mState = newState;
                Intent intent = new Intent(NfcAdapter.ACTION_ADAPTER_STATE_CHANGED);
                intent.setFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
                intent.putExtra(NfcAdapter.EXTRA_ADAPTER_STATE, mState);
                mContext.sendBroadcastAsUser(intent, UserHandle.CURRENT);
            }
        }
    }

    void saveNfcOnSetting(boolean on) {
        synchronized (NfcService.this) {
            mPrefsEditor.putBoolean(PREF_NFC_ON, on);
            mPrefsEditor.apply();
        }
    }

    public void playSound(int sound) {
        synchronized (this) {
            if (mSoundPool == null) {
                Log.w(TAG, "Not playing sound when NFC is disabled");
                return;
            }
            switch (sound) {
                case SOUND_START:
                    mSoundPool.play(mStartSound, 1.0f, 1.0f, 0, 0, 1.0f);
                    break;
                case SOUND_END:
                    mSoundPool.play(mEndSound, 1.0f, 1.0f, 0, 0, 1.0f);
                    break;
                case SOUND_ERROR:
                    mSoundPool.play(mErrorSound, 1.0f, 1.0f, 0, 0, 1.0f);
                    break;
            }
        }
    }

    synchronized int getUserId() {
        return mUserId;
    }

    void setBeamShareActivityState(boolean enabled) {
        UserManager um = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
        // Propagate the state change to all user profiles related to the current
        // user. Note that the list returned by getUserProfiles contains the
        // current user.
        List <UserHandle> luh = um.getUserProfiles();
        for (UserHandle uh : luh){
            enforceBeamShareActivityPolicy(mContext, uh, enabled);
        }
    }

    void enforceBeamShareActivityPolicy(Context context, UserHandle uh,
            boolean isGlobalEnabled){
        UserManager um = (UserManager) context.getSystemService(Context.USER_SERVICE);
        IPackageManager mIpm = IPackageManager.Stub.asInterface(ServiceManager.getService("package"));
        boolean isActiveForUser =
                (!um.hasUserRestriction(UserManager.DISALLOW_OUTGOING_BEAM, uh)) &&
                isGlobalEnabled;
        if (DBG){
            Log.d(TAG, "Enforcing a policy change on user: " + uh +
                    ", isActiveForUser = " + isActiveForUser);
        }
        try {
            mIpm.setComponentEnabledSetting(new ComponentName(
                    BeamShareActivity.class.getPackageName$(),
                    BeamShareActivity.class.getName()),
                    isActiveForUser ?
                            PackageManager.COMPONENT_ENABLED_STATE_ENABLED :
                            PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                            PackageManager.DONT_KILL_APP,
                    uh.getIdentifier());
        } catch (RemoteException e) {
            Log.w(TAG, "Unable to change Beam status for user " + uh);
        }
    }

    final class NfcAdapterService extends INfcAdapter.Stub {
        @Override
        public boolean enable() throws RemoteException {
            NfcPermissions.enforceAdminPermissions(mContext);
            int val =  mDeviceHost.GetDefaultSE();
            Log.i(TAG, "getDefaultSE " + val);

            saveNfcOnSetting(true);

            if (mIsAirplaneSensitive && isAirplaneModeOn()) {
                if (!mIsAirplaneToggleable) {
                    Log.i(TAG, "denying enable() request (airplane mode)");
                    return false;
                }
                // Make sure the override survives a reboot
                mPrefsEditor.putBoolean(PREF_AIRPLANE_OVERRIDE, true);
                mPrefsEditor.apply();
            }
            new EnableDisableTask().execute(TASK_ENABLE);

            return true;
        }



        @Override
        public boolean disable(boolean saveState) throws RemoteException {
            NfcPermissions.enforceAdminPermissions(mContext);
            Log.d(TAG,"Disabling Nfc.");

            //Check if this a device shutdown or Nfc only Nfc disable.
            if(!mPowerShutDown)
            {
                Log.i(TAG, "Disabling NFC Disabling ESE/UICC");
                //Since only Nfc is getting disabled so disable CE from EE.
                mDeviceHost.doSetScreenOrPowerState(ScreenStateHelper.POWER_STATE_ON);
                mDeviceHost.doDeselectSecureElement(UICC_ID_TYPE);
                mDeviceHost.doDeselectSecureElement(SMART_MX_ID_TYPE);
            } else {
                Log.i(TAG, "Power off : Disabling NFC Disabling ESE/UICC");
                mPowerShutDown = false;
                mCardEmulationManager.onPreferredForegroundServiceChanged(null);
            }

            if (saveState) {
                saveNfcOnSetting(false);
            }

            new EnableDisableTask().execute(TASK_DISABLE);

            return true;
        }

        @Override
        public void pausePolling(int timeoutInMs) {
            NfcPermissions.enforceAdminPermissions(mContext);

            if (timeoutInMs <= 0 || timeoutInMs > MAX_POLLING_PAUSE_TIMEOUT) {
                Log.e(TAG, "Refusing to pause polling for " + timeoutInMs + "ms.");
                return;
            }

            synchronized (NfcService.this) {
                mPollingPaused = true;
                mDeviceHost.disableDiscovery();
                mHandler.sendMessageDelayed(
                        mHandler.obtainMessage(MSG_RESUME_POLLING), timeoutInMs);
            }
        }

        @Override
        public void resumePolling() {
            NfcPermissions.enforceAdminPermissions(mContext);

            synchronized (NfcService.this) {
                if (!mPollingPaused) {
                    return;
                }

                mHandler.removeMessages(MSG_RESUME_POLLING);
                mPollingPaused = false;
                new ApplyRoutingTask().execute();
            }
        }

        @Override
        public boolean isNdefPushEnabled() throws RemoteException {
            synchronized (NfcService.this) {
                return mState == NfcAdapter.STATE_ON && mIsNdefPushEnabled;
            }
        }

        @Override
        public boolean enableNdefPush() throws RemoteException {
            NfcPermissions.enforceAdminPermissions(mContext);
            synchronized (NfcService.this) {
                if (mIsNdefPushEnabled) {
                    return true;
                }
                Log.i(TAG, "enabling NDEF Push");
                mPrefsEditor.putBoolean(PREF_NDEF_PUSH_ON, true);
                mPrefsEditor.apply();
                mIsNdefPushEnabled = true;
                mDeviceHost.doEnablep2p(mIsNdefPushEnabled);
                setBeamShareActivityState(true);
                if (isNfcEnabled()) {
                    mP2pLinkManager.enableDisable(true, true);
                }
            }
            return true;
        }

        @Override
        public boolean disableNdefPush() throws RemoteException {
            NfcPermissions.enforceAdminPermissions(mContext);
            synchronized (NfcService.this) {
                if (!mIsNdefPushEnabled) {
                    return true;
                }
                Log.i(TAG, "disabling NDEF Push");
                mPrefsEditor.putBoolean(PREF_NDEF_PUSH_ON, false);
                mPrefsEditor.apply();
                mIsNdefPushEnabled = false;
                setBeamShareActivityState(false);
                if (isNfcEnabled()) {
                    mP2pLinkManager.enableDisable(false, true);
                }
            }
            return true;
        }

        @Override
        public void setForegroundDispatch(PendingIntent intent,
                IntentFilter[] filters, TechListParcel techListsParcel) {
            NfcPermissions.enforceUserPermissions(mContext);

            // Short-cut the disable path
            if (intent == null && filters == null && techListsParcel == null) {
                mNfcDispatcher.setForegroundDispatch(null, null, null);
                return;
            }

            // Validate the IntentFilters
            if (filters != null) {
                if (filters.length == 0) {
                    filters = null;
                } else {
                    for (IntentFilter filter : filters) {
                        if (filter == null) {
                            throw new IllegalArgumentException("null IntentFilter");
                        }
                    }
                }
            }

            // Validate the tech lists
            String[][] techLists = null;
            if (techListsParcel != null) {
                techLists = techListsParcel.getTechLists();
            }

            mNfcDispatcher.setForegroundDispatch(intent, filters, techLists);
        }

        @Override
        public void setAppCallback(IAppCallback callback) {
            NfcPermissions.enforceUserPermissions(mContext);

            // don't allow Beam for managed profiles, or devices with a device owner or policy owner
            UserInfo userInfo = mUserManager.getUserInfo(UserHandle.getCallingUserId());
            if(!mUserManager.hasUserRestriction(
            UserManager.DISALLOW_OUTGOING_BEAM, userInfo.getUserHandle())) {
                mP2pLinkManager.setNdefCallback(callback, Binder.getCallingUid());
            } else if (DBG) {
                Log.d(TAG, "Disabling default Beam behavior");
            }
        }

        @Override
        public INfcAdapterExtras getNfcAdapterExtrasInterface(String pkg) {
            NfcService.this.enforceNfceeAdminPerm(pkg);
            return mExtrasService;
        }

       @Override
        public void verifyNfcPermission() {
            NfcPermissions.enforceUserPermissions(mContext);
        }

        @Override
        public void invokeBeam() {
            NfcPermissions.enforceUserPermissions(mContext);

            if (mForegroundUtils.isInForeground(Binder.getCallingUid())) {
                mP2pLinkManager.onManualBeamInvoke(null);
            } else {
                Log.e(TAG, "Calling activity not in foreground.");
            }
        }

        @Override
        public void invokeBeamInternal(BeamShareData shareData) {
            NfcPermissions.enforceAdminPermissions(mContext);
            Message msg = Message.obtain();
            msg.what = MSG_INVOKE_BEAM;
            msg.obj = shareData;
            // We have to send this message delayed for two reasons:
            // 1) This is an IPC call from BeamShareActivity, which is
            //    running when the user has invoked Beam through the
            //    share menu. As soon as BeamShareActivity closes, the UI
            //    will need some time to rebuild the original Activity.
            //    Waiting here for a while gives a better chance of the UI
            //    having been rebuilt, which means the screenshot that the
            //    Beam animation is using will be more accurate.
            // 2) Similarly, because the Activity that launched BeamShareActivity
            //    with an ACTION_SEND intent is now in paused state, the NDEF
            //    callbacks that it has registered may no longer be valid.
            //    Allowing the original Activity to resume will make sure we
            //    it has a chance to re-register the NDEF message / callback,
            //    so we share the right data.
            //
            //    Note that this is somewhat of a hack because the delay may not actually
            //    be long enough for 2) on very slow devices, but there's no better
            //    way to do this right now without additional framework changes.
            mHandler.sendMessageDelayed(msg, INVOKE_BEAM_DELAY_MS);
        }

        @Override
        public INfcTag getNfcTagInterface() throws RemoteException {
            return mNfcTagService;
        }

        @Override
        public INfcCardEmulation getNfcCardEmulationInterface() {
            if (mIsHceCapable) {
                return mCardEmulationManager.getNfcCardEmulationInterface();
            } else {
                return null;
            }
        }


        @Override
        public int getState() throws RemoteException {
            synchronized (NfcService.this) {
                return mState;
            }
        }

        @Override
        protected void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
            NfcService.this.dump(fd, pw, args);
        }

        @Override
        public void dispatch(Tag tag) throws RemoteException {
            NfcPermissions.enforceAdminPermissions(mContext);
            mNfcDispatcher.dispatchTag(tag);
        }

        @Override
        public void setP2pModes(int initiatorModes, int targetModes) throws RemoteException {
            NfcPermissions.enforceAdminPermissions(mContext);
            mDeviceHost.setP2pInitiatorModes(initiatorModes);
            mDeviceHost.setP2pTargetModes(targetModes);
            applyRouting(true);
        }

        @Override
        public void setReaderMode(IBinder binder, IAppCallback callback, int flags, Bundle extras)
                throws RemoteException {
            synchronized (NfcService.this) {
                if (flags != 0) {
                    try {
                        mReaderModeParams = new ReaderModeParams();
                        mReaderModeParams.callback = callback;
                        mReaderModeParams.flags = flags;
                        mReaderModeParams.presenceCheckDelay = extras != null
                                ? (extras.getInt(NfcAdapter.EXTRA_READER_PRESENCE_CHECK_DELAY,
                                        DEFAULT_PRESENCE_CHECK_DELAY))
                                : DEFAULT_PRESENCE_CHECK_DELAY;
                        binder.linkToDeath(mReaderModeDeathRecipient, 0);
                    } catch (RemoteException e) {
                        Log.e(TAG, "Remote binder has already died.");
                        return;
                    }
                } else {
                    try {
                        mReaderModeParams = null;
                        binder.unlinkToDeath(mReaderModeDeathRecipient, 0);
                    } catch (NoSuchElementException e) {
                        Log.e(TAG, "Reader mode Binder was never registered.");
                    }
                }
                Log.e(TAG, "applyRouting -4");
                applyRouting(false);
            }
        }


        @Override
        public void addNfcUnlockHandler(INfcUnlockHandler unlockHandler, int[] techList) {
            NfcPermissions.enforceAdminPermissions(mContext);

            int lockscreenPollMask = computeLockscreenPollMask(techList);
            synchronized (NfcService.this) {
                mNfcUnlockManager.addUnlockHandler(unlockHandler, lockscreenPollMask);
            }

            applyRouting(false);
        }

        @Override
        public void removeNfcUnlockHandler(INfcUnlockHandler token) throws RemoteException {
            synchronized (NfcService.this) {
                mNfcUnlockManager.removeUnlockHandler(token.asBinder());
            }

            applyRouting(false);
        }
        private int computeLockscreenPollMask(int[] techList) {

            Map<Integer, Integer> techCodeToMask = new HashMap<Integer, Integer>();

            techCodeToMask.put(TagTechnology.NFC_A, NfcService.NFC_POLL_A);
            techCodeToMask.put(TagTechnology.NFC_B, NfcService.NFC_POLL_B);
            techCodeToMask.put(TagTechnology.NFC_V, NfcService.NFC_POLL_ISO15693);
            techCodeToMask.put(TagTechnology.NFC_F, NfcService.NFC_POLL_F);
            techCodeToMask.put(TagTechnology.NFC_BARCODE, NfcService.NFC_POLL_KOVIO);
            techCodeToMask.put(TagTechnology.MIFARE_CLASSIC, NfcService.NFC_POLL_A);
            techCodeToMask.put(TagTechnology.MIFARE_ULTRALIGHT, NfcService.NFC_POLL_A);

            int mask = 0;

            for (int i = 0; i < techList.length; i++) {
                if (techCodeToMask.containsKey(techList[i])) {
                    mask |= techCodeToMask.get(techList[i]).intValue();
                }
            }

            return mask;
        }
        @Override
        public IBinder getNfcAdapterVendorInterface(String vendor) {
            return mNxpNfcAdapter;
        }
    }
    final class NxpNfcAdapterService extends INxpNfcAdapter.Stub {
        @Override
        public INxpNfcAdapterExtras getNxpNfcAdapterExtrasInterface() throws RemoteException {
            return mNxpExtrasService;
        }
        @Override
        public IeSEClientServicesAdapter getNfcEseClientServicesAdapterInterface() {
            if(mEseClientServicesAdapter == null){
                mEseClientServicesAdapter = new EseClientServicesAdapter();
            }
            return mEseClientServicesAdapter;
        }

        //GSMA Changes
        @Override
        public INxpNfcController getNxpNfcControllerInterface() {
            return mNxpNfcController.getNxpNfcControllerInterface();
        }

        @Override
        public INfcDta getNfcDtaInterface() {
            NfcPermissions.enforceAdminPermissions(mContext);
            //begin
            if(mDtaService == null){
                mDtaService = new NfcDtaService();
            }
            //end
            return mDtaService;
        }
        @Override
        public INfcVzw getNfcVzwInterface() {
            NfcPermissions.enforceAdminPermissions(mContext);
            //begin
            if(mVzwService == null){
                mVzwService = new NfcVzwService();
            }
            //end
            return mVzwService;
        }

        @Override
        public int setEmvCoPollProfile(boolean enable, int route) throws RemoteException {
            return mDeviceHost.setEmvCoPollProfile(enable, route);
        }

        @Override
        public int[] getSecureElementList(String pkg) throws RemoteException {
            NfcService.this.enforceNfcSeAdminPerm(pkg);

            int[] list = null;
            if (isNfcEnabled()) {
                list = mDeviceHost.doGetSecureElementList();
            }
            return list;
        }

        @Override
        public int[] getActiveSecureElementList(String pkg) throws RemoteException {
            NfcService.this.enforceNfcSeAdminPerm(pkg);

            int[] list = null;
            if (isNfcEnabled()) {
                list = mDeviceHost.doGetActiveSecureElementList();
            }
            for(int i=0; i< list.length; i++) {
                Log.d(TAG, "Active element = "+ list[i]);
            }
            return list;
        }

        public INxpNfcAccessExtras getNxpNfcAccessExtrasInterface(String pkg) {
            NfcService.this.enforceNfcSccAdminPerm(pkg);
            Log.d(TAG, "getNxpNfcAccessExtrasInterface1");
            if(mNfcAccessExtrasService == null){
                mNfcAccessExtrasService = new NfcAccessExtrasService();
            }
            return mNfcAccessExtrasService;
        }

        @Override
        public int getSelectedSecureElement(String pkg) throws RemoteException {
            NfcService.this.enforceNfcSeAdminPerm(pkg);
            return mSelectedSeId;
        }
        @Override
        public int deselectSecureElement(String pkg) throws RemoteException {
            NfcService.this.enforceNfcSeAdminPerm(pkg);

            // Check if NFC is enabled
            if (!isNfcEnabled()) {
                return ErrorCodes.ERROR_NOT_INITIALIZED;
            }

            if (mSelectedSeId == 0) {
                return ErrorCodes.ERROR_NO_SE_CONNECTED;
            }

            if (mSelectedSeId != ALL_SE_ID_TYPE/* SECURE_ELEMENT_ALL */) {
                mDeviceHost.doDeselectSecureElement(mSelectedSeId);
            } else {

                /* Get SE List */
                int[] Se_list = mDeviceHost.doGetSecureElementList();

                for (int i = 0; i < Se_list.length; i++) {
                    mDeviceHost.doDeselectSecureElement(Se_list[i]);
                }

             //   mDeviceHost.doSetMultiSEState(false);
            }
            mNfcSecureElementState = false;
            mSelectedSeId = 0;

            /* store preference */
            mNxpPrefsEditor.putBoolean(PREF_SECURE_ELEMENT_ON, false);
            mNxpPrefsEditor.putInt(PREF_SECURE_ELEMENT_ID, 0);
            mNxpPrefsEditor.apply();

            return ErrorCodes.SUCCESS;
        }





        @Override
        public void storeSePreference(int seId) {
            NfcPermissions.enforceAdminPermissions(mContext);
            /* store */
            Log.d(TAG, "SE Preference stored");
            mNxpPrefsEditor.putBoolean(PREF_SECURE_ELEMENT_ON, true);
            mNxpPrefsEditor.putInt(PREF_SECURE_ELEMENT_ID, seId);
            mNxpPrefsEditor.apply();
        }

        @Override
        public int selectSecureElement(String pkg,int seId) throws RemoteException {
            NfcService.this.enforceNfcSeAdminPerm(pkg);

            // Check if NFC is enabled
            if (!isNfcEnabled()) {
                return ErrorCodes.ERROR_NOT_INITIALIZED;
            }

            if (mSelectedSeId == seId) {
                return ErrorCodes.ERROR_SE_ALREADY_SELECTED;
            }

            if (mSelectedSeId != 0) {
                return ErrorCodes.ERROR_SE_CONNECTED;
            }
            /* Get SE List */
            int[] Se_list = mDeviceHost.doGetSecureElementList();

            mSelectedSeId = seId;
            if (seId != ALL_SE_ID_TYPE/* SECURE_ELEMENT_ALL */) {
                mDeviceHost.doSelectSecureElement(mSelectedSeId);
            } else {
                if (Se_list.length > 1) {
                    for (int i = 0; i < Se_list.length; i++) {
                        mDeviceHost.doSelectSecureElement(Se_list[i]);
                        try{
                            //Delay b/w two SE selection.
                            Thread.sleep(200);
                        } catch(Exception e) {
                            e.printStackTrace();
                        }
                    }
                }
            }
            /* store */
            mNxpPrefsEditor.putBoolean(PREF_SECURE_ELEMENT_ON, true);
            mNxpPrefsEditor.putInt(PREF_SECURE_ELEMENT_ID, mSelectedSeId);
            mNxpPrefsEditor.apply();

            mNfcSecureElementState = true;

            return ErrorCodes.SUCCESS;
        }

        public void MifareDesfireRouteSet(int routeLoc, boolean fullPower, boolean lowPower, boolean noPower)
        throws RemoteException
        {
            int protoRouteEntry = 0;
            protoRouteEntry=((routeLoc & 0x03)== 0x01) ? (0x01 << 3) : (((routeLoc & 0x03) == 0x02) ? (0x01 << 4) : 0x00) ;
            protoRouteEntry |= ((fullPower ? 0x01 : 0) | (lowPower ? 0x01 << 1 :0 ) | (noPower ? 0x01 << 2 :0) );
            mNxpPrefsEditor = mNxpPrefs.edit();
            mNxpPrefsEditor.putInt("PREF_MIFARE_DESFIRE_PROTO_ROUTE_ID", protoRouteEntry);
            mNxpPrefsEditor.commit();
            Log.i(TAG,"MifareDesfireRouteSet function in");
            commitRouting();
        }

        public void DefaultRouteSet(int routeLoc, boolean fullPower, boolean lowPower, boolean noPower)
                throws RemoteException
        {
            if (mIsHceCapable) {
                int protoRouteEntry = 0;
                protoRouteEntry=((routeLoc & 0x03)== 0x01) ? (0x01 << 3) : (((routeLoc & 0x03) == 0x02) ? (0x01 << 4) : 0x00) ;
                protoRouteEntry |= ((fullPower ? 0x01 : 0) | (lowPower ? 0x01 << 1 :0 ) | (noPower ? 0x01 << 2 :0));
                if(GetDefaultRouteLoc() != routeLoc)
                {
                    mNxpPrefsEditor = mNxpPrefs.edit();
                    mNxpPrefsEditor.putInt("PREF_SET_DEFAULT_ROUTE_ID", protoRouteEntry );
                    mNxpPrefsEditor.commit();
                    mIsRouteForced = true;
                    if (mIsHceCapable) {
                        mAidRoutingManager.onNfccRoutingTableCleared();
                        mCardEmulationManager.onRoutingTableChanged();
                    }
                    mIsRouteForced = false;
                    /*As commit routing is already taken care
                    in onRoutingTableChanged once*/
                    //commitRouting();
                }
            }
            else{
                Log.i(TAG,"DefaultRoute can not be set. mIsHceCapable = flase");
            }
        }

        public void MifareCLTRouteSet(int routeLoc, boolean fullPower, boolean lowPower, boolean noPower)
        throws RemoteException
        {
            int techRouteEntry=0;
            techRouteEntry=((routeLoc & 0x03)== 0x01) ? (0x01 << 3) : (((routeLoc & 0x03) == 0x02) ? (0x01 << 4) : 0x00) ;
            techRouteEntry |= ((fullPower ? 0x01 : 0) | (lowPower ? 0x01 << 1 :0 ) | (noPower ? 0x01 << 2 :0) );
            techRouteEntry |=0x20;
            mNxpPrefsEditor = mNxpPrefs.edit();
            mNxpPrefsEditor.putInt("PREF_MIFARE_CLT_ROUTE_ID", techRouteEntry);
            mNxpPrefsEditor.commit();
            commitRouting();
        }
        @Override
        public byte[] getFWVersion()
        {
            byte[] buf = new byte[2];
            Log.i(TAG, "Starting getFwVersion");
            int fwver = mDeviceHost.getFWVersion();
            buf[0] = (byte)((fwver&0xFF00)>>8);
            buf[1] = (byte)((fwver&0xFF));
            Log.i(TAG, "Firmware version is 0x"+ buf[0]+" 0x"+buf[1]);
            return buf;
        }

        @Override
        public Map<String,Integer> getServicesAidCacheSize(int userId, String category){
            return mCardEmulationManager.getServicesAidCacheSize(userId, category);
        }

        @Override
        public int updateServiceState(int userId , Map serviceState) {
            return mCardEmulationManager.updateServiceState(userId ,serviceState);
        }

        @Override
        public int getSeInterface(int type) throws RemoteException {
            return mDeviceHost.doGetSeInterface(type);
        }

//       @Override
//       public boolean setVzwAidList(RouteEntry[] entries) throws RemoteException {
//           Log.i(TAG,"setVzwAidList - enter");
//
//           if(mIsSmartCardServiceSupported == false) {
//               return false;
//           }
//
//           Log.i(TAG,"setVzwAidList - entries length = " + entries.length);
//           mVzwFilterList = new HashMap<String, RouteEntry>(entries.length);
//
//           for (int i = 0; i < entries.length; i++) {
//               RouteEntry routeEntry = entries[i];
//               Log.i(TAG,"AID " + toHexString(routeEntry.getAid(),0,routeEntry.getAid().length).toUpperCase() + "  : location " + routeEntry.getLocation() +
//                       " : powerstate = " + routeEntry.getPowerState());
//
//               mVzwFilterList.put(toHexString(routeEntry.getAid(),0,routeEntry.getAid().length).toUpperCase(), routeEntry);
//           }
//
//           Set<String> aids = mAppsAidCache.keySet();
//           for (Iterator iterator = aids.iterator(); iterator.hasNext();) {
//               String appAid = (String) iterator.next();
//               Log.i(TAG,"APP AID :" + appAid);
//
//               if(mVzwFilterList.containsKey(appAid)) {
//                   Log.i(TAG,"APP AID :" + appAid + "  VZW AID");
//
//                   if(mVzwFilterList.get(appAid).isAllowed()) {
//                       Log.i(TAG,"APP AID :" + appAid + "  VZW AID ALLOWED");
//
//                       /* This is a verizone AID and allowed.*/
//                       Message msg = mHandler.obtainMessage();
//                       msg.what = MSG_VZW_ROUTE_AID;
//                       msg.arg1 = mVzwFilterList.get(appAid).getLocation();
//                       msg.arg2 = mVzwFilterList.get(appAid).getPowerState();
//                       msg.obj = mVzwFilterList.get(appAid).getAid();
//                       //send the msg to os handler.
//                       mHandler.sendMessage(msg);
//                   } else {
//                       Log.i(TAG,"APP AID :" + appAid + "  VZW AID NOT ALLOWED");
//                       /* This is a verizone AID and NOT allowed.*/
//                       /* Ignore this AID.*/
//                   }
//               } else {
//                   Log.i(TAG,"APP AID :" + appAid + "  APP AID ALLOWED");
//
//                   Message msg = mHandler.obtainMessage();
//                   msg.what = MSG_VZW_ROUTE_AID;
//
//                   //Non VZW AID, treat normally.
//                   msg.arg1 = mAppsAidCache.get(appAid).getLocation();
//                   msg.arg2 = mAppsAidCache.get(appAid).getPowerState();
//                   msg.obj = mAppsAidCache.get(appAid).getAid();
//                   //send the msg to os handler.
//                   mHandler.sendMessage(msg);
//
//               }
//
//           }
//           mHandler.sendEmptyMessage(MSG_VZW_COMMIT_ROUTING);
//           return true;
//       }

//        @Override
//        public void setScreenOffCondition(boolean enable)  throws RemoteException{
//
//            if(mIsSmartCardServiceSupported == false) {
//                return;
//            }
//
//            if(enable && mScreenState <= SCREEN_STATE_ON_LOCKED) {
//                //Do Nothing. Already taken care by NfcService.
//            } else if(enable == false && mScreenState == SCREEN_STATE_ON_UNLOCKED){
//                //Do Nothing. Already taken care by NfcService.
//            } else {
//                if(enable) {
//                    mScreenState = SCREEN_STATE_OFF;
//                    new ApplyRoutingTask().execute(Integer.valueOf(mScreenState));
//                } else {
//                    mScreenState = SCREEN_STATE_ON_UNLOCKED;
//                    new ApplyRoutingTask().execute(Integer.valueOf(mScreenState));
//                }
//
//                //Never come here.
//            }
//        }
    }

    final class ReaderModeDeathRecipient implements IBinder.DeathRecipient {
        @Override
        public void binderDied() {
            synchronized (NfcService.this) {
                if (mReaderModeParams != null) {
                    mReaderModeParams = null;
                    Log.e(TAG, "applyRouting -5");
                    applyRouting(false);
                }
            }
        }
    }

    final class TagService extends INfcTag.Stub {
        @Override
        public int close(int nativeHandle) throws RemoteException {
            NfcPermissions.enforceUserPermissions(mContext);

            TagEndpoint tag = null;

            if (!isNfcEnabled()) {
                return ErrorCodes.ERROR_NOT_INITIALIZED;
            }

            /* find the tag in the hmap */
            tag = (TagEndpoint) findObject(nativeHandle);
            if (tag != null) {
                /* Remove the device from the hmap */
                unregisterObject(nativeHandle);
                tag.disconnect();
                return ErrorCodes.SUCCESS;
            }
            /* Restart polling loop for notification */
            Log.e(TAG, "applyRouting -6");
            applyRouting(true);
            return ErrorCodes.ERROR_DISCONNECT;
        }

        @Override
        public int connect(int nativeHandle, int technology) throws RemoteException {
            NfcPermissions.enforceUserPermissions(mContext);

            TagEndpoint tag = null;

            if (!isNfcEnabled()) {
                return ErrorCodes.ERROR_NOT_INITIALIZED;
            }

            /* find the tag in the hmap */
            tag = (TagEndpoint) findObject(nativeHandle);
            if (tag == null) {
                return ErrorCodes.ERROR_DISCONNECT;
            }

            if (!tag.isPresent()) {
                return ErrorCodes.ERROR_DISCONNECT;
            }

            // Note that on most tags, all technologies are behind a single
            // handle. This means that the connect at the lower levels
            // will do nothing, as the tag is already connected to that handle.
            if (tag.connect(technology)) {
                return ErrorCodes.SUCCESS;
            } else {
                return ErrorCodes.ERROR_DISCONNECT;
            }
        }

        @Override
        public int reconnect(int nativeHandle) throws RemoteException {
            NfcPermissions.enforceUserPermissions(mContext);

            TagEndpoint tag = null;

            // Check if NFC is enabled
            if (!isNfcEnabled()) {
                return ErrorCodes.ERROR_NOT_INITIALIZED;
            }

            /* find the tag in the hmap */
            tag = (TagEndpoint) findObject(nativeHandle);
            if (tag != null) {
                if (tag.reconnect()) {
                    return ErrorCodes.SUCCESS;
                } else {
                    return ErrorCodes.ERROR_DISCONNECT;
                }
            }
            return ErrorCodes.ERROR_DISCONNECT;
        }

        @Override
        public int[] getTechList(int nativeHandle) throws RemoteException {
            NfcPermissions.enforceUserPermissions(mContext);

            // Check if NFC is enabled
            if (!isNfcEnabled()) {
                return null;
            }

            /* find the tag in the hmap */
            TagEndpoint tag = (TagEndpoint) findObject(nativeHandle);
            if (tag != null) {
                return tag.getTechList();
            }
            return null;
        }

        @Override
        public boolean isPresent(int nativeHandle) throws RemoteException {
            TagEndpoint tag = null;

            // Check if NFC is enabled
            if (!isNfcEnabled()) {
                return false;
            }

            /* find the tag in the hmap */
            tag = (TagEndpoint) findObject(nativeHandle);
            if (tag == null) {
                return false;
            }

            return tag.isPresent();
        }

        @Override
        public boolean isNdef(int nativeHandle) throws RemoteException {
            NfcPermissions.enforceUserPermissions(mContext);

            TagEndpoint tag = null;

            // Check if NFC is enabled
            if (!isNfcEnabled()) {
                return false;
            }

            /* find the tag in the hmap */
            tag = (TagEndpoint) findObject(nativeHandle);
            int[] ndefInfo = new int[2];
            if (tag == null) {
                return false;
            }
            return tag.checkNdef(ndefInfo);
        }

        @Override
        public TransceiveResult transceive(int nativeHandle, byte[] data, boolean raw)
                throws RemoteException {
            NfcPermissions.enforceUserPermissions(mContext);

            TagEndpoint tag = null;
            byte[] response;

            // Check if NFC is enabled
            if (!isNfcEnabled()) {
                return null;
            }

            /* find the tag in the hmap */
            tag = (TagEndpoint) findObject(nativeHandle);
            if (tag != null) {
                // Check if length is within limits
                if (data.length > getMaxTransceiveLength(tag.getConnectedTechnology())) {
                    return new TransceiveResult(TransceiveResult.RESULT_EXCEEDED_LENGTH, null);
                }
                int[] targetLost = new int[1];
                response = tag.transceive(data, raw, targetLost);
                int result;
                if (response != null) {
                    result = TransceiveResult.RESULT_SUCCESS;
                } else if (targetLost[0] == 1) {
                    result = TransceiveResult.RESULT_TAGLOST;
                } else {
                    result = TransceiveResult.RESULT_FAILURE;
                }
                return new TransceiveResult(result, response);
            }
            return null;
        }

        @Override
        public NdefMessage ndefRead(int nativeHandle) throws RemoteException {
            NfcPermissions.enforceUserPermissions(mContext);

            TagEndpoint tag;

            // Check if NFC is enabled
            if (!isNfcEnabled()) {
                return null;
            }

            /* find the tag in the hmap */
            tag = (TagEndpoint) findObject(nativeHandle);
            if (tag != null) {
                byte[] buf = tag.readNdef();
                if (buf == null) {
                    return null;
                }

                /* Create an NdefMessage */
                try {
                    return new NdefMessage(buf);
                } catch (FormatException e) {
                    return null;
                }
            }
            return null;
        }

        @Override
        public int ndefWrite(int nativeHandle, NdefMessage msg) throws RemoteException {
            NfcPermissions.enforceUserPermissions(mContext);

            TagEndpoint tag;

            // Check if NFC is enabled
            if (!isNfcEnabled()) {
                return ErrorCodes.ERROR_NOT_INITIALIZED;
            }

            /* find the tag in the hmap */
            tag = (TagEndpoint) findObject(nativeHandle);
            if (tag == null) {
                return ErrorCodes.ERROR_IO;
            }

            if (msg == null) return ErrorCodes.ERROR_INVALID_PARAM;

            if (tag.writeNdef(msg.toByteArray())) {
                return ErrorCodes.SUCCESS;
            } else {
                return ErrorCodes.ERROR_IO;
            }

        }

        @Override
        public boolean ndefIsWritable(int nativeHandle) throws RemoteException {
            throw new UnsupportedOperationException();
        }

        @Override
        public int ndefMakeReadOnly(int nativeHandle) throws RemoteException {
            NfcPermissions.enforceUserPermissions(mContext);

            TagEndpoint tag;

            // Check if NFC is enabled
            if (!isNfcEnabled()) {
                return ErrorCodes.ERROR_NOT_INITIALIZED;
            }

            /* find the tag in the hmap */
            tag = (TagEndpoint) findObject(nativeHandle);
            if (tag == null) {
                return ErrorCodes.ERROR_IO;
            }

            if (tag.makeReadOnly()) {
                return ErrorCodes.SUCCESS;
            } else {
                return ErrorCodes.ERROR_IO;
            }
        }

        @Override
        public int formatNdef(int nativeHandle, byte[] key) throws RemoteException {
            NfcPermissions.enforceUserPermissions(mContext);

            TagEndpoint tag;

            // Check if NFC is enabled
            if (!isNfcEnabled()) {
                return ErrorCodes.ERROR_NOT_INITIALIZED;
            }

            /* find the tag in the hmap */
            tag = (TagEndpoint) findObject(nativeHandle);
            if (tag == null) {
                return ErrorCodes.ERROR_IO;
            }

            if (tag.formatNdef(key)) {
                return ErrorCodes.SUCCESS;
            } else {
                return ErrorCodes.ERROR_IO;
            }
        }

        @Override
        public Tag rediscover(int nativeHandle) throws RemoteException {
            NfcPermissions.enforceUserPermissions(mContext);

            TagEndpoint tag = null;

            // Check if NFC is enabled
            if (!isNfcEnabled()) {
                return null;
            }

            /* find the tag in the hmap */
            tag = (TagEndpoint) findObject(nativeHandle);
            if (tag != null) {
                // For now the prime usecase for rediscover() is to be able
                // to access the NDEF technology after formatting without
                // having to remove the tag from the field, or similar
                // to have access to NdefFormatable in case low-level commands
                // were used to remove NDEF. So instead of doing a full stack
                // rediscover (which is poorly supported at the moment anyway),
                // we simply remove these two technologies and detect them
                // again.
                tag.removeTechnology(TagTechnology.NDEF);
                tag.removeTechnology(TagTechnology.NDEF_FORMATABLE);
                tag.findAndReadNdef();
                // Build a new Tag object to return
                Tag newTag = new Tag(tag.getUid(), tag.getTechList(),
                        tag.getTechExtras(), tag.getHandle(), this);
                return newTag;
            }
            return null;
        }

        @Override
        public int setTimeout(int tech, int timeout) throws RemoteException {
            NfcPermissions.enforceUserPermissions(mContext);
            boolean success = mDeviceHost.setTimeout(tech, timeout);
            if (success) {
                return ErrorCodes.SUCCESS;
            } else {
                return ErrorCodes.ERROR_INVALID_PARAM;
            }
        }

        @Override
        public int getTimeout(int tech) throws RemoteException {
            NfcPermissions.enforceUserPermissions(mContext);

            return mDeviceHost.getTimeout(tech);
        }

        @Override
        public void resetTimeouts() throws RemoteException {
            NfcPermissions.enforceUserPermissions(mContext);

            mDeviceHost.resetTimeouts();
        }

        @Override
        public boolean canMakeReadOnly(int ndefType) throws RemoteException {
            return mDeviceHost.canMakeReadOnly(ndefType);
        }

        @Override
        public int getMaxTransceiveLength(int tech) throws RemoteException {
            return mDeviceHost.getMaxTransceiveLength(tech);
        }

        @Override
        public boolean getExtendedLengthApdusSupported() throws RemoteException {
            return mDeviceHost.getExtendedLengthApdusSupported();
        }
    }

    final class NfcJcopService extends IJcopService.Stub{

        public int jcopOsDownload(String pkg) throws RemoteException
        {
            NfcService.this.enforceNfceeAdminPerm(pkg);
            int status = ErrorCodes.SUCCESS;
            boolean mode;
            mode = mDeviceHost.doCheckJcopDlAtBoot();
            if(mode == false) {
                Log.i(TAG, "Starting getChipName");
                int Ver = mDeviceHost.getChipVer();
                if(Ver == PN66T_ID || Ver == PN65T_ID) {
                    status = mDeviceHost.JCOSDownload();
                }
            }
            else {
                status = ErrorCodes.ERROR_NOT_SUPPORTED;
            }
            return status;
        }
    }
    final class EseClientServicesAdapter extends IeSEClientServicesAdapter.Stub{
        @Override
        public IJcopService getJcopService() {
            if(mJcopService == null){
                mJcopService = new NfcJcopService();
            }
            return mJcopService;
        }
        @Override
        public ILoaderService getLoaderService() {
            if(mAlaService == null){
                mAlaService = new NfcAlaService();
            }
            return mAlaService;
        }
        @Override
        public INxpExtrasService getNxpExtrasService() {
            if(mNxpExtras == null){
                mNxpExtras = new NxpExtrasService();
            }
            return mNxpExtras;
        }
    };

    final class NfcAlaService extends ILoaderService.Stub{
        private boolean isRecovery = false;
        private String appName = null;
        private String srcIn = null;
        private String respOut = null;
        private String status = "false";

        void NfcAlaService()
        {
            appName = null;
            srcIn = null;
            respOut = null;
            status = "false";
            isRecovery = false;
        }
        private synchronized void LSReexecute()
        {
             byte[] ret = {0x4E,0x02,(byte)0x69,(byte)0x87};
             byte retry = LS_RETRY_CNT;
             PrintWriter out= null;
             BufferedReader br = null;
             Log.i(TAG, "Enter: NfcAlaService constructor");
             try{
             File f = new File(LS_BACKUP_PATH);

             Log.i(TAG, "Enter: NfcAlaService constructor file open");
             /*If the file does not exists*/
             if(!(f.isFile()))
             {
                 Log.i(TAG, "FileNotFound ls backup");
                 out = new PrintWriter(LS_BACKUP_PATH);
                 this.status = "true";
                 out.println("null");
                 out.println("null");
                 out.println("null");
                 out.println("true");
                 out.close();
             }
             /*If the file exists*/
             else
             {
                 Log.i(TAG, "File Found ls backup");
                 br = new BufferedReader(new FileReader(LS_BACKUP_PATH));
                 this.appName = br.readLine();
                 this.srcIn = br.readLine();
                 this.respOut = br.readLine();
                 this.status = br.readLine();
             }
             }catch(FileNotFoundException f)
             {
                 Log.i(TAG, "FileNotFoundException Raised during LS Initialization");
                 return;
             }
             catch(IOException ie)
             {
                 Log.i(TAG, "IOException Raised during LS Initialization ");
                 return;
             }
             finally{
                 try{
                 if(out != null)
                 out.close();
                 if(br != null)
                     br.close();
                 }catch(IOException e)
                 {
                     Log.i(TAG, "IOException Raised during LS Initialization ");
                     return;
                 }
             }
             if(this.status.equals("true"))
             {
                 Log.i(TAG, "LS Script execution completed");
             }
             else
             {
                 Log.i(TAG, "LS Script execution aborted or tear down happened");
                 Log.i(TAG, "Input script path"+ this.srcIn);
                 Log.i(TAG, "Output response path"+ this.respOut);
                 Log.i(TAG, "Application name which invoked LS"+ this.appName);
                 try{
                 File fSrc = new File(this.srcIn);
                 File fRsp = new File(this.respOut);
                 if((fSrc.isFile() && fRsp.isFile()))
                 {
                     byte[] lsAppletStatus = {0x6F,0x00};
                         /*Perform lsExecuteScript on tear down*/
                         WatchDogThread watchDog =
                               new WatchDogThread("Loader service ", INIT_WATCHDOG_LS_MS);
                         watchDog.start();
                         try {
                             mRoutingWakeLock.acquire();
                              try {
                                  /*Reset retry counter*/
                                  while((retry-- > 0))
                                  {
                                      Thread.sleep(1000);
                                      lsAppletStatus = mNfcAla.doLsGetAppletStatus();
                                      if((lsAppletStatus[0]==0x63) && (lsAppletStatus[1]==0x40))
                                      {
                                          Log.i(TAG, "Started LS recovery since previous session failed");
                                          this.isRecovery = true;
                                          ret = this.lsExecuteScript(this.srcIn, this.respOut);
                                      }
                                      else
                                      {
                                          break;
                                      }
                                  }
                                  } finally {
                                      this.isRecovery = false;
                                      watchDog.cancel();
                                      mRoutingWakeLock.release();
                                }
                              }
                         catch(RemoteException ie)
                         {
                             Log.i(TAG, "LS recovery Exception: ");
                         }
                         lsAppletStatus = mNfcAla.doLsGetAppletStatus();
                         if((lsAppletStatus[0]==(byte)0x90)&&(lsAppletStatus[1]==(byte)0x00))
                         {
                            out = new PrintWriter(LS_BACKUP_PATH);
                            out.println("null");
                            out.println("null");
                            out.println("null");
                            out.println("true");
                            out.close();
                            Log.i(TAG, "Commiting Default Values of Loader Service AS RETRY ENDS: ");
                         }
             }
             else
             {
                 Log.i(TAG, "LS recovery not required");
             }
            }catch(InterruptedException re)
            {
                /*Retry failed todo*/
                Log.i(TAG, "RemoteException while LS recovery");
            }catch(FileNotFoundException re)
            {
                /*Retry failed todo*/
                Log.i(TAG, "InterruptException while LS recovery");
            }
           }
         }
         private void updateLoaderService() {
             byte[] ret = {0x4E,0x02,(byte)0x69,(byte)0x87};
            Log.i(TAG, "Enter: NfcAlaService constructor file open");
             File f = new File(LS_UPDATE_BACKUP_PATH);
            /*If the file does not exists*/
            if((f.isFile()))
            {
                Log.i(TAG, "File Found LS update required");
                WatchDogThread watchDog =
                       new WatchDogThread("LS Update Loader service ", (INIT_WATCHDOG_LS_MS+INIT_WATCHDOG_LS_MS));
               watchDog.start();
               try {
                       try {
                               /*Reset retry counter*/
                               {
                                       Log.i(TAG, "Started LS update");
                                       ret = this.lsExecuteScript(LS_UPDATE_BACKUP_PATH, LS_UPDATE_BACKUP_OUT_PATH);
                                       if(ret[2] == (byte)(0x90) && ret[3] == (byte)(0x00))
                                       {
                                               Log.i(TAG, " LS update successfully completed");
                                               f.delete();
                                       } else {
                                               Log.i(TAG, " LS update failed");
                                       }
                               }
                       } finally {
                               watchDog.cancel();
                       }
               }
               catch(RemoteException ie)
               {
                       Log.i(TAG, "LS update recovery Exception: ");
               }
            }
            /*If the file exists*/
            else
            {
                Log.i(TAG, "No LS update");
            }
       }

        public byte[] lsExecuteScript( String srcIn, String respOut) throws RemoteException {
            String pkg_name = getCallingAppPkg(mContext);
            byte[] sha_data = CreateSHA(pkg_name, 2);
            byte[] respSW = {0x4e,0x02,0x69,(byte)0x87};
            InputStream is = null;
            OutputStream os = null;
            PrintWriter out = null;
            FileReader fr = null;
            byte[] buffer = new byte[1024];
            int length = 0;
            String srcBackup = srcIn+"mw";
            String rspBackup = null;
            File rspFile = null;
            if(respOut != null)
            rspBackup = respOut+"mw";
            File srcFile = new File(srcBackup);
            if(respOut != null)
            rspFile = new File(rspBackup);


            /*If Previously Tear down happened before completion of LS execution*/
            if(this.isRecovery != false)
            {
                try{
                fr =  new FileReader(LS_BACKUP_PATH);
                if(fr != null)
                {
                    BufferedReader br = new BufferedReader(fr);
                    if(br != null)
                    {
                        String appName = br.readLine();
                        if(appName != null)
                        {
                            sha_data = CreateSHA(appName, 2);
                            pkg_name = appName;
                        }
                    }
                   }
                }catch(IOException ioe)
                {
                    Log.i(TAG, "IOException thrown for opening ");
                }
                finally{
                    try{
                    if(fr != null)
                        fr.close();
                    }
                    catch(IOException e)
                    {
                        Log.i(TAG, "IOException thrown for opening ");
                    }
                }
            }
            /*Store it in File*/
            try{
            out = new PrintWriter(LS_BACKUP_PATH);
            out.println(pkg_name);
            out.println(srcIn);
            out.println(respOut);
            out.println(false);
            }catch(IOException fe)
            {
                Log.i(TAG, "IOException thrown during clearing ");
            }
            finally{
                if(out != null)
                out.close();
            }
            try{
                /*To avoid rewriting of backup file*/
                if(!(this.isRecovery)){
                is = new FileInputStream(srcIn);
                os = new FileOutputStream(srcBackup);

                while((length = is.read(buffer))>0)
                {
                    os.write(buffer,0,length);
                }
                if(is != null)is.close();
                if(os != null)os.close();}
                Log.i(TAG, "sha_data len : " + sha_data.length);
                Log.i(TAG, "Calling package APP Name is "+ pkg_name);
                if(sha_data != null)
                {
                    respSW = mNfcAla.doLsExecuteScript(srcBackup, rspBackup, sha_data);
                }
                /*resp file is not null*/
                if(respOut != null){
                    is = new FileInputStream(rspBackup);
                    os = new FileOutputStream(respOut);
                    length = 0;
                    while((length = is.read(buffer))>0)
                    {
                       os.write(buffer,0,length);
                    }
                }
                if(is != null)is.close();
                if(os != null)os.close();
            }catch(IOException ioe)
            {
                Log.i(TAG, "LS File not found");
            }catch(SecurityException se)
            {
                Log.i(TAG, "LS File access permission deneied");
            }
            finally{
            byte[] status = mNfcAla.doLsGetStatus();
            Log.i(TAG, "LS getStatus return SW1 : "+status[0]);
            Log.i(TAG, "LS getStatus return  SW2: "+status[1]);
            if((status[0]== (byte)0x90) && (status[1] == 0x00))
            {
                try{
                out = new PrintWriter(LS_BACKUP_PATH);
                out.println("null");
                out.println("null");
                out.println("null");
                out.println("true");
                }catch(IOException fe)
                {
                    Log.i(TAG, "FileNotFoundException thrown during clearing ");
                }
                finally
                {
                    if(out != null)
                    out.close();
                }
                Log.i(TAG, "COMMITTING THE DEFAULT VALUES OF LS : ");
                srcFile.delete();
                rspFile.delete();
            }
            else
            {
                Log.i(TAG, "NOT COMMITTING THE DEFAULT VALUES OF LS : ");
            }
          }
            return respSW;
        }
        public byte[] lsGetVersion()
        {
            byte[] respApdu = {0x4e,0x02,0x69,(byte)0x87};

            respApdu = mNfcAla.doLsGetVersion();
            return respApdu;
        }
        public int appletLoadApplet(String pkg, String choice) throws RemoteException {
            String pkg_name = getCallingAppPkg(mContext);
            int state = 0;
            byte[] sha_data = CreateSHA(pkg_name, 1);
            Log.i(TAG, "sha_data len : " + sha_data.length);

            if(sha_data != null)
            {
                state = mNfcAla.doAppletLoadApplet(choice, sha_data);
                return state;
            }
            else
                return 0xFF;
        }
        public int getListofApplets(String pkg, String[] name) throws RemoteException {
            int cnt = mNfcAla.GetAppletsList(name);
            Log.i(TAG, "GetListofApplets count : " + cnt);
            for(int i=0;i<cnt;i++) {
                Log.i(TAG, "GetListofApplets " + name[i]);
            }

            return cnt;
        }

        //TODO: Just Stub for compilation.
        public byte[] getKeyCertificate() throws RemoteException {
            return null;
        }

    };

    //    public byte[] getKeyCertificate() throws RemoteException {
      //      return mNfcAla.GetCertificateKey();
       // }
   // }
    final class NxpExtrasService extends INxpExtrasService.Stub {
        private Bundle writeNoException() {
            Bundle p = new Bundle();
            p.putInt("e", 0);
            return p;
        }

        private Bundle writeEeException(int exceptionType, String message) {
            Bundle p = new Bundle();
            p.putInt("e", exceptionType);
            p.putString("m", message);
            return p;
        }

        @Override
        public boolean isEnabled()
        {
            try {
                return (mState == NfcAdapter.STATE_ON);
            } catch (Exception e) {
                Log.d(TAG, "Exception " + e.getMessage());
                return false;
            }
        }

        @Override
        public byte[] getSecureElementUid(String pkg) throws RemoteException {
            NfcService.this.enforceNfceeAdminPerm(pkg);
            return mDeviceHost.getSecureElementUid();
        }

        @Override
        public Bundle open(String pkg, IBinder b) throws RemoteException {
            NfcService.this.enforceNfceeAdminPerm(pkg);

            Bundle result;
            int handle = _open(b);
            if (handle < 0) {
                result = writeEeException(handle, "NFCEE open exception.");
            } else {
                result = writeNoException();
            }
            return result;
        }

        /**
         * Opens a connection to the secure element.
         *
         * @return A handle with a value >= 0 in case of success, or a
         *         negative value in case of failure.
         */
        private int _open(IBinder b) {
            synchronized(NfcService.this) {
                if (!isNfcEnabled()) {
                    return EE_ERROR_NFC_DISABLED;
                }
                if (mInProvisionMode) {
                    // Deny access to the NFCEE as long as the device is being setup
                    return EE_ERROR_IO;
                }
                if (mP2pLinkManager.isLlcpActive()) {
                    // Don't allow PN544-based devices to open the SE while the LLCP
                    // link is still up or in a debounce state. This avoids race
                    // conditions in the NXP stack around P2P/SMX switching.
                    return EE_ERROR_EXT_FIELD;
                }
                if (mOpenEe != null) {
                    return EE_ERROR_ALREADY_OPEN;
                }

                boolean restorePolling = false;
                if (mNfcPollingEnabled) {
                    // Disable polling for tags/P2P when connecting to the SMX
                    // on PN544-based devices. Whenever nfceeClose is called,
                    // the polling configuration will be restored.
                    mDeviceHost.disableDiscovery();
                    mNfcPollingEnabled = false;
                    restorePolling = true;
                }
                int handle = doOpenSecureElementConnection();
                if (handle < 0) {

                    if (restorePolling) {
                        mDeviceHost.enableDiscovery(mCurrentDiscoveryParameters, true);
                        mNfcPollingEnabled = true;
                    }
                    return handle;
                }
                mDeviceHost.setTimeout(TagTechnology.ISO_DEP, 30000);

                mOpenEe = new OpenSecureElement(getCallingPid(), handle, b);
                try {
                    b.linkToDeath(mOpenEe, 0);
                } catch (RemoteException e) {
                    mOpenEe.binderDied();
                }

                // Add the calling package to the list of packages that have accessed
                // the secure element.
                for (String packageName : mContext.getPackageManager().getPackagesForUid(getCallingUid())) {
                    mSePackages.add(packageName);
                }

                return handle;
           }
        }

        @Override
        public Bundle close(String pkg, IBinder binder) throws RemoteException {
            NfcService.this.enforceNfceeAdminPerm(pkg);

            Bundle result;
            try {
                _nfcEeClose(getCallingPid(), binder);
                result = writeNoException();
            } catch (IOException e) {
                result = writeEeException(EE_ERROR_IO, e.getMessage());
            }
            return result;
        }
        @Override
        public Bundle transceive(String pkg, byte[] in) throws RemoteException {
            NfcService.this.enforceNfceeAdminPerm(pkg);

            Bundle result;
            byte[] out;
            try {
                out = _transceive(in);
                result = writeNoException();
                result.putByteArray("out", out);
            } catch (IOException e) {
                result = writeEeException(EE_ERROR_IO, e.getMessage());
            }
            return result;
        }

        private byte[] _transceive(byte[] data) throws IOException {
            synchronized(NfcService.this) {
                if (!isNfcEnabled()) {
                    throw new IOException("NFC is not enabled");
                }
                if (mOpenEe == null) {
                    throw new IOException("NFC EE is not open");
                }
                if (getCallingPid() != mOpenEe.pid) {
                    throw new SecurityException("Wrong PID");
                }
            }

            return doTransceive(mOpenEe.handle, data);
        }
    };
    final class NfcDtaService extends INfcDta.Stub {

        public boolean snepDtaCmd(String cmdType, String serviceName, int serviceSap, int miu, int rwSize, int testCaseId) throws RemoteException
        {
            NfcPermissions.enforceAdminPermissions(mContext);
            if(cmdType.equals(null))
                return false;
            if(cmdType.equals("enabledta") && (!mIsDtaMode)) {
                mDeviceHost.enableDtaMode();
                mIsDtaMode = true;
                Log.d(TAG, "DTA Mode is Enabled ");
            }else if(cmdType.equals("disableDta") && (mIsDtaMode)) {
                mDeviceHost.disableDtaMode();
                mIsDtaMode = false;
            } else if(cmdType.equals("enableserver")) {
                if(serviceName.equals(null))
                    return false;
                mP2pLinkManager.enableExtDtaSnepServer(serviceName, serviceSap, miu, rwSize,testCaseId);
            } else if(cmdType.equals("disableserver")) {
                mP2pLinkManager.disableExtDtaSnepServer();
            } else if(cmdType.equals("enableclient")) {
               if(testCaseId == 0)
                    return false;
                if(testCaseId>20){
                    mIsShortRecordLayout=true;
                    testCaseId=testCaseId-20;
                }else{
                    mIsShortRecordLayout=false;
                }
                Log.d("testCaseId", ""+testCaseId);
                mP2pLinkManager.enableDtaSnepClient(serviceName, miu, rwSize, testCaseId);
            } else if(cmdType.equals("disableclient")) {
               mP2pLinkManager.disableDtaSnepClient();
            } else {
                Log.d(TAG, "Unkown DTA Command");
                return false;
            }
            return true;
        }

    };

    final class NfcVzwService extends INfcVzw.Stub {
        @Override
        public void setScreenOffCondition(boolean enable) throws RemoteException {

               Message msg = mHandler.obtainMessage();
               msg.what=MSG_SET_SCREEN_STATE;
               msg.arg1= (enable)?1:0;
               mHandler.sendMessage(msg);

        }

        @Override
        public boolean setVzwAidList(RouteEntry[] entries)
                throws RemoteException {
            // enforceAdminPerm(mContext);
            Log.i(TAG, "setVzwAidList enter");
            Log.i(TAG, "setVzwAidList  entries length =" + entries.length);
            if (mIsHceCapable) {
                mAidRoutingManager.ClearVzwCache();
                for (int i = 0; i < entries.length; i++) {
                    RouteEntry routeEntry = entries[i];
                    mAidRoutingManager.UpdateVzwCache(routeEntry.getAid(),
                            routeEntry.getLocation(), routeEntry.getPowerState(),
                            routeEntry.isAllowed());

                    Log.i(TAG,
                            "AID" + routeEntry.getAid() + "Location "
                                    + routeEntry.getLocation() + "powerstate "
                                    + routeEntry.getPowerState());
                }
                mAidRoutingManager.onNfccRoutingTableCleared();
                mCardEmulationManager.onRoutingTableChanged();
                return true;
            } else {
                return false;
            }
            // TO-DO
        }

    };

    void _nfcEeClose(int callingPid, IBinder binder) throws IOException {
        // Blocks until a pending open() or transceive() times out.
        //TODO: This is incorrect behavior - the close should interrupt pending
        // operations. However this is not supported by current hardware.

        synchronized (NfcService.this) {
            if (!isNfcEnabledOrShuttingDown()) {
                throw new IOException("NFC adapter is disabled");
            }
            if (mOpenEe == null) {
                throw new IOException("NFC EE closed");
            }
            if (callingPid != -1 && callingPid != mOpenEe.pid) {
                throw new SecurityException("Wrong PID");
            }
            if (mOpenEe.binder != binder) {
                throw new SecurityException("Wrong binder handle");
            }

            binder.unlinkToDeath(mOpenEe, 0);
            mDeviceHost.resetTimeouts();
            doDisconnect(mOpenEe.handle);
            mOpenEe = null;
            //Log.e(TAG, "applyRouting -8");
            //applyRouting(true);
        }
    }

    boolean _nfcEeReset() throws IOException {
       synchronized (NfcService.this) {
           if (!isNfcEnabledOrShuttingDown()) {
              throw new IOException("NFC adapter is disabled");
           }
           if (mOpenEe == null) {
              throw new IOException("NFC EE closed");
           }
           return mSecureElement.doReset(mOpenEe.handle);
       }
    }

    final class NfcAccessExtrasService extends INxpNfcAccessExtras.Stub {
            public boolean checkChannelAdminAccess(String pkg) throws RemoteException {
                boolean result = true;
                try {
                NfcService.this.enforceNfcSccAdminPerm(pkg);
                } catch (Exception e) {
                    e.printStackTrace();
                    result = false;
                }
                return result;
            }
        };

    final class NfcAdapterExtrasService extends INfcAdapterExtras.Stub {
        private Bundle writeNoException() {
            Bundle p = new Bundle();
            p.putInt("e", 0);
            return p;
        }

        private Bundle writeEeException(int exceptionType, String message) {
            Bundle p = new Bundle();
            p.putInt("e", exceptionType);
            p.putString("m", message);
            return p;
        }

        @Override
        public Bundle open(String pkg, IBinder b) throws RemoteException {
            NfcService.this.enforceNfceeAdminPerm(pkg);

            Bundle result;
            int handle = _open(b);
            if (handle < 0) {
                result = writeEeException(handle, "NFCEE open exception.");
            } else {
                result = writeNoException();
            }
            return result;
        }

        /**
         * Opens a connection to the secure element.
         *
         * @return A handle with a value >= 0 in case of success, or a
         *         negative value in case of failure.
         */
        private int _open(IBinder b) {
            synchronized(NfcService.this) {
                if (!isNfcEnabled()) {
                    return EE_ERROR_NFC_DISABLED;
                }
                if (mInProvisionMode) {
                    // Deny access to the NFCEE as long as the device is being setup
                    return EE_ERROR_IO;
                }
                if (mP2pLinkManager.isLlcpActive()) {
                    // Don't allow PN544-based devices to open the SE while the LLCP
                    // link is still up or in a debounce state. This avoids race
                    // conditions in the NXP stack around P2P/SMX switching.
                    return EE_ERROR_EXT_FIELD;
                }
                if (mOpenEe != null) {
                    Log.i(TAG, "SE is Busy. returning..");
                    return EE_ERROR_ALREADY_OPEN;
                }
                boolean restorePolling = false;
                if (mNfcPollingEnabled) {
                    // Disable polling for tags/P2P when connecting to the SMX
                    // on PN544-based devices. Whenever nfceeClose is called,
                    // the polling configuration will be restored.
                    mDeviceHost.disableDiscovery();
                    mNfcPollingEnabled = false;
                    restorePolling = true;
                }

                int handle = doOpenSecureElementConnection();
                if (handle < 0) {

                    if (restorePolling) {
                        mDeviceHost.enableDiscovery(mCurrentDiscoveryParameters, true);
                        mNfcPollingEnabled = true;
                    }
                    return handle;
                }
                mDeviceHost.setTimeout(TagTechnology.ISO_DEP, 30000);
                mOpenEe = new OpenSecureElement(getCallingPid(), handle, b);
                try {
                    b.linkToDeath(mOpenEe, 0);
                } catch (RemoteException e) {
                    mOpenEe.binderDied();
                }

                // Add the calling package to the list of packages that have accessed
                // the secure element.
                for (String packageName : mContext.getPackageManager().getPackagesForUid(getCallingUid())) {
                    mSePackages.add(packageName);
                }

                return handle;
           }
        }

        @Override
        public Bundle close(String pkg, IBinder binder) throws RemoteException {
            NfcService.this.enforceNfceeAdminPerm(pkg);

            Bundle result;
            try {
                _nfcEeClose(getCallingPid(), binder);
                result = writeNoException();
            } catch (IOException e) {
                result = writeEeException(EE_ERROR_IO, e.getMessage());
            }
            return result;
        }

//        @Override
//        public boolean reset(String pkg) throws RemoteException {
//            NfcService.this.enforceNfceeAdminPerm(pkg);
//            Bundle result;
//            boolean stat = false;
//            try {
//                stat = _nfcEeReset();
//                result = writeNoException();
//            } catch (IOException e) {
//                result = writeEeException(EE_ERROR_IO, e.getMessage());
//            }
//            return stat;
//        }

        @Override
        public Bundle transceive(String pkg, byte[] in) throws RemoteException {
            NfcService.this.enforceNfceeAdminPerm(pkg);

            Bundle result;
            byte[] out;
            try {
                out = _transceive(in);
                result = writeNoException();
                result.putByteArray("out", out);
            } catch (IOException e) {
                result = writeEeException(EE_ERROR_IO, e.getMessage());
            }
            return result;
        }

        private byte[] _transceive(byte[] data) throws IOException {
            synchronized(NfcService.this) {
                if (!isNfcEnabled()) {
                    throw new IOException("NFC is not enabled");
                }
                if (mOpenEe == null) {
                    throw new IOException("NFC EE is not open");
                }
                if (getCallingPid() != mOpenEe.pid) {
                    throw new SecurityException("Wrong PID");
                }
            }

            return doTransceive(mOpenEe.handle, data);
        }

        //@Override
        //public Bundle getAtr(String pkg) throws RemoteException {
          //  NfcService.this.enforceNfceeAdminPerm(pkg);

            //Bundle result;
            //byte[] out;
            //try {
              //  out = _getAtr();
               // result = writeNoException();
               // result.putByteArray("out", out);
           // } catch (IOException e) {
             //   result = writeEeException(EE_ERROR_IO, e.getMessage());
            //}
            //return result;
        //}
        //private byte[] _getAtr() throws IOException {
         //   synchronized(NfcService.this) {
           //     if (!isNfcEnabled()) {
             //       throw new IOException("NFC is not enabled");
              //  }
                //if (mOpenEe == null) {
                  //  throw new IOException("NFC EE is not open");
               // }
                //if (getCallingPid() != mOpenEe.pid) {
                  //  throw new SecurityException("Wrong PID");
                //}
           // }
            //return mSecureElement.doGetAtr(mOpenEe.handle);
        //}

        @Override
        public int getCardEmulationRoute(String pkg) throws RemoteException {
            NfcService.this.enforceNfceeAdminPerm(pkg);
            return mEeRoutingState;
        }

        @Override
        public void setCardEmulationRoute(String pkg, int route) throws RemoteException {
            NfcService.this.enforceNfceeAdminPerm(pkg);
            mEeRoutingState = route;
            ApplyRoutingTask applyRoutingTask = new ApplyRoutingTask();
            applyRoutingTask.execute();
            try {
                // Block until route is set
                applyRoutingTask.get();
            } catch (ExecutionException e) {
                Log.e(TAG, "failed to set card emulation mode");
            } catch (InterruptedException e) {
                Log.e(TAG, "failed to set card emulation mode");
            }
        }

        @Override
        public void authenticate(String pkg, byte[] token) throws RemoteException {
            NfcService.this.enforceNfceeAdminPerm(pkg);
        }

        @Override
        public String getDriverName(String pkg) throws RemoteException {
            NfcService.this.enforceNfceeAdminPerm(pkg);
            return mDeviceHost.getName();
        }

    }
    final class NxpNfcAdapterExtrasService extends INxpNfcAdapterExtras.Stub {
        private Bundle writeNoException() {
            Bundle p = new Bundle();
            p.putInt("e", 0);
            return p;
        }

        private Bundle writeEeException(int exceptionType, String message) {
            Bundle p = new Bundle();
            p.putInt("e", exceptionType);
            p.putString("m", message);
            return p;
        }
        @Override

        public boolean reset(String pkg) throws RemoteException {
            NfcService.this.enforceNfceeAdminPerm(pkg);
            Bundle result;
            boolean stat = false;
            try {
                stat = _nfcEeReset();
                result = writeNoException();
            } catch (IOException e) {
                result = writeEeException(EE_ERROR_IO, e.getMessage());
            }
            Log.d(TAG,"reset" + stat);
            return stat;
        }

        boolean _nfcEeReset() throws IOException {
            synchronized (NfcService.this) {
                if (!isNfcEnabledOrShuttingDown()) {
                   throw new IOException("NFC adapter is disabled");
                }
                if (mOpenEe == null) {
                   throw new IOException("NFC EE closed");
                }
                return mSecureElement.doReset(mOpenEe.handle);
            }
         }

        @Override
        public int getSecureElementTechList(String pkg) throws RemoteException {
            NfcService.this.enforceNfceeAdminPerm(pkg);
            return mDeviceHost.doGetSecureElementTechList();
        }

        @Override
        public byte[] getSecureElementUid(String pkg) throws RemoteException {
            NfcService.this.enforceNfceeAdminPerm(pkg);
            return mDeviceHost.getSecureElementUid();
        }

        @Override
        public void notifyCheckCertResult(String pkg, boolean success)
                throws RemoteException {
            if (DBG) Log.d(TAG, "notifyCheckCertResult() " + pkg + ", success=" + success);

            NfcService.this.enforceNfceeAdminPerm(pkg);
            //NfcPermissions.enforceAdminPermissions(mContext);
            mNxpNfcController.setResultForX509Certificates(success);
/*            synchronized (mWaitOMACheckCert) {
                if (mWaitOMACheckCert != null) {
                    if (success) {
                        mHasOMACert = true;
                    } else {
                        mHasOMACert = false;
                    }
                    mWaitOMACheckCert.notify();
                }
            }*/
        }

        @Override
        public void deliverSeIntent(String pkg, Intent seIntent)
                throws RemoteException {
            Log.d(TAG, "deliverSeIntent() " + pkg + " " + seIntent.getAction());
            NfcService.this.enforceNfceeAdminPerm(pkg);
            //NfcPermissions.enforceAdminPermissions(mContext);
            sendMessage(MSG_SE_DELIVER_INTENT, seIntent);
        }

        @Override
        public byte[] doGetRouting() throws RemoteException {
            return mDeviceHost.doGetRouting();
        }

        @Override
        public Bundle getAtr(String pkg) throws RemoteException {
            NfcService.this.enforceNfceeAdminPerm(pkg);

            Bundle result;
            byte[] out;
            try {
                out = _getAtr();
                result = writeNoException();
                result.putByteArray("out", out);
            } catch (IOException e) {
                result = writeEeException(EE_ERROR_IO, e.getMessage());
            }
            Log.d(TAG,"getAtr result " + result);
            return result;
        }

        private byte[] _getAtr() throws IOException {
            synchronized(NfcService.this) {
                if (!isNfcEnabled()) {
                    throw new IOException("NFC is not enabled");
                }
                if (mOpenEe == null) {
                    throw new IOException("NFC EE is not open");
                }
                if (getCallingPid() != mOpenEe.pid) {
                    throw new SecurityException("Wrong PID");
                }
            }
            return mSecureElement.doGetAtr(mOpenEe.handle);
        }


    }

    /** resources kept while secure element is open */
    private class OpenSecureElement implements IBinder.DeathRecipient {
        public int pid;  // pid that opened SE
        // binder handle used for DeathReceipient. Must keep
        // a reference to this, otherwise it can get GC'd and
        // the binder stub code might create a different BinderProxy
        // for the same remote IBinder, causing mismatched
        // link()/unlink()
        public IBinder binder;
        public int handle; // low-level handle
        public OpenSecureElement(int pid, int handle, IBinder binder) {
            this.pid = pid;
            this.handle = handle;
            this.binder = binder;
        }
        @Override
        public void binderDied() {
            synchronized (NfcService.this) {
                Log.i(TAG, "Tracked app " + pid + " died");
                pid = -1;
                try {
                    _nfcEeClose(-1, binder);
                } catch (IOException e) { /* already closed */ }
            }
        }
        @Override
        public String toString() {
            return new StringBuilder('@').append(Integer.toHexString(hashCode())).append("[pid=")
                    .append(pid).append(" handle=").append(handle).append("]").toString();
        }
    }

    boolean isNfcEnabledOrShuttingDown() {
        synchronized (this) {
            return (mState == NfcAdapter.STATE_ON || mState == NfcAdapter.STATE_TURNING_OFF);
        }
    }

    boolean isNfcEnabled() {
        synchronized (this) {
            return mState == NfcAdapter.STATE_ON;
        }
    }

    class WatchDogThread extends Thread {
        final Object mCancelWaiter = new Object();
        final int mTimeout;
        boolean mCanceled = false;

        public WatchDogThread(String threadName, int timeout) {
            super(threadName);
            mTimeout = timeout;
        }

        @Override
        public void run() {
            try {
                synchronized (mCancelWaiter) {
                    mCancelWaiter.wait(mTimeout);
                    if (mCanceled) {
                        return;
                    }
                }
            } catch (InterruptedException e) {
                // Should not happen; fall-through to abort.
                Log.w(TAG, "Watchdog thread interruped.");
                interrupt();
            }
            Log.e(TAG, "Watchdog triggered, aborting.");
            mDeviceHost.doAbort();
        }

        public synchronized void cancel() {
            synchronized (mCancelWaiter) {
                mCanceled = true;
                mCancelWaiter.notify();
            }
        }
    }

    /* For Toast from background process*/

    public class ToastHandler
    {
        // General attributes
        private Context mContext;
        private Handler mHandler;

        public ToastHandler(Context _context)
        {
        this.mContext = _context;
        this.mHandler = new Handler();
        }

        /**
         * Runs the <code>Runnable</code> in a separate <code>Thread</code>.
         *
         * @param _runnable
         *            The <code>Runnable</code> containing the <code>Toast</code>
         */
        private void runRunnable(final Runnable _runnable)
        {
        Thread thread = new Thread()
        {
            public void run()
            {
            mHandler.post(_runnable);
            }
        };

        thread.start();
        thread.interrupt();
        thread = null;
        }

        public void showToast(final CharSequence _text, final int _duration)
        {
        final Runnable runnable = new Runnable()
        {
            @Override
            public void run()
            {
            Toast.makeText(mContext, _text, _duration).show();
            }
        };

        runRunnable(runnable);
        }
    }

    static byte[] hexStringToBytes(String s) {
        if (s == null || s.length() == 0) return null;
        int len = s.length();
        if (len % 2 != 0) {
            s = '0' + s;
            len++;
        }
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
                    + Character.digit(s.charAt(i + 1), 16));
        }
        return data;
    }

    static String toHexString(byte[] buffer, int offset, int length) {
        final char[] HEX_CHARS = "0123456789abcdef".toCharArray();
        char[] chars = new char[2 * length];
        for (int j = offset; j < offset + length; ++j) {
            chars[2 * (j-offset)] = HEX_CHARS[(buffer[j] & 0xF0) >>> 4];
            chars[2 * (j-offset) + 1] = HEX_CHARS[buffer[j] & 0x0F];
        }
        return new String(chars);
    }

    /**
     * Read mScreenState and apply NFC-C polling and NFC-EE routing
     */
    void applyRouting(boolean force) {
        Log.d(TAG, "applyRouting - enter force = " + force + " mScreenState = " + mScreenState);

        synchronized (this) {
            //Since Reader mode during wired mode is supported
            //enableDiscovery or disableDiscovery is allowed
            //if (!isNfcEnabledOrShuttingDown() || mOpenEe != null) {
            if (!isNfcEnabledOrShuttingDown()) {
                // PN544 cannot be reconfigured while EE is open
                return;
            }
            WatchDogThread watchDog = new WatchDogThread("applyRouting", ROUTING_WATCHDOG_MS);
            if (mInProvisionMode) {
                mInProvisionMode = Settings.Secure.getInt(mContentResolver,
                        Settings.Global.DEVICE_PROVISIONED, 0) == 0;
                if (!mInProvisionMode) {
                    // Notify dispatcher it's fine to dispatch to any package now
                    // and allow handover transfers.
                    mNfcDispatcher.disableProvisioningMode();
                    /* if provision mode is disabled, then send this info to lower layers as well */
                    mDeviceHost.doSetProvisionMode(mInProvisionMode);
                }
            }
            // Special case: if we're transitioning to unlocked state while
            // still talking to a tag, postpone re-configuration.
            if (mScreenState == ScreenStateHelper.SCREEN_STATE_ON_UNLOCKED && isTagPresent()) {
                Log.d(TAG, "Not updating discovery parameters, tag connected.");
                mHandler.sendMessageDelayed(mHandler.obtainMessage(MSG_RESUME_POLLING),
                        APPLY_ROUTING_RETRY_TIMEOUT_MS);
                return;
            }

            try {
                watchDog.start();
                // Compute new polling parameters
                NfcDiscoveryParameters newParams = computeDiscoveryParameters(mScreenState);
                if (force || !newParams.equals(mCurrentDiscoveryParameters)) {
                    if (newParams.shouldEnableDiscovery()) {
                        boolean shouldRestart = mCurrentDiscoveryParameters.shouldEnableDiscovery();
                        mDeviceHost.enableDiscovery(newParams, shouldRestart);
                    } else {
                        mDeviceHost.disableDiscovery();
                    }
                    mCurrentDiscoveryParameters = newParams;
                } else {
                    Log.d(TAG, "Discovery configuration equal, not updating.");
                }
            } finally {
                watchDog.cancel();
            }
        }
    }

    private NfcDiscoveryParameters computeDiscoveryParameters(int screenState) {
        // Recompute discovery parameters based on screen state
        NfcDiscoveryParameters.Builder paramsBuilder = NfcDiscoveryParameters.newBuilder();
        // Polling
        if (screenState >= NFC_POLLING_MODE) {
            // Check if reader-mode is enabled
            if (mReaderModeParams != null) {
                int techMask = 0;
                if ((mReaderModeParams.flags & NfcAdapter.FLAG_READER_NFC_A) != 0)
                    techMask |= NFC_POLL_A;
                if ((mReaderModeParams.flags & NfcAdapter.FLAG_READER_NFC_B) != 0)
                    techMask |= NFC_POLL_B;
                if ((mReaderModeParams.flags & NfcAdapter.FLAG_READER_NFC_F) != 0)
                    techMask |= NFC_POLL_F;
                if ((mReaderModeParams.flags & NfcAdapter.FLAG_READER_NFC_V) != 0)
                    techMask |= NFC_POLL_ISO15693;
                if ((mReaderModeParams.flags & NfcAdapter.FLAG_READER_NFC_BARCODE) != 0)
                    techMask |= NFC_POLL_KOVIO;

                paramsBuilder.setTechMask(techMask);
                paramsBuilder.setEnableReaderMode(true);
            } else {
                paramsBuilder.setTechMask(NfcDiscoveryParameters.NFC_POLL_DEFAULT);
                paramsBuilder.setEnableP2p(mIsNdefPushEnabled);
            }
        } else if (screenState == ScreenStateHelper.SCREEN_STATE_ON_LOCKED && mInProvisionMode) {
            paramsBuilder.setTechMask(NfcDiscoveryParameters.NFC_POLL_DEFAULT);
            // enable P2P for MFM/EDU/Corp provisioning
            paramsBuilder.setEnableP2p(true);
        } else if (screenState == ScreenStateHelper.SCREEN_STATE_ON_LOCKED &&
                mNfcUnlockManager.isLockscreenPollingEnabled()) {
            // For lock-screen tags, no low-power polling
            paramsBuilder.setTechMask(mNfcUnlockManager.getLockscreenPollMask());
            paramsBuilder.setEnableLowPowerDiscovery(false);
            paramsBuilder.setEnableP2p(false);
        }

        if (mIsHceCapable && mScreenState >= ScreenStateHelper.SCREEN_STATE_ON_LOCKED) {
            // Host routing is always enabled at lock screen or later
            paramsBuilder.setEnableHostRouting(true);
        }
        //To make routing table update.
        if(mIsRoutingTableDirty) {
            mIsRoutingTableDirty = false;
            //TODO: Take this logic from L_OSP_EXT [PN547C2]
            int protoRoute = mNxpPrefs.getInt("PREF_MIFARE_DESFIRE_PROTO_ROUTE_ID", GetDefaultMifareDesfireRouteEntry());
            int defaultRoute=mNxpPrefs.getInt("PREF_SET_DEFAULT_ROUTE_ID", GetDefaultRouteEntry());
            int techRoute=mNxpPrefs.getInt("PREF_MIFARE_CLT_ROUTE_ID", GetDefaultMifateCLTRouteEntry());
            Log.d(TAG, "Set default Route Entry");
            setDefaultRoute(defaultRoute, protoRoute, techRoute);
        }
/*
        //Configure for Felica Routing
        if(mScreenState >= ScreenStateHelper.SCREEN_STATE_ON_LOCKED && mIsFelicaOnHostConfiguring) {
            mIsFelicaOnHostConfiguring = false;
//            mDeviceHost.disableDiscovery();
            paramsBuilder.setTechMask(0);
            paramsBuilder.setEnableHostRouting(false);
        }

        if(mScreenState >= ScreenStateHelper.SCREEN_STATE_ON_LOCKED && mIsFelicaOnHostConfigured) {
            mIsFelicaOnHostConfigured = false;
            //mDeviceHost.enableDiscovery();
        }
*/
        return paramsBuilder.build();
    }

    private boolean isTagPresent() {
        for (Object object : mObjectMap.values()) {
            if (object instanceof TagEndpoint) {
                return ((TagEndpoint) object).isPresent();
            }
        }
        return false;
    }
    /**
     * Disconnect any target if present
     */
    void maybeDisconnectTarget() {
        if (!isNfcEnabledOrShuttingDown()) {
            return;
        }
        Object[] objectsToDisconnect;
        synchronized (this) {
            Object[] objectValues = mObjectMap.values().toArray();
            // Copy the array before we clear mObjectMap,
            // just in case the HashMap values are backed by the same array
            objectsToDisconnect = Arrays.copyOf(objectValues, objectValues.length);
            mObjectMap.clear();
        }
        for (Object o : objectsToDisconnect) {
            if (DBG) Log.d(TAG, "disconnecting " + o.getClass().getName());
            if (o instanceof TagEndpoint) {
                // Disconnect from tags
                TagEndpoint tag = (TagEndpoint) o;
                tag.disconnect();
            } else if (o instanceof NfcDepEndpoint) {
                // Disconnect from P2P devices
                NfcDepEndpoint device = (NfcDepEndpoint) o;
                if (device.getMode() == NfcDepEndpoint.MODE_P2P_TARGET) {
                    // Remote peer is target, request disconnection
                    device.disconnect();
                } else {
                    // Remote peer is initiator, we cannot disconnect
                    // Just wait for field removal
                }
            }
        }
    }

    Object findObject(int key) {
        synchronized (this) {
            Object device = mObjectMap.get(key);
            if (device == null) {
                Log.w(TAG, "Handle not found");
            }
            return device;
        }
    }

    void registerTagObject(TagEndpoint tag) {
        synchronized (this) {
            mObjectMap.put(tag.getHandle(), tag);
        }
    }

    void unregisterObject(int handle) {
        synchronized (this) {
            mObjectMap.remove(handle);
        }
    }

    /**
     * For use by code in this process
     */
    public LlcpSocket createLlcpSocket(int sap, int miu, int rw, int linearBufferLength)
            throws LlcpException {
        return mDeviceHost.createLlcpSocket(sap, miu, rw, linearBufferLength);
    }

    /**
     * For use by code in this process
     */
    public LlcpConnectionlessSocket createLlcpConnectionLessSocket(int sap, String sn)
            throws LlcpException {
        return mDeviceHost.createLlcpConnectionlessSocket(sap, sn);
    }

    /**
     * For use by code in this process
     */
    public LlcpServerSocket createLlcpServerSocket(int sap, String sn, int miu, int rw,
            int linearBufferLength) throws LlcpException {
        return mDeviceHost.createLlcpServerSocket(sap, sn, miu, rw, linearBufferLength);
    }

    public void sendMockNdefTag(NdefMessage msg) {
        sendMessage(MSG_MOCK_NDEF, msg);
    }

    public void notifyRoutingTableFull()
    {
        mNxpPrefsEditor = mNxpPrefs.edit();
        mNxpPrefsEditor.putInt("PREF_SET_AID_ROUTING_TABLE_FULL",0x01);
        mNxpPrefsEditor.commit();
        //broadcast Aid Routing Table Full intent to the user
        Intent aidTableFull = new Intent();
        aidTableFull.setAction(NxpConstants.ACTION_ROUTING_TABLE_FULL);
        if (DBG) {
            Log.d(TAG, "notify aid routing table full to the user");
        }
        mContext.sendBroadcast(aidTableFull);
    }
    /**
     * set default  Aid route entry in case application does not configure this route entry
     */
    public void setDefaultAidRouteLoc( int defaultAidRouteEntry)
    {
        mNxpPrefsEditor = mNxpPrefs.edit();
        Log.d(TAG, "writing to preferences setDefaultAidRouteLoc  :" + defaultAidRouteEntry);
        mNxpPrefsEditor.putInt("PREF_SET_DEFAULT_ROUTE_ID", ((defaultAidRouteEntry << ROUTE_LOC_MASK)| (mDeviceHost.getDefaultAidPowerState() & 0x1F)));
        mNxpPrefsEditor.commit();
        int defaultRoute=mNxpPrefs.getInt("PREF_SET_DEFAULT_ROUTE_ID",0xFF);
        Log.d(TAG, "reading preferences from user  :" + defaultRoute);
    }

    public int getAidRoutingTableSize ()
    {
        int aidTableSize = 0x00;
        aidTableSize = mDeviceHost.getAidTableSize();
        return aidTableSize;
    }

    public void routeAids(String aid, int route, int powerState) {
            Message msg = mHandler.obtainMessage();
            msg.what = MSG_ROUTE_AID;
            msg.arg1 = route;
            msg.arg2 = powerState;
            msg.obj = aid;
            mHandler.sendMessage(msg);
    }

    public void unrouteAids(String aid) {
        sendMessage(MSG_UNROUTE_AID, aid);
    }

    public void commitRouting() {
        mHandler.sendEmptyMessage(MSG_COMMIT_ROUTING);
    }
    /**
     * get default Aid route entry in case application does not configure this route entry
     */
    public int GetDefaultRouteLoc()
    {
        int defaultRouteLoc = mNxpPrefs.getInt("PREF_SET_DEFAULT_ROUTE_ID", GetDefaultRouteEntry()) >> ROUTE_LOC_MASK;
        Log.d(TAG, "GetDefaultRouteLoc  :" + defaultRouteLoc);
        return defaultRouteLoc ;
    }

    /**
     * get default MifareDesfireRoute route entry in case application does not configure this route entry
     */
    public int GetDefaultMifareDesfireRouteEntry()
    {
        //return ((ROUTE_ID_UICC << ROUTE_LOC_MASK ) | ROUTE_SWITCH_ON | ROUTE_SWITCH_OFF) ;
        Log.d(TAG, "GetDefaultMifareDesfireRouteEntry :" + mDeviceHost.getDefaultDesfireRoute() );
        return ((mDeviceHost.getDefaultDesfirePowerState() & 0x1F) | (mDeviceHost.getDefaultDesfireRoute() << ROUTE_LOC_MASK)) ;
    }
    /**
     * set default Aid route entry in case application does not configure this route entry
     */

    public int GetDefaultRouteEntry()
    {
        int defaultAidRoute = ((mDeviceHost.getDefaultAidPowerState() & 0x1F) | (mDeviceHost.getDefaultAidRoute()<< ROUTE_LOC_MASK));
        Log.d(TAG, "GetDefaultRouteEntry :" + defaultAidRoute );
        return defaultAidRoute;
    }

    /**
     * get default MifateCLT route entry in case application does not configure this route entry
     */
    public int GetDefaultMifateCLTRouteEntry()
    {
        //return ((ROUTE_ID_UICC << ROUTE_LOC_MASK ) | ROUTE_SWITCH_ON | ROUTE_SWITCH_OFF | (TECH_TYPE_A << TECH_TYPE_MASK)) ;
        Log.d(TAG, "getDefaultMifareCLTRoute :" + mDeviceHost.getDefaultMifareCLTRoute() );
        return ((mDeviceHost.getDefaultMifareCLTPowerState() & 0x1F) | (mDeviceHost.getDefaultMifareCLTRoute() << ROUTE_LOC_MASK) | (TECH_TYPE_A << TECH_TYPE_MASK)) ;
    }

    public boolean setDefaultRoute(int defaultRouteEntry, int defaultProtoRouteEntry, int defaultTechRouteEntry) {
        boolean ret = mDeviceHost.setDefaultRoute(defaultRouteEntry, defaultProtoRouteEntry, defaultTechRouteEntry);
        return ret;
    }

    public int getDefaultRoute() {
        return mNxpPrefs.getInt(PREF_DEFAULT_ROUTE_ID, DEFAULT_ROUTE_ID_DEFAULT);
    }


    public void commitingFelicaRouting() {
        mHandler.sendEmptyMessage(MSG_COMMITINF_FELICA_ROUTING);
    }

    public void commitedFelicaRouting() {
        mHandler.sendEmptyMessage(MSG_COMMITED_FELICA_ROUTING);
    }

    public int getAidRoutingTableStatus() {
        int aidTableStatus = 0x00;
        aidTableStatus = mNxpPrefs.getInt("PREF_SET_AID_ROUTING_TABLE_FULL",0x00);
        return aidTableStatus;
    }

    public void routeNfcid2(String nfcid2, String syscode, String optparam) {
        Message msg = mHandler.obtainMessage();
        msg.what = MSG_ROUTE_NFCID2;
        Bundle extras = new Bundle();
        extras.putString(Nfcid2RoutingCache.EXTRA_NFCID2, nfcid2);
        extras.putString(Nfcid2RoutingCache.EXTRA_SYSCODE, syscode);
        extras.putString(Nfcid2RoutingCache.EXTRA_OPTPARAM, optparam);
        msg.obj = extras;
        mHandler.sendMessage(msg);
    }

    public void unrouteNfcid2(String nfcid2) {
        Message msg = mHandler.obtainMessage();
        msg.what = MSG_UNROUTE_NFCID2;
        msg.obj = nfcid2;
        mHandler.sendMessage(msg);
    }

    public void clearRouting() {
        mHandler.sendEmptyMessage(MSG_CLEAR_ROUTING);
    }

    public boolean isVzwFeatureEnabled(){
        return mDeviceHost.isVzwFeatureEnabled();
    }

    public boolean sendData(byte[] data) {
        return mDeviceHost.sendRawFrame(data);
    }

    public int getDefaultSecureElement() {
        int[] seList = mDeviceHost.doGetSecureElementList();
        if ( seList == null || seList.length != 1) {
            //use default value
            return -1;
        } else {
            return seList[0];
        }
    }

    public void etsiStartConfig(int eeHandle) {
        Log.d(TAG, "etsiStartConfig Enter");

        Log.d(TAG, "etsiStartConfig : etsiInitConfig");
        mDeviceHost.etsiInitConfig();

        Log.d(TAG, "etsiStartConfig : disableDiscovery");
        mDeviceHost.disableDiscovery();

        Log.d(TAG, "etsiStartConfig : etsiReaderConfig");
        mDeviceHost.etsiReaderConfig(eeHandle);

        Log.d(TAG, "etsiStartConfig : notifyEEReaderEvent");
        mDeviceHost.notifyEEReaderEvent(ETSI_READER_REQUESTED);

        Log.d(TAG, "etsiStartConfig : setEtsiReaederState");
        mDeviceHost.setEtsiReaederState(STATE_SE_RDR_MODE_STARTED);
        //broadcast SWP_READER_ACTIVATED evt
        Intent SWPReaderRequestedIntent = new Intent();
        SWPReaderRequestedIntent.setAction(NxpConstants.ACTION_SWP_READER_REQUESTED);
        if (DBG) {
            Log.d(TAG, "SWP READER - Requested");
        }
        mContext.sendBroadcast(SWPReaderRequestedIntent);

        Log.d(TAG, "etsiStartConfig : enableDiscovery");
        mDeviceHost.enableDiscovery(mCurrentDiscoveryParameters, true);

        Log.d(TAG, "etsiStartConfig Exit");
    }

    public void etsiStopConfig(int discNtfTimeout) {
        Log.d(TAG, "etsiStopConfig Enter");
        if( mDeviceHost.getEtsiReaederState() == STATE_SE_RDR_MODE_STOP_IN_PROGRESS)
        {
            Log.d(TAG, "Attempting etsiStopConfig while STATE_SE_RDR_MODE_STOP_IN_PROGRESS. Returning..");
            return;
        }
        ETSI_STOP_CONFIG = true;
        TagRemoveTask tagRemoveTask = new TagRemoveTask();
        tagRemoveTask.execute(discNtfTimeout);
        Log.d(TAG, "etsiStopConfig : etsiInitConfig");
        mDeviceHost.etsiInitConfig();

        //mDeviceHost.setEtsiReaederState(STATE_SE_RDR_MODE_STOPPED);
        Log.d(TAG, "etsiStopConfig : disableDiscovery");
        mDeviceHost.disableDiscovery();

        if(mDeviceHost.getEtsiReaederState() == STATE_SE_RDR_MODE_STOPPED)
        {
            Log.d(TAG, "etsiStopConfig :etsi reader already Stopped. Returning..");
            ETSI_STOP_CONFIG = false;
            return;
        }
        Log.d(TAG, "etsiStopConfig : etsiResetReaderConfig");
        mDeviceHost.etsiResetReaderConfig();

        Log.d(TAG, "etsiStopConfig : notifyEEReaderEvent");
        mDeviceHost.notifyEEReaderEvent(ETSI_READER_STOP);

        Intent SWPReaderDeActivatedIntent = new Intent();

      //broadcast SWP_READER_DEACTIVATED evt
        SWPReaderDeActivatedIntent
                .setAction(NxpConstants.ACTION_SWP_READER_DEACTIVATED);
        if (DBG) {
            Log.d(TAG, "SWP READER - DeActivated");
        }
        mContext.sendBroadcast(SWPReaderDeActivatedIntent);

        Log.d(TAG, "etsiStopConfig : setEtsiReaederState");
        mDeviceHost.setEtsiReaederState(STATE_SE_RDR_MODE_STOPPED);

        Log.d(TAG, "etsiStopConfig : enableDiscovery");
        mDeviceHost.enableDiscovery(mCurrentDiscoveryParameters, true);
        ETSI_STOP_CONFIG = false;

        Log.d(TAG, "etsiStopConfig : updateScreenState");
        mDeviceHost.updateScreenState();

        Log.d(TAG, "etsiStopConfig Exit");
    }

    void sendMessage(int what, Object obj) {
        Message msg = mHandler.obtainMessage();
        msg.what = what;
        msg.obj = obj;
        mHandler.sendMessage(msg);
    }

    final class NfcServiceHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_ROUTE_AID: {
                    int route = msg.arg1;
                    int power = msg.arg2;
                    String aid = (String) msg.obj;
                    if(aid.endsWith("*")) {
                      String cuttedAid = aid.substring(0, aid.length() - 1);
                      mDeviceHost.routeAid(hexStringToBytes(cuttedAid), route, power, true);
                    } else {
                        mDeviceHost.routeAid(hexStringToBytes(aid), route, power, false);
                    }
                    // Restart polling config
                    break;
                }
                case MSG_INVOKE_BEAM: {
                    mP2pLinkManager.onManualBeamInvoke((BeamShareData)msg.obj);
                    break;
                }
                case MSG_UNROUTE_AID: {
                    String aid = (String) msg.obj;
                    mDeviceHost.unrouteAid(hexStringToBytes(aid));
                    break;
                }

                case MSG_ROUTE_NFCID2: {
                    Bundle extras = (Bundle) msg.obj;
                    String nfcid2 = extras.getString(Nfcid2RoutingCache.EXTRA_NFCID2, null);
                    String syscode = extras.getString(Nfcid2RoutingCache.EXTRA_SYSCODE, null);
                    String optcode = extras.getString(Nfcid2RoutingCache.EXTRA_OPTPARAM, null);

                    mDeviceHost.routeNfcid2(hexStringToBytes(nfcid2), hexStringToBytes(syscode), hexStringToBytes(optcode));
                    // Restart polling config
                    break;
                }

                case MSG_UNROUTE_NFCID2: {
                    String nfcid2 = (String) msg.obj;
                    mDeviceHost.unrouteNfcid2(hexStringToBytes(nfcid2));
                    // Restart polling config
                    break;
                }

                case MSG_COMMITINF_FELICA_ROUTING: {
                    Log.e(TAG, "applyRouting -10");
                    mIsFelicaOnHostConfiguring = true;
                    applyRouting(true);
                    break;
                }

                case MSG_COMMITED_FELICA_ROUTING: {
                    Log.e(TAG, "applyRouting -11");
                    mIsFelicaOnHostConfigured = true;
                    applyRouting(true);
                    break;
                }


                case MSG_COMMIT_ROUTING: {
                    Log.e(TAG, "applyRouting -9");
                   boolean commit = false;
                    synchronized (NfcService.this) {
                        if (mCurrentDiscoveryParameters.shouldEnableDiscovery()) {
                            commit = true;
                        } else {
                            Log.d(TAG, "Not committing routing because discovery is disabled.");
                        }
                    }
                    if (commit) {
                        mIsRoutingTableDirty = true;
                        applyRouting(false);
                    }


                    break;
                }
                case MSG_CLEAR_ROUTING: {
                    mDeviceHost.clearAidTable();
                    break;
                }

                case MSG_CHANGE_DEFAULT_ROUTE:
                    Log.d(TAG, "Handler: Change default route");
                    try{
                        mNxpNfcAdapter.DefaultRouteSet(ROUTE_ID_HOST, true, false, false);
                    } catch(RemoteException re) {
                        Log.d(TAG, "NxpNci: onAidRoutingTableFull: Exception to change default route to host!");
                    }
                    break;

                case MSG_MOCK_NDEF: {
                    NdefMessage ndefMsg = (NdefMessage) msg.obj;
                    Bundle extras = new Bundle();
                    extras.putParcelable(Ndef.EXTRA_NDEF_MSG, ndefMsg);
                    extras.putInt(Ndef.EXTRA_NDEF_MAXLENGTH, 0);
                    extras.putInt(Ndef.EXTRA_NDEF_CARDSTATE, Ndef.NDEF_MODE_READ_ONLY);
                    extras.putInt(Ndef.EXTRA_NDEF_TYPE, Ndef.TYPE_OTHER);
                    Tag tag = Tag.createMockTag(new byte[]{0x00},
                            new int[]{TagTechnology.NDEF},
                            new Bundle[]{extras});
                    Log.d(TAG, "mock NDEF tag, starting corresponding activity");
                    Log.d(TAG, tag.toString());
                    int dispatchStatus = mNfcDispatcher.dispatchTag(tag);
                    if (dispatchStatus == NfcDispatcher.DISPATCH_SUCCESS) {
                        playSound(SOUND_END);
                    } else if (dispatchStatus == NfcDispatcher.DISPATCH_FAIL) {
                        playSound(SOUND_ERROR);
                    }
                    break;
                }

                case MSG_SE_DELIVER_INTENT: {
                    Log.d(TAG, "SE DELIVER INTENT");
                    Intent seIntent = (Intent) msg.obj;

                    String action = seIntent.getAction();
                    if (action.equals("com.gsma.services.nfc.action.TRANSACTION_EVENT")) {
                        byte[] byteAid = seIntent.getByteArrayExtra("com.android.nfc_extras.extra.AID");
                        byte[] data = seIntent.getByteArrayExtra("com.android.nfc_extras.extra.DATA");
                        String seName = seIntent.getStringExtra("com.android.nfc_extras.extra.SE_NAME");
                        StringBuffer strAid = new StringBuffer();
                        for (int i = 0; i < byteAid.length; i++) {
                            String hex = Integer.toHexString(0xFF & byteAid[i]);
                            if (hex.length() == 1)
                                strAid.append('0');
                            strAid.append(hex);
                        }
                        Intent gsmaIntent = new Intent();
                        gsmaIntent.setAction("com.gsma.services.nfc.action.TRANSACTION_EVENT");
                        if (byteAid != null)
                            gsmaIntent.putExtra("com.gsma.services.nfc.extra.AID", byteAid);
                        if (data != null)
                            gsmaIntent.putExtra("com.gsma.services.nfc.extra.DATA", data);

                        //"nfc://secure:0/<seName>/<strAid>"
                        String url = new String ("nfc://secure:0/" + seName + "/" + strAid);
                        gsmaIntent.setData(Uri.parse(url));
                        gsmaIntent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
                        gsmaIntent.setPackage(seIntent.getPackage());

                        Boolean receptionMode = mNxpNfcController.mMultiReceptionMap.get(seName);
                        if (receptionMode == null)
                            receptionMode = defaultTransactionEventReceptionMode;

                        if (receptionMode == multiReceptionMode) {
                            // if multicast reception for GSMA
                            mContext.sendBroadcast(gsmaIntent);
                        } else {
                            // if unicast reception for GSMA
                            try {
                                if (mIsSentUnicastReception == false) {
                                    //start gsma
                                    gsmaIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                                    mContext.startActivity(gsmaIntent);
                                    mIsSentUnicastReception = true;
                                }
                            } catch (Exception e) {
                                if (DBG) Log.d(TAG, "Exception: " + e.getMessage());
                            }
                        }
                    } else {
                        mContext.sendBroadcast(seIntent);
                    }
                    break;
                }

                case MSG_NDEF_TAG:
                    if (DBG) Log.d(TAG, "Tag detected, notifying applications");
                    TagEndpoint tag = (TagEndpoint) msg.obj;
                    ReaderModeParams readerParams = null;
                    int presenceCheckDelay = DEFAULT_PRESENCE_CHECK_DELAY;
                    DeviceHost.TagDisconnectedCallback callback =
                            new DeviceHost.TagDisconnectedCallback() {
                                @Override
                                public void onTagDisconnected(long handle) {
                                    applyRouting(false);
                                }
                            };
                    synchronized (NfcService.this) {
                        readerParams = mReaderModeParams;
                    }
                    if (readerParams != null) {
                        presenceCheckDelay = readerParams.presenceCheckDelay;
                        if ((readerParams.flags & NfcAdapter.FLAG_READER_SKIP_NDEF_CHECK) != 0) {
                            if (DBG) Log.d(TAG, "Skipping NDEF detection in reader mode");
                            tag.startPresenceChecking(presenceCheckDelay, callback);
                            dispatchTagEndpoint(tag, readerParams);
                            break;
                        }
                    }

                    boolean playSound = readerParams == null ||
                        (readerParams.flags & NfcAdapter.FLAG_READER_NO_PLATFORM_SOUNDS) == 0;
                    if (mScreenState == ScreenStateHelper.SCREEN_STATE_ON_UNLOCKED && playSound) {
                        playSound(SOUND_START);
                    }
                    if (tag.getConnectedTechnology() == TagTechnology.NFC_BARCODE) {
                        // When these tags start containing NDEF, they will require
                        // the stack to deal with them in a different way, since
                        // they are activated only really shortly.
                        // For now, don't consider NDEF on these.
                        if (DBG) Log.d(TAG, "Skipping NDEF detection for NFC Barcode");
                        tag.startPresenceChecking(presenceCheckDelay, callback);
                        dispatchTagEndpoint(tag, readerParams);
                        break;
                    }
                    NdefMessage ndefMsg = tag.findAndReadNdef();

                    if (ndefMsg != null) {
                        if(tag.getConnectedTechnology() == TagTechnology.NFC_F)
                        {
                            presenceCheckDelay = NFC_F_TRANSCEIVE_PRESENCE_CHECK_DELAY;
                        }
                        tag.startPresenceChecking(presenceCheckDelay, callback);
                        dispatchTagEndpoint(tag, readerParams);
                    } else {
                        if (tag.reconnect()) {
                            if(tag.getConnectedTechnology() == TagTechnology.NFC_F)
                            {
                                 presenceCheckDelay = NFC_F_TRANSCEIVE_PRESENCE_CHECK_DELAY;
                            }
                            tag.startPresenceChecking(presenceCheckDelay, callback);
                            dispatchTagEndpoint(tag, readerParams);
                        } else {
                            tag.disconnect();
                            playSound(SOUND_ERROR);
                        }
                    }
                    break;

                case MSG_CARD_EMULATION:
                    if (DBG) Log.d(TAG, "Card Emulation message");
                    /* Tell the host-emu manager an AID has been selected on
                     * a secure element.
                     */
                    if (mCardEmulationManager != null) {
                        mCardEmulationManager.onOffHostAidSelected();
                    }
                    Pair<byte[], Pair> transactionInfo = (Pair<byte[], Pair>) msg.obj;
                    Pair<byte[], Integer> dataSrcInfo = (Pair<byte[], Integer>) transactionInfo.second;

                    String gsmaSrc = "";
                    String gsmaDataAID = toHexString(transactionInfo.first, 0, transactionInfo.first.length);
                    {
                        Log.d(TAG, "Event source " + dataSrcInfo.second);

                        String evtSrc = "";
                        if(dataSrcInfo.second == UICC_ID_TYPE) {
                            evtSrc = NxpConstants.UICC_ID;
                            gsmaSrc = "SIM1";
                        } else if(dataSrcInfo.second == SMART_MX_ID_TYPE) {
                            evtSrc = NxpConstants.SMART_MX_ID;
                            gsmaSrc = "ESE";
                        }
                        /* Send broadcast ordered */
                        Intent TransactionIntent = new Intent();
                        TransactionIntent.setAction(NxpConstants.ACTION_TRANSACTION_DETECTED);
                        TransactionIntent.putExtra(NxpConstants.EXTRA_AID, transactionInfo.first);
                        TransactionIntent.putExtra(NxpConstants.EXTRA_DATA, dataSrcInfo.first);
                        TransactionIntent.putExtra(NxpConstants.EXTRA_SOURCE, evtSrc);

                        if (DBG) {
                            Log.d(TAG, "Start Activity Card Emulation event");
                        }
                        mIsSentUnicastReception = false;
                        mContext.sendBroadcast(TransactionIntent, NfcPermissions.NFC_PERMISSION);
                    }
                    /* Send broadcast */
                    Intent aidIntent = new Intent();
                    aidIntent.setAction(ACTION_AID_SELECTED);
                    aidIntent.putExtra(EXTRA_AID, transactionInfo.first);
                    if (DBG) Log.d(TAG, "Broadcasting " + ACTION_AID_SELECTED);
                    sendSeBroadcast(aidIntent);

                    /* Send "transaction events" to all authorized/registered components" */
                    Intent evtIntent = new Intent();
                    evtIntent.setAction(NxpConstants.ACTION_MULTI_EVT_TRANSACTION);
                    evtIntent.setData(Uri.parse("nfc://secure:0/"+ gsmaSrc+"/"+ gsmaDataAID));
                    evtIntent.putExtra(NxpConstants.EXTRA_GSMA_AID, transactionInfo.first);
                    evtIntent.putExtra(NxpConstants.EXTRA_GSMA_DATA, dataSrcInfo.first);
                    Log.d(TAG, "Broadcasting " + NxpConstants.ACTION_MULTI_EVT_TRANSACTION);
                    sendMultiEvtBroadcast(evtIntent);

                    break;

                case MSG_CONNECTIVITY_EVENT:
                    if (DBG) {
                        Log.d(TAG, "SE EVENT CONNECTIVITY");
                    }
                    Integer evtSrcInfo = (Integer) msg.obj;
                    Log.d(TAG, "Event source " + evtSrcInfo);
                    String evtSrc = "";
                    if(evtSrcInfo == UICC_ID_TYPE) {
                        evtSrc = NxpConstants.UICC_ID;
                    } else if(evtSrcInfo == SMART_MX_ID_TYPE) {
                        evtSrc = NxpConstants.SMART_MX_ID;
                    }

                    Intent eventConnectivityIntent = new Intent();
                    eventConnectivityIntent.setAction(NxpConstants.ACTION_CONNECTIVITY_EVENT_DETECTED);
                    eventConnectivityIntent.putExtra(NxpConstants.EXTRA_SOURCE, evtSrc);
                    if (DBG) {
                        Log.d(TAG, "Broadcasting Intent");
                    }
                    mContext.sendBroadcast(eventConnectivityIntent, NfcPermissions.NFC_PERMISSION);
                    break;

                case MSG_EMVCO_MULTI_CARD_DETECTED_EVENT:
                    if (DBG) {
                        Log.d(TAG, "EMVCO MULTI CARD DETECTED EVENT");
                    }

                    Intent eventEmvcoMultiCardIntent = new Intent();
                    eventEmvcoMultiCardIntent.setAction(ACTION_EMVCO_MULTIPLE_CARD_DETECTED);
                    if (DBG) {
                        Log.d(TAG, "Broadcasting Intent");
                    }
                    mContext.sendBroadcast(eventEmvcoMultiCardIntent, NFC_PERM);
                    break;

                case MSG_SE_EMV_CARD_REMOVAL:
                    if (DBG) Log.d(TAG, "Card Removal message");
                    /* Send broadcast */
                    Intent cardRemovalIntent = new Intent();
                    cardRemovalIntent.setAction(ACTION_EMV_CARD_REMOVAL);
                    if (DBG) Log.d(TAG, "Broadcasting " + ACTION_EMV_CARD_REMOVAL);
                    sendSeBroadcast(cardRemovalIntent);
                    break;

                case MSG_SE_APDU_RECEIVED:
                    if (DBG) Log.d(TAG, "APDU Received message");
                    byte[] apduBytes = (byte[]) msg.obj;
                    /* Send broadcast */
                    Intent apduReceivedIntent = new Intent();
                    apduReceivedIntent.setAction(ACTION_APDU_RECEIVED);
                    if (apduBytes != null && apduBytes.length > 0) {
                        apduReceivedIntent.putExtra(EXTRA_APDU_BYTES, apduBytes);
                    }
                    if (DBG) Log.d(TAG, "Broadcasting " + ACTION_APDU_RECEIVED);
                    sendSeBroadcast(apduReceivedIntent);
                    break;

                case MSG_SE_MIFARE_ACCESS:
                    if (DBG) Log.d(TAG, "MIFARE access message");
                    /* Send broadcast */
                    byte[] mifareCmd = (byte[]) msg.obj;
                    Intent mifareAccessIntent = new Intent();
                    mifareAccessIntent.setAction(ACTION_MIFARE_ACCESS_DETECTED);
                    if (mifareCmd != null && mifareCmd.length > 1) {
                        int mifareBlock = mifareCmd[1] & 0xff;
                        if (DBG) Log.d(TAG, "Mifare Block=" + mifareBlock);
                        mifareAccessIntent.putExtra(EXTRA_MIFARE_BLOCK, mifareBlock);
                    }
                    if (DBG) Log.d(TAG, "Broadcasting " + ACTION_MIFARE_ACCESS_DETECTED);
                    sendSeBroadcast(mifareAccessIntent);
                    break;

                case MSG_LLCP_LINK_ACTIVATION:
                    if (mIsDebugBuild) {
                        Intent actIntent = new Intent(ACTION_LLCP_UP);
                        mContext.sendBroadcast(actIntent);
                    }
                    llcpActivated((NfcDepEndpoint) msg.obj);
                    break;

                case MSG_LLCP_LINK_DEACTIVATED:
                    if (mIsDebugBuild) {
                        Intent deactIntent = new Intent(ACTION_LLCP_DOWN);
                        mContext.sendBroadcast(deactIntent);
                    }
                    NfcDepEndpoint device = (NfcDepEndpoint) msg.obj;
                    boolean needsDisconnect = false;

                    Log.d(TAG, "LLCP Link Deactivated message. Restart polling loop.");
                    synchronized (NfcService.this) {
                        /* Check if the device has been already unregistered */
                        if (mObjectMap.remove(device.getHandle()) != null) {
                            /* Disconnect if we are initiator */
                            if (device.getMode() == NfcDepEndpoint.MODE_P2P_TARGET) {
                                if (DBG) Log.d(TAG, "disconnecting from target");
                                needsDisconnect = true;
                            } else {
                                if (DBG) Log.d(TAG, "not disconnecting from initiator");
                            }
                        }
                    }
                    if (needsDisconnect) {
                        device.disconnect();  // restarts polling loop
                    }

                    mP2pLinkManager.onLlcpDeactivated();
                    break;
                case MSG_LLCP_LINK_FIRST_PACKET:
                    mP2pLinkManager.onLlcpFirstPacketReceived();
                    break;
                case MSG_TARGET_DESELECTED:
                    /* Broadcast Intent Target Deselected */
                    if (DBG) Log.d(TAG, "Target Deselected");
                    Intent intent = new Intent();
                    intent.setAction(NativeNfcManager.INTERNAL_TARGET_DESELECTED_ACTION);
                    if (DBG) Log.d(TAG, "Broadcasting Intent");
                    mContext.sendOrderedBroadcast(intent, NfcPermissions.NFC_PERMISSION);
                    break;

                case MSG_SE_FIELD_ACTIVATED: {
                    if (DBG) Log.d(TAG, "SE FIELD ACTIVATED");
                    Intent eventFieldOnIntent = new Intent();
                    eventFieldOnIntent.setAction(ACTION_RF_FIELD_ON_DETECTED);
                    sendSeBroadcast(eventFieldOnIntent);
                    break;
                }
                case MSG_RESUME_POLLING:
                    mNfcAdapter.resumePolling();
                    break;

                case MSG_SE_FIELD_DEACTIVATED: {
                    if (DBG) Log.d(TAG, "SE FIELD DEACTIVATED");
                    Intent eventFieldOffIntent = new Intent();
                    eventFieldOffIntent.setAction(ACTION_RF_FIELD_OFF_DETECTED);
                    sendSeBroadcast(eventFieldOffIntent);
                    break;
                }

                case MSG_SE_LISTEN_ACTIVATED: {
                    if (DBG) Log.d(TAG, "SE LISTEN MODE ACTIVATED");
                    Intent listenModeActivated = new Intent();
                    listenModeActivated.setAction(ACTION_SE_LISTEN_ACTIVATED);
                    sendSeBroadcast(listenModeActivated);
                    break;
                }

                case MSG_SE_LISTEN_DEACTIVATED: {
                    if (DBG) Log.d(TAG, "SE LISTEN MODE DEACTIVATED");
                    Intent listenModeDeactivated = new Intent();
                    listenModeDeactivated.setAction(ACTION_SE_LISTEN_DEACTIVATED);
                    sendSeBroadcast(listenModeDeactivated);
                    break;
                }

                case MSG_SWP_READER_REQUESTED:

                    /* Send broadcast ordered */
                    Intent SWPReaderRequestedIntent = new Intent();
                    ArrayList<Integer> techList = (ArrayList<Integer>) msg.obj;
                    Integer techs[] = techList.toArray(new Integer[techList.size()]);
                    SWPReaderRequestedIntent
                            .setAction(NxpConstants.ACTION_SWP_READER_REQUESTED);
                    if (DBG) {
                        Log.d(TAG, "SWP READER - Requested");
                    }
                    mContext.sendBroadcast(SWPReaderRequestedIntent);
                    break;

                case MSG_SWP_READER_REQUESTED_FAIL:

                    /* Send broadcast ordered */
                    Intent SWPReaderRequestedFailIntent = new Intent();
                    //ArrayList<Integer> techList = (ArrayList<Integer>) msg.obj;
                    //Integer techs[] = techList.toArray(new Integer[techList.size()]);
                    //Intent SWPReaderRequestedFailIntent = new Intent();
                    //Integer FailCause = (Integer) msg.obj;

                    //SWPReaderRequestedFailIntent.setAction(NfcAdapter.ACTION_SWP_READER_REQUESTED_FAIL);
                    //SWPReaderRequestedFailIntent.putExtra(NfcAdapter.EXTRA_SWP_READER_REQ_FAIL,FailCause);
                    SWPReaderRequestedFailIntent
                    .setAction(NxpConstants.ACTION_SWP_READER_REQUESTED_FAILED);
                    if (DBG) {
                        Log.d(TAG, "SWP READER - Requested Fail");
                    }
                    mContext.sendBroadcast(SWPReaderRequestedFailIntent);
                    break;

                case MSG_SWP_READER_ACTIVATED:

                    /* Send broadcast ordered */
                    Intent SWPReaderActivatedIntent = new Intent();

                    SWPReaderActivatedIntent
                            .setAction(NxpConstants.ACTION_SWP_READER_ACTIVATED);
                    if (DBG) {
                        Log.d(TAG, "SWP READER - Activated");
                    }
                    mContext.sendBroadcast(SWPReaderActivatedIntent);
                    break;

                case MSG_ETSI_START_CONFIG:
                {
                    Log.d(TAG, "NfcServiceHandler - MSG_ETSI_START_CONFIG");
                    ArrayList<Integer> configList = (ArrayList<Integer>) msg.obj;
                    int eeHandle;
                    if(configList.contains(0x402))
                    {
                        eeHandle = 0x402;
                    }
                    else{
                        eeHandle = 0x4C0;
                    }
                    etsiStartConfig(eeHandle);
                }
                    break;

                case MSG_ETSI_STOP_CONFIG:

                    Log.d(TAG, "NfcServiceHandler - MSG_ETSI_STOP_CONFIG");

                    etsiStopConfig((int)msg.obj);
                    break;

                case MSG_ETSI_SWP_TIMEOUT:

                    Log.d(TAG, "NfcServiceHandler - MSG_ETSI_SWP_TIMEOUT");

                    mDeviceHost.setEtsiReaederState(STATE_SE_RDR_MODE_STOP_CONFIG);
                    etsiStopConfig((int)msg.obj);

                    break;
                case MSG_APPLY_SREEN_STATE:

                    mScreenState = (int)msg.obj;
                    mDeviceHost.doSetScreenOrPowerState(mScreenState);
                    mRoutingWakeLock.acquire();
                    try {
                        Log.e(TAG, "applyRouting -20");
                        applyRouting(false);
                    } finally {
                        mRoutingWakeLock.release();
                    }
                    break;
                default:
                    Log.e(TAG, "Unknown message received");
                    break;
            }
        }

        private void sendSeBroadcast(Intent intent) {
            intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
            // Resume app switches so the receivers can start activites without delay
            mNfcDispatcher.resumeAppSwitches();
            ArrayList<String> matchingPackages = new ArrayList<String>();
            ArrayList<String> preferredPackages = new ArrayList<String>();
            if(mInstalledPackages == null) {
                Log.d(TAG, "No packages to send broadcast.");
                return;
            }
            synchronized(this) {
                for (PackageInfo pkg : mInstalledPackages) {
                    if (pkg != null && pkg.applicationInfo != null) {
                        if (mNfceeAccessControl.check(pkg.applicationInfo)) {
                            matchingPackages.add(pkg.packageName);
                            if (mCardEmulationManager != null &&
                                    mCardEmulationManager.packageHasPreferredService(pkg.packageName)) {
                                preferredPackages.add(pkg.packageName);
                            }
                        }
                    }
                }
                if (preferredPackages.size() > 0) {
                    // If there's any packages in here which are preferred, only
                    // send field events to those packages, to prevent other apps
                    // with signatures in nfcee_access.xml from acting upon the events.
                    for (String packageName : preferredPackages){
                        intent.setPackage(packageName);
                        mContext.sendBroadcast(intent);
                    }
                } else {
                    for (String packageName : matchingPackages){
                        intent.setPackage(packageName);
                        mContext.sendBroadcast(intent);
                    }
                }
            }
        }

        private void sendMultiEvtBroadcast(Intent intent) {

            ArrayList<String> packageList = mNxpNfcController.getEnabledMultiEvtsPackageList();
            ComponentName unicastComponent = null;
            if(packageList.size() == 0) {
                Log.d(TAG, "No packages to send broadcast.");
                unicastComponent = mNxpNfcController.getUnicastPackage();
                if(unicastComponent != null)
                {
                    intent.setComponent(unicastComponent);
                    try {
                        //start gsma
                        Log.d(TAG, "Starting activity uincast Pkg"+unicastComponent.flattenToString());
                        intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
                        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                        if(mContext.getPackageManager().resolveActivity(intent, 0) != null)
                        {
                            mContext.startActivity(intent);
                        } else {
                            Log.d(TAG, "Intent not resolved");
                        }
                    } catch (Exception e) {
                        if (DBG) Log.d(TAG, "Exception: " + e.getMessage());
                    }
                }
                return;
            }

            for(int i=0; i<packageList.size(); i++) {
                Log.d(TAG,"MultiEvt Enabled Application packageName: " + packageList.get(i));
                intent.setPackage(packageList.get(i));
                mContext.sendBroadcast(intent, NxpConstants.PERMISSIONS_TRANSACTION_EVENT);
            }
        }

        private boolean llcpActivated(NfcDepEndpoint device) {
            Log.d(TAG, "LLCP Activation message");

            if (device.getMode() == NfcDepEndpoint.MODE_P2P_TARGET) {
                if (DBG) Log.d(TAG, "NativeP2pDevice.MODE_P2P_TARGET");
                if (device.connect()) {
                    /* Check LLCP compliance */
                    if (mDeviceHost.doCheckLlcp()) {
                        /* Activate LLCP Link */
                        if (mDeviceHost.doActivateLlcp()) {
                            if (DBG) Log.d(TAG, "Initiator Activate LLCP OK");
                            synchronized (NfcService.this) {
                                // Register P2P device
                                mObjectMap.put(device.getHandle(), device);
                            }
                            mP2pLinkManager.onLlcpActivated(device.getLlcpVersion());
                            return true;
                        } else {
                            /* should not happen */
                            Log.w(TAG, "Initiator LLCP activation failed. Disconnect.");
                            device.disconnect();
                        }
                    } else {
                        if (DBG) Log.d(TAG, "Remote Target does not support LLCP. Disconnect.");
                        device.disconnect();
                    }
                } else {
                    if (DBG) Log.d(TAG, "Cannot connect remote Target. Polling loop restarted.");
                    /*
                     * The polling loop should have been restarted in failing
                     * doConnect
                     */
                }
            } else if (device.getMode() == NfcDepEndpoint.MODE_P2P_INITIATOR) {
                if (DBG) Log.d(TAG, "NativeP2pDevice.MODE_P2P_INITIATOR");
                /* Check LLCP compliancy */
                if (mDeviceHost.doCheckLlcp()) {
                    /* Activate LLCP Link */
                    if (mDeviceHost.doActivateLlcp()) {
                        if (DBG) Log.d(TAG, "Target Activate LLCP OK");
                        synchronized (NfcService.this) {
                            // Register P2P device
                            mObjectMap.put(device.getHandle(), device);
                        }
                        mP2pLinkManager.onLlcpActivated(device.getLlcpVersion());
                        return true;
                    }
                } else {
                    Log.w(TAG, "checkLlcp failed");
                }
            }

            return false;
        }

        private void dispatchTagEndpoint(TagEndpoint tagEndpoint, ReaderModeParams readerParams) {
            Tag tag = new Tag(tagEndpoint.getUid(), tagEndpoint.getTechList(),
                    tagEndpoint.getTechExtras(), tagEndpoint.getHandle(), mNfcTagService);
            registerTagObject(tagEndpoint);
            if (readerParams != null) {
                try {
                    if ((readerParams.flags & NfcAdapter.FLAG_READER_NO_PLATFORM_SOUNDS) == 0) {
                        playSound(SOUND_END);
                    }
                    if (readerParams.callback != null) {
                        readerParams.callback.onTagDiscovered(tag);
                        return;
                    } else {
                        // Follow normal dispatch below
                    }
                } catch (RemoteException e) {
                    Log.e(TAG, "Reader mode remote has died, falling back.", e);
                    // Intentional fall-through
                } catch (Exception e) {
                    // Catch any other exception
                    Log.e(TAG, "App exception, not dispatching.", e);
                    return;
                }
            }
            int dispatchResult = mNfcDispatcher.dispatchTag(tag);
            if (dispatchResult == NfcDispatcher.DISPATCH_FAIL) {
                unregisterObject(tagEndpoint.getHandle());
                playSound(SOUND_ERROR);
            } else if (dispatchResult == NfcDispatcher.DISPATCH_SUCCESS) {
                playSound(SOUND_END);
            }
        }
    }

    private NfcServiceHandler mHandler = new NfcServiceHandler();

    class ApplyRoutingTask extends AsyncTask<Integer, Void, Void> {
        @Override
        protected Void doInBackground(Integer... params) {
            synchronized (NfcService.this) {
                if (params == null || params.length != 1) {
                    // force apply current routing
                    Log.e(TAG, "applyRouting -1");
                    applyRouting(true);
                    return null;
                }
                mScreenState = params[0].intValue();
                mDeviceHost.doSetScreenOrPowerState(mScreenState);
                mRoutingWakeLock.acquire();
                try {
                    Log.e(TAG, "applyRouting -2");
                    applyRouting(false);
                } finally {
                    mRoutingWakeLock.release();
                }
                return null;
            }
        }
    }

    class TagRemoveTask extends AsyncTask<Integer, Void, Void> {
        @Override
        protected Void doInBackground(Integer... params) {

                Intent SWPReaderTagRemoveIntent = new Intent();

                SWPReaderTagRemoveIntent.setAction(NxpConstants.ACTION_SWP_READER_TAG_REMOVE);

                int counter = 0;
                int discNtfTimeout = params[0].intValue();

                while(ETSI_STOP_CONFIG == true)
                { /* Send broadcast ordered */

                    if(discNtfTimeout == 0)
                    {
                        /*Do nothing*/
                    }
                    else if(counter < discNtfTimeout)
                    {
                        counter++;
                    }
                    else
                    {
                        ETSI_STOP_CONFIG = false;
                        break;
                    }
                    if (DBG) {
                        Log.d(TAG, "SWP READER - Tag Remove");
                    }
                    mContext.sendBroadcast(SWPReaderTagRemoveIntent);
                    try{
                        Thread.sleep(1000);
                    } catch(Exception e) {
                        e.printStackTrace();
                    }
                }
                return null;
        }
    }


    private final BroadcastReceiver x509CertificateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            boolean result = intent.getBooleanExtra(NxpConstants.EXTRA_RESULT, false);
            mNxpNfcController.setResultForCertificates(result);
        }
    };

    private final BroadcastReceiver mEnableNfc = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Intent nfcDialogIntent = new Intent(mContext, EnableNfcDialogActivity.class);
            nfcDialogIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
            mContext.startActivityAsUser(nfcDialogIntent, UserHandle.CURRENT);
        }
    };

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(
                    NativeNfcManager.INTERNAL_TARGET_DESELECTED_ACTION)) {
                // Perform applyRouting() in AsyncTask to serialize blocking calls
                new ApplyRoutingTask().execute();
            } else if ((action.equals(Intent.ACTION_SCREEN_ON)
                    || action.equals(Intent.ACTION_SCREEN_OFF)
                    || action.equals(Intent.ACTION_USER_PRESENT)) &&
                       mState == NfcAdapter.STATE_ON)  {
                // Perform applyRouting() in AsyncTask to serialize blocking calls
                int screenState = ScreenStateHelper.SCREEN_STATE_OFF;
                if (action.equals(Intent.ACTION_SCREEN_OFF)) {
                    if(mScreenState != ScreenStateHelper.SCREEN_STATE_OFF)
                    {
                        screenState = ScreenStateHelper.SCREEN_STATE_OFF;
                    }
                    //mDeviceHost.doSetScreenOrPowerState(ScreenStateHelper.SCREEN_STATE_OFF);
                } else if (action.equals(Intent.ACTION_SCREEN_ON)) {
                    screenState = mKeyguard.isKeyguardLocked() ?
                            ScreenStateHelper.SCREEN_STATE_ON_LOCKED : ScreenStateHelper.SCREEN_STATE_ON_UNLOCKED;
                    if(screenState == ScreenStateHelper.SCREEN_STATE_ON_LOCKED && mScreenState == ScreenStateHelper.SCREEN_STATE_OFF) {
                        //mDeviceHost.doSetScreenOrPowerState(ScreenStateHelper.SCREEN_STATE_ON_LOCKED);
                    } else if (screenState == ScreenStateHelper.SCREEN_STATE_ON_LOCKED && mScreenState == ScreenStateHelper.SCREEN_STATE_ON_LOCKED) {
                        return;
                    }
                } else if (action.equals(Intent.ACTION_USER_PRESENT)) {
                    if (mScreenState != ScreenStateHelper.SCREEN_STATE_ON_UNLOCKED) {
                        screenState = ScreenStateHelper.SCREEN_STATE_ON_UNLOCKED;
                        //mDeviceHost.doSetScreenOrPowerState(ScreenStateHelper.SCREEN_STATE_ON_UNLOCKED);
                    } else {
                        return;
                    }
                }
                // TODO mCardEmulationManager.setScreenState(screenState);
                sendMessage(NfcService.MSG_APPLY_SREEN_STATE, screenState);
                //new ApplyRoutingTask().execute(Integer.valueOf(screenState));
            } else if (action.equals(Intent.ACTION_AIRPLANE_MODE_CHANGED)) {
                boolean isAirplaneModeOn = intent.getBooleanExtra("state", false);
                // Query the airplane mode from Settings.System just to make sure that
                // some random app is not sending this intent
                if (isAirplaneModeOn != isAirplaneModeOn()) {
                    return;
                }
                if (!mIsAirplaneSensitive) {
                    return;
                }
                mPrefsEditor.putBoolean(PREF_AIRPLANE_OVERRIDE, false);
                mPrefsEditor.apply();
                if (isAirplaneModeOn) {
                    new EnableDisableTask().execute(TASK_DISABLE);
                } else if (!isAirplaneModeOn && mPrefs.getBoolean(PREF_NFC_ON, NFC_ON_DEFAULT)) {
                    new EnableDisableTask().execute(TASK_ENABLE);
                }
            } else if (action.equals(Intent.ACTION_USER_SWITCHED)) {
                int screenState = ScreenStateHelper.SCREEN_STATE_OFF;
                int userId = intent.getIntExtra(Intent.EXTRA_USER_HANDLE, 0);
                synchronized (this) {
                    mUserId = userId;
                }
                mP2pLinkManager.onUserSwitched(getUserId());
                if (mIsHceCapable) {
                    mCardEmulationManager.onUserSwitched(getUserId());
                }
                if (mScreenState == ScreenStateHelper.SCREEN_STATE_ON_UNLOCKED) {
                    screenState = ScreenStateHelper.SCREEN_STATE_ON_LOCKED;
                    }else {
                        return;
                }
                new ApplyRoutingTask().execute(Integer.valueOf(screenState));
            } else if(action.equals(Intent.ACTION_SHUTDOWN)) {
                mPowerShutDown = true;
                Log.d(TAG,"Device is shutting down.");
                mDeviceHost.doSetScreenOrPowerState(ScreenStateHelper.POWER_STATE_OFF);
            }
        }
    };

    private final BroadcastReceiver mOwnerReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_PACKAGE_REMOVED) ||
                    action.equals(Intent.ACTION_PACKAGE_ADDED) ||
                    action.equals(Intent.ACTION_EXTERNAL_APPLICATIONS_AVAILABLE) ||
                    action.equals(Intent.ACTION_EXTERNAL_APPLICATIONS_UNAVAILABLE)) {
                updatePackageCache();

                if (action.equals(Intent.ACTION_PACKAGE_REMOVED)) {
                    // Clear the NFCEE access cache in case a UID gets recycled
                    mNfceeAccessControl.invalidateCache();
                }
            }
        }
    };

    final BroadcastReceiver mAlaReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (NfcAdapter.ACTION_ADAPTER_STATE_CHANGED.equals(intent.getAction())) {
                int state = intent.getIntExtra(NfcAdapter.EXTRA_ADAPTER_STATE,
                        NfcAdapter.STATE_OFF);
                if (state == NfcAdapter.STATE_ON) {
                    Log.e(TAG, "Loader service update start from NFC_ON Broadcast");
                    NfcAlaService nas = new NfcAlaService();
                    if(mNfcAla.doGetLSConfigVersion() == LOADER_SERVICE_VERSION_21)
                        nas.updateLoaderService();
            }
        }
      }
    };

    private final BroadcastReceiver mPolicyReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent){
            String action = intent.getAction();
            if (DevicePolicyManager.ACTION_DEVICE_POLICY_MANAGER_STATE_CHANGED
                    .equals(action)) {
                enforceBeamShareActivityPolicy(context,
                        new UserHandle(getSendingUserId()), mIsNdefPushEnabled);
            }
        }
    };

    /**
     * Returns true if airplane mode is currently on
     */
    boolean isAirplaneModeOn() {
        return Settings.System.getInt(mContentResolver,
                Settings.Global.AIRPLANE_MODE_ON, 0) == 1;
    }

    /**
     * for debugging only - no i18n
     */
    static String stateToString(int state) {
        switch (state) {
            case NfcAdapter.STATE_OFF:
                return "off";
            case NfcAdapter.STATE_TURNING_ON:
                return "turning on";
            case NfcAdapter.STATE_ON:
                return "on";
            case NfcAdapter.STATE_TURNING_OFF:
                return "turning off";
            default:
                return "<error>";
        }
    }

    void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        if (mContext.checkCallingOrSelfPermission(android.Manifest.permission.DUMP)
                != PackageManager.PERMISSION_GRANTED) {
            pw.println("Permission Denial: can't dump nfc from from pid="
                    + Binder.getCallingPid() + ", uid=" + Binder.getCallingUid()
                    + " without permission " + android.Manifest.permission.DUMP);
            return;
        }

        synchronized (this) {
            pw.println("mState=" + stateToString(mState));
            pw.println("mIsZeroClickRequested=" + mIsNdefPushEnabled);
            pw.println("mScreenState=" + ScreenStateHelper.screenStateToString(mScreenState));
            pw.println("mNfcPollingEnabled=" + mNfcPollingEnabled);
            pw.println("mNfceeRouteEnabled=" + mNfceeRouteEnabled);
            pw.println("mOpenEe=" + mOpenEe);
            pw.println("mIsAirplaneSensitive=" + mIsAirplaneSensitive);
            pw.println("mIsAirplaneToggleable=" + mIsAirplaneToggleable);
            pw.println("mLockscreenPollMask=" + mLockscreenPollMask);
            pw.println(mCurrentDiscoveryParameters);
            mP2pLinkManager.dump(fd, pw, args);
            if (mIsHceCapable) {
                mCardEmulationManager.dump(fd, pw, args);
            }
            mNfceeAccessControl.dump(fd, pw, args);
            mNfcDispatcher.dump(fd, pw, args);
            pw.println(mDeviceHost.dump());

        }
    }
}
