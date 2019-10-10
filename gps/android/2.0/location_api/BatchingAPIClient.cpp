/* Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
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
#define LOG_TAG "LocSvc_BatchingAPIClient"

#include <log_util.h>
#include <loc_cfg.h>

#include "LocationUtil.h"
#include "BatchingAPIClient.h"

#include "limits.h"


namespace android {
namespace hardware {
namespace gnss {
namespace V2_0 {
namespace implementation {

using ::android::hardware::gnss::V2_0::IGnssBatching;
using ::android::hardware::gnss::V2_0::IGnssBatchingCallback;
using ::android::hardware::gnss::V2_0::GnssLocation;

static void convertBatchOption(const IGnssBatching::Options& in, LocationOptions& out,
        LocationCapabilitiesMask mask);

BatchingAPIClient::BatchingAPIClient(const sp<V1_0::IGnssBatchingCallback>& callback) :
    LocationAPIClientBase(),
    mGnssBatchingCbIface(nullptr),
    mDefaultId(UINT_MAX),
    mLocationCapabilitiesMask(0),
    mGnssBatchingCbIface_2_0(nullptr)
{
    LOC_LOGD("%s]: (%p)", __FUNCTION__, &callback);

    gnssUpdateCallbacks(callback);
}

BatchingAPIClient::BatchingAPIClient(const sp<V2_0::IGnssBatchingCallback>& callback) :
    LocationAPIClientBase(),
    mGnssBatchingCbIface(nullptr),
    mDefaultId(UINT_MAX),
    mLocationCapabilitiesMask(0),
    mGnssBatchingCbIface_2_0(nullptr)
{
    LOC_LOGD("%s]: (%p)", __FUNCTION__, &callback);

    gnssUpdateCallbacks_2_0(callback);
}

BatchingAPIClient::~BatchingAPIClient()
{
    LOC_LOGD("%s]: ()", __FUNCTION__);
}

int BatchingAPIClient::getBatchSize()
{
    LOC_LOGD("%s]: ()", __FUNCTION__);
    return locAPIGetBatchSize();
}

void BatchingAPIClient::setCallbacks()
{
    LocationCallbacks locationCallbacks;
    memset(&locationCallbacks, 0, sizeof(LocationCallbacks));
    locationCallbacks.size = sizeof(LocationCallbacks);

    locationCallbacks.trackingCb = nullptr;
    locationCallbacks.batchingCb = nullptr;
    locationCallbacks.batchingCb = [this](size_t count, Location* location,
        BatchingOptions batchOptions) {
        onBatchingCb(count, location, batchOptions);
    };
    locationCallbacks.geofenceBreachCb = nullptr;
    locationCallbacks.geofenceStatusCb = nullptr;
    locationCallbacks.gnssLocationInfoCb = nullptr;
    locationCallbacks.gnssNiCb = nullptr;
    locationCallbacks.gnssSvCb = nullptr;
    locationCallbacks.gnssNmeaCb = nullptr;
    locationCallbacks.gnssMeasurementsCb = nullptr;

    locAPISetCallbacks(locationCallbacks);
}

void BatchingAPIClient::gnssUpdateCallbacks(const sp<V1_0::IGnssBatchingCallback>& callback)
{
    mMutex.lock();
    mGnssBatchingCbIface = callback;
    mMutex.unlock();

    if (mGnssBatchingCbIface != nullptr) {
        setCallbacks();
    }
}

void BatchingAPIClient::gnssUpdateCallbacks_2_0(const sp<V2_0::IGnssBatchingCallback>& callback)
{
    mMutex.lock();
    mGnssBatchingCbIface_2_0 = callback;
    mMutex.unlock();

    if (mGnssBatchingCbIface_2_0 != nullptr) {
        setCallbacks();
    }
}

int BatchingAPIClient::startSession(const IGnssBatching::Options& opts)
{
    LOC_LOGD("%s]: (%lld %d)", __FUNCTION__,
            static_cast<long long>(opts.periodNanos), static_cast<uint8_t>(opts.flags));
    int retVal = -1;
    LocationOptions options;
    convertBatchOption(opts, options, mLocationCapabilitiesMask);
    uint32_t mode = 0;
    if (opts.flags == static_cast<uint8_t>(IGnssBatching::Flag::WAKEUP_ON_FIFO_FULL)) {
        mode = SESSION_MODE_ON_FULL;
    }
    if (locAPIStartSession(mDefaultId, mode, options) == LOCATION_ERROR_SUCCESS) {
        retVal = 1;
    }
    return retVal;
}

int BatchingAPIClient::updateSessionOptions(const IGnssBatching::Options& opts)
{
    LOC_LOGD("%s]: (%lld %d)", __FUNCTION__,
            static_cast<long long>(opts.periodNanos), static_cast<uint8_t>(opts.flags));
    int retVal = -1;
    LocationOptions options;
    convertBatchOption(opts, options, mLocationCapabilitiesMask);

    uint32_t mode = 0;
    if (opts.flags == static_cast<uint8_t>(IGnssBatching::Flag::WAKEUP_ON_FIFO_FULL)) {
        mode = SESSION_MODE_ON_FULL;
    }
    if (locAPIUpdateSessionOptions(mDefaultId, mode, options) == LOCATION_ERROR_SUCCESS) {
        retVal = 1;
    }
    return retVal;
}

int BatchingAPIClient::stopSession()
{
    LOC_LOGD("%s]: ", __FUNCTION__);
    int retVal = -1;
    if (locAPIStopSession(mDefaultId) == LOCATION_ERROR_SUCCESS) {
        retVal = 1;
    }
    return retVal;
}

void BatchingAPIClient::getBatchedLocation(int last_n_locations)
{
    LOC_LOGD("%s]: (%d)", __FUNCTION__, last_n_locations);
    locAPIGetBatchedLocations(mDefaultId, last_n_locations);
}

void BatchingAPIClient::flushBatchedLocations()
{
    LOC_LOGD("%s]: ()", __FUNCTION__);
    locAPIGetBatchedLocations(mDefaultId, SIZE_MAX);
}

void BatchingAPIClient::onCapabilitiesCb(LocationCapabilitiesMask capabilitiesMask)
{
    LOC_LOGD("%s]: (%02x)", __FUNCTION__, capabilitiesMask);
    mLocationCapabilitiesMask = capabilitiesMask;
}

void BatchingAPIClient::onBatchingCb(size_t count, Location* location,
        BatchingOptions /*batchOptions*/)
{
    mMutex.lock();
    auto gnssBatchingCbIface(mGnssBatchingCbIface);
    auto gnssBatchingCbIface_2_0(mGnssBatchingCbIface_2_0);
    mMutex.unlock();

    LOC_LOGD("%s]: (count: %zu)", __FUNCTION__, count);
    if (gnssBatchingCbIface_2_0 != nullptr && count > 0) {
        hidl_vec<V2_0::GnssLocation> locationVec;
        locationVec.resize(count);
        for (size_t i = 0; i < count; i++) {
            convertGnssLocation(location[i], locationVec[i]);
        }
        auto r = gnssBatchingCbIface_2_0->gnssLocationBatchCb(locationVec);
        if (!r.isOk()) {
            LOC_LOGE("%s] Error from gnssLocationBatchCb 2.0 description=%s",
                __func__, r.description().c_str());
        }
    } else if (gnssBatchingCbIface != nullptr && count > 0) {
        hidl_vec<V1_0::GnssLocation> locationVec;
        locationVec.resize(count);
        for (size_t i = 0; i < count; i++) {
            convertGnssLocation(location[i], locationVec[i]);
        }
        auto r = gnssBatchingCbIface->gnssLocationBatchCb(locationVec);
        if (!r.isOk()) {
            LOC_LOGE("%s] Error from gnssLocationBatchCb 1.0 description=%s",
                __func__, r.description().c_str());
        }
    }
}

static void convertBatchOption(const IGnssBatching::Options& in, LocationOptions& out,
        LocationCapabilitiesMask mask)
{
    memset(&out, 0, sizeof(LocationOptions));
    out.size = sizeof(LocationOptions);
    out.minInterval = (uint32_t)(in.periodNanos / 1000000L);
    out.minDistance = 0;
    out.mode = GNSS_SUPL_MODE_STANDALONE;
    if (mask & LOCATION_CAPABILITIES_GNSS_MSA_BIT)
        out.mode = GNSS_SUPL_MODE_MSA;
    if (mask & LOCATION_CAPABILITIES_GNSS_MSB_BIT)
        out.mode = GNSS_SUPL_MODE_MSB;
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
