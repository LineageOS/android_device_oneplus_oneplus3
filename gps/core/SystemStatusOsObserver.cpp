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
#define LOG_TAG "LocSvc_SystemStatusOsObserver"

#include <algorithm>
#include <SystemStatus.h>
#include <SystemStatusOsObserver.h>
#include <IDataItemCore.h>
#include <IClientIndex.h>
#include <IDataItemIndex.h>
#include <IndexFactory.h>
#include <DataItemsFactoryProxy.h>

namespace loc_core
{
SystemStatusOsObserver::SystemStatusOsObserver(const MsgTask* msgTask) :
    mAddress("SystemStatusOsObserver"),
    mClientIndex(IndexFactory<IDataItemObserver*, DataItemId> :: createClientIndex()),
    mDataItemIndex(IndexFactory<IDataItemObserver*, DataItemId> :: createDataItemIndex())
{
    mContext.mMsgTask = msgTask;
}

SystemStatusOsObserver::~SystemStatusOsObserver()
{
    // Close data-item library handle
    DataItemsFactoryProxy::closeDataItemLibraryHandle();

    // Destroy cache
    for (auto each : mDataItemCache) {
        if (nullptr != each.second) {
            delete each.second;
        }
    }

    mDataItemCache.clear();
    delete mClientIndex;
    delete mDataItemIndex;
}

void SystemStatusOsObserver::setSubscriptionObj(IDataItemSubscription* subscriptionObj)
{
    mContext.mSubscriptionObj = subscriptionObj;

    LOC_LOGD("Request cache size -  Subscribe:%zu RequestData:%zu",
            mSubscribeReqCache.size(), mReqDataCache.size());

    // we have received the subscription object. process cached requests
    // process - subscribe request cache
    for (auto each : mSubscribeReqCache) {
        subscribe(each.second, each.first);
    }
    // process - requestData request cache
    for (auto each : mReqDataCache) {
        requestData(each.second, each.first);
    }
}

// Helper to cache requests subscribe and requestData till subscription obj is obtained
void SystemStatusOsObserver::cacheObserverRequest(ObserverReqCache& reqCache,
        const list<DataItemId>& l, IDataItemObserver* client)
{
    ObserverReqCache::iterator dicIter = reqCache.find(client);
    if (dicIter != reqCache.end()) {
        // found
        list<DataItemId> difference(0);
        set_difference(l.begin(), l.end(),
                dicIter->second.begin(), dicIter->second.end(),
                inserter(difference, difference.begin()));
        if (!difference.empty()) {
            difference.sort();
            dicIter->second.merge(difference);
            dicIter->second.unique();
        }
    }
    else {
        // not found
        reqCache[client] = l;
    }
}

/******************************************************************************
 IDataItemSubscription Overrides
******************************************************************************/
void SystemStatusOsObserver::subscribe(
        const list<DataItemId>& l, IDataItemObserver* client)
{
    if (nullptr == mContext.mSubscriptionObj) {
        LOC_LOGD("%s]: Subscription object is NULL. Caching requests", __func__);
        cacheObserverRequest(mSubscribeReqCache, l, client);
        return;
    }

    struct HandleSubscribeReq : public LocMsg {
        HandleSubscribeReq(SystemStatusOsObserver* parent,
                const list<DataItemId>& l, IDataItemObserver* client) :
                mParent(parent), mClient(client), mDataItemList(l) {}
        virtual ~HandleSubscribeReq() {}
        void proc() const {

            if (mDataItemList.empty()) {
                LOC_LOGV("mDataItemList is empty. Nothing to do. Exiting");
                return;
            }

            // Handle First Response
            list<DataItemId> pendingFirstResponseList(0);
            mParent->mClientIndex->add(mClient, mDataItemList, pendingFirstResponseList);

            // Do not send first response for only pendingFirstResponseList,
            // instead send for all the data items  (present in the cache) that
            // have been subscribed for each time.
            mParent->sendFirstResponse(mDataItemList, mClient);

            list<DataItemId> yetToSubscribeDataItemsList(0);
            mParent->mDataItemIndex->add(mClient, mDataItemList, yetToSubscribeDataItemsList);

            // Send subscription list to framework
            if (!yetToSubscribeDataItemsList.empty()) {
                mParent->mContext.mSubscriptionObj->subscribe(yetToSubscribeDataItemsList, mParent);
                LOC_LOGD("Subscribe Request sent to framework for the following");
                mParent->logMe(yetToSubscribeDataItemsList);
            }
        }
        SystemStatusOsObserver* mParent;
        IDataItemObserver* mClient;
        const list<DataItemId> mDataItemList;
    };
    mContext.mMsgTask->sendMsg(new (nothrow) HandleSubscribeReq(this, l, client));
}

void SystemStatusOsObserver::updateSubscription(
        const list<DataItemId>& l, IDataItemObserver* client)
{
    if (nullptr == mContext.mSubscriptionObj) {
        LOC_LOGE("%s:%d]: Subscription object is NULL", __func__, __LINE__);
        return;
    }

    struct HandleUpdateSubscriptionReq : public LocMsg {
        HandleUpdateSubscriptionReq(SystemStatusOsObserver* parent,
                const list<DataItemId>& l, IDataItemObserver* client) :
                mParent(parent), mClient(client), mDataItemList(l) {}
        virtual ~HandleUpdateSubscriptionReq() {}
        void proc() const {
            if (mDataItemList.empty()) {
                LOC_LOGV("mDataItemList is empty. Nothing to do. Exiting");
                return;
            }

            list<DataItemId> currentlySubscribedList(0);
            mParent->mClientIndex->getSubscribedList(mClient, currentlySubscribedList);

            list<DataItemId> removeDataItemList(0);
            set_difference(currentlySubscribedList.begin(), currentlySubscribedList.end(),
                    mDataItemList.begin(), mDataItemList.end(),
                    inserter(removeDataItemList,removeDataItemList.begin()));

            // Handle First Response
            list<DataItemId> pendingFirstResponseList(0);
            mParent->mClientIndex->add(mClient, mDataItemList, pendingFirstResponseList);

            // Send First Response
            mParent->sendFirstResponse(pendingFirstResponseList, mClient);

            list<DataItemId> yetToSubscribeDataItemsList(0);
            mParent->mDataItemIndex->add(
                    mClient, mDataItemList, yetToSubscribeDataItemsList);

            // Send subscription list to framework
            if (!yetToSubscribeDataItemsList.empty()) {
                mParent->mContext.mSubscriptionObj->subscribe(
                        yetToSubscribeDataItemsList, mParent);
                LOC_LOGD("Subscribe Request sent to framework for the following");
                mParent->logMe(yetToSubscribeDataItemsList);
            }

            list<DataItemId> unsubscribeList(0);
            list<DataItemId> unused(0);
            mParent->mClientIndex->remove(mClient, removeDataItemList, unused);

            if (!mParent->mClientIndex->isSubscribedClient(mClient)) {
                mParent->mDataItemIndex->remove(
                        list<IDataItemObserver*> (1,mClient), unsubscribeList);
            }
            if (!unsubscribeList.empty()) {
                // Send unsubscribe to framework
                mParent->mContext.mSubscriptionObj->unsubscribe(unsubscribeList, mParent);
                LOC_LOGD("Unsubscribe Request sent to framework for the following");
                mParent->logMe(unsubscribeList);
            }
        }
        SystemStatusOsObserver* mParent;
        IDataItemObserver* mClient;
        const list<DataItemId> mDataItemList;
    };
    mContext.mMsgTask->sendMsg(new (nothrow) HandleUpdateSubscriptionReq(this, l, client));
}

void SystemStatusOsObserver::requestData(
        const list<DataItemId>& l, IDataItemObserver* client)
{
    if (nullptr == mContext.mSubscriptionObj) {
        LOC_LOGD("%s]: Subscription object is NULL. Caching requests", __func__);
        cacheObserverRequest(mReqDataCache, l, client);
        return;
    }

    struct HandleRequestData : public LocMsg {
        HandleRequestData(SystemStatusOsObserver* parent,
                const list<DataItemId>& l, IDataItemObserver* client) :
                mParent(parent), mClient(client), mDataItemList(l) {}
        virtual ~HandleRequestData() {}
        void proc() const {
            if (mDataItemList.empty()) {
                LOC_LOGV("mDataItemList is empty. Nothing to do. Exiting");
                return;
            }

            list<DataItemId> yetToSubscribeDataItemsList(0);
            mParent->mClientIndex->add(
                    mClient, mDataItemList, yetToSubscribeDataItemsList);
            mParent->mDataItemIndex->add(
                    mClient, mDataItemList, yetToSubscribeDataItemsList);

            // Send subscription list to framework
            if (!mDataItemList.empty()) {
                mParent->mContext.mSubscriptionObj->requestData(mDataItemList, mParent);
                LOC_LOGD("Subscribe Request sent to framework for the following");
                mParent->logMe(yetToSubscribeDataItemsList);
            }
        }
        SystemStatusOsObserver* mParent;
        IDataItemObserver* mClient;
        const list<DataItemId> mDataItemList;
    };
    mContext.mMsgTask->sendMsg(new (nothrow) HandleRequestData(this, l, client));
}

void SystemStatusOsObserver::unsubscribe(
        const list<DataItemId>& l, IDataItemObserver* client)
{
    if (nullptr == mContext.mSubscriptionObj) {
        LOC_LOGE("%s:%d]: Subscription object is NULL", __func__, __LINE__);
        return;
    }
    struct HandleUnsubscribeReq : public LocMsg {
        HandleUnsubscribeReq(SystemStatusOsObserver* parent,
                const list<DataItemId>& l, IDataItemObserver* client) :
                mParent(parent), mClient(client), mDataItemList(l) {}
        virtual ~HandleUnsubscribeReq() {}
        void proc() const {
            if (mDataItemList.empty()) {
                LOC_LOGV("mDataItemList is empty. Nothing to do. Exiting");
                return;
            }

            list<DataItemId> unsubscribeList(0);
            list<DataItemId> unused(0);
            mParent->mClientIndex->remove(mClient, mDataItemList, unused);

            for (auto each : mDataItemList) {
                list<IDataItemObserver*> clientListSubs(0);
                list<IDataItemObserver*> clientListOut(0);
                mParent->mDataItemIndex->remove(
                        each, list<IDataItemObserver*> (1,mClient), clientListOut);
                // check if there are any other subscribed client for this data item id
                mParent->mDataItemIndex->getListOfSubscribedClients(each, clientListSubs);
                if (clientListSubs.empty())
                {
                    LOC_LOGD("Client list subscribed is empty for dataitem - %d", each);
                    unsubscribeList.push_back(each);
                }
            }

            if (!unsubscribeList.empty()) {
                // Send unsubscribe to framework
                mParent->mContext.mSubscriptionObj->unsubscribe(unsubscribeList, mParent);
                LOC_LOGD("Unsubscribe Request sent to framework for the following data items");
                mParent->logMe(unsubscribeList);
            }
        }
        SystemStatusOsObserver* mParent;
        IDataItemObserver* mClient;
        const list<DataItemId> mDataItemList;
    };
    mContext.mMsgTask->sendMsg(new (nothrow) HandleUnsubscribeReq(this, l, client));
}

void SystemStatusOsObserver::unsubscribeAll(IDataItemObserver* client)
{
    if (nullptr == mContext.mSubscriptionObj) {
        LOC_LOGE("%s:%d]: Subscription object is NULL", __func__, __LINE__);
        return;
    }

    struct HandleUnsubscribeAllReq : public LocMsg {
        HandleUnsubscribeAllReq(SystemStatusOsObserver* parent,
                IDataItemObserver* client) :
                mParent(parent), mClient(client) {}
        virtual ~HandleUnsubscribeAllReq() {}
        void proc() const {
            list<IDataItemObserver*> clients(1, mClient);
            list<DataItemId> unsubscribeList(0);
            if(0 == mParent->mClientIndex->remove(mClient)) {
                return;
            }
            mParent->mDataItemIndex->remove(clients, unsubscribeList);

            if (!unsubscribeList.empty()) {
                // Send unsubscribe to framework
                mParent->mContext.mSubscriptionObj->unsubscribe(unsubscribeList, mParent);
                LOC_LOGD("Unsubscribe Request sent to framework for the following data items");
                mParent->logMe(unsubscribeList);
            }
        }
        SystemStatusOsObserver* mParent;
        IDataItemObserver* mClient;
    };
    mContext.mMsgTask->sendMsg(new (nothrow) HandleUnsubscribeAllReq(this, client));
}

/******************************************************************************
 IDataItemObserver Overrides
******************************************************************************/
void SystemStatusOsObserver::notify(const list<IDataItemCore*>& dlist)
{
    list<IDataItemCore*> dataItemList(0);

    for (auto each : dlist) {
        string dv;
        each->stringify(dv);
        LOC_LOGD("notify: DataItem In Value:%s", dv.c_str());

        IDataItemCore* di = DataItemsFactoryProxy::createNewDataItem(each->getId());
        if (nullptr == di) {
            LOC_LOGE("Unable to create dataitem:%d", each->getId());
            return;
        }

        // Copy contents into the newly created data item
        di->copy(each);
        dataItemList.push_back(di);
        // Request systemstatus to record this dataitem in its cache
        SystemStatus* systemstatus = SystemStatus::getInstance(mContext.mMsgTask);
        if(nullptr != systemstatus) {
            systemstatus->eventDataItemNotify(di);
        }
    }

    struct HandleNotify : public LocMsg {
        HandleNotify(SystemStatusOsObserver* parent, const list<IDataItemCore*>& l) :
            mParent(parent), mDList(l) {}
        virtual ~HandleNotify() {
            for (auto each : mDList) {
                delete each;
            }
        }
        void proc() const {
            // Update Cache with received data items and prepare
            // list of data items to be sent.
            list<DataItemId> dataItemIdsToBeSent(0);
            for (auto item : mDList) {
                bool dataItemUpdated = false;
                mParent->updateCache(item, dataItemUpdated);
                if (dataItemUpdated) {
                    dataItemIdsToBeSent.push_back(item->getId());
                }
            }

            // Send data item to all subscribed clients
            list<IDataItemObserver*> clientList(0);
            for (auto each : dataItemIdsToBeSent) {
                list<IDataItemObserver*> clients(0);
                mParent->mDataItemIndex->getListOfSubscribedClients(each, clients);
                for (auto each_cient: clients) {
                    clientList.push_back(each_cient);
                }
            }
            clientList.unique();

            for (auto client : clientList) {
                list<DataItemId> dataItemIdsSubscribedByThisClient(0);
                list<DataItemId> dataItemIdsToBeSentForThisClient(0);
                mParent->mClientIndex->getSubscribedList(
                        client, dataItemIdsSubscribedByThisClient);
                dataItemIdsSubscribedByThisClient.sort();
                dataItemIdsToBeSent.sort();

                set_intersection(dataItemIdsToBeSent.begin(),
                        dataItemIdsToBeSent.end(),
                        dataItemIdsSubscribedByThisClient.begin(),
                        dataItemIdsSubscribedByThisClient.end(),
                        inserter(dataItemIdsToBeSentForThisClient,
                        dataItemIdsToBeSentForThisClient.begin()));

                mParent->sendCachedDataItems(dataItemIdsToBeSentForThisClient, client);
                dataItemIdsSubscribedByThisClient.clear();
                dataItemIdsToBeSentForThisClient.clear();
            }
        }
        SystemStatusOsObserver* mParent;
        const list<IDataItemCore*> mDList;
    };
    mContext.mMsgTask->sendMsg(new (nothrow) HandleNotify(this, dataItemList));
}

/******************************************************************************
 IFrameworkActionReq Overrides
******************************************************************************/
void SystemStatusOsObserver::turnOn(DataItemId dit, int timeOut)
{
    if (nullptr == mContext.mFrameworkActionReqObj) {
        LOC_LOGE("%s:%d]: Framework action request object is NULL", __func__, __LINE__);
        return;
    }

    // Check if data item exists in mActiveRequestCount
    map<DataItemId, int>::iterator citer = mActiveRequestCount.find(dit);
    if (citer == mActiveRequestCount.end()) {
        // Data item not found in map
        // Add reference count as 1 and add dataitem to map
        pair<DataItemId, int> cpair(dit, 1);
        mActiveRequestCount.insert(cpair);
        LOC_LOGD("Sending turnOn request");

        // Send action turn on to framework
        struct HandleTurnOnMsg : public LocMsg {
            HandleTurnOnMsg(IFrameworkActionReq* framework,
                    DataItemId dit, int timeOut) :
                    mFrameworkActionReqObj(framework), mDataItemId(dit), mTimeOut(timeOut) {}
            virtual ~HandleTurnOnMsg() {}
            void proc() const {
                mFrameworkActionReqObj->turnOn(mDataItemId, mTimeOut);
            }
            IFrameworkActionReq* mFrameworkActionReqObj;
            DataItemId mDataItemId;
            int mTimeOut;
        };
        mContext.mMsgTask->sendMsg(new (nothrow) HandleTurnOnMsg(this, dit, timeOut));
    }
    else {
        // Found in map, update reference count
        citer->second++;
        LOC_LOGD("turnOn - Data item:%d Num_refs:%d", dit, citer->second);
    }
}

void SystemStatusOsObserver::turnOff(DataItemId dit)
{
    if (nullptr == mContext.mFrameworkActionReqObj) {
        LOC_LOGE("%s:%d]: Framework action request object is NULL", __func__, __LINE__);
        return;
    }

    // Check if data item exists in mActiveRequestCount
    map<DataItemId, int>::iterator citer = mActiveRequestCount.find(dit);
    if (citer != mActiveRequestCount.end()) {
        // found
        citer->second--;
        LOC_LOGD("turnOff - Data item:%d Remaining:%d", dit, citer->second);
        if(citer->second == 0) {
            // if this was last reference, remove item from map and turn off module
            mActiveRequestCount.erase(citer);

            // Send action turn off to framework
            struct HandleTurnOffMsg : public LocMsg {
                HandleTurnOffMsg(IFrameworkActionReq* framework, DataItemId dit) :
                    mFrameworkActionReqObj(framework), mDataItemId(dit) {}
                virtual ~HandleTurnOffMsg() {}
                void proc() const {
                    mFrameworkActionReqObj->turnOff(mDataItemId);
                }
                IFrameworkActionReq* mFrameworkActionReqObj;
                DataItemId mDataItemId;
            };
            mContext.mMsgTask->sendMsg(
                    new (nothrow) HandleTurnOffMsg(mContext.mFrameworkActionReqObj, dit));
        }
    }
}

/******************************************************************************
 Helpers
******************************************************************************/
void SystemStatusOsObserver::sendFirstResponse(
        const list<DataItemId>& l, IDataItemObserver* to)
{
    if (l.empty()) {
        LOC_LOGV("list is empty. Nothing to do. Exiting");
        return;
    }

    string clientName;
    to->getName(clientName);
    list<IDataItemCore*> dataItems(0);

    for (auto each : l) {
        map<DataItemId, IDataItemCore*>::const_iterator citer = mDataItemCache.find(each);
        if (citer != mDataItemCache.end()) {
            string dv;
            citer->second->stringify(dv);
            LOC_LOGI("DataItem: %s >> %s", dv.c_str(), clientName.c_str());
            dataItems.push_back(citer->second);
        }
    }
    if (dataItems.empty()) {
        LOC_LOGV("No items to notify. Nothing to do. Exiting");
        return;
    }
    to->notify(dataItems);
}

void SystemStatusOsObserver::sendCachedDataItems(
        const list<DataItemId>& l, IDataItemObserver* to)
{
    string clientName;
    to->getName(clientName);
    list<IDataItemCore*> dataItems(0);

    for (auto each : l) {
        string dv;
        IDataItemCore* di = mDataItemCache[each];
        di->stringify(dv);
        LOC_LOGI("DataItem: %s >> %s", dv.c_str(), clientName.c_str());
        dataItems.push_back(di);
    }
    to->notify(dataItems);
}

void SystemStatusOsObserver::updateCache(IDataItemCore* d, bool& dataItemUpdated)
{
    if (nullptr == d) {
        return;
    }

    // Check if data item exists in cache
    map<DataItemId, IDataItemCore*>::iterator citer =
            mDataItemCache.find(d->getId());
    if (citer == mDataItemCache.end()) {
        // New data item; not found in cache
        IDataItemCore* dataitem = DataItemsFactoryProxy::createNewDataItem(d->getId());
        if (nullptr == dataitem) {
            return;
        }

        // Copy the contents of the data item
        dataitem->copy(d);
        pair<DataItemId, IDataItemCore*> cpair(d->getId(), dataitem);
        // Insert in mDataItemCache
        mDataItemCache.insert(cpair);
        dataItemUpdated = true;
    }
    else {
        // Found in cache; Update cache if necessary
        if(0 == citer->second->copy(d, &dataItemUpdated)) {
            return;
        }
    }

    if (dataItemUpdated) {
        LOC_LOGV("DataItem:%d updated:%d", d->getId(), dataItemUpdated);
    }
}

} // namespace loc_core

