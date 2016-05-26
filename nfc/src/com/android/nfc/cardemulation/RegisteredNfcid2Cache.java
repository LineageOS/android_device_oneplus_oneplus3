/*
 * Copyright (C) 2015 NXP Semiconductors
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

import android.app.ActivityManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.database.ContentObserver;
import android.net.Uri;
import android.nfc.cardemulation.ApduServiceInfo;
import android.nfc.cardemulation.CardEmulation;
import android.nfc.cardemulation.ApduServiceInfo.Nfcid2Group;
import android.nfc.cardemulation.ApduServiceInfo.ESeInfo;
import android.os.Handler;
import android.os.Looper;
import android.os.UserHandle;
import android.provider.Settings;
import android.util.Log;

import com.android.nfc.cardemulation.Nfcid2RoutingManager;
import com.google.android.collect.Maps;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.SortedMap;
import java.util.TreeMap;

public class RegisteredNfcid2Cache /*implements RegisteredServicesCache.Callback*/ {
    static final String TAG = "RegisteredNfcid2Cache";

    static final boolean DBG = true;

    // mAidServices is a tree that maps an AID to a list of handling services
    // on Android. It is only valid for the current user.
    final TreeMap<String, ArrayList<ApduServiceInfo>> mNfcid2ToServices =
            new TreeMap<String, ArrayList<ApduServiceInfo>>();

    // mNfcid2Cache is a lookup table for quickly mapping an AID to one or
    // more services. It differs from mAidServices in the sense that it
    // has already accounted for defaults, and hence its return value
    // is authoritative for the current set of services and defaults.
    // It is only valid for the current user.
    final HashMap<String, Nfcid2ResolveInfo> mNfcid2Cache =
            Maps.newHashMap();

    final HashMap<String, ComponentName> mCategoryDefaults =
            Maps.newHashMap();

    final class Nfcid2ResolveInfo {
        List<ApduServiceInfo> services;
        ApduServiceInfo defaultService;
        String nfcid2;
    }

    /**
     * AIDs per category
     */
    public final HashMap<String, Set<String>> mCategoryNfcid2s =
            Maps.newHashMap();

    final Handler mHandler = new Handler(Looper.getMainLooper());
    final RegisteredServicesCache mServiceCache;

    final Object mLock = new Object();
    final Context mContext;
    final Nfcid2RoutingManager mRoutingManager;
    final SettingsObserver mSettingsObserver;

    ComponentName mNextTapComponent = null;
    boolean mIsResolvingConflict = false;
    boolean mNfcEnabled = false;
    List<ApduServiceInfo> mServicesCache;

    private final class SettingsObserver extends ContentObserver {
        public SettingsObserver(Handler handler) {
            super(handler);
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            super.onChange(selfChange, uri);
            synchronized (mLock) {
                // Do it just for the current user. If it was in fact
                // a change made for another user, we'll sync it down
                // on user switch.
//                int currentUser = ActivityManager.getCurrentUser();
//                boolean changed = updateFromSettingsLocked(currentUser);
//                if (changed) {
//                    mRoutingManager.onNfccRoutingTableCleared();
//                    generateNfcid2TreeLocked(currentUser, mServicesCache);
//                    generateNfcid2CategoriesLocked(mServicesCache);
//                    generateNfcid2CacheLocked();
//                    updateRoutingLocked(currentUser);
//                } else {
//                    if (DBG) Log.d(TAG, "Not updating aid cache + routing: nothing changed.");
//                }
            }
        }
    };

    public RegisteredNfcid2Cache(Context context, Nfcid2RoutingManager routingManager, RegisteredServicesCache serviceCache) {
        mSettingsObserver = new SettingsObserver(mHandler);
        mContext = context;
        mServiceCache = serviceCache;
        mRoutingManager = routingManager;

        mContext.getContentResolver().registerContentObserver(
                Settings.Secure.getUriFor(Settings.Secure.NFC_PAYMENT_DEFAULT_COMPONENT),
                true, mSettingsObserver, UserHandle.USER_ALL);
    }

    public boolean isNextTapOverriden() {
        synchronized (mLock) {
            return mNextTapComponent != null;
        }
    }

    public Nfcid2ResolveInfo resolveNfcid2Prefix(String nfcid2) {
        synchronized (mLock) {
            if (mNfcid2ToServices.containsKey(nfcid2)) {
                Nfcid2ResolveInfo resolveInfo = mNfcid2Cache.get(nfcid2);
                // Let the caller know which NFCID2 got selected
                resolveInfo.nfcid2 = nfcid2;
                return resolveInfo;
            }
            return null;
        }
    }

    public String getCategoryForNfcid2(String nfcid2) {
        synchronized (mLock) {
            Set<String> paymentAids = mCategoryNfcid2s.get(CardEmulation.CATEGORY_PAYMENT);
            if (paymentAids != null && paymentAids.contains(nfcid2)) {
                return CardEmulation.CATEGORY_PAYMENT;
            } else {
                return CardEmulation.CATEGORY_OTHER;
            }
        }
    }

    public boolean setDefaultServiceForCategory(int userId, ComponentName service,
            String category) {
        if (!CardEmulation.CATEGORY_PAYMENT.equals(category)) {
            Log.e(TAG, "Not allowing defaults for category " + category);
            return false;
        }
        synchronized (mLock) {
            // TODO Not really nice to be writing to Settings.Secure here...
            // ideally we overlay our local changes over whatever is in
            // Settings.Secure
            if (service == null || mServiceCache.hasService(userId, service)) {
                Settings.Secure.putStringForUser(mContext.getContentResolver(),
                        Settings.Secure.NFC_PAYMENT_DEFAULT_COMPONENT,
                        service != null ? service.flattenToString() : null, userId);
            } else {
                Log.e(TAG, "Could not find default service to make default: " + service);
            }
        }
        return true;
    }

    public boolean isDefaultServiceForCategory(int userId, String category,
            ComponentName service) {
        boolean serviceFound = false;
        synchronized (mLock) {
            // If we don't know about this service yet, it may have just been enabled
            // using PackageManager.setComponentEnabledSetting(). The PackageManager
            // broadcasts are delayed by 10 seconds in that scenario, which causes
            // calls to our APIs referencing that service to fail.
            // Hence, update the cache in case we don't know about the service.
            serviceFound = mServiceCache.hasService(userId, service);
        }
        if (!serviceFound) {
            if (DBG) Log.d(TAG, "Didn't find passed in service, invalidating cache.");
            mServiceCache.invalidateCache(userId);
        }
        ComponentName defaultService =
                getDefaultServiceForCategory(userId, category, true);
        return (defaultService != null && defaultService.equals(service));
    }

    ComponentName getDefaultServiceForCategory(int userId, String category,
            boolean validateInstalled) {
        if (!CardEmulation.CATEGORY_PAYMENT.equals(category)) {
            Log.e(TAG, "Not allowing defaults for category " + category);
            return null;
        }
        synchronized (mLock) {
            // Load current payment default from settings
            String name = Settings.Secure.getStringForUser(
                    mContext.getContentResolver(), Settings.Secure.NFC_PAYMENT_DEFAULT_COMPONENT,
                    userId);
            if (name != null) {
                ComponentName service = ComponentName.unflattenFromString(name);
                if (!validateInstalled || service == null) {
                    return service;
                } else {
                    return mServiceCache.hasService(userId, service) ? service : null;
                }
            } else {
                return null;
            }
        }
    }

    public List<ApduServiceInfo> getServicesForCategory(int userId, String category) {
        return mServiceCache.getServicesForCategory(userId, category);
    }

    public boolean setDefaultForNextTap(int userId, ComponentName service) {
        synchronized (mLock) {
            if (service != null) {
                mNextTapComponent = service;
            } else {
                mNextTapComponent = null;
            }
            // Update cache and routing table
            mRoutingManager.onNfccRoutingTableCleared();
            generateNfcid2TreeLocked(userId, mServicesCache);
            generateNfcid2CategoriesLocked(mServicesCache);
            generateNfcid2CacheLocked();
            updateRoutingLocked(userId);
        }
        return true;
    }

    /**
     * Resolves an NFCID2 to a set of services that can handle it.
     */
     Nfcid2ResolveInfo resolveNfcid2Locked(List<ApduServiceInfo> resolvedServices, String nfcid2) {
        if (resolvedServices == null || resolvedServices.size() == 0) {
            if (DBG) Log.d(TAG, "Could not resolve NFCID2 " + nfcid2 + " to any service.");
            return null;
        }
        Nfcid2ResolveInfo resolveInfo = new Nfcid2ResolveInfo();
        if (DBG) Log.d(TAG, "resolveNfcid2Locked: resolving AID " + nfcid2);
        resolveInfo.services = new ArrayList<ApduServiceInfo>();
        resolveInfo.services.addAll(resolvedServices);
        resolveInfo.defaultService = null;

        ComponentName defaultComponent = mNextTapComponent;
        if (DBG) Log.d(TAG, "resolveNfcid2Locked: next tap component is " + defaultComponent);

        Set<String> paymentAids = mCategoryNfcid2s.get(CardEmulation.CATEGORY_PAYMENT);
        if (paymentAids != null && paymentAids.contains(nfcid2)) {
            if (DBG) Log.d(TAG, "resolveNfcid2Locked: AID " + nfcid2 + " is a payment NFCID2");
            if (DBG) Log.d(TAG, "resolveNfcid2Locked: default payment component is "
                    + defaultComponent);
            if (resolvedServices.size() == 1) {
                ApduServiceInfo resolvedService = resolvedServices.get(0);
                if (DBG) Log.d(TAG, "resolveNfcid2Locked: resolved single service " +
                        resolvedService.getComponent());

                if (defaultComponent != null &&
                        defaultComponent.equals(resolvedService.getComponent())) {
                    if (DBG) Log.d(TAG, "resolveNfcid2Locked: DECISION: routing to (default) " +
                        resolvedService.getComponent());
                    resolveInfo.defaultService = resolvedService;
                } else {
                    // So..since we resolved to only one service, and this AID
                    // is a payment AID, we know that this service is the only
                    // service that has registered for this AID and in fact claimed
                    // it was a payment AID.
                    // There's two cases:
                    // 1. All other AIDs in the payment group are uncontended:
                    //    in this case, just route to this app. It won't get
                    //    in the way of other apps, and is likely to interact
                    //    with different terminal infrastructure anyway.
                    // 2. At least one AID in the payment group is contended:
                    //    in this case, we should ask the user to confirm,
                    //    since it is likely to contend with other apps, even
                    //    when touching the same terminal.
                    boolean foundConflict = false;
                    for (Nfcid2Group nfcid2Group : resolvedService.getNfcid2Groups()) {
                        if (nfcid2Group.getCategory().equals(CardEmulation.CATEGORY_PAYMENT)) {
                            for (String registeredNfcid2 : nfcid2Group.getNfcid2s()) {

                                ArrayList<ApduServiceInfo> servicesForNfcid2 =
                                        mNfcid2ToServices.get(registeredNfcid2);

                                if (servicesForNfcid2 != null && servicesForNfcid2.size() > 1) {
                                    foundConflict = true;
                                }

                            }
                        }
                    }
                    if (!foundConflict) {
                        if (DBG) Log.d(TAG, "resolveNfcid2Locked: DECISION: routing to " +
                            resolvedService.getComponent());
                        // Treat this as if it's the default for this AID
                        resolveInfo.defaultService = resolvedService;
                    } else {
                        // Allow this service to handle, but don't set as default
                        if (DBG) Log.d(TAG, "resolveNfcid2Locked: DECISION: routing AID " + nfcid2 +
                                " to " + resolvedService.getComponent() +
                                ", but will ask confirmation because its AID group is contended.");
                    }
                }
            } else if (resolvedServices.size() > 1) {
                // More services have registered. If there's a default and it
                // registered this AID, go with the default. Otherwise, add all.
                if (DBG) Log.d(TAG, "resolveNfcid2Locked: multiple services matched.");
                if (defaultComponent != null) {
                    for (ApduServiceInfo service : resolvedServices) {
                        if (service.getComponent().equals(defaultComponent)) {
                            if (DBG) Log.d(TAG, "resolveNfcid2Locked: DECISION: routing to (default) "
                                    + service.getComponent());
                            resolveInfo.defaultService = service;
                            break;
                        }
                    }
                    if (resolveInfo.defaultService == null) {
                        if (DBG) Log.d(TAG, "resolveNfcid2Locked: DECISION: routing to all services");
                    }
                }
            } // else -> should not hit, we checked for 0 before.
        } else {
            // This AID is not a payment AID, just return all components
            // that can handle it, but be mindful of (next tap) defaults.
            for (ApduServiceInfo service : resolvedServices) {
                if (service.getComponent().equals(defaultComponent)) {
                    if (DBG) Log.d(TAG, "resolveAidLocked: DECISION: cat OTHER AID, " +
                            "routing to (default) " + service.getComponent());
                    resolveInfo.defaultService = service;
                    break;
                }
            }
            if (resolveInfo.defaultService == null) {
                // If we didn't find the default, mark the first as default
                // if there is only one.
                if (resolveInfo.services.size() >= 1 ) {
                    resolveInfo.defaultService = resolveInfo.services.get(0);
                    if (DBG)
                        Log.d(TAG,
                                "resolveAidLocked: DECISION: cat OTHER AID, "
                                        + "routing to (default) "
                                        + resolveInfo.defaultService
                                        .getComponent());
                } else {
                    if (DBG)
                        Log.d(TAG,
                                "resolveAidLocked: DECISION: cat OTHER AID, routing all");
                }
            }
        }
        return resolveInfo;
     }

     void generateNfcid2TreeLocked(int userId, List<ApduServiceInfo> services) {
         // Easiest is to just build the entire tree again
         mNfcid2ToServices.clear();
         for (ApduServiceInfo service : services) {
             if (DBG)Log.d(TAG,"generateAidTree component: " + service.getComponent());
             // Go through aid groups
             for (Nfcid2Group group : service.getNfcid2Groups()) {
                 for (String nfcid2 : group.getNfcid2s()) {
                     if (DBG) Log.d(TAG, "generateNFCID2Tree AID: " + nfcid2);
                     if (mNfcid2ToServices.containsKey(nfcid2)) {
                         final ArrayList<ApduServiceInfo> nfcid2Services = mNfcid2ToServices.get(nfcid2);
                         nfcid2Services.add(service);
                     } else {
                         final ArrayList<ApduServiceInfo> nfcid2Services = new ArrayList<ApduServiceInfo>();
                         nfcid2Services.add(service);
                         mNfcid2ToServices.put(nfcid2, nfcid2Services);
                     }
                 }
             }
         }
     }

    void generateNfcid2CategoriesLocked(List<ApduServiceInfo> services) {
        // Trash existing mapping
        mCategoryNfcid2s.clear();

        for (ApduServiceInfo service : services) {
            ArrayList<Nfcid2Group> nfcid2Groups = service.getNfcid2Groups();
            if (nfcid2Groups == null) continue;
            for (Nfcid2Group nfcid2Group : nfcid2Groups) {
                String groupCategory = nfcid2Group.getCategory();
                Set<String> categoryNfcid2s = mCategoryNfcid2s.get(groupCategory);
                if (categoryNfcid2s == null) {
                    categoryNfcid2s = new HashSet<String>();
                }
                categoryNfcid2s.addAll(nfcid2Group.getNfcid2s());
                mCategoryNfcid2s.put(groupCategory, categoryNfcid2s);
            }
        }
    }

    void generateNfcid2CacheLocked() {
        mNfcid2Cache.clear();
        for (Map.Entry<String, ArrayList<ApduServiceInfo>> nfcid2Entry:
                    mNfcid2ToServices.entrySet()) {
            String nfcid2 = nfcid2Entry.getKey();
            if (!mNfcid2Cache.containsKey(nfcid2)) {
                Log.d(TAG, "generateAidCacheLocked: mNfcid2Cache nfcid2 " + nfcid2 +" , service count " + nfcid2Entry.getValue().size());
                mNfcid2Cache.put(nfcid2, resolveNfcid2Locked(nfcid2Entry.getValue(), nfcid2));
            }
        }
        Log.d(TAG, "generateAidCacheLocked: mNfcid2Cache size " + mNfcid2Cache.size());
    }

    void updateRoutingLocked(int userId) {
        if (!mNfcEnabled) {
            if (DBG) Log.d(TAG, "Not updating routing table because NFC is off.");
            return;
        }
        final Set<String> handledNfcid2s = new HashSet<String>();
        // For each NFCID2, find interested services
        Log.d(TAG, "updateRoutingLocked:  mNfcid2Cache size " + mNfcid2Cache.size());

        for (Map.Entry<String, Nfcid2ResolveInfo> nfcid2Entry:
                mNfcid2Cache.entrySet()) {
            String nfcid2 = nfcid2Entry.getKey();
            Nfcid2ResolveInfo resolveInfo = nfcid2Entry.getValue();
            Log.d(TAG, "updateRoutingLocked:  nfcid2  " + nfcid2 + " , service count " + resolveInfo.services.size());
            if (resolveInfo.services.size() == 0) {
                // No interested services, if there is a current routing remove it
                mRoutingManager.removeNfcid2(nfcid2);
            } else if (resolveInfo.defaultService != null) {
                ArrayList<Nfcid2Group> group = resolveInfo.defaultService.getNfcid2Groups();
                String syscode = group.get(0).getSyscodeForNfcid2(nfcid2);
                String optparam = group.get(0).getOptparamForNfcid2(nfcid2);
                // There is a default service set, route to that service
                if(mNextTapComponent != null) {
                    mIsResolvingConflict = true;
                    mRoutingManager.setRouteForNfcid2(nfcid2, syscode, optparam, true);
                } else {
                    mRoutingManager.setRouteForNfcid2(nfcid2, syscode, optparam, false);

                }
            } else if (resolveInfo.services.size() == 1) {
                // Only one service, but not the default, must route to host
                // to ask the user to confirm.
                mRoutingManager.setRouteForNfcid2(nfcid2, "FFFF", "FFFFFFFFFFFFFFFF", false);
            } else if (resolveInfo.services.size() > 1) {
                // Multiple services, need to route to host to ask
                Log.d(TAG, "updateRoutingLocked: Multiple services, need to route to host to ask");
                if(mIsResolvingConflict) {
                    mIsResolvingConflict = false;
                    mRoutingManager.setRouteForNfcid2(nfcid2, "FFFF", "FFFFFFFFFFFFFFFF", true);
                } else {
                    mRoutingManager.setRouteForNfcid2(nfcid2, "FFFF", "FFFFFFFFFFFFFFFF", false);
                }
            }
            handledNfcid2s.add(nfcid2);
        }
        Log.d(TAG, "Finding NFCID2s to be removed");
        // Now, find NFCID2s that are no longer routed to
        // and remove them.
        Set<String> routedNfcid2s = mRoutingManager.getRoutedNfcid2s();
        for (String nfcid2 : routedNfcid2s) {
            if (!handledNfcid2s.contains(nfcid2)) {
                if (DBG) Log.d(TAG, "Removing routing for AID " + nfcid2 + ", because " +
                        "there are no no interested services.");
                mRoutingManager.removeNfcid2(nfcid2);
            }
        }
        // And commit the routing
        mRoutingManager.commitRouting();
    }

//    @Override
    public void onServicesUpdated(int userId, List<ApduServiceInfo> services) {
        Log.d(TAG, "onServicesUpdated");
        synchronized (mLock) {
            if (ActivityManager.getCurrentUser() == userId) {
                mServicesCache = services;
                // Rebuild our internal data-structures
//                checkDefaultsLocked(userId, services);
                generateNfcid2TreeLocked(userId, services);
                generateNfcid2CategoriesLocked(services);
                generateNfcid2CacheLocked();
                updateRoutingLocked(userId);
            } else {
                if (DBG) Log.d(TAG, "Ignoring update because it's not for the current user.");
            }
        }
    }

    public void onAidFilterUpdated() {
        int userID = ActivityManager.getCurrentUser();
        generateNfcid2TreeLocked(userID,mServicesCache);
        generateNfcid2CategoriesLocked(mServicesCache);
        generateNfcid2CacheLocked();
        updateRoutingLocked(userID);
    }
    public void invalidateCache(int currentUser) {
        mServiceCache.invalidateCache(currentUser);
    }

    public void onNfcDisabled() {
        synchronized (mLock) {
            mNfcEnabled = false;
        }
//        mServiceCache.onNfcDisabled();
        mRoutingManager.onNfccRoutingTableCleared();
    }

    public void onNfcEnabled() {
        synchronized (mLock) {
            mNfcEnabled = true;
//            updateFromSettingsLocked(ActivityManager.getCurrentUser());
        }
//        mServiceCache.onNfcEnabled();
        updateRoutingLocked(ActivityManager.getCurrentUser());
    }

    String dumpEntry(Map.Entry<String, Nfcid2ResolveInfo> entry) {
        StringBuilder sb = new StringBuilder();
        sb.append("    \"" + entry.getKey() + "\"\n");
        ApduServiceInfo defaultService = entry.getValue().defaultService;
        ComponentName defaultComponent = defaultService != null ?
                defaultService.getComponent() : null;

        for (ApduServiceInfo service : entry.getValue().services) {
            sb.append("        ");
            if (service.getComponent().equals(defaultComponent)) {
                sb.append("*DEFAULT* ");
            }
            sb.append(service.getComponent() +
                    " (Description: " + service.getDescription() + ")\n");
        }
        return sb.toString();
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
       mServiceCache.dump(fd, pw, args);
       pw.println("AID cache entries: ");
       for (Map.Entry<String, Nfcid2ResolveInfo> entry : mNfcid2Cache.entrySet()) {
           pw.println(dumpEntry(entry));
       }
       pw.println("Category defaults: ");
       for (Map.Entry<String, ComponentName> entry : mCategoryDefaults.entrySet()) {
           pw.println("    " + entry.getKey() + "->" + entry.getValue());
       }
       pw.println("");
    }
}