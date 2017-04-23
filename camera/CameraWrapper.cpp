/*
 * Copyright (C) 2015, The CyanogenMod Project
 *           (C) 2017, The LineageOS Project
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

#define LOG_NDEBUG 0
#define LOG_PARAMETERS

#define LOG_TAG "CameraWrapper"
#include <cutils/log.h>

#include "CameraWrapper.h"
#include "Camera2Wrapper.h"
#include "Camera3Wrapper.h"

static camera_module_t *gVendorModule = 0;
static char prop[PROPERTY_VALUE_MAX];

static int check_vendor_module()
{
    int rv = 0;

    if(gVendorModule)
        return 0;

    rv = hw_get_module_by_class("camera", "vendor", (const hw_module_t **)&gVendorModule);
    if (rv)
        ALOGE("failed to open vendor camera module");
    return rv;
}

static struct hw_module_methods_t camera_module_methods = {
        open: camera_device_open
};

camera_module_t HAL_MODULE_INFO_SYM = {
    .common = {
         .tag = HARDWARE_MODULE_TAG,
         .module_api_version = CAMERA_MODULE_API_VERSION_2_4,
         .hal_api_version = HARDWARE_HAL_API_VERSION,
         .id = CAMERA_HARDWARE_MODULE_ID,
         .name = "OnePlus 3 Camera Wrapper",
         .author = "The CyanogenMod Project",
         .methods = &camera_module_methods,
         .dso = NULL,
         .reserved = {0},
    },
    .get_number_of_cameras = camera_get_number_of_cameras,
    .get_camera_info = camera_get_camera_info,
    .set_callbacks = camera_set_callbacks,
    .get_vendor_tag_ops = camera_get_vendor_tag_ops,
    .open_legacy = camera_open_legacy,
    .set_torch_mode = camera_set_torch_mode,
    .init = NULL,
    .reserved = {0},
};

static int camera_device_open(const hw_module_t* module, const char* name,
                hw_device_t** device)
{
    int rv = -EINVAL;

    property_get("persist.camera.HAL3.enabled", prop, "1");
    int isHAL3Enabled = atoi(prop);

    ALOGV("%s: property persist.camera.HAL3.enabled is set to %d", __FUNCTION__, isHAL3Enabled);

    if (name != NULL) {
        if (check_vendor_module())
            return -EINVAL;

        if (isHAL3Enabled) {
            ALOGV("%s: using HAL3", __FUNCTION__);
            rv = camera3_device_open(module, name, device);
        } else {
            ALOGV("%s: using HAL2", __FUNCTION__);
            rv = camera2_device_open(module, name, device);
        }
    }

    ALOGV("%s: rv = %d", __FUNCTION__, rv);

    return rv;
}

static int camera_get_number_of_cameras(void)
{
    int rv = 0;

    ALOGV("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;

    rv = gVendorModule->get_number_of_cameras();
    ALOGV("%s: rv = %d", __FUNCTION__, rv);

    return rv;
}

static int camera_get_camera_info(int camera_id, struct camera_info *info)
{
    int rv = -EINVAL;

    ALOGV("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;

    rv = gVendorModule->get_camera_info(camera_id, info);
    ALOGV("%s: rv = %d", __FUNCTION__, rv);

    return rv;
}

static int camera_set_callbacks(const camera_module_callbacks_t *callbacks)
{
    int rv = -EINVAL;

    ALOGV("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;

    rv = gVendorModule->set_callbacks(callbacks);
    ALOGV("%s: rv = %d", __FUNCTION__, rv);

    return rv;
}

static void camera_get_vendor_tag_ops(vendor_tag_ops_t* ops)
{
    ALOGV("%s", __FUNCTION__);
    if (check_vendor_module())
        return;
    return gVendorModule->get_vendor_tag_ops(ops);
}

static int camera_open_legacy(const struct hw_module_t* module, const char* id, uint32_t halVersion, struct hw_device_t** device)
{
    int rv = -EINVAL;

    ALOGV("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;

    switch(halVersion)
    {
        case CAMERA_DEVICE_API_VERSION_1_0:
        {
            rv = camera2_device_open(module, id, device);
            break;
        }
        default:
            ALOGE("%s: Device API version %d invalid", __FUNCTION__, halVersion);
    }
    
    ALOGV("%s: rv = %d", __FUNCTION__, rv);
    
    return rv;
}

static int camera_set_torch_mode(const char* camera_id, bool on)
{
    int rv = -EINVAL;

    ALOGV("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;

    rv = gVendorModule->set_torch_mode(camera_id, on);
    ALOGV("%s: rv = %d", __FUNCTION__, rv);

    return rv;
}
