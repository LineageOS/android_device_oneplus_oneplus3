/*
 * Copyright (C) 2016 The CyanogenMod Project
 *               2017-2019 The LineageOS Project
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

#include <poll.h>
#include <unistd.h>

#include <cutils/sockets.h>

#include "Utils.h"

namespace {
struct sdm_feature_version {
    uint8_t x, y;
    uint16_t z;
};
}  // anonymous namespace

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_0 {
namespace sdm {

int Utils::sendDPPSCommand(char* buf, size_t len) {
    int rc = 0;
    int sock = socket_local_client("pps", ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM);
    if (sock < 0) {
        return sock;
    }

    if (write(sock, buf, strlen(buf) + 1) > 0) {
        memset(buf, 0, len);
        ssize_t ret;
        while ((ret = read(sock, buf, len)) > 0) {
            if ((size_t)ret == len) {
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
    } else {
        rc = -EIO;
    }

    close(sock);
    return rc;
}

bool Utils::checkFeatureVersion(SDMController* controller, uint64_t cookie,
                                feature_ver_sw feature) {
    sdm_feature_version version;
    uint32_t flags = 0;

    if (controller->get_feature_version(cookie, feature, &version, &flags) != 0) {
        return false;
    }

    if (version.x <= 0 && version.y <= 0 && version.z <= 0) {
        return false;
    }

    return true;
}

}  // namespace sdm
}  // namespace V2_0
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
