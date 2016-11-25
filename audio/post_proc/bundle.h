/*
 * Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
 * Not a contribution.
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

#ifndef OFFLOAD_EFFECT_BUNDLE_H
#define OFFLOAD_EFFECT_BUNDLE_H

#include <tinyalsa/asoundlib.h>
#include <sound/audio_effects.h>
#include "effect_api.h"

/* Retry for delay for mixer open */
#define RETRY_NUMBER 10
#define RETRY_US 500000
#ifdef HW_ACCELERATED_EFFECTS
#define EFFECT_CMD_HW_ACC 20
#endif
#define MIXER_CARD 0
#define SOUND_CARD 0

extern const struct effect_interface_s effect_interface;

typedef struct output_context_s output_context_t;
typedef struct effect_ops_s effect_ops_t;
typedef struct effect_context_s effect_context_t;

struct output_context_s {
    /* node in active_outputs_list */
    struct listnode outputs_list_node;
    /* io handle */
    audio_io_handle_t handle;
    /* list of effects attached to this output */
    struct listnode effects_list;
    /* pcm device id */
    int pcm_device_id;
    struct mixer *mixer;
    struct mixer_ctl *ctl;
    struct mixer_ctl *ref_ctl;
};

/* effect specific operations.
 * Only the init() and process() operations must be defined.
 * Others are optional.
 */
struct effect_ops_s {
    int (*init)(effect_context_t *context);
    int (*release)(effect_context_t *context);
    int (*reset)(effect_context_t *context);
    int (*enable)(effect_context_t *context);
    int (*start)(effect_context_t *context, output_context_t *output);
    int (*stop)(effect_context_t *context, output_context_t *output);
    int (*disable)(effect_context_t *context);
    int (*process)(effect_context_t *context, audio_buffer_t *in, audio_buffer_t *out);
    int (*set_parameter)(effect_context_t *context, effect_param_t *param, uint32_t size);
    int (*get_parameter)(effect_context_t *context, effect_param_t *param, uint32_t *size);
    int (*set_device)(effect_context_t *context, uint32_t device);
    int (*set_hw_acc_mode)(effect_context_t *context, int32_t value);
    int (*command)(effect_context_t *context, uint32_t cmdCode, uint32_t cmdSize,
            void *pCmdData, uint32_t *replySize, void *pReplyData);
};

struct effect_context_s {
    const struct effect_interface_s *itfe;
    /* node in created_effects_list */
    struct listnode effects_list_node;
    /* node in output_context_t.effects_list */
    struct listnode output_node;
    effect_config_t config;
    const effect_descriptor_t *desc;
    /* io handle of the output the effect is attached to */
    audio_io_handle_t out_handle;
    uint32_t state;
    bool offload_enabled;
    bool hw_acc_enabled;
    effect_ops_t ops;
};

int set_config(effect_context_t *context, effect_config_t *config);

bool effect_is_active(effect_context_t *context);

#endif /* OFFLOAD_EFFECT_BUNDLE_H */
