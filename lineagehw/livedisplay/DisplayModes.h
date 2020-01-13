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

#pragma once

#include <android-base/macros.h>
#include <vendor/lineage/livedisplay/2.0/IDisplayModes.h>

#include "SDMController.h"

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_0 {
namespace sdm {

using ::android::hardware::Return;
using ::android::hardware::Void;

class DisplayModes : public IDisplayModes {
  public:
    explicit DisplayModes(std::shared_ptr<SDMController> controller);

    using on_set_cb_function = std::function<void()>;
    void registerCb(on_set_cb_function cb_function);

    // Methods from ::vendor::lineage::livedisplay::V2_0::IDisplayModes follow.
    Return<void> getDisplayModes(getDisplayModes_cb _hidl_cb) override;
    Return<void> getCurrentDisplayMode(getCurrentDisplayMode_cb _hidl_cb) override;
    Return<void> getDefaultDisplayMode(getDefaultDisplayMode_cb _hidl_cb) override;
    Return<bool> setDisplayMode(int32_t modeID, bool makeDefault) override;

  private:
    std::shared_ptr<SDMController> controller_;
    int32_t active_mode_id_;
    on_set_cb_function cb_function_;

    bool isSupported();

    std::vector<DisplayMode> getDisplayModesInternal();
    std::vector<DisplayMode> getDisplayModesQDCM();
    std::vector<DisplayMode> getDisplayModesSysfs();
    DisplayMode getDisplayModeById(int32_t id);
    DisplayMode getCurrentDisplayModeInternal();
    int32_t getCurrentDisplayModeId();
    DisplayMode getDefaultDisplayModeInternal();
    int32_t getDefaultDisplayModeId();
    int32_t getDefaultDisplayModeIdQDCM();
    bool setModeState(int32_t modeId, bool on);
    bool saveInitialDisplayMode();

    DISALLOW_IMPLICIT_CONSTRUCTORS(DisplayModes);
};

}  // namespace sdm
}  // namespace V2_0
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
