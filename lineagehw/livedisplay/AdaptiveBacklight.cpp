/*
 * Copyright (C) 2019-2020 The LineageOS Project
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

#define LOG_TAG "vendor.lineage.livedisplay@2.0-impl.oneplus3"

#include "AdaptiveBacklight.h"

#include <android-base/logging.h>
#include <android-base/properties.h>

#include "Utils.h"

namespace {
constexpr size_t kDppsBufSize = 64;
constexpr char kFossOn[] = "foss:on";
constexpr char kFossOff[] = "foss:off";
constexpr char kFossProperty[] = "ro.vendor.display.foss";
}  // anonymous namespace

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_0 {
namespace sdm {

using ::android::base::GetBoolProperty;

bool AdaptiveBacklight::isSupported() {
    bool supported = GetBoolProperty(kFossProperty, false);
    if (!supported) {
        LOG(ERROR) << "AdaptiveBacklight not supported!";
    }
    return supported;
}

// Methods from ::vendor::lineage::livedisplay::V2_0::IAdaptiveBacklight follow.
Return<bool> AdaptiveBacklight::isEnabled() {
    return enabled_;
}

Return<bool> AdaptiveBacklight::setEnabled(bool enabled) {
    if (enabled_ == enabled) {
        return true;
    }

    char buf[kDppsBufSize];
    sprintf(buf, "%s", enabled ? kFossOn : kFossOff);
    if (utils::SendDPPSCommand(buf, kDppsBufSize) == 0) {
        if (strncmp(buf, "Success", 7) == 0) {
            enabled_ = enabled;
            return true;
        }
    }

    return false;
}

}  // namespace sdm
}  // namespace V2_0
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
