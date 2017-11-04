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

#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_GeofenceApiClient"

#include <log_util.h>
#include <loc_cfg.h>

#include "LocationUtil.h"
#include "GeofenceAPIClient.h"

namespace android {
namespace hardware {
namespace gnss {
namespace V1_0 {
namespace implementation {


GeofenceAPIClient::GeofenceAPIClient(const sp<IGnssGeofenceCallback>& callback) :
    LocationAPIClientBase(),
    mGnssGeofencingCbIface(callback)
{
    LOC_LOGD("%s]: (%p)", __FUNCTION__, &callback);

    LocationCallbacks locationCallbacks;
    memset(&locationCallbacks, 0, sizeof(LocationCallbacks));
    locationCallbacks.size = sizeof(LocationCallbacks);

    locationCallbacks.trackingCb = nullptr;
    locationCallbacks.batchingCb = nullptr;

    locationCallbacks.geofenceBreachCb = nullptr;
    if (mGnssGeofencingCbIface != nullptr) {
        locationCallbacks.geofenceBreachCb =
            [this](GeofenceBreachNotification geofenceBreachNotification) {
                onGeofenceBreachCb(geofenceBreachNotification);
            };

        locationCallbacks.geofenceStatusCb =
            [this](GeofenceStatusNotification geofenceStatusNotification) {
                onGeofenceStatusCb(geofenceStatusNotification);
            };
    }

    locationCallbacks.gnssLocationInfoCb = nullptr;
    locationCallbacks.gnssNiCb = nullptr;
    locationCallbacks.gnssSvCb = nullptr;
    locationCallbacks.gnssNmeaCb = nullptr;
    locationCallbacks.gnssMeasurementsCb = nullptr;

    locAPISetCallbacks(locationCallbacks);
}

void GeofenceAPIClient::geofenceAdd(uint32_t geofence_id, double latitude, double longitude,
        double radius_meters, int32_t last_transition, int32_t monitor_transitions,
        uint32_t notification_responsiveness_ms, uint32_t unknown_timer_ms)
{
    LOC_LOGD("%s]: (%d %f %f %f %d %d %d %d)", __FUNCTION__,
            geofence_id, latitude, longitude, radius_meters,
            last_transition, monitor_transitions, notification_responsiveness_ms, unknown_timer_ms);

    GeofenceOption options;
    memset(&options, 0, sizeof(GeofenceOption));
    options.size = sizeof(GeofenceOption);
    if (monitor_transitions & IGnssGeofenceCallback::GeofenceTransition::ENTERED)
        options.breachTypeMask |= GEOFENCE_BREACH_ENTER_BIT;
    if (monitor_transitions & IGnssGeofenceCallback::GeofenceTransition::EXITED)
        options.breachTypeMask |=  GEOFENCE_BREACH_EXIT_BIT;
    options.responsiveness = notification_responsiveness_ms;

    GeofenceInfo data;
    data.size = sizeof(GeofenceInfo);
    data.latitude = latitude;
    data.longitude = longitude;
    data.radius = radius_meters;

    LocationError err = (LocationError)locAPIAddGeofences(1, &geofence_id, &options, &data);
    if (LOCATION_ERROR_SUCCESS != err) {
        onAddGeofencesCb(1, &err, &geofence_id);
    }
}

void GeofenceAPIClient::geofencePause(uint32_t geofence_id)
{
    LOC_LOGD("%s]: (%d)", __FUNCTION__, geofence_id);
    locAPIPauseGeofences(1, &geofence_id);
}

void GeofenceAPIClient::geofenceResume(uint32_t geofence_id, int32_t monitor_transitions)
{
    LOC_LOGD("%s]: (%d %d)", __FUNCTION__, geofence_id, monitor_transitions);
    GeofenceBreachTypeMask mask = 0;
    if (monitor_transitions & IGnssGeofenceCallback::GeofenceTransition::ENTERED)
        mask |= GEOFENCE_BREACH_ENTER_BIT;
    if (monitor_transitions & IGnssGeofenceCallback::GeofenceTransition::EXITED)
        mask |=  GEOFENCE_BREACH_EXIT_BIT;
    locAPIResumeGeofences(1, &geofence_id, &mask);
}

void GeofenceAPIClient::geofenceRemove(uint32_t geofence_id)
{
    LOC_LOGD("%s]: (%d)", __FUNCTION__, geofence_id);
    locAPIRemoveGeofences(1, &geofence_id);
}

void GeofenceAPIClient::geofenceRemoveAll()
{
    LOC_LOGD("%s]", __FUNCTION__);
    // TODO locAPIRemoveAllGeofences();
}

// callbacks
void GeofenceAPIClient::onGeofenceBreachCb(GeofenceBreachNotification geofenceBreachNotification)
{
    LOC_LOGD("%s]: (%zu)", __FUNCTION__, geofenceBreachNotification.count);
    if (mGnssGeofencingCbIface != nullptr) {
        for (size_t i = 0; i < geofenceBreachNotification.count; i++) {
            GnssLocation gnssLocation;
            convertGnssLocation(geofenceBreachNotification.location, gnssLocation);

            IGnssGeofenceCallback::GeofenceTransition transition;
            if (geofenceBreachNotification.type == GEOFENCE_BREACH_ENTER)
                transition = IGnssGeofenceCallback::GeofenceTransition::ENTERED;
            else if (geofenceBreachNotification.type == GEOFENCE_BREACH_EXIT)
                transition = IGnssGeofenceCallback::GeofenceTransition::EXITED;
            else {
                // continue with other breach if transition is
                // nether GPS_GEOFENCE_ENTERED nor GPS_GEOFENCE_EXITED
                continue;
            }

            auto r = mGnssGeofencingCbIface->gnssGeofenceTransitionCb(
                    geofenceBreachNotification.ids[i], gnssLocation, transition,
                    static_cast<GnssUtcTime>(geofenceBreachNotification.timestamp));
            if (!r.isOk()) {
                LOC_LOGE("%s] Error from gnssGeofenceTransitionCb description=%s",
                    __func__, r.description().c_str());
            }
        }
    }
}

void GeofenceAPIClient::onGeofenceStatusCb(GeofenceStatusNotification geofenceStatusNotification)
{
    LOC_LOGD("%s]: (%d)", __FUNCTION__, geofenceStatusNotification.available);
    if (mGnssGeofencingCbIface != nullptr) {
        IGnssGeofenceCallback::GeofenceAvailability status =
            IGnssGeofenceCallback::GeofenceAvailability::UNAVAILABLE;
        if (geofenceStatusNotification.available == GEOFENCE_STATUS_AVAILABILE_YES) {
            status = IGnssGeofenceCallback::GeofenceAvailability::AVAILABLE;
        }
        GnssLocation gnssLocation;
        memset(&gnssLocation, 0, sizeof(GnssLocation));
        auto r = mGnssGeofencingCbIface->gnssGeofenceStatusCb(status, gnssLocation);
        if (!r.isOk()) {
            LOC_LOGE("%s] Error from gnssGeofenceStatusCb description=%s",
                __func__, r.description().c_str());
        }
    }
}

void GeofenceAPIClient::onAddGeofencesCb(size_t count, LocationError* errors, uint32_t* ids)
{
    LOC_LOGD("%s]: (%zu)", __FUNCTION__, count);
    if (mGnssGeofencingCbIface != nullptr) {
        for (size_t i = 0; i < count; i++) {
            IGnssGeofenceCallback::GeofenceStatus status =
                IGnssGeofenceCallback::GeofenceStatus::ERROR_GENERIC;
            if (errors[i] == LOCATION_ERROR_SUCCESS)
                status = IGnssGeofenceCallback::GeofenceStatus::OPERATION_SUCCESS;
            else if (errors[i] == LOCATION_ERROR_ID_EXISTS)
                status = IGnssGeofenceCallback::GeofenceStatus::ERROR_ID_EXISTS;
            auto r = mGnssGeofencingCbIface->gnssGeofenceAddCb(ids[i], status);
            if (!r.isOk()) {
                LOC_LOGE("%s] Error from gnssGeofenceAddCb description=%s",
                    __func__, r.description().c_str());
            }
        }
    }
}

void GeofenceAPIClient::onRemoveGeofencesCb(size_t count, LocationError* errors, uint32_t* ids)
{
    LOC_LOGD("%s]: (%zu)", __FUNCTION__, count);
    if (mGnssGeofencingCbIface != nullptr) {
        for (size_t i = 0; i < count; i++) {
            IGnssGeofenceCallback::GeofenceStatus status =
                IGnssGeofenceCallback::GeofenceStatus::ERROR_GENERIC;
            if (errors[i] == LOCATION_ERROR_SUCCESS)
                status = IGnssGeofenceCallback::GeofenceStatus::OPERATION_SUCCESS;
            else if (errors[i] == LOCATION_ERROR_ID_UNKNOWN)
                status = IGnssGeofenceCallback::GeofenceStatus::ERROR_ID_UNKNOWN;
            auto r = mGnssGeofencingCbIface->gnssGeofenceRemoveCb(ids[i], status);
            if (!r.isOk()) {
                LOC_LOGE("%s] Error from gnssGeofenceRemoveCb description=%s",
                    __func__, r.description().c_str());
            }
        }
    }
}

void GeofenceAPIClient::onPauseGeofencesCb(size_t count, LocationError* errors, uint32_t* ids)
{
    LOC_LOGD("%s]: (%zu)", __FUNCTION__, count);
    if (mGnssGeofencingCbIface != nullptr) {
        for (size_t i = 0; i < count; i++) {
            IGnssGeofenceCallback::GeofenceStatus status =
                IGnssGeofenceCallback::GeofenceStatus::ERROR_GENERIC;
            if (errors[i] == LOCATION_ERROR_SUCCESS)
                status = IGnssGeofenceCallback::GeofenceStatus::OPERATION_SUCCESS;
            else if (errors[i] == LOCATION_ERROR_ID_UNKNOWN)
                status = IGnssGeofenceCallback::GeofenceStatus::ERROR_ID_UNKNOWN;
            auto r = mGnssGeofencingCbIface->gnssGeofencePauseCb(ids[i], status);
            if (!r.isOk()) {
                LOC_LOGE("%s] Error from gnssGeofencePauseCb description=%s",
                    __func__, r.description().c_str());
            }
        }
    }
}

void GeofenceAPIClient::onResumeGeofencesCb(size_t count, LocationError* errors, uint32_t* ids)
{
    LOC_LOGD("%s]: (%zu)", __FUNCTION__, count);
    if (mGnssGeofencingCbIface != nullptr) {
        for (size_t i = 0; i < count; i++) {
            IGnssGeofenceCallback::GeofenceStatus status =
                IGnssGeofenceCallback::GeofenceStatus::ERROR_GENERIC;
            if (errors[i] == LOCATION_ERROR_SUCCESS)
                status = IGnssGeofenceCallback::GeofenceStatus::OPERATION_SUCCESS;
            else if (errors[i] == LOCATION_ERROR_ID_UNKNOWN)
                status = IGnssGeofenceCallback::GeofenceStatus::ERROR_ID_UNKNOWN;
            auto r = mGnssGeofencingCbIface->gnssGeofenceResumeCb(ids[i], status);
            if (!r.isOk()) {
                LOC_LOGE("%s] Error from gnssGeofenceResumeCb description=%s",
                    __func__, r.description().c_str());
            }
        }
    }
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
