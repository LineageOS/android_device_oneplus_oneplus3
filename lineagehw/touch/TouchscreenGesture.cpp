/*
 * Copyright (C) 2019,2021 The LineageOS Project
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

#define LOG_TAG "TouchscreenGestureService"

#include <unordered_map>

#include <android-base/file.h>
#include <android-base/logging.h>

#include "TouchscreenGesture.h"

namespace {
typedef struct {
    int32_t keycode;
    const char* name;
    const char* path;
} GestureInfo;

// id -> info
const std::unordered_map<int32_t, GestureInfo> kGestureInfoMap = {
        {0, {255, "Up arrow", "/proc/touchpanel/up_arrow_enable"}},
        {1, {252, "Down arrow", "/proc/touchpanel/down_arrow_enable"}},
        {2, {253, "Left arrow", "/proc/touchpanel/left_arrow_enable"}},
        {3, {254, "Right arrow", "/proc/touchpanel/right_arrow_enable"}},
        {4, {251, "Two finger down swipe", "/proc/touchpanel/double_swipe_enable"}},
        {5, {66, "One finger up swipe", "/proc/touchpanel/up_swipe_enable"}},
        {6, {65, "One finger down swipe", "/proc/touchpanel/down_swipe_enable"}},
        {7, {64, "One finger left swipe", "/proc/touchpanel/left_swipe_enable"}},
        {8, {63, "One finger right swipe", "/proc/touchpanel/right_swipe_enable"}},
        {9, {250, "Letter O", "/proc/touchpanel/letter_o_enable"}},
};
}  // anonymous namespace

namespace vendor {
namespace lineage {
namespace touch {
namespace V1_0 {
namespace implementation {

Return<void> TouchscreenGesture::getSupportedGestures(getSupportedGestures_cb resultCb) {
    std::vector<Gesture> gestures;

    for (const auto& entry : kGestureInfoMap) {
        gestures.push_back({entry.first, entry.second.name, entry.second.keycode});
    }
    resultCb(gestures);

    return Void();
}

Return<bool> TouchscreenGesture::setGestureEnabled(
        const ::vendor::lineage::touch::V1_0::Gesture& gesture, bool enabled) {
    const auto entry = kGestureInfoMap.find(gesture.id);
    if (entry == kGestureInfoMap.end()) {
        return false;
    }

    if (!android::base::WriteStringToFile(std::to_string(enabled), entry->second.path)) {
        LOG(ERROR) << "Wrote file " << entry->second.path << " failed";
        return false;
    }
    return true;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace touch
}  // namespace lineage
}  // namespace vendor
