/* Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
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
#define LOG_TAG "LocSvc_GnssAPIClient"

#include <log_util.h>
#include <loc_cfg.h>

#include "LocationUtil.h"
#include "GnssAPIClient.h"
#include <LocDualContext.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V1_0 {
namespace implementation {

using ::android::hardware::gnss::V1_0::IGnss;
using ::android::hardware::gnss::V1_0::IGnssCallback;
using ::android::hardware::gnss::V1_0::IGnssNiCallback;
using ::android::hardware::gnss::V1_0::GnssLocation;

static void convertGnssSvStatus(GnssSvNotification& in, IGnssCallback::GnssSvStatus& out);

GnssAPIClient::GnssAPIClient(const sp<IGnssCallback>& gpsCb,
    const sp<IGnssNiCallback>& niCb) :
    LocationAPIClientBase(),
    mGnssCbIface(nullptr),
    mGnssNiCbIface(nullptr),
    mControlClient(new LocationAPIControlClient()),
    mLocationCapabilitiesMask(0),
    mLocationCapabilitiesCached(false)
{
    LOC_LOGD("%s]: (%p %p)", __FUNCTION__, &gpsCb, &niCb);

    // set default LocationOptions.
    memset(&mLocationOptions, 0, sizeof(LocationOptions));
    mLocationOptions.size = sizeof(LocationOptions);
    mLocationOptions.minInterval = 1000;
    mLocationOptions.minDistance = 0;
    mLocationOptions.mode = GNSS_SUPL_MODE_STANDALONE;

    gnssUpdateCallbacks(gpsCb, niCb);
}

GnssAPIClient::~GnssAPIClient()
{
    LOC_LOGD("%s]: ()", __FUNCTION__);
    if (mControlClient) {
        delete mControlClient;
        mControlClient = nullptr;
    }
}

// for GpsInterface
void GnssAPIClient::gnssUpdateCallbacks(const sp<IGnssCallback>& gpsCb,
    const sp<IGnssNiCallback>& niCb)
{
    LOC_LOGD("%s]: (%p %p)", __FUNCTION__, &gpsCb, &niCb);

    mMutex.lock();
    mGnssCbIface = gpsCb;
    mGnssNiCbIface = niCb;
    mMutex.unlock();

    LocationCallbacks locationCallbacks;
    memset(&locationCallbacks, 0, sizeof(LocationCallbacks));
    locationCallbacks.size = sizeof(LocationCallbacks);

    locationCallbacks.trackingCb = nullptr;
    if (mGnssCbIface != nullptr) {
        locationCallbacks.trackingCb = [this](Location location) {
            onTrackingCb(location);
        };
    }

    locationCallbacks.batchingCb = nullptr;
    locationCallbacks.geofenceBreachCb = nullptr;
    locationCallbacks.geofenceStatusCb = nullptr;
    locationCallbacks.gnssLocationInfoCb = nullptr;

    locationCallbacks.gnssNiCb = nullptr;
    loc_core::ContextBase* context =
            loc_core::LocDualContext::getLocFgContext(
                    NULL, NULL,
                    loc_core::LocDualContext::mLocationHalName, false);
    if (mGnssNiCbIface != nullptr && !context->hasAgpsExtendedCapabilities()) {
        LOC_LOGD("Registering NI CB");
        locationCallbacks.gnssNiCb = [this](uint32_t id, GnssNiNotification gnssNiNotification) {
            onGnssNiCb(id, gnssNiNotification);
        };
    }

    locationCallbacks.gnssSvCb = nullptr;
    if (mGnssCbIface != nullptr) {
        locationCallbacks.gnssSvCb = [this](GnssSvNotification gnssSvNotification) {
            onGnssSvCb(gnssSvNotification);
        };
    }

    locationCallbacks.gnssNmeaCb = nullptr;
    if (mGnssCbIface != nullptr) {
        locationCallbacks.gnssNmeaCb = [this](GnssNmeaNotification gnssNmeaNotification) {
            onGnssNmeaCb(gnssNmeaNotification);
        };
    }

    locationCallbacks.gnssMeasurementsCb = nullptr;

    locAPISetCallbacks(locationCallbacks);
}

bool GnssAPIClient::gnssStart()
{
    LOC_LOGD("%s]: ()", __FUNCTION__);
    bool retVal = true;
    locAPIStartTracking(mLocationOptions);
    return retVal;
}

bool GnssAPIClient::gnssStop()
{
    LOC_LOGD("%s]: ()", __FUNCTION__);
    bool retVal = true;
    locAPIStopTracking();
    return retVal;
}

bool GnssAPIClient::gnssSetPositionMode(IGnss::GnssPositionMode mode,
        IGnss::GnssPositionRecurrence recurrence, uint32_t minIntervalMs,
        uint32_t preferredAccuracyMeters, uint32_t preferredTimeMs)
{
    LOC_LOGD("%s]: (%d %d %d %d %d)", __FUNCTION__,
            (int)mode, recurrence, minIntervalMs, preferredAccuracyMeters, preferredTimeMs);
    bool retVal = true;
    memset(&mLocationOptions, 0, sizeof(LocationOptions));
    mLocationOptions.size = sizeof(LocationOptions);
    mLocationOptions.minInterval = minIntervalMs;
    mLocationOptions.minDistance = preferredAccuracyMeters;
    if (IGnss::GnssPositionRecurrence::RECURRENCE_SINGLE == recurrence) {
        mLocationOptions.minInterval =
                std::numeric_limits<decltype(mLocationOptions.minInterval)>::max();
    }
    if (mode == IGnss::GnssPositionMode::STANDALONE)
        mLocationOptions.mode = GNSS_SUPL_MODE_STANDALONE;
    else if (mode == IGnss::GnssPositionMode::MS_BASED)
        mLocationOptions.mode = GNSS_SUPL_MODE_MSB;
    else if (mode ==  IGnss::GnssPositionMode::MS_ASSISTED)
        mLocationOptions.mode = GNSS_SUPL_MODE_MSA;
    else {
        LOC_LOGD("%s]: invalid GnssPositionMode: %d", __FUNCTION__, (int)mode);
        retVal = false;
    }
    locAPIUpdateTrackingOptions(mLocationOptions);
    return retVal;
}

// for GpsNiInterface
void GnssAPIClient::gnssNiRespond(int32_t notifId,
        IGnssNiCallback::GnssUserResponseType userResponse)
{
    LOC_LOGD("%s]: (%d %d)", __FUNCTION__, notifId, static_cast<int>(userResponse));
    GnssNiResponse data = GNSS_NI_RESPONSE_IGNORE;
    if (userResponse == IGnssNiCallback::GnssUserResponseType::RESPONSE_ACCEPT)
        data = GNSS_NI_RESPONSE_ACCEPT;
    else if (userResponse == IGnssNiCallback::GnssUserResponseType::RESPONSE_DENY)
        data = GNSS_NI_RESPONSE_DENY;
    else if (userResponse == IGnssNiCallback::GnssUserResponseType::RESPONSE_NORESP)
        data = GNSS_NI_RESPONSE_NO_RESPONSE;
    else {
        LOC_LOGD("%s]: invalid GnssUserResponseType: %d", __FUNCTION__, (int)userResponse);
        return;
    }
    locAPIGnssNiResponse(notifId, data);
}

// these apis using LocationAPIControlClient
void GnssAPIClient::gnssDeleteAidingData(IGnss::GnssAidingData aidingDataFlags)
{
    LOC_LOGD("%s]: (%02hx)", __FUNCTION__, aidingDataFlags);
    if (mControlClient == nullptr) {
        return;
    }
    GnssAidingData data;
    memset(&data, 0, sizeof (GnssAidingData));
    data.sv.svTypeMask = GNSS_AIDING_DATA_SV_TYPE_GPS_BIT |
        GNSS_AIDING_DATA_SV_TYPE_GLONASS_BIT |
        GNSS_AIDING_DATA_SV_TYPE_QZSS_BIT |
        GNSS_AIDING_DATA_SV_TYPE_BEIDOU_BIT |
        GNSS_AIDING_DATA_SV_TYPE_GALILEO_BIT;

    if (aidingDataFlags == IGnss::GnssAidingData::DELETE_ALL)
        data.deleteAll = true;
    else {
        if (aidingDataFlags & IGnss::GnssAidingData::DELETE_EPHEMERIS)
            data.sv.svMask |= GNSS_AIDING_DATA_SV_EPHEMERIS_BIT;
        if (aidingDataFlags & IGnss::GnssAidingData::DELETE_ALMANAC)
            data.sv.svMask |= GNSS_AIDING_DATA_SV_ALMANAC_BIT;
        if (aidingDataFlags & IGnss::GnssAidingData::DELETE_POSITION)
            data.common.mask |= GNSS_AIDING_DATA_COMMON_POSITION_BIT;
        if (aidingDataFlags & IGnss::GnssAidingData::DELETE_TIME)
            data.common.mask |= GNSS_AIDING_DATA_COMMON_TIME_BIT;
        if (aidingDataFlags & IGnss::GnssAidingData::DELETE_IONO)
            data.sv.svMask |= GNSS_AIDING_DATA_SV_IONOSPHERE_BIT;
        if (aidingDataFlags & IGnss::GnssAidingData::DELETE_UTC)
            data.common.mask |= GNSS_AIDING_DATA_COMMON_UTC_BIT;
        if (aidingDataFlags & IGnss::GnssAidingData::DELETE_HEALTH)
            data.sv.svMask |= GNSS_AIDING_DATA_SV_HEALTH_BIT;
        if (aidingDataFlags & IGnss::GnssAidingData::DELETE_SVDIR)
            data.sv.svMask |= GNSS_AIDING_DATA_SV_DIRECTION_BIT;
        if (aidingDataFlags & IGnss::GnssAidingData::DELETE_SVSTEER)
            data.sv.svMask |= GNSS_AIDING_DATA_SV_STEER_BIT;
        if (aidingDataFlags & IGnss::GnssAidingData::DELETE_SADATA)
            data.sv.svMask |= GNSS_AIDING_DATA_SV_SA_DATA_BIT;
        if (aidingDataFlags & IGnss::GnssAidingData::DELETE_RTI)
            data.common.mask |= GNSS_AIDING_DATA_COMMON_RTI_BIT;
        if (aidingDataFlags & IGnss::GnssAidingData::DELETE_CELLDB_INFO)
            data.common.mask |= GNSS_AIDING_DATA_COMMON_CELLDB_BIT;
    }
    mControlClient->locAPIGnssDeleteAidingData(data);
}

void GnssAPIClient::gnssEnable(LocationTechnologyType techType)
{
    LOC_LOGD("%s]: (%0d)", __FUNCTION__, techType);
    if (mControlClient == nullptr) {
        return;
    }
    mControlClient->locAPIEnable(techType);
}

void GnssAPIClient::gnssDisable()
{
    LOC_LOGD("%s]: ()", __FUNCTION__);
    if (mControlClient == nullptr) {
        return;
    }
    mControlClient->locAPIDisable();
}

void GnssAPIClient::gnssConfigurationUpdate(const GnssConfig& gnssConfig)
{
    LOC_LOGD("%s]: (%02x)", __FUNCTION__, gnssConfig.flags);
    if (mControlClient == nullptr) {
        return;
    }
    mControlClient->locAPIGnssUpdateConfig(gnssConfig);
}

void GnssAPIClient::requestCapabilities() {
    // only send capablities if it's already cached, otherwise the first time LocationAPI
    // is initialized, capabilities will be sent by LocationAPI
    if (mLocationCapabilitiesCached) {
        onCapabilitiesCb(mLocationCapabilitiesMask);
    }
}

// callbacks
void GnssAPIClient::onCapabilitiesCb(LocationCapabilitiesMask capabilitiesMask)
{
    LOC_LOGD("%s]: (%02x)", __FUNCTION__, capabilitiesMask);
    mLocationCapabilitiesMask = capabilitiesMask;
    mLocationCapabilitiesCached = true;

    mMutex.lock();
    auto gnssCbIface(mGnssCbIface);
    mMutex.unlock();

    if (gnssCbIface != nullptr) {
        uint32_t data = 0;
        if ((capabilitiesMask & LOCATION_CAPABILITIES_TIME_BASED_TRACKING_BIT) ||
                (capabilitiesMask & LOCATION_CAPABILITIES_TIME_BASED_BATCHING_BIT) ||
                (capabilitiesMask & LOCATION_CAPABILITIES_DISTANCE_BASED_TRACKING_BIT) ||
                (capabilitiesMask & LOCATION_CAPABILITIES_DISTANCE_BASED_BATCHING_BIT))
            data |= IGnssCallback::Capabilities::SCHEDULING;
        if (capabilitiesMask & LOCATION_CAPABILITIES_GEOFENCE_BIT)
            data |= IGnssCallback::Capabilities::GEOFENCING;
        if (capabilitiesMask & LOCATION_CAPABILITIES_GNSS_MEASUREMENTS_BIT)
            data |= IGnssCallback::Capabilities::MEASUREMENTS;
        if (capabilitiesMask & LOCATION_CAPABILITIES_GNSS_MSB_BIT)
            data |= IGnssCallback::Capabilities::MSB;
        if (capabilitiesMask & LOCATION_CAPABILITIES_GNSS_MSA_BIT)
            data |= IGnssCallback::Capabilities::MSA;
        auto r = gnssCbIface->gnssSetCapabilitesCb(data);
        if (!r.isOk()) {
            LOC_LOGE("%s] Error from gnssSetCapabilitesCb description=%s",
                __func__, r.description().c_str());
        }
    }
    if (gnssCbIface != nullptr) {
        IGnssCallback::GnssSystemInfo gnssInfo;
        if (capabilitiesMask & LOCATION_CAPABILITIES_DEBUG_NMEA_BIT) {
            gnssInfo.yearOfHw = 2017;
        } else if (capabilitiesMask & LOCATION_CAPABILITIES_GNSS_MEASUREMENTS_BIT) {
            gnssInfo.yearOfHw = 2016;
        } else {
            gnssInfo.yearOfHw = 2015;
        }
        LOC_LOGV("%s:%d] set_system_info_cb (%d)", __FUNCTION__, __LINE__, gnssInfo.yearOfHw);
        auto r = gnssCbIface->gnssSetSystemInfoCb(gnssInfo);
        if (!r.isOk()) {
            LOC_LOGE("%s] Error from gnssSetSystemInfoCb description=%s",
                __func__, r.description().c_str());
        }
    }
}

void GnssAPIClient::onTrackingCb(Location location)
{
    LOC_LOGD("%s]: (flags: %02x)", __FUNCTION__, location.flags);
    mMutex.lock();
    auto gnssCbIface(mGnssCbIface);
    mMutex.unlock();

    if (gnssCbIface != nullptr) {
        GnssLocation gnssLocation;
        convertGnssLocation(location, gnssLocation);
        auto r = gnssCbIface->gnssLocationCb(gnssLocation);
        if (!r.isOk()) {
            LOC_LOGE("%s] Error from gnssLocationCb description=%s",
                __func__, r.description().c_str());
        }
    }
}

void GnssAPIClient::onGnssNiCb(uint32_t id, GnssNiNotification gnssNiNotification)
{
    LOC_LOGD("%s]: (id: %d)", __FUNCTION__, id);
    mMutex.lock();
    auto gnssNiCbIface(mGnssNiCbIface);
    mMutex.unlock();

    if (gnssNiCbIface == nullptr) {
        LOC_LOGE("%s]: mGnssNiCbIface is nullptr", __FUNCTION__);
        return;
    }

    IGnssNiCallback::GnssNiNotification notificationGnss = {};

    notificationGnss.notificationId = id;

    if (gnssNiNotification.type == GNSS_NI_TYPE_VOICE)
        notificationGnss.niType = IGnssNiCallback::GnssNiType::VOICE;
    else if (gnssNiNotification.type == GNSS_NI_TYPE_SUPL)
        notificationGnss.niType = IGnssNiCallback::GnssNiType::UMTS_SUPL;
    else if (gnssNiNotification.type == GNSS_NI_TYPE_CONTROL_PLANE)
        notificationGnss.niType = IGnssNiCallback::GnssNiType::UMTS_CTRL_PLANE;
    else if (gnssNiNotification.type == GNSS_NI_TYPE_EMERGENCY_SUPL)
        notificationGnss.niType = IGnssNiCallback::GnssNiType::EMERGENCY_SUPL;

    if (gnssNiNotification.options & GNSS_NI_OPTIONS_NOTIFICATION_BIT)
        notificationGnss.notifyFlags |= IGnssNiCallback::GnssNiNotifyFlags::NEED_NOTIFY;
    if (gnssNiNotification.options & GNSS_NI_OPTIONS_VERIFICATION_BIT)
        notificationGnss.notifyFlags |= IGnssNiCallback::GnssNiNotifyFlags::NEED_VERIFY;
    if (gnssNiNotification.options & GNSS_NI_OPTIONS_PRIVACY_OVERRIDE_BIT)
        notificationGnss.notifyFlags |= IGnssNiCallback::GnssNiNotifyFlags::PRIVACY_OVERRIDE;

    notificationGnss.timeoutSec = gnssNiNotification.timeout;

    if (gnssNiNotification.timeoutResponse == GNSS_NI_RESPONSE_ACCEPT)
        notificationGnss.defaultResponse = IGnssNiCallback::GnssUserResponseType::RESPONSE_ACCEPT;
    else if (gnssNiNotification.timeoutResponse == GNSS_NI_RESPONSE_DENY)
        notificationGnss.defaultResponse = IGnssNiCallback::GnssUserResponseType::RESPONSE_DENY;
    else if (gnssNiNotification.timeoutResponse == GNSS_NI_RESPONSE_NO_RESPONSE ||
            gnssNiNotification.timeoutResponse == GNSS_NI_RESPONSE_IGNORE)
        notificationGnss.defaultResponse = IGnssNiCallback::GnssUserResponseType::RESPONSE_NORESP;

    notificationGnss.requestorId = gnssNiNotification.requestor;

    notificationGnss.notificationMessage = gnssNiNotification.message;

    if (gnssNiNotification.requestorEncoding == GNSS_NI_ENCODING_TYPE_NONE)
        notificationGnss.requestorIdEncoding =
            IGnssNiCallback::GnssNiEncodingType::ENC_NONE;
    else if (gnssNiNotification.requestorEncoding == GNSS_NI_ENCODING_TYPE_GSM_DEFAULT)
        notificationGnss.requestorIdEncoding =
            IGnssNiCallback::GnssNiEncodingType::ENC_SUPL_GSM_DEFAULT;
    else if (gnssNiNotification.requestorEncoding == GNSS_NI_ENCODING_TYPE_UTF8)
        notificationGnss.requestorIdEncoding =
            IGnssNiCallback::GnssNiEncodingType::ENC_SUPL_UTF8;
    else if (gnssNiNotification.requestorEncoding == GNSS_NI_ENCODING_TYPE_UCS2)
        notificationGnss.requestorIdEncoding =
            IGnssNiCallback::GnssNiEncodingType::ENC_SUPL_UCS2;

    if (gnssNiNotification.messageEncoding == GNSS_NI_ENCODING_TYPE_NONE)
        notificationGnss.notificationIdEncoding =
            IGnssNiCallback::GnssNiEncodingType::ENC_NONE;
    else if (gnssNiNotification.messageEncoding == GNSS_NI_ENCODING_TYPE_GSM_DEFAULT)
        notificationGnss.notificationIdEncoding =
            IGnssNiCallback::GnssNiEncodingType::ENC_SUPL_GSM_DEFAULT;
    else if (gnssNiNotification.messageEncoding == GNSS_NI_ENCODING_TYPE_UTF8)
        notificationGnss.notificationIdEncoding =
            IGnssNiCallback::GnssNiEncodingType::ENC_SUPL_UTF8;
    else if (gnssNiNotification.messageEncoding == GNSS_NI_ENCODING_TYPE_UCS2)
        notificationGnss.notificationIdEncoding =
            IGnssNiCallback::GnssNiEncodingType::ENC_SUPL_UCS2;

    gnssNiCbIface->niNotifyCb(notificationGnss);
}

void GnssAPIClient::onGnssSvCb(GnssSvNotification gnssSvNotification)
{
    LOC_LOGD("%s]: (count: %zu)", __FUNCTION__, gnssSvNotification.count);
    mMutex.lock();
    auto gnssCbIface(mGnssCbIface);
    mMutex.unlock();

    if (gnssCbIface != nullptr) {
        IGnssCallback::GnssSvStatus svStatus;
        convertGnssSvStatus(gnssSvNotification, svStatus);
        auto r = gnssCbIface->gnssSvStatusCb(svStatus);
        if (!r.isOk()) {
            LOC_LOGE("%s] Error from gnssSvStatusCb description=%s",
                __func__, r.description().c_str());
        }
    }
}

void GnssAPIClient::onGnssNmeaCb(GnssNmeaNotification gnssNmeaNotification)
{
    mMutex.lock();
    auto gnssCbIface(mGnssCbIface);
    mMutex.unlock();

    if (gnssCbIface != nullptr) {
        android::hardware::hidl_string nmeaString;
        nmeaString.setToExternal(gnssNmeaNotification.nmea, gnssNmeaNotification.length);
        auto r = gnssCbIface->gnssNmeaCb(
            static_cast<V1_0::GnssUtcTime>(gnssNmeaNotification.timestamp), nmeaString);
        if (!r.isOk()) {
            LOC_LOGE("%s] Error from gnssNmeaCb nmea=%s length=%zu description=%s", __func__,
                gnssNmeaNotification.nmea, gnssNmeaNotification.length, r.description().c_str());
        }
    }
}

void GnssAPIClient::onStartTrackingCb(LocationError error)
{
    LOC_LOGD("%s]: (%d)", __FUNCTION__, error);
    mMutex.lock();
    auto gnssCbIface(mGnssCbIface);
    mMutex.unlock();

    if (error == LOCATION_ERROR_SUCCESS && gnssCbIface != nullptr) {
        auto r = gnssCbIface->gnssStatusCb(IGnssCallback::GnssStatusValue::ENGINE_ON);
        if (!r.isOk()) {
            LOC_LOGE("%s] Error from gnssStatusCb ENGINE_ON description=%s",
                __func__, r.description().c_str());
        }
        r = gnssCbIface->gnssStatusCb(IGnssCallback::GnssStatusValue::SESSION_BEGIN);
        if (!r.isOk()) {
            LOC_LOGE("%s] Error from gnssStatusCb SESSION_BEGIN description=%s",
                __func__, r.description().c_str());
        }
    }
}

void GnssAPIClient::onStopTrackingCb(LocationError error)
{
    LOC_LOGD("%s]: (%d)", __FUNCTION__, error);
    mMutex.lock();
    auto gnssCbIface(mGnssCbIface);
    mMutex.unlock();

    if (error == LOCATION_ERROR_SUCCESS && gnssCbIface != nullptr) {
        auto r = gnssCbIface->gnssStatusCb(IGnssCallback::GnssStatusValue::SESSION_END);
        if (!r.isOk()) {
            LOC_LOGE("%s] Error from gnssStatusCb SESSION_END description=%s",
                __func__, r.description().c_str());
        }
        r = gnssCbIface->gnssStatusCb(IGnssCallback::GnssStatusValue::ENGINE_OFF);
        if (!r.isOk()) {
            LOC_LOGE("%s] Error from gnssStatusCb ENGINE_OFF description=%s",
                __func__, r.description().c_str());
        }
    }
}

static void convertGnssSvStatus(GnssSvNotification& in, IGnssCallback::GnssSvStatus& out)
{
    memset(&out, 0, sizeof(IGnssCallback::GnssSvStatus));
    out.numSvs = in.count;
    if (out.numSvs > static_cast<uint32_t>(V1_0::GnssMax::SVS_COUNT)) {
        LOC_LOGW("%s]: Too many satellites %u. Clamps to %d.",
                __FUNCTION__,  out.numSvs, V1_0::GnssMax::SVS_COUNT);
        out.numSvs = static_cast<uint32_t>(V1_0::GnssMax::SVS_COUNT);
    }
    for (size_t i = 0; i < out.numSvs; i++) {
        IGnssCallback::GnssSvInfo& info = out.gnssSvList[i];
        info.svid = in.gnssSvs[i].svId;
        convertGnssConstellationType(in.gnssSvs[i].type, info.constellation);
        info.cN0Dbhz = in.gnssSvs[i].cN0Dbhz;
        info.elevationDegrees = in.gnssSvs[i].elevation;
        info.azimuthDegrees = in.gnssSvs[i].azimuth;
        info.svFlag = static_cast<uint8_t>(IGnssCallback::GnssSvFlags::NONE);
        if (in.gnssSvs[i].gnssSvOptionsMask & GNSS_SV_OPTIONS_HAS_EPHEMER_BIT)
            info.svFlag |= IGnssCallback::GnssSvFlags::HAS_EPHEMERIS_DATA;
        if (in.gnssSvs[i].gnssSvOptionsMask & GNSS_SV_OPTIONS_HAS_ALMANAC_BIT)
            info.svFlag |= IGnssCallback::GnssSvFlags::HAS_ALMANAC_DATA;
        if (in.gnssSvs[i].gnssSvOptionsMask & GNSS_SV_OPTIONS_USED_IN_FIX_BIT)
            info.svFlag |= IGnssCallback::GnssSvFlags::USED_IN_FIX;
    }
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
