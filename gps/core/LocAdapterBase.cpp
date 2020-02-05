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
#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_LocAdapterBase"

#include <dlfcn.h>
#include <LocAdapterBase.h>
#include <loc_target.h>
#include <log_util.h>
#include <LocAdapterProxyBase.h>

namespace loc_core {

// This is the top level class, so the constructor will
// always gets called. Here we prepare for the default.
// But if getLocApi(targetEnumType target) is overriden,
// the right locApi should get created.
LocAdapterBase::LocAdapterBase(const LOC_API_ADAPTER_EVENT_MASK_T mask,
                               ContextBase* context, bool isMaster,
                               LocAdapterProxyBase *adapterProxyBase,
                               bool waitForDoneInit) :
    mIsMaster(isMaster), mEvtMask(mask), mContext(context),
    mLocApi(context->getLocApi()), mLocAdapterProxyBase(adapterProxyBase),
    mMsgTask(context->getMsgTask()),
    mIsEngineCapabilitiesKnown(ContextBase::sIsEngineCapabilitiesKnown)
{
    LOC_LOGd("waitForDoneInit: %d", waitForDoneInit);
    if (!waitForDoneInit) {
        mLocApi->addAdapter(this);
        mAdapterAdded = true;
    } else {
        mAdapterAdded = false;
    }
}

uint32_t LocAdapterBase::mSessionIdCounter(1);

uint32_t LocAdapterBase::generateSessionId()
{
    if (++mSessionIdCounter == 0xFFFFFFFF)
        mSessionIdCounter = 1;

     return mSessionIdCounter;
}

void LocAdapterBase::handleEngineUpEvent()
{
    if (mLocAdapterProxyBase) {
        mLocAdapterProxyBase->handleEngineUpEvent();
    }
}

void LocAdapterBase::handleEngineDownEvent()
{
    if (mLocAdapterProxyBase) {
        mLocAdapterProxyBase->handleEngineDownEvent();
    }
}

void LocAdapterBase::
    reportPositionEvent(const UlpLocation& location,
                        const GpsLocationExtended& locationExtended,
                        enum loc_sess_status status,
                        LocPosTechMask loc_technology_mask,
                        GnssDataNotification* pDataNotify,
                        int msInWeek)
{
    if (mLocAdapterProxyBase != NULL) {
        mLocAdapterProxyBase->reportPositionEvent((UlpLocation&)location,
                                                   (GpsLocationExtended&)locationExtended,
                                                   status,
                                                   loc_technology_mask);
    } else {
        DEFAULT_IMPL()
    }
}

void LocAdapterBase::
    reportSvEvent(const GnssSvNotification& /*svNotify*/,
                  bool /*fromEngineHub*/)
DEFAULT_IMPL()

void LocAdapterBase::
    reportSvPolynomialEvent(GnssSvPolynomial &/*svPolynomial*/)
DEFAULT_IMPL()

void LocAdapterBase::
    reportSvEphemerisEvent(GnssSvEphemerisReport &/*svEphemeris*/)
DEFAULT_IMPL()


void LocAdapterBase::
    reportStatus(LocGpsStatusValue /*status*/)
DEFAULT_IMPL()


void LocAdapterBase::
    reportNmeaEvent(const char* /*nmea*/, size_t /*length*/)
DEFAULT_IMPL()

void LocAdapterBase::
    reportDataEvent(const GnssDataNotification& /*dataNotify*/,
                    int /*msInWeek*/)
DEFAULT_IMPL()

bool LocAdapterBase::
    reportXtraServer(const char* /*url1*/, const char* /*url2*/,
                     const char* /*url3*/, const int /*maxlength*/)
DEFAULT_IMPL(false)

void LocAdapterBase::
    reportLocationSystemInfoEvent(const LocationSystemInfo& /*locationSystemInfo*/)
DEFAULT_IMPL()

bool LocAdapterBase::
    requestXtraData()
DEFAULT_IMPL(false)

bool LocAdapterBase::
    requestTime()
DEFAULT_IMPL(false)

bool LocAdapterBase::
    requestLocation()
DEFAULT_IMPL(false)

bool LocAdapterBase::
    requestATL(int /*connHandle*/, LocAGpsType /*agps_type*/,
               LocApnTypeMask /*apn_type_mask*/)
DEFAULT_IMPL(false)

bool LocAdapterBase::
    releaseATL(int /*connHandle*/)
DEFAULT_IMPL(false)

bool LocAdapterBase::
    requestNiNotifyEvent(const GnssNiNotification &/*notify*/,
                         const void* /*data*/,
                         const LocInEmergency emergencyState)
DEFAULT_IMPL(false)

void LocAdapterBase::
reportGnssMeasurementsEvent(const GnssMeasurements& /*gnssMeasurements*/,
                                   int /*msInWeek*/)
DEFAULT_IMPL()

bool LocAdapterBase::
    reportWwanZppFix(LocGpsLocation &/*zppLoc*/)
DEFAULT_IMPL(false)

bool LocAdapterBase::
    reportZppBestAvailableFix(LocGpsLocation& /*zppLoc*/,
            GpsLocationExtended& /*location_extended*/, LocPosTechMask /*tech_mask*/)
DEFAULT_IMPL(false)

void LocAdapterBase::reportGnssSvIdConfigEvent(const GnssSvIdConfig& /*config*/)
DEFAULT_IMPL()

void LocAdapterBase::reportGnssSvTypeConfigEvent(const GnssSvTypeConfig& /*config*/)
DEFAULT_IMPL()

bool LocAdapterBase::
    requestOdcpiEvent(OdcpiRequestInfo& /*request*/)
DEFAULT_IMPL(false)

bool LocAdapterBase::
    reportGnssEngEnergyConsumedEvent(uint64_t /*energyConsumedSinceFirstBoot*/)
DEFAULT_IMPL(false)

bool LocAdapterBase::
    reportDeleteAidingDataEvent(GnssAidingData & /*aidingData*/)
DEFAULT_IMPL(false)

bool LocAdapterBase::
    reportKlobucharIonoModelEvent(GnssKlobucharIonoModel& /*ionoModel*/)
DEFAULT_IMPL(false)

bool LocAdapterBase::
    reportGnssAdditionalSystemInfoEvent(GnssAdditionalSystemInfo& /*additionalSystemInfo*/)
DEFAULT_IMPL(false)

void LocAdapterBase::
    reportNfwNotificationEvent(GnssNfwNotification& /*notification*/)
DEFAULT_IMPL()

void
LocAdapterBase::geofenceBreachEvent(size_t /*count*/, uint32_t* /*hwIds*/, Location& /*location*/,
                                    GeofenceBreachType /*breachType*/, uint64_t /*timestamp*/)
DEFAULT_IMPL()

void
LocAdapterBase::geofenceStatusEvent(GeofenceStatusAvailable /*available*/)
DEFAULT_IMPL()

void
LocAdapterBase::reportLocationsEvent(const Location* /*locations*/, size_t /*count*/,
                                     BatchingMode /*batchingMode*/)
DEFAULT_IMPL()

void
LocAdapterBase::reportCompletedTripsEvent(uint32_t /*accumulated_distance*/)
DEFAULT_IMPL()

void
LocAdapterBase::reportBatchStatusChangeEvent(BatchingStatus /*batchStatus*/)
DEFAULT_IMPL()

void
LocAdapterBase::reportPositionEvent(UlpLocation& /*location*/,
                                    GpsLocationExtended& /*locationExtended*/,
                                    enum loc_sess_status /*status*/,
                                    LocPosTechMask /*loc_technology_mask*/)
DEFAULT_IMPL()

void
LocAdapterBase::saveClient(LocationAPI* client, const LocationCallbacks& callbacks)
{
    mClientData[client] = callbacks;
    updateClientsEventMask();
}

void
LocAdapterBase::eraseClient(LocationAPI* client)
{
    auto it = mClientData.find(client);
    if (it != mClientData.end()) {
        mClientData.erase(it);
    }
    updateClientsEventMask();
}

LocationCallbacks
LocAdapterBase::getClientCallbacks(LocationAPI* client)
{
    LocationCallbacks callbacks = {};
    auto it = mClientData.find(client);
    if (it != mClientData.end()) {
        callbacks = it->second;
    }
    return callbacks;
}

LocationCapabilitiesMask
LocAdapterBase::getCapabilities()
{
    LocationCapabilitiesMask mask = 0;

    if (isEngineCapabilitiesKnown()) {
        // time based tracking always supported
        mask |= LOCATION_CAPABILITIES_TIME_BASED_TRACKING_BIT;
        if (ContextBase::isMessageSupported(
                LOC_API_ADAPTER_MESSAGE_DISTANCE_BASE_LOCATION_BATCHING)){
            mask |= LOCATION_CAPABILITIES_TIME_BASED_BATCHING_BIT |
                    LOCATION_CAPABILITIES_DISTANCE_BASED_BATCHING_BIT;
        }
        if (ContextBase::isMessageSupported(LOC_API_ADAPTER_MESSAGE_DISTANCE_BASE_TRACKING)) {
            mask |= LOCATION_CAPABILITIES_DISTANCE_BASED_TRACKING_BIT;
        }
        if (ContextBase::isMessageSupported(LOC_API_ADAPTER_MESSAGE_OUTDOOR_TRIP_BATCHING)) {
            mask |= LOCATION_CAPABILITIES_OUTDOOR_TRIP_BATCHING_BIT;
        }
        // geofence always supported
        mask |= LOCATION_CAPABILITIES_GEOFENCE_BIT;
        if (ContextBase::gnssConstellationConfig()) {
            mask |= LOCATION_CAPABILITIES_GNSS_MEASUREMENTS_BIT;
        }
        uint32_t carrierCapabilities = ContextBase::getCarrierCapabilities();
        if (carrierCapabilities & LOC_GPS_CAPABILITY_MSB) {
            mask |= LOCATION_CAPABILITIES_GNSS_MSB_BIT;
        }
        if (LOC_GPS_CAPABILITY_MSA & carrierCapabilities) {
            mask |= LOCATION_CAPABILITIES_GNSS_MSA_BIT;
        }
        if (ContextBase::isFeatureSupported(LOC_SUPPORTED_FEATURE_DEBUG_NMEA_V02)) {
            mask |= LOCATION_CAPABILITIES_DEBUG_NMEA_BIT;
        }
        if (ContextBase::isFeatureSupported(LOC_SUPPORTED_FEATURE_CONSTELLATION_ENABLEMENT_V02)) {
            mask |= LOCATION_CAPABILITIES_CONSTELLATION_ENABLEMENT_BIT;
        }
        if (ContextBase::isFeatureSupported(LOC_SUPPORTED_FEATURE_AGPM_V02)) {
            mask |= LOCATION_CAPABILITIES_AGPM_BIT;
        }
        if (ContextBase::isFeatureSupported(LOC_SUPPORTED_FEATURE_LOCATION_PRIVACY)) {
            mask |= LOCATION_CAPABILITIES_PRIVACY_BIT;
        }
    } else {
        LOC_LOGE("%s]: attempt to get capabilities before they are known.", __func__);
    }

    return mask;
}

void
LocAdapterBase::broadcastCapabilities(LocationCapabilitiesMask mask)
{
    for (auto clientData : mClientData) {
        if (nullptr != clientData.second.capabilitiesCb) {
            clientData.second.capabilitiesCb(mask);
        }
    }
}

void
LocAdapterBase::updateClientsEventMask()
DEFAULT_IMPL()

void
LocAdapterBase::stopClientSessions(LocationAPI* client)
DEFAULT_IMPL()

void
LocAdapterBase::addClientCommand(LocationAPI* client, const LocationCallbacks& callbacks)
{
    LOC_LOGD("%s]: client %p", __func__, client);

    struct MsgAddClient : public LocMsg {
        LocAdapterBase& mAdapter;
        LocationAPI* mClient;
        const LocationCallbacks mCallbacks;
        inline MsgAddClient(LocAdapterBase& adapter,
                            LocationAPI* client,
                            const LocationCallbacks& callbacks) :
            LocMsg(),
            mAdapter(adapter),
            mClient(client),
            mCallbacks(callbacks) {}
        inline virtual void proc() const {
            mAdapter.saveClient(mClient, mCallbacks);
        }
    };

    sendMsg(new MsgAddClient(*this, client, callbacks));
}

void
LocAdapterBase::removeClientCommand(LocationAPI* client,
                                removeClientCompleteCallback rmClientCb)
{
    LOC_LOGD("%s]: client %p", __func__, client);

    struct MsgRemoveClient : public LocMsg {
        LocAdapterBase& mAdapter;
        LocationAPI* mClient;
        removeClientCompleteCallback mRmClientCb;
        inline MsgRemoveClient(LocAdapterBase& adapter,
                               LocationAPI* client,
                               removeClientCompleteCallback rmCb) :
            LocMsg(),
            mAdapter(adapter),
            mClient(client),
            mRmClientCb(rmCb){}
        inline virtual void proc() const {
            mAdapter.stopClientSessions(mClient);
            mAdapter.eraseClient(mClient);
            if (nullptr != mRmClientCb) {
                (mRmClientCb)(mClient);
            }
        }
    };

    sendMsg(new MsgRemoveClient(*this, client, rmClientCb));
}

void
LocAdapterBase::requestCapabilitiesCommand(LocationAPI* client)
{
    LOC_LOGD("%s]: ", __func__);

    struct MsgRequestCapabilities : public LocMsg {
        LocAdapterBase& mAdapter;
        LocationAPI* mClient;
        inline MsgRequestCapabilities(LocAdapterBase& adapter,
                                      LocationAPI* client) :
            LocMsg(),
            mAdapter(adapter),
            mClient(client) {}
        inline virtual void proc() const {
            if (!mAdapter.isEngineCapabilitiesKnown()) {
                mAdapter.mPendingMsgs.push_back(new MsgRequestCapabilities(*this));
                return;
            }
            LocationCallbacks callbacks = mAdapter.getClientCallbacks(mClient);
            if (callbacks.capabilitiesCb != nullptr) {
                callbacks.capabilitiesCb(mAdapter.getCapabilities());
            }
        }
    };

    sendMsg(new MsgRequestCapabilities(*this, client));
}

} // namespace loc_core
