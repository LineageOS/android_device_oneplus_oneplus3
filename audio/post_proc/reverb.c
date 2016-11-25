/*
 * Copyright (c) 2013 - 2014, The Linux Foundation. All rights reserved.
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

#define LOG_TAG "offload_effect_reverb"
//#define LOG_NDEBUG 0

#include <cutils/list.h>
#include <cutils/log.h>
#include <tinyalsa/asoundlib.h>
#include <sound/audio_effects.h>
#include <audio_effects/effect_environmentalreverb.h>
#include <audio_effects/effect_presetreverb.h>

#include "effect_api.h"
#include "reverb.h"

/* Offload auxiliary environmental reverb UUID: 79a18026-18fd-4185-8233-0002a5d5c51b */
const effect_descriptor_t aux_env_reverb_descriptor = {
        { 0xc2e5d5f0, 0x94bd, 0x4763, 0x9cac, { 0x4e, 0x23, 0x4d, 0x06, 0x83, 0x9e } },
        { 0x79a18026, 0x18fd, 0x4185, 0x8233, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } },
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_AUXILIARY | EFFECT_FLAG_HW_ACC_TUNNEL),
        0, /* TODO */
        1,
        "MSM offload Auxiliary Environmental Reverb",
        "The Android Open Source Project",
};

/* Offload insert environmental reverb UUID: eb64ea04-973b-43d2-8f5e-0002a5d5c51b */
const effect_descriptor_t ins_env_reverb_descriptor = {
        {0xc2e5d5f0, 0x94bd, 0x4763, 0x9cac, {0x4e, 0x23, 0x4d, 0x06, 0x83, 0x9e}},
        {0xeb64ea04, 0x973b, 0x43d2, 0x8f5e, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}},
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_INSERT_FIRST | EFFECT_FLAG_HW_ACC_TUNNEL),
        0, /* TODO */
        1,
        "MSM offload Insert Environmental Reverb",
        "The Android Open Source Project",
};

// Offload auxiliary preset reverb UUID: 6987be09-b142-4b41-9056-0002a5d5c51b */
const effect_descriptor_t aux_preset_reverb_descriptor = {
        {0x47382d60, 0xddd8, 0x11db, 0xbf3a, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}},
        {0x6987be09, 0xb142, 0x4b41, 0x9056, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}},
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_AUXILIARY | EFFECT_FLAG_HW_ACC_TUNNEL),
        0, /* TODO */
        1,
        "MSM offload Auxiliary Preset Reverb",
        "The Android Open Source Project",
};

// Offload insert preset reverb UUID: aa2bebf6-47cf-4613-9bca-0002a5d5c51b */
const effect_descriptor_t ins_preset_reverb_descriptor = {
        {0x47382d60, 0xddd8, 0x11db, 0xbf3a, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}},
        {0xaa2bebf6, 0x47cf, 0x4613, 0x9bca, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}},
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_INSERT_FIRST | EFFECT_FLAG_HW_ACC_TUNNEL),
        0, /* TODO */
        1,
        "MSM offload Insert Preset Reverb",
        "The Android Open Source Project",
};

static const reverb_settings_t reverb_presets[] = {
        // REVERB_PRESET_NONE: values are unused
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        // REVERB_PRESET_SMALLROOM
        {-400, -600, 1100, 830, -400, 5, 500, 10, 1000, 1000},
        // REVERB_PRESET_MEDIUMROOM
        {-400, -600, 1300, 830, -1000, 20, -200, 20, 1000, 1000},
        // REVERB_PRESET_LARGEROOM
        {-400, -600, 1500, 830, -1600, 5, -1000, 40, 1000, 1000},
        // REVERB_PRESET_MEDIUMHALL
        {-400, -600, 1800, 700, -1300, 15, -800, 30, 1000, 1000},
        // REVERB_PRESET_LARGEHALL
        {-400, -600, 1800, 700, -2000, 30, -1400, 60, 1000, 1000},
        // REVERB_PRESET_PLATE
        {-400, -200, 1300, 900, 0, 2, 0, 10, 1000, 750},
};


void reverb_auxiliary_init(reverb_context_t *context)
{
    context->auxiliary = true;
    context->preset = false;
}

void reverb_insert_init(reverb_context_t *context)
{
    context->auxiliary = false;
    context->preset = true;
    context->cur_preset = REVERB_PRESET_LAST + 1;
    context->next_preset = REVERB_DEFAULT_PRESET;
}

void reverb_preset_init(reverb_context_t *context)
{
    context->auxiliary = false;
    context->preset = true;
    context->cur_preset = REVERB_PRESET_LAST + 1;
    context->next_preset = REVERB_DEFAULT_PRESET;
}

/*
 * Reverb operations
 */
int16_t reverb_get_room_level(reverb_context_t *context)
{
    ALOGV("%s: ctxt %p, room level: %d", __func__, context, context->reverb_settings.roomLevel);
    return context->reverb_settings.roomLevel;
}

void reverb_set_room_level(reverb_context_t *context, int16_t room_level)
{
    ALOGV("%s: ctxt %p, room level: %d", __func__, context, room_level);
    context->reverb_settings.roomLevel = room_level;
    offload_reverb_set_room_level(&(context->offload_reverb), room_level);
    if (context->ctl)
        offload_reverb_send_params(context->ctl, &context->offload_reverb,
                                   OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                   OFFLOAD_SEND_REVERB_ROOM_LEVEL);
    if (context->hw_acc_fd > 0)
        hw_acc_reverb_send_params(context->hw_acc_fd, &context->offload_reverb,
                                  OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                  OFFLOAD_SEND_REVERB_ROOM_LEVEL);
}

int16_t reverb_get_room_hf_level(reverb_context_t *context)
{
    ALOGV("%s: ctxt %p, room hf level: %d", __func__, context,
          context->reverb_settings.roomHFLevel);
    return context->reverb_settings.roomHFLevel;
}

void reverb_set_room_hf_level(reverb_context_t *context, int16_t room_hf_level)
{
    ALOGV("%s: ctxt %p, room hf level: %d", __func__, context, room_hf_level);
    context->reverb_settings.roomHFLevel = room_hf_level;
    offload_reverb_set_room_hf_level(&(context->offload_reverb), room_hf_level);
    if (context->ctl)
        offload_reverb_send_params(context->ctl, &context->offload_reverb,
                                   OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                   OFFLOAD_SEND_REVERB_ROOM_HF_LEVEL);
    if (context->hw_acc_fd > 0)
        hw_acc_reverb_send_params(context->hw_acc_fd, &context->offload_reverb,
                                  OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                  OFFLOAD_SEND_REVERB_ROOM_HF_LEVEL);
}

uint32_t reverb_get_decay_time(reverb_context_t *context)
{
    ALOGV("%s: ctxt %p, decay time: %d", __func__, context,
                         context->reverb_settings.decayTime);
    return context->reverb_settings.decayTime;
}

void reverb_set_decay_time(reverb_context_t *context, uint32_t decay_time)
{
    ALOGV("%s: ctxt %p, decay_time: %d", __func__, context, decay_time);
    context->reverb_settings.decayTime = decay_time;
    offload_reverb_set_decay_time(&(context->offload_reverb), decay_time);
    if (context->ctl)
        offload_reverb_send_params(context->ctl, &context->offload_reverb,
                                   OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                   OFFLOAD_SEND_REVERB_DECAY_TIME);
    if (context->hw_acc_fd > 0)
        hw_acc_reverb_send_params(context->hw_acc_fd, &context->offload_reverb,
                                  OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                  OFFLOAD_SEND_REVERB_DECAY_TIME);
}

int16_t reverb_get_decay_hf_ratio(reverb_context_t *context)
{
    ALOGV("%s: ctxt %p, decay hf ratio: %d", __func__, context,
          context->reverb_settings.decayHFRatio);
    return context->reverb_settings.decayHFRatio;
}

void reverb_set_decay_hf_ratio(reverb_context_t *context, int16_t decay_hf_ratio)
{
    ALOGV("%s: ctxt %p, decay_hf_ratio: %d", __func__, context, decay_hf_ratio);
    context->reverb_settings.decayHFRatio = decay_hf_ratio;
    offload_reverb_set_decay_hf_ratio(&(context->offload_reverb), decay_hf_ratio);
    if (context->ctl)
        offload_reverb_send_params(context->ctl, &context->offload_reverb,
                                   OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                   OFFLOAD_SEND_REVERB_DECAY_HF_RATIO);
    if (context->hw_acc_fd > 0)
        hw_acc_reverb_send_params(context->hw_acc_fd, &context->offload_reverb,
                                  OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                  OFFLOAD_SEND_REVERB_DECAY_HF_RATIO);
}

int16_t reverb_get_reverb_level(reverb_context_t *context)
{
    ALOGV("%s: ctxt %p, reverb level: %d", __func__, context,
                         context->reverb_settings.reverbLevel);
    return context->reverb_settings.reverbLevel;
}

void reverb_set_reverb_level(reverb_context_t *context, int16_t reverb_level)
{
    ALOGV("%s: ctxt %p, reverb level: %d", __func__, context, reverb_level);
    context->reverb_settings.reverbLevel = reverb_level;
    offload_reverb_set_reverb_level(&(context->offload_reverb), reverb_level);
    if (context->ctl)
        offload_reverb_send_params(context->ctl, &context->offload_reverb,
                                   OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                   OFFLOAD_SEND_REVERB_LEVEL);
    if (context->hw_acc_fd > 0)
        hw_acc_reverb_send_params(context->hw_acc_fd, &context->offload_reverb,
                                  OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                  OFFLOAD_SEND_REVERB_LEVEL);
}

uint32_t reverb_get_reverb_delay(reverb_context_t *context)
{
    ALOGV("%s: ctxt %p, reverb delay: %d", __func__, context,
                          context->reverb_settings.reverbDelay);
    return context->reverb_settings.reverbDelay;
}

void reverb_set_reverb_delay(reverb_context_t *context, uint32_t delay)
{
    ALOGV("%s: ctxt %p, reverb delay: %d", __func__, context, delay);
    context->reverb_settings.reverbDelay = delay;
    offload_reverb_set_delay(&(context->offload_reverb), delay);
    if (context->ctl)
        offload_reverb_send_params(context->ctl, &context->offload_reverb,
                                   OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                   OFFLOAD_SEND_REVERB_DELAY);
    if (context->hw_acc_fd > 0)
        hw_acc_reverb_send_params(context->hw_acc_fd, &context->offload_reverb,
                                  OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                  OFFLOAD_SEND_REVERB_DELAY);
}

int16_t reverb_get_reflections_level(reverb_context_t *context)
{
    ALOGV("%s: ctxt %p, reflection level: %d", __func__, context,
                          context->reverb_settings.reflectionsLevel);
    return context->reverb_settings.reflectionsLevel;
}

void reverb_set_reflections_level(reverb_context_t *context, int16_t level)
{
    ALOGV("%s: ctxt %p, reflection level: %d", __func__, context, level);
    context->reverb_settings.reflectionsLevel = level;
    offload_reverb_set_reflections_level(&(context->offload_reverb), level);
    if (context->ctl)
        offload_reverb_send_params(context->ctl, &context->offload_reverb,
                                   OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                   OFFLOAD_SEND_REVERB_REFLECTIONS_LEVEL);
    if (context->hw_acc_fd > 0)
        hw_acc_reverb_send_params(context->hw_acc_fd, &context->offload_reverb,
                                  OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                  OFFLOAD_SEND_REVERB_REFLECTIONS_LEVEL);
}

uint32_t reverb_get_reflections_delay(reverb_context_t *context)
{
    ALOGV("%s: ctxt %p, reflection delay: %d", __func__, context,
                          context->reverb_settings.reflectionsDelay);
    return context->reverb_settings.reflectionsDelay;
}

void reverb_set_reflections_delay(reverb_context_t *context, uint32_t delay)
{
    ALOGV("%s: ctxt %p, reflection delay: %d", __func__, context, delay);
    context->reverb_settings.reflectionsDelay = delay;
    offload_reverb_set_reflections_delay(&(context->offload_reverb), delay);
    if (context->ctl)
        offload_reverb_send_params(context->ctl, &context->offload_reverb,
                                   OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                   OFFLOAD_SEND_REVERB_REFLECTIONS_DELAY);
    if (context->hw_acc_fd > 0)
        hw_acc_reverb_send_params(context->hw_acc_fd, &context->offload_reverb,
                                  OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                  OFFLOAD_SEND_REVERB_REFLECTIONS_DELAY);
}

int16_t reverb_get_diffusion(reverb_context_t *context)
{
    ALOGV("%s: ctxt %p, diffusion: %d", __func__, context,
                        context->reverb_settings.diffusion);
    return context->reverb_settings.diffusion;
}

void reverb_set_diffusion(reverb_context_t *context, int16_t diffusion)
{
    ALOGV("%s: ctxt %p, diffusion: %d", __func__, context, diffusion);
    context->reverb_settings.diffusion = diffusion;
    offload_reverb_set_diffusion(&(context->offload_reverb), diffusion);
    if (context->ctl)
        offload_reverb_send_params(context->ctl, &context->offload_reverb,
                                   OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                   OFFLOAD_SEND_REVERB_DIFFUSION);
    if (context->hw_acc_fd > 0)
        hw_acc_reverb_send_params(context->hw_acc_fd, &context->offload_reverb,
                                  OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                  OFFLOAD_SEND_REVERB_DIFFUSION);
}

int16_t reverb_get_density(reverb_context_t *context)
{
    ALOGV("%s: ctxt %p, density: %d", __func__, context,
                          context->reverb_settings.density);
    return context->reverb_settings.density;
}

void reverb_set_density(reverb_context_t *context, int16_t density)
{
    ALOGV("%s: ctxt %p, density: %d", __func__, context, density);
    context->reverb_settings.density = density;
    offload_reverb_set_density(&(context->offload_reverb), density);
    if (context->ctl)
        offload_reverb_send_params(context->ctl, &context->offload_reverb,
                                   OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                   OFFLOAD_SEND_REVERB_DENSITY);
    if (context->hw_acc_fd > 0)
        hw_acc_reverb_send_params(context->hw_acc_fd, &context->offload_reverb,
                                  OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                  OFFLOAD_SEND_REVERB_DENSITY);
}

void reverb_set_preset(reverb_context_t *context, int16_t preset)
{
    bool enable;
    ALOGV("%s: ctxt %p, preset: %d", __func__, context, preset);
    context->next_preset = preset;
    offload_reverb_set_preset(&(context->offload_reverb), preset);

    if (context->enabled_by_client) {
        enable = (preset == REVERB_PRESET_NONE) ? false: true;
        offload_reverb_set_enable_flag(&(context->offload_reverb), enable);

        if (context->ctl)
            offload_reverb_send_params(context->ctl, &context->offload_reverb,
                                   OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                   OFFLOAD_SEND_REVERB_PRESET);
        if (context->hw_acc_fd > 0)
            hw_acc_reverb_send_params(context->hw_acc_fd, &context->offload_reverb,
                                  OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                  OFFLOAD_SEND_REVERB_PRESET);
    }
}

void reverb_set_all_properties(reverb_context_t *context,
                               reverb_settings_t *reverb_settings)
{
    ALOGV("%s: ctxt %p", __func__, context);
    context->reverb_settings.roomLevel = reverb_settings->roomLevel;
    context->reverb_settings.roomHFLevel = reverb_settings->roomHFLevel;
    context->reverb_settings.decayTime = reverb_settings->decayTime;
    context->reverb_settings.decayHFRatio = reverb_settings->decayHFRatio;
    context->reverb_settings.reverbLevel = reverb_settings->reverbLevel;
    context->reverb_settings.reverbDelay = reverb_settings->reverbDelay;
    context->reverb_settings.reflectionsLevel = reverb_settings->reflectionsLevel;
    context->reverb_settings.reflectionsDelay = reverb_settings->reflectionsDelay;
    context->reverb_settings.diffusion = reverb_settings->diffusion;
    context->reverb_settings.density = reverb_settings->density;

    offload_reverb_set_room_level(&(context->offload_reverb),
                               reverb_settings->roomLevel);
    offload_reverb_set_room_hf_level(&(context->offload_reverb),
                               reverb_settings->roomHFLevel);
    offload_reverb_set_decay_time(&(context->offload_reverb),
                               reverb_settings->decayTime);
    offload_reverb_set_decay_hf_ratio(&(context->offload_reverb),
                               reverb_settings->decayHFRatio);
    offload_reverb_set_reverb_level(&(context->offload_reverb),
                               reverb_settings->reverbLevel);
    offload_reverb_set_delay(&(context->offload_reverb),
                               reverb_settings->reverbDelay);
    offload_reverb_set_reflections_level(&(context->offload_reverb),
                               reverb_settings->reflectionsLevel);
    offload_reverb_set_reflections_delay(&(context->offload_reverb),
                               reverb_settings->reflectionsDelay);
    offload_reverb_set_diffusion(&(context->offload_reverb),
                               reverb_settings->diffusion);
    offload_reverb_set_density(&(context->offload_reverb),
                               reverb_settings->density);

    if (context->ctl)
        offload_reverb_send_params(context->ctl, &context->offload_reverb,
                                   OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                   OFFLOAD_SEND_REVERB_ROOM_LEVEL |
                                   OFFLOAD_SEND_REVERB_ROOM_HF_LEVEL |
                                   OFFLOAD_SEND_REVERB_DECAY_TIME |
                                   OFFLOAD_SEND_REVERB_DECAY_HF_RATIO |
                                   OFFLOAD_SEND_REVERB_LEVEL |
                                   OFFLOAD_SEND_REVERB_DELAY |
                                   OFFLOAD_SEND_REVERB_REFLECTIONS_LEVEL |
                                   OFFLOAD_SEND_REVERB_REFLECTIONS_DELAY |
                                   OFFLOAD_SEND_REVERB_DIFFUSION |
                                   OFFLOAD_SEND_REVERB_DENSITY);
    if (context->hw_acc_fd > 0)
        hw_acc_reverb_send_params(context->hw_acc_fd, &context->offload_reverb,
                                  OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                  OFFLOAD_SEND_REVERB_ROOM_LEVEL |
                                  OFFLOAD_SEND_REVERB_ROOM_HF_LEVEL |
                                  OFFLOAD_SEND_REVERB_DECAY_TIME |
                                  OFFLOAD_SEND_REVERB_DECAY_HF_RATIO |
                                  OFFLOAD_SEND_REVERB_LEVEL |
                                  OFFLOAD_SEND_REVERB_DELAY |
                                  OFFLOAD_SEND_REVERB_REFLECTIONS_LEVEL |
                                  OFFLOAD_SEND_REVERB_REFLECTIONS_DELAY |
                                  OFFLOAD_SEND_REVERB_DIFFUSION |
                                  OFFLOAD_SEND_REVERB_DENSITY);
}

void reverb_load_preset(reverb_context_t *context)
{
    context->cur_preset = context->next_preset;

    if (context->cur_preset != REVERB_PRESET_NONE) {
        const reverb_settings_t *preset = &reverb_presets[context->cur_preset];
        reverb_set_room_level(context, preset->roomLevel);
        reverb_set_room_hf_level(context, preset->roomHFLevel);
        reverb_set_decay_time(context, preset->decayTime);
        reverb_set_decay_hf_ratio(context, preset->decayHFRatio);
        reverb_set_reverb_level(context, preset->reverbLevel);
        reverb_set_diffusion(context, preset->diffusion);
        reverb_set_density(context, preset->density);
    }
}

int reverb_get_parameter(effect_context_t *context, effect_param_t *p,
                         uint32_t *size)
{
    reverb_context_t *reverb_ctxt = (reverb_context_t *)context;
    int voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);
    int32_t *param_tmp = (int32_t *)p->data;
    int32_t param = *param_tmp++;
    void *value = p->data + voffset;
    reverb_settings_t *reverb_settings;
    int i;

    ALOGV("%s: ctxt %p, param %d", __func__, reverb_ctxt, param);

    p->status = 0;

    if (reverb_ctxt->preset) {
        if (param != REVERB_PARAM_PRESET || p->vsize < sizeof(uint16_t))
            return -EINVAL;
        *(uint16_t *)value = reverb_ctxt->next_preset;
        ALOGV("get REVERB_PARAM_PRESET, preset %d", reverb_ctxt->next_preset);
        return 0;
    }
    switch (param) {
    case REVERB_PARAM_ROOM_LEVEL:
        if (p->vsize < sizeof(uint16_t))
           p->status = -EINVAL;
        p->vsize = sizeof(uint16_t);
        break;
    case REVERB_PARAM_ROOM_HF_LEVEL:
        if (p->vsize < sizeof(uint16_t))
           p->status = -EINVAL;
        p->vsize = sizeof(uint16_t);
        break;
    case REVERB_PARAM_DECAY_TIME:
        if (p->vsize < sizeof(uint32_t))
           p->status = -EINVAL;
        p->vsize = sizeof(uint32_t);
        break;
    case REVERB_PARAM_DECAY_HF_RATIO:
        if (p->vsize < sizeof(uint16_t))
           p->status = -EINVAL;
        p->vsize = sizeof(uint16_t);
        break;
    case REVERB_PARAM_REVERB_LEVEL:
        if (p->vsize < sizeof(uint16_t))
           p->status = -EINVAL;
        p->vsize = sizeof(uint16_t);
        break;
    case REVERB_PARAM_REVERB_DELAY:
        if (p->vsize < sizeof(uint32_t))
           p->status = -EINVAL;
        p->vsize = sizeof(uint32_t);
        break;
    case REVERB_PARAM_REFLECTIONS_LEVEL:
        if (p->vsize < sizeof(uint16_t))
           p->status = -EINVAL;
        p->vsize = sizeof(uint16_t);
        break;
    case REVERB_PARAM_REFLECTIONS_DELAY:
        if (p->vsize < sizeof(uint32_t))
           p->status = -EINVAL;
        p->vsize = sizeof(uint32_t);
        break;
    case REVERB_PARAM_DIFFUSION:
        if (p->vsize < sizeof(uint16_t))
           p->status = -EINVAL;
        p->vsize = sizeof(uint16_t);
        break;
    case REVERB_PARAM_DENSITY:
        if (p->vsize < sizeof(uint16_t))
           p->status = -EINVAL;
        p->vsize = sizeof(uint16_t);
        break;
    case REVERB_PARAM_PROPERTIES:
        if (p->vsize < sizeof(reverb_settings_t))
           p->status = -EINVAL;
        p->vsize = sizeof(reverb_settings_t);
        break;
    default:
        p->status = -EINVAL;
    }

    *size = sizeof(effect_param_t) + voffset + p->vsize;

    if (p->status != 0)
        return 0;

    switch (param) {
    case REVERB_PARAM_ROOM_LEVEL:
        *(int16_t *)value = reverb_get_room_level(reverb_ctxt);
        break;
    case REVERB_PARAM_ROOM_HF_LEVEL:
        *(int16_t *)value = reverb_get_room_hf_level(reverb_ctxt);
        break;
    case REVERB_PARAM_DECAY_TIME:
        *(uint32_t *)value = reverb_get_decay_time(reverb_ctxt);
        break;
    case REVERB_PARAM_DECAY_HF_RATIO:
        *(int16_t *)value = reverb_get_decay_hf_ratio(reverb_ctxt);
        break;
    case REVERB_PARAM_REVERB_LEVEL:
        *(int16_t *)value = reverb_get_reverb_level(reverb_ctxt);
        break;
    case REVERB_PARAM_REVERB_DELAY:
        *(uint32_t *)value = reverb_get_reverb_delay(reverb_ctxt);
        break;
    case REVERB_PARAM_REFLECTIONS_LEVEL:
        *(int16_t *)value = reverb_get_reflections_level(reverb_ctxt);
        break;
    case REVERB_PARAM_REFLECTIONS_DELAY:
        *(uint32_t *)value = reverb_get_reflections_delay(reverb_ctxt);
        break;
    case REVERB_PARAM_DIFFUSION:
        *(int16_t *)value = reverb_get_diffusion(reverb_ctxt);
        break;
    case REVERB_PARAM_DENSITY:
        *(int16_t *)value = reverb_get_density(reverb_ctxt);
        break;
    case REVERB_PARAM_PROPERTIES:
        reverb_settings = (reverb_settings_t *)value;
        reverb_settings->roomLevel = reverb_get_room_level(reverb_ctxt);
        reverb_settings->roomHFLevel = reverb_get_room_hf_level(reverb_ctxt);
        reverb_settings->decayTime = reverb_get_decay_time(reverb_ctxt);
        reverb_settings->decayHFRatio = reverb_get_decay_hf_ratio(reverb_ctxt);
        reverb_settings->reverbLevel = reverb_get_reverb_level(reverb_ctxt);
        reverb_settings->reverbDelay = reverb_get_reverb_delay(reverb_ctxt);
        reverb_settings->reflectionsLevel = reverb_get_reflections_level(reverb_ctxt);
        reverb_settings->reflectionsDelay = reverb_get_reflections_delay(reverb_ctxt);
        reverb_settings->diffusion = reverb_get_diffusion(reverb_ctxt);
        reverb_settings->density = reverb_get_density(reverb_ctxt);
        break;
    default:
        p->status = -EINVAL;
        break;
    }

    return 0;
}

int reverb_set_parameter(effect_context_t *context, effect_param_t *p,
                         uint32_t size __unused)
{
    reverb_context_t *reverb_ctxt = (reverb_context_t *)context;
    int voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);
    void *value = p->data + voffset;
    int32_t *param_tmp = (int32_t *)p->data;
    int32_t param = *param_tmp++;
    reverb_settings_t *reverb_settings;
    int16_t level;
    int16_t ratio;
    uint32_t time;
    uint32_t delay;

    ALOGV("%s: ctxt %p, param %d", __func__, reverb_ctxt, param);

    p->status = 0;

    if (reverb_ctxt->preset) {
        if (param != REVERB_PARAM_PRESET)
            return -EINVAL;
        uint16_t preset = *(uint16_t *)value;
        ALOGV("set REVERB_PARAM_PRESET, preset %d", preset);
        if (preset > REVERB_PRESET_LAST) {
            return -EINVAL;
        }
        reverb_set_preset(reverb_ctxt, preset);
        return 0;
    }
    switch (param) {
    case REVERB_PARAM_PROPERTIES:
        reverb_settings = (reverb_settings_t *)value;
        reverb_set_all_properties(reverb_ctxt, reverb_settings);
        break;
    case REVERB_PARAM_ROOM_LEVEL:
        level = *(int16_t *)value;
        reverb_set_room_level(reverb_ctxt, level);
        break;
    case REVERB_PARAM_ROOM_HF_LEVEL:
        level = *(int16_t *)value;
        reverb_set_room_hf_level(reverb_ctxt, level);
        break;
    case REVERB_PARAM_DECAY_TIME:
        time = *(uint32_t *)value;
        reverb_set_decay_time(reverb_ctxt, time);
        break;
    case REVERB_PARAM_DECAY_HF_RATIO:
        ratio = *(int16_t *)value;
        reverb_set_decay_hf_ratio(reverb_ctxt, ratio);
        break;
    case REVERB_PARAM_REVERB_LEVEL:
        level = *(int16_t *)value;
        reverb_set_reverb_level(reverb_ctxt, level);
        break;
    case REVERB_PARAM_REVERB_DELAY:
        delay = *(uint32_t *)value;
        reverb_set_reverb_delay(reverb_ctxt, delay);
        break;
    case REVERB_PARAM_REFLECTIONS_LEVEL:
        level = *(int16_t *)value;
        reverb_set_reflections_level(reverb_ctxt, level);
        break;
    case REVERB_PARAM_REFLECTIONS_DELAY:
        delay = *(uint32_t *)value;
        reverb_set_reflections_delay(reverb_ctxt, delay);
        break;
    case REVERB_PARAM_DIFFUSION:
        ratio = *(int16_t *)value;
        reverb_set_diffusion(reverb_ctxt, ratio);
        break;
    case REVERB_PARAM_DENSITY:
        ratio = *(int16_t *)value;
        reverb_set_density(reverb_ctxt, ratio);
        break;
    default:
        p->status = -EINVAL;
        break;
    }

    return 0;
}

int reverb_set_device(effect_context_t *context, uint32_t device)
{
    reverb_context_t *reverb_ctxt = (reverb_context_t *)context;

    ALOGV("%s: ctxt %p, device: 0x%x", __func__, reverb_ctxt, device);
    reverb_ctxt->device = device;
    offload_reverb_set_device(&(reverb_ctxt->offload_reverb), device);
    return 0;
}

int reverb_reset(effect_context_t *context)
{
    reverb_context_t *reverb_ctxt = (reverb_context_t *)context;

    return 0;
}

int reverb_init(effect_context_t *context)
{
    reverb_context_t *reverb_ctxt = (reverb_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, reverb_ctxt);
    context->config.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
    /*
       FIXME: channel mode is mono for auxiliary. is it needed for offload ?
              If so, this set config needs to be updated accordingly
    */
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

    reverb_ctxt->hw_acc_fd = -1;
    reverb_ctxt->enabled_by_client = false;
    memset(&(reverb_ctxt->reverb_settings), 0, sizeof(reverb_settings_t));
    memset(&(reverb_ctxt->offload_reverb), 0, sizeof(struct reverb_params));

    if (reverb_ctxt->preset &&
        reverb_ctxt->next_preset != reverb_ctxt->cur_preset)
        reverb_load_preset(reverb_ctxt);

    return 0;
}

int reverb_enable(effect_context_t *context)
{
    reverb_context_t *reverb_ctxt = (reverb_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, reverb_ctxt);
    reverb_ctxt->enabled_by_client = true;

    /* REVERB_PRESET_NONE is equivalent to disabled state,
     * But support for this state is not provided in DSP.
     * Hence, do not set enable flag, if in peset mode with preset "NONE".
     * Effect would be enabled when valid preset is set.
     */
    if ((reverb_ctxt->preset == true) &&
        (reverb_ctxt->next_preset == REVERB_PRESET_NONE))
        return 0;

    if (!offload_reverb_get_enable_flag(&(reverb_ctxt->offload_reverb)))
        offload_reverb_set_enable_flag(&(reverb_ctxt->offload_reverb), true);
    return 0;
}

int reverb_disable(effect_context_t *context)
{
    reverb_context_t *reverb_ctxt = (reverb_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, reverb_ctxt);
    reverb_ctxt->enabled_by_client = false;
    if (offload_reverb_get_enable_flag(&(reverb_ctxt->offload_reverb))) {
        offload_reverb_set_enable_flag(&(reverb_ctxt->offload_reverb), false);
        if (reverb_ctxt->ctl)
            offload_reverb_send_params(reverb_ctxt->ctl,
                                       &reverb_ctxt->offload_reverb,
                                       OFFLOAD_SEND_REVERB_ENABLE_FLAG);
        if (reverb_ctxt->hw_acc_fd > 0)
            hw_acc_reverb_send_params(reverb_ctxt->hw_acc_fd,
                                      &reverb_ctxt->offload_reverb,
                                      OFFLOAD_SEND_REVERB_ENABLE_FLAG);
    }
    return 0;
}

int reverb_start(effect_context_t *context, output_context_t *output)
{
    reverb_context_t *reverb_ctxt = (reverb_context_t *)context;

    ALOGV("%s: ctxt %p, ctl %p", __func__, reverb_ctxt, output->ctl);
    reverb_ctxt->ctl = output->ctl;
    if (offload_reverb_get_enable_flag(&(reverb_ctxt->offload_reverb))) {
        if (reverb_ctxt->ctl && reverb_ctxt->preset) {
            offload_reverb_send_params(reverb_ctxt->ctl, &reverb_ctxt->offload_reverb,
                                       OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                       OFFLOAD_SEND_REVERB_PRESET);
        }
        if ((reverb_ctxt->hw_acc_fd > 0) && reverb_ctxt->preset) {
            hw_acc_reverb_send_params(reverb_ctxt->hw_acc_fd,
                                      &reverb_ctxt->offload_reverb,
                                      OFFLOAD_SEND_REVERB_ENABLE_FLAG |
                                      OFFLOAD_SEND_REVERB_PRESET);
        }
    }

    return 0;
}

int reverb_stop(effect_context_t *context, output_context_t *output __unused)
{
    reverb_context_t *reverb_ctxt = (reverb_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, reverb_ctxt);
    if (offload_reverb_get_enable_flag(&(reverb_ctxt->offload_reverb)) &&
        reverb_ctxt->ctl) {
        struct reverb_params reverb;
        reverb.enable_flag = false;
        offload_reverb_send_params(reverb_ctxt->ctl, &reverb,
                                   OFFLOAD_SEND_REVERB_ENABLE_FLAG);
    }
    reverb_ctxt->ctl = NULL;
    return 0;
}

int reverb_set_mode(effect_context_t *context, int32_t hw_acc_fd)
{
    reverb_context_t *reverb_ctxt = (reverb_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, reverb_ctxt);
    reverb_ctxt->hw_acc_fd = hw_acc_fd;
    if ((reverb_ctxt->hw_acc_fd > 0) &&
        (offload_reverb_get_enable_flag(&(reverb_ctxt->offload_reverb))))
        hw_acc_reverb_send_params(reverb_ctxt->hw_acc_fd,
                                  &reverb_ctxt->offload_reverb,
                                  OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG |
                                  OFFLOAD_SEND_BASSBOOST_STRENGTH);
    return 0;
}
