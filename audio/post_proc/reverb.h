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

#ifndef OFFLOAD_REVERB_H_
#define OFFLOAD_REVERB_H_

#include "bundle.h"

#define REVERB_DEFAULT_PRESET REVERB_PRESET_NONE

extern const effect_descriptor_t aux_env_reverb_descriptor;
extern const effect_descriptor_t ins_env_reverb_descriptor;
extern const effect_descriptor_t aux_preset_reverb_descriptor;
extern const effect_descriptor_t ins_preset_reverb_descriptor;

typedef struct reverb_settings_s {
    int16_t     roomLevel;
    int16_t     roomHFLevel;
    uint32_t    decayTime;
    int16_t     decayHFRatio;
    int16_t     reflectionsLevel;
    uint32_t    reflectionsDelay;
    int16_t     reverbLevel;
    uint32_t    reverbDelay;
    int16_t     diffusion;
    int16_t     density;
}  __attribute__((packed)) reverb_settings_t;

typedef struct reverb_context_s {
    effect_context_t common;

    // Offload vars
    struct mixer_ctl *ctl;
    int hw_acc_fd;
    bool enabled_by_client;
    bool auxiliary;
    bool preset;
    uint16_t cur_preset;
    uint16_t next_preset;
    reverb_settings_t reverb_settings;
    uint32_t device;
    struct reverb_params offload_reverb;
} reverb_context_t;


void reverb_auxiliary_init(reverb_context_t *context);

void reverb_preset_init(reverb_context_t *context);

void reverb_insert_init(reverb_context_t *context);

int reverb_get_parameter(effect_context_t *context, effect_param_t *p,
                            uint32_t *size);

int reverb_set_parameter(effect_context_t *context, effect_param_t *p,
                            uint32_t size);

int reverb_set_device(effect_context_t *context,  uint32_t device);

int reverb_set_mode(effect_context_t *context,  int32_t hw_acc_fd);

int reverb_reset(effect_context_t *context);

int reverb_init(effect_context_t *context);

int reverb_enable(effect_context_t *context);

int reverb_disable(effect_context_t *context);

int reverb_start(effect_context_t *context, output_context_t *output);

int reverb_stop(effect_context_t *context, output_context_t *output);

#endif /* OFFLOAD_REVERB_H_ */
