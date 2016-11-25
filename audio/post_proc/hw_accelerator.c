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

#define LOG_TAG "hw_accelerator_effect"
/*#define LOG_NDEBUG 0*/

#include <cutils/list.h>
#include <cutils/log.h>
#include <fcntl.h>
#include <tinyalsa/asoundlib.h>
#include <sound/audio_effects.h>
#include <audio_effects/effect_hwaccelerator.h>

#include "effect_api.h"
#include "hw_accelerator.h"


/* hw_accelerator UUID: 7d1580bd-297f-4683-9239-e475b6d1d69f */
const effect_descriptor_t hw_accelerator_descriptor = {
        EFFECT_UIID_HWACCELERATOR__,
        {0x7d1580bd, 0x297f, 0x4683, 0x9239, {0xe4, 0x75, 0xb6, 0xd1, 0xd6, 0x9f}}, // uuid
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_DEVICE_IND),
        0, /* TODO */
        1,
        "HwAccelerated Library",
        "QTI",
};

int hw_accelerator_get_parameter(effect_context_t *context,
                                 effect_param_t *p, uint32_t *size)
{
    hw_accelerator_context_t *hw_acc_ctxt = (hw_accelerator_context_t *)context;
    int voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);
    int32_t *param_tmp = (int32_t *)p->data;
    int32_t param = *param_tmp++;
    void *value = p->data + voffset;
    int i;

    ALOGV("%s: ctxt %p, param %d", __func__, hw_acc_ctxt, param);

    p->status = 0;

    switch (param) {
    case HW_ACCELERATOR_FD:
        if (p->vsize < sizeof(int32_t))
           p->status = -EINVAL;
        p->vsize = sizeof(int32_t);
        break;
    default:
        p->status = -EINVAL;
    }

    *size = sizeof(effect_param_t) + voffset + p->vsize;

    if (p->status != 0)
        return 0;

    switch (param) {
    case HW_ACCELERATOR_FD:
        ALOGV("%s: HW_ACCELERATOR_FD", __func__);
        *(int32_t *)value = hw_acc_ctxt->fd;
        break;

    default:
        p->status = -EINVAL;
        break;
    }

    return 0;
}

int hw_accelerator_set_parameter(effect_context_t *context, effect_param_t *p,
                            uint32_t size)
{
    hw_accelerator_context_t *hw_acc_ctxt = (hw_accelerator_context_t *)context;
    int voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);
    void *value = p->data + voffset;
    int32_t *param_tmp = (int32_t *)p->data;
    int32_t param = *param_tmp++;

    ALOGV("%s: ctxt %p, param %d", __func__, hw_acc_ctxt, param);

    p->status = 0;

    switch (param) {
    case HW_ACCELERATOR_HPX_STATE: {
        int hpxState = (uint32_t)(*(int32_t *)value);
        if (hpxState)
            hw_acc_hpx_send_params(hw_acc_ctxt->fd, OFFLOAD_SEND_HPX_STATE_ON);
        else
            hw_acc_hpx_send_params(hw_acc_ctxt->fd, OFFLOAD_SEND_HPX_STATE_OFF);
        break;
    }
    default:
        p->status = -EINVAL;
        break;
    }

    return 0;
}

int hw_accelerator_set_device(effect_context_t *context, uint32_t device)
{
    hw_accelerator_context_t *hw_acc_ctxt = (hw_accelerator_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, hw_acc_ctxt);
    hw_acc_ctxt->device = device;
    return 0;
}

int hw_accelerator_init(effect_context_t *context)
{
    hw_accelerator_context_t *hw_acc_ctxt = (hw_accelerator_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, hw_acc_ctxt);
    context->config.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
    context->config.inputCfg.channels = AUDIO_CHANNEL_OUT_7POINT1;
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

    hw_acc_ctxt->fd = -1;
    memset(&(hw_acc_ctxt->cfg), 0, sizeof(struct msm_hwacc_effects_config));

    return 0;
}

int hw_accelerator_reset(effect_context_t *context)
{
    ALOGV("%s", __func__);
    return 0;
}

int hw_accelerator_set_mode(effect_context_t *context, int32_t frame_count)
{
    hw_accelerator_context_t *hw_acc_ctxt = (hw_accelerator_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, hw_acc_ctxt);
    hw_acc_ctxt->cfg.output.sample_rate = context->config.inputCfg.samplingRate;
    hw_acc_ctxt->cfg.input.sample_rate = context->config.outputCfg.samplingRate;

    hw_acc_ctxt->cfg.output.num_channels = popcount(context->config.inputCfg.channels);
    hw_acc_ctxt->cfg.input.num_channels = popcount(context->config.outputCfg.channels);

    hw_acc_ctxt->cfg.output.bits_per_sample = 8 *
                                audio_bytes_per_sample(context->config.inputCfg.format);
    hw_acc_ctxt->cfg.input.bits_per_sample = 8 *
                                audio_bytes_per_sample(context->config.outputCfg.format);

    ALOGV("write: sample_rate: %d, channel: %d, bit_width: %d",
           hw_acc_ctxt->cfg.output.sample_rate, hw_acc_ctxt->cfg.output.num_channels,
           hw_acc_ctxt->cfg.output.bits_per_sample);
    ALOGV("read: sample_rate: %d, channel: %d, bit_width: %d",
           hw_acc_ctxt->cfg.input.sample_rate, hw_acc_ctxt->cfg.input.num_channels,
           hw_acc_ctxt->cfg.input.bits_per_sample);

    hw_acc_ctxt->cfg.output.num_buf = 4;
    hw_acc_ctxt->cfg.input.num_buf = 2;

    hw_acc_ctxt->cfg.output.buf_size = frame_count *
                    hw_acc_ctxt->cfg.output.num_channels *
                    audio_bytes_per_sample(context->config.inputCfg.format) *
                    ((hw_acc_ctxt->cfg.output.sample_rate/hw_acc_ctxt->cfg.input.sample_rate) +
                     (hw_acc_ctxt->cfg.output.sample_rate%hw_acc_ctxt->cfg.input.sample_rate ? 1 : 0));
    hw_acc_ctxt->cfg.input.buf_size = frame_count *
                    hw_acc_ctxt->cfg.input.num_channels *
                    audio_bytes_per_sample(context->config.outputCfg.format);

    hw_acc_ctxt->cfg.meta_mode_enabled = 0;
    /* TODO: overwrite this for effects using custom topology*/
    hw_acc_ctxt->cfg.overwrite_topology = 0;
    hw_acc_ctxt->cfg.topology = 0;

    return 0;
}

int hw_accelerator_enable(effect_context_t *context)
{
    hw_accelerator_context_t *hw_acc_ctxt = (hw_accelerator_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, hw_acc_ctxt);
    hw_acc_ctxt->fd = open("/dev/msm_hweffects", O_RDWR | O_NONBLOCK);
    /* open driver */
    if (hw_acc_ctxt->fd < 0) {
         ALOGE("Audio Effects driver open failed");
         return -EFAULT;
    }
    /* set config */
    if (ioctl(hw_acc_ctxt->fd, AUDIO_SET_EFFECTS_CONFIG, &hw_acc_ctxt->cfg) < 0) {
        ALOGE("setting audio effects drivers config failed");
        if (close(hw_acc_ctxt->fd) < 0)
            ALOGE("releasing hardware accelerated effects driver failed");
        hw_acc_ctxt->fd = -1;
        return -EFAULT;
    }
    /* start */
    if (ioctl(hw_acc_ctxt->fd,  AUDIO_START, 0) < 0) {
        ALOGE("audio effects drivers prepare failed");
        if (close(hw_acc_ctxt->fd) < 0)
            ALOGE("releasing hardware accelerated effects driver failed");
        hw_acc_ctxt->fd = -1;
        return -EFAULT;
    }
    return 0;
}

int hw_accelerator_disable(effect_context_t *context)
{
    hw_accelerator_context_t *hw_acc_ctxt = (hw_accelerator_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, hw_acc_ctxt);
    if (hw_acc_ctxt->fd > 0)
        if (close(hw_acc_ctxt->fd) < 0)
            ALOGE("releasing hardware accelerated effects driver failed");
    hw_acc_ctxt->fd = -1;
    return 0;
}

int hw_accelerator_release(effect_context_t *context)
{
    hw_accelerator_context_t *hw_acc_ctxt = (hw_accelerator_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, hw_acc_ctxt);
    if (hw_acc_ctxt->fd > 0)
        if (close(hw_acc_ctxt->fd) < 0)
            ALOGE("releasing hardware accelerated effects driver failed");
    hw_acc_ctxt->fd = -1;
    return 0;
}

int hw_accelerator_process(effect_context_t *context, audio_buffer_t *in_buf,
                           audio_buffer_t *out_buf)
{
    hw_accelerator_context_t *hw_acc_ctxt = (hw_accelerator_context_t *)context;
    struct msm_hwacc_buf_cfg buf_cfg;
    struct msm_hwacc_buf_avail buf_avail;
    int ret = 0;

    ALOGV("%s: ctxt %p", __func__, hw_acc_ctxt);
    if (in_buf == NULL || in_buf->raw == NULL ||
        out_buf == NULL || out_buf->raw == NULL)
        return -EINVAL;

    buf_cfg.output_len = in_buf->frameCount *
                         audio_bytes_per_sample(context->config.inputCfg.format) *
                         hw_acc_ctxt->cfg.output.num_channels;
    buf_cfg.input_len = out_buf->frameCount *
                         audio_bytes_per_sample(context->config.outputCfg.format) *
                         hw_acc_ctxt->cfg.input.num_channels;

    if (ioctl(hw_acc_ctxt->fd, AUDIO_EFFECTS_GET_BUF_AVAIL, &buf_avail) < 0) {
        ALOGE("AUDIO_EFFECTS_GET_BUF_AVAIL failed");
        return -ENOMEM;
    }

    if (!hw_acc_ctxt->intial_buffer_done) {
        if (ioctl(hw_acc_ctxt->fd, AUDIO_EFFECTS_SET_BUF_LEN, &buf_cfg) < 0) {
            ALOGE("AUDIO_EFFECTS_BUF_CFG failed");
            return -EFAULT;
        }
        if (ioctl(hw_acc_ctxt->fd, AUDIO_EFFECTS_WRITE, (char *)in_buf->raw) < 0) {
            ALOGE("AUDIO_EFFECTS_WRITE failed");
            return -EFAULT;
        }
        ALOGV("Request for more data");
        hw_acc_ctxt->intial_buffer_done = true;
        return -ENODATA;
    }
    if (buf_avail.output_num_avail > 1) {
        if (ioctl(hw_acc_ctxt->fd, AUDIO_EFFECTS_SET_BUF_LEN, &buf_cfg) < 0) {
            ALOGE("AUDIO_EFFECTS_BUF_CFG failed");
            return -EFAULT;
        }
        if (ioctl(hw_acc_ctxt->fd, AUDIO_EFFECTS_WRITE, (char *)in_buf->raw) < 0) {
            ALOGE("AUDIO_EFFECTS_WRITE failed");
            return -EFAULT;
        }
        ret = in_buf->frameCount;
    }
    if (ioctl(hw_acc_ctxt->fd, AUDIO_EFFECTS_READ, (char *)out_buf->raw) < 0) {
        ALOGE("AUDIO_EFFECTS_READ failed");
        return -EFAULT;
    }

    return ret;
}
