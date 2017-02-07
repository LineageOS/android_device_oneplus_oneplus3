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

#define LOG_TAG "VrHALImpl"

#include <cutils/log.h>
#include <cutils/properties.h>

#include <hardware/vr.h>
#include <hardware/hardware.h>

static void restart_thermal_engine() {
    if (property_set("ctl.restart", "thermal-engine")) {
        ALOGE("%s: couldn't set a system property, "
              "ctl.restart.", __FUNCTION__);
    }
}

static void vr_init(struct vr_module *module) {
    // NOOP
}

static void vr_set_vr_mode(struct vr_module *module, bool enabled) {
    if (enabled) {
        if (property_set("sys.qcom.thermalcfg", "/vendor/etc/thermal-engine-vr.conf")) {
            ALOGE("%s: couldn't set a system property, "
                  "sys.qcom.thermalcfg.", __FUNCTION__);
        }
    } else {
        if (property_set("sys.qcom.thermalcfg", "/vendor/etc/thermal-engine.conf")) {
            ALOGE("%s: couldn't set a system property, "
                  "sys.qcom.thermalcfg.", __FUNCTION__);
        }
    }
    restart_thermal_engine();
}

static struct hw_module_methods_t vr_module_methods = {
    .open = NULL, // There are no devices for this HAL interface.
};


vr_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag                = HARDWARE_MODULE_TAG,
        .module_api_version = VR_MODULE_API_VERSION_1_0,
        .hal_api_version    = HARDWARE_HAL_API_VERSION,
        .id                 = VR_HARDWARE_MODULE_ID,
        .name               = "OnePlus 3 VR HAL",
        .author             = "The Android Open Source Project",
        .methods            = &vr_module_methods,
    },

    .init = vr_init,
    .set_vr_mode = vr_set_vr_mode,
};
