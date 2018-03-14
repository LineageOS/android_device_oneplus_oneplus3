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

#ifndef MEASUREMENT_API_CLINET_H
#define MEASUREMENT_API_CLINET_H

#include <mutex>
#include <android/hardware/gnss/1.0/IGnssMeasurement.h>
#include <android/hardware/gnss/1.0/IGnssMeasurementCallback.h>
#include <LocationAPIClientBase.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V1_0 {
namespace implementation {

using ::android::hardware::gnss::V1_0::IGnssMeasurement;
using ::android::sp;

class MeasurementAPIClient : public LocationAPIClientBase
{
public:
    MeasurementAPIClient();
    virtual ~MeasurementAPIClient();
    MeasurementAPIClient(const MeasurementAPIClient&) = delete;
    MeasurementAPIClient& operator=(const MeasurementAPIClient&) = delete;

    // for GpsMeasurementInterface
    Return<IGnssMeasurement::GnssMeasurementStatus> measurementSetCallback(
            const sp<IGnssMeasurementCallback>& callback);
    void measurementClose();

    // callbacks we are interested in
    void onGnssMeasurementsCb(GnssMeasurementsNotification gnssMeasurementsNotification) final;

private:
    sp<IGnssMeasurementCallback> mGnssMeasurementCbIface;
    std::mutex mMutex;
    bool mTracking;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
#endif // MEASUREMENT_API_CLINET_H
