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

#ifndef VENDOR_LINEAGE_TOUCH_V1_0_KEYDISABLER_H
#define VENDOR_LINEAGE_TOUCH_V1_0_KEYDISABLER_H

#include <vendor/lineage/touch/1.0/IKeyDisabler.h>

namespace vendor {
namespace lineage {
namespace touch {
namespace V1_0 {
namespace implementation {

using ::android::hardware::Return;

class KeyDisabler : public IKeyDisabler {
   public:
    KeyDisabler();
    // Methods from ::vendor::lineage::touch::V1_0::IKeyDisabler follow.
    Return<bool> isEnabled() override;
    Return<bool> setEnabled(bool enabled) override;

   private:
    bool mHasKeyDisabler;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace touch
}  // namespace lineage
}  // namespace vendor

#endif  // VENDOR_LINEAGE_TOUCH_V1_0_KEYDISABLER_H
