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

#include <unordered_map>

#include "Utils.h"

namespace {
struct sdm_disp_mode {
    int32_t id;
    int32_t type;
    int32_t len;
    char* name;
    sdm_disp_mode() : id(-1), type(0), len(128) { name = new char[128]; }
    ~sdm_disp_mode() { delete[] name; }
};

const std::string kSysfsModeBasePath = "/sys/class/graphics/fb0/";
const std::unordered_map<int32_t, std::string> kSysfsModeMap = {
        {601, "srgb"},
        {602, "dci_p3"},
};

inline bool IsSysfsMode(int32_t modeId) {
    return modeId > 600;
}
}  // anonymous namespace

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_0 {
namespace sdm {

using ::android::base::WriteStringToFile;

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
    int32_t count = 0;
    uint32_t flags = 0;

    if (!utils::CheckFeatureVersion(controller_, FEATURE_VER_SW_SAVEMODES_API) ||
        controller_->getNumDisplayModes(0, 0, &count, &flags) != 0) {
        return 0;
    }
    return count > 0;
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
    uint32_t flags = 0;

    if (controller_->getNumDisplayModes(0, 0, &count, &flags) != 0 || count == 0) {
        return modes;
    }

    sdm_disp_mode tmp[count];

    if (controller_->getDisplayModes(0, 0, tmp, count, &flags) == 0) {
        for (int i = 0; i < count; i++) {
            modes.push_back({tmp[i].id, std::string(tmp[i].name)});
        }
    }

    return modes;
}

std::vector<DisplayMode> DisplayModes::getDisplayModesSysfs() {
    std::vector<DisplayMode> modes;

    for (auto&& entry : kSysfsModeMap) {
        if (!access((kSysfsModeBasePath + entry.second).c_str(), R_OK | W_OK)) {
            modes.push_back({entry.first, entry.second});
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
    int32_t id = utils::ReadLocalModeId();
    return (id >= 0 ? id : getDefaultDisplayModeIdQDCM());
}

int32_t DisplayModes::getDefaultDisplayModeIdQDCM() {
    int32_t id = 0;
    uint32_t flags = 0;

    if (controller_->getDefaultDisplayMode(0, &id, &flags) != 0) {
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
        id = utils::ReadInitialModeId();
        if (id < 0) {
            return false;
        }
        if (id == mode_id) {
            // Can't turn off the initial mode
            return true;
        }
    }
    return controller_->setActiveDisplayMode(0, id, 0) == 0;
}

bool DisplayModes::saveInitialDisplayMode() {
    int32_t id = utils::ReadInitialModeId();
    if (id < 0) {
        return utils::WriteInitialModeId(std::max(0, getDefaultDisplayModeIdQDCM()));
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
        if (!utils::WriteLocalModeId(mode_id)) {
            return false;
        }

        if (IsSysfsMode(mode_id)) {
            mode_id = utils::ReadInitialModeId();
            if (mode_id < 0) {
                return false;
            }
        }
        if (controller_->setDefaultDisplayMode(0, mode_id, 0) != 0) {
            return false;
        }
    }

    if (cb_function_) {
        cb_function_();
    }

    return true;
}

void DisplayModes::registerCb(on_set_cb_function cb_function) {
    cb_function_ = cb_function;
}

}  // namespace sdm
}  // namespace V2_0
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
