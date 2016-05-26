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

import android.util.Log;

import java.util.Collections;
import java.util.Hashtable;
import java.util.List;
import java.util.Iterator;
import com.android.nfc.NfcService;
import java.util.Set;
import java.util.HashSet;
import java.util.Map;
import com.vzw.nfc.RouteEntry;
import java.util.HashMap;

public class VzwRoutingCache {
    private static final boolean DBG = false;
    private static final String TAG = "VzwRoutingCache";

    // Cached routing table
    // final private static Hashtable<String, RouteEntry> mVzwCache = new
    // Hashtable<String, RouteEntry>(CAPACITY);
    final HashMap<String, RouteEntry> mVzwCache = new HashMap<String, RouteEntry>();

    VzwRoutingCache() {
    }

    boolean addAid(byte[] aid, int route, int power, boolean isAllowed) {
        RouteEntry elem = new RouteEntry(aid, power, route, isAllowed);
        Log.d(TAG, "aid"
                + toHexString(elem.getAid(), 0, elem.getAid().length)
                .toUpperCase());
        Log.d(TAG, "power " + power);
        Log.d(TAG, "power state" + elem.getPowerState());
        Log.d(TAG, "is allowed" + elem.isAllowed());
        mVzwCache.put(toHexString(elem.getAid(), 0, elem.getAid().length)
                .toUpperCase(), elem);
        return true;
    }

    boolean IsAidAllowed(String aid) {
        RouteEntry elem = (RouteEntry) mVzwCache.get(aid);
        return elem.isAllowed();
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

    int getPowerState(String aid) {
        RouteEntry elem = (RouteEntry) mVzwCache.get(aid);
        return elem.getPowerState();

    }

    boolean isAidPresent(String aid) {
        return (mVzwCache.containsKey(aid));
    }

    void clear() {
        mVzwCache.clear();
    }
}
