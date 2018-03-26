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

using namespace loc_core;

#define XTRA_HAL_SOCKET_NAME "/data/vendor/location/xtra/socket_hal_xtra"

bool XtraSystemStatusObserver::updateLockStatus(uint32_t lock) {
    stringstream ss;
    ss <<  "gpslock";
    ss << " " << lock;
    ss << "\n"; // append seperator
    return ( sendEvent(ss) );
}

bool XtraSystemStatusObserver::updateConnectionStatus(bool connected, uint32_t type) {
    stringstream ss;
    ss <<  "connection";
    ss << " " << (connected ? "1" : "0");
    ss << " " << (int)type;
    ss << "\n"; // append seperator
    return ( sendEvent(ss) );
}
bool XtraSystemStatusObserver::updateTac(const string& tac) {
    stringstream ss;
    ss <<  "tac";
    ss << " " << tac.c_str();
    ss << "\n"; // append seperator
    return ( sendEvent(ss) );
}

bool XtraSystemStatusObserver::updateMccMnc(const string& mccmnc) {
    stringstream ss;
    ss <<  "mncmcc";
    ss << " " << mccmnc.c_str();
    ss << "\n"; // append seperator
    return ( sendEvent(ss) );
}

bool XtraSystemStatusObserver::sendEvent(const stringstream& event) {
    int socketFd = createSocket();
    if (socketFd < 0) {
        LOC_LOGe("XTRA unreachable. sending failed.");
        return false;
    }

    const string& data = event.str();
    int remain = data.length();
    ssize_t sent = 0;
    while (remain > 0 &&
          (sent = ::send(socketFd, data.c_str() + (data.length() - remain),
                       remain, MSG_NOSIGNAL)) > 0) {
        remain -= sent;
    }

    if (sent < 0) {
        LOC_LOGe("sending error. reason:%s", strerror(errno));
    }

    closeSocket(socketFd);

    return (remain == 0);
}


int XtraSystemStatusObserver::createSocket() {
    int socketFd = -1;

    if ((socketFd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        LOC_LOGe("create socket error. reason:%s", strerror(errno));

     } else {
        const char* socketPath = XTRA_HAL_SOCKET_NAME ;
        struct sockaddr_un addr = { .sun_family = AF_UNIX };
        snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socketPath);

        if (::connect(socketFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            LOC_LOGe("cannot connect to XTRA. reason:%s", strerror(errno));
            if (::close(socketFd)) {
                LOC_LOGe("close socket error. reason:%s", strerror(errno));
            }
            socketFd = -1;
        }
    }

    return socketFd;
}

void XtraSystemStatusObserver::closeSocket(const int socketFd) {
    if (socketFd >= 0) {
        if(::close(socketFd)) {
            LOC_LOGe("close socket error. reason:%s", strerror(errno));
        }
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
    struct handleOsObserverUpdateMsg : public LocMsg {
        XtraSystemStatusObserver* mXtraSysStatObj;
        list <IDataItemCore*> mDataItemList;

        inline handleOsObserverUpdateMsg(XtraSystemStatusObserver* xtraSysStatObs,
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

        inline ~handleOsObserverUpdateMsg() {
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
                        SystemStatusNetworkInfo* networkInfo =
                                reinterpret_cast<SystemStatusNetworkInfo*>(each);
                        mXtraSysStatObj->updateConnectionStatus(networkInfo->mConnected,
                                networkInfo->mType);
                    }
                    break;

                    case TAC_DATA_ITEM_ID:
                    {
                        SystemStatusTac* tac = reinterpret_cast<SystemStatusTac*>(each);
                        mXtraSysStatObj->updateTac(tac->mValue);
                    }
                    break;

                    case MCCMNC_DATA_ITEM_ID:
                    {
                        SystemStatusMccMnc* mccmnc = reinterpret_cast<SystemStatusMccMnc*>(each);
                        mXtraSysStatObj->updateMccMnc(mccmnc->mValue);
                    }
                    break;

                    default:
                    break;
                }
            }
        }
    };
    mMsgTask->sendMsg(new (nothrow) handleOsObserverUpdateMsg(this, dlist));
}


