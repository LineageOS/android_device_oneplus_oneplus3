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
#define LOG_TAG "LocSvc_XtraSystemStatusObs"

#include <sys/stat.h>
#include <sys/un.h>
#include <errno.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <math.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <loc_log.h>
#include <loc_nmea.h>
#include <SystemStatus.h>
#include <vector>
#include <sstream>
#include <XtraSystemStatusObserver.h>
#include <LocAdapterBase.h>
#include <DataItemId.h>
#include <DataItemsFactoryProxy.h>
#include <DataItemConcreteTypesBase.h>

using namespace loc_core;

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "LocSvc_XSSO"

bool XtraSystemStatusObserver::updateLockStatus(uint32_t lock) {
    mGpsLock = lock;

    if (!mReqStatusReceived) {
        return true;
    }

    stringstream ss;
    ss <<  "gpslock";
    ss << " " << lock;
    return ( send(LOC_IPC_XTRA, ss.str()) );
}

bool XtraSystemStatusObserver::updateConnections(uint64_t allConnections) {
    mIsConnectivityStatusKnown = true;
    mConnections = allConnections;

    if (!mReqStatusReceived) {
        return true;
    }

    stringstream ss;
    ss <<  "connection";
    ss << " " << mConnections;
    return ( send(LOC_IPC_XTRA, ss.str()) );
}

bool XtraSystemStatusObserver::updateTac(const string& tac) {
    mTac = tac;

    if (!mReqStatusReceived) {
        return true;
    }

    stringstream ss;
    ss <<  "tac";
    ss << " " << tac.c_str();
    return ( send(LOC_IPC_XTRA, ss.str()) );
}

bool XtraSystemStatusObserver::updateMccMnc(const string& mccmnc) {
    mMccmnc = mccmnc;

    if (!mReqStatusReceived) {
        return true;
    }

    stringstream ss;
    ss <<  "mncmcc";
    ss << " " << mccmnc.c_str();
    return ( send(LOC_IPC_XTRA, ss.str()) );
}

bool XtraSystemStatusObserver::updateXtraThrottle(const bool enabled) {
    mXtraThrottle = enabled;

    if (!mReqStatusReceived) {
        return true;
    }

    stringstream ss;
    ss <<  "xtrathrottle";
    ss << " " << (enabled ? 1 : 0);
    return ( send(LOC_IPC_XTRA, ss.str()) );
}

inline bool XtraSystemStatusObserver::onStatusRequested(int32_t xtraStatusUpdated) {
    mReqStatusReceived = true;

    if (xtraStatusUpdated) {
        return true;
    }

    stringstream ss;

    ss << "respondStatus" << endl;
    (mGpsLock == -1 ? ss : ss << mGpsLock) << endl << mConnections << endl
            << mTac << endl << mMccmnc << endl << mIsConnectivityStatusKnown;

    return ( send(LOC_IPC_XTRA, ss.str()) );
}

void XtraSystemStatusObserver::onReceive(const std::string& data) {
    if (!strncmp(data.c_str(), "ping", sizeof("ping") - 1)) {
        LOC_LOGd("ping received");

#ifdef USE_GLIB
    } else if (!strncmp(data.c_str(), "connectBackhaul", sizeof("connectBackhaul") - 1)) {
        mSystemStatusObsrvr->connectBackhaul();

    } else if (!strncmp(data.c_str(), "disconnectBackhaul", sizeof("disconnectBackhaul") - 1)) {
        mSystemStatusObsrvr->disconnectBackhaul();
#endif

    } else if (!strncmp(data.c_str(), "requestStatus", sizeof("requestStatus") - 1)) {
        int32_t xtraStatusUpdated = 0;
        sscanf(data.c_str(), "%*s %d", &xtraStatusUpdated);

        struct HandleStatusRequestMsg : public LocMsg {
            XtraSystemStatusObserver& mXSSO;
            int32_t mXtraStatusUpdated;
            inline HandleStatusRequestMsg(XtraSystemStatusObserver& xsso,
                    int32_t xtraStatusUpdated) :
                    mXSSO(xsso), mXtraStatusUpdated(xtraStatusUpdated) {}
            inline void proc() const override { mXSSO.onStatusRequested(mXtraStatusUpdated); }
        };
        mMsgTask->sendMsg(new (nothrow) HandleStatusRequestMsg(*this, xtraStatusUpdated));

    } else {
        LOC_LOGw("unknown event: %s", data.c_str());
    }
}

void XtraSystemStatusObserver::subscribe(bool yes)
{
    // Subscription data list
    list<DataItemId> subItemIdList;
    subItemIdList.push_back(NETWORKINFO_DATA_ITEM_ID);
    subItemIdList.push_back(MCCMNC_DATA_ITEM_ID);

    if (yes) {
        mSystemStatusObsrvr->subscribe(subItemIdList, this);

        list<DataItemId> reqItemIdList;
        reqItemIdList.push_back(TAC_DATA_ITEM_ID);

        mSystemStatusObsrvr->requestData(reqItemIdList, this);

    } else {
        mSystemStatusObsrvr->unsubscribe(subItemIdList, this);
    }
}

// IDataItemObserver overrides
void XtraSystemStatusObserver::getName(string& name)
{
    name = "XtraSystemStatusObserver";
}

void XtraSystemStatusObserver::notify(const list<IDataItemCore*>& dlist)
{
    struct HandleOsObserverUpdateMsg : public LocMsg {
        XtraSystemStatusObserver* mXtraSysStatObj;
        list <IDataItemCore*> mDataItemList;

        inline HandleOsObserverUpdateMsg(XtraSystemStatusObserver* xtraSysStatObs,
                const list<IDataItemCore*>& dataItemList) :
                mXtraSysStatObj(xtraSysStatObs) {
            for (auto eachItem : dataItemList) {
                IDataItemCore* dataitem = DataItemsFactoryProxy::createNewDataItem(
                        eachItem->getId());
                if (NULL == dataitem) {
                    break;
                }
                // Copy the contents of the data item
                dataitem->copy(eachItem);

                mDataItemList.push_back(dataitem);
            }
        }

        inline ~HandleOsObserverUpdateMsg() {
            for (auto each : mDataItemList) {
                delete each;
            }
        }

        inline void proc() const {
            for (auto each : mDataItemList) {
                switch (each->getId())
                {
                    case NETWORKINFO_DATA_ITEM_ID:
                    {
                        NetworkInfoDataItemBase* networkInfo =
                                static_cast<NetworkInfoDataItemBase*>(each);
                        mXtraSysStatObj->updateConnections(networkInfo->getAllTypes());
                    }
                    break;

                    case TAC_DATA_ITEM_ID:
                    {
                        TacDataItemBase* tac =
                                 static_cast<TacDataItemBase*>(each);
                        mXtraSysStatObj->updateTac(tac->mValue);
                    }
                    break;

                    case MCCMNC_DATA_ITEM_ID:
                    {
                        MccmncDataItemBase* mccmnc =
                                static_cast<MccmncDataItemBase*>(each);
                        mXtraSysStatObj->updateMccMnc(mccmnc->mValue);
                    }
                    break;

                    default:
                    break;
                }
            }
        }
    };
    mMsgTask->sendMsg(new (nothrow) HandleOsObserverUpdateMsg(this, dlist));
}
