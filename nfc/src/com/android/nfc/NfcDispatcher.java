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

package com.android.nfc;

import android.bluetooth.BluetoothAdapter;
import com.android.nfc.RegisteredComponentCache.ComponentInfo;
import com.android.nfc.handover.HandoverDataParser;
import com.android.nfc.handover.PeripheralHandoverService;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManagerNative;
import android.app.IActivityManager;
import android.app.PendingIntent;
import android.app.PendingIntent.CanceledException;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ResolveInfo;
import android.content.res.Resources.NotFoundException;
import android.net.Uri;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.NfcAdapter;
import android.nfc.Tag;
import android.nfc.tech.Ndef;
import android.nfc.tech.NfcBarcode;
import android.os.RemoteException;
import android.os.UserHandle;
import android.util.Log;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

/**
 * Dispatch of NFC events to start activities
 */
class NfcDispatcher {
    private static final boolean DBG = false;
    private static final String TAG = "NfcDispatcher";

    static final int DISPATCH_SUCCESS = 1;
    static final int DISPATCH_FAIL = 2;
    static final int DISPATCH_UNLOCK = 3;

    private final Context mContext;
    private final IActivityManager mIActivityManager;
    private final RegisteredComponentCache mTechListFilters;
    private final ContentResolver mContentResolver;
    private final HandoverDataParser mHandoverDataParser;
    private final String[] mProvisioningMimes;
    private final ScreenStateHelper mScreenStateHelper;
    private final NfcUnlockManager mNfcUnlockManager;
    private final boolean mDeviceSupportsBluetooth;

    // Locked on this
    private PendingIntent mOverrideIntent;
    private IntentFilter[] mOverrideFilters;
    private String[][] mOverrideTechLists;
    private boolean mProvisioningOnly;

    NfcDispatcher(Context context,
                  HandoverDataParser handoverDataParser,
                  boolean provisionOnly) {
        mContext = context;
        mIActivityManager = ActivityManagerNative.getDefault();
        mTechListFilters = new RegisteredComponentCache(mContext,
                NfcAdapter.ACTION_TECH_DISCOVERED, NfcAdapter.ACTION_TECH_DISCOVERED);
        mContentResolver = context.getContentResolver();
        mHandoverDataParser = handoverDataParser;
        mScreenStateHelper = new ScreenStateHelper(context);
        mNfcUnlockManager = NfcUnlockManager.getInstance();
        mDeviceSupportsBluetooth = BluetoothAdapter.getDefaultAdapter() != null;

        synchronized (this) {
            mProvisioningOnly = provisionOnly;
        }
        String[] provisionMimes = null;
        if (provisionOnly) {
            try {
                // Get accepted mime-types
                provisionMimes = context.getResources().
                        getStringArray(R.array.provisioning_mime_types);
            } catch (NotFoundException e) {
               provisionMimes = null;
            }
        }
        mProvisioningMimes = provisionMimes;
    }

    public synchronized void setForegroundDispatch(PendingIntent intent,
            IntentFilter[] filters, String[][] techLists) {
        if (DBG) Log.d(TAG, "Set Foreground Dispatch");
        mOverrideIntent = intent;
        mOverrideFilters = filters;
        mOverrideTechLists = techLists;
    }

    public synchronized void disableProvisioningMode() {
       mProvisioningOnly = false;
    }

    /**
     * Helper for re-used objects and methods during a single tag dispatch.
     */
    static class DispatchInfo {
        public final Intent intent;

        final Intent rootIntent;
        final Uri ndefUri;
        final String ndefMimeType;
        final PackageManager packageManager;
        final Context context;

        public DispatchInfo(Context context, Tag tag, NdefMessage message) {
            intent = new Intent();
            intent.putExtra(NfcAdapter.EXTRA_TAG, tag);
            intent.putExtra(NfcAdapter.EXTRA_ID, tag.getId());
            if (message != null) {
                intent.putExtra(NfcAdapter.EXTRA_NDEF_MESSAGES, new NdefMessage[] {message});
                ndefUri = message.getRecords()[0].toUri();
                ndefMimeType = message.getRecords()[0].toMimeType();
            } else {
                ndefUri = null;
                ndefMimeType = null;
            }

            rootIntent = new Intent(context, NfcRootActivity.class);
            rootIntent.putExtra(NfcRootActivity.EXTRA_LAUNCH_INTENT, intent);
            rootIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);

            this.context = context;
            packageManager = context.getPackageManager();
        }

        public Intent setNdefIntent() {
            intent.setAction(NfcAdapter.ACTION_NDEF_DISCOVERED);
            if (ndefUri != null) {
                intent.setData(ndefUri);
                return intent;
            } else if (ndefMimeType != null) {
                intent.setType(ndefMimeType);
                return intent;
            }
            return null;
        }

        public Intent setTechIntent() {
            intent.setData(null);
            intent.setType(null);
            intent.setAction(NfcAdapter.ACTION_TECH_DISCOVERED);
            return intent;
        }

        public Intent setTagIntent() {
            intent.setData(null);
            intent.setType(null);
            intent.setAction(NfcAdapter.ACTION_TAG_DISCOVERED);
            return intent;
        }

        /**
         * Launch the activity via a (single) NFC root task, so that it
         * creates a new task stack instead of interfering with any existing
         * task stack for that activity.
         * NfcRootActivity acts as the task root, it immediately calls
         * start activity on the intent it is passed.
         */
        boolean tryStartActivity() {
            // Ideally we'd have used startActivityForResult() to determine whether the
            // NfcRootActivity was able to launch the intent, but startActivityForResult()
            // is not available on Context. Instead, we query the PackageManager beforehand
            // to determine if there is an Activity to handle this intent, and base the
            // result of off that.
            List<ResolveInfo> activities = packageManager.queryIntentActivitiesAsUser(intent, 0,
                    ActivityManager.getCurrentUser());
            if (activities.size() > 0) {
                context.startActivityAsUser(rootIntent, UserHandle.CURRENT);
                return true;
            }
            return false;
        }

        boolean tryStartActivity(Intent intentToStart) {
            List<ResolveInfo> activities = packageManager.queryIntentActivitiesAsUser(
                    intentToStart, 0, ActivityManager.getCurrentUser());
            if (activities.size() > 0) {
                rootIntent.putExtra(NfcRootActivity.EXTRA_LAUNCH_INTENT, intentToStart);
                context.startActivityAsUser(rootIntent, UserHandle.CURRENT);
                return true;
            }
            return false;
        }
    }

    /** Returns:
     * <ul>
     *  <li /> DISPATCH_SUCCESS if dispatched to an activity,
     *  <li /> DISPATCH_FAIL if no activities were found to dispatch to,
     *  <li /> DISPATCH_UNLOCK if the tag was used to unlock the device
     * </ul>
     */
    public int dispatchTag(Tag tag) {
        PendingIntent overrideIntent;
        IntentFilter[] overrideFilters;
        String[][] overrideTechLists;
        boolean provisioningOnly;

        synchronized (this) {
            overrideFilters = mOverrideFilters;
            overrideIntent = mOverrideIntent;
            overrideTechLists = mOverrideTechLists;
            provisioningOnly = mProvisioningOnly;
        }

        boolean screenUnlocked = false;
        if (!provisioningOnly &&
                mScreenStateHelper.checkScreenState() == ScreenStateHelper.SCREEN_STATE_ON_LOCKED) {
            screenUnlocked = handleNfcUnlock(tag);
            if (!screenUnlocked) {
                return DISPATCH_FAIL;
            }
        }

        NdefMessage message = null;
        Ndef ndef = Ndef.get(tag);
        if (ndef != null) {
            message = ndef.getCachedNdefMessage();
        }else {
            NfcBarcode nfcBarcode = NfcBarcode.get(tag);
            if (nfcBarcode != null && nfcBarcode.getType() == NfcBarcode.TYPE_KOVIO) {
                message = decodeNfcBarcodeUri(nfcBarcode);
            }
        }

        if (DBG) Log.d(TAG, "dispatch tag: " + tag.toString() + " message: " + message);

        DispatchInfo dispatch = new DispatchInfo(mContext, tag, message);

        resumeAppSwitches();

        if (tryOverrides(dispatch, tag, message, overrideIntent, overrideFilters,
                overrideTechLists)) {
            return screenUnlocked ? DISPATCH_UNLOCK : DISPATCH_SUCCESS;
        }

        if (tryPeripheralHandover(message)) {
            if (DBG) Log.i(TAG, "matched BT HANDOVER");
            return screenUnlocked ? DISPATCH_UNLOCK : DISPATCH_SUCCESS;
        }

        if (NfcWifiProtectedSetup.tryNfcWifiSetup(ndef, mContext)) {
            if (DBG) Log.i(TAG, "matched NFC WPS TOKEN");
            return screenUnlocked ? DISPATCH_UNLOCK : DISPATCH_SUCCESS;
        }

        if (tryNdef(dispatch, message, provisioningOnly)) {
            return screenUnlocked ? DISPATCH_UNLOCK : DISPATCH_SUCCESS;
        }

        if (screenUnlocked) {
            // We only allow NDEF-based mimeType matching in case of an unlock
            return DISPATCH_UNLOCK;
        }

        if (provisioningOnly) {
            // We only allow NDEF-based mimeType matching
            return DISPATCH_FAIL;
        }

        // Only allow NDEF-based mimeType matching for unlock tags
        if (tryTech(dispatch, tag)) {
            return DISPATCH_SUCCESS;
        }

        dispatch.setTagIntent();
        if (dispatch.tryStartActivity()) {
            if (DBG) Log.i(TAG, "matched TAG");
            return DISPATCH_SUCCESS;
        }

        if (DBG) Log.i(TAG, "no match");
        return DISPATCH_FAIL;
    }

    private boolean handleNfcUnlock(Tag tag) {
        return mNfcUnlockManager.tryUnlock(tag);
    }

    /**
     * Checks for the presence of a URL stored in a tag with tech NfcBarcode.
     * If found, decodes URL and returns NdefMessage message containing an
     * NdefRecord containing the decoded URL. If not found, returns null.
     *
     * URLs are decoded as follows:
     *
     * Ignore first byte (which is 0x80 ORd with a manufacturer ID, corresponding
     * to ISO/IEC 7816-6).
     * The second byte describes the payload data format. There are four defined data
     * format values that identify URL data. Depending on the data format value, the
     * associated prefix is appended to the URL data:
     *
     * 0x01: URL with "http://www." prefix
     * 0x02: URL with "https://www." prefix
     * 0x03: URL with "http://" prefix
     * 0x04: URL with "https://" prefix
     *
     * Other data format values do not identify URL data and are not handled by this function.
     * URL payload is encoded in US-ASCII, following the limitations defined in RFC3987.
     * see http://www.ietf.org/rfc/rfc3987.txt
     *
     * The final two bytes of a tag with tech NfcBarcode are always reserved for CRC data,
     * and are therefore not part of the payload. They are ignored in the decoding of a URL.
     *
     * The default assumption is that the URL occupies the entire payload of the NfcBarcode
     * ID and all bytes of the NfcBarcode payload are decoded until the CRC (final two bytes)
     * is reached. However, the OPTIONAL early terminator byte 0xfe can be used to signal
     * an early end of the URL. Once this function reaches an early terminator byte 0xfe,
     * URL decoding stops and the NdefMessage is created and returned. Any payload data after
     * the first early terminator byte is ignored for the purposes of URL decoding.
     */
    private NdefMessage decodeNfcBarcodeUri(NfcBarcode nfcBarcode) {
        final byte URI_PREFIX_HTTP_WWW  = (byte) 0x01; // "http://www."
        final byte URI_PREFIX_HTTPS_WWW = (byte) 0x02; // "https://www."
        final byte URI_PREFIX_HTTP      = (byte) 0x03; // "http://"
        final byte URI_PREFIX_HTTPS     = (byte) 0x04; // "https://"

        NdefMessage message = null;
        byte[] tagId = nfcBarcode.getTag().getId();
        // All tags of NfcBarcode technology and Kovio type have lengths of a multiple of 16 bytes
        if (tagId.length >= 4
                && (tagId[1] == URI_PREFIX_HTTP_WWW || tagId[1] == URI_PREFIX_HTTPS_WWW
                || tagId[1] == URI_PREFIX_HTTP || tagId[1] == URI_PREFIX_HTTPS)) {
            // Look for optional URI terminator (0xfe), used to indicate the end of a URI prior to
            // the end of the full NfcBarcode payload. No terminator means that the URI occupies the
            // entire length of the payload field. Exclude checking the CRC in the final two bytes
            // of the NfcBarcode tagId.
            int end = 2;
            for (; end < tagId.length - 2; end++) {
                if (tagId[end] == (byte) 0xfe) {
                    break;
                }
            }
            byte[] payload = new byte[end - 1]; // Skip also first byte (manufacturer ID)
            System.arraycopy(tagId, 1, payload, 0, payload.length);
            NdefRecord uriRecord = new NdefRecord(
                    NdefRecord.TNF_WELL_KNOWN, NdefRecord.RTD_URI, tagId, payload);
            message = new NdefMessage(uriRecord);
        }
        return message;
    }

    boolean tryOverrides(DispatchInfo dispatch, Tag tag, NdefMessage message, PendingIntent overrideIntent,
            IntentFilter[] overrideFilters, String[][] overrideTechLists) {
        if (overrideIntent == null) {
            return false;
        }
        Intent intent;

        // NDEF
        if (message != null) {
            intent = dispatch.setNdefIntent();
            if (intent != null &&
                    isFilterMatch(intent, overrideFilters, overrideTechLists != null)) {
                try {
                    overrideIntent.send(mContext, Activity.RESULT_OK, intent);
                    if (DBG) Log.i(TAG, "matched NDEF override");
                    return true;
                } catch (CanceledException e) {
                    return false;
                }
            }
        }

        // TECH
        intent = dispatch.setTechIntent();
        if (isTechMatch(tag, overrideTechLists)) {
            try {
                overrideIntent.send(mContext, Activity.RESULT_OK, intent);
                if (DBG) Log.i(TAG, "matched TECH override");
                return true;
            } catch (CanceledException e) {
                return false;
            }
        }

        // TAG
        intent = dispatch.setTagIntent();
        if (isFilterMatch(intent, overrideFilters, overrideTechLists != null)) {
            try {
                overrideIntent.send(mContext, Activity.RESULT_OK, intent);
                if (DBG) Log.i(TAG, "matched TAG override");
                return true;
            } catch (CanceledException e) {
                return false;
            }
        }
        return false;
    }

    boolean isFilterMatch(Intent intent, IntentFilter[] filters, boolean hasTechFilter) {
        if (filters != null) {
            for (IntentFilter filter : filters) {
                if (filter.match(mContentResolver, intent, false, TAG) >= 0) {
                    return true;
                }
            }
        } else if (!hasTechFilter) {
            return true;  // always match if both filters and techlists are null
        }
        return false;
    }

    boolean isTechMatch(Tag tag, String[][] techLists) {
        if (techLists == null) {
            return false;
        }

        String[] tagTechs = tag.getTechList();
        Arrays.sort(tagTechs);
        for (String[] filterTechs : techLists) {
            if (filterMatch(tagTechs, filterTechs)) {
                return true;
            }
        }
        return false;
    }

    boolean tryNdef(DispatchInfo dispatch, NdefMessage message, boolean provisioningOnly) {
        if (message == null) {
            return false;
        }
        Intent intent = dispatch.setNdefIntent();

        // Bail out if the intent does not contain filterable NDEF data
        if (intent == null) return false;

        if (provisioningOnly) {
            if (mProvisioningMimes == null ||
                    !(Arrays.asList(mProvisioningMimes).contains(intent.getType()))) {
                Log.e(TAG, "Dropping NFC intent in provisioning mode.");
                return false;
            }
        }

        // Try to start AAR activity with matching filter
        List<String> aarPackages = extractAarPackages(message);
        for (String pkg : aarPackages) {
            dispatch.intent.setPackage(pkg);
            if (dispatch.tryStartActivity()) {
                if (DBG) Log.i(TAG, "matched AAR to NDEF");
                return true;
            }
        }

        // Try to perform regular launch of the first AAR
        if (aarPackages.size() > 0) {
            String firstPackage = aarPackages.get(0);
            PackageManager pm;
            try {
                UserHandle currentUser = new UserHandle(ActivityManager.getCurrentUser());
                pm = mContext.createPackageContextAsUser("android", 0,
                        currentUser).getPackageManager();
            } catch (NameNotFoundException e) {
                Log.e(TAG, "Could not create user package context");
                return false;
            }
            Intent appLaunchIntent = pm.getLaunchIntentForPackage(firstPackage);
            if (appLaunchIntent != null && dispatch.tryStartActivity(appLaunchIntent)) {
                if (DBG) Log.i(TAG, "matched AAR to application launch");
                return true;
            }
            // Find the package in Market:
            Intent marketIntent = getAppSearchIntent(firstPackage);
            if (marketIntent != null && dispatch.tryStartActivity(marketIntent)) {
                if (DBG) Log.i(TAG, "matched AAR to market launch");
                return true;
            }
        }

        // regular launch
        dispatch.intent.setPackage(null);
        if (dispatch.tryStartActivity()) {
            if (DBG) Log.i(TAG, "matched NDEF");
            return true;
        }

        return false;
    }

    static List<String> extractAarPackages(NdefMessage message) {
        List<String> aarPackages = new LinkedList<String>();
        for (NdefRecord record : message.getRecords()) {
            String pkg = checkForAar(record);
            if (pkg != null) {
                aarPackages.add(pkg);
            }
        }
        return aarPackages;
    }

    boolean tryTech(DispatchInfo dispatch, Tag tag) {
        dispatch.setTechIntent();

        String[] tagTechs = tag.getTechList();
        Arrays.sort(tagTechs);

        // Standard tech dispatch path
        ArrayList<ResolveInfo> matches = new ArrayList<ResolveInfo>();
        List<ComponentInfo> registered = mTechListFilters.getComponents();

        PackageManager pm;
        try {
            UserHandle currentUser = new UserHandle(ActivityManager.getCurrentUser());
            pm = mContext.createPackageContextAsUser("android", 0,
                    currentUser).getPackageManager();
        } catch (NameNotFoundException e) {
            Log.e(TAG, "Could not create user package context");
            return false;
        }
        // Check each registered activity to see if it matches
        for (ComponentInfo info : registered) {
            // Don't allow wild card matching
            if (filterMatch(tagTechs, info.techs) &&
                    isComponentEnabled(pm, info.resolveInfo)) {
                // Add the activity as a match if it's not already in the list
                if (!matches.contains(info.resolveInfo)) {
                    matches.add(info.resolveInfo);
                }
            }
        }

        if (matches.size() == 1) {
            // Single match, launch directly
            ResolveInfo info = matches.get(0);
            dispatch.intent.setClassName(info.activityInfo.packageName, info.activityInfo.name);
            if (dispatch.tryStartActivity()) {
                if (DBG) Log.i(TAG, "matched single TECH");
                return true;
            }
            dispatch.intent.setComponent(null);
        } else if (matches.size() > 1) {
            // Multiple matches, show a custom activity chooser dialog
            Intent intent = new Intent(mContext, TechListChooserActivity.class);
            intent.putExtra(Intent.EXTRA_INTENT, dispatch.intent);
            intent.putParcelableArrayListExtra(TechListChooserActivity.EXTRA_RESOLVE_INFOS,
                    matches);
            if (dispatch.tryStartActivity(intent)) {
                if (DBG) Log.i(TAG, "matched multiple TECH");
                return true;
            }
        }
        return false;
    }

    public boolean tryPeripheralHandover(NdefMessage m) {
        if (m == null || !mDeviceSupportsBluetooth) return false;

        if (DBG) Log.d(TAG, "tryHandover(): " + m.toString());

        HandoverDataParser.BluetoothHandoverData handover = mHandoverDataParser.parseBluetooth(m);
        if (handover == null || !handover.valid) return false;

        Intent intent = new Intent(mContext, PeripheralHandoverService.class);
        intent.putExtra(PeripheralHandoverService.EXTRA_PERIPHERAL_DEVICE, handover.device);
        intent.putExtra(PeripheralHandoverService.EXTRA_PERIPHERAL_NAME, handover.name);
        intent.putExtra(PeripheralHandoverService.EXTRA_PERIPHERAL_TRANSPORT, handover.transport);
        mContext.startServiceAsUser(intent, UserHandle.CURRENT);

        return true;
    }

    /**
     * Tells the ActivityManager to resume allowing app switches.
     *
     * If the current app called stopAppSwitches() then our startActivity() can
     * be delayed for several seconds. This happens with the default home
     * screen.  As a system service we can override this behavior with
     * resumeAppSwitches().
    */
    void resumeAppSwitches() {
        try {
            mIActivityManager.resumeAppSwitches();
        } catch (RemoteException e) { }
    }

    /** Returns true if the tech list filter matches the techs on the tag */
    boolean filterMatch(String[] tagTechs, String[] filterTechs) {
        if (filterTechs == null || filterTechs.length == 0) return false;

        for (String tech : filterTechs) {
            if (Arrays.binarySearch(tagTechs, tech) < 0) {
                return false;
            }
        }
        return true;
    }

    static String checkForAar(NdefRecord record) {
        if (record.getTnf() == NdefRecord.TNF_EXTERNAL_TYPE &&
                Arrays.equals(record.getType(), NdefRecord.RTD_ANDROID_APP)) {
            return new String(record.getPayload(), StandardCharsets.US_ASCII);
        }
        return null;
    }

    /**
     * Returns an intent that can be used to find an application not currently
     * installed on the device.
     */
    static Intent getAppSearchIntent(String pkg) {
        Intent market = new Intent(Intent.ACTION_VIEW);
        market.setData(Uri.parse("market://details?id=" + pkg));
        return market;
    }

    static boolean isComponentEnabled(PackageManager pm, ResolveInfo info) {
        boolean enabled = false;
        ComponentName compname = new ComponentName(
                info.activityInfo.packageName, info.activityInfo.name);
        try {
            // Note that getActivityInfo() will internally call
            // isEnabledLP() to determine whether the component
            // enabled. If it's not, null is returned.
            if (pm.getActivityInfo(compname,0) != null) {
                enabled = true;
            }
        } catch (PackageManager.NameNotFoundException e) {
            enabled = false;
        }
        if (!enabled) {
            Log.d(TAG, "Component not enabled: " + compname);
        }
        return enabled;
    }

    void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        synchronized (this) {
            pw.println("mOverrideIntent=" + mOverrideIntent);
            pw.println("mOverrideFilters=" + mOverrideFilters);
            pw.println("mOverrideTechLists=" + mOverrideTechLists);
        }
    }
}
