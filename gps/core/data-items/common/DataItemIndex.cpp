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

#include <string>
#include <algorithm>
#include <iterator>
#include <DataItemIndex.h>
#include <platform_lib_log_util.h>
#include <IDataItemObserver.h>
#include <DataItemId.h>

using namespace std;
using namespace loc_core;

template <typename CT, typename DIT>
inline DataItemIndex <CT,DIT> :: DataItemIndex () {}

template <typename CT, typename DIT>
inline DataItemIndex <CT,DIT> :: ~DataItemIndex () {}

template <typename CT, typename DIT>
void DataItemIndex <CT,DIT> :: getListOfSubscribedClients
 (
    DIT id,
    list <CT> & out
)
{
    typename map < DIT, list <CT> > :: iterator cdiIter =
        mClientsPerDataItemMap.find (id);
    if (cdiIter != mClientsPerDataItemMap.end ()) {
        out = cdiIter->second;
    }
}


template <typename CT, typename DIT>
int DataItemIndex <CT,DIT> :: remove (DIT id) {
    int result = 0;
    ENTRY_LOG ();
    mClientsPerDataItemMap.erase (id);
    EXIT_LOG_WITH_ERROR ("%d",result);
    return result;
}

template <typename CT, typename DIT>
void DataItemIndex <CT,DIT> :: remove (const list <CT> & r, list <DIT> & out) {
    ENTRY_LOG ();
    typename map < DIT, list <CT> > :: iterator cdiIter =
        mClientsPerDataItemMap.begin ();
    while (cdiIter != mClientsPerDataItemMap.end()) {
        typename list <CT> :: const_iterator it = r.begin ();
        for (; it != r.end (); ++it) {
            typename list <CT> :: iterator iter =
                find
                (
                    cdiIter->second.begin (),
                    cdiIter->second.end (),
                    *it
                );
            if (iter != cdiIter->second.end ()) {
                cdiIter->second.erase (iter);
            }
        }

        if (cdiIter->second.empty ()) {
            out.push_back (cdiIter->first);
            // Post-increment operator increases the iterator but returns the
            // prevous one that will be invalidated by erase()
            mClientsPerDataItemMap.erase (cdiIter++);
        } else {
            ++cdiIter;
        }
    }
    EXIT_LOG_WITH_ERROR ("%d",0);
}

template <typename CT, typename DIT>
void DataItemIndex <CT,DIT> :: remove
(
    DIT id,
    const list <CT> & r,
    list <CT> & out
)
{
    ENTRY_LOG ();

    typename map < DIT, list <CT> > :: iterator cdiIter =
        mClientsPerDataItemMap.find (id);
    if (cdiIter != mClientsPerDataItemMap.end ()) {
        set_intersection (cdiIter->second.begin (), cdiIter->second.end (),
                         r.begin (), r.end (),
                         inserter (out, out.begin ()));
        if (!out.empty ()) {
            typename list <CT> :: iterator it = out.begin ();
            for (; it != out.end (); ++it) {
                cdiIter->second.erase (find (cdiIter->second.begin (),
                                            cdiIter->second.end (),
                                            *it));
            }
        }
        if (cdiIter->second.empty ()) {
            mClientsPerDataItemMap.erase (cdiIter);
            EXIT_LOG_WITH_ERROR ("%d",0);
        }
    }
    EXIT_LOG_WITH_ERROR ("%d",0);
}

template <typename CT, typename DIT>
void DataItemIndex <CT,DIT> :: add
(
    DIT id,
    const list <CT> & l,
    list <CT> & out
)
{
    ENTRY_LOG ();
    list <CT> difference;
    typename map < DIT, list <CT> > :: iterator cdiIter =
        mClientsPerDataItemMap.find (id);
    if (cdiIter != mClientsPerDataItemMap.end ()) {
        set_difference (l.begin (), l.end (),
                       cdiIter->second.begin (), cdiIter->second.end (),
                       inserter (difference, difference.begin ()));
        if (!difference.empty ()) {
            difference.sort ();
            out = difference;
            cdiIter->second.merge (difference);
        }
    } else {
        out = l;
        pair < DIT, list <CT> > cndipair (id, out);
        mClientsPerDataItemMap.insert (cndipair);
    }
    EXIT_LOG_WITH_ERROR ("%d",0);
}

template <typename CT, typename DIT>
void DataItemIndex <CT,DIT> :: add
(
    CT client,
    const list <DIT> & l,
    list <DIT> & out
)
{
    ENTRY_LOG ();
    typename map < DIT, list <CT> > :: iterator cdiIter;
    typename list <DIT> :: const_iterator it = l.begin ();
    for (; it != l.end (); ++it) {
        cdiIter = mClientsPerDataItemMap.find (*it);
        if (cdiIter == mClientsPerDataItemMap.end ()) {
            out.push_back (*it);
            pair < DIT, list <CT> > cndiPair (*it, list <CT> (1, client));
            mClientsPerDataItemMap.insert (cndiPair);
        } else {
          typename list<CT> :: iterator clientIter =
              find
              (
                cdiIter->second.begin (),
                cdiIter->second.end (),
                client
              );
            if (clientIter == cdiIter->second.end()) {
              cdiIter->second.push_back (client);
            }
        }
    }
    EXIT_LOG_WITH_ERROR ("%d",0);
}

// Explicit instantiation must occur in same namespace where class is defined
namespace loc_core
{
  template class DataItemIndex <IDataItemObserver *, DataItemId>;
  template class DataItemIndex <string, DataItemId>;
}
