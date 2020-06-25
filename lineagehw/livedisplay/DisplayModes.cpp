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

#include "DisplayModes.h"

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

DisplayModes::DisplayModes(std::shared_ptr<SDMController> controller)
    : controller_(std::move(controller)) {
    if (!isSupported() || !saveInitialDisplayMode()) {
        LOG(FATAL) << "Failed to initialize DisplayModes";
    }

    active_mode_id_ = getDefaultDisplayModeId();
    if (IsSysfsMode(active_mode_id_)) {
        setModeState(active_mode_id_, true);
    }
}

bool DisplayModes::isSupported() {
    static int supported = -1;

    if (supported >= 0) {
        return supported;
    }

    if (utils::CheckFeatureVersion(controller_, utils::FEATURE_VER_SW_SAVEMODES_API) != OK) {
        supported = 0;
        return false;
    }

    int32_t count = 0;
    if (controller_->getNumDisplayModes(&count) != OK) {
        count = 0;
    }
    supported = (count > 0);

    return supported;
}

std::vector<DisplayMode> DisplayModes::getDisplayModesInternal() {
    std::vector<DisplayMode> modes = getDisplayModesQDCM();
    std::vector<DisplayMode> sysfs_modes = getDisplayModesSysfs();

    modes.insert(modes.end(), sysfs_modes.begin(), sysfs_modes.end());

    return modes;
}

std::vector<DisplayMode> DisplayModes::getDisplayModesQDCM() {
    std::vector<DisplayMode> modes;
    int32_t count = 0;

    if (controller_->getNumDisplayModes(&count) != OK || count == 0) {
        return modes;
    }

    std::vector<SdmDispMode> tmp_modes(count);
    if (controller_->getDisplayModes(tmp_modes.data(), count) == OK) {
        for (auto&& mode : tmp_modes) {
            modes.push_back({mode.id, mode.name});
        }
    }

    return modes;
}

std::vector<DisplayMode> DisplayModes::getDisplayModesSysfs() {
    std::vector<DisplayMode> modes;

    for (auto&& [id, name] : kSysfsModeMap) {
        if (!access((kSysfsModeBasePath + name).c_str(), R_OK | W_OK)) {
            modes.push_back({id, name});
        }
    }

    return modes;
}

DisplayMode DisplayModes::getDisplayModeById(int32_t id) {
    if (id >= 0) {
        std::vector<DisplayMode> modes = getDisplayModesInternal();

        for (auto&& mode : modes) {
            if (mode.id == id) {
                return mode;
            }
        }
    }

    return DisplayMode{-1, ""};
}

DisplayMode DisplayModes::getCurrentDisplayModeInternal() {
    return getDisplayModeById(getCurrentDisplayModeId());
}

int32_t DisplayModes::getCurrentDisplayModeId() {
    return active_mode_id_;
}

DisplayMode DisplayModes::getDefaultDisplayModeInternal() {
    return getDisplayModeById(getDefaultDisplayModeId());
}

int32_t DisplayModes::getDefaultDisplayModeId() {
    int32_t id = ReadLocalModeId();
    return (id >= 0 ? id : getDefaultDisplayModeIdQDCM());
}

int32_t DisplayModes::getDefaultDisplayModeIdQDCM() {
    int32_t id = 0;

    if (controller_->getDefaultDisplayMode(&id) != OK) {
        id = -1;
    }

    return id;
}

bool DisplayModes::setModeState(int32_t mode_id, bool on) {
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

bool DisplayModes::saveInitialDisplayMode() {
    int32_t id = ReadInitialModeId();
    if (id < 0) {
        return WriteInitialModeId(std::max(0, getDefaultDisplayModeIdQDCM()));
    }
    return true;
}

// Methods from ::vendor::lineage::livedisplay::V2_0::IDisplayModes follow.
Return<void> DisplayModes::getDisplayModes(getDisplayModes_cb _hidl_cb) {
    _hidl_cb(getDisplayModesInternal());
    return Void();
}

Return<void> DisplayModes::getCurrentDisplayMode(getCurrentDisplayMode_cb _hidl_cb) {
    _hidl_cb(getCurrentDisplayModeInternal());
    return Void();
}

Return<void> DisplayModes::getDefaultDisplayMode(getDefaultDisplayMode_cb _hidl_cb) {
    _hidl_cb(getDefaultDisplayModeInternal());
    return Void();
}

Return<bool> DisplayModes::setDisplayMode(int32_t mode_id, bool make_default) {
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

    if (onSetDisplayMode) {
        onSetDisplayMode();
    }

    return true;
}

void DisplayModes::registerCb(on_set_cb cb) {
    onSetDisplayMode = cb;
}

}  // namespace sdm
}  // namespace V2_0
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
