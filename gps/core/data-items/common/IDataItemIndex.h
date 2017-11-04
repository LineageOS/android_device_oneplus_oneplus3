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

#ifndef __IDATAITEMINDEX_H__
#define __IDATAITEMINDEX_H__

#include <list>

namespace loc_core
{

template <typename CT, typename DIT>

class IDataItemIndex {

public:

    // gets std :: list of subscribed clients
    virtual void getListOfSubscribedClients
    (
        DIT id,
        std :: list <CT> & out
    ) = 0;

    // removes an entry from
    virtual int remove (DIT id) = 0;

    // removes list of clients and returns a list of data items
    // removed if any.
    virtual void remove
    (
        const std :: list <CT> & r,
        std :: list <DIT> & out
    ) = 0;

    // removes list of clients indexed by data item and returns list of
    // clients removed if any.
    virtual void remove
    (
        DIT id,
        const std :: list <CT> & r,
        std :: list <CT> & out
    ) = 0;

    // adds/modifies entry and returns new clients added
    virtual void add
    (
        DIT id,
        const std :: list <CT> & l,
        std :: list <CT> & out
    ) = 0;

    // adds/modifies entry and returns yet to subscribe list of data items
    virtual void add
    (
        CT client,
        const std :: list <DIT> & l,
        std :: list <DIT> & out
    ) = 0;

    // dtor
    virtual ~IDataItemIndex () {}
};

} // namespace loc_core

#endif // #ifndef __IDATAITEMINDEX_H__

