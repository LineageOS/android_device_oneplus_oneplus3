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

#define LOG_TAG "LocSvc_GnssConfigurationInterface"

#include <log_util.h>
#include "Gnss.h"
#include "GnssConfiguration.h"
#include <android/hardware/gnss/1.0/types.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V1_0 {
namespace implementation {

using ::android::hardware::gnss::V1_0::GnssConstellationType;

GnssConfiguration::GnssConfiguration(Gnss* gnss) : mGnss(gnss) {
}

// Methods from ::android::hardware::gps::V1_0::IGnssConfiguration follow.
Return<bool> GnssConfiguration::setSuplEs(bool enabled)  {
    if (mGnss == nullptr) {
        LOC_LOGE("%s]: mGnss is nullptr", __FUNCTION__);
        return false;
    }

    GnssConfig config;
    memset(&config, 0, sizeof(GnssConfig));
    config.size = sizeof(GnssConfig);
    config.flags = GNSS_CONFIG_FLAGS_SUPL_EM_SERVICES_BIT;
    config.suplEmergencyServices = (enabled ?
            GNSS_CONFIG_SUPL_EMERGENCY_SERVICES_YES :
            GNSS_CONFIG_SUPL_EMERGENCY_SERVICES_NO);

    return mGnss->updateConfiguration(config);
}

Return<bool> GnssConfiguration::setSuplVersion(uint32_t version)  {
    if (mGnss == nullptr) {
        LOC_LOGE("%s]: mGnss is nullptr", __FUNCTION__);
        return false;
    }

    GnssConfig config;
    memset(&config, 0, sizeof(GnssConfig));
    config.size = sizeof(GnssConfig);
    config.flags = GNSS_CONFIG_FLAGS_SUPL_VERSION_VALID_BIT;
    switch (version) {
        case 0x00020002:
            config.suplVersion = GNSS_CONFIG_SUPL_VERSION_2_0_2;
            break;
        case 0x00020000:
            config.suplVersion = GNSS_CONFIG_SUPL_VERSION_2_0_0;
            break;
        case 0x00010000:
            config.suplVersion = GNSS_CONFIG_SUPL_VERSION_1_0_0;
            break;
        default:
            LOC_LOGE("%s]: invalid version: 0x%x.", __FUNCTION__, version);
            return false;
            break;
    }

    return mGnss->updateConfiguration(config);
}

Return<bool> GnssConfiguration::setSuplMode(uint8_t mode)  {
    if (mGnss == nullptr) {
        LOC_LOGE("%s]: mGnss is nullptr", __FUNCTION__);
        return false;
    }

    GnssConfig config;
    memset(&config, 0, sizeof(GnssConfig));
    config.size = sizeof(GnssConfig);
    config.flags = GNSS_CONFIG_FLAGS_SUPL_MODE_BIT;
    switch (mode) {
        case 0:
            config.suplModeMask = 0; // STANDALONE ONLY
            break;
        case 1:
            config.suplModeMask = GNSS_CONFIG_SUPL_MODE_MSB_BIT;
            break;
        case 2:
            config.suplModeMask = GNSS_CONFIG_SUPL_MODE_MSA_BIT;
            break;
        case 3:
            config.suplModeMask = GNSS_CONFIG_SUPL_MODE_MSB_BIT | GNSS_CONFIG_SUPL_MODE_MSA_BIT;
            break;
        default:
            LOC_LOGE("%s]: invalid mode: %d.", __FUNCTION__, mode);
            return false;
            break;
    }

    return mGnss->updateConfiguration(config);
}

Return<bool> GnssConfiguration::setLppProfile(uint8_t lppProfile) {
    if (mGnss == nullptr) {
        LOC_LOGE("%s]: mGnss is nullptr", __FUNCTION__);
        return false;
    }

    GnssConfig config;
    memset(&config, 0, sizeof(GnssConfig));
    config.size = sizeof(GnssConfig);
    config.flags = GNSS_CONFIG_FLAGS_LPP_PROFILE_VALID_BIT;
    switch (lppProfile) {
        case 0:
            config.lppProfile = GNSS_CONFIG_LPP_PROFILE_RRLP_ON_LTE;
            break;
        case 1:
            config.lppProfile = GNSS_CONFIG_LPP_PROFILE_USER_PLANE;
            break;
        case 2:
            config.lppProfile = GNSS_CONFIG_LPP_PROFILE_CONTROL_PLANE;
            break;
        case 3:
            config.lppProfile = GNSS_CONFIG_LPP_PROFILE_USER_PLANE_AND_CONTROL_PLANE;
            break;
        default:
            LOC_LOGE("%s]: invalid lppProfile: %d.", __FUNCTION__, lppProfile);
            return false;
            break;
    }

    return mGnss->updateConfiguration(config);
}

Return<bool> GnssConfiguration::setGlonassPositioningProtocol(uint8_t protocol) {
    if (mGnss == nullptr) {
        LOC_LOGE("%s]: mGnss is nullptr", __FUNCTION__);
        return false;
    }

    GnssConfig config;
    memset(&config, 0, sizeof(GnssConfig));
    config.size = sizeof(GnssConfig);

    config.flags = GNSS_CONFIG_FLAGS_AGLONASS_POSITION_PROTOCOL_VALID_BIT;
    if (protocol & (1<<0)) {
        config.aGlonassPositionProtocolMask |= GNSS_CONFIG_RRC_CONTROL_PLANE_BIT;
    }
    if (protocol & (1<<1)) {
        config.aGlonassPositionProtocolMask |= GNSS_CONFIG_RRLP_USER_PLANE_BIT;
    }
    if (protocol & (1<<2)) {
        config.aGlonassPositionProtocolMask |= GNSS_CONFIG_LLP_USER_PLANE_BIT;
    }
    if (protocol & (1<<3)) {
        config.aGlonassPositionProtocolMask |= GNSS_CONFIG_LLP_CONTROL_PLANE_BIT;
    }

    return mGnss->updateConfiguration(config);
}

Return<bool> GnssConfiguration::setGpsLock(uint8_t lock) {
    if (mGnss == nullptr) {
        LOC_LOGE("%s]: mGnss is nullptr", __FUNCTION__);
        return false;
    }

    GnssConfig config;
    memset(&config, 0, sizeof(GnssConfig));
    config.size = sizeof(GnssConfig);
    config.flags = GNSS_CONFIG_FLAGS_GPS_LOCK_VALID_BIT;
    switch (lock) {
        case 0:
            config.gpsLock = GNSS_CONFIG_GPS_LOCK_NONE;
            break;
        case 1:
            config.gpsLock = GNSS_CONFIG_GPS_LOCK_MO;
            break;
        case 2:
            config.gpsLock = GNSS_CONFIG_GPS_LOCK_NI;
            break;
        case 3:
            config.gpsLock = GNSS_CONFIG_GPS_LOCK_MO_AND_NI;
            break;
        default:
            LOC_LOGE("%s]: invalid lock: %d.", __FUNCTION__, lock);
            return false;
            break;
    }

    return mGnss->updateConfiguration(config);
}

Return<bool> GnssConfiguration::setEmergencySuplPdn(bool enabled) {
    if (mGnss == nullptr) {
        LOC_LOGE("%s]: mGnss is nullptr", __FUNCTION__);
        return false;
    }

    GnssConfig config;
    memset(&config, 0, sizeof(GnssConfig));
    config.size = sizeof(GnssConfig);
    config.flags = GNSS_CONFIG_FLAGS_EM_PDN_FOR_EM_SUPL_VALID_BIT;
    config.emergencyPdnForEmergencySupl = (enabled ?
            GNSS_CONFIG_EMERGENCY_PDN_FOR_EMERGENCY_SUPL_YES :
            GNSS_CONFIG_EMERGENCY_PDN_FOR_EMERGENCY_SUPL_NO);

    return mGnss->updateConfiguration(config);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
