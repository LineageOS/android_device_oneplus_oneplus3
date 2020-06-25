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

#include "MixedDisplayModes.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>
#include <livedisplay/sdm/Utils.h>

#include <unordered_map>

namespace {
using ::android::base::ReadFileToString;
using ::android::base::Trim;
using ::android::base::WriteStringToFile;

const std::string kSysfsModeBasePath = "/sys/class/graphics/fb0/";
const std::unordered_map<int32_t, std::string> kSysfsModeMap = {
        {601, "srgb"},
        {602, "dci_p3"},
};

inline bool IsSysfsMode(int32_t modeId) {
    return modeId > 600;
}

constexpr const char* kLocalModeId = "/data/vendor/display/livedisplay_mode";
constexpr const char* kLocalInitialModeId = "/data/vendor/display/livedisplay_initial_mode";

int32_t ReadLocalModeId() {
    std::string buf;
    if (ReadFileToString(kLocalModeId, &buf)) {
        return std::stoi(Trim(buf));
    }
    return -1;
}

bool WriteLocalModeId(int32_t id) {
    return WriteStringToFile(std::to_string(id), kLocalModeId);
}

int32_t ReadInitialModeId() {
    std::string buf;
    if (ReadFileToString(kLocalInitialModeId, &buf)) {
        return std::stoi(Trim(buf));
    }
    return -1;
}

bool WriteInitialModeId(int32_t id) {
    return WriteStringToFile(std::to_string(id), kLocalInitialModeId);
}
}  // anonymous namespace

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_0 {
namespace sdm {

using ::android::OK;
using ::android::hardware::Void;

MixedDisplayModes::MixedDisplayModes(std::shared_ptr<SDMController> controller)
    : DisplayModes(std::move(controller)) {
    if (!saveInitialDisplayMode()) {
        LOG(FATAL) << "Failed to initialize DisplayModes";
    }

    active_mode_id_ = getDefaultDisplayModeId();
    if (IsSysfsMode(active_mode_id_)) {
        setModeState(active_mode_id_, true);
    }
}

std::vector<DisplayMode> MixedDisplayModes::getDisplayModesQDCM() {
    return DisplayModes::getDisplayModesInternal();
}

std::vector<DisplayMode> MixedDisplayModes::getDisplayModesSysfs() {
    std::vector<DisplayMode> modes;

    for (auto&& [id, name] : kSysfsModeMap) {
        if (!access((kSysfsModeBasePath + name).c_str(), R_OK | W_OK)) {
            modes.push_back({id, name});
        }
    }

    return modes;
}

int32_t MixedDisplayModes::getCurrentDisplayModeId() {
    return active_mode_id_;
}

int32_t MixedDisplayModes::getDefaultDisplayModeId() {
    int32_t id = ReadLocalModeId();
    return (id >= 0 ? id : getDefaultDisplayModeIdQDCM());
}

int32_t MixedDisplayModes::getDefaultDisplayModeIdQDCM() {
    return DisplayModes::getDefaultDisplayModeInternal().id;
}

bool MixedDisplayModes::setModeState(int32_t mode_id, bool on) {
    // To set sysfs mode state, just write to the node
    if (IsSysfsMode(mode_id)) {
        auto&& mode = kSysfsModeMap.find(mode_id);
        if (mode == kSysfsModeMap.end()) {
            return false;
        }
        return WriteStringToFile(on ? "1" : "0", kSysfsModeBasePath + mode->second);
    }

    int32_t id;
    if (on) {
        // To turn on QDCM mode, just enable it
        id = mode_id;
    } else {
        // To turn off QDCM mode, turn on the initial QDCM mode instead
        id = ReadInitialModeId();
        if (id < 0) {
            return false;
        }
        if (id == mode_id) {
            // Can't turn off the initial mode
            return true;
        }
    }
    return controller_->setActiveDisplayMode(id) == OK;
}

bool MixedDisplayModes::saveInitialDisplayMode() {
    int32_t id = ReadInitialModeId();
    if (id < 0) {
        return WriteInitialModeId(std::max(0, getDefaultDisplayModeIdQDCM()));
    }
    return true;
}

// Methods from ::vendor::lineage::livedisplay::V2_0::sdm::DisplayModes follow.
std::vector<DisplayMode> MixedDisplayModes::getDisplayModesInternal() {
    std::vector<DisplayMode> modes = getDisplayModesQDCM();
    std::vector<DisplayMode> sysfs_modes = getDisplayModesSysfs();

    modes.insert(modes.end(), sysfs_modes.begin(), sysfs_modes.end());

    return modes;
}

DisplayMode MixedDisplayModes::getCurrentDisplayModeInternal() {
    return getDisplayModeById(getCurrentDisplayModeId());
}

DisplayMode MixedDisplayModes::getDefaultDisplayModeInternal() {
    return getDisplayModeById(getDefaultDisplayModeId());
}

// Methods from ::vendor::lineage::livedisplay::V2_0::IDisplayModes follow.
Return<bool> MixedDisplayModes::setDisplayMode(int32_t mode_id, bool make_default) {
    int32_t cur_mode_id = getCurrentDisplayModeId();

    if (cur_mode_id >= 0 && cur_mode_id == mode_id) {
        return true;
    }

    DisplayMode mode = getDisplayModeById(mode_id);
    if (mode.id < 0) {
        return false;
    }

    /**
     * Sysfs mode is active or switching to sysfs mode,
     * turn off the active mode before switching.
     */
    if (IsSysfsMode(cur_mode_id) || IsSysfsMode(mode_id)) {
        if (!setModeState(cur_mode_id, false)) {
            return false;
        }
    }

    if (!setModeState(mode_id, true)) {
        return false;
    }

    active_mode_id_ = mode_id;

    if (make_default) {
        if (!WriteLocalModeId(mode_id)) {
            return false;
        }

        if (IsSysfsMode(mode_id)) {
            mode_id = ReadInitialModeId();
            if (mode_id < 0) {
                return false;
            }
        }
        if (controller_->setDefaultDisplayMode(mode_id) != OK) {
            return false;
        }
    }

    if (on_display_mode_set_) {
        on_display_mode_set_();
    }

    return true;
}

}  // namespace sdm
}  // namespace V2_0
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
