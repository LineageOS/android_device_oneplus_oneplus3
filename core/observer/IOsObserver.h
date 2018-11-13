/* Copyright (c) 2017, The Linux Foundation. All rights reserved.
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

#ifndef __IOSOBSERVER_H__
#define __IOSOBSERVER_H__

#include  <list>
#include <string>
#include <IDataItemObserver.h>
#include <IDataItemSubscription.h>
#include <IFrameworkActionReq.h>

using namespace std;

namespace loc_core
{

/**
 * @brief IOsObserver interface
 * @details IOsObserver interface;
 *          In OS dependent code this type serves as a handle to
 *          an OS independent instance of this interface.
 */
class IOsObserver :
                public IDataItemObserver,
                public IDataItemSubscription,
                public IFrameworkActionReq {

public:

    // To set the subscription object
    virtual void setSubscriptionObj(IDataItemSubscription *subscriptionObj) = 0;

    // To set the framework action request object
    virtual void setFrameworkActionReqObj(IFrameworkActionReq *frameworkActionReqObj) = 0;

    // IDataItemObserver Overrides
    inline virtual void getName (string & /*name*/) {}
    inline virtual void notify (const std::list <IDataItemCore *> & /*dlist*/) {}

    // IDataItemSubscription Overrides
    inline virtual void subscribe
    (
        const std :: list <DataItemId> & /*l*/,
        IDataItemObserver * /*client*/
    ){}
    inline virtual void updateSubscription
    (
        const std :: list <DataItemId> & /*l*/,
        IDataItemObserver * /*client*/
    ){}
    inline virtual void requestData
    (
        const std :: list <DataItemId> & /*l*/,
        IDataItemObserver * /*client*/
    ){}
    inline virtual void unsubscribe
    (
        const std :: list <DataItemId> & /*l*/,
        IDataItemObserver * /*client*/
    ){}
    inline virtual void unsubscribeAll (IDataItemObserver * /*client*/){}

    // IFrameworkActionReq Overrides
    inline virtual void turnOn (DataItemId /*dit*/, int /*timeOut*/){}
    inline virtual void turnOff (DataItemId /*dit*/) {}
#ifdef USE_GLIB
    inline virtual bool connectBackhaul() {}
    inline virtual bool disconnectBackhaul() {}
#endif

    /**
     * @brief Destructor
     * @details Destructor
     */
    virtual ~IOsObserver () {}
};

} // namespace loc_core

#endif // #ifndef __IOSOBSERVER_H__
