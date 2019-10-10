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

#ifndef ANDROID_HARDWARE_GNSS_V2_0_GNSSMEASUREMENT_H
#define ANDROID_HARDWARE_GNSS_V2_0_GNSSMEASUREMENT_H

#include <android/hardware/gnss/2.0/IGnssMeasurement.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V2_0 {
namespace implementation {

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

class MeasurementAPIClient;
struct GnssMeasurement : public V2_0::IGnssMeasurement {
    GnssMeasurement();
    ~GnssMeasurement();

    /*
     * Methods from ::android::hardware::gnss::V1_0::IGnssMeasurement follow.
     * These declarations were generated from IGnssMeasurement.hal.
     */
    Return<GnssMeasurement::GnssMeasurementStatus> setCallback(
        const sp<V1_0::IGnssMeasurementCallback>& callback) override;
    Return<void> close() override;

    // Methods from ::android::hardware::gnss::V1_1::IGnssMeasurement follow.
    Return<GnssMeasurement::GnssMeasurementStatus> setCallback_1_1(
            const sp<V1_1::IGnssMeasurementCallback>& callback,
            bool enableFullTracking) override;

    // Methods from ::android::hardware::gnss::V2_0::IGnssMeasurement follow.
    Return<GnssMeasurement::GnssMeasurementStatus> setCallback_2_0(
            const sp<V2_0::IGnssMeasurementCallback>& callback,
            bool enableFullTracking) override;
 private:
    struct GnssMeasurementDeathRecipient : hidl_death_recipient {
        GnssMeasurementDeathRecipient(sp<GnssMeasurement> gnssMeasurement) :
            mGnssMeasurement(gnssMeasurement) {
        }
        ~GnssMeasurementDeathRecipient() = default;
        virtual void serviceDied(uint64_t cookie, const wp<IBase>& who) override;
        sp<GnssMeasurement> mGnssMeasurement;
    };

 private:
    sp<GnssMeasurementDeathRecipient> mGnssMeasurementDeathRecipient = nullptr;
    sp<V1_0::IGnssMeasurementCallback> mGnssMeasurementCbIface = nullptr;
    sp<V1_1::IGnssMeasurementCallback> mGnssMeasurementCbIface_1_1 = nullptr;
    sp<V2_0::IGnssMeasurementCallback> mGnssMeasurementCbIface_2_0 = nullptr;
    MeasurementAPIClient* mApi;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_V2_0_GNSSMEASUREMENT_H
