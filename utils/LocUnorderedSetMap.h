/* Copyright (c) 2015, 2017 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef __LOC_UNORDERDED_SETMAP_H__
#define __LOC_UNORDERDED_SETMAP_H__

#include <algorithm>
#include <unordered_set>
#include <unordered_map>

using std::unordered_set;
using std::unordered_map;

namespace loc_util {

// Trim from *fromSet* any elements that also exist in *rVals*.
// The optional *goneVals*, if not null, will be populated with removed elements.
template <typename T>
inline static void trimSet(unordered_set<T>& fromSet, const unordered_set<T>& rVals,
                           unordered_set<T>* goneVals) {
    for (auto val : rVals) {
        if (fromSet.erase(val) > 0 && nullptr != goneVals) {
            goneVals->insert(val);
        }
    }
}

// this method is destructive to the input unordered_sets.
// the return set is the interset extracted out from the two input sets, *s1* and *s2*.
// *s1* and *s2* will be left with the intersect removed from them.
template <typename T>
static unordered_set<T> removeAndReturnInterset(unordered_set<T>& s1, unordered_set<T>& s2) {
    unordered_set<T> common(0);
    for (auto b = s2.begin(); b != s2.end(); b++) {
        auto a = find(s1.begin(), s1.end(), *b);
        if (a != s1.end()) {
            // this is a common item of both l1 and l2, remove from both
            // but after we add to common
            common.insert(*a);
            s1.erase(a);
            s2.erase(b);
        }
    }
    return common;
}

template <typename KEY, typename VAL>
class LocUnorderedSetMap {
    unordered_map<KEY, unordered_set<VAL>> mMap;


    // Trim the VALs pointed to by *iter*, with everything that also exist in *rVals*.
    // If the set becomes empty, remove the map entry. *goneVals*, if not null, records
    // the trimmed VALs.
    bool trimOrRemove(typename unordered_map<KEY, unordered_set<VAL>>::iterator iter,
                      const unordered_set<VAL>& rVals, unordered_set<VAL>* goneVals) {
        trimSet<VAL>(iter->second, rVals, goneVals);
        bool removeEntry = (iter->second.empty());
        if (removeEntry) {
            mMap.erase(iter);
        }
        return removeEntry;
    }

public:
    inline LocUnorderedSetMap() {}
    inline LocUnorderedSetMap(size_t size) : mMap(size) {}

    inline bool empty() { return mMap.empty(); }

    // This gets the raw pointer to the VALs pointed to by *key*
    // If the entry is not in the map, nullptr will be returned.
    inline unordered_set<VAL>* getValSetPtr(const KEY& key) {
        auto entry = mMap.find(key);
        return (entry != mMap.end()) ? &(entry->second) : nullptr;
    }

    //  This gets a copy of VALs pointed to by *key*
    // If the entry is not in the map, an empty set will be returned.
    inline unordered_set<VAL> getValSet(const KEY& key) {
        auto entry = mMap.find(key);
        return (entry != mMap.end()) ? entry->second : unordered_set<VAL>(0);
    }

    // This gets all the KEYs from the map
    inline unordered_set<KEY> getKeys() {
        unordered_set<KEY> keys(0);
        for (auto entry : mMap) {
            keys.insert(entry.first);
        }
        return keys;
    }

    inline bool remove(const KEY& key) {
        return mMap.erase(key) > 0;
    }

    // This looks into all the entries keyed by *keys*. Remove any VALs from the entries
    // that also exist in *rVals*. If the entry is left with an empty set, the entry will
    // be removed. The optional parameters *goneKeys* and *goneVals* will record the KEYs
    // (or entries) and the collapsed VALs removed from the map, respectively.
    inline void trimOrRemove(unordered_set<KEY>&& keys, const unordered_set<VAL>& rVals,
                             unordered_set<KEY>* goneKeys, unordered_set<VAL>* goneVals) {
        trimOrRemove(keys, rVals, goneKeys, goneVals);
    }
    inline void trimOrRemove(unordered_set<KEY>& keys, const unordered_set<VAL>& rVals,
                             unordered_set<KEY>* goneKeys, unordered_set<VAL>* goneVals) {
        for (auto key : keys) {
            auto iter = mMap.find(key);
            if (iter != mMap.end() && trimOrRemove(iter, rVals, goneVals) && nullptr != goneKeys) {
                goneKeys->insert(iter->first);
            }
        }
    }

    // This adds all VALs from *newVals* to the map entry keyed by *key*. Or if it
    // doesn't exist yet, add the set to the map.
    bool add(const KEY& key, const unordered_set<VAL>& newVals) {
        bool newEntryAdded = false;
        if (!newVals.empty()) {
            auto iter = mMap.find(key);
            if (iter != mMap.end()) {
                iter->second.insert(newVals.begin(), newVals.end());
            } else {
                mMap[key] = newVals;
                newEntryAdded = true;
            }
        }
        return newEntryAdded;
    }

    // This adds to each of entries in the map keyed by *keys* with the VALs in the
    // *enwVals*. If there new entries added (new key in *keys*), *newKeys*, if not
    // null, would be populated with those keys.
    inline void add(const unordered_set<KEY>& keys, const unordered_set<VAL>&& newVals,
                    unordered_set<KEY>* newKeys) {
        add(keys, newVals, newKeys);
    }
    inline void add(const unordered_set<KEY>& keys, const unordered_set<VAL>& newVals,
                    unordered_set<KEY>* newKeys) {
        for (auto key : keys) {
            if (add(key, newVals) && nullptr != newKeys) {
                newKeys->insert(key);
            }
        }
    }

    // This puts *newVals* into the map keyed by *key*, and returns the VALs that are
    // in effect removed from the keyed VAL set in the map entry.
    // This call would also remove those same VALs from *newVals*.
    inline unordered_set<VAL> update(const KEY& key, unordered_set<VAL>& newVals) {
        unordered_set<VAL> goneVals(0);

        if (newVals.empty()) {
            mMap.erase(key);
        } else {
            auto curVals = mMap[key];
            mMap[key] = newVals;
            goneVals = removeAndReturnInterset(curVals, newVals);
        }
        return goneVals;
    }
};

} // namespace loc_util

#endif // #ifndef __LOC_UNORDERDED_SETMAP_H__
