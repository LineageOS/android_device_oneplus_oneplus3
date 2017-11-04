/*
 * Copyright (c) 2017, The Linux Foundation. All rights reserved.
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

#ifndef ANDROID_HARDWARE_GNSS_V1_0_AGNSSRIL_H_
#define ANDROID_HARDWARE_GNSS_V1_0_AGNSSRIL_H_

#include <android/hardware/gnss/1.0/IAGnssRil.h>
#include <hidl/Status.h>
#include <location_interface.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V1_0 {
namespace implementation {

using ::android::hardware::gnss::V1_0::IAGnssRil;
using ::android::hardware::gnss::V1_0::IAGnssRilCallback;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct Gnss;
/*
 * Extended interface for AGNSS RIL support. An Assisted GNSS Radio Interface Layer interface
 * allows the GNSS chipset to request radio interface layer information from Android platform.
 * Examples of such information are reference location, unique subscriber ID, phone number string
 * and network availability changes. Also contains wrapper methods to allow methods from
 * IAGnssiRilCallback interface to be passed into the conventional implementation of the GNSS HAL.
 */
struct AGnssRil : public IAGnssRil {
    AGnssRil(Gnss* gnss);
    ~AGnssRil();

    /*
     * Methods from ::android::hardware::gnss::V1_0::IAGnssRil follow.
     * These declarations were generated from IAGnssRil.hal.
     */
    Return<void> setCallback(const sp<IAGnssRilCallback>& /*callback*/) override {
        return Void();
    }
    Return<void> setRefLocation(const IAGnssRil::AGnssRefLocation& /*agnssReflocation*/) override {
        return Void();
    }
    Return<bool> setSetId(IAGnssRil::SetIDType /*type*/, const hidl_string& /*setid*/) override {
        return false;
    }
    Return<bool> updateNetworkAvailability(bool /*available*/,
                                    const hidl_string& /*apn*/) override {
        return false;
    }
    Return<bool> updateNetworkState(bool connected, NetworkType type, bool roaming) override;

 private:
    Gnss* mGnss = nullptr;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_V1_0_AGNSSRIL_H_
