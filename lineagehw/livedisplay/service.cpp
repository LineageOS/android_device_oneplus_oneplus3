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


#define LOG_TAG "vendor.lineage.livedisplay@2.0-service.oneplus3"

#include <android-base/logging.h>
#include <binder/ProcessState.h>
#include <hidl/HidlTransportSupport.h>

#include <functional>

#include "AdaptiveBacklight.h"
#include "DisplayModes.h"
#include "PictureAdjustment.h"
#include "SDMController.h"
#include "SunlightEnhancement.h"

using ::android::OK;
using ::android::sp;
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::joinRpcThreadpool;

using ::vendor::lineage::livedisplay::V2_0::sdm::AdaptiveBacklight;
using ::vendor::lineage::livedisplay::V2_0::sdm::DisplayModes;
using ::vendor::lineage::livedisplay::V2_0::sdm::PictureAdjustment;
using ::vendor::lineage::livedisplay::V2_0::sdm::SDMController;
using ::vendor::lineage::livedisplay::V2_0::sysfs::SunlightEnhancement;

bool RegisterSdmServices() {
    std::shared_ptr<SDMController> controller = std::make_shared<SDMController>();
    sp<DisplayModes> dm = new DisplayModes(controller);
    sp<PictureAdjustment> pa = new PictureAdjustment(controller);

    // Update default PA on setDisplayMode
    dm->registerCb(std::bind(&PictureAdjustment::updateDefaultPictureAdjustment, pa));

    return dm->registerAsService() == OK && pa->registerAsService() == OK;
}

bool RegisterSysfsServices() {
    if (AdaptiveBacklight::isSupported()) {
        sp<AdaptiveBacklight> ab = new AdaptiveBacklight();
        if (ab->registerAsService() != OK) {
            return false;
        }
    }

    if (SunlightEnhancement::isSupported()) {
        sp<SunlightEnhancement> se = new SunlightEnhancement();
        if (se->registerAsService() != OK) {
            return false;
        }
    }

    return true;
}

int main() {
    android::ProcessState::initWithDriver("/dev/vndbinder");

    LOG(DEBUG) << "LiveDisplay HAL service is starting.";

    configureRpcThreadpool(1, true /*callerWillJoin*/);

    if (RegisterSdmServices() && RegisterSysfsServices()) {
        LOG(DEBUG) << "LiveDisplay HAL service is ready.";
        joinRpcThreadpool();
    } else {
        LOG(ERROR) << "Could not register service for LiveDisplay HAL";
    }

    // In normal operation, we don't expect the thread pool to shutdown
    LOG(ERROR) << "LiveDisplay HAL service is shutting down.";
    return 1;
}
