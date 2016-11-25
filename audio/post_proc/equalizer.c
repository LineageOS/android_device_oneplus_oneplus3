/*
 * Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
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

#define LOG_TAG "offload_effect_equalizer"
//#define LOG_NDEBUG 0

#include <cutils/list.h>
#include <cutils/log.h>
#include <tinyalsa/asoundlib.h>
#include <sound/audio_effects.h>
#include <audio_effects/effect_equalizer.h>

#include "effect_api.h"
#include "equalizer.h"

/* Offload equalizer UUID: a0dac280-401c-11e3-9379-0002a5d5c51b */
const effect_descriptor_t equalizer_descriptor = {
        {0x0bed4300, 0xddd6, 0x11db, 0x8f34, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // type
        {0xa0dac280, 0x401c, 0x11e3, 0x9379, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // uuid
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_HW_ACC_TUNNEL),
        0, /* TODO */
        1,
        "MSM offload equalizer",
        "The Android Open Source Project",
};

static const char *equalizer_preset_names[] = {
                                        "Normal",
                                        "Classical",
                                        "Dance",
                                        "Flat",
                                        "Folk",
                                        "Heavy Metal",
                                        "Hip Hop",
                                        "Jazz",
                                        "Pop",
                                        "Rock"
};

static const uint32_t equalizer_band_freq_range[NUM_EQ_BANDS][2] = {
                                       {30000, 120000},
                                       {120001, 460000},
                                       {460001, 1800000},
                                       {1800001, 7000000},
                                       {7000001, 20000000}};

static const int16_t equalizer_band_presets_level[] = {
                                        3, 0, 0, 0, 3,      /* Normal Preset */
                                        5, 3, -2, 4, 4,     /* Classical Preset */
                                        6, 0, 2, 4, 1,      /* Dance Preset */
                                        0, 0, 0, 0, 0,      /* Flat Preset */
                                        3, 0, 0, 2, -1,     /* Folk Preset */
                                        4, 1, 9, 3, 0,      /* Heavy Metal Preset */
                                        5, 3, 0, 1, 3,      /* Hip Hop Preset */
                                        4, 2, -2, 2, 5,     /* Jazz Preset */
                                       -1, 2, 5, 1, -2,     /* Pop Preset */
                                        5, 3, -1, 3, 5};    /* Rock Preset */

const uint16_t equalizer_band_presets_freq[NUM_EQ_BANDS] = {
                                        60,      /* Frequencies in Hz */
                                        230,
                                        910,
                                        3600,
                                        14000
};

/*
 * Equalizer operations
 */

int equalizer_get_band_level(equalizer_context_t *context, int32_t band)
{
    ALOGV("%s: ctxt %p, band: %d level: %d", __func__, context, band,
           context->band_levels[band] * 100);
    return context->band_levels[band] * 100;
}

int equalizer_set_band_level(equalizer_context_t *context, int32_t band,
                             int32_t level)
{
    ALOGV("%s: ctxt %p, band: %d, level: %d", __func__, context, band, level);
    if (level > 0) {
        level = (int)((level+50)/100);
    } else {
        level = (int)((level-50)/100);
    }
    context->band_levels[band] = level;
    context->preset = PRESET_CUSTOM;

    offload_eq_set_preset(&(context->offload_eq), PRESET_CUSTOM);
    offload_eq_set_bands_level(&(context->offload_eq),
                               NUM_EQ_BANDS,
                               equalizer_band_presets_freq,
                               context->band_levels);
    if (context->ctl)
        offload_eq_send_params(context->ctl, &context->offload_eq,
                               OFFLOAD_SEND_EQ_ENABLE_FLAG |
                               OFFLOAD_SEND_EQ_BANDS_LEVEL);
    if (context->hw_acc_fd > 0)
        hw_acc_eq_send_params(context->hw_acc_fd, &context->offload_eq,
                              OFFLOAD_SEND_EQ_ENABLE_FLAG |
                              OFFLOAD_SEND_EQ_BANDS_LEVEL);
    return 0;
}

int equalizer_get_center_frequency(equalizer_context_t *context, int32_t band)
{
    ALOGV("%s: ctxt %p, band: %d", __func__, context, band);
    return (equalizer_band_freq_range[band][0] +
            equalizer_band_freq_range[band][1]) / 2;
}

int equalizer_get_band_freq_range(equalizer_context_t *context, int32_t band,
                                  uint32_t *low, uint32_t *high)
{
    ALOGV("%s: ctxt %p, band: %d", __func__, context, band);
    *low = equalizer_band_freq_range[band][0];
    *high = equalizer_band_freq_range[band][1];
   return 0;
}

int equalizer_get_band(equalizer_context_t *context, uint32_t freq)
{
    int i;

    ALOGV("%s: ctxt %p, freq: %d", __func__, context, freq);
    for(i = 0; i < NUM_EQ_BANDS; i++) {
        if (freq <= equalizer_band_freq_range[i][1]) {
            return i;
        }
    }
    return NUM_EQ_BANDS - 1;
}

int equalizer_get_preset(equalizer_context_t *context)
{
    ALOGV("%s: ctxt %p, preset: %d", __func__, context, context->preset);
    return context->preset;
}

int equalizer_set_preset(equalizer_context_t *context, int preset)
{
    int i;

    ALOGV("%s: ctxt %p, preset: %d", __func__, context, preset);
    context->preset = preset;
    for (i=0; i<NUM_EQ_BANDS; i++)
        context->band_levels[i] =
                 equalizer_band_presets_level[i + preset * NUM_EQ_BANDS];

    offload_eq_set_preset(&(context->offload_eq), preset);
    offload_eq_set_bands_level(&(context->offload_eq),
                               NUM_EQ_BANDS,
                               equalizer_band_presets_freq,
                               context->band_levels);
    if(context->ctl)
        offload_eq_send_params(context->ctl, &context->offload_eq,
                               OFFLOAD_SEND_EQ_ENABLE_FLAG |
                               OFFLOAD_SEND_EQ_PRESET);
    if(context->hw_acc_fd > 0)
        hw_acc_eq_send_params(context->hw_acc_fd, &context->offload_eq,
                              OFFLOAD_SEND_EQ_ENABLE_FLAG |
                              OFFLOAD_SEND_EQ_PRESET);
    return 0;
}

const char * equalizer_get_preset_name(equalizer_context_t *context,
                                       int32_t preset)
{
    ALOGV("%s: ctxt %p, preset: %s", __func__, context,
                        equalizer_preset_names[preset]);
    if (preset == PRESET_CUSTOM) {
        return "Custom";
    } else {
        return equalizer_preset_names[preset];
    }
}

int equalizer_get_num_presets(equalizer_context_t *context)
{
    ALOGV("%s: ctxt %p, presets_num: %d", __func__, context,
           sizeof(equalizer_preset_names)/sizeof(char *));
    return sizeof(equalizer_preset_names)/sizeof(char *);
}

int equalizer_get_parameter(effect_context_t *context, effect_param_t *p,
                            uint32_t *size)
{
    equalizer_context_t *eq_ctxt = (equalizer_context_t *)context;
    int voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);
    int32_t *param_tmp = (int32_t *)p->data;
    int32_t param = *param_tmp++;
    int32_t param2;
    char *name;
    void *value = p->data + voffset;
    int i;

    ALOGV("%s: ctxt %p, param %d", __func__, eq_ctxt, param);

    p->status = 0;

    switch (param) {
    case EQ_PARAM_NUM_BANDS:
    case EQ_PARAM_CUR_PRESET:
    case EQ_PARAM_GET_NUM_OF_PRESETS:
    case EQ_PARAM_BAND_LEVEL:
    case EQ_PARAM_GET_BAND:
        if (p->vsize < sizeof(int16_t))
           p->status = -EINVAL;
        p->vsize = sizeof(int16_t);
        break;

    case EQ_PARAM_LEVEL_RANGE:
        if (p->vsize < 2 * sizeof(int16_t))
            p->status = -EINVAL;
        p->vsize = 2 * sizeof(int16_t);
        break;
    case EQ_PARAM_BAND_FREQ_RANGE:
       if (p->vsize < 2 * sizeof(int32_t))
            p->status = -EINVAL;
        p->vsize = 2 * sizeof(int32_t);
        break;

   case EQ_PARAM_CENTER_FREQ:
        if (p->vsize < sizeof(int32_t))
            p->status = -EINVAL;
        p->vsize = sizeof(int32_t);
        break;

    case EQ_PARAM_GET_PRESET_NAME:
        break;

    case EQ_PARAM_PROPERTIES:
        if (p->vsize < (2 + NUM_EQ_BANDS) * sizeof(uint16_t))
            p->status = -EINVAL;
        p->vsize = (2 + NUM_EQ_BANDS) * sizeof(uint16_t);
        break;

    default:
        p->status = -EINVAL;
    }

    *size = sizeof(effect_param_t) + voffset + p->vsize;

    if (p->status != 0)
        return 0;

    switch (param) {
    case EQ_PARAM_NUM_BANDS:
        *(uint16_t *)value = (uint16_t)NUM_EQ_BANDS;
        break;

    case EQ_PARAM_LEVEL_RANGE:
        *(int16_t *)value = -1500;
        *((int16_t *)value + 1) = 1500;
        break;

    case EQ_PARAM_BAND_LEVEL:
        param2 = *param_tmp;
        if (param2 >= NUM_EQ_BANDS) {
            p->status = -EINVAL;
            break;
        }
        *(int16_t *)value = (int16_t)equalizer_get_band_level(eq_ctxt, param2);
        break;

    case EQ_PARAM_CENTER_FREQ:
        param2 = *param_tmp;
        if (param2 >= NUM_EQ_BANDS) {
           p->status = -EINVAL;
            break;
        }
        *(int32_t *)value = equalizer_get_center_frequency(eq_ctxt, param2);
        break;

    case EQ_PARAM_BAND_FREQ_RANGE:
        param2 = *param_tmp;
        if (param2 >= NUM_EQ_BANDS) {
            p->status = -EINVAL;
           break;
        }
       equalizer_get_band_freq_range(eq_ctxt, param2, (uint32_t *)value,
                                     ((uint32_t *)value + 1));
        break;

    case EQ_PARAM_GET_BAND:
        param2 = *param_tmp;
        *(uint16_t *)value = (uint16_t)equalizer_get_band(eq_ctxt, param2);
        break;

    case EQ_PARAM_CUR_PRESET:
        *(uint16_t *)value = (uint16_t)equalizer_get_preset(eq_ctxt);
        break;

    case EQ_PARAM_GET_NUM_OF_PRESETS:
        *(uint16_t *)value = (uint16_t)equalizer_get_num_presets(eq_ctxt);
        break;

    case EQ_PARAM_GET_PRESET_NAME:
        param2 = *param_tmp;
        ALOGV("%s: EQ_PARAM_GET_PRESET_NAME: param2: %d", __func__, param2);
        if (param2 >= equalizer_get_num_presets(eq_ctxt)) {
            p->status = -EINVAL;
            break;
        }
        name = (char *)value;
        strlcpy(name, equalizer_get_preset_name(eq_ctxt, param2), p->vsize - 1);
        name[p->vsize - 1] = 0;
        p->vsize = strlen(name) + 1;
        break;

    case EQ_PARAM_PROPERTIES: {
        int16_t *prop = (int16_t *)value;
        prop[0] = (int16_t)equalizer_get_preset(eq_ctxt);
        prop[1] = (int16_t)NUM_EQ_BANDS;
        for (i = 0; i < NUM_EQ_BANDS; i++) {
            prop[2 + i] = (int16_t)equalizer_get_band_level(eq_ctxt, i);
        }
    } break;

    default:
        p->status = -EINVAL;
        break;
    }

    return 0;
}

int equalizer_set_parameter(effect_context_t *context, effect_param_t *p,
                            uint32_t size __unused)
{
    equalizer_context_t *eq_ctxt = (equalizer_context_t *)context;
    int voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);
    void *value = p->data + voffset;
    int32_t *param_tmp = (int32_t *)p->data;
    int32_t param = *param_tmp++;
    int32_t preset;
    int32_t band;
    int32_t level;
    int i;

    ALOGV("%s: ctxt %p, param %d", __func__, eq_ctxt, param);

    p->status = 0;

    switch (param) {
    case EQ_PARAM_CUR_PRESET:
        preset = (int32_t)(*(uint16_t *)value);

        if ((preset >= equalizer_get_num_presets(eq_ctxt)) || (preset < 0)) {
           p->status = -EINVAL;
            break;
        }
        equalizer_set_preset(eq_ctxt, preset);
        break;
    case EQ_PARAM_BAND_LEVEL:
        band =  *param_tmp;
        level = (int32_t)(*(int16_t *)value);
        if (band >= NUM_EQ_BANDS) {
           p->status = -EINVAL;
            break;
        }
        equalizer_set_band_level(eq_ctxt, band, level);
        break;
    case EQ_PARAM_PROPERTIES: {
        int16_t *prop = (int16_t *)value;
        if ((int)prop[0] >= equalizer_get_num_presets(eq_ctxt)) {
            p->status = -EINVAL;
            break;
        }
        if (prop[0] >= 0) {
            equalizer_set_preset(eq_ctxt, (int)prop[0]);
        } else {
            if ((int)prop[1] != NUM_EQ_BANDS) {
                p->status = -EINVAL;
                break;
            }
            for (i = 0; i < NUM_EQ_BANDS; i++) {
               equalizer_set_band_level(eq_ctxt, i, (int)prop[2 + i]);
            }
        }
    } break;
    default:
        p->status = -EINVAL;
        break;
    }

    return 0;
}

int equalizer_set_device(effect_context_t *context,  uint32_t device)
{
    ALOGV("%s: ctxt %p, device: 0x%x", __func__, context, device);
    equalizer_context_t *eq_ctxt = (equalizer_context_t *)context;
    eq_ctxt->device = device;
    offload_eq_set_device(&(eq_ctxt->offload_eq), device);
    return 0;
}

int equalizer_reset(effect_context_t *context)
{
    equalizer_context_t *eq_ctxt = (equalizer_context_t *)context;

    return 0;
}

int equalizer_init(effect_context_t *context)
{
    ALOGV("%s: ctxt %p", __func__, context);
    equalizer_context_t *eq_ctxt = (equalizer_context_t *)context;

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

    eq_ctxt->hw_acc_fd = -1;
    memset(&(eq_ctxt->offload_eq), 0, sizeof(struct eq_params));
    offload_eq_set_preset(&(eq_ctxt->offload_eq), INVALID_PRESET);

    return 0;
}

int equalizer_enable(effect_context_t *context)
{
    equalizer_context_t *eq_ctxt = (equalizer_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, context);

    if (!offload_eq_get_enable_flag(&(eq_ctxt->offload_eq))) {
        offload_eq_set_enable_flag(&(eq_ctxt->offload_eq), true);
        if (eq_ctxt->ctl)
            offload_eq_send_params(eq_ctxt->ctl, &eq_ctxt->offload_eq,
                                   OFFLOAD_SEND_EQ_ENABLE_FLAG |
                                   OFFLOAD_SEND_EQ_BANDS_LEVEL);
        if (eq_ctxt->hw_acc_fd > 0)
            hw_acc_eq_send_params(eq_ctxt->hw_acc_fd, &eq_ctxt->offload_eq,
                                  OFFLOAD_SEND_EQ_ENABLE_FLAG |
                                  OFFLOAD_SEND_EQ_BANDS_LEVEL);
    }
    return 0;
}

int equalizer_disable(effect_context_t *context)
{
    equalizer_context_t *eq_ctxt = (equalizer_context_t *)context;

    ALOGV("%s:ctxt %p", __func__, eq_ctxt);
    if (offload_eq_get_enable_flag(&(eq_ctxt->offload_eq))) {
        offload_eq_set_enable_flag(&(eq_ctxt->offload_eq), false);
        if (eq_ctxt->ctl)
            offload_eq_send_params(eq_ctxt->ctl, &eq_ctxt->offload_eq,
                                   OFFLOAD_SEND_EQ_ENABLE_FLAG);
        if (eq_ctxt->hw_acc_fd > 0)
            hw_acc_eq_send_params(eq_ctxt->hw_acc_fd, &eq_ctxt->offload_eq,
                                  OFFLOAD_SEND_EQ_ENABLE_FLAG);
    }
    return 0;
}

int equalizer_start(effect_context_t *context, output_context_t *output)
{
    equalizer_context_t *eq_ctxt = (equalizer_context_t *)context;

    ALOGV("%s: ctxt %p, ctl %p", __func__, eq_ctxt, output->ctl);
    eq_ctxt->ctl = output->ctl;
    if (offload_eq_get_enable_flag(&(eq_ctxt->offload_eq))) {
        if (eq_ctxt->ctl)
            offload_eq_send_params(eq_ctxt->ctl, &eq_ctxt->offload_eq,
                                   OFFLOAD_SEND_EQ_ENABLE_FLAG |
                                   OFFLOAD_SEND_EQ_BANDS_LEVEL);
        if (eq_ctxt->hw_acc_fd > 0)
            hw_acc_eq_send_params(eq_ctxt->hw_acc_fd, &eq_ctxt->offload_eq,
                                  OFFLOAD_SEND_EQ_ENABLE_FLAG |
                                  OFFLOAD_SEND_EQ_BANDS_LEVEL);
    }
    return 0;
}

int equalizer_stop(effect_context_t *context, output_context_t *output __unused)
{
    equalizer_context_t *eq_ctxt = (equalizer_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, eq_ctxt);
    if (offload_eq_get_enable_flag(&(eq_ctxt->offload_eq)) &&
        eq_ctxt->ctl) {
        struct eq_params eq;
        eq.enable_flag = false;
        offload_eq_send_params(eq_ctxt->ctl, &eq, OFFLOAD_SEND_EQ_ENABLE_FLAG);
    }
    eq_ctxt->ctl = NULL;
    return 0;
}

int equalizer_set_mode(effect_context_t *context, int32_t hw_acc_fd)
{
    equalizer_context_t *eq_ctxt = (equalizer_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, eq_ctxt);
    eq_ctxt->hw_acc_fd = hw_acc_fd;
    if ((eq_ctxt->hw_acc_fd > 0) &&
        (offload_eq_get_enable_flag(&(eq_ctxt->offload_eq))))
        hw_acc_eq_send_params(eq_ctxt->hw_acc_fd, &eq_ctxt->offload_eq,
                              OFFLOAD_SEND_EQ_ENABLE_FLAG |
                              OFFLOAD_SEND_EQ_BANDS_LEVEL);
    return 0;
}
