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

import java.util.Collections;
import java.util.Hashtable;
import java.util.List;
import java.util.Iterator;
import com.android.nfc.NfcService;

public class Nfcid2RoutingCache {
    private static final boolean DBG = true;
    private static final String TAG = "Nfcid2RoutingCache";

    private static final int CAPACITY = 16;

    public static final String EXTRA_NFCID2 = "extra_nfcid2";
    public static final String EXTRA_SYSCODE = "extra_syscode";
    public static final String EXTRA_OPTPARAM = "extra_optparam";

    // Cached routing table
    private static final Hashtable<String, Nfcid2Element> mRouteCache = new Hashtable<String, Nfcid2Element>(CAPACITY);

    Nfcid2RoutingCache() {
    }

    boolean addNfcid2(String nfcid2, String syscode, String optparam, boolean isDefault, boolean isConflicting) {
        Nfcid2Element elem = new Nfcid2Element(nfcid2, syscode, optparam, isDefault, isConflicting);
        if (mRouteCache.size() >= CAPACITY) {
            return false;
        }

        mRouteCache.put(nfcid2.toUpperCase(), elem);
        return true;
    }

    boolean removeNfcid2(String nfcid2/*, boolean force*/) {
//        if(force) {
//            return (mRouteCache.remove(nfcid2) != null);
//        } else  {
            Nfcid2Element element = mRouteCache.get(nfcid2);
            if(element != null) element.setDirty();
            return element != null;
//        }
    }

    boolean isDefault(String aid) {
        Nfcid2Element elem = (Nfcid2Element)mRouteCache.get(aid);

        return elem!= null && elem.isDefault();
    }

    void clear() {
        mRouteCache.clear();
    }

    void commit() {
        List<Nfcid2Element> list = Collections.list(mRouteCache.elements());
        Collections.sort(list);
        Iterator<Nfcid2Element> it = list.iterator();

//        NfcService.getInstance().clearRouting();

        while(it.hasNext()){
            Nfcid2Element element =it.next();
            if (DBG) Log.d (TAG, element.toString());

            if(element.isConflicting())
            {
                NfcService.getInstance().unrouteNfcid2(element.getNfcid2());
                element.resolveConflict();
            }

            if(!element.getDirty())
            {
                NfcService.getInstance().routeNfcid2(
                        element.getNfcid2(),
                        element.getSyscode(),
                        element.getOptparam()
                        );
            }
            else
            {
                NfcService.getInstance().unrouteNfcid2(element.getNfcid2());
                mRouteCache.remove(element.getNfcid2());
            }
        }
    }
}

class Nfcid2Element implements Comparable {

    private String mNfcid2;
    private String mSyscode;
    private String mOptparam;
    private boolean mIsDefault;
    private boolean mIsDirty;
    private boolean mIsConflicting;

    public Nfcid2Element(String nfcid2, String syscode, String optparam, boolean isDefault, boolean isConflicting) {
        mNfcid2 = nfcid2;
        mSyscode = syscode;
        mOptparam = optparam;
        mIsDefault = isDefault;
        mIsDirty = false;
        mIsConflicting = isConflicting;
    }

    public void setDirty() {
        mIsDirty = true;
    }

    public void setConflicting() {
        mIsConflicting = true;
    }

    public void resolveConflict() {
        mIsConflicting = false;
    }

    public boolean isConflicting() {
        return mIsConflicting;
    }

    public boolean getDirty() {
        return mIsDirty;
    }

    public boolean isDefault() {
        return mIsDefault;
    }


    public String getNfcid2() {
        return mNfcid2;
    }

    public String getSyscode() {
        return mSyscode;
    }

    public String getOptparam() {
        return mOptparam;
    }

    public boolean isIsDefault() {
        return mIsDefault;
    }

    @Override
    public int compareTo(Object o) {

        Nfcid2Element elem = (Nfcid2Element)o;

        if (mIsDefault && !elem.isDefault()) {
            return -1;
        }
        else if (!mIsDefault && elem.isDefault()) {
            return 1;
        }
        return (elem.getNfcid2().compareToIgnoreCase(mNfcid2));
    }

    @Override
    public String toString() {
        return "aid: " + mNfcid2 + ", syscode: " + mSyscode
                    + ", optparam: " + mOptparam + ",isDefault: " + mIsDefault;
    }
}
