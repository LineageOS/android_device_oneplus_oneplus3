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

#ifndef __ICLIENTINDEX_H__
#define __ICLIENTINDEX_H__

#include <list>

namespace loc_core
{

template  <typename CT, typename DIT>

class IClientIndex {
public:

    // Checks if client is subscribed
    virtual bool isSubscribedClient (CT client) = 0;

    // gets subscription list
    virtual void getSubscribedList (CT client, std :: list <DIT> & out) = 0;

    // removes an entry
    virtual int remove (CT client) = 0;

    // removes std :: list of data items and returns a list of clients
    // removed if any.
    virtual void remove
    (
        const std :: list <DIT> & r,
        std :: list <CT> & out
    ) = 0;

    // removes list of data items indexed by client and returns list
    // of data items removed if any.
    virtual void remove
    (
        CT client,
        const std :: list <DIT> & r,
        std :: list <DIT> & out
    ) = 0;

    // adds/modifies entry in  map and returns new data items added.
    virtual void add
    (
        CT client,
        const std :: list <DIT> & l,
        std :: list <DIT> & out
    ) = 0;

    // dtor
    virtual ~IClientIndex () {}
};

} // namespace loc_core

#endif // #ifndef __ICLIENTINDEX_H__
