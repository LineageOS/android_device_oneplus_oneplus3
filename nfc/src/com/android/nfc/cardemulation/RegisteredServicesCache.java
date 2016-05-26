/*
 * Copyright (C) 2013 The Android Open Source Project
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

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import android.app.ActivityManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.nfc.cardemulation.AidGroup;
import android.nfc.cardemulation.ApduServiceInfo;
import android.nfc.cardemulation.CardEmulation;
import android.nfc.cardemulation.HostApduService;
import android.nfc.cardemulation.OffHostApduService;
import android.os.UserHandle;
import android.util.AtomicFile;
import android.util.Log;
import android.util.SparseArray;
import android.util.Xml;

import com.android.internal.util.FastXmlSerializer;
import com.google.android.collect.Maps;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicReference;
import com.gsma.nfc.internal.RegisteredNxpServicesCache;
import com.android.nfc.NfcService;

/**
 * This class is inspired by android.content.pm.RegisteredServicesCache
 * That class was not re-used because it doesn't support dynamically
 * registering additional properties, but generates everything from
 * the manifest. Since we have some properties that are not in the manifest,
 * it's less suited.
 */
public class RegisteredServicesCache {
    static final String XML_INDENT_OUTPUT_FEATURE = "http://xmlpull.org/v1/doc/features.html#indent-output";
    static final String TAG = "RegisteredServicesCache";
    static final boolean DEBUG = true;

    final Context mContext;
    final AtomicReference<BroadcastReceiver> mReceiver;

    final Object mLock = new Object();
    // All variables below synchronized on mLock

    // mUserServices holds the card emulation services that are running for each user
    final SparseArray<UserServices> mUserServices = new SparseArray<UserServices>();
    final Callback mCallback;
    final AtomicFile mDynamicAidsFile;
    final AtomicFile mServiceStateFile;
    //public ArrayList<ApduServiceInfo> mAllServices = new ArrayList<ApduServiceInfo>();
    final HashMap<ComponentName, ApduServiceInfo> mAllServices = Maps.newHashMap();

    private RegisteredNxpServicesCache mRegisteredNxpServicesCache;

    public interface Callback {
        void onServicesUpdated(int userId, final List<ApduServiceInfo> services);
    };

    static class DynamicAids {
        public final int uid;
        public final HashMap<String, AidGroup> aidGroups = Maps.newHashMap();

        DynamicAids(int uid) {
            this.uid = uid;
        }
    };

    private static class UserServices {
        /**
         * All services that have registered
         */
        final HashMap<ComponentName, ApduServiceInfo> services =
                Maps.newHashMap(); // Re-built at run-time
        final HashMap<ComponentName, DynamicAids> dynamicAids =
                Maps.newHashMap(); // In memory cache of dynamic AID store
    };

    private UserServices findOrCreateUserLocked(int userId) {
        UserServices services = mUserServices.get(userId);
        if (services == null) {
            services = new UserServices();
            mUserServices.put(userId, services);
        }
        return services;
    }

    public RegisteredServicesCache(Context context, Callback callback) {
        mContext = context;
        mCallback = callback;

        final BroadcastReceiver receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                final int uid = intent.getIntExtra(Intent.EXTRA_UID, -1);
                String action = intent.getAction();
                if (DEBUG) Log.d(TAG, "Intent action: " + action);
                if (uid != -1) {
                    boolean replaced = intent.getBooleanExtra(Intent.EXTRA_REPLACING, false) &&
                            (Intent.ACTION_PACKAGE_ADDED.equals(action) ||
                             Intent.ACTION_PACKAGE_REMOVED.equals(action));
                    if (!replaced) {
                        int currentUser = ActivityManager.getCurrentUser();
                        if(Intent.ACTION_PACKAGE_REMOVED.equals(action)) {
                            Uri uri = intent.getData();
                            String pkg = uri != null ? uri.getSchemeSpecificPart() : null;
                            mRegisteredNxpServicesCache.onPackageRemoved(pkg); //GSMA changes
                            mRegisteredNxpServicesCache.writeDynamicApduService();
                        }
                        if (currentUser == UserHandle.getUserId(uid)) {
                            invalidateCache(UserHandle.getUserId(uid));
                        } else {
                            // Cache will automatically be updated on user switch
                        }
                    } else {
                        if (DEBUG) Log.d(TAG, "Ignoring package intent due to package being replaced.");
                    }
                }
            }
        };
        mReceiver = new AtomicReference<BroadcastReceiver>(receiver);

        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(Intent.ACTION_PACKAGE_ADDED);
        intentFilter.addAction(Intent.ACTION_PACKAGE_CHANGED);
        intentFilter.addAction(Intent.ACTION_PACKAGE_REMOVED);
        intentFilter.addAction(Intent.ACTION_PACKAGE_REPLACED);
        intentFilter.addAction(Intent.ACTION_PACKAGE_FIRST_LAUNCH);
        intentFilter.addAction(Intent.ACTION_PACKAGE_RESTARTED);
        intentFilter.addDataScheme("package");
        mContext.registerReceiverAsUser(mReceiver.get(), UserHandle.ALL, intentFilter, null, null);

        // Register for events related to sdcard operations
        IntentFilter sdFilter = new IntentFilter();
        sdFilter.addAction(Intent.ACTION_EXTERNAL_APPLICATIONS_AVAILABLE);
        sdFilter.addAction(Intent.ACTION_EXTERNAL_APPLICATIONS_UNAVAILABLE);
        mContext.registerReceiverAsUser(mReceiver.get(), UserHandle.ALL, sdFilter, null, null);

        File dataDir = mContext.getFilesDir();
        mDynamicAidsFile = new AtomicFile(new File(dataDir, "dynamic_aids.xml"));
        mServiceStateFile = new AtomicFile(new File(dataDir, "service_state.xml"));
    }

    void initialize(RegisteredNxpServicesCache registeredNxpServicesCache) {
        mRegisteredNxpServicesCache = registeredNxpServicesCache;
        synchronized (mLock) {
            readDynamicAidsLocked();
            mRegisteredNxpServicesCache.readDynamicApduService();
        }
        invalidateCache(ActivityManager.getCurrentUser());
    }

    void dump(ArrayList<ApduServiceInfo> services) {
        for (ApduServiceInfo service : services) {
            if (DEBUG) Log.d(TAG, service.toString());
        }
    }

    boolean containsServiceLocked(ArrayList<ApduServiceInfo> services, ComponentName serviceName) {
        for (ApduServiceInfo service : services) {
            if (service.getComponent().equals(serviceName)) return true;
        }
        return false;
    }

    public boolean hasService(int userId, ComponentName service) {
        return getService(userId, service) != null;
    }

    public ApduServiceInfo getService(int userId, ComponentName service) {
        synchronized (mLock) {
            UserServices userServices = findOrCreateUserLocked(userId);
            return userServices.services.get(service);
        }
    }

    public List<ApduServiceInfo> getServices(int userId) {
        final ArrayList<ApduServiceInfo> services = new ArrayList<ApduServiceInfo>();
        synchronized (mLock) {
            UserServices userServices = findOrCreateUserLocked(userId);
            services.addAll(userServices.services.values());
        }
        return services;
    }

    public List<ApduServiceInfo> getServicesForCategory(int userId, String category) {
        final ArrayList<ApduServiceInfo> services = new ArrayList<ApduServiceInfo>();
        synchronized (mLock) {
            UserServices userServices = findOrCreateUserLocked(userId);
            for (ApduServiceInfo service : userServices.services.values()) {
                if (service.hasCategory(category)) services.add(service);
            }
        }
        return services;
    }

    ArrayList<ApduServiceInfo> getInstalledServices(int userId) {
        PackageManager pm;
        try {
            pm = mContext.createPackageContextAsUser("android", 0,
                    new UserHandle(userId)).getPackageManager();
        } catch (NameNotFoundException e) {
            Log.e(TAG, "Could not create user package context");
            return null;
        }
        mAllServices.clear();
        ArrayList<ApduServiceInfo> validServices = new ArrayList<ApduServiceInfo>();

        List<ResolveInfo> resolvedServices = pm.queryIntentServicesAsUser(
                new Intent(HostApduService.SERVICE_INTERFACE),
                PackageManager.GET_META_DATA, userId);

        List<ResolveInfo> resolvedOffHostServices = pm.queryIntentServicesAsUser(
                new Intent(OffHostApduService.SERVICE_INTERFACE),
                PackageManager.GET_META_DATA, userId);
        resolvedServices.addAll(resolvedOffHostServices);

        for (ResolveInfo resolvedService : resolvedServices) {
            try {
                boolean onHost = !resolvedOffHostServices.contains(resolvedService);
                ServiceInfo si = resolvedService.serviceInfo;
                ComponentName componentName = new ComponentName(si.packageName, si.name);
                // Check if the package holds the NFC permission
                if (pm.checkPermission(android.Manifest.permission.NFC, si.packageName) !=
                        PackageManager.PERMISSION_GRANTED) {
                    Log.e(TAG, "Skipping APDU service " + componentName +
                            ": it does not require the permission " +
                            android.Manifest.permission.NFC);
                    continue;
                }
                if (!android.Manifest.permission.BIND_NFC_SERVICE.equals(
                        si.permission)) {
                    Log.e(TAG, "Skipping APDU service " + componentName +
                            ": it does not require the permission " +
                            android.Manifest.permission.BIND_NFC_SERVICE);
                    continue;
                }
                ApduServiceInfo service = new ApduServiceInfo(pm, resolvedService, onHost);
                if (service != null) {
                    validServices.add(service);
                    if(!onHost)
                        mAllServices.put(componentName, service);
                }
            } catch (XmlPullParserException e) {
                Log.w(TAG, "Unable to load component info " + resolvedService.toString(), e);
            } catch (IOException e) {
                Log.w(TAG, "Unable to load component info " + resolvedService.toString(), e);
            }
        }
        AddGsmaServices(validServices);
        return validServices;
    }

    public ArrayList<ApduServiceInfo> getAllServices() {
        return new ArrayList<ApduServiceInfo>(mAllServices.values());//mAllServices;
    }

    public HashMap<ComponentName, ApduServiceInfo> getAllStaticHashServices() {
        return mAllServices;
    }

//Adding the GSMA Services to the Service List
 private void AddGsmaServices(ArrayList<ApduServiceInfo> validServices){
    validServices.addAll(mRegisteredNxpServicesCache.getApduservicesList());
 }

    public void invalidateCache(int userId) {
        final ArrayList<ApduServiceInfo> validServices = getInstalledServices(userId);
        if (validServices == null) {
            return;
        }
        synchronized (mLock) {
            UserServices userServices = findOrCreateUserLocked(userId);

            // Find removed services
            Iterator<Map.Entry<ComponentName, ApduServiceInfo>> it =
                    userServices.services.entrySet().iterator();
            while (it.hasNext()) {
                Map.Entry<ComponentName, ApduServiceInfo> entry =
                        (Map.Entry<ComponentName, ApduServiceInfo>) it.next();
                if (!containsServiceLocked(validServices, entry.getKey())) {
                    Log.d(TAG, "Service removed: " + entry.getKey());
                    it.remove();
                }
            }
            for (ApduServiceInfo service : validServices) {
                if (DEBUG) Log.d(TAG, "Adding service: " + service.getComponent() +
                        " AIDs: " + service.getAids());
                userServices.services.put(service.getComponent(), service);
            }

            // Apply dynamic AID mappings
            ArrayList<ComponentName> toBeRemoved = new ArrayList<ComponentName>();
            for (Map.Entry<ComponentName, DynamicAids> entry :
                    userServices.dynamicAids.entrySet()) {
                // Verify component / uid match
                ComponentName component = entry.getKey();
                DynamicAids dynamicAids = entry.getValue();
                ApduServiceInfo serviceInfo = userServices.services.get(component);
                if (serviceInfo == null || (serviceInfo.getUid() != dynamicAids.uid)) {
                    toBeRemoved.add(component);
                    continue;
                } else {
                    for (AidGroup group : dynamicAids.aidGroups.values()) {
                        serviceInfo.setOrReplaceDynamicAidGroup(group);
                    }
                }
            }

            if (toBeRemoved.size() > 0) {
                for (ComponentName component : toBeRemoved) {
                    Log.d(TAG, "Removing dynamic AIDs registered by " + component);
                    userServices.dynamicAids.remove(component);
                }
                // Persist to filesystem
                writeDynamicAidsLocked();
            }
            updateServiceStateFromFile(userId);
            writeServiceStateToFile(userId);
        }

        mCallback.onServicesUpdated(userId, Collections.unmodifiableList(validServices));
        dump(validServices);
    }

    private void readDynamicAidsLocked() {
        FileInputStream fis = null;
        try {
            if (!mDynamicAidsFile.getBaseFile().exists()) {
                Log.d(TAG, "Dynamic AIDs file does not exist.");
                return;
            }
            fis = mDynamicAidsFile.openRead();
            XmlPullParser parser = Xml.newPullParser();
            parser.setInput(fis, null);
            int eventType = parser.getEventType();
            while (eventType != XmlPullParser.START_TAG &&
                    eventType != XmlPullParser.END_DOCUMENT) {
                eventType = parser.next();
            }
            String tagName = parser.getName();
            if ("services".equals(tagName)) {
                boolean inService = false;
                ComponentName currentComponent = null;
                int currentUid = -1;
                ArrayList<AidGroup> currentGroups = new ArrayList<AidGroup>();
                while (eventType != XmlPullParser.END_DOCUMENT) {
                    tagName = parser.getName();
                    if (eventType == XmlPullParser.START_TAG) {
                        if ("service".equals(tagName) && parser.getDepth() == 2) {
                            String compString = parser.getAttributeValue(null, "component");
                            String uidString = parser.getAttributeValue(null, "uid");
                            if (compString == null || uidString == null) {
                                Log.e(TAG, "Invalid service attributes");
                            } else {
                                try {
                                    currentUid = Integer.parseInt(uidString);
                                    currentComponent = ComponentName.unflattenFromString(compString);
                                    inService = true;
                                } catch (NumberFormatException e) {
                                    Log.e(TAG, "Could not parse service uid");
                                }
                            }
                        }
                        if ("aid-group".equals(tagName) && parser.getDepth() == 3 && inService) {
                            AidGroup group = AidGroup.createFromXml(parser);
                            if (group != null) {
                                currentGroups.add(group);
                            } else {
                                Log.e(TAG, "Could not parse AID group.");
                            }
                        }
                    } else if (eventType == XmlPullParser.END_TAG) {
                        if ("service".equals(tagName)) {
                            // See if we have a valid service
                            if (currentComponent != null && currentUid >= 0 &&
                                    currentGroups.size() > 0) {
                                final int userId = UserHandle.getUserId(currentUid);
                                DynamicAids dynAids = new DynamicAids(currentUid);
                                for (AidGroup group : currentGroups) {
                                    dynAids.aidGroups.put(group.getCategory(), group);
                                }
                                UserServices services = findOrCreateUserLocked(userId);
                                services.dynamicAids.put(currentComponent, dynAids);
                            }
                            currentUid = -1;
                            currentComponent = null;
                            currentGroups.clear();
                            inService = false;
                        }
                    }
                    eventType = parser.next();
                };
            }
        } catch (Exception e) {
            Log.e(TAG, "Could not parse dynamic AIDs file, trashing.");
            mDynamicAidsFile.delete();
        } finally {
            if (fis != null) {
                try {
                    fis.close();
                } catch (IOException e) {
                }
            }
        }
    }

    private boolean writeDynamicAidsLocked() {
        FileOutputStream fos = null;
        try {
            fos = mDynamicAidsFile.startWrite();
            XmlSerializer out = new FastXmlSerializer();
            out.setOutput(fos, "utf-8");
            out.startDocument(null, true);
            out.setFeature(XML_INDENT_OUTPUT_FEATURE, true);
            out.startTag(null, "services");
            for (int i = 0; i < mUserServices.size(); i++) {
                final UserServices user = mUserServices.valueAt(i);
                for (Map.Entry<ComponentName, DynamicAids> service : user.dynamicAids.entrySet()) {
                    out.startTag(null, "service");
                    out.attribute(null, "component", service.getKey().flattenToString());
                    out.attribute(null, "uid", Integer.toString(service.getValue().uid));
                    for (AidGroup group : service.getValue().aidGroups.values()) {
                        group.writeAsXml(out);
                    }
                    out.endTag(null, "service");
                }
            }
            out.endTag(null, "services");
            out.endDocument();
            mDynamicAidsFile.finishWrite(fos);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "Error writing dynamic AIDs", e);
            if (fos != null) {
                mDynamicAidsFile.failWrite(fos);
            }
            return false;
        }
    }

    private void updateServiceStateFromFile(int currUserId)
    {
        FileInputStream fis = null;
        try {
             if(NfcService.getInstance().getAidRoutingTableStatus() == 0x00) {
                 Log.e(TAG, " Aid Routing Table still  availble , No need to disable services");
                 return;
             }
             if(!mServiceStateFile.getBaseFile().exists()) {
                 Log.d(TAG,"mServiceStateFile does not exist");
                 return;
             }
             fis = mServiceStateFile.openRead();
             XmlPullParser parser = Xml.newPullParser();
             parser.setInput(fis , null);
             int eventType = parser.getEventType();
             int currUid = -1;
             ComponentName currComponent = null;
             HashMap<ComponentName ,ApduServiceInfo> nxpOffHostServiceMap = mRegisteredNxpServicesCache.getApduservicesMaps();
             boolean state = true;
             while (eventType != XmlPullParser.START_TAG &&
                     eventType != XmlPullParser.END_DOCUMENT) {
                 eventType = parser.next();
             }
             String tagName = parser.getName();
             if ("services".equals(tagName)) {
                 while (eventType != XmlPullParser.END_DOCUMENT) {
                     tagName = parser.getName();
                     if (eventType == XmlPullParser.START_TAG) {
                         if("service".equals(tagName) && parser.getDepth() == 0x02) {
                             String compString  = parser.getAttributeValue(null ,"component");
                             String uidString   = parser.getAttributeValue(null ,"uid");
                             String stateString = parser.getAttributeValue(null ,"serviceState");
                             if(compString == null || uidString == null || stateString == null) {
                                 Log.e(TAG, "Invalid service attributes");
                             } else {
                                try {
                                    currUid       = Integer.parseInt(uidString);
                                    currComponent = ComponentName.unflattenFromString(compString);
                                    Log.d(TAG, " curr component "+compString);
                                    Log.d(TAG, " curr uid "+uidString);
                                    Log.d(TAG, " curr state "+stateString);
                                    if(stateString.equalsIgnoreCase("false")) { state = false;}
                                } catch (NumberFormatException e) {
                                    Log.e(TAG, "could not parse the service attributes");
                                }
                             }
                         }
                     } else  if (eventType == XmlPullParser.END_TAG) {
                         if("service".equals(tagName)) {
                             final int userId = UserHandle.getUserId(currUid);
                             if(currUserId == userId) {
                                 UserServices serviceCache = findOrCreateUserLocked(userId);
                                 ApduServiceInfo serviceInfo = serviceCache.services.get(currComponent);
                                 if(serviceInfo == null) {
                                 // CHECK for GSMA related services also.
                                     serviceInfo = nxpOffHostServiceMap.get(currComponent);
                                     if(serviceInfo == null) {
                                         Log.e(TAG, "could not find the required serviceInfo");
                                     } else serviceInfo.setServiceState(CardEmulation.CATEGORY_OTHER ,state);
                                 } else   serviceInfo.setServiceState(CardEmulation.CATEGORY_OTHER ,state);
                             }
                         }
                         currUid       = -1;
                         currComponent = null;
                         state         = true;
                     }

                     eventType = parser.next();
                 }
             }
        } catch(Exception e) {
            mServiceStateFile.delete();
            Log.e(TAG, "could not parse the seriveState file , thrashing the file");
        } finally {
            try {
                if(fis != null) {
                    fis.close();
                }
            } catch ( Exception e) {
            }
        }
    }

    private boolean writeServiceStateToFile(int currUserId) {
        FileOutputStream fos = null;
        ArrayList<ApduServiceInfo> nxpOffHostServiceCache = mRegisteredNxpServicesCache.getApduservicesList();
        if(NfcService.getInstance().getAidRoutingTableStatus() == 0x00) {
            Log.e(TAG, " Aid Routing Table still  availble , No need to disable services");
            return false;
        }
        if(currUserId != ActivityManager.getCurrentUser()) {
            return false;
        }
        try {
            fos = mServiceStateFile.startWrite();
            XmlSerializer out = new FastXmlSerializer();
            out.setOutput(fos, "utf-8");
            out.startDocument(null , true);
            out.setFeature(XML_INDENT_OUTPUT_FEATURE, true);
            out.startTag(null ,"services");
            for(int userId = 0; userId < mUserServices.size(); userId++) {
                final UserServices userServices = mUserServices.valueAt(userId);
                for (ApduServiceInfo serviceInfo : userServices.services.values()) {
                    if(!serviceInfo.hasCategory(CardEmulation.CATEGORY_OTHER)) {
                        continue;
                    }
                    out.startTag(null ,"service");
                    out.attribute(null, "component", serviceInfo.getComponent().flattenToString());
                    Log.d(TAG,"component name"+ serviceInfo.getComponent().flattenToString());
                    out.attribute(null, "uid", Integer.toString(serviceInfo.getUid()));
                    Log.d(TAG,"uid name"+ Integer.toString(serviceInfo.getUid()));
                    out.attribute(null, "serviceState", Boolean.toString(serviceInfo.getServiceState(CardEmulation.CATEGORY_OTHER)));
                    Log.d(TAG,"curr name"+ Boolean.toString(serviceInfo.getServiceState(CardEmulation.CATEGORY_OTHER)));
                    out.endTag(null, "service");
                }
            }
            dump(nxpOffHostServiceCache);
            //ADD GSMA services Cache
            for(ApduServiceInfo serviceInfo : nxpOffHostServiceCache) {
                out.startTag(null ,"service");
                out.attribute(null, "component", serviceInfo.getComponent().flattenToString());
                Log.d(TAG,"component name"+ serviceInfo.getComponent().flattenToString());
                out.attribute(null, "uid", Integer.toString(serviceInfo.getUid()));
                Log.d(TAG,"uid name"+ Integer.toString(serviceInfo.getUid()));
                out.attribute(null, "serviceState", Boolean.toString(serviceInfo.getServiceState(CardEmulation.CATEGORY_OTHER)));
                Log.d(TAG,"curr name"+ Boolean.toString(serviceInfo.getServiceState(CardEmulation.CATEGORY_OTHER)));
                out.endTag(null, "service");
            }
            out.endTag(null ,"services");
            out.endDocument();
            mServiceStateFile.finishWrite(fos);
            return true;
        } catch ( Exception e){
            Log.e(TAG,"Failed to write serviceStateFile xml");
            e.printStackTrace();
            if (fos != null) {
                mServiceStateFile.failWrite(fos);
            }
            return false;
        }
    }

    public int updateServiceState(int userId , int uid,
            Map<String , Boolean> serviceState) {
        boolean success = false;
        HashMap<ComponentName ,ApduServiceInfo> nxpOffHostServiceMap = mRegisteredNxpServicesCache.getApduservicesMaps();
        if(NfcService.getInstance().getAidRoutingTableStatus() == 0x00) {
            Log.e(TAG, " Aid Routing Table still  availble , No need to disable services");
            return 0xFF;
        }
        synchronized(mLock) {
            Iterator<Map.Entry<String , Boolean>> it =
                    serviceState.entrySet().iterator();
            while(it.hasNext()) {
                Map.Entry<String , Boolean> entry =
                        (Map.Entry<String , Boolean>) it.next();
                ComponentName componentName = ComponentName.unflattenFromString(entry.getKey());
                ApduServiceInfo serviceInfo = getService(userId, componentName);
                Log.e(TAG, "updateServiceState " + entry.getKey());
                Log.e(TAG, "updateServiceState  " + entry.getValue());
                if (serviceInfo != null) {
                    serviceInfo.setServiceState(CardEmulation.CATEGORY_OTHER, entry.getValue());
                } else if ((serviceInfo = nxpOffHostServiceMap.get(componentName)) != null) {
                      // CHECK for GSMA cache
                      serviceInfo.setServiceState(CardEmulation.CATEGORY_OTHER, entry.getValue());
                } else {
                      Log.e(TAG, "Could not find service " + componentName);
                      return 0xFF;
                }
            }
            success = writeServiceStateToFile(userId);
        }
        invalidateCache(ActivityManager.getCurrentUser());
        return (success?0x00:0xFF);
    }

    public boolean registerAidGroupForService(int userId, int uid,
            ComponentName componentName, AidGroup aidGroup) {
        ArrayList<ApduServiceInfo> newServices = null;
        boolean success;
        synchronized (mLock) {
            UserServices services = findOrCreateUserLocked(userId);
            // Check if we can find this service
            ApduServiceInfo serviceInfo = getService(userId, componentName);
            if (serviceInfo == null) {
                Log.e(TAG, "Service " + componentName + " does not exist.");
                return false;
            }
            if (serviceInfo.getUid() != uid) {
                // This is probably a good indication something is wrong here.
                // Either newer service installed with different uid (but then
                // we should have known about it), or somebody calling us from
                // a different uid.
                Log.e(TAG, "UID mismatch.");
                return false;
            }
            // Do another AID validation, since a caller could have thrown in a modified
            // AidGroup object with invalid AIDs over Binder.
            List<String> aids = aidGroup.getAids();
            for (String aid : aids) {
                if (!CardEmulation.isValidAid(aid)) {
                    Log.e(TAG, "AID " + aid + " is not a valid AID");
                    return false;
                }
            }
            serviceInfo.setOrReplaceDynamicAidGroup(aidGroup);
            DynamicAids dynAids = services.dynamicAids.get(componentName);
            if (dynAids == null) {
                dynAids = new DynamicAids(uid);
                services.dynamicAids.put(componentName, dynAids);
            }
            dynAids.aidGroups.put(aidGroup.getCategory(), aidGroup);
            success = writeDynamicAidsLocked();
            if (success) {
                newServices = new ArrayList<ApduServiceInfo>(services.services.values());
            } else {
                Log.e(TAG, "Failed to persist AID group.");
                // Undo registration
                dynAids.aidGroups.remove(aidGroup.getCategory());
            }
        }
        if (success) {
            // Make callback without the lock held
            mCallback.onServicesUpdated(userId, newServices);
        }
        return success;
    }

    public AidGroup getAidGroupForService(int userId, int uid, ComponentName componentName,
            String category) {
        ApduServiceInfo serviceInfo = getService(userId, componentName);
        if (serviceInfo != null) {
            if (serviceInfo.getUid() != uid) {
                Log.e(TAG, "UID mismatch");
                return null;
            }
            return serviceInfo.getDynamicAidGroupForCategory(category);
        } else {
            Log.e(TAG, "Could not find service " + componentName);
            return null;
        }
    }

    public boolean removeAidGroupForService(int userId, int uid, ComponentName componentName,
            String category) {
        boolean success = false;
        ArrayList<ApduServiceInfo> newServices = null;
        synchronized (mLock) {
            UserServices services = findOrCreateUserLocked(userId);
            ApduServiceInfo serviceInfo = getService(userId, componentName);
            if (serviceInfo != null) {
                if (serviceInfo.getUid() != uid) {
                    // Calling from different uid
                    Log.e(TAG, "UID mismatch");
                    return false;
                }
                if (!serviceInfo.removeDynamicAidGroupForCategory(category)) {
                    Log.e(TAG," Could not find dynamic AIDs for category " + category);
                    return false;
                }
                // Remove from local cache
                DynamicAids dynAids = services.dynamicAids.get(componentName);
                if (dynAids != null) {
                    AidGroup deletedGroup = dynAids.aidGroups.remove(category);
                    success = writeDynamicAidsLocked();
                    if (success) {
                        newServices = new ArrayList<ApduServiceInfo>(services.services.values());
                    } else {
                        Log.e(TAG, "Could not persist deleted AID group.");
                        dynAids.aidGroups.put(category, deletedGroup);
                        return false;
                    }
                } else {
                    Log.e(TAG, "Could not find aid group in local cache.");
                }
            } else {
                Log.e(TAG, "Service " + componentName + " does not exist.");
            }
        }
        if (success) {
            mCallback.onServicesUpdated(userId, newServices);
        }
        return success;
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("Registered HCE services for current user: ");
        UserServices userServices = findOrCreateUserLocked(ActivityManager.getCurrentUser());
        for (ApduServiceInfo service : userServices.services.values()) {
            service.dump(fd, pw, args);
            pw.println("");
        }
        pw.println("");
    }

}
