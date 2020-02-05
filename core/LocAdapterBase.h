/* Copyright (c) 2011-2014, 2016-2019 The Linux Foundation. All rights reserved.
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
#include <ContextBase.h>
#include <LocationAPI.h>
#include <map>

#define MIN_TRACKING_INTERVAL (100) // 100 msec

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

typedef void (*removeClientCompleteCallback)(LocationAPI* client);

namespace loc_core {

class LocAdapterProxyBase;

class LocAdapterBase {
private:
    static uint32_t mSessionIdCounter;
    const bool mIsMaster;
    bool mIsEngineCapabilitiesKnown = false;

protected:
    LOC_API_ADAPTER_EVENT_MASK_T mEvtMask;
    ContextBase* mContext;
    LocApiBase* mLocApi;
    LocAdapterProxyBase* mLocAdapterProxyBase;
    const MsgTask* mMsgTask;
    bool mAdapterAdded;

    inline LocAdapterBase(const MsgTask* msgTask) :
        mIsMaster(false), mEvtMask(0), mContext(NULL), mLocApi(NULL),
        mLocAdapterProxyBase(NULL), mMsgTask(msgTask), mAdapterAdded(false) {}

    /* ==== CLIENT ========================================================================= */
    typedef std::map<LocationAPI*, LocationCallbacks> ClientDataMap;
    ClientDataMap mClientData;
    std::vector<LocMsg*> mPendingMsgs; // For temporal storage of msgs before Open is completed
    /* ======== UTILITIES ================================================================== */
    void saveClient(LocationAPI* client, const LocationCallbacks& callbacks);
    void eraseClient(LocationAPI* client);
    LocationCallbacks getClientCallbacks(LocationAPI* client);
    LocationCapabilitiesMask getCapabilities();
    void broadcastCapabilities(LocationCapabilitiesMask mask);
    virtual void updateClientsEventMask();
    virtual void stopClientSessions(LocationAPI* client);

public:
    inline virtual ~LocAdapterBase() { mLocApi->removeAdapter(this); }
    // When waitForDoneInit is not specified or specified as false,
    // handleEngineUpEvent may be called on the child adapter object from
    // a different thread before the constructor of the child
    // object finishes.
    //
    // If the handleEngineUpEvent relies on member variables of the constructor
    // of the child adapter to be initialized first, we need to specify the
    // waitForDoneInit to *TRUE* to delay handleEngineUpEvent to get called
    // until when the child adapter finishes its initialization and notify
    // LocAdapterBase via doneInit method.
    LocAdapterBase(const LOC_API_ADAPTER_EVENT_MASK_T mask,
                   ContextBase* context, bool isMaster = false,
                   LocAdapterProxyBase *adapterProxyBase = NULL,
                   bool waitForDoneInit = false);

    inline void doneInit() {
        if (!mAdapterAdded) {
            mLocApi->addAdapter(this);
            mAdapterAdded = true;
        }
    }

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

    inline void updateNmeaMask(uint32_t mask)
    {
        mLocApi->updateNmeaMask(mask);
    }

    inline bool isFeatureSupported(uint8_t featureVal) {
        return ContextBase::isFeatureSupported(featureVal);
    }

    uint32_t generateSessionId();

    inline bool isAdapterMaster() {
        return mIsMaster;
    }

    inline bool isEngineCapabilitiesKnown() { return mIsEngineCapabilitiesKnown;}
    inline void setEngineCapabilitiesKnown(bool value) { mIsEngineCapabilitiesKnown = value;}

    virtual void handleEngineUpEvent();
    virtual void handleEngineDownEvent();
    virtual void reportPositionEvent(const UlpLocation& location,
                                     const GpsLocationExtended& locationExtended,
                                     enum loc_sess_status status,
                                     LocPosTechMask loc_technology_mask,
                                     GnssDataNotification* pDataNotify = nullptr,
                                     int msInWeek = -1);
    virtual void reportEnginePositionsEvent(unsigned int count,
                                            EngineLocationInfo* locationArr) {
        (void)count;
        (void)locationArr;
    }
    virtual void reportSvEvent(const GnssSvNotification& svNotify,
                               bool fromEngineHub=false);
    virtual void reportDataEvent(const GnssDataNotification& dataNotify, int msInWeek);
    virtual void reportNmeaEvent(const char* nmea, size_t length);
    virtual void reportSvPolynomialEvent(GnssSvPolynomial &svPolynomial);
    virtual void reportSvEphemerisEvent(GnssSvEphemerisReport &svEphemeris);
    virtual void reportStatus(LocGpsStatusValue status);
    virtual bool reportXtraServer(const char* url1, const char* url2,
                                  const char* url3, const int maxlength);
    virtual void reportLocationSystemInfoEvent(const LocationSystemInfo& locationSystemInfo);

    virtual bool requestXtraData();
    virtual bool requestTime();
    virtual bool requestLocation();
    virtual bool requestATL(int connHandle, LocAGpsType agps_type,
                            LocApnTypeMask apn_type_mask);
    virtual bool releaseATL(int connHandle);
    virtual bool requestNiNotifyEvent(const GnssNiNotification &notify, const void* data,
                                      const LocInEmergency emergencyState);
    inline virtual bool isInSession() { return false; }
    ContextBase* getContext() const { return mContext; }
    virtual void reportGnssMeasurementsEvent(const GnssMeasurements& gnssMeasurements,
                                                int msInWeek);
    virtual bool reportWwanZppFix(LocGpsLocation &zppLoc);
    virtual bool reportZppBestAvailableFix(LocGpsLocation &zppLoc,
            GpsLocationExtended &location_extended, LocPosTechMask tech_mask);
    virtual void reportGnssSvIdConfigEvent(const GnssSvIdConfig& config);
    virtual void reportGnssSvTypeConfigEvent(const GnssSvTypeConfig& config);
    virtual bool requestOdcpiEvent(OdcpiRequestInfo& request);
    virtual bool reportGnssEngEnergyConsumedEvent(uint64_t energyConsumedSinceFirstBoot);
    virtual bool reportDeleteAidingDataEvent(GnssAidingData &aidingData);
    virtual bool reportKlobucharIonoModelEvent(GnssKlobucharIonoModel& ionoModel);
    virtual bool reportGnssAdditionalSystemInfoEvent(
            GnssAdditionalSystemInfo& additionalSystemInfo);
    virtual void reportNfwNotificationEvent(GnssNfwNotification& notification);

    virtual void geofenceBreachEvent(size_t count, uint32_t* hwIds, Location& location,
                                     GeofenceBreachType breachType, uint64_t timestamp);
    virtual void geofenceStatusEvent(GeofenceStatusAvailable available);

    virtual void reportPositionEvent(UlpLocation &location,
                                     GpsLocationExtended &locationExtended,
                                     enum loc_sess_status status,
                                     LocPosTechMask loc_technology_mask);

    virtual void reportLocationsEvent(const Location* locations, size_t count,
            BatchingMode batchingMode);
    virtual void reportCompletedTripsEvent(uint32_t accumulated_distance);
    virtual void reportBatchStatusChangeEvent(BatchingStatus batchStatus);

    /* ==== CLIENT ========================================================================= */
    /* ======== COMMANDS ====(Called from Client Thread)==================================== */
    void addClientCommand(LocationAPI* client, const LocationCallbacks& callbacks);
    void removeClientCommand(LocationAPI* client,
                             removeClientCompleteCallback rmClientCb);
    void requestCapabilitiesCommand(LocationAPI* client);

};

} // namespace loc_core

#endif //LOC_API_ADAPTER_BASE_H
