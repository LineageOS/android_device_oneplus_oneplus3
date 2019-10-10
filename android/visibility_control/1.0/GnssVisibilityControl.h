/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 *     * Neither the name of The Linux Foundation nor the names of its
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
 */

#ifndef ANDROID_HARDWARE_GNSS_V1_0_GnssVisibilityControl_H
#define ANDROID_HARDWARE_GNSS_V1_0_GnssVisibilityControl_H

#include <android/hardware/gnss/visibility_control/1.0/IGnssVisibilityControl.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include <gps_extended_c.h>
#include <location_interface.h>
#include "Gnss.h"

namespace android {
namespace hardware {
namespace gnss {
namespace visibility_control {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using ::android::hardware::gnss::V2_0::implementation::Gnss;

struct GnssVisibilityControl : public IGnssVisibilityControl {
    GnssVisibilityControl(Gnss* gnss);
    ~GnssVisibilityControl();

    // Methods from ::android::hardware::gnss::visibility_control::V1_0::IGnssVisibilityControl follow.
    Return<bool> enableNfwLocationAccess(const hidl_vec<::android::hardware::hidl_string>& proxyApps) override;
    /**
     * Registers the callback for HAL implementation to use.
     *
     * @param callback Handle to IGnssVisibilityControlCallback interface.
     */
    Return<bool> setCallback(const ::android::sp<::android::hardware::gnss::visibility_control::V1_0::IGnssVisibilityControlCallback>& callback) override;

    void statusCb(GnssNfwNotification notification);
    bool isE911Session();

    /* Data call setup callback passed down to GNSS HAL implementation */
    static void nfwStatusCb(GnssNfwNotification notification);
    static bool isInEmergencySession();

private:
    Gnss* mGnss = nullptr;
    sp<IGnssVisibilityControlCallback> mGnssVisibilityControlCbIface = nullptr;
};


}  // namespace implementation
}  // namespace V1_0
}  // namespace visibility_control
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_V1_0_GnssVisibilityControl_H
