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

#define LOG_TAG "LocSvc_GnssInterface"
#define LOG_NDEBUG 0

#include <fstream>
#include <log_util.h>
#include <dlfcn.h>
#include <cutils/properties.h>
#include "Gnss.h"
#include <LocationUtil.h>

#include "battery_listener.h"

typedef const GnssInterface* (getLocationInterface)();

#define IMAGES_INFO_FILE "/sys/devices/soc0/images"
#define DELIMITER ";"

namespace android {
namespace hardware {
namespace gnss {
namespace V1_1 {
namespace implementation {

static sp<Gnss> sGnss;
static std::string getVersionString() {
    static std::string version;
    if (!version.empty())
        return version;

    char value[PROPERTY_VALUE_MAX] = {0};
    property_get("ro.hardware", value, "unknown");
    version.append(value).append(DELIMITER);

    std::ifstream in(IMAGES_INFO_FILE);
    std::string s;
    while(getline(in, s)) {
        std::size_t found = s.find("CRM:");
        if (std::string::npos == found) {
            continue;
        }

        // skip over space characters after "CRM:"
        const char* substr = s.c_str();
        found += 4;
        while (0 != substr[found] && isspace(substr[found])) {
            found++;
        }
        if (s.find("11:") != found) {
            continue;
        }
        s.erase(0, found + 3);

        found = s.find_first_of("\r\n");
        if (std::string::npos != found) {
            s.erase(s.begin() + found, s.end());
        }
        version.append(s).append(DELIMITER);
    }
    return version;
}

void Gnss::GnssDeathRecipient::serviceDied(uint64_t cookie, const wp<IBase>& who) {
    LOC_LOGE("%s] service died. cookie: %llu, who: %p",
            __FUNCTION__, static_cast<unsigned long long>(cookie), &who);
    if (mGnss != nullptr) {
        mGnss->stop();
        mGnss->cleanup();
    }
}

void location_on_battery_status_changed(bool charging) {
    LOC_LOGd("battery status changed to %s charging", charging ? "" : "not");
    if (sGnss != nullptr) {
        sGnss->getGnssInterface()->updateBatteryStatus(charging);
    }
}
Gnss::Gnss() {
    ENTRY_LOG_CALLFLOW();
    sGnss = this;
    // initilize gnss interface at first in case needing notify battery status
    sGnss->getGnssInterface()->initialize();
    // register health client to listen on battery change
    loc_extn_battery_properties_listener_init(location_on_battery_status_changed);
    // clear pending GnssConfig
    memset(&mPendingConfig, 0, sizeof(GnssConfig));

    mGnssDeathRecipient = new GnssDeathRecipient(this);
}

Gnss::~Gnss() {
    ENTRY_LOG_CALLFLOW();
    if (mApi != nullptr) {
        delete mApi;
        mApi = nullptr;
    }
    sGnss = nullptr;
}

GnssAPIClient* Gnss::getApi() {
    if (mApi == nullptr && (mGnssCbIface != nullptr || mGnssNiCbIface != nullptr)) {
        mApi = new GnssAPIClient(mGnssCbIface, mGnssNiCbIface);
        if (mApi == nullptr) {
            LOC_LOGE("%s] faild to create GnssAPIClient", __FUNCTION__);
            return mApi;
        }

        if (mPendingConfig.size == sizeof(GnssConfig)) {
            // we have pending GnssConfig
            mApi->gnssConfigurationUpdate(mPendingConfig);
            // clear size to invalid mPendingConfig
            mPendingConfig.size = 0;
            if (mPendingConfig.assistanceServer.hostName != nullptr) {
                free((void*)mPendingConfig.assistanceServer.hostName);
            }
        }
    }
    if (mApi == nullptr) {
        LOC_LOGW("%s] GnssAPIClient is not ready", __FUNCTION__);
    }
    return mApi;
}

const GnssInterface* Gnss::getGnssInterface() {
    static bool getGnssInterfaceFailed = false;
    if (nullptr == mGnssInterface && !getGnssInterfaceFailed) {
        LOC_LOGD("%s]: loading libgnss.so::getGnssInterface ...", __func__);
        getLocationInterface* getter = NULL;
        const char *error = NULL;
        dlerror();
        void *handle = dlopen("libgnss.so", RTLD_NOW);
        if (NULL == handle || (error = dlerror()) != NULL)  {
            LOC_LOGW("dlopen for libgnss.so failed, error = %s", error);
        } else {
            getter = (getLocationInterface*)dlsym(handle, "getGnssInterface");
            if ((error = dlerror()) != NULL)  {
                LOC_LOGW("dlsym for libgnss.so::getGnssInterface failed, error = %s", error);
                getter = NULL;
            }
        }

        if (NULL == getter) {
            getGnssInterfaceFailed = true;
        } else {
            mGnssInterface = (const GnssInterface*)(*getter)();
        }
    }
    return mGnssInterface;
}

Return<bool> Gnss::setCallback(const sp<V1_0::IGnssCallback>& callback)  {
    ENTRY_LOG_CALLFLOW();
    if (mGnssCbIface != nullptr) {
        mGnssCbIface->unlinkToDeath(mGnssDeathRecipient);
    }
    mGnssCbIface = callback;
    if (mGnssCbIface != nullptr) {
        mGnssCbIface->linkToDeath(mGnssDeathRecipient, 0 /*cookie*/);
    }

    GnssAPIClient* api = getApi();
    if (api != nullptr) {
        api->gnssUpdateCallbacks(mGnssCbIface, mGnssNiCbIface);
        api->gnssEnable(LOCATION_TECHNOLOGY_TYPE_GNSS);
        api->requestCapabilities();
    }
    return true;
}

Return<bool> Gnss::setGnssNiCb(const sp<IGnssNiCallback>& callback) {
    ENTRY_LOG_CALLFLOW();
    mGnssNiCbIface = callback;
    GnssAPIClient* api = getApi();
    if (api != nullptr) {
        api->gnssUpdateCallbacks(mGnssCbIface, mGnssNiCbIface);
    }
    return true;
}

Return<bool> Gnss::updateConfiguration(GnssConfig& gnssConfig) {
    ENTRY_LOG_CALLFLOW();
    GnssAPIClient* api = getApi();
    if (api) {
        api->gnssConfigurationUpdate(gnssConfig);
    } else if (gnssConfig.flags != 0) {
        // api is not ready yet, update mPendingConfig with gnssConfig
        mPendingConfig.size = sizeof(GnssConfig);

        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_GPS_LOCK_VALID_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_GPS_LOCK_VALID_BIT;
            mPendingConfig.gpsLock = gnssConfig.gpsLock;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_SUPL_VERSION_VALID_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_SUPL_VERSION_VALID_BIT;
            mPendingConfig.suplVersion = gnssConfig.suplVersion;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_SET_ASSISTANCE_DATA_VALID_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_SET_ASSISTANCE_DATA_VALID_BIT;
            mPendingConfig.assistanceServer.size = sizeof(GnssConfigSetAssistanceServer);
            mPendingConfig.assistanceServer.type = gnssConfig.assistanceServer.type;
            if (mPendingConfig.assistanceServer.hostName != nullptr) {
                free((void*)mPendingConfig.assistanceServer.hostName);
                mPendingConfig.assistanceServer.hostName =
                    strdup(gnssConfig.assistanceServer.hostName);
            }
            mPendingConfig.assistanceServer.port = gnssConfig.assistanceServer.port;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_LPP_PROFILE_VALID_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_LPP_PROFILE_VALID_BIT;
            mPendingConfig.lppProfile = gnssConfig.lppProfile;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_LPPE_CONTROL_PLANE_VALID_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_LPPE_CONTROL_PLANE_VALID_BIT;
            mPendingConfig.lppeControlPlaneMask = gnssConfig.lppeControlPlaneMask;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_LPPE_USER_PLANE_VALID_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_LPPE_USER_PLANE_VALID_BIT;
            mPendingConfig.lppeUserPlaneMask = gnssConfig.lppeUserPlaneMask;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_AGLONASS_POSITION_PROTOCOL_VALID_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_AGLONASS_POSITION_PROTOCOL_VALID_BIT;
            mPendingConfig.aGlonassPositionProtocolMask = gnssConfig.aGlonassPositionProtocolMask;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_EM_PDN_FOR_EM_SUPL_VALID_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_EM_PDN_FOR_EM_SUPL_VALID_BIT;
            mPendingConfig.emergencyPdnForEmergencySupl = gnssConfig.emergencyPdnForEmergencySupl;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_SUPL_EM_SERVICES_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_SUPL_EM_SERVICES_BIT;
            mPendingConfig.suplEmergencyServices = gnssConfig.suplEmergencyServices;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_SUPL_MODE_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_SUPL_MODE_BIT;
            mPendingConfig.suplModeMask = gnssConfig.suplModeMask;
        }
        if (gnssConfig.flags & GNSS_CONFIG_FLAGS_BLACKLISTED_SV_IDS_BIT) {
            mPendingConfig.flags |= GNSS_CONFIG_FLAGS_BLACKLISTED_SV_IDS_BIT;
            mPendingConfig.blacklistedSvIds = gnssConfig.blacklistedSvIds;
        }
    }
    return true;
}

Return<bool> Gnss::start()  {
    ENTRY_LOG_CALLFLOW();
    bool retVal = false;
    GnssAPIClient* api = getApi();
    if (api) {
        retVal = api->gnssStart();
    }
    return retVal;
}

Return<bool> Gnss::stop()  {
    ENTRY_LOG_CALLFLOW();
    bool retVal = false;
    GnssAPIClient* api = getApi();
    if (api) {
        retVal = api->gnssStop();
    }
    return retVal;
}

Return<void> Gnss::cleanup()  {
    ENTRY_LOG_CALLFLOW();

    if (mApi != nullptr) {
        mApi->gnssDisable();
    }

    return Void();
}

Return<bool> Gnss::injectLocation(double latitudeDegrees,
                                  double longitudeDegrees,
                                  float accuracyMeters)  {
    ENTRY_LOG_CALLFLOW();
    const GnssInterface* gnssInterface = getGnssInterface();
    if (nullptr != gnssInterface) {
        gnssInterface->injectLocation(latitudeDegrees, longitudeDegrees, accuracyMeters);
        return true;
    } else {
        return false;
    }
}

Return<bool> Gnss::injectTime(int64_t timeMs, int64_t timeReferenceMs,
                              int32_t uncertaintyMs) {
    ENTRY_LOG_CALLFLOW();
    const GnssInterface* gnssInterface = getGnssInterface();
    if (nullptr != gnssInterface) {
        gnssInterface->injectTime(timeMs, timeReferenceMs, uncertaintyMs);
        return true;
    } else {
        return false;
    }
}

Return<void> Gnss::deleteAidingData(V1_0::IGnss::GnssAidingData aidingDataFlags)  {
    ENTRY_LOG_CALLFLOW();
    GnssAPIClient* api = getApi();
    if (api) {
        api->gnssDeleteAidingData(aidingDataFlags);
    }
    return Void();
}

Return<bool> Gnss::setPositionMode(V1_0::IGnss::GnssPositionMode mode,
                                   V1_0::IGnss::GnssPositionRecurrence recurrence,
                                   uint32_t minIntervalMs,
                                   uint32_t preferredAccuracyMeters,
                                   uint32_t preferredTimeMs)  {
    ENTRY_LOG_CALLFLOW();
    bool retVal = false;
    GnssAPIClient* api = getApi();
    if (api) {
        retVal = api->gnssSetPositionMode(mode, recurrence, minIntervalMs,
                preferredAccuracyMeters, preferredTimeMs);
    }
    return retVal;
}

Return<sp<V1_0::IAGnss>> Gnss::getExtensionAGnss()  {
    ENTRY_LOG_CALLFLOW();
    mAGnssIface = new AGnss(this);
    return mAGnssIface;
}

Return<sp<V1_0::IGnssNi>> Gnss::getExtensionGnssNi()  {
    ENTRY_LOG_CALLFLOW();
    mGnssNi = new GnssNi(this);
    return mGnssNi;
}

Return<sp<V1_0::IGnssMeasurement>> Gnss::getExtensionGnssMeasurement() {
    ENTRY_LOG_CALLFLOW();
    if (mGnssMeasurement == nullptr)
        mGnssMeasurement = new GnssMeasurement();
    return mGnssMeasurement;
}

Return<sp<V1_0::IGnssConfiguration>> Gnss::getExtensionGnssConfiguration()  {
    ENTRY_LOG_CALLFLOW();
    mGnssConfig = new GnssConfiguration(this);
    return mGnssConfig;
}

Return<sp<V1_0::IGnssGeofencing>> Gnss::getExtensionGnssGeofencing()  {
    ENTRY_LOG_CALLFLOW();
    mGnssGeofencingIface = new GnssGeofencing();
    return mGnssGeofencingIface;
}

Return<sp<V1_0::IGnssBatching>> Gnss::getExtensionGnssBatching()  {
    mGnssBatching = new GnssBatching();
    return mGnssBatching;
}

Return<sp<V1_0::IGnssDebug>> Gnss::getExtensionGnssDebug() {
    ENTRY_LOG_CALLFLOW();
    mGnssDebug = new GnssDebug(this);
    return mGnssDebug;
}

Return<sp<V1_0::IAGnssRil>> Gnss::getExtensionAGnssRil() {
    mGnssRil = new AGnssRil(this);
    return mGnssRil;
}

// Methods from ::android::hardware::gnss::V1_1::IGnss follow.
Return<bool> Gnss::setCallback_1_1(const sp<V1_1::IGnssCallback>& callback) {
    ENTRY_LOG_CALLFLOW();
    callback->gnssNameCb(getVersionString());
    mGnssCbIface_1_1 = callback;
    const GnssInterface* gnssInterface = getGnssInterface();
    if (nullptr != gnssInterface) {
        OdcpiRequestCallback cb = [this](const OdcpiRequestInfo& odcpiRequest) {
            odcpiRequestCb(odcpiRequest);
        };
        gnssInterface->odcpiInit(cb);
    }
    return setCallback(callback);
}

Return<bool> Gnss::setPositionMode_1_1(V1_0::IGnss::GnssPositionMode mode,
        V1_0::IGnss::GnssPositionRecurrence recurrence,
        uint32_t minIntervalMs,
        uint32_t preferredAccuracyMeters,
        uint32_t preferredTimeMs,
        bool lowPowerMode) {
    ENTRY_LOG_CALLFLOW();
    bool retVal = false;
    GnssAPIClient* api = getApi();
    if (api) {
        GnssPowerMode powerMode = lowPowerMode?
                GNSS_POWER_MODE_M4 : GNSS_POWER_MODE_M2;
        retVal = api->gnssSetPositionMode(mode, recurrence, minIntervalMs,
                preferredAccuracyMeters, preferredTimeMs, powerMode, minIntervalMs);
    }
    return retVal;
}

Return<sp<V1_1::IGnssMeasurement>> Gnss::getExtensionGnssMeasurement_1_1() {
    ENTRY_LOG_CALLFLOW();
    if (mGnssMeasurement == nullptr)
        mGnssMeasurement = new GnssMeasurement();
    return mGnssMeasurement;
}

Return<sp<V1_1::IGnssConfiguration>> Gnss::getExtensionGnssConfiguration_1_1() {
    ENTRY_LOG_CALLFLOW();
    if (mGnssConfig == nullptr)
        mGnssConfig = new GnssConfiguration(this);
    return mGnssConfig;
}

Return<bool> Gnss::injectBestLocation(const GnssLocation& gnssLocation) {
    ENTRY_LOG_CALLFLOW();
    const GnssInterface* gnssInterface = getGnssInterface();
    if (nullptr != gnssInterface) {
        Location location = {};
        convertGnssLocation(gnssLocation, location);
        gnssInterface->odcpiInject(location);
    }
    return true;
}

void Gnss::odcpiRequestCb(const OdcpiRequestInfo& request) {
    ENTRY_LOG_CALLFLOW();
    if (mGnssCbIface_1_1 != nullptr) {
        // For emergency mode, request DBH (Device based hybrid) location
        // Mark Independent from GNSS flag to false.
        if (ODCPI_REQUEST_TYPE_START == request.type) {
            auto r = mGnssCbIface_1_1->gnssRequestLocationCb(!request.isEmergencyMode);
            if (!r.isOk()) {
                LOC_LOGe("Error invoking gnssRequestLocationCb %s", r.description().c_str());
            }
        } else {
            LOC_LOGv("Unsupported ODCPI request type: %d", request.type);
        }
    } else {
        LOC_LOGe("ODCPI request not supported.");
    }
}

V1_0::IGnss* HIDL_FETCH_IGnss(const char* hal) {
    ENTRY_LOG_CALLFLOW();
    V1_0::IGnss* iface = nullptr;
    iface = new Gnss();
    if (iface == nullptr) {
        LOC_LOGE("%s]: failed to get %s", __FUNCTION__, hal);
    }
    return iface;
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace gnss
}  // namespace hardware
}  // namespace android
