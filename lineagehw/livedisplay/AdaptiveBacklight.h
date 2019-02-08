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

#ifndef VENDOR_LINEAGE_LIVEDISPLAY_V2_0_ADAPTIVEBACKLIGHT_H
#define VENDOR_LINEAGE_LIVEDISPLAY_V2_0_ADAPTIVEBACKLIGHT_H

#include <vendor/lineage/livedisplay/2.0/IAdaptiveBacklight.h>

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_0 {
namespace sdm {

using ::android::hardware::Return;

class AdaptiveBacklight : public IAdaptiveBacklight {
   public:
    bool isSupported();

    // Methods from ::vendor::lineage::livedisplay::V2_0::IAdaptiveBacklight follow.
    Return<bool> isEnabled() override;
    Return<bool> setEnabled(bool enabled) override;

   private:
    bool mEnabled = false;
};

}  // namespace sdm
}  // namespace V2_0
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor

#endif  // VENDOR_LINEAGE_LIVEDISPLAY_V2_0_ADAPTIVEBACKLIGHT_H
