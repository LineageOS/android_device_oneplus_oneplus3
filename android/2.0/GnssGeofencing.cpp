/*
 * Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
 * Not a Contribution
 */
/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "GnssHal_GnssGeofencing"

#include <log_util.h>
#include <GeofenceAPIClient.h>
#include "GnssGeofencing.h"

namespace android {
namespace hardware {
namespace gnss {
namespace V2_0 {
namespace implementation {

void GnssGeofencing::GnssGeofencingDeathRecipient::serviceDied(
        uint64_t cookie, const wp<IBase>& who) {
    LOC_LOGE("%s] service died. cookie: %llu, who: %p",
            __FUNCTION__, static_cast<unsigned long long>(cookie), &who);
    if (mGnssGeofencing != nullptr) {
        mGnssGeofencing->removeAllGeofences();
    }
}

GnssGeofencing::GnssGeofencing() : mApi(nullptr) {
    mGnssGeofencingDeathRecipient = new GnssGeofencingDeathRecipient(this);
}

GnssGeofencing::~GnssGeofencing() {
    if (mApi != nullptr) {
        delete mApi;
        mApi = nullptr;
    }
}

// Methods from ::android::hardware::gnss::V1_0::IGnssGeofencing follow.
Return<void> GnssGeofencing::setCallback(const sp<IGnssGeofenceCallback>& callback)  {
    if (mApi != nullptr) {
        LOC_LOGd("mApi is NOT nullptr");
        return Void();
    }

    mApi = new GeofenceAPIClient(callback);
    if (mApi == nullptr) {
        LOC_LOGE("%s]: failed to create mApi", __FUNCTION__);
    }

    if (mGnssGeofencingCbIface != nullptr) {
        mGnssGeofencingCbIface->unlinkToDeath(mGnssGeofencingDeathRecipient);
    }
    mGnssGeofencingCbIface = callback;
    if (mGnssGeofencingCbIface != nullptr) {
        mGnssGeofencingCbIface->linkToDeath(mGnssGeofencingDeathRecipient, 0 /*cookie*/);
    }

    return Void();
}

Return<void> GnssGeofencing::addGeofence(
        int32_t geofenceId,
        double latitudeDegrees,
        double longitudeDegrees,
        double radiusMeters,
        IGnssGeofenceCallback::GeofenceTransition lastTransition,
        int32_t monitorTransitions,
        uint32_t notificationResponsivenessMs,
        uint32_t unknownTimerMs)  {
    if (mApi == nullptr) {
        LOC_LOGE("%s]: mApi is nullptr", __FUNCTION__);
    } else {
        mApi->geofenceAdd(
                geofenceId,
                latitudeDegrees,
                longitudeDegrees,
                radiusMeters,
                static_cast<int32_t>(lastTransition),
                monitorTransitions,
                notificationResponsivenessMs,
                unknownTimerMs);
    }
    return Void();
}

Return<void> GnssGeofencing::pauseGeofence(int32_t geofenceId)  {
    if (mApi == nullptr) {
        LOC_LOGE("%s]: mApi is nullptr", __FUNCTION__);
    } else {
        mApi->geofencePause(geofenceId);
    }
    return Void();
}

Return<void> GnssGeofencing::resumeGeofence(int32_t geofenceId, int32_t monitorTransitions)  {
    if (mApi == nullptr) {
        LOC_LOGE("%s]: mApi is nullptr", __FUNCTION__);
    } else {
        mApi->geofenceResume(geofenceId, monitorTransitions);
    }
    return Void();
}

Return<void> GnssGeofencing::removeGeofence(int32_t geofenceId)  {
    if (mApi == nullptr) {
        LOC_LOGE("%s]: mApi is nullptr", __FUNCTION__);
    } else {
        mApi->geofenceRemove(geofenceId);
    }
    return Void();
}

Return<void> GnssGeofencing::removeAllGeofences()  {
    if (mApi == nullptr) {
        LOC_LOGD("%s]: mApi is nullptr, do nothing", __FUNCTION__);
    } else {
        mApi->geofenceRemoveAll();
    }
    return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
