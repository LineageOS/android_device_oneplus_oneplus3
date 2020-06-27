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

#ifndef ANDROID_HARDWARE_GNSS_V1_0_GNSSDEBUG_H
#define ANDROID_HARDWARE_GNSS_V1_0_GNSSDEBUG_H


#include <android/hardware/gnss/1.0/IGnssDebug.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V1_0 {
namespace implementation {

using ::android::hardware::gnss::V1_0::IGnssDebug;
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

    /*
     * Methods from ::android::hardware::gnss::V1_0::IGnssDebug follow.
     * These declarations were generated from IGnssDebug.hal.
     */
    Return<void> getDebugData(getDebugData_cb _hidl_cb) override;

private:
    Gnss* mGnss = nullptr;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_V1_0_GNSSDEBUG_H
