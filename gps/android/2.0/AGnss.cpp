/*
 * Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
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

#define LOG_TAG "LocSvc_AGnssInterface"

#include <log_util.h>
#include "Gnss.h"
#include "AGnss.h"

namespace android {
namespace hardware {
namespace gnss {
namespace V2_0 {
namespace implementation {

static AGnss* spAGnss = nullptr;

AGnss::AGnss(Gnss* gnss) : mGnss(gnss) {
    spAGnss = this;
}

AGnss::~AGnss() {
    spAGnss = nullptr;
}

void AGnss::agnssStatusIpV4Cb(AGnssExtStatusIpV4 status) {
    if (nullptr != spAGnss) {
        spAGnss->statusCb(status.type, status.status);
    }
}

void AGnss::statusCb(AGpsExtType type, LocAGpsStatusValue status) {

    V2_0::IAGnssCallback::AGnssType  aType;
    IAGnssCallback::AGnssStatusValue aStatus;

    switch (type) {
    case LOC_AGPS_TYPE_SUPL:
        aType = IAGnssCallback::AGnssType::SUPL;
        break;
    case LOC_AGPS_TYPE_SUPL_ES:
        aType = IAGnssCallback::AGnssType::SUPL_EIMS;
        break;
    default:
        LOC_LOGE("invalid type: %d", type);
        return;
    }

    switch (status) {
    case LOC_GPS_REQUEST_AGPS_DATA_CONN:
        aStatus = IAGnssCallback::AGnssStatusValue::REQUEST_AGNSS_DATA_CONN;
        break;
    case LOC_GPS_RELEASE_AGPS_DATA_CONN:
        aStatus = IAGnssCallback::AGnssStatusValue::RELEASE_AGNSS_DATA_CONN;
        break;
    case LOC_GPS_AGPS_DATA_CONNECTED:
        aStatus = IAGnssCallback::AGnssStatusValue::AGNSS_DATA_CONNECTED;
        break;
    case LOC_GPS_AGPS_DATA_CONN_DONE:
        aStatus = IAGnssCallback::AGnssStatusValue::AGNSS_DATA_CONN_DONE;
        break;
    case LOC_GPS_AGPS_DATA_CONN_FAILED:
        aStatus = IAGnssCallback::AGnssStatusValue::AGNSS_DATA_CONN_FAILED;
        break;
    default:
        LOC_LOGE("invalid status: %d", status);
        return;
    }

    if (mAGnssCbIface != nullptr) {
        auto r = mAGnssCbIface->agnssStatusCb(aType, aStatus);
        if (!r.isOk()) {
            LOC_LOGw("Error invoking AGNSS status cb %s", r.description().c_str());
        }
    }
    else {
        LOC_LOGw("setCallback has not been called yet");
    }
}

Return<void> AGnss::setCallback(const sp<V2_0::IAGnssCallback>& callback) {

    if(mGnss == nullptr || mGnss->getGnssInterface() == nullptr){
        LOC_LOGE("Null GNSS interface");
        return Void();
    }

    // Save the interface
    mAGnssCbIface = callback;

    AgpsCbInfo cbInfo = {};
    cbInfo.statusV4Cb = (void*)agnssStatusIpV4Cb;
    cbInfo.atlType = AGPS_ATL_TYPE_SUPL | AGPS_ATL_TYPE_SUPL_ES;

    mGnss->getGnssInterface()->agpsInit(cbInfo);
    return Void();
}

Return<bool> AGnss::dataConnClosed() {

    if(mGnss == nullptr || mGnss->getGnssInterface() == nullptr){
        LOC_LOGE("Null GNSS interface");
        return false;
    }

    mGnss->getGnssInterface()->agpsDataConnClosed(LOC_AGPS_TYPE_SUPL);
    return true;
}

Return<bool> AGnss::dataConnFailed() {

    if(mGnss == nullptr || mGnss->getGnssInterface() == nullptr){
        LOC_LOGE("Null GNSS interface");
        return false;
    }

    mGnss->getGnssInterface()->agpsDataConnFailed(LOC_AGPS_TYPE_SUPL);
    return true;
}

Return<bool> AGnss::dataConnOpen(uint64_t /*networkHandle*/, const hidl_string& apn,
        V2_0::IAGnss::ApnIpType apnIpType) {

    if(mGnss == nullptr || mGnss->getGnssInterface() == nullptr){
        LOC_LOGE("Null GNSS interface");
        return false;
    }

    /* Validate */
    if(apn.empty()){
        LOC_LOGE("Invalid APN");
        return false;
    }

    LOC_LOGD("dataConnOpen APN name = [%s]", apn.c_str());

    AGpsBearerType bearerType;
    switch (apnIpType) {
    case IAGnss::ApnIpType::IPV4:
        bearerType = AGPS_APN_BEARER_IPV4;
        break;
    case IAGnss::ApnIpType::IPV6:
        bearerType = AGPS_APN_BEARER_IPV6;
        break;
    case IAGnss::ApnIpType::IPV4V6:
        bearerType = AGPS_APN_BEARER_IPV4V6;
        break;
    default:
        bearerType = AGPS_APN_BEARER_IPV4;
        break;
    }

    mGnss->getGnssInterface()->agpsDataConnOpen(
        LOC_AGPS_TYPE_SUPL, apn.c_str(), apn.size(), (int)bearerType);
    return true;
}

Return<bool> AGnss::setServer(V2_0::IAGnssCallback::AGnssType type,
                              const hidl_string& hostname,
                              int32_t port) {
    if (mGnss == nullptr) {
        LOC_LOGE("%s]: mGnss is nullptr", __FUNCTION__);
        return false;
    }

    GnssConfig config;
    memset(&config, 0, sizeof(GnssConfig));
    config.size = sizeof(GnssConfig);
    config.flags = GNSS_CONFIG_FLAGS_SET_ASSISTANCE_DATA_VALID_BIT;
    config.assistanceServer.size = sizeof(GnssConfigSetAssistanceServer);
    if (type == IAGnssCallback::AGnssType::SUPL) {
        config.assistanceServer.type = GNSS_ASSISTANCE_TYPE_SUPL;
    } else if (type == IAGnssCallback::AGnssType::C2K) {
        config.assistanceServer.type = GNSS_ASSISTANCE_TYPE_C2K;
    } else if (type == IAGnssCallback::AGnssType::SUPL_EIMS) {
        config.assistanceServer.type = GNSS_ASSISTANCE_TYPE_SUPL_EIMS;
    } else if (type == IAGnssCallback::AGnssType::SUPL_IMS) {
        config.assistanceServer.type = GNSS_ASSISTANCE_TYPE_SUPL_IMS;
    } else {
        LOC_LOGE("%s]: invalid AGnssType: %d", __FUNCTION__, static_cast<uint8_t>(type));
        return false;
    }
    config.assistanceServer.hostName = strdup(hostname.c_str());
    config.assistanceServer.port = port;
    return mGnss->updateConfiguration(config);
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
