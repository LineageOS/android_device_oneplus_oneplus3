/*
 * Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
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

#ifndef ANDROID_HARDWARE_GNSS_V1_0_GNSSBATCHING_H
#define ANDROID_HARDWARE_GNSS_V1_0_GNSSBATCHING_H

#include <android/hardware/gnss/1.0/IGnssBatching.h>
#include <hidl/Status.h>


namespace android {
namespace hardware {
namespace gnss {
namespace V1_0 {
namespace implementation {

using ::android::hardware::gnss::V1_0::IGnssBatching;
using ::android::hardware::gnss::V1_0::IGnssBatchingCallback;
using ::android::hidl::base::V1_0::IBase;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

class BatchingAPIClient;
struct GnssBatching : public IGnssBatching {
    GnssBatching();
    ~GnssBatching();

    // Methods from ::android::hardware::gnss::V1_0::IGnssBatching follow.
    Return<bool> init(const sp<IGnssBatchingCallback>& callback) override;
    Return<uint16_t> getBatchSize() override;
    Return<bool> start(const IGnssBatching::Options& options ) override;
    Return<void> flush() override;
    Return<bool> stop() override;
    Return<void> cleanup() override;

 private:
    struct GnssBatchingDeathRecipient : hidl_death_recipient {
        GnssBatchingDeathRecipient(sp<GnssBatching> gnssBatching) :
            mGnssBatching(gnssBatching) {
        }
        ~GnssBatchingDeathRecipient() = default;
        virtual void serviceDied(uint64_t cookie, const wp<IBase>& who) override;
        sp<GnssBatching> mGnssBatching;
    };

 private:
    sp<GnssBatchingDeathRecipient> mGnssBatchingDeathRecipient = nullptr;
    sp<IGnssBatchingCallback> mGnssBatchingCbIface = nullptr;
    BatchingAPIClient* mApi = nullptr;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_V1_0_GNSSBATCHING_H
