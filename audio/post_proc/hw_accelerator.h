/*
 * Copyright (c) 2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
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
 */

#ifndef HW_ACCELERATOR_EFFECT_H_
#define HW_ACCELERATOR_EFFECT_H_

#include "bundle.h"

#include <linux/msm_audio.h>

#define HWACCELERATOR_OUTPUT_CHANNELS AUDIO_CHANNEL_OUT_STEREO

extern const effect_descriptor_t hw_accelerator_descriptor;

typedef struct hw_accelerator_context_s {
    effect_context_t common;

    int fd;
    uint32_t device;
    bool intial_buffer_done;
    struct msm_hwacc_effects_config cfg;
} hw_accelerator_context_t;

int hw_accelerator_get_parameter(effect_context_t *context,
                                 effect_param_t *p, uint32_t *size);

int hw_accelerator_set_parameter(effect_context_t *context,
                                 effect_param_t *p, uint32_t size);

int hw_accelerator_set_device(effect_context_t *context,  uint32_t device);

int hw_accelerator_reset(effect_context_t *context);

int hw_accelerator_init(effect_context_t *context);

int hw_accelerator_enable(effect_context_t *context);

int hw_accelerator_disable(effect_context_t *context);

int hw_accelerator_release(effect_context_t *context);

int hw_accelerator_set_mode(effect_context_t *context,  int32_t frame_count);

int hw_accelerator_process(effect_context_t *context, audio_buffer_t *in,
                           audio_buffer_t *out);

#endif /* HW_ACCELERATOR_EFFECT_H_ */
