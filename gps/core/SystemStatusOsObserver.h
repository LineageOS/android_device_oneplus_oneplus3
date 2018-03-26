/* Copyright (c) 2015-2017, The Linux Foundation. All rights reserved.
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
#ifndef __SYSTEM_STATUS_OSOBSERVER__
#define __SYSTEM_STATUS_OSOBSERVER__

#include <cinttypes>
#include <string>
#include <list>
#include <map>
#include <new>
#include <vector>

#include <MsgTask.h>
#include <DataItemId.h>
#include <IOsObserver.h>
#include <platform_lib_log_util.h>

namespace loc_core
{
/******************************************************************************
 SystemStatusOsObserver
******************************************************************************/
using namespace std;

// Forward Declarations
class IDataItemCore;
template<typename CT, typename DIT> class IClientIndex;
template<typename CT, typename DIT> class IDataItemIndex;

struct SystemContext {
    IDataItemSubscription* mSubscriptionObj;
    IFrameworkActionReq* mFrameworkActionReqObj;
    const MsgTask* mMsgTask;

    inline SystemContext() :
        mSubscriptionObj(NULL),
        mFrameworkActionReqObj(NULL),
        mMsgTask(NULL) {}
};

typedef map<IDataItemObserver*, list<DataItemId>> ObserverReqCache;

// Clients wanting to get data from OS/Framework would need to
// subscribe with OSObserver using IDataItemSubscription interface.
// Such clients would need to implement IDataItemObserver interface
// to receive data when it becomes available.
class SystemStatusOsObserver : public IOsObserver {

public:
    // ctor
    SystemStatusOsObserver(const MsgTask* msgTask);

    // dtor
    ~SystemStatusOsObserver();

    // To set the subscription object
    virtual void setSubscriptionObj(IDataItemSubscription* subscriptionObj);

    // To set the framework action request object
    inline void setFrameworkActionReqObj(IFrameworkActionReq* frameworkActionReqObj) {
        mContext.mFrameworkActionReqObj = frameworkActionReqObj;
    }

    // IDataItemSubscription Overrides
    virtual void subscribe(const list<DataItemId>& l, IDataItemObserver* client);
    virtual void updateSubscription(const list<DataItemId>& l, IDataItemObserver* client);
    virtual void requestData(const list<DataItemId>& l, IDataItemObserver* client);
    virtual void unsubscribe(const list<DataItemId>& l, IDataItemObserver* client);
    virtual void unsubscribeAll(IDataItemObserver* client);

    // IDataItemObserver Overrides
    virtual void notify(const list<IDataItemCore*>& dlist);
    inline virtual void getName(string& name) {
        name = mAddress;
    }

    // IFrameworkActionReq Overrides
    virtual void turnOn(DataItemId dit, int timeOut = 0);
    virtual void turnOff(DataItemId dit);

private:
    SystemContext                                    mContext;
    const string                                     mAddress;
    IClientIndex<IDataItemObserver*, DataItemId>*    mClientIndex;
    IDataItemIndex<IDataItemObserver*, DataItemId>*  mDataItemIndex;
    map<DataItemId, IDataItemCore*>                  mDataItemCache;
    map<DataItemId, int>                             mActiveRequestCount;

    // Cache the subscribe and requestData till subscription obj is obtained
    ObserverReqCache mSubscribeReqCache;
    ObserverReqCache mReqDataCache;
    void cacheObserverRequest(ObserverReqCache& reqCache,
            const list<DataItemId>& l, IDataItemObserver* client);

    // Helpers
    void sendFirstResponse(const list<DataItemId>& l, IDataItemObserver* to);
    void sendCachedDataItems(const list<DataItemId>& l, IDataItemObserver* to);
    void updateCache(IDataItemCore* d, bool& dataItemUpdated);
    inline void logMe(const list<DataItemId>& l) {
        for (auto id : l) {
            LOC_LOGD("DataItem %d", id);
        }
    }
};

} // namespace loc_core

#endif //__SYSTEM_STATUS__

