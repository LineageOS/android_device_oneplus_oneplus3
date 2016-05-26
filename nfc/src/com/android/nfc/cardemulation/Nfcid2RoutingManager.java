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
import com.android.nfc.cardemulation.Nfcid2RoutingCache;

import android.nfc.cardemulation.ApduServiceInfo;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.Iterator;

public class Nfcid2RoutingManager {
    static final String TAG = "Nfcid2RoutingManager";

    static final boolean DBG = true;

    // This is the default IsoDep protocol route; it means
    // that for any AID that needs to be routed to this
    // destination, we won't need to add a rule to the routing
    // table, because this destination is already the default route.
    //
    // For Nexus devices, the default route is always 0x00.
    static final int DEFAULT_ROUTE = 0x00;

    // For Nexus devices, just a static route to the eSE
    // OEMs/Carriers could manually map off-host AIDs
    // to the correct eSE/UICC based on state they keep.
//    static final int DEFAULT_OFFHOST_ROUTE = ApduServiceInfo.SECURE_ELEMENT_ROUTE_UICC;

    final Object mLock = new Object();

    // mNfcid2RoutingTable contains the current routing table. The index is the route ID.
//    final SparseArray<Set<String>> mNfcid2RoutingTable = new SparseArray<Set<String>>();

    // Easy look-up what the route is for a certain AID
    final Map<String,Boolean> mRouteForNfcid2 = new HashMap<String,Boolean>();

    // Whether the routing table is dirty
    boolean mDirty;

    //default secure element id if not specified
//    int mDefaultSeId;

    // Store final sorted outing table
    final Nfcid2RoutingCache mRoutnigCache;

    public Nfcid2RoutingManager() {
//        mDefaultSeId = -1;
        mRoutnigCache = new Nfcid2RoutingCache();
    }

    public Set<String> getRoutedNfcid2s() {
        Set<String> routedNfcid2s = new HashSet<String>();
        synchronized (mLock) {
            for (Map.Entry<String, Boolean> nfcid2Entry : mRouteForNfcid2.entrySet()) {
                routedNfcid2s.add(nfcid2Entry.getKey());
            }
        }
        return routedNfcid2s;
    }


    /**
     * This notifies that the AID routing table in the controller
     * has been cleared (usually due to NFC being turned off).
     */
    public void onNfccRoutingTableCleared() {
        // The routing table in the controller was cleared
        // To stay in sync, clear our own tables.
        synchronized (mLock) {
//            mNfcid2RoutingTable.clear();
            mRouteForNfcid2.clear();
            mRoutnigCache.clear();
        }
    }

    public void commitRouting() {
        synchronized (mLock) {
            if (mDirty) {

                if (DBG) Log.d(TAG, "commitRouting-----");
                //NfcService.getInstance().commitingFelicaRouting();

                mRoutnigCache.commit();
                //No Need for this here.
                //NfcService.getInstance().commitedFelicaRouting();
                mDirty = false;
            } else {
                if (DBG) Log.d(TAG, "Not committing routing because table not dirty.");
            }
        }
    }

    int getConflictForNfcid2Locked(String nfcid2) {
        Boolean conflict = mRouteForNfcid2.get(nfcid2);
        return conflict == null ? -1 : (conflict ? 0:1);
    }

    public boolean setRouteForNfcid2(String nfcid2, String syscode, String optparam, boolean isResolvingConflict) {

        synchronized (mLock) {
            mRouteForNfcid2.put(nfcid2,true);

            /* For conflicting NFCID2 as well, we need to set the LF_T3T_IDENTIFIER_X,
             * as otherwise This NFCID2 will never be activated.*/
//            if (!isConflict) {
                if (DBG) Log.d(TAG, "setRouteForNfcid2(): nfcid2:" + nfcid2 + ", syscode=" + syscode);
                mRoutnigCache.addNfcid2(nfcid2, syscode, optparam, false, isResolvingConflict);
                mDirty = true;
//            }
        }
        return true;
}

    public boolean removeNfcid2(String nfcid2) {

        if (DBG) Log.d(TAG, "removeNfcid2(String nfcid2): nfcid2:" + nfcid2);
        synchronized (mLock) {
            int currentConflict = getConflictForNfcid2Locked(nfcid2);
            if (currentConflict == -1) {
               if (DBG) Log.d(TAG, "removeNfcid2(): No existing route for " + nfcid2);
               return false;
            }
            mRouteForNfcid2.remove(nfcid2);
            if (DBG) Log.d(TAG, "removeNfcid2(): nfcid2:" + nfcid2 + ", currentConflict="+currentConflict);
            if (mRoutnigCache.removeNfcid2(nfcid2/*,false*/)) {
                mDirty = true;
            }
        }
        return true;
    }

}
