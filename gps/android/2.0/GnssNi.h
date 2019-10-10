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

#ifndef ANDROID_HARDWARE_GNSS_V2_0_GNSSNI_H
#define ANDROID_HARDWARE_GNSS_V2_0_GNSSNI_H

#include <android/hardware/gnss/1.0/IGnssNi.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V2_0 {
namespace implementation {

using ::android::hardware::gnss::V1_0::IGnssNi;
using ::android::hardware::gnss::V1_0::IGnssNiCallback;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct Gnss;
struct GnssNi : public IGnssNi {
    GnssNi(Gnss* gnss);
    ~GnssNi() = default;

    /*
     * Methods from ::android::hardware::gnss::V1_0::IGnssNi follow.
     * These declarations were generated from IGnssNi.hal.
     */
    Return<void> setCallback(const sp<IGnssNiCallback>& callback) override;
    Return<void> respond(int32_t notifId,
                         IGnssNiCallback::GnssUserResponseType userResponse) override;

 private:
    struct GnssNiDeathRecipient : hidl_death_recipient {
        GnssNiDeathRecipient(sp<GnssNi> gnssNi) : mGnssNi(gnssNi) {
        }
        ~GnssNiDeathRecipient() = default;
        virtual void serviceDied(uint64_t cookie, const wp<IBase>& who) override;
        sp<GnssNi> mGnssNi;
    };

 private:
    sp<GnssNiDeathRecipient> mGnssNiDeathRecipient = nullptr;
    sp<IGnssNiCallback> mGnssNiCbIface = nullptr;
    Gnss* mGnss = nullptr;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_V2_0_GNSSNI_H
