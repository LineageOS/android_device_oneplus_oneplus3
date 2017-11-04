/*
 * Copyright (c) 2017, The Linux Foundation. All rights reserved.
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

#ifndef ANDROID_HARDWARE_GNSS_V1_1_GNSS_H
#define ANDROID_HARDWARE_GNSS_V1_1_GNSS_H

#include <AGnss.h>
#include <AGnssRil.h>
#include <GnssBatching.h>
#include <GnssConfiguration.h>
#include <GnssGeofencing.h>
#include <GnssMeasurement.h>
#include <GnssNi.h>
#include <GnssDebug.h>

#include <android/hardware/gnss/1.0/IGnss.h>
#include <hidl/Status.h>

#include <GnssAPIClient.h>
#include <location_interface.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V1_0 {
namespace implementation {

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct Gnss : public IGnss {
    Gnss();
    ~Gnss();

    // registerAsService will call interfaceChain to determine the version of service
    /* comment this out until we know really how to manipulate hidl version
    using interfaceChain_cb = std::function<
        void(const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& descriptors)>;
    virtual ::android::hardware::Return<void> interfaceChain(interfaceChain_cb _hidl_cb) override {
        _hidl_cb({
                "android.hardware.gnss@1.1::IGnss",
                ::android::hidl::base::V1_0::IBase::descriptor,
                });
        return ::android::hardware::Void();
    }
    */

    /*
     * Methods from ::android::hardware::gnss::V1_0::IGnss follow.
     * These declarations were generated from Gnss.hal.
     */
    Return<bool> setCallback(const sp<IGnssCallback>& callback)  override;
    Return<bool> start()  override;
    Return<bool> stop()  override;
    Return<void> cleanup()  override;
    Return<bool> injectLocation(double latitudeDegrees,
                                double longitudeDegrees,
                                float accuracyMeters)  override;
    Return<bool> injectTime(int64_t timeMs,
                            int64_t timeReferenceMs,
                            int32_t uncertaintyMs) override;
    Return<void> deleteAidingData(IGnss::GnssAidingData aidingDataFlags)  override;
    Return<bool> setPositionMode(IGnss::GnssPositionMode mode,
                                 IGnss::GnssPositionRecurrence recurrence,
                                 uint32_t minIntervalMs,
                                 uint32_t preferredAccuracyMeters,
                                 uint32_t preferredTimeMs)  override;
    Return<sp<IAGnss>> getExtensionAGnss() override;
    Return<sp<IGnssNi>> getExtensionGnssNi() override;
    Return<sp<IGnssMeasurement>> getExtensionGnssMeasurement() override;
    Return<sp<IGnssConfiguration>> getExtensionGnssConfiguration() override;
    Return<sp<IGnssGeofencing>> getExtensionGnssGeofencing() override;
    Return<sp<IGnssBatching>> getExtensionGnssBatching() override;

    Return<sp<IAGnssRil>> getExtensionAGnssRil() override;

    inline Return<sp<IGnssNavigationMessage>> getExtensionGnssNavigationMessage() override {
        return nullptr;
    }

    inline Return<sp<IGnssXtra>> getExtensionXtra() override {
        return nullptr;
    }

    Return<sp<IGnssDebug>> getExtensionGnssDebug() override;

    // These methods are not part of the IGnss base class.
    GnssAPIClient* getApi();
    Return<bool> setGnssNiCb(const sp<IGnssNiCallback>& niCb);
    Return<bool> updateConfiguration(GnssConfig& gnssConfig);
    GnssInterface* getGnssInterface();

 private:
    struct GnssDeathRecipient : hidl_death_recipient {
        GnssDeathRecipient(sp<Gnss> gnss) : mGnss(gnss) {
        }
        ~GnssDeathRecipient() = default;
        virtual void serviceDied(uint64_t cookie, const wp<IBase>& who) override;
        sp<Gnss> mGnss;
    };

 private:
    sp<GnssDeathRecipient> mGnssDeathRecipient = nullptr;

    sp<AGnss> mAGnssIface = nullptr;
    sp<GnssNi> mGnssNi = nullptr;
    sp<GnssMeasurement> mGnssMeasurement = nullptr;
    sp<GnssConfiguration> mGnssConfig = nullptr;
    sp<GnssGeofencing> mGnssGeofencingIface = nullptr;
    sp<GnssBatching> mGnssBatching = nullptr;
    sp<IGnssDebug> mGnssDebug = nullptr;
    sp<AGnssRil> mGnssRil = nullptr;

    GnssAPIClient* mApi = nullptr;
    sp<IGnssCallback> mGnssCbIface = nullptr;
    sp<IGnssNiCallback> mGnssNiCbIface = nullptr;
    GnssConfig mPendingConfig;
    GnssInterface* mGnssInterface = nullptr;
};

extern "C" IGnss* HIDL_FETCH_IGnss(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_V1_1_GNSS_H
