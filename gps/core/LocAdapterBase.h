/* Copyright (c) 2011-2014, 2016-2017 The Linux Foundation. All rights reserved.
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
#ifndef LOC_API_ADAPTER_BASE_H
#define LOC_API_ADAPTER_BASE_H

#include <gps_extended.h>
#include <UlpProxyBase.h>
#include <ContextBase.h>
#include <LocationAPI.h>
#include <map>

typedef struct LocationSessionKey {
    LocationAPI* client;
    uint32_t id;
    inline LocationSessionKey(LocationAPI* _client, uint32_t _id) :
        client(_client), id(_id) {}
} LocationSessionKey;
inline bool operator <(LocationSessionKey const& left, LocationSessionKey const& right) {
    return left.id < right.id || (left.id == right.id && left.client < right.client);
}
inline bool operator ==(LocationSessionKey const& left, LocationSessionKey const& right) {
    return left.id == right.id && left.client == right.client;
}
inline bool operator !=(LocationSessionKey const& left, LocationSessionKey const& right) {
    return left.id != right.id || left.client != right.client;
}
typedef std::map<LocationSessionKey, LocationOptions> LocationSessionMap;

namespace loc_core {

class LocAdapterProxyBase;

class LocAdapterBase {
private:
    static uint32_t mSessionIdCounter;
protected:
    LOC_API_ADAPTER_EVENT_MASK_T mEvtMask;
    ContextBase* mContext;
    LocApiBase* mLocApi;
    LocAdapterProxyBase* mLocAdapterProxyBase;
    const MsgTask* mMsgTask;
    inline LocAdapterBase(const MsgTask* msgTask) :
        mEvtMask(0), mContext(NULL), mLocApi(NULL),
        mLocAdapterProxyBase(NULL), mMsgTask(msgTask) {}
public:
    inline virtual ~LocAdapterBase() { mLocApi->removeAdapter(this); }
    LocAdapterBase(const LOC_API_ADAPTER_EVENT_MASK_T mask,
                   ContextBase* context, LocAdapterProxyBase *adapterProxyBase = NULL);
    inline LOC_API_ADAPTER_EVENT_MASK_T
        checkMask(LOC_API_ADAPTER_EVENT_MASK_T mask) const {
        return mEvtMask & mask;
    }

    inline LOC_API_ADAPTER_EVENT_MASK_T getEvtMask() const {
        return mEvtMask;
    }

    inline void sendMsg(const LocMsg* msg) const {
        mMsgTask->sendMsg(msg);
    }

    inline void sendMsg(const LocMsg* msg) {
        mMsgTask->sendMsg(msg);
    }

    inline void updateEvtMask(LOC_API_ADAPTER_EVENT_MASK_T event,
                              loc_registration_mask_status status)
    {
        switch(status) {
            case (LOC_REGISTRATION_MASK_ENABLED):
                mEvtMask = mEvtMask | event;
                break;
            case (LOC_REGISTRATION_MASK_DISABLED):
                mEvtMask = mEvtMask &~ event;
                break;
            case (LOC_REGISTRATION_MASK_SET):
                mEvtMask = event;
                break;
        }
        mLocApi->updateEvtMask();
    }

    inline bool isFeatureSupported(uint8_t featureVal) {
        return mLocApi->isFeatureSupported(featureVal);
    }

    uint32_t generateSessionId();

    // This will be overridden by the individual adapters
    // if necessary.
    inline virtual void setUlpProxyCommand(UlpProxyBase* ulp) {

        (void)ulp;
    }
    virtual void handleEngineUpEvent();
    virtual void handleEngineDownEvent();
    inline virtual void setPositionModeCommand(LocPosMode& posMode) {

        (void)posMode;
    }
    virtual void startTrackingCommand() {}
    virtual void stopTrackingCommand() {}
    virtual void getZppCommand() {}
    virtual void reportPositionEvent(const UlpLocation& location,
                                     const GpsLocationExtended& locationExtended,
                                     enum loc_sess_status status,
                                     LocPosTechMask loc_technology_mask,
                                     bool fromUlp=false);
    virtual void reportSvEvent(const GnssSvNotification& svNotify, bool fromUlp=false);
    virtual void reportNmeaEvent(const char* nmea, size_t length, bool fromUlp=false);
    virtual void reportSvMeasurementEvent(GnssSvMeasurementSet &svMeasurementSet);
    virtual void reportSvPolynomialEvent(GnssSvPolynomial &svPolynomial);
    virtual void reportStatus(LocGpsStatusValue status);
    virtual bool reportXtraServer(const char* url1, const char* url2,
                                  const char* url3, const int maxlength);
    virtual bool requestXtraData();
    virtual bool requestTime();
    virtual bool requestLocation();
    virtual bool requestATL(int connHandle, LocAGpsType agps_type);
    virtual bool releaseATL(int connHandle);
    virtual bool requestSuplES(int connHandle);
    virtual bool reportDataCallOpened();
    virtual bool reportDataCallClosed();
    virtual bool requestNiNotifyEvent(const GnssNiNotification &notify, const void* data);
    inline virtual bool isInSession() { return false; }
    ContextBase* getContext() const { return mContext; }
    virtual void reportGnssMeasurementDataEvent(const GnssMeasurementsNotification& measurements,
                                                int msInWeek);
    virtual bool reportWwanZppFix(LocGpsLocation &zppLoc);
    virtual bool reportOdcpiRequestEvent(OdcpiRequestInfo& request);
};

} // namespace loc_core

#endif //LOC_API_ADAPTER_BASE_H
