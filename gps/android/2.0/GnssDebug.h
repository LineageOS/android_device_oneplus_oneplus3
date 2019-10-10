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

#ifndef ANDROID_HARDWARE_GNSS_V2_0_GNSSDEBUG_H
#define ANDROID_HARDWARE_GNSS_V2_0_GNSSDEBUG_H


#include <android/hardware/gnss/2.0/IGnssDebug.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V2_0 {
namespace implementation {

using ::android::hardware::gnss::V2_0::IGnssDebug;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

/* Interface for GNSS Debug support. */
struct Gnss;
struct GnssDebug : public IGnssDebug {
    GnssDebug(Gnss* gnss);
    ~GnssDebug() {};

    //  Methods from ::android::hardware::gnss::V1_0::IGnssDebug follow
    Return<void> getDebugData(getDebugData_cb _hidl_cb) override;
    //  Methods from ::android::hardware::gnss::V2_0::IGnssDebug follow.
    Return<void> getDebugData_2_0(getDebugData_2_0_cb _hidl_cb) override;

private:
    Gnss* mGnss = nullptr;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_V2_0_GNSSDEBUG_H
