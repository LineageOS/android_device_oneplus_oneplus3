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
package com.android.nfc.cardemulation;

import android.util.Log;
import android.util.SparseArray;

import com.android.nfc.NfcService;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.ArrayList;
public class AidRoutingManager {
    static final String TAG = "AidRoutingManager";

    static final boolean DBG = true;

    static final int ROUTE_HOST = 0x00;

    // Every routing table entry is matched exact
    static final int AID_MATCHING_EXACT_ONLY = 0x00;
    // Every routing table entry can be matched either exact or prefix
    static final int AID_MATCHING_EXACT_OR_PREFIX = 0x01;
    // Every routing table entry is matched as a prefix
    static final int AID_MATCHING_PREFIX_ONLY = 0x02;
    //Behavior as per Android-L, supporting prefix match and full
    //match for both OnHost and OffHost apps.
    static final int AID_MATCHING_L = 0x01;
    //Behavior as per Android-KitKat by NXP, supporting prefix match for
    //OffHost and prefix and full both for OnHost apps.
    static final int AID_MATCHING_K = 0x02;
    // This is the default IsoDep protocol route; it means
    // that for any AID that needs to be routed to this
    // destination, we won't need to add a rule to the routing
    // table, because this destination is already the default route.
    //
    // For Nexus devices, the default route is always 0x00.
    int mDefaultRoute;

    // For Nexus devices, just a static route to the eSE
    // OEMs/Carriers could manually map off-host AIDs
    // to the correct eSE/UICC based on state they keep.
    final int mDefaultOffHostRoute;

    // How the NFC controller can match AIDs in the routing table;
    // see AID_MATCHING constants
    final int mAidMatchingSupport;
    final int mAidMatchingPlatform;
    //Changed from final to private int to update RoutingtableSize later in configureRouting.
    private int mAidRoutingTableSize;
    // Maximum AID routing table size
    final Object mLock = new Object();

    // mAidRoutingTable contains the current routing table. The index is the route ID.
    // The route can include routes to a eSE/UICC.
    SparseArray<Set<String>> mAidRoutingTable =
            new SparseArray<Set<String>>();

    // Easy look-up what the route is for a certain AID
    HashMap<String, Integer> mRouteForAid = new HashMap<String, Integer>();

    // Easy look-up what the power state is for a certain AID
    HashMap<String, Integer> mPowerForAid = new HashMap<String, Integer>();


    private native int doGetDefaultRouteDestination();
    private native int doGetDefaultOffHostRouteDestination();
    private native int doGetAidMatchingMode();
    private native int doGetAidMatchingPlatform();

    final VzwRoutingCache mVzwRoutingCache;

    public AidRoutingManager() {
        mDefaultRoute = doGetDefaultRouteDestination();
        if (DBG) Log.d(TAG, "mDefaultRoute=0x" + Integer.toHexString(mDefaultRoute));
        mDefaultOffHostRoute = doGetDefaultOffHostRouteDestination();
        if (DBG) Log.d(TAG, "mDefaultOffHostRoute=0x" + Integer.toHexString(mDefaultOffHostRoute));
        mAidMatchingSupport = doGetAidMatchingMode();
        if (DBG) Log.d(TAG, "mAidMatchingSupport=0x" + Integer.toHexString(mAidMatchingSupport));
        mAidMatchingPlatform = doGetAidMatchingPlatform();
        if (DBG) Log.d(TAG, "mAidTableSize=0x" + Integer.toHexString(mAidRoutingTableSize));
        mVzwRoutingCache = new VzwRoutingCache();
    }

    public boolean supportsAidPrefixRouting() {
        return mAidMatchingSupport == AID_MATCHING_EXACT_OR_PREFIX ||
                mAidMatchingSupport == AID_MATCHING_PREFIX_ONLY;
    }
    void clearNfcRoutingTableLocked() {
        NfcService.getInstance().clearRouting();
        mRouteForAid.clear();
        mAidRoutingTable.clear();
    }

    public boolean configureRouting(HashMap<String, AidElement> aidMap) {
        mDefaultRoute = NfcService.getInstance().GetDefaultRouteLoc();
        boolean aidRouteResolved = false;
        if (DBG) Log.d(TAG, "mAidMatchingPlatform=0x" + Integer.toHexString(mAidMatchingPlatform));
        mAidRoutingTableSize = NfcService.getInstance().getAidRoutingTableSize();
        if (DBG) Log.d(TAG, "mDefaultRoute=0x" + Integer.toHexString(mDefaultRoute));
        Hashtable<String, AidElement> routeCache = new Hashtable<String, AidElement>(50);
        SparseArray<Set<String>> aidRoutingTable = new SparseArray<Set<String>>(aidMap.size());
        HashMap<String, Integer> routeForAid = new HashMap<String, Integer>(aidMap.size());
        HashMap<String, Integer> powerForAid = new HashMap<String, Integer>(aidMap.size());
        // Then, populate internal data structures first
        DefaultAidRouteResolveCache defaultRouteCache = new DefaultAidRouteResolveCache();

        for (Map.Entry<String, AidElement> aidEntry : aidMap.entrySet())  {
            AidElement elem = aidEntry.getValue();
            int route = elem.getRouteLocation();
            int power = elem.getPowerState();
            if (route == -1 ) {
                route = mDefaultOffHostRoute;
                elem.setRouteLocation(route);
            }
            String aid = aidEntry.getKey();
            Set<String> entries = aidRoutingTable.get(route, new HashSet<String>());
            entries.add(aid);
            aidRoutingTable.put(route, entries);
            routeForAid.put(aid, route);
            powerForAid.put(aid, power);
            if (DBG) Log.d(TAG, "#######Routing AID " + aid + " to route "
                        + Integer.toString(route) + " with power "+ power);
        }

        synchronized (mLock) {
            if (routeForAid.equals(mRouteForAid)) {
                if (DBG) Log.d(TAG, "Routing table unchanged, not updating");
                return false;
            }

            // Otherwise, update internal structures and commit new routing
            clearNfcRoutingTableLocked();
            mRouteForAid = routeForAid;
            mPowerForAid = powerForAid;
            mAidRoutingTable = aidRoutingTable;
            for(int routeIndex=0; routeIndex < 0x03; routeIndex++) {
                routeCache.clear();
                if (mAidMatchingSupport == AID_MATCHING_PREFIX_ONLY ||
                    mAidMatchingPlatform == AID_MATCHING_K) {
                /* If a non-default route registers an exact AID which is shorter
                 * than this exact AID, this will create a problem with controllers
                 * that treat every AID in the routing table as a prefix.
                 * For example, if App A registers F0000000041010 as an exact AID,
                 * and App B registers F000000004 as an exact AID, and App B is not
                 * the default route, the following would be added to the routing table:
                 * F000000004 -> non-default destination
                 * However, because in this mode, the controller treats every routing table
                 * entry as a prefix, it means F0000000041010 would suddenly go to the non-default
                 * destination too, whereas it should have gone to the default.
                 *
                 * The only way to prevent this is to add the longer AIDs of the
                 * default route at the top of the table, so they will be matched first.
                 */
                Set<String> defaultRouteAids = mAidRoutingTable.get(mDefaultRoute);
               if (defaultRouteAids != null) {
                    for (String defaultRouteAid : defaultRouteAids) {
                        // Check whether there are any shorted AIDs routed to non-default
                        // TODO this is O(N^2) run-time complexity...
                        for (Map.Entry<String, Integer> aidEntry : mRouteForAid.entrySet()) {
                            String aid = aidEntry.getKey();
                            int route = aidEntry.getValue();
                            if (defaultRouteAid.startsWith(aid) && route != mDefaultRoute) {
                                if (DBG) Log.d(TAG, "Adding AID " + defaultRouteAid + " for default " +
                                        "route, because a conflicting shorter AID will be added" +
                                        "to the routing table");
                                AidElement elem = aidMap.get(defaultRouteAid);
                                elem.setRouteLocation(mDefaultRoute);
                                routeCache.put(defaultRouteAid, elem);
//                                NfcService.getInstance().routeAids(defaultRouteAid, mDefaultRoute, mPowerForAid.get(defaultRouteAid));
                            }
                        }
                    }
                }
            }

            // Add AID entries for all non-default routes
            for (int i = 0; i < mAidRoutingTable.size(); i++) {
                int route = mAidRoutingTable.keyAt(i);
                if (route != mDefaultRoute) {
                    Set<String> aidsForRoute = mAidRoutingTable.get(route);
                    for (String aid : aidsForRoute) {
                        if (aid.endsWith("*")) {
                            if (mAidMatchingSupport == AID_MATCHING_EXACT_ONLY) {
                                Log.e(TAG, "This device does not support prefix AIDs.");
                            } else if (mAidMatchingSupport == AID_MATCHING_PREFIX_ONLY) {
                                if (DBG) Log.d(TAG, "Routing prefix AID " + aid + " to route "
                                        + Integer.toString(route));
                                // Cut off '*' since controller anyway treats all AIDs as a prefix
                                AidElement elem = aidMap.get(aid);
                                elem.setAid(aid.substring(0,aid.length() - 1));
                                routeCache.put(aid, elem);
//                                NfcService.getInstance().routeAids(aid.substring(0,
//                                                aid.length() - 1), route, mPowerForAid.get(aid));
                            } else if (mAidMatchingSupport == AID_MATCHING_EXACT_OR_PREFIX) {
                                Log.d(TAG, "Routing AID in AID_MATCHING_EXACT_OR_PREFIX");
                                if (DBG) Log.d(TAG, "Routing prefix AID " + aid + " to route "
                                        + Integer.toString(route));
                                AidElement elem = aidMap.get(aid);
                                routeCache.put(aid, elem);
//                                NfcService.getInstance().routeAids(aid, route, (mPowerForAid.get(aid)));
                            }
                        } else {
                            if (DBG) Log.d(TAG, "Routing exact AID " + aid + " to route "
                                    + Integer.toString(route));
                            AidElement elem = aidMap.get(aid);
                            routeCache.put(aid, elem);
//                            NfcService.getInstance().routeAids(aid, route, mPowerForAid.get(aid));
                        }
                    }
                }
            }

            if((routeIndex == 0x00) && (defaultRouteCache.calculateAidRouteSize(routeCache) <= mAidRoutingTableSize)) {
                // maximum aid table size is less than  AID route table size
                aidRouteResolved = true;
                break;
            } else {
                defaultRouteCache.updateDefaultAidRouteCache(routeCache , mDefaultRoute);
                mDefaultRoute = defaultRouteCache.getNextRouteLoc();
                if(mDefaultRoute == 0xFF)
                {
                    break;
                }
            }

        }
        }
        if(aidRouteResolved == false) {
            if(mAidRoutingTable.size() == 0x00) {
                aidRouteResolved = true;
                //update the cache , no applications present.
            }
            else if(defaultRouteCache.resolveDefaultAidRoute() == true) {
                aidRouteResolved = true;
                routeCache = defaultRouteCache.getResolvedAidRouteCache();
                //update preferences if different
                NfcService.getInstance().setDefaultAidRouteLoc(defaultRouteCache.getResolvedAidRoute());
            } else {
                NfcService.getInstance().notifyRoutingTableFull();
            }
        }
        // And finally commit the routing
        if(aidRouteResolved == true) {
            commit(routeCache);
        }
//        NfcService.getInstance().commitRouting();

        return true;
    }

    private void commit(Hashtable<String, AidElement> routeCache ) {

        if(routeCache != null) {
            List<AidElement> list = Collections.list(routeCache.elements());
            Collections.sort(list);
            Iterator<AidElement> it = list.iterator();
            NfcService.getInstance().clearRouting();
            while(it.hasNext()){
                AidElement element =it.next();
                if (DBG) Log.d (TAG, element.toString());
                NfcService.getInstance().routeAids(
                    element.getAid(),
                    element.getRouteLocation(),
                    element.getPowerState()
                    );
            }
        }
        // And finally commit the routing
        NfcService.getInstance().commitRouting();
    }

    /**
     * This notifies that the AID routing table in the controller
     * has been cleared (usually due to NFC being turned off).
     */
    public void onNfccRoutingTableCleared() {
        // The routing table in the controller was cleared
        // To stay in sync, clear our own tables.
        synchronized (mLock) {
            mAidRoutingTable.clear();
            mRouteForAid.clear();
        }
    }

    public boolean UpdateVzwCache(byte[] aid,int route,int power,boolean isAllowed){
       mVzwRoutingCache.addAid(aid,route,power,isAllowed);
       return true;
    }

    public VzwRoutingCache GetVzwCache(){
       return mVzwRoutingCache;
    }

    public void ClearVzwCache() {
        mVzwRoutingCache.clear();
    }

    public int getAidMatchMode() {
        return mAidMatchingSupport;
    }

    public int getAidMatchingPlatform() {
        return mAidMatchingPlatform;
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("Routing table:");
        pw.println("    Default route: " + ((mDefaultRoute == 0x00) ? "host" : "secure element"));
        synchronized (mLock) {
            for (int i = 0; i < mAidRoutingTable.size(); i++) {
                Set<String> aids = mAidRoutingTable.valueAt(i);
                pw.println("    Routed to 0x" + Integer.toHexString(mAidRoutingTable.keyAt(i)) + ":");
                for (String aid : aids) {
                    pw.println("        \"" + aid + "\"");
                }
            }
        }
    }
    final class DefaultAidRouteResolveCache {
        static final int AID_HDR_LENGTH = 0x04; // TAG + ROUTE + LENGTH_BYTE + POWER
        static final int MAX_AID_ENTRIES = 50;
        //AidCacheTable contains the current aid routing table for particular route.
        //The index is the route ID.
        private SparseArray<Hashtable<String, AidElement>> aidCacheTable;
        private HashMap <Integer , Integer> aidCacheSize;
        //HashMap containing aid routing size .
        //The index is route ID.
        private Hashtable<String, AidElement> resolvedAidCache;//HashTable containing resolved default aid routeCache
        private int mCurrDefaultAidRoute; // current default route in preferences
        private int mResolvedAidRoute;    //resolved aid route location
        private boolean mAidRouteResolvedStatus; // boolean value
        private ArrayList<Integer> aidRoutes;//non-default route location

        DefaultAidRouteResolveCache () {
            aidCacheTable = new SparseArray<Hashtable<String, AidElement>>(0x03);
            resolvedAidCache = new Hashtable<String, AidElement>();
            aidCacheSize= new HashMap <Integer , Integer>(0x03);
            aidRoutes = new ArrayList<Integer>();
            mCurrDefaultAidRoute = NfcService.getInstance().GetDefaultRouteLoc();
            mAidRouteResolvedStatus = false;
        }

        private Hashtable<String, AidElement> extractResolvedCache()
        {
            if(mAidRouteResolvedStatus == false) {
                return null;
            }
            for(int i=0;i< aidCacheTable.size() ;i++) {
                int route = aidCacheTable.keyAt(i);
                if(route == mResolvedAidRoute) {
                    return aidCacheTable.get(route);
                }
            }
            return null;
        }


        public int calculateAidRouteSize(Hashtable<String, AidElement> routeCache) {
            int routeTableSize = 0x00;
            int routeAidCount = 0x00;
            for(Map.Entry<String, AidElement> aidEntry : routeCache.entrySet()) {
                String aid = aidEntry.getKey();
                if(aid.endsWith("*")) {
                    routeTableSize += ((aid.length() - 0x01) / 0x02) + AID_HDR_LENGTH; // removing prefix length
                } else {
                    routeTableSize += (aid.length() / 0x02)+ AID_HDR_LENGTH;
                }
                routeAidCount++;
            }
            Log.d(TAG, "calculateAidRouteSize final Routing table size" +routeTableSize);
            if(routeTableSize <= mAidRoutingTableSize && routeAidCount > MAX_AID_ENTRIES) {
                routeTableSize = mAidRoutingTableSize + 0x01;
            }
                return routeTableSize;
        }

       public int updateDefaultAidRouteCache(Hashtable<String, AidElement> routeCache , int route) {
           int routesize = 0x00;
           Hashtable<String, AidElement> tempRouteCache = new Hashtable<String, AidElement>(0x50);
           tempRouteCache.putAll(routeCache);
           routesize = calculateAidRouteSize(tempRouteCache);
           aidCacheTable.put(route, tempRouteCache);
           aidCacheSize.put(route, routesize);
           Log.d(TAG, "updateDefaultAidRouteCache Routing table size" +routesize);
           return routesize;
       }

       public boolean resolveDefaultAidRoute () {

           int minRoute = 0xFF;
           int minAidRouteSize = mAidRoutingTableSize;
           int tempRoute = 0x00;
           int tempCacheSize = 0x00;
           Hashtable<String, AidElement> aidRouteCache = new Hashtable<String, AidElement>();
           Set<Integer> keys = aidCacheSize.keySet();
           for (Integer key : keys) {
               tempRoute = key;
               tempCacheSize = aidCacheSize.get(key);
               if (tempCacheSize <= minAidRouteSize) {
                   minAidRouteSize = tempCacheSize;
                   minRoute = tempRoute;
               }
           }
           if(minRoute != 0xFF) {
               mAidRouteResolvedStatus = true;
               mResolvedAidRoute = minRoute;
               Log.d(TAG, "min route found "+mResolvedAidRoute);
           }

           return mAidRouteResolvedStatus;
       }

       public int getResolvedAidRoute () {
           return mResolvedAidRoute;
       }

       public Hashtable<String, AidElement>  getResolvedAidRouteCache() {

           return extractResolvedCache();
       }

       public int getNextRouteLoc() {

           for (int i = 0; i < 0x03; i++) {
               if((i != mCurrDefaultAidRoute) && (!aidRoutes.contains(i)))
               {
                   aidRoutes.add(i);
                   return i;
               }
           }
           return 0xFF;
       }

    }
}
