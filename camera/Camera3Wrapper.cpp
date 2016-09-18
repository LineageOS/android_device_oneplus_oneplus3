/*
 * Copyright (C) 2012, The CyanogenMod Project
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

#define LOG_TAG "Camera3Wrapper"
#include <cutils/log.h>

#include "CameraWrapper.h"
#include "Camera3Wrapper.h"

typedef struct wrapper_camera3_device {
    camera3_device_t base;
    int id;
    camera3_device_t *vendor;
} wrapper_camera3_device_t;

#define VENDOR_CALL(device, func, ...) ({ \
    wrapper_camera3_device_t *__wrapper_dev = (wrapper_camera3_device_t*) device; \
    __wrapper_dev->vendor->ops->func(__wrapper_dev->vendor, ##__VA_ARGS__); \
})

#define CAMERA_ID(device) (((wrapper_camera3_device_t *)(device))->id)

static camera_module_t *gVendorModule = 0;

static int check_vendor_module()
{
    int rv = 0;
    ALOGV("%s", __FUNCTION__);

    if(gVendorModule)
        return 0;

    rv = hw_get_module_by_class("camera", "vendor", (const hw_module_t **)&gVendorModule);
    if (rv)
        ALOGE("failed to open vendor camera module");
    return rv;
}

/*******************************************************************
 * implementation of camera_device_ops functions
 *******************************************************************/

static int camera3_initialize(const camera3_device_t *device, const camera3_callback_ops_t *callback_ops)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
        (uintptr_t)(((wrapper_camera3_device_t*)device)->vendor));

    if (!device)
        return -1;

    return VENDOR_CALL(device, initialize, callback_ops);
}

static int camera3_configure_streams(const camera3_device *device, camera3_stream_configuration_t *stream_list)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
        (uintptr_t)(((wrapper_camera3_device_t*)device)->vendor));

    if (!device)
        return -1;

    return VENDOR_CALL(device, configure_streams, stream_list);
}

static int camera3_register_stream_buffers(const camera3_device *device, const camera3_stream_buffer_set_t *buffer_set)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
        (uintptr_t)(((wrapper_camera3_device_t*)device)->vendor));

    if (!device)
        return -1;

    return VENDOR_CALL(device, register_stream_buffers, buffer_set);
}

static const camera_metadata_t *camera3_construct_default_request_settings(const camera3_device_t *device, int type)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
        (uintptr_t)(((wrapper_camera3_device_t*)device)->vendor));

    if (!device)
        return NULL;

    return VENDOR_CALL(device, construct_default_request_settings, type);
}

static int camera3_process_capture_request(const camera3_device_t *device, camera3_capture_request_t *request)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
        (uintptr_t)(((wrapper_camera3_device_t*)device)->vendor));

    if (!device)
        return -1;

    return VENDOR_CALL(device, process_capture_request, request);
}

static void camera3_get_metadata_vendor_tag_ops(const camera3_device *device, vendor_tag_query_ops_t* ops)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
        (uintptr_t)(((wrapper_camera3_device_t*)device)->vendor));

    if (!device)
        return;

    VENDOR_CALL(device, get_metadata_vendor_tag_ops, ops);
}

static void camera3_dump(const camera3_device_t *device, int fd)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
        (uintptr_t)(((wrapper_camera3_device_t*)device)->vendor));

    if (!device)
        return;

    VENDOR_CALL(device, dump, fd);
}

static int camera3_flush(const camera3_device_t* device)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
        (uintptr_t)(((wrapper_camera3_device_t*)device)->vendor));

    if (!device)
        return -1;

    return VENDOR_CALL(device, flush);
}

static int camera3_device_close(hw_device_t *device)
{
    int ret = 0;
    wrapper_camera3_device_t *wrapper_dev = NULL;

    ALOGV("%s", __FUNCTION__);

    android::Mutex::Autolock lock(gCameraWrapperLock);

    if (!device) {
        ret = -EINVAL;
        goto done;
    }

    wrapper_dev = (wrapper_camera3_device_t*) device;

    wrapper_dev->vendor->common.close((hw_device_t*)wrapper_dev->vendor);
    if (wrapper_dev->base.ops)
        free(wrapper_dev->base.ops);
    free(wrapper_dev);
done:
    return ret;
}

/*******************************************************************
 * implementation of camera_module functions
 *******************************************************************/

/* open device handle to one of the cameras
 *
 * assume camera service will keep singleton of each camera
 * so this function will always only be called once per camera instance
 */

int camera3_device_open(const hw_module_t *module, const char *name,
        hw_device_t **device)
{
    int rv = 0;
    //int num_cameras = 0;
    int cameraid;
    wrapper_camera3_device_t *camera3_device = NULL;
    camera3_device_ops_t *camera3_ops = NULL;

    android::Mutex::Autolock lock(gCameraWrapperLock);

    ALOGV("%s", __FUNCTION__);

    if (name != NULL) {
        if (check_vendor_module())
            return -EINVAL;

        cameraid = atoi(name);
        /*
        num_cameras = gVendorModule->get_number_of_cameras();

        if (cameraid > num_cameras) {
            ALOGE("camera service provided cameraid out of bounds, "
                    "cameraid = %d, num supported = %d",
                    cameraid, num_cameras);
            rv = -EINVAL;
            goto fail;
        }
        */

        camera3_device = (wrapper_camera3_device_t*)malloc(sizeof(*camera3_device));
        if (!camera3_device) {
            ALOGE("camera3_device allocation fail");
            rv = -ENOMEM;
            goto fail;
        }
        memset(camera3_device, 0, sizeof(*camera3_device));
        camera3_device->id = cameraid;

        rv = gVendorModule->common.methods->open((const hw_module_t*)gVendorModule, name, (hw_device_t**)&(camera3_device->vendor));
        if (rv)
        {
            ALOGE("vendor camera open fail");
            goto fail;
        }
        ALOGV("%s: got vendor camera device 0x%08X", __FUNCTION__, (uintptr_t)(camera3_device->vendor));

        camera3_ops = (camera3_device_ops_t*)malloc(sizeof(*camera3_ops));
        if (!camera3_ops) {
            ALOGE("camera3_ops allocation fail");
            rv = -ENOMEM;
            goto fail;
        }

        memset(camera3_ops, 0, sizeof(*camera3_ops));

        camera3_device->base.common.tag = HARDWARE_DEVICE_TAG;
        camera3_device->base.common.version = CAMERA_DEVICE_API_VERSION_3_2;
        camera3_device->base.common.module = (hw_module_t *)(module);
        camera3_device->base.common.close = camera3_device_close;
        camera3_device->base.ops = camera3_ops;

        camera3_ops->initialize = camera3_initialize;
        camera3_ops->configure_streams = camera3_configure_streams;
        camera3_ops->register_stream_buffers = NULL;
        camera3_ops->construct_default_request_settings = camera3_construct_default_request_settings;
        camera3_ops->process_capture_request = camera3_process_capture_request;
        camera3_ops->get_metadata_vendor_tag_ops = camera3_get_metadata_vendor_tag_ops;
        camera3_ops->dump = camera3_dump;
        camera3_ops->flush = camera3_flush;

        *device = &camera3_device->base.common;
    }

    return rv;

fail:
    if (camera3_device) {
        free(camera3_device);
        camera3_device = NULL;
    }
    if (camera3_ops) {
        free(camera3_ops);
        camera3_ops = NULL;
    }
    *device = NULL;
    return rv;
}
