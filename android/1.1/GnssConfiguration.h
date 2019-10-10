/*
 * Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 * Not a Contribution
 */

 /* Copyright (C) 2016 The Android Open Source Project
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


#ifndef ANDROID_HARDWARE_GNSS_V1_1_GNSSCONFIGURATION_H
#define ANDROID_HARDWARE_GNSS_V1_1_GNSSCONFIGURATION_H

#include <android/hardware/gnss/1.1/IGnssConfiguration.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V1_1 {
namespace implementation {

using ::android::hardware::gnss::V1_1::IGnssConfiguration;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

/*
 * Interface for passing GNSS configuration info from platform to HAL.
 */
struct Gnss;
struct GnssConfiguration : public IGnssConfiguration {
    GnssConfiguration(Gnss* gnss);
    ~GnssConfiguration() = default;

    /*
     * Methods from ::android::hardware::gnss::V1_0::IGnssConfiguration follow.
     * These declarations were generated from IGnssConfiguration.hal.
     */
    Return<bool> setSuplVersion(uint32_t version) override;
    Return<bool> setSuplMode(uint8_t mode) override;
    Return<bool> setSuplEs(bool enabled) override;
    Return<bool> setLppProfile(uint8_t lppProfile) override;
    Return<bool> setGlonassPositioningProtocol(uint8_t protocol) override;
    Return<bool> setEmergencySuplPdn(bool enable) override;
    Return<bool> setGpsLock(uint8_t lock) override;

    // Methods from ::android::hardware::gnss::V1_1::IGnssConfiguration follow.
    Return<bool> setBlacklist(
            const hidl_vec<GnssConfiguration::BlacklistedSource>& blacklist) override;

 private:
    Gnss* mGnss = nullptr;
    bool setBlacklistedSource(
            GnssSvIdSource& copyToSource,
            const GnssConfiguration::BlacklistedSource& copyFromSource);
};

}  // namespace implementation
}  // namespace V1_1
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_V1_1_GNSSCONFIGURATION_H
