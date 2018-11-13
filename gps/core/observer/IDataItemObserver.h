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

#ifndef __IDATAITEMOBSERVER_H__
#define __IDATAITEMOBSERVER_H__

#include  <list>
#include <string>

using namespace std;

namespace loc_core
{
class IDataItemCore;

/**
 * @brief IDataItemObserver interface
 * @details IDataItemObserver interface;
 *          In OS dependent code this type serves as a handle to an OS independent instance of this interface.
 */
class IDataItemObserver {

public:

    /**
     * @brief Gets name of Data Item Observer
     * @details Gets name of Data Item Observer
     *
     * @param name reference to name of Data Item Observer
     */
    virtual void getName (string & name) = 0;

    /**
     * @brief Notify updated values of Data Items
     * @details Notifys updated values of Data items
     *
     * @param dlist List of updated data items
     */
    virtual void notify (const std :: list <IDataItemCore *> & dlist)  = 0;

    /**
     * @brief Destructor
     * @details Destructor
     */
    virtual ~IDataItemObserver () {}
};

} // namespace loc_core

#endif // #ifndef __IDATAITEMOBSERVER_H__
