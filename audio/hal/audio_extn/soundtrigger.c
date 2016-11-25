/* Copyright (c) 2013-2014, 2016 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#define LOG_TAG "soundtrigger"
/* #define LOG_NDEBUG 0 */
#define LOG_NDDEBUG 0

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <cutils/log.h>
#include "audio_hw.h"
#include "audio_extn.h"
#include "platform.h"
#include "platform_api.h"
#include "sound_trigger_prop_intf.h"

#define XSTR(x) STR(x)
#define STR(x) #x

struct sound_trigger_info  {
    struct sound_trigger_session_info st_ses;
    bool lab_stopped;
    struct listnode list;
};

struct sound_trigger_audio_device {
    void *lib_handle;
    struct audio_device *adev;
    sound_trigger_hw_call_back_t st_callback;
    struct listnode st_ses_list;
    pthread_mutex_t lock;
};

static struct sound_trigger_audio_device *st_dev;

static struct sound_trigger_info *
get_sound_trigger_info(int capture_handle)
{
    struct sound_trigger_info  *st_ses_info = NULL;
    struct listnode *node;
    ALOGV("%s: list empty %d capture_handle %d", __func__,
           list_empty(&st_dev->st_ses_list), capture_handle);
    list_for_each(node, &st_dev->st_ses_list) {
        st_ses_info = node_to_item(node, struct sound_trigger_info , list);
        if (st_ses_info->st_ses.capture_handle == capture_handle)
            return st_ses_info;
    }
    return NULL;
}

int audio_hw_call_back(sound_trigger_event_type_t event,
                       sound_trigger_event_info_t* config)
{
    int status = 0;
    struct sound_trigger_info  *st_ses_info;

    if (!st_dev)
       return -EINVAL;

    pthread_mutex_lock(&st_dev->lock);
    switch (event) {
    case ST_EVENT_SESSION_REGISTER:
        if (!config) {
            ALOGE("%s: NULL config", __func__);
            status = -EINVAL;
            break;
        }
        st_ses_info= calloc(1, sizeof(struct sound_trigger_info ));
        if (!st_ses_info) {
            ALOGE("%s: st_ses_info alloc failed", __func__);
            status = -ENOMEM;
            break;
        }
        memcpy(&st_ses_info->st_ses, &config->st_ses, sizeof (config->st_ses));
        ALOGV("%s: add capture_handle %d pcm %p", __func__,
              st_ses_info->st_ses.capture_handle, st_ses_info->st_ses.pcm);
        list_add_tail(&st_dev->st_ses_list, &st_ses_info->list);
        break;

    case ST_EVENT_SESSION_DEREGISTER:
        if (!config) {
            ALOGE("%s: NULL config", __func__);
            status = -EINVAL;
            break;
        }
        st_ses_info = get_sound_trigger_info(config->st_ses.capture_handle);
        if (!st_ses_info) {
            ALOGE("%s: pcm %p not in the list!", __func__, config->st_ses.pcm);
            status = -EINVAL;
            break;
        }
        ALOGV("%s: remove capture_handle %d pcm %p", __func__,
              st_ses_info->st_ses.capture_handle, st_ses_info->st_ses.pcm);
        list_remove(&st_ses_info->list);
        free(st_ses_info);
        break;
    default:
        ALOGW("%s: Unknown event %d", __func__, event);
        break;
    }
    pthread_mutex_unlock(&st_dev->lock);
    return status;
}

int audio_extn_sound_trigger_read(struct stream_in *in, void *buffer,
                       size_t bytes)
{
    int ret = -1;
    struct sound_trigger_info  *st_info = NULL;
    audio_event_info_t event;

    if (!st_dev)
       return ret;

    if (!in->is_st_session_active) {
        ALOGE(" %s: Sound trigger is not active", __func__);
        goto exit;
    }
    if(in->standby)
        in->standby = false;

    pthread_mutex_lock(&st_dev->lock);
    st_info = get_sound_trigger_info(in->capture_handle);
    pthread_mutex_unlock(&st_dev->lock);
    if (st_info) {
        event.u.aud_info.ses_info = &st_info->st_ses;
        event.u.aud_info.buf = buffer;
        event.u.aud_info.num_bytes = bytes;
        ret = st_dev->st_callback(AUDIO_EVENT_READ_SAMPLES, &event);
    }

exit:
    if (ret) {
        if (-ENETRESET == ret)
            in->is_st_session_active = false;
        memset(buffer, 0, bytes);
        ALOGV("%s: read failed status %d - sleep", __func__, ret);
        usleep((bytes * 1000000) / (audio_stream_in_frame_size((struct audio_stream_in *)in) *
                                   in->config.rate));
    }
    return ret;
}

void audio_extn_sound_trigger_stop_lab(struct stream_in *in)
{
    struct sound_trigger_info  *st_ses_info = NULL;
    audio_event_info_t event;

    if (!st_dev || !in || !in->is_st_session_active)
       return;

    pthread_mutex_lock(&st_dev->lock);
    st_ses_info = get_sound_trigger_info(in->capture_handle);
    pthread_mutex_unlock(&st_dev->lock);
    if (st_ses_info) {
        event.u.ses_info = st_ses_info->st_ses;
        ALOGV("%s: AUDIO_EVENT_STOP_LAB pcm %p", __func__, st_ses_info->st_ses.pcm);
        st_dev->st_callback(AUDIO_EVENT_STOP_LAB, &event);
        in->is_st_session_active = false;
    }
}
void audio_extn_sound_trigger_check_and_get_session(struct stream_in *in)
{
    struct sound_trigger_info  *st_ses_info = NULL;
    struct listnode *node;

    if (!st_dev || !in)
       return;

    pthread_mutex_lock(&st_dev->lock);
    in->is_st_session = false;
    ALOGV("%s: list %d capture_handle %d", __func__,
          list_empty(&st_dev->st_ses_list), in->capture_handle);
    list_for_each(node, &st_dev->st_ses_list) {
        st_ses_info = node_to_item(node, struct sound_trigger_info , list);
        if (st_ses_info->st_ses.capture_handle == in->capture_handle) {
            in->pcm = st_ses_info->st_ses.pcm;
            in->config = st_ses_info->st_ses.config;
            in->channel_mask = audio_channel_in_mask_from_count(in->config.channels);
            in->is_st_session = true;
            in->is_st_session_active = true;
            ALOGD("%s: capture_handle %d is sound trigger", __func__, in->capture_handle);
            break;
        }
    }
    pthread_mutex_unlock(&st_dev->lock);
}

void audio_extn_sound_trigger_update_device_status(snd_device_t snd_device,
                                     st_event_type_t event)
{
    bool raise_event = false;
    int device_type = -1;

    if (!st_dev)
       return;

    if (snd_device >= SND_DEVICE_OUT_BEGIN &&
        snd_device < SND_DEVICE_OUT_END)
        device_type = PCM_PLAYBACK;
    else if (snd_device >= SND_DEVICE_IN_BEGIN &&
        snd_device < SND_DEVICE_IN_END)
        device_type = PCM_CAPTURE;
    else {
        ALOGE("%s: invalid device 0x%x, for event %d",
                           __func__, snd_device, event);
        return;
    }

    raise_event = platform_sound_trigger_device_needs_event(snd_device);
    ALOGI("%s: device 0x%x of type %d for Event %d, with Raise=%d",
        __func__, snd_device, device_type, event, raise_event);
    if (raise_event && (device_type == PCM_CAPTURE)) {
        switch(event) {
        case ST_EVENT_SND_DEVICE_FREE:
            st_dev->st_callback(AUDIO_EVENT_CAPTURE_DEVICE_INACTIVE, NULL);
            break;
        case ST_EVENT_SND_DEVICE_BUSY:
            st_dev->st_callback(AUDIO_EVENT_CAPTURE_DEVICE_ACTIVE, NULL);
            break;
        default:
            ALOGW("%s:invalid event %d for device 0x%x",
                                  __func__, event, snd_device);
        }
    }/*Events for output device, if required can be placed here in else*/
}

void audio_extn_sound_trigger_update_stream_status(struct audio_usecase *uc_info,
                                     st_event_type_t event)
{
    bool raise_event = false;
    audio_usecase_t uc_id;
    int usecase_type = -1;

    if (!st_dev) {
        return;
    }

    if (uc_info == NULL) {
        ALOGE("%s: usecase is NULL!!!", __func__);
        return;
    }
    uc_id = uc_info->id;
    usecase_type = uc_info->type;

    raise_event = platform_sound_trigger_usecase_needs_event(uc_id);
    ALOGD("%s: uc_id %d of type %d for Event %d, with Raise=%d",
        __func__, uc_id, usecase_type, event, raise_event);
    if (raise_event && (usecase_type == PCM_PLAYBACK)) {
        switch(event) {
        case ST_EVENT_STREAM_FREE:
            st_dev->st_callback(AUDIO_EVENT_PLAYBACK_STREAM_INACTIVE, NULL);
            break;
        case ST_EVENT_STREAM_BUSY:
            st_dev->st_callback(AUDIO_EVENT_PLAYBACK_STREAM_ACTIVE, NULL);
            break;
        default:
            ALOGW("%s:invalid event %d, for usecase %d",
                                  __func__, event, uc_id);
        }
    }/*Events for capture usecase, if required can be placed here in else*/
}

void audio_extn_sound_trigger_set_parameters(struct audio_device *adev __unused,
                               struct str_parms *params)
{
    audio_event_info_t event;
    char value[32];
    int ret, val;

    if(!st_dev || !params) {
        ALOGE("%s: str_params NULL", __func__);
        return;
    }

    ret = str_parms_get_str(params, "SND_CARD_STATUS", value,
                            sizeof(value));
    if (ret > 0) {
        if (strstr(value, "OFFLINE")) {
            event.u.status = SND_CARD_STATUS_OFFLINE;
            st_dev->st_callback(AUDIO_EVENT_SSR, &event);
        }
        else if (strstr(value, "ONLINE")) {
            event.u.status = SND_CARD_STATUS_ONLINE;
            st_dev->st_callback(AUDIO_EVENT_SSR, &event);
        }
        else
            ALOGE("%s: unknown snd_card_status", __func__);
    }

    ret = str_parms_get_str(params, "CPE_STATUS", value, sizeof(value));
    if (ret > 0) {
        if (strstr(value, "OFFLINE")) {
            event.u.status = CPE_STATUS_OFFLINE;
            st_dev->st_callback(AUDIO_EVENT_SSR, &event);
        }
        else if (strstr(value, "ONLINE")) {
            event.u.status = CPE_STATUS_ONLINE;
            st_dev->st_callback(AUDIO_EVENT_SSR, &event);
        }
        else
            ALOGE("%s: unknown CPE status", __func__);
    }

    ret = str_parms_get_int(params, "SVA_NUM_SESSIONS", &val);
    if (ret >= 0) {
        event.u.value = val;
        st_dev->st_callback(AUDIO_EVENT_NUM_ST_SESSIONS, &event);
    }
}

int audio_extn_sound_trigger_init(struct audio_device *adev)
{
    int status = 0;
    char sound_trigger_lib[100];

    ALOGI("%s: Enter", __func__);

    st_dev = (struct sound_trigger_audio_device*)
                        calloc(1, sizeof(struct sound_trigger_audio_device));
    if (!st_dev) {
        ALOGE("%s: ERROR. sound trigger alloc failed", __func__);
        return -ENOMEM;
    }

    snprintf(sound_trigger_lib, sizeof(sound_trigger_lib),
             "/system/vendor/lib/hw/sound_trigger.primary.%s.so",
              XSTR(SOUND_TRIGGER_PLATFORM_NAME));

    st_dev->lib_handle = dlopen(sound_trigger_lib, RTLD_NOW);

    if (st_dev->lib_handle == NULL) {
        ALOGE("%s: DLOPEN failed for %s. error = %s", __func__, sound_trigger_lib,
                dlerror());
        status = -EINVAL;
        goto cleanup;
    }
    ALOGI("%s: DLOPEN successful for %s", __func__, sound_trigger_lib);

    st_dev->st_callback = (sound_trigger_hw_call_back_t)
              dlsym(st_dev->lib_handle, "sound_trigger_hw_call_back");

    if (st_dev->st_callback == NULL) {
       ALOGE("%s: ERROR. dlsym Error:%s sound_trigger_hw_call_back", __func__,
               dlerror());
       goto cleanup;
    }

    st_dev->adev = adev;
    list_init(&st_dev->st_ses_list);

    return 0;

cleanup:
    if (st_dev->lib_handle)
        dlclose(st_dev->lib_handle);
    free(st_dev);
    st_dev = NULL;
    return status;

}

void audio_extn_sound_trigger_deinit(struct audio_device *adev)
{
    ALOGI("%s: Enter", __func__);
    if (st_dev && (st_dev->adev == adev) && st_dev->lib_handle) {
        dlclose(st_dev->lib_handle);
        free(st_dev);
        st_dev = NULL;
    }
}
