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

#ifndef OFFLOAD_VIRTUALIZER_H_
#define OFFLOAD_VIRTUALIZER_H_

#include "bundle.h"

extern const effect_descriptor_t virtualizer_descriptor;

typedef struct virtualizer_context_s {
    effect_context_t common;

    int strength;

    // Offload vars
    struct mixer_ctl *ctl;
    int hw_acc_fd;
    bool enabled_by_client;
    bool temp_disabled;
    audio_devices_t forced_device;
    audio_devices_t device;
    struct virtualizer_params offload_virt;
} virtualizer_context_t;

int virtualizer_get_parameter(effect_context_t *context, effect_param_t *p,
                            uint32_t *size);

int virtualizer_set_parameter(effect_context_t *context, effect_param_t *p,
                            uint32_t size);

int virtualizer_set_device(effect_context_t *context,  uint32_t device);

int virtualizer_set_mode(effect_context_t *context,  int32_t hw_acc_fd);

int virtualizer_reset(effect_context_t *context);

int virtualizer_init(effect_context_t *context);

int virtualizer_enable(effect_context_t *context);

int virtualizer_disable(effect_context_t *context);

int virtualizer_start(effect_context_t *context, output_context_t *output);

int virtualizer_stop(effect_context_t *context, output_context_t *output);

#endif /* OFFLOAD_VIRTUALIZER_H_ */
