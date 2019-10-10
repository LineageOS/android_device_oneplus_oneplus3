/*
 * Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 * Not a Contribution
 */
/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "LocSvc_GnssNiInterface"

#include <log_util.h>
#include "Gnss.h"
#include "GnssNi.h"

namespace android {
namespace hardware {
namespace gnss {
namespace V1_1 {
namespace implementation {

void GnssNi::GnssNiDeathRecipient::serviceDied(uint64_t cookie, const wp<IBase>& who) {
    LOC_LOGE("%s] service died. cookie: %llu, who: %p",
            __FUNCTION__, static_cast<unsigned long long>(cookie), &who);
    // we do nothing here
    // Gnss::GnssDeathRecipient will stop the session
}

GnssNi::GnssNi(Gnss* gnss) : mGnss(gnss) {
    mGnssNiDeathRecipient = new GnssNiDeathRecipient(this);
}

// Methods from ::android::hardware::gnss::V1_0::IGnssNi follow.
Return<void> GnssNi::setCallback(const sp<IGnssNiCallback>& callback)  {
    if (mGnss == nullptr) {
        LOC_LOGE("%s]: mGnss is nullptr", __FUNCTION__);
        return Void();
    }

    mGnss->setGnssNiCb(callback);

    if (mGnssNiCbIface != nullptr) {
        mGnssNiCbIface->unlinkToDeath(mGnssNiDeathRecipient);
    }
    mGnssNiCbIface = callback;
    if (mGnssNiCbIface != nullptr) {
        mGnssNiCbIface->linkToDeath(mGnssNiDeathRecipient, 0 /*cookie*/);
    }

    return Void();
}

Return<void> GnssNi::respond(int32_t notifId, IGnssNiCallback::GnssUserResponseType userResponse)  {
    if (mGnss == nullptr) {
        LOC_LOGE("%s]: mGnss is nullptr", __FUNCTION__);
        return Void();
    }

    GnssAPIClient* api = mGnss->getApi();
    if (api == nullptr) {
        LOC_LOGE("%s]: api is nullptr", __FUNCTION__);
        return Void();
    }

    api->gnssNiRespond(notifId, userResponse);

    return Void();
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace gnss
}  // namespace hardware
}  // namespace android
