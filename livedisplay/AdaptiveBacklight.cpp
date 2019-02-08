/*
 * Copyright (C) 2019 The LineageOS Project
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

#include <android-base/properties.h>

#include "AdaptiveBacklight.h"
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
    return GetBoolProperty(kFossProperty, false);
}

// Methods from ::vendor::lineage::livedisplay::V2_0::IAdaptiveBacklight follow.
Return<bool> AdaptiveBacklight::isEnabled() {
    return mEnabled;
}

Return<bool> AdaptiveBacklight::setEnabled(bool enabled) {
    if (mEnabled == enabled) {
        return true;
    }

    char buf[kDppsBufSize];
    sprintf(buf, "%s", enabled ? kFossOn : kFossOff);
    if (Utils::sendDPPSCommand(buf, kDppsBufSize) == 0) {
        if (strncmp(buf, "Success", 7) == 0) {
            mEnabled = enabled;
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
