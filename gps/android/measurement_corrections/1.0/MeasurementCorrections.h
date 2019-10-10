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

#ifndef ANDROID_HARDWARE_GNSS_V1_0_MeasurementCorrections_H
#define ANDROID_HARDWARE_GNSS_V1_0_MeasurementCorrections_H

#include <android/hardware/gnss/measurement_corrections/1.0/IMeasurementCorrections.h>
#include <android/hardware/gnss/measurement_corrections/1.0/IMeasurementCorrectionsCallback.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include <location_interface.h>

namespace android {
namespace hardware {
namespace gnss {
namespace measurement_corrections {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using ::android::hardware::gnss::V1_0::GnssLocation;
using ::android::hardware::gnss::measurement_corrections::V1_0::IMeasurementCorrectionsCallback;

struct MeasurementCorrections : public IMeasurementCorrections {
    MeasurementCorrections();
    ~MeasurementCorrections();

// Methods from ::android::hardware::gnss::measurement_corrections::V1_0::IMeasurementCorrections follow.
Return<bool> setCorrections(const ::android::hardware::gnss::measurement_corrections::V1_0::MeasurementCorrections& corrections) override;

Return<bool> setCallback(const sp<IMeasurementCorrectionsCallback>& callback) override;

};


}  // namespace implementation
}  // namespace V1_0
}  // namespace measurement_corrections
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_V1_0_MeasurementCorrections_H
