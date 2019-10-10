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
namespace V1_1 {
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

// Methods from ::android::hardware::gnss::V1_1::IGnssConfiguration follow.
Return<bool> GnssConfiguration::setBlacklist(
            const hidl_vec<GnssConfiguration::BlacklistedSource>& blacklist) {

    ENTRY_LOG_CALLFLOW();
    if (nullptr == mGnss) {
        LOC_LOGe("mGnss is null");
        return false;
    }

    // blValid is true if blacklist is empty, i.e. clearing the BL;
    // if blacklist is not empty, blValid is initialied to false, and later
    // updated in the for loop to become true only if there is at least
    // one {constellation, svid} in the list that is valid.
    bool blValid = (0 == blacklist.size());
    GnssConfig config;
    memset(&config, 0, sizeof(GnssConfig));
    config.size = sizeof(GnssConfig);
    config.flags = GNSS_CONFIG_FLAGS_BLACKLISTED_SV_IDS_BIT;
    config.blacklistedSvIds.clear();

    GnssSvIdSource source = {};
    for (int idx = 0; idx < (int)blacklist.size(); idx++) {
        // Set blValid true if any one source is valid
        blValid = setBlacklistedSource(source, blacklist[idx]) || blValid;
        config.blacklistedSvIds.push_back(source);
    }

    // Update configuration only if blValid is true
    // i.e. only if atleast one source is valid for blacklisting
    return (blValid && mGnss->updateConfiguration(config));
}

bool GnssConfiguration::setBlacklistedSource(
        GnssSvIdSource& copyToSource,
        const GnssConfiguration::BlacklistedSource& copyFromSource) {

    bool retVal = true;
    uint16_t svIdOffset = 0;
    copyToSource.size = sizeof(GnssSvIdSource);
    copyToSource.svId = copyFromSource.svid;

    switch(copyFromSource.constellation) {
    case GnssConstellationType::GPS:
        copyToSource.constellation = GNSS_SV_TYPE_GPS;
        LOC_LOGe("GPS SVs can't be blacklisted.");
        retVal = false;
        break;
    case GnssConstellationType::SBAS:
        copyToSource.constellation = GNSS_SV_TYPE_SBAS;
        LOC_LOGe("SBAS SVs can't be blacklisted.");
        retVal = false;
        break;
    case GnssConstellationType::GLONASS:
        copyToSource.constellation = GNSS_SV_TYPE_GLONASS;
        svIdOffset = GNSS_SV_CONFIG_GLO_INITIAL_SV_ID - 1;
        break;
    case GnssConstellationType::QZSS:
        copyToSource.constellation = GNSS_SV_TYPE_QZSS;
        svIdOffset = 0;
        break;
    case GnssConstellationType::BEIDOU:
        copyToSource.constellation = GNSS_SV_TYPE_BEIDOU;
        svIdOffset = GNSS_SV_CONFIG_BDS_INITIAL_SV_ID - 1;
        break;
    case GnssConstellationType::GALILEO:
        copyToSource.constellation = GNSS_SV_TYPE_GALILEO;
        svIdOffset = GNSS_SV_CONFIG_GAL_INITIAL_SV_ID - 1;
        break;
    default:
        copyToSource.constellation = GNSS_SV_TYPE_UNKNOWN;
        LOC_LOGe("Invalid constellation %d", copyFromSource.constellation);
        retVal = false;
        break;
    }

    if (copyToSource.svId > 0 && svIdOffset > 0) {
        copyToSource.svId += svIdOffset;
    }

    return retVal;
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace gnss
}  // namespace hardware
}  // namespace android
