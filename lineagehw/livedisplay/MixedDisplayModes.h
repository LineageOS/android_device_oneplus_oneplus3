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

#include <livedisplay/sdm/DisplayModes.h>

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_0 {
namespace sdm {

class MixedDisplayModes : public DisplayModes {
  public:
    explicit MixedDisplayModes(std::shared_ptr<SDMController> controller);

    // Methods from ::vendor::lineage::livedisplay::V2_0::IDisplayModes follow.
    Return<bool> setDisplayMode(int32_t modeID, bool makeDefault) override;

  private:
    int32_t active_mode_id_;

    std::vector<DisplayMode> getDisplayModesQDCM();
    std::vector<DisplayMode> getDisplayModesSysfs();
    int32_t getCurrentDisplayModeId();
    int32_t getDefaultDisplayModeId();
    int32_t getDefaultDisplayModeIdQDCM();
    bool setModeState(int32_t modeId, bool on);
    bool saveInitialDisplayMode();

    // Methods from ::vendor::lineage::livedisplay::V2_0::sdm::DisplayModes follow.
    std::vector<DisplayMode> getDisplayModesInternal() override;
    DisplayMode getCurrentDisplayModeInternal() override;
    DisplayMode getDefaultDisplayModeInternal() override;

    DISALLOW_IMPLICIT_CONSTRUCTORS(MixedDisplayModes);
};

}  // namespace sdm
}  // namespace V2_0
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
