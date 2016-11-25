/*
 * Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 *
 * Copyright (C) 2013 The Android Open Source Project
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

#define LOG_TAG "offload_effect_bass"
//#define LOG_NDEBUG 0

#include <cutils/list.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <tinyalsa/asoundlib.h>
#include <sound/audio_effects.h>
#include <audio_effects/effect_bassboost.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "effect_api.h"
#include "bass_boost.h"

/* Offload bassboost UUID: 2c4a8c24-1581-487f-94f6-0002a5d5c51b */
const effect_descriptor_t bassboost_descriptor = {
        {0x0634f220, 0xddd4, 0x11db, 0xa0fc, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b }},
        {0x2c4a8c24, 0x1581, 0x487f, 0x94f6, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // uuid
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_DEVICE_IND | EFFECT_FLAG_HW_ACC_TUNNEL),
        0, /* TODO */
        1,
        "MSM offload bassboost",
        "The Android Open Source Project",
};

#define LIB_ACDB_LOADER "libacdbloader.so"
#define PBE_CONF_APP_ID 0x00011134

enum {
        AUDIO_DEVICE_CAL_TYPE = 0,
        AUDIO_STREAM_CAL_TYPE,
};

typedef struct acdb_audio_cal_cfg {
        uint32_t persist;
        uint32_t snd_dev_id;
        uint32_t dev_id;
        int32_t acdb_dev_id;
        uint32_t app_type;
        uint32_t topo_id;
        uint32_t sampling_rate;
        uint32_t cal_type;
        uint32_t module_id;
        uint32_t param_id;
} acdb_audio_cal_cfg_t;

typedef int (*acdb_get_audio_cal_t) (void *, void *, uint32_t*);
static int pbe_load_config(struct pbe_params *params);

/*
 * Bass operations
 */
int bass_get_parameter(effect_context_t *context, effect_param_t *p,
                       uint32_t *size)
{
    bass_context_t *bass_ctxt = (bass_context_t *)context;
    int voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);
    int32_t *param_tmp = (int32_t *)p->data;
    int32_t param = *param_tmp++;
    void *value = p->data + voffset;
    int i;

    ALOGV("%s", __func__);

    p->status = 0;

    switch (param) {
    case BASSBOOST_PARAM_STRENGTH_SUPPORTED:
        if (p->vsize < sizeof(uint32_t))
           p->status = -EINVAL;
        p->vsize = sizeof(uint32_t);
        break;
    case BASSBOOST_PARAM_STRENGTH:
        if (p->vsize < sizeof(int16_t))
           p->status = -EINVAL;
        p->vsize = sizeof(int16_t);
        break;
    default:
        p->status = -EINVAL;
    }

    *size = sizeof(effect_param_t) + voffset + p->vsize;

    if (p->status != 0)
        return 0;

    switch (param) {
    case BASSBOOST_PARAM_STRENGTH_SUPPORTED:
        ALOGV("%s: BASSBOOST_PARAM_STRENGTH_SUPPORTED", __func__);
        if (bass_ctxt->active_index == BASS_BOOST)
            *(uint32_t *)value = 1;
        else
            *(uint32_t *)value = 0;
        break;

    case BASSBOOST_PARAM_STRENGTH:
        ALOGV("%s: BASSBOOST_PARAM_STRENGTH", __func__);
        if (bass_ctxt->active_index == BASS_BOOST)
            *(int16_t *)value = bassboost_get_strength(&(bass_ctxt->bassboost_ctxt));
        else
            *(int16_t *)value = 0;
        break;

    default:
        p->status = -EINVAL;
        break;
    }

    return 0;
}

int bass_set_parameter(effect_context_t *context, effect_param_t *p,
                       uint32_t size __unused)
{
    bass_context_t *bass_ctxt = (bass_context_t *)context;
    int voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);
    void *value = p->data + voffset;
    int32_t *param_tmp = (int32_t *)p->data;
    int32_t param = *param_tmp++;
    uint32_t strength;

    ALOGV("%s", __func__);

    p->status = 0;

    switch (param) {
    case BASSBOOST_PARAM_STRENGTH:
        ALOGV("%s BASSBOOST_PARAM_STRENGTH", __func__);
        if (bass_ctxt->active_index == BASS_BOOST) {
            strength = (uint32_t)(*(int16_t *)value);
            bassboost_set_strength(&(bass_ctxt->bassboost_ctxt), strength);
        } else {
            /* stength supported only for BB and not for PBE, but do not
             * return error for unsupported case, as it fails cts test
             */
            ALOGD("%s ignore set strength, index %d",
                  __func__, bass_ctxt->active_index);
            break;
        }
        break;
    default:
        p->status = -EINVAL;
        break;
    }

    return 0;
}

int bass_set_device(effect_context_t *context, uint32_t device)
{
    bass_context_t *bass_ctxt = (bass_context_t *)context;

    if (device == AUDIO_DEVICE_OUT_SPEAKER) {
        bass_ctxt->active_index = BASS_PBE;
        ALOGV("%s: set PBE mode, device: %x", __func__, device);
    } else if (device == AUDIO_DEVICE_OUT_WIRED_HEADSET ||
        device == AUDIO_DEVICE_OUT_WIRED_HEADPHONE ||
        device == AUDIO_DEVICE_OUT_BLUETOOTH_A2DP ||
        device == AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES) {
        ALOGV("%s: set BB mode, device: %x", __func__, device);
        bass_ctxt->active_index = BASS_BOOST;
    } else {
        ALOGI("%s: disabled by device: %x", __func__, device);
        bass_ctxt->active_index = BASS_INVALID;
    }

    bassboost_set_device((effect_context_t *)&(bass_ctxt->bassboost_ctxt), device);
    pbe_set_device((effect_context_t *)&(bass_ctxt->pbe_ctxt), device);

    return 0;
}

int bass_reset(effect_context_t *context)
{
    bass_context_t *bass_ctxt = (bass_context_t *)context;

    bassboost_reset((effect_context_t *)&(bass_ctxt->bassboost_ctxt));
    pbe_reset((effect_context_t *)&(bass_ctxt->pbe_ctxt));

    return 0;
}

int bass_init(effect_context_t *context)
{
    bass_context_t *bass_ctxt = (bass_context_t *)context;

    // convery i/o channel config to sub effects
    bass_ctxt->bassboost_ctxt.common.config = context->config;
    bass_ctxt->pbe_ctxt.common.config = context->config;

    ALOGV("%s", __func__);

    bass_ctxt->active_index = BASS_BOOST;


    bassboost_init((effect_context_t *)&(bass_ctxt->bassboost_ctxt));
    pbe_init((effect_context_t *)&(bass_ctxt->pbe_ctxt));

    return 0;
}

int bass_enable(effect_context_t *context)
{
    bass_context_t *bass_ctxt = (bass_context_t *)context;

    ALOGV("%s", __func__);

    bassboost_enable((effect_context_t *)&(bass_ctxt->bassboost_ctxt));
    pbe_enable((effect_context_t *)&(bass_ctxt->pbe_ctxt));

    return 0;
}

int bass_disable(effect_context_t *context)
{
    bass_context_t *bass_ctxt = (bass_context_t *)context;

    ALOGV("%s", __func__);

    bassboost_disable((effect_context_t *)&(bass_ctxt->bassboost_ctxt));
    pbe_disable((effect_context_t *)&(bass_ctxt->pbe_ctxt));

    return 0;
}

int bass_start(effect_context_t *context, output_context_t *output)
{
    bass_context_t *bass_ctxt = (bass_context_t *)context;

    ALOGV("%s", __func__);

    bassboost_start((effect_context_t *)&(bass_ctxt->bassboost_ctxt), output);
    pbe_start((effect_context_t *)&(bass_ctxt->pbe_ctxt), output);

    return 0;
}

int bass_stop(effect_context_t *context, output_context_t *output)
{
    bass_context_t *bass_ctxt = (bass_context_t *)context;

    ALOGV("%s", __func__);

    bassboost_stop((effect_context_t *)&(bass_ctxt->bassboost_ctxt), output);
    pbe_stop((effect_context_t *)&(bass_ctxt->pbe_ctxt), output);

    return 0;
}

int bass_set_mode(effect_context_t *context,  int32_t hw_acc_fd)
{
    bass_context_t *bass_ctxt = (bass_context_t *)context;

    ALOGV("%s", __func__);

    bassboost_set_mode((effect_context_t *)&(bass_ctxt->bassboost_ctxt), hw_acc_fd);
    pbe_set_mode((effect_context_t *)&(bass_ctxt->pbe_ctxt), hw_acc_fd);

    return 0;
}

#undef LOG_TAG
#define LOG_TAG "offload_effect_bb"
/*
 * Bassboost operations
 */

int bassboost_get_strength(bassboost_context_t *context)
{
    ALOGV("%s: ctxt %p, strength: %d", __func__,
                      context,  context->strength);
    return context->strength;
}

int bassboost_set_strength(bassboost_context_t *context, uint32_t strength)
{
    ALOGV("%s: ctxt %p, strength: %d", __func__, context, strength);
    context->strength = strength;

    offload_bassboost_set_strength(&(context->offload_bass), strength);
    if (context->ctl)
        offload_bassboost_send_params(context->ctl, &context->offload_bass,
                                      OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG |
                                      OFFLOAD_SEND_BASSBOOST_STRENGTH);
    if (context->hw_acc_fd > 0)
        hw_acc_bassboost_send_params(context->hw_acc_fd,
                                     &context->offload_bass,
                                     OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG |
                                     OFFLOAD_SEND_BASSBOOST_STRENGTH);
    return 0;
}

int bassboost_set_device(effect_context_t *context, uint32_t device)
{
    bassboost_context_t *bass_ctxt = (bassboost_context_t *)context;

    ALOGV("%s: ctxt %p, device 0x%x", __func__, bass_ctxt, device);
    bass_ctxt->device = device;
    if (device == AUDIO_DEVICE_OUT_WIRED_HEADSET ||
        device == AUDIO_DEVICE_OUT_WIRED_HEADPHONE ||
        device == AUDIO_DEVICE_OUT_BLUETOOTH_A2DP ||
        device == AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES) {
        if (bass_ctxt->temp_disabled) {
            if (effect_is_active(&bass_ctxt->common)) {
                offload_bassboost_set_enable_flag(&(bass_ctxt->offload_bass), true);
                if (bass_ctxt->ctl)
                    offload_bassboost_send_params(bass_ctxt->ctl,
                                                  &bass_ctxt->offload_bass,
                                                  OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG);
                if (bass_ctxt->hw_acc_fd > 0)
                    hw_acc_bassboost_send_params(bass_ctxt->hw_acc_fd,
                                                 &bass_ctxt->offload_bass,
                                                 OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG);
            }
            bass_ctxt->temp_disabled = false;
        }
    } else {
        if (!bass_ctxt->temp_disabled) {
            if (effect_is_active(&bass_ctxt->common)) {
                offload_bassboost_set_enable_flag(&(bass_ctxt->offload_bass), false);
                if (bass_ctxt->ctl)
                    offload_bassboost_send_params(bass_ctxt->ctl,
                                                  &bass_ctxt->offload_bass,
                                                  OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG);
                if (bass_ctxt->hw_acc_fd > 0)
                    hw_acc_bassboost_send_params(bass_ctxt->hw_acc_fd,
                                                 &bass_ctxt->offload_bass,
                                                 OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG);
            }
            bass_ctxt->temp_disabled = true;
        }
        ALOGI("%s: ctxt %p, disabled based on device", __func__, bass_ctxt);
    }
    offload_bassboost_set_device(&(bass_ctxt->offload_bass), device);
    return 0;
}

int bassboost_reset(effect_context_t *context)
{
    bassboost_context_t *bass_ctxt = (bassboost_context_t *)context;

    return 0;
}

int bassboost_init(effect_context_t *context)
{
    bassboost_context_t *bass_ctxt = (bassboost_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, bass_ctxt);
    context->config.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
    context->config.inputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    context->config.inputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    context->config.inputCfg.samplingRate = 44100;
    context->config.inputCfg.bufferProvider.getBuffer = NULL;
    context->config.inputCfg.bufferProvider.releaseBuffer = NULL;
    context->config.inputCfg.bufferProvider.cookie = NULL;
    context->config.inputCfg.mask = EFFECT_CONFIG_ALL;
    context->config.outputCfg.accessMode = EFFECT_BUFFER_ACCESS_ACCUMULATE;
    context->config.outputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    context->config.outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    context->config.outputCfg.samplingRate = 44100;
    context->config.outputCfg.bufferProvider.getBuffer = NULL;
    context->config.outputCfg.bufferProvider.releaseBuffer = NULL;
    context->config.outputCfg.bufferProvider.cookie = NULL;
    context->config.outputCfg.mask = EFFECT_CONFIG_ALL;

    set_config(context, &context->config);

    bass_ctxt->hw_acc_fd = -1;
    bass_ctxt->temp_disabled = false;
    memset(&(bass_ctxt->offload_bass), 0, sizeof(struct bass_boost_params));

    return 0;
}

int bassboost_enable(effect_context_t *context)
{
    bassboost_context_t *bass_ctxt = (bassboost_context_t *)context;

    ALOGV("%s: ctxt %p, strength %d", __func__, bass_ctxt, bass_ctxt->strength);

    if (!offload_bassboost_get_enable_flag(&(bass_ctxt->offload_bass)) &&
        !(bass_ctxt->temp_disabled)) {
        offload_bassboost_set_enable_flag(&(bass_ctxt->offload_bass), true);
        if (bass_ctxt->ctl && bass_ctxt->strength)
            offload_bassboost_send_params(bass_ctxt->ctl,
                                          &bass_ctxt->offload_bass,
                                          OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG |
                                          OFFLOAD_SEND_BASSBOOST_STRENGTH);
        if ((bass_ctxt->hw_acc_fd > 0) && (bass_ctxt->strength))
            hw_acc_bassboost_send_params(bass_ctxt->hw_acc_fd,
                                         &bass_ctxt->offload_bass,
                                         OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG |
                                         OFFLOAD_SEND_BASSBOOST_STRENGTH);
    }
    return 0;
}

int bassboost_disable(effect_context_t *context)
{
    bassboost_context_t *bass_ctxt = (bassboost_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, bass_ctxt);
    if (offload_bassboost_get_enable_flag(&(bass_ctxt->offload_bass))) {
        offload_bassboost_set_enable_flag(&(bass_ctxt->offload_bass), false);
        if (bass_ctxt->ctl)
            offload_bassboost_send_params(bass_ctxt->ctl,
                                          &bass_ctxt->offload_bass,
                                          OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG);
        if (bass_ctxt->hw_acc_fd > 0)
            hw_acc_bassboost_send_params(bass_ctxt->hw_acc_fd,
                                         &bass_ctxt->offload_bass,
                                         OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG);
    }
    return 0;
}

int bassboost_start(effect_context_t *context, output_context_t *output)
{
    bassboost_context_t *bass_ctxt = (bassboost_context_t *)context;

    ALOGV("%s: ctxt %p, ctl %p, strength %d", __func__, bass_ctxt,
                                   output->ctl, bass_ctxt->strength);
    bass_ctxt->ctl = output->ctl;
    if (offload_bassboost_get_enable_flag(&(bass_ctxt->offload_bass))) {
        if (bass_ctxt->ctl)
            offload_bassboost_send_params(bass_ctxt->ctl, &bass_ctxt->offload_bass,
                                          OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG |
                                          OFFLOAD_SEND_BASSBOOST_STRENGTH);
        if (bass_ctxt->hw_acc_fd > 0)
            hw_acc_bassboost_send_params(bass_ctxt->hw_acc_fd,
                                         &bass_ctxt->offload_bass,
                                         OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG |
                                         OFFLOAD_SEND_BASSBOOST_STRENGTH);
    }
    return 0;
}

int bassboost_stop(effect_context_t *context, output_context_t *output __unused)
{
    bassboost_context_t *bass_ctxt = (bassboost_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, bass_ctxt);
    if (offload_bassboost_get_enable_flag(&(bass_ctxt->offload_bass)) &&
        bass_ctxt->ctl) {
        struct bass_boost_params bassboost;
        bassboost.enable_flag = false;
        offload_bassboost_send_params(bass_ctxt->ctl, &bassboost,
                                      OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG);
    }
    bass_ctxt->ctl = NULL;
    return 0;
}

int bassboost_set_mode(effect_context_t *context, int32_t hw_acc_fd)
{
    bassboost_context_t *bass_ctxt = (bassboost_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, bass_ctxt);
    bass_ctxt->hw_acc_fd = hw_acc_fd;
    if ((bass_ctxt->hw_acc_fd > 0) &&
        (offload_bassboost_get_enable_flag(&(bass_ctxt->offload_bass))))
        hw_acc_bassboost_send_params(bass_ctxt->hw_acc_fd,
                                     &bass_ctxt->offload_bass,
                                     OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG |
                                     OFFLOAD_SEND_BASSBOOST_STRENGTH);
    return 0;
}

#undef LOG_TAG
#define LOG_TAG "offload_effect_pbe"
/*
 * PBE operations
 */

int pbe_set_device(effect_context_t *context, uint32_t device)
{
    pbe_context_t *pbe_ctxt = (pbe_context_t *)context;
    char          propValue[PROPERTY_VALUE_MAX];
    bool          pbe_enabled_by_prop = false;

    ALOGV("%s: device: %d", __func__, device);
    pbe_ctxt->device = device;

    if (property_get("audio.safx.pbe.enabled", propValue, NULL)) {
        pbe_enabled_by_prop = atoi(propValue) ||
                              !strncmp("true", propValue, 4);
    }

    if (device == AUDIO_DEVICE_OUT_SPEAKER && pbe_enabled_by_prop == true) {
        if (pbe_ctxt->temp_disabled) {
            if (effect_is_active(&pbe_ctxt->common)) {
                offload_pbe_set_enable_flag(&(pbe_ctxt->offload_pbe), true);
                if (pbe_ctxt->ctl)
                    offload_pbe_send_params(pbe_ctxt->ctl,
                                        &pbe_ctxt->offload_pbe,
                                        OFFLOAD_SEND_PBE_ENABLE_FLAG |
                                        OFFLOAD_SEND_PBE_CONFIG);
                if (pbe_ctxt->hw_acc_fd > 0)
                    hw_acc_pbe_send_params(pbe_ctxt->hw_acc_fd,
                                        &pbe_ctxt->offload_pbe,
                                        OFFLOAD_SEND_PBE_ENABLE_FLAG |
                                        OFFLOAD_SEND_PBE_CONFIG);
            }
            pbe_ctxt->temp_disabled = false;
        }
    } else {
        if (!pbe_ctxt->temp_disabled) {
            if (effect_is_active(&pbe_ctxt->common)) {
                offload_pbe_set_enable_flag(&(pbe_ctxt->offload_pbe), false);
                if (pbe_ctxt->ctl)
                    offload_pbe_send_params(pbe_ctxt->ctl,
                                        &pbe_ctxt->offload_pbe,
                                        OFFLOAD_SEND_PBE_ENABLE_FLAG);
                if (pbe_ctxt->hw_acc_fd > 0)
                    hw_acc_pbe_send_params(pbe_ctxt->hw_acc_fd,
                                        &pbe_ctxt->offload_pbe,
                                        OFFLOAD_SEND_PBE_ENABLE_FLAG);
            }
            pbe_ctxt->temp_disabled = true;
        }
    }
    offload_pbe_set_device(&(pbe_ctxt->offload_pbe), device);
    return 0;
}

int pbe_reset(effect_context_t *context)
{
    pbe_context_t *pbe_ctxt = (pbe_context_t *)context;

    return 0;
}

int pbe_init(effect_context_t *context)
{
    pbe_context_t *pbe_ctxt = (pbe_context_t *)context;

    ALOGV("%s", __func__);
    context->config.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
    context->config.inputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    context->config.inputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    context->config.inputCfg.samplingRate = 44100;
    context->config.inputCfg.bufferProvider.getBuffer = NULL;
    context->config.inputCfg.bufferProvider.releaseBuffer = NULL;
    context->config.inputCfg.bufferProvider.cookie = NULL;
    context->config.inputCfg.mask = EFFECT_CONFIG_ALL;
    context->config.outputCfg.accessMode = EFFECT_BUFFER_ACCESS_ACCUMULATE;
    context->config.outputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    context->config.outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    context->config.outputCfg.samplingRate = 44100;
    context->config.outputCfg.bufferProvider.getBuffer = NULL;
    context->config.outputCfg.bufferProvider.releaseBuffer = NULL;
    context->config.outputCfg.bufferProvider.cookie = NULL;
    context->config.outputCfg.mask = EFFECT_CONFIG_ALL;

    set_config(context, &context->config);

    pbe_ctxt->hw_acc_fd = -1;
    pbe_ctxt->temp_disabled = false;
    memset(&(pbe_ctxt->offload_pbe), 0, sizeof(struct pbe_params));
    pbe_load_config(&(pbe_ctxt->offload_pbe));

    return 0;
}

int pbe_enable(effect_context_t *context)
{
    pbe_context_t *pbe_ctxt = (pbe_context_t *)context;

    ALOGV("%s", __func__);

    if (!offload_pbe_get_enable_flag(&(pbe_ctxt->offload_pbe)) &&
        !(pbe_ctxt->temp_disabled)) {
        offload_pbe_set_enable_flag(&(pbe_ctxt->offload_pbe), true);
        if (pbe_ctxt->ctl)
            offload_pbe_send_params(pbe_ctxt->ctl,
                                    &pbe_ctxt->offload_pbe,
                                    OFFLOAD_SEND_PBE_ENABLE_FLAG |
                                    OFFLOAD_SEND_PBE_CONFIG);
        if (pbe_ctxt->hw_acc_fd > 0)
            hw_acc_pbe_send_params(pbe_ctxt->hw_acc_fd,
                                   &pbe_ctxt->offload_pbe,
                                   OFFLOAD_SEND_PBE_ENABLE_FLAG |
                                   OFFLOAD_SEND_PBE_CONFIG);
    }
    return 0;
}

int pbe_disable(effect_context_t *context)
{
    pbe_context_t *pbe_ctxt = (pbe_context_t *)context;

    ALOGV("%s", __func__);
    if (offload_pbe_get_enable_flag(&(pbe_ctxt->offload_pbe))) {
        offload_pbe_set_enable_flag(&(pbe_ctxt->offload_pbe), false);
        if (pbe_ctxt->ctl)
            offload_pbe_send_params(pbe_ctxt->ctl,
                                    &pbe_ctxt->offload_pbe,
                                    OFFLOAD_SEND_PBE_ENABLE_FLAG);
        if (pbe_ctxt->hw_acc_fd > 0)
            hw_acc_pbe_send_params(pbe_ctxt->hw_acc_fd,
                                   &pbe_ctxt->offload_pbe,
                                   OFFLOAD_SEND_PBE_ENABLE_FLAG);
    }
    return 0;
}

int pbe_start(effect_context_t *context, output_context_t *output)
{
    pbe_context_t *pbe_ctxt = (pbe_context_t *)context;

    ALOGV("%s", __func__);
    pbe_ctxt->ctl = output->ctl;
    ALOGV("output->ctl: %p", output->ctl);
    if (offload_pbe_get_enable_flag(&(pbe_ctxt->offload_pbe))) {
        if (pbe_ctxt->ctl)
            offload_pbe_send_params(pbe_ctxt->ctl, &pbe_ctxt->offload_pbe,
                                    OFFLOAD_SEND_PBE_ENABLE_FLAG |
                                    OFFLOAD_SEND_PBE_CONFIG);
        if (pbe_ctxt->hw_acc_fd > 0)
            hw_acc_pbe_send_params(pbe_ctxt->hw_acc_fd,
                                   &pbe_ctxt->offload_pbe,
                                   OFFLOAD_SEND_PBE_ENABLE_FLAG |
                                   OFFLOAD_SEND_PBE_CONFIG);
    }
    return 0;
}

int pbe_stop(effect_context_t *context, output_context_t *output  __unused)
{
    pbe_context_t *pbe_ctxt = (pbe_context_t *)context;

    ALOGV("%s", __func__);
    pbe_ctxt->ctl = NULL;
    return 0;
}

int pbe_set_mode(effect_context_t *context, int32_t hw_acc_fd)
{
    pbe_context_t *pbe_ctxt = (pbe_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, pbe_ctxt);
    pbe_ctxt->hw_acc_fd = hw_acc_fd;
    if ((pbe_ctxt->hw_acc_fd > 0) &&
        (offload_pbe_get_enable_flag(&(pbe_ctxt->offload_pbe))))
        hw_acc_pbe_send_params(pbe_ctxt->hw_acc_fd,
                               &pbe_ctxt->offload_pbe,
                               OFFLOAD_SEND_PBE_ENABLE_FLAG |
                               OFFLOAD_SEND_PBE_CONFIG);
    return 0;
}

static int pbe_load_config(struct pbe_params *params)
{
    int                  ret = 0;
    uint32_t             len = 0;
    uint32_t             propValue = 0;
    uint32_t             pbe_app_type = PBE_CONF_APP_ID;
    char                 propValueStr[PROPERTY_VALUE_MAX];
    void                 *acdb_handle = NULL;
    acdb_get_audio_cal_t acdb_get_audio_cal = NULL;
    acdb_audio_cal_cfg_t cal_cfg = {0};

    acdb_handle = dlopen(LIB_ACDB_LOADER, RTLD_NOW);
    if (acdb_handle == NULL) {
        ALOGE("%s error opening library %s", __func__, LIB_ACDB_LOADER);
        return -EFAULT;
    }

    acdb_get_audio_cal = (acdb_get_audio_cal_t)dlsym(acdb_handle,
                                  "acdb_loader_get_audio_cal_v2");
    if (acdb_get_audio_cal == NULL) {
        dlclose(acdb_handle);
        ALOGE("%s error resolving acdb func symbols", __func__);
        return -EFAULT;
    }
    if (property_get("audio.safx.pbe.app.type", propValueStr, "0")) {
        propValue = atoll(propValueStr);
        if (propValue != 0) {
            pbe_app_type = propValue;
        }
    }
    ALOGD("%s pbe_app_type = 0x%.8x", __func__, pbe_app_type);

    cal_cfg.persist              = 1;
    cal_cfg.cal_type             = AUDIO_STREAM_CAL_TYPE;
    cal_cfg.app_type             = pbe_app_type;
    cal_cfg.module_id            = PBE_CONF_MODULE_ID;
    cal_cfg.param_id             = PBE_CONF_PARAM_ID;

    len = sizeof(params->config);
    ret = acdb_get_audio_cal((void *)&cal_cfg, (void*)&(params->config), &len);
    ALOGD("%s ret = %d, len = %u", __func__, ret, len);
    if (ret == 0)
        params->cfg_len = len;

    dlclose(acdb_handle);
    return ret;
}
