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

#ifndef OFFLOAD_EQUALIZER_H_
#define OFFLOAD_EQUALIZER_H_

#include "bundle.h"

#define NUM_EQ_BANDS              5
#define INVALID_PRESET		 -2
#define PRESET_CUSTOM		 -1

extern const effect_descriptor_t equalizer_descriptor;

typedef struct equalizer_context_s {
    effect_context_t common;

    int preset;
    int band_levels[NUM_EQ_BANDS];

    // Offload vars
    struct mixer_ctl *ctl;
    int hw_acc_fd;
    uint32_t device;
    struct eq_params offload_eq;
} equalizer_context_t;

int equalizer_get_parameter(effect_context_t *context, effect_param_t *p,
                            uint32_t *size);

int equalizer_set_parameter(effect_context_t *context, effect_param_t *p,
                            uint32_t size);

int equalizer_set_device(effect_context_t *context,  uint32_t device);

int equalizer_set_mode(effect_context_t *context,  int32_t hw_acc_fd);

int equalizer_reset(effect_context_t *context);

int equalizer_init(effect_context_t *context);

int equalizer_enable(effect_context_t *context);

int equalizer_disable(effect_context_t *context);

int equalizer_start(effect_context_t *context, output_context_t *output);

int equalizer_stop(effect_context_t *context, output_context_t *output);

#endif /*OFFLOAD_EQUALIZER_H_*/
