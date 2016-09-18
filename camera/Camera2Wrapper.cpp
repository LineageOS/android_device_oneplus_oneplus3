/*
 * Copyright (C) 2015, The CyanogenMod Project
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

#define LOG_TAG "Camera2Wrapper"
#include <cutils/log.h>

#include "CameraWrapper.h"
#include "Camera2Wrapper.h"

typedef struct wrapper_camera2_device {
    camera_device_t base;
    int id;
    camera_device_t *vendor;
} wrapper_camera2_device_t;

#define VENDOR_CALL(device, func, ...) ({ \
    wrapper_camera2_device_t *__wrapper_dev = (wrapper_camera2_device_t*) device; \
    __wrapper_dev->vendor->ops->func(__wrapper_dev->vendor, ##__VA_ARGS__); \
})

#define CAMERA_ID(device) (((wrapper_camera2_device_t *)(device))->id)

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

static char * camera2_fixup_getparams(int id, const char * settings)
{
    android::CameraParameters params;
    params.unflatten(android::String8(settings));

#ifdef LOG_PARAMETERS
    ALOGV("%s: Original parameters:", __FUNCTION__);
    params.dump();
#endif

    android::String8 strParams = params.flatten();
    char *ret = strdup(strParams.string());

#ifdef LOG_PARAMETERS
    ALOGV("%s: Fixed parameters:", __FUNCTION__);
    params.dump();
#endif

    return ret;
}

static char * camera2_fixup_setparams(int id __unused, const char * settings)
{
    android::CameraParameters params;
    params.unflatten(android::String8(settings));

#ifdef LOG_PARAMETERS
    ALOGV("%s: Original parameters:", __FUNCTION__);
    params.dump();
#endif

    android::String8 strParams = params.flatten();
    char *ret = strdup(strParams.string());

#ifdef LOG_PARAMETERS
    ALOGV("%s: Fixed parameters:", __FUNCTION__);
    params.dump();
#endif

    return ret;
}

/*******************************************************************
 * implementation of camera_device_ops functions
 *******************************************************************/

static int camera2_set_preview_window(struct camera_device * device,
        struct preview_stream_ops *window)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device || !window)
        return -EINVAL;

    return VENDOR_CALL(device, set_preview_window, window);
}

static void camera2_set_callbacks(struct camera_device * device,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void *user)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return;

    VENDOR_CALL(device, set_callbacks, notify_cb, data_cb, data_cb_timestamp, get_memory, user);
}

static void camera2_enable_msg_type(struct camera_device * device, int32_t msg_type)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return;

    VENDOR_CALL(device, enable_msg_type, msg_type);
}

static void camera2_disable_msg_type(struct camera_device * device, int32_t msg_type)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return;

    VENDOR_CALL(device, disable_msg_type, msg_type);
}

static int camera2_msg_type_enabled(struct camera_device * device, int32_t msg_type)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return 0;

    return VENDOR_CALL(device, msg_type_enabled, msg_type);
}

static int camera2_start_preview(struct camera_device * device)
{
    int rc = 0;
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return -EINVAL;

    rc = VENDOR_CALL(device, start_preview);
    return rc;
}

static void camera2_stop_preview(struct camera_device * device)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return;

    VENDOR_CALL(device, stop_preview);
}

static int camera2_preview_enabled(struct camera_device * device)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return -EINVAL;

    return VENDOR_CALL(device, preview_enabled);
}

static int camera2_store_meta_data_in_buffers(struct camera_device * device, int enable)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return -EINVAL;

    return VENDOR_CALL(device, store_meta_data_in_buffers, enable);
}

static int camera2_start_recording(struct camera_device * device)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return EINVAL;

    return VENDOR_CALL(device, start_recording);
}

static void camera2_stop_recording(struct camera_device * device)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return;


    VENDOR_CALL(device, stop_recording);
}

static int camera2_recording_enabled(struct camera_device * device)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return -EINVAL;

    return VENDOR_CALL(device, recording_enabled);
}

static void camera2_release_recording_frame(struct camera_device * device,
                const void *opaque)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return;

    VENDOR_CALL(device, release_recording_frame, opaque);
}

static int camera2_auto_focus(struct camera_device * device)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return -EINVAL;


    return VENDOR_CALL(device, auto_focus);
}

static int camera2_cancel_auto_focus(struct camera_device * device)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return -EINVAL;

    return VENDOR_CALL(device, cancel_auto_focus);
}

static int camera2_take_picture(struct camera_device * device)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return -EINVAL;

    return VENDOR_CALL(device, take_picture);
}

static int camera2_cancel_picture(struct camera_device * device)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return -EINVAL;

    return VENDOR_CALL(device, cancel_picture);
}

static int camera2_set_parameters(struct camera_device * device, const char *params)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return -EINVAL;

    char *tmp = NULL;
    tmp = camera2_fixup_setparams(CAMERA_ID(device), params);

    int ret = VENDOR_CALL(device, set_parameters, tmp);

    return ret;
}

static char* camera2_get_parameters(struct camera_device * device)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return NULL;

    char* params = VENDOR_CALL(device, get_parameters);

    char * tmp = camera2_fixup_getparams(CAMERA_ID(device), params);
    VENDOR_CALL(device, put_parameters, params);
    params = tmp;

    return params;
}

static void camera2_put_parameters(struct camera_device *device, char *params)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(params)
        free(params);
}

static int camera2_send_command(struct camera_device * device,
            int32_t cmd, int32_t arg1, int32_t arg2)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return -EINVAL;

    return VENDOR_CALL(device, send_command, cmd, arg1, arg2);
}

static void camera2_release(struct camera_device * device)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return;

    VENDOR_CALL(device, release);
}

static int camera2_dump(struct camera_device * device, int fd)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device, (uintptr_t)(((wrapper_camera2_device_t*)device)->vendor));

    if(!device)
        return -EINVAL;

    return VENDOR_CALL(device, dump, fd);
}

static int camera2_device_close(hw_device_t* device)
{
    int ret = 0;
    wrapper_camera2_device_t *wrapper_dev = NULL;

    ALOGV("%s", __FUNCTION__);

    android::Mutex::Autolock lock(gCameraWrapperLock);

    if (!device) {
        ret = -EINVAL;
        goto done;
    }

    wrapper_dev = (wrapper_camera2_device_t*) device;

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

int camera2_device_open(const hw_module_t* module, const char* name,
                hw_device_t** device)
{
    int rv = 0;
    //int num_cameras = 0;
    int cameraid;
    wrapper_camera2_device_t* camera2_device = NULL;
    camera_device_ops_t* camera2_ops = NULL;

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

        camera2_device = (wrapper_camera2_device_t*)malloc(sizeof(*camera2_device));
        if (!camera2_device) {
            ALOGE("camera2_device allocation fail");
            rv = -ENOMEM;
            goto fail;
        }
        memset(camera2_device, 0, sizeof(*camera2_device));
        camera2_device->id = cameraid;

        rv = gVendorModule->common.methods->open((const hw_module_t*)gVendorModule, name, (hw_device_t**)&(camera2_device->vendor));
        if (rv)
        {
            ALOGE("vendor camera open fail");
            goto fail;
        }
        ALOGV("%s: got vendor camera device 0x%08X", __FUNCTION__, (uintptr_t)(camera2_device->vendor));

        camera2_ops = (camera_device_ops_t*)malloc(sizeof(*camera2_ops));
        if (!camera2_ops) {
            ALOGE("camera_ops allocation fail");
            rv = -ENOMEM;
            goto fail;
        }

        memset(camera2_ops, 0, sizeof(*camera2_ops));

        camera2_device->base.common.tag = HARDWARE_DEVICE_TAG;
        camera2_device->base.common.version = CAMERA_DEVICE_API_VERSION_1_0;
        camera2_device->base.common.module = (hw_module_t *)(module);
        camera2_device->base.common.close = camera2_device_close;
        camera2_device->base.ops = camera2_ops;

        camera2_ops->set_preview_window = camera2_set_preview_window;
        camera2_ops->set_callbacks = camera2_set_callbacks;
        camera2_ops->enable_msg_type = camera2_enable_msg_type;
        camera2_ops->disable_msg_type = camera2_disable_msg_type;
        camera2_ops->msg_type_enabled = camera2_msg_type_enabled;
        camera2_ops->start_preview = camera2_start_preview;
        camera2_ops->stop_preview = camera2_stop_preview;
        camera2_ops->preview_enabled = camera2_preview_enabled;
        camera2_ops->store_meta_data_in_buffers = camera2_store_meta_data_in_buffers;
        camera2_ops->start_recording = camera2_start_recording;
        camera2_ops->stop_recording = camera2_stop_recording;
        camera2_ops->recording_enabled = camera2_recording_enabled;
        camera2_ops->release_recording_frame = camera2_release_recording_frame;
        camera2_ops->auto_focus = camera2_auto_focus;
        camera2_ops->cancel_auto_focus = camera2_cancel_auto_focus;
        camera2_ops->take_picture = camera2_take_picture;
        camera2_ops->cancel_picture = camera2_cancel_picture;
        camera2_ops->set_parameters = camera2_set_parameters;
        camera2_ops->get_parameters = camera2_get_parameters;
        camera2_ops->put_parameters = camera2_put_parameters;
        camera2_ops->send_command = camera2_send_command;
        camera2_ops->release = camera2_release;
        camera2_ops->dump = camera2_dump;

        *device = &camera2_device->base.common;
    }

    return rv;

fail:
    if(camera2_device) {
        free(camera2_device);
        camera2_device = NULL;
    }
    if(camera2_ops) {
        free(camera2_ops);
        camera2_ops = NULL;
    }
    *device = NULL;
    return rv;
}
