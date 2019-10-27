/* Copyright (c) 2017, 2019, The Linux Foundation. All rights reserved.
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

using namespace loc_util;
using namespace loc_core;

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "LocSvc_XSSO"

class XtraIpcListener : public ILocIpcListener {
    IOsObserver*    mSystemStatusObsrvr;
    const MsgTask* mMsgTask;
    XtraSystemStatusObserver& mXSSO;
public:
    inline XtraIpcListener(IOsObserver* observer, const MsgTask* msgTask,
                           XtraSystemStatusObserver& xsso) :
            mSystemStatusObsrvr(observer), mMsgTask(msgTask), mXSSO(xsso) {}
    virtual void onReceive(const char* data, uint32_t length,
                           const LocIpcRecver* recver) override {
#define STRNCMP(str, constStr) strncmp(str, constStr, sizeof(constStr)-1)
        if (!STRNCMP(data, "ping")) {
            LOC_LOGd("ping received");
#ifdef USE_GLIB
        } else if (!STRNCMP(data, "connectBackhaul")) {
            mSystemStatusObsrvr->connectBackhaul();
        } else if (!STRNCMP(data, "disconnectBackhaul")) {
            mSystemStatusObsrvr->disconnectBackhaul();
#endif
        } else if (!STRNCMP(data, "requestStatus")) {
            int32_t xtraStatusUpdated = 0;
            sscanf(data, "%*s %d", &xtraStatusUpdated);

            struct HandleStatusRequestMsg : public LocMsg {
                XtraSystemStatusObserver& mXSSO;
                int32_t mXtraStatusUpdated;
                inline HandleStatusRequestMsg(XtraSystemStatusObserver& xsso,
                                              int32_t xtraStatusUpdated) :
                        mXSSO(xsso), mXtraStatusUpdated(xtraStatusUpdated) {}
                inline void proc() const override {
                    mXSSO.onStatusRequested(mXtraStatusUpdated);
                }
            };
            mMsgTask->sendMsg(new HandleStatusRequestMsg(mXSSO, xtraStatusUpdated));
        } else {
            LOC_LOGw("unknown event: %s", data);
        }
    }
};

XtraSystemStatusObserver::XtraSystemStatusObserver(IOsObserver* sysStatObs,
                                                   const MsgTask* msgTask) :
        mSystemStatusObsrvr(sysStatObs), mMsgTask(msgTask),
        mGpsLock(-1), mConnections(~0), mXtraThrottle(true),
        mReqStatusReceived(false),
        mIsConnectivityStatusKnown(false),
        mSender(LocIpc::getLocIpcLocalSender(LOC_IPC_XTRA)),
        mDelayLocTimer(*mSender) {
    subscribe(true);
    auto recver = LocIpc::getLocIpcLocalRecver(
            make_shared<XtraIpcListener>(sysStatObs, msgTask, *this),
            LOC_IPC_HAL);
    mIpc.startNonBlockingListening(recver);
    mDelayLocTimer.start(100 /*.1 sec*/,  false);
}

bool XtraSystemStatusObserver::updateLockStatus(GnssConfigGpsLock lock) {
    // mask NI(NFW bit) since from XTRA's standpoint GPS is enabled if
    // MO(AFW bit) is enabled and disabled when MO is disabled
    mGpsLock = lock & ~GNSS_CONFIG_GPS_LOCK_NI;

    if (!mReqStatusReceived) {
        return true;
    }

    stringstream ss;
    ss <<  "gpslock";
    ss << " " << mGpsLock;
    string s = ss.str();
    return ( LocIpc::send(*mSender, (const uint8_t*)s.data(), s.size()) );
}

bool XtraSystemStatusObserver::updateConnections(uint64_t allConnections,
        NetworkInfoType* networkHandleInfo) {
    mIsConnectivityStatusKnown = true;
    mConnections = allConnections;

    LOC_LOGd("updateConnections mConnections:%" PRIx64, mConnections);
    for (uint8_t i = 0; i < MAX_NETWORK_HANDLES; ++i) {
        mNetworkHandle[i] = networkHandleInfo[i];
        LOC_LOGd("updateConnections [%d] networkHandle:%" PRIx64 " networkType:%u",
            i, mNetworkHandle[i].networkHandle, mNetworkHandle[i].networkType);
    }

    if (!mReqStatusReceived) {
        return true;
    }

    stringstream ss;
    ss << "connection" << endl << mConnections << endl
            << mNetworkHandle[0].toString() << endl
            << mNetworkHandle[1].toString() << endl
            << mNetworkHandle[2].toString() << endl
            << mNetworkHandle[3].toString() << endl
            << mNetworkHandle[4].toString() << endl
            << mNetworkHandle[5].toString() << endl
            << mNetworkHandle[6].toString() << endl
            << mNetworkHandle[7].toString() << endl
            << mNetworkHandle[8].toString() << endl
            << mNetworkHandle[MAX_NETWORK_HANDLES-1].toString();
    string s = ss.str();
    return ( LocIpc::send(*mSender, (const uint8_t*)s.data(), s.size()) );
}

bool XtraSystemStatusObserver::updateTac(const string& tac) {
    mTac = tac;

    if (!mReqStatusReceived) {
        return true;
    }

    stringstream ss;
    ss <<  "tac";
    ss << " " << tac.c_str();
    string s = ss.str();
    return ( LocIpc::send(*mSender, (const uint8_t*)s.data(), s.size()) );
}

bool XtraSystemStatusObserver::updateMccMnc(const string& mccmnc) {
    mMccmnc = mccmnc;

    if (!mReqStatusReceived) {
        return true;
    }

    stringstream ss;
    ss <<  "mncmcc";
    ss << " " << mccmnc.c_str();
    string s = ss.str();
    return ( LocIpc::send(*mSender, (const uint8_t*)s.data(), s.size()) );
}

bool XtraSystemStatusObserver::updateXtraThrottle(const bool enabled) {
    mXtraThrottle = enabled;

    if (!mReqStatusReceived) {
        return true;
    }

    stringstream ss;
    ss <<  "xtrathrottle";
    ss << " " << (enabled ? 1 : 0);
    string s = ss.str();
    return ( LocIpc::send(*mSender, (const uint8_t*)s.data(), s.size()) );
}

inline bool XtraSystemStatusObserver::onStatusRequested(int32_t xtraStatusUpdated) {
    mReqStatusReceived = true;

    if (xtraStatusUpdated) {
        return true;
    }

    stringstream ss;

    ss << "respondStatus" << endl;
    (mGpsLock == -1 ? ss : ss << mGpsLock) << endl;
    (mConnections == (uint64_t)~0 ? ss : ss << mConnections) << endl
            << mNetworkHandle[0].toString() << endl
            << mNetworkHandle[1].toString() << endl
            << mNetworkHandle[2].toString() << endl
            << mNetworkHandle[3].toString() << endl
            << mNetworkHandle[4].toString() << endl
            << mNetworkHandle[5].toString() << endl
            << mNetworkHandle[6].toString() << endl
            << mNetworkHandle[7].toString() << endl
            << mNetworkHandle[8].toString() << endl
            << mNetworkHandle[MAX_NETWORK_HANDLES-1].toString() << endl
            << mTac << endl << mMccmnc << endl << mIsConnectivityStatusKnown;

    string s = ss.str();
    return ( LocIpc::send(*mSender, (const uint8_t*)s.data(), s.size()) );
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
            for (auto itor = mDataItemList.begin(); itor != mDataItemList.end(); ++itor) {
                if (*itor != nullptr) {
                    delete *itor;
                    *itor = nullptr;
                }
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
                        NetworkInfoType* networkHandleInfo =
                                static_cast<NetworkInfoType*>(networkInfo->getNetworkHandle());
                        mXtraSysStatObj->updateConnections(networkInfo->getAllTypes(),
                                networkHandleInfo);
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
