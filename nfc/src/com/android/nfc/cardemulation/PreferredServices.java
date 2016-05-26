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
package com.android.nfc.cardemulation;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

import com.android.nfc.ForegroundUtils;

import android.app.ActivityManager;
import android.content.ComponentName;
import android.content.Context;
import android.database.ContentObserver;
import android.net.Uri;
import android.nfc.cardemulation.ApduServiceInfo;
import android.nfc.cardemulation.CardEmulation;
import android.os.Handler;
import android.os.Looper;
import android.os.UserHandle;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.util.Log;

/**
 * This class keeps track of what HCE/SE-based services are
 * preferred by the user. It currently has 3 inputs:
 * 1) The default set in tap&pay menu for payment category
 * 2) An app in the foreground asking for a specific
 *    service for a specific category
 * 3) If we had to disambiguate a previous tap (because no
 *    preferred service was there), we need to temporarily
 *    store the user's choice for the next tap.
 *
 * This class keeps track of all 3 inputs, and computes a new
 * preferred services as needed. It then passes this service
 * (if it changed) through a callback, which allows other components
 * to adapt as necessary (ie the AID cache can update its AID
 * mappings and the routing table).
 */
public class PreferredServices implements com.android.nfc.ForegroundUtils.Callback {
    static final String TAG = "PreferredCardEmulationServices";
    static final Uri paymentDefaultUri = Settings.Secure.getUriFor(
            Settings.Secure.NFC_PAYMENT_DEFAULT_COMPONENT);
    static final Uri paymentForegroundUri = Settings.Secure.getUriFor(
            Settings.Secure.NFC_PAYMENT_FOREGROUND);

    final SettingsObserver mSettingsObserver;
    final Context mContext;
    final RegisteredServicesCache mServiceCache;
    final RegisteredAidCache mAidCache;
    final Callback mCallback;
    final ForegroundUtils mForegroundUtils = ForegroundUtils.getInstance();
    final Handler mHandler = new Handler(Looper.getMainLooper());

    final class PaymentDefaults {
        boolean preferForeground; // The current selection mode for this category
        ComponentName settingsDefault; // The component preferred in settings (eg Tap&Pay)
        ComponentName currentPreferred; // The computed preferred component
    }

    final Object mLock = new Object();
    // Variables below synchronized on mLock
    PaymentDefaults mPaymentDefaults = new PaymentDefaults();

    ComponentName mForegroundRequested; // The component preferred by fg app
    int mForegroundUid; // The UID of the fg app, or -1 if fg app didn't request

    ComponentName mNextTapDefault; // The component preferred by active disambig dialog
    boolean mClearNextTapDefault = false; // Set when the next tap default must be cleared

    ComponentName mForegroundCurrent; // The currently computed foreground component

    public interface Callback {
        void onPreferredPaymentServiceChanged(ComponentName service);
        void onPreferredForegroundServiceChanged(ComponentName service);
    }

    public PreferredServices(Context context, RegisteredServicesCache serviceCache,
            RegisteredAidCache aidCache, Callback callback) {
        mContext = context;
        mServiceCache = serviceCache;
        mAidCache = aidCache;
        mCallback = callback;
        mSettingsObserver = new SettingsObserver(mHandler);
        mContext.getContentResolver().registerContentObserver(
                paymentDefaultUri,
                true, mSettingsObserver, UserHandle.USER_ALL);

        mContext.getContentResolver().registerContentObserver(
                paymentForegroundUri,
                true, mSettingsObserver, UserHandle.USER_ALL);

        // Load current settings defaults for payments
        loadDefaultsFromSettings(ActivityManager.getCurrentUser());
    }

    private final class SettingsObserver extends ContentObserver {
        public SettingsObserver(Handler handler) {
            super(handler);
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            super.onChange(selfChange, uri);
            // Do it just for the current user. If it was in fact
            // a change made for another user, we'll sync it down
            // on user switch.
            int currentUser = ActivityManager.getCurrentUser();
            loadDefaultsFromSettings(currentUser);
        }
    };

    void loadDefaultsFromSettings(int userId) {
        boolean paymentDefaultChanged = false;
        boolean paymentPreferForegroundChanged = false;
        // Load current payment default from settings
        String name = Settings.Secure.getStringForUser(
                mContext.getContentResolver(), Settings.Secure.NFC_PAYMENT_DEFAULT_COMPONENT,
                userId);
        ComponentName newDefault = name != null ? ComponentName.unflattenFromString(name) : null;
        boolean preferForeground = false;
        try {
            preferForeground = Settings.Secure.getInt(mContext.getContentResolver(),
                    Settings.Secure.NFC_PAYMENT_FOREGROUND) != 0;
        } catch (SettingNotFoundException e) {
        }
        synchronized (mLock) {
            paymentPreferForegroundChanged = (preferForeground != mPaymentDefaults.preferForeground);
            mPaymentDefaults.preferForeground = preferForeground;

            mPaymentDefaults.settingsDefault = newDefault;
            if (newDefault != null && !newDefault.equals(mPaymentDefaults.currentPreferred)) {
                paymentDefaultChanged = true;
                mPaymentDefaults.currentPreferred = newDefault;
            } else if (newDefault == null && mPaymentDefaults.currentPreferred != null) {
                paymentDefaultChanged = true;
                mPaymentDefaults.currentPreferred = newDefault;
            } else {
                // Same default as before
            }
        }
        // Notify if anything changed
        if (paymentDefaultChanged) {
            mCallback.onPreferredPaymentServiceChanged(newDefault);
        }
        if (paymentPreferForegroundChanged) {
            computePreferredForegroundService();
        }
    }

    void computePreferredForegroundService() {
        ComponentName preferredService = null;
        boolean changed = false;
        synchronized (mLock) {
            // Prio 1: next tap default
            preferredService = mNextTapDefault;
            if (preferredService == null) {
                // Prio 2: foreground requested by app
                preferredService = mForegroundRequested;
            }
            if (preferredService != null && !preferredService.equals(mForegroundCurrent)) {
                mForegroundCurrent = preferredService;
                changed = true;
            } else if (preferredService == null && mForegroundCurrent != null){
                mForegroundCurrent = preferredService;
                changed = true;
            }
        }
        // Notify if anything changed
        if (changed) {
            mCallback.onPreferredForegroundServiceChanged(preferredService);
        }
    }

    public boolean setDefaultForNextTap(ComponentName service) {
        // This is a trusted API, so update without checking
        synchronized (mLock) {
            mNextTapDefault = service;
        }
        computePreferredForegroundService();
        return true;
    }

    public void onServicesUpdated() {
        // If this service is the current foreground service, verify
        // there are no conflicts
        boolean changed = false;
        synchronized (mLock) {
            // Check if the current foreground service is still allowed to override;
            // it could have registered new AIDs that make it conflict with user
            // preferences.
            if (mForegroundCurrent != null) {
                if (!isForegroundAllowedLocked(mForegroundCurrent))  {
                    Log.d(TAG, "Removing foreground preferred service because of conflict.");
                    mForegroundRequested = null;
                    mForegroundUid = -1;
                    changed = true;
                }
            } else {
                // Don't care about this service
            }
        }
        if (changed) {
            computePreferredForegroundService();
        }
    }

    // Verifies whether a service is allowed to register as preferred
    boolean isForegroundAllowedLocked(ComponentName service) {
        if (service.equals(mPaymentDefaults.currentPreferred)) {
            // If the requester is already the payment default, allow it to request foreground
            // override as well (it could use this to make sure it handles AIDs of category OTHER)
            return true;
        }
        ApduServiceInfo serviceInfo = mServiceCache.getService(ActivityManager.getCurrentUser(),
                service);
        // Do some sanity checking
        if (!mPaymentDefaults.preferForeground) {
            // Foreground apps are not allowed to override payment default
            // Check if this app registers payment AIDs, in which case we'll fail anyway
            if (serviceInfo.hasCategory(CardEmulation.CATEGORY_PAYMENT)) {
                Log.d(TAG, "User doesn't allow payment services to be overridden.");
                return false;
            }
            // If no payment AIDs, get AIDs of category other, and see if there's any
            // conflict with payment AIDs of current default payment app. That means
            // the current default payment app said this was a payment AID, and the
            // foreground app says it was not. In this case we'll still prefer the payment
            // app, since that is the one that the user has explicitly selected (and said
            // it's not allowed to be overridden).
            final List<String> otherAids = serviceInfo.getAids();
            ApduServiceInfo paymentServiceInfo = mServiceCache.getService(
                    ActivityManager.getCurrentUser(), mPaymentDefaults.currentPreferred);
            if (paymentServiceInfo != null && otherAids != null && otherAids.size() > 0) {
                for (String aid : otherAids) {
                    RegisteredAidCache.AidResolveInfo resolveInfo = mAidCache.resolveAid(aid);
                    if (CardEmulation.CATEGORY_PAYMENT.equals(resolveInfo.category) &&
                            paymentServiceInfo.equals(resolveInfo.defaultService) ) {
                        Log.d(TAG, "AID " + aid + " is handled by the default payment app, " +
                                "and the user has not allowed payments to be overridden.");
                        return false;
                    }
                }
                return true;
            } else {
                // Could not find payment service or fg app doesn't register other AIDs;
                // okay to proceed.
                return true;
            }
        } else {
            // Payment allows override, so allow anything.
            return true;
        }
    }

    public boolean registerPreferredForegroundService(ComponentName service, int callingUid) {
        boolean success = false;
        synchronized (mLock) {
            if (isForegroundAllowedLocked(service)) {
                if (mForegroundUtils.registerUidToBackgroundCallback(this, callingUid)) {
                    mForegroundRequested = service;
                    mForegroundUid = callingUid;
                    success = true;
                } else {
                    Log.e(TAG, "Calling UID is not in the foreground, ignorning!");
                    success = false;
                }
            } else {
                Log.e(TAG, "Requested foreground service conflicts with default payment app.");
            }
        }
        if (success) {
            computePreferredForegroundService();
        }
        return success;
    }

    boolean unregisterForegroundService(int uid) {
        boolean success = false;
        synchronized (mLock) {
            if (mForegroundUid == uid) {
                mForegroundRequested = null;
                mForegroundUid = -1;
                success = true;
            } // else, other UID in foreground
        }
        if (success) {
            computePreferredForegroundService();
        }
        return success;
    }

    public boolean unregisteredPreferredForegroundService(int callingUid) {
        // Verify the calling UID is in the foreground
        if (mForegroundUtils.isInForeground(callingUid)) {
            return unregisterForegroundService(callingUid);
        } else {
            Log.e(TAG, "Calling UID is not in the foreground, ignorning!");
            return false;
        }
    }

    @Override
    public void onUidToBackground(int uid) {
        unregisterForegroundService(uid);
    }

    public void onHostEmulationActivated() {
        synchronized (mLock) {
            mClearNextTapDefault = (mNextTapDefault != null);
        }
    }

    public void onHostEmulationDeactivated() {
        // If we had any next tap defaults set, clear them out
        boolean changed = false;
        synchronized (mLock) {
            if (mClearNextTapDefault) {
                // The reason we need to check this boolean is because the next tap
                // default may have been set while the user held the phone
                // on the reader; when the user then removes his phone from
                // the reader (causing the "onHostEmulationDeactivated" event),
                // the next tap default would immediately be cleared
                // again. Instead, clear out defaults only if a next tap default
                // had already been set at time of activation, which is captured
                // by mClearNextTapDefault.
                if (mNextTapDefault != null) {
                    mNextTapDefault = null;
                    changed = true;
                }
                mClearNextTapDefault = false;
            }
        }
        if (changed) {
            computePreferredForegroundService();
        }
    }

    public void onUserSwitched(int userId) {
        loadDefaultsFromSettings(userId);
    }

    public boolean packageHasPreferredService(String packageName) {
        if (packageName == null) return false;

        if (mPaymentDefaults.currentPreferred != null &&
                packageName.equals(mPaymentDefaults.currentPreferred.getPackageName())) {
            return true;
        } else if (mForegroundCurrent != null &&
                packageName.equals(mForegroundCurrent.getPackageName())) {
            return true;
        } else {
            return false;
        }
    }


    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("Preferred services (in order of importance): ");
        pw.println("    *** Current preferred foreground service: " + mForegroundCurrent);
        pw.println("    *** Current preferred payment service: " + mPaymentDefaults.currentPreferred);
        pw.println("        Next tap default: " + mNextTapDefault);
        pw.println("        Default for foreground app (UID: " + mForegroundUid +
                "): " + mForegroundRequested);
        pw.println("        Default in payment settings: " + mPaymentDefaults.settingsDefault);
        pw.println("        Payment settings allows override: " + mPaymentDefaults.preferForeground);
        pw.println("");
    }
}
