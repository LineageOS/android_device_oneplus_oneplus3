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

#ifndef ANDROID_HARDWARE_GNSS_V2_0_AGNSS_H
#define ANDROID_HARDWARE_GNSS_V2_0_AGNSS_H

#include <android/hardware/gnss/2.0/IAGnss.h>
#include <hidl/Status.h>
#include <gps_extended_c.h>

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

struct Gnss;
struct AGnss : public V2_0::IAGnss {

    AGnss(Gnss* gnss);
    ~AGnss();
    /*
     * Methods from ::android::hardware::gnss::V2_0::IAGnss interface follow.
     * These declarations were generated from IAGnss.hal.
     */
    Return<void> setCallback(const sp<V2_0::IAGnssCallback>& callback) override;

    Return<bool> dataConnClosed() override;

    Return<bool> dataConnFailed() override;

    Return<bool> dataConnOpen(uint64_t networkHandle, const hidl_string& apn,
            V2_0::IAGnss::ApnIpType apnIpType) override;

    Return<bool> setServer(V2_0::IAGnssCallback::AGnssType type,
                         const hidl_string& hostname, int32_t port) override;

    void statusCb(AGpsExtType type, LocAGpsStatusValue status);

    /* Data call setup callback passed down to GNSS HAL implementation */
    static void agnssStatusIpV4Cb(AGnssExtStatusIpV4 status);

 private:
    Gnss* mGnss = nullptr;
    sp<IAGnssCallback> mAGnssCbIface = nullptr;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_V2_0_AGNSS_H
