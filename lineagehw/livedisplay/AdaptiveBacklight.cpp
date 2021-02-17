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

#include "AdaptiveBacklight.h"

#include <android-base/properties.h>
#include <android-base/unique_fd.h>
#include <cutils/sockets.h>
#include <poll.h>

namespace {
constexpr size_t kDppsBufSize = 10;

constexpr const char* kDaemonSocket = "pps";
constexpr const char* kFossOff = "foss:off";
constexpr const char* kFossOn = "foss:on";
constexpr const char* kFossProperty = "ro.vendor.display.foss";
constexpr const char* kSuccess = "Success";

android::status_t SendDppsCommand(const char* cmd) {
    android::base::unique_fd sock(
            socket_local_client(kDaemonSocket, ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM));
    if (sock < 0) {
        return android::NO_INIT;
    }

    if (TEMP_FAILURE_RETRY(write(sock, cmd, strlen(cmd))) <= 0) {
        return android::FAILED_TRANSACTION;
    }

    std::string result(kDppsBufSize, 0);
    size_t len = result.length();
    char* buf = &result[0];
    ssize_t ret;
    while ((ret = TEMP_FAILURE_RETRY(read(sock, buf, len))) > 0) {
        if (ret == len) {
            break;
        }
        len -= ret;
        buf += ret;

        struct pollfd p = {.fd = sock, .events = POLLIN, .revents = 0};

        ret = poll(&p, 1, 20);
        if ((ret <= 0) || !(p.revents & POLLIN)) {
            break;
        }
    }

    if (result.compare(0, strlen(kSuccess), kSuccess) == 0) {
        return android::OK;
    }

    return android::BAD_VALUE;
}
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
    return enabled_;
}

Return<bool> AdaptiveBacklight::setEnabled(bool enabled) {
    if (enabled_ == enabled) {
        return true;
    }

    if (SendDppsCommand(enabled ? kFossOn : kFossOff) == android::OK) {
        enabled_ = enabled;
        return true;
    }

    return false;
}

}  // namespace sdm
}  // namespace V2_0
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
