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

#ifndef GEOFENCE_API_CLINET_H
#define GEOFENCE_API_CLINET_H


#include <android/hardware/gnss/1.0/IGnssGeofenceCallback.h>
#include <LocationAPIClientBase.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V1_0 {
namespace implementation {

using ::android::sp;

class GeofenceAPIClient : public LocationAPIClientBase
{
public:
    GeofenceAPIClient(const sp<IGnssGeofenceCallback>& callback);
    virtual ~GeofenceAPIClient() = default;

    void geofenceAdd(uint32_t geofence_id, double latitude, double longitude,
            double radius_meters, int32_t last_transition, int32_t monitor_transitions,
            uint32_t notification_responsiveness_ms, uint32_t unknown_timer_ms);
    void geofencePause(uint32_t geofence_id);
    void geofenceResume(uint32_t geofence_id, int32_t monitor_transitions);
    void geofenceRemove(uint32_t geofence_id);
    void geofenceRemoveAll();

    // callbacks
    void onGeofenceBreachCb(GeofenceBreachNotification geofenceBreachNotification) final;
    void onGeofenceStatusCb(GeofenceStatusNotification geofenceStatusNotification) final;
    void onAddGeofencesCb(size_t count, LocationError* errors, uint32_t* ids) final;
    void onRemoveGeofencesCb(size_t count, LocationError* errors, uint32_t* ids) final;
    void onPauseGeofencesCb(size_t count, LocationError* errors, uint32_t* ids) final;
    void onResumeGeofencesCb(size_t count, LocationError* errors, uint32_t* ids) final;

private:
    sp<IGnssGeofenceCallback> mGnssGeofencingCbIface;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
#endif // GEOFENCE_API_CLINET_H
