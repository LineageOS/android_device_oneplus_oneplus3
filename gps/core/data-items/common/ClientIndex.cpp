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
#include <algorithm>
#include <iterator>
#include <string>
#include <platform_lib_log_util.h>
#include <ClientIndex.h>
#include <IDataItemObserver.h>
#include <DataItemId.h>

using namespace std;
using namespace loc_core;

template <typename CT, typename DIT>
inline ClientIndex <CT,DIT> :: ClientIndex () {}

template <typename CT, typename DIT>
inline ClientIndex <CT,DIT> :: ~ClientIndex () {}

template <typename CT, typename DIT>
bool ClientIndex <CT,DIT> :: isSubscribedClient (CT client) {
    bool result = false;
    ENTRY_LOG ();
    typename map < CT, list <DIT> > :: iterator it =
        mDataItemsPerClientMap.find (client);
    if (it != mDataItemsPerClientMap.end ()) {
        result = true;
    }
    EXIT_LOG_WITH_ERROR ("%d",result);
    return result;
}

template <typename CT, typename DIT>
void ClientIndex <CT,DIT> :: getSubscribedList (CT client, list <DIT> & out) {
    ENTRY_LOG ();
    typename map < CT, list <DIT> > :: iterator it =
        mDataItemsPerClientMap.find (client);
    if (it != mDataItemsPerClientMap.end ()) {
        out = it->second;
    }
    EXIT_LOG_WITH_ERROR ("%d",0);
}

template <typename CT, typename DIT>
int ClientIndex <CT,DIT> :: remove (CT client) {
    int result = 0;
    ENTRY_LOG ();
    mDataItemsPerClientMap.erase (client);
    EXIT_LOG_WITH_ERROR ("%d",result);
    return result;
}

template <typename CT, typename DIT>
void ClientIndex <CT,DIT> :: remove (const list <DIT> & r, list <CT> & out) {
    ENTRY_LOG ();
    typename map < CT, list <DIT> > :: iterator dicIter =
        mDataItemsPerClientMap.begin ();
    while (dicIter != mDataItemsPerClientMap.end()) {
        typename list <DIT> :: const_iterator it = r.begin ();
        for (; it != r.end (); ++it) {
            typename list <DIT> :: iterator iter =
                find (dicIter->second.begin (), dicIter->second.end (), *it);
            if (iter != dicIter->second.end ()) {
                dicIter->second.erase (iter);
            }
        }
        if (dicIter->second.empty ()) {
            out.push_back (dicIter->first);
            // Post-increment operator increases the iterator but returns the
            // prevous one that will be invalidated by erase()
            mDataItemsPerClientMap.erase (dicIter++);
        } else {
            ++dicIter;
        }
    }
    EXIT_LOG_WITH_ERROR ("%d",0);
}

template <typename CT, typename DIT>
void ClientIndex <CT,DIT> :: remove
(
    CT client,
    const list <DIT> & r,
    list <DIT> & out
)
{
    ENTRY_LOG ();
    typename map < CT, list <DIT> > :: iterator dicIter =
        mDataItemsPerClientMap.find (client);
    if (dicIter != mDataItemsPerClientMap.end ()) {
        set_intersection (dicIter->second.begin (), dicIter->second.end (),
                         r.begin (), r.end (),
                         inserter (out,out.begin ()));
        if (!out.empty ()) {
            typename list <DIT> :: iterator it = out.begin ();
            for (; it != out.end (); ++it) {
                dicIter->second.erase (find (dicIter->second.begin (),
                                            dicIter->second.end (),
                                            *it));
            }
        }
        if (dicIter->second.empty ()) {
            mDataItemsPerClientMap.erase (dicIter);
            EXIT_LOG_WITH_ERROR ("%d",0);
        }
    }
    EXIT_LOG_WITH_ERROR ("%d",0);
}

template <typename CT, typename DIT>
void ClientIndex <CT,DIT> :: add
(
    CT client,
    const list <DIT> & l,
    list <DIT> & out
)
{
    ENTRY_LOG ();
    list <DIT> difference;
    typename map < CT, list <DIT> > :: iterator dicIter =
        mDataItemsPerClientMap.find (client);
    if (dicIter != mDataItemsPerClientMap.end ()) {
        set_difference (l.begin (), l.end (),
                       dicIter->second.begin (), dicIter->second.end (),
                       inserter (difference,difference.begin ()));
        if (!difference.empty ()) {
            difference.sort ();
            out = difference;
            dicIter->second.merge (difference);
            dicIter->second.unique ();
        }
    } else {
        out = l;
        pair < CT, list <DIT> > dicnpair (client, out);
        mDataItemsPerClientMap.insert (dicnpair);
    }
    EXIT_LOG_WITH_ERROR ("%d",0);
}

// Explicit instantiation must occur in same namespace where class is defined
namespace loc_core
{
  template class ClientIndex <IDataItemObserver *, DataItemId>;
  template class ClientIndex <string, DataItemId>;
}
