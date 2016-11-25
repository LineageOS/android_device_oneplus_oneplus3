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

#ifndef OFFLOAD_EFFECT_BASS_BOOST_H_
#define OFFLOAD_EFFECT_BASS_BOOST_H_

#include "bundle.h"

enum {
    BASS_INVALID = -1,
    BASS_BOOST = 0,      // index of bassboost
    BASS_PBE,        // index of PBE
    BASS_COUNT       // totol number of bass type
};

extern const effect_descriptor_t bassboost_descriptor;

typedef struct bassboost_context_s {
    effect_context_t common;

    int strength;

    // Offload vars
    struct mixer_ctl *ctl;
    int hw_acc_fd;
    bool temp_disabled;
    uint32_t device;
    struct bass_boost_params offload_bass;
} bassboost_context_t;

typedef struct pbe_context_s {
    effect_context_t common;

    // Offload vars
    struct mixer_ctl *ctl;
    int hw_acc_fd;
    bool temp_disabled;
    uint32_t device;
    struct pbe_params offload_pbe;
} pbe_context_t;

typedef struct bass_context_s {
    effect_context_t    common;
    bassboost_context_t bassboost_ctxt;
    pbe_context_t       pbe_ctxt;
    int                 active_index;
} bass_context_t;

int bass_get_parameter(effect_context_t *context, effect_param_t *p,
                       uint32_t *size);

int bass_set_parameter(effect_context_t *context, effect_param_t *p,
                       uint32_t size);

int bass_set_device(effect_context_t *context,  uint32_t device);

int bass_set_mode(effect_context_t *context,  int32_t hw_acc_fd);

int bass_reset(effect_context_t *context);

int bass_init(effect_context_t *context);

int bass_enable(effect_context_t *context);

int bass_disable(effect_context_t *context);

int bass_start(effect_context_t *context, output_context_t *output);

int bass_stop(effect_context_t *context, output_context_t *output);


int bassboost_get_strength(bassboost_context_t *context);

int bassboost_set_strength(bassboost_context_t *context, uint32_t strength);

int bassboost_set_device(effect_context_t *context,  uint32_t device);

int bassboost_set_mode(effect_context_t *context,  int32_t hw_acc_fd);

int bassboost_reset(effect_context_t *context);

int bassboost_init(effect_context_t *context);

int bassboost_enable(effect_context_t *context);

int bassboost_disable(effect_context_t *context);

int bassboost_start(effect_context_t *context, output_context_t *output);

int bassboost_stop(effect_context_t *context, output_context_t *output);

int pbe_set_device(effect_context_t *context,  uint32_t device);

int pbe_set_mode(effect_context_t *context,  int32_t hw_acc_fd);

int pbe_reset(effect_context_t *context);

int pbe_init(effect_context_t *context);

int pbe_enable(effect_context_t *context);

int pbe_disable(effect_context_t *context);

int pbe_start(effect_context_t *context, output_context_t *output);

int pbe_stop(effect_context_t *context, output_context_t *output);

#endif /* OFFLOAD_EFFECT_BASS_BOOST_H_ */
