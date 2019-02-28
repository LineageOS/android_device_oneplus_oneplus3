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

#include <unordered_map>

#include <android-base/file.h>

#include "DisplayModes.h"
#include "PictureAdjustment.h"
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

inline bool isSysfsMode(int32_t modeId) {
    return modeId > 600;
}
}  // anonymous namespace

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_0 {
namespace sdm {

using ::android::base::WriteStringToFile;

DisplayModes::DisplayModes(std::shared_ptr<SDMController> controller, uint64_t cookie)
    : mController(std::move(controller)), mCookie(cookie) {
    if (!isSupported() || !saveInitialDisplayMode()) {
        return;
    }

    mActiveModeId = getDefaultDisplayModeId();
    if (isSysfsMode(mActiveModeId)) {
        setModeState(mActiveModeId, true);
    }
}

bool DisplayModes::isSupported() {
    int32_t count = 0;
    uint32_t flags = 0;
    static int supported = -1;

    if (supported >= 0) {
        goto out;
    }

    if (!Utils::checkFeatureVersion(mController, mCookie, FEATURE_VER_SW_SAVEMODES_API)) {
        supported = 0;
        goto out;
    }

    if (mController->get_num_display_modes(mCookie, 0, 0, &count, &flags) != 0) {
        supported = 0;
        goto out;
    }

    supported = (count > 0);
out:
    return supported;
}

std::vector<DisplayMode> DisplayModes::getDisplayModesInternal() {
    std::vector<DisplayMode> modes = getDisplayModesQDCM();
    std::vector<DisplayMode> sysfsModes = getDisplayModesSysfs();

    modes.insert(modes.end(), sysfsModes.begin(), sysfsModes.end());

    return modes;
}

std::vector<DisplayMode> DisplayModes::getDisplayModesQDCM() {
    std::vector<DisplayMode> modes;
    int32_t count = 0;
    uint32_t flags = 0;

    if (mController->get_num_display_modes(mCookie, 0, 0, &count, &flags) != 0 || count == 0) {
        return modes;
    }

    sdm_disp_mode tmp[count];

    if (mController->get_display_modes(mCookie, 0, 0, tmp, count, &flags) == 0) {
        for (int i = 0; i < count; i++) {
            modes.push_back({tmp[i].id, std::string(tmp[i].name)});
        }
    }

    return modes;
}

std::vector<DisplayMode> DisplayModes::getDisplayModesSysfs() {
    std::vector<DisplayMode> modes;

    for (const auto& entry : kSysfsModeMap) {
        if (!access((kSysfsModeBasePath + entry.second).c_str(), F_OK)) {
            modes.push_back({entry.first, entry.second});
        }
    }

    return modes;
}

DisplayMode DisplayModes::getDisplayModeById(int32_t id) {
    if (id >= 0) {
        std::vector<DisplayMode> modes = getDisplayModesInternal();

        for (const DisplayMode& mode : modes) {
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
    return mActiveModeId;
}

DisplayMode DisplayModes::getDefaultDisplayModeInternal() {
    return getDisplayModeById(getDefaultDisplayModeId());
}

int32_t DisplayModes::getDefaultDisplayModeId() {
    int32_t id = Utils::readLocalModeId();
    return (id >= 0 ? id : getDefaultDisplayModeIdQDCM());
}

int32_t DisplayModes::getDefaultDisplayModeIdQDCM() {
    int32_t id = 0;
    uint32_t flags = 0;

    if (mController->get_default_display_mode(mCookie, 0, &id, &flags) != 0) {
        id = -1;
    }

    return id;
}

bool DisplayModes::setModeState(int32_t modeId, bool on) {
    // To set sysfs mode state, just write to the node
    if (isSysfsMode(modeId)) {
        const auto mode = kSysfsModeMap.find(modeId);
        if (mode == kSysfsModeMap.end()) {
            return false;
        }
        return WriteStringToFile(on ? "1" : "0", kSysfsModeBasePath + mode->second);
    }

    int32_t id;
    if (on) {
        // To turn on QDCM mode, just enable it
        id = modeId;
    } else {
        // To turn off QDCM mode, turn on the initial QDCM mode instead
        id = Utils::readInitialModeId();
        if (id < 0) {
            return false;
        }
        if (id == modeId) {
            // Can't turn off the initial mode
            return true;
        }
    }
    return mController->set_active_display_mode(mCookie, 0, id, 0) == 0;
}

bool DisplayModes::saveInitialDisplayMode() {
    int32_t id = Utils::readInitialModeId();
    if (id < 0) {
        return Utils::writeInitialModeId(std::max(0, getDefaultDisplayModeIdQDCM()));
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

Return<bool> DisplayModes::setDisplayMode(int32_t modeID, bool makeDefault) {
    int32_t curModeId = getCurrentDisplayModeId();

    if (curModeId >= 0 && curModeId == modeID) {
        return true;
    }

    DisplayMode mode = getDisplayModeById(modeID);
    if (mode.id < 0) {
        return false;
    }

    /**
     * Sysfs mode is active or switching to sysfs mode,
     * turn off the active mode before switching.
     */
    if (isSysfsMode(curModeId) || isSysfsMode(modeID)) {
        if (!setModeState(curModeId, false)) {
            return false;
        }
    }

    if (!setModeState(modeID, true)) {
        return false;
    }

    mActiveModeId = modeID;

    if (makeDefault) {
        if (!Utils::writeLocalModeId(modeID)) {
            return false;
        }

        if (isSysfsMode(modeID)) {
            modeID = Utils::readInitialModeId();
            if (modeID < 0) {
                return false;
            }
        }
        if (mController->set_default_display_mode(mCookie, 0, modeID, 0) != 0) {
            return false;
        }
    }

    PictureAdjustment::updateDefaultPictureAdjustment();

    return true;
}

}  // namespace sdm
}  // namespace V2_0
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
