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

#define LOG_TAG "offload_effect_virtualizer"
//#define LOG_NDEBUG 0

#include <cutils/list.h>
#include <cutils/log.h>
#include <tinyalsa/asoundlib.h>
#include <sound/audio_effects.h>
#include <audio_effects/effect_virtualizer.h>

#include "effect_api.h"
#include "virtualizer.h"

/* Offload Virtualizer UUID: 509a4498-561a-4bea-b3b1-0002a5d5c51b */
const effect_descriptor_t virtualizer_descriptor = {
        {0x37cc2c00, 0xdddd, 0x11db, 0x8577, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}},
        {0x509a4498, 0x561a, 0x4bea, 0xb3b1, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // uuid
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_DEVICE_IND | EFFECT_FLAG_HW_ACC_TUNNEL),
        0, /* TODO */
        1,
        "MSM offload virtualizer",
        "The Android Open Source Project",
};

/*
 * Virtualizer operations
 */

int virtualizer_get_strength(virtualizer_context_t *context)
{
    ALOGV("%s: ctxt %p, strength: %d", __func__, context, context->strength);
    return context->strength;
}

int virtualizer_set_strength(virtualizer_context_t *context, uint32_t strength)
{
    ALOGV("%s: ctxt %p, strength: %d", __func__, context, strength);
    context->strength = strength;

    /*
     *  Zero strength is not equivalent to disable state as down mix
     *  is still happening for multichannel inputs.
     *  For better user experience, explicitly disable virtualizer module
     *  when strength is 0.
     */
    if (context->enabled_by_client)
        offload_virtualizer_set_enable_flag(&(context->offload_virt),
                                        ((strength > 0) && !(context->temp_disabled)) ?
                                        true : false);
    offload_virtualizer_set_strength(&(context->offload_virt), strength);
    if (context->ctl)
        offload_virtualizer_send_params(context->ctl, &context->offload_virt,
                                        OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG |
                                        OFFLOAD_SEND_VIRTUALIZER_STRENGTH);
    if (context->hw_acc_fd > 0)
        hw_acc_virtualizer_send_params(context->hw_acc_fd,
                                       &context->offload_virt,
                                       OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG |
                                       OFFLOAD_SEND_VIRTUALIZER_STRENGTH);
    return 0;
}

/*
 *  Check if an audio device is supported by this implementation
 *
 *  [in]
 *  device    device that is intented for processing (e.g. for binaural vs transaural)
 *  [out]
 *  false     device is not applicable for effect
 *  true      device is applicable for effect
 */
bool virtualizer_is_device_supported(audio_devices_t device) {
    switch (device) {
    case AUDIO_DEVICE_OUT_SPEAKER:
    case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT:
    case AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER:
#ifdef AFE_PROXY_ENABLED
    case AUDIO_DEVICE_OUT_PROXY:
#endif
    case AUDIO_DEVICE_OUT_AUX_DIGITAL:
    case AUDIO_DEVICE_OUT_USB_ACCESSORY:
    case AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET:
        return false;
    default :
        return true;
    }
}

/*
 *  Check if a channel mask + audio device is supported by this implementation
 *
 *  [in]
 *  channel_mask channel mask of input buffer
 *  device       device that is intented for processing (e.g. for binaural vs transaural)
 *  [out]
 *  false        if the configuration is not supported or it is unknown
 *  true         if the configuration is supported
 */
bool virtualizer_is_configuration_supported(audio_channel_mask_t channel_mask,
        audio_devices_t device) {
    uint32_t channelCount = audio_channel_count_from_out_mask(channel_mask);
    if ((channelCount == 0) || (channelCount > 2)) {
        return false;
    }

    return virtualizer_is_device_supported(device);
}

/*
 *  Force the virtualization mode to that of the given audio device
 *
 *  [in]
 *  context       effect engine context
 *  forced_device device whose virtualization mode we'll always use
 *  [out]
 *  -EINVAL       if the device is not supported or is unknown
 *  0             if the device is supported and the virtualization mode forced
 */
int virtualizer_force_virtualization_mode(virtualizer_context_t *context,
        audio_devices_t forced_device) {
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;
    int status = 0;
    bool use_virt = false;
    int is_virt_enabled = virt_ctxt->enabled_by_client;

    ALOGV("%s: ctxt %p, forcedDev=0x%x enabled=%d tmpDisabled=%d", __func__, virt_ctxt,
            forced_device, is_virt_enabled, virt_ctxt->temp_disabled);

    if (virtualizer_is_device_supported(forced_device) == false) {
        if (forced_device != AUDIO_DEVICE_NONE) {
            //forced device is not supported, make it behave as a reset of forced mode
            forced_device = AUDIO_DEVICE_NONE;
            // but return an error
            status = -EINVAL;
        }
    }

    if (forced_device == AUDIO_DEVICE_NONE) {
        // disabling forced virtualization mode:
        // verify whether the virtualization should be enabled or disabled
        if (virtualizer_is_device_supported(virt_ctxt->device)) {
            use_virt = (is_virt_enabled == true);
        }
        virt_ctxt->forced_device = AUDIO_DEVICE_NONE;
    } else {
        // forcing virtualization mode:
        // TODO: we assume device is supported, so hard coded a fixed one.
        virt_ctxt->forced_device = AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
        // TODO: only enable for a supported mode, when the effect is enabled
        use_virt = (is_virt_enabled == true);
    }

    if (use_virt) {
        if (virt_ctxt->temp_disabled == true) {
            if (effect_is_active(&virt_ctxt->common)) {
                offload_virtualizer_set_enable_flag(&(virt_ctxt->offload_virt), true);
                if (virt_ctxt->ctl)
                    offload_virtualizer_send_params(virt_ctxt->ctl,
                                                    &virt_ctxt->offload_virt,
                                                    OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG);
                if (virt_ctxt->hw_acc_fd > 0)
                    hw_acc_virtualizer_send_params(virt_ctxt->hw_acc_fd,
                                                   &virt_ctxt->offload_virt,
                                                   OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG);
            }
            ALOGV("%s: re-enable VIRTUALIZER", __func__);
            virt_ctxt->temp_disabled = false;
        } else {
            ALOGV("%s: leaving VIRTUALIZER enabled", __func__);
        }
    } else {
        if (virt_ctxt->temp_disabled == false) {
            if (effect_is_active(&virt_ctxt->common)) {
                offload_virtualizer_set_enable_flag(&(virt_ctxt->offload_virt), false);
                if (virt_ctxt->ctl)
                    offload_virtualizer_send_params(virt_ctxt->ctl,
                                                    &virt_ctxt->offload_virt,
                                                    OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG);
                if (virt_ctxt->hw_acc_fd > 0)
                    hw_acc_virtualizer_send_params(virt_ctxt->hw_acc_fd,
                                                   &virt_ctxt->offload_virt,
                                                   OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG);
            }
            ALOGV("%s: disable VIRTUALIZER", __func__);
            virt_ctxt->temp_disabled = true;
        } else {
            ALOGV("%s: leaving VIRTUALIZER disabled", __func__);
        }
    }

    ALOGV("after %s: ctxt %p, enabled=%d tmpDisabled=%d", __func__, virt_ctxt,
            is_virt_enabled, virt_ctxt->temp_disabled);

    return status;
}

/*
 *  Get the virtual speaker angles for a channel mask + audio device configuration
 *  which is guaranteed to be supported by this implementation
 *
 *  [in]
 *  channel_mask   the channel mask of the input to virtualize
 *  device         the type of device that affects the processing (e.g. for binaural vs transaural)
 *  [in/out]
 *  speaker_angles the array of integer where each speaker angle is written as a triplet in the
 *                 following format:
 *                  int32_t a bit mask with a single value selected for each speaker, following
 *                  the convention of the audio_channel_mask_t type
 *                  int32_t a value in degrees expressing the speaker azimuth, where 0 is in front
 *                  of the user, 180 behind, -90 to the left, 90 to the right of the user
 *                  int32_t a value in degrees expressing the speaker elevation, where 0 is the
 *                  horizontal plane, +90 is directly above the user, -90 below
 *
 */
void virtualizer_get_speaker_angles(audio_channel_mask_t channel_mask __unused,
        audio_devices_t device __unused, int32_t *speaker_angles) {
    // the channel count is guaranteed to be 1 or 2
    // the device is guaranteed to be of type headphone
    // this virtualizer is always 2in with speakers at -90 and 90deg of azimuth, 0deg of elevation
    *speaker_angles++ = (int32_t) AUDIO_CHANNEL_OUT_FRONT_LEFT;
    *speaker_angles++ = -90; // azimuth
    *speaker_angles++ = 0;   // elevation
    *speaker_angles++ = (int32_t) AUDIO_CHANNEL_OUT_FRONT_RIGHT;
    *speaker_angles++ = 90;  // azimuth
    *speaker_angles   = 0;   // elevation
}

/*
 *  Retrieve the current device whose processing mode is used by this effect
 *
 *  [out]
 *  AUDIO_DEVICE_NONE if the effect is not virtualizing
 *  or the device type if the effect is virtualizing
 */
audio_devices_t virtualizer_get_virtualization_mode(virtualizer_context_t *context) {
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;
    audio_devices_t device = AUDIO_DEVICE_NONE;

    if ((offload_virtualizer_get_enable_flag(&(virt_ctxt->offload_virt)))
            && (virt_ctxt->temp_disabled == false)) {
        if (virt_ctxt->forced_device != AUDIO_DEVICE_NONE) {
            // virtualization mode is forced, return that device
            device = virt_ctxt->forced_device;
        } else {
            // no forced mode, return the current device
            device = virt_ctxt->device;
        }
    }
    ALOGV("%s: returning 0x%x", __func__, device);
    return device;
}

int virtualizer_get_parameter(effect_context_t *context, effect_param_t *p,
                              uint32_t *size)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;
    int voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);
    int32_t *param_tmp = (int32_t *)p->data;
    int32_t param = *param_tmp++;
    void *value = p->data + voffset;
    int i;

    ALOGV("%s: ctxt %p, param %d", __func__, virt_ctxt, param);

    p->status = 0;

    switch (param) {
    case VIRTUALIZER_PARAM_STRENGTH_SUPPORTED:
        if (p->vsize < sizeof(uint32_t))
           p->status = -EINVAL;
        p->vsize = sizeof(uint32_t);
        break;
    case VIRTUALIZER_PARAM_STRENGTH:
        if (p->vsize < sizeof(int16_t))
           p->status = -EINVAL;
        p->vsize = sizeof(int16_t);
        break;
    case VIRTUALIZER_PARAM_VIRTUAL_SPEAKER_ANGLES:
        // return value size can only be interpreted as relative to input value,
        // deferring validity check to below
        break;
    case VIRTUALIZER_PARAM_VIRTUALIZATION_MODE:
        if (p->vsize != sizeof(uint32_t))
           p->status = -EINVAL;
        p->vsize = sizeof(uint32_t);
        break;
    default:
        p->status = -EINVAL;
    }

    *size = sizeof(effect_param_t) + voffset + p->vsize;

    if (p->status != 0)
        return 0;

    switch (param) {
    case VIRTUALIZER_PARAM_STRENGTH_SUPPORTED:
        *(uint32_t *)value = 1;
        break;

    case VIRTUALIZER_PARAM_STRENGTH:
        *(int16_t *)value = virtualizer_get_strength(virt_ctxt);
        break;

    case VIRTUALIZER_PARAM_VIRTUAL_SPEAKER_ANGLES:
    {
        const audio_channel_mask_t channel_mask = (audio_channel_mask_t) *param_tmp++;
        const audio_devices_t device = (audio_devices_t) *param_tmp;
        uint32_t channel_cnt = audio_channel_count_from_out_mask(channel_mask);

        if (p->vsize < 3 * channel_cnt * sizeof(int32_t)){
            p->status = -EINVAL;
            break;
        }
        // verify the configuration is supported
        if(virtualizer_is_configuration_supported(channel_mask, device)) {
            // configuration is supported, get the angles
            virtualizer_get_speaker_angles(channel_mask, device, (int32_t *)value);
        } else {
            p->status = -EINVAL;
        }

        break;
    }

    case VIRTUALIZER_PARAM_VIRTUALIZATION_MODE:
        *(uint32_t *)value  = (uint32_t) virtualizer_get_virtualization_mode(virt_ctxt);
        break;

    default:
        p->status = -EINVAL;
        break;
    }

    return 0;
}

int virtualizer_set_parameter(effect_context_t *context, effect_param_t *p,
                              uint32_t size __unused)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;
    int voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);
    void *value = p->data + voffset;
    int32_t *param_tmp = (int32_t *)p->data;
    int32_t param = *param_tmp++;
    uint32_t strength;

    ALOGV("%s: ctxt %p, param %d", __func__, virt_ctxt, param);

    p->status = 0;

    switch (param) {
    case VIRTUALIZER_PARAM_STRENGTH:
        strength = (uint32_t)(*(int16_t *)value);
        virtualizer_set_strength(virt_ctxt, strength);
        break;
    case VIRTUALIZER_PARAM_FORCE_VIRTUALIZATION_MODE:
    {
        const audio_devices_t device = *(audio_devices_t *)value;
        if (0 != virtualizer_force_virtualization_mode(virt_ctxt, device)) {
            p->status = -EINVAL;
        }
        break;
    }
    default:
        p->status = -EINVAL;
        break;
    }

    return 0;
}

int virtualizer_set_device(effect_context_t *context, uint32_t device)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;

    ALOGV("%s: ctxt %p, device: 0x%x", __func__, virt_ctxt, device);
    virt_ctxt->device = device;

    if (virt_ctxt->forced_device == AUDIO_DEVICE_NONE) {
        // default case unless configuration is forced
        if (virtualizer_is_device_supported(device) == false) {
            if (!virt_ctxt->temp_disabled) {
                if (effect_is_active(&virt_ctxt->common) && virt_ctxt->enabled_by_client) {
                    offload_virtualizer_set_enable_flag(&(virt_ctxt->offload_virt), false);
                    if (virt_ctxt->ctl)
                        offload_virtualizer_send_params(virt_ctxt->ctl,
                                                        &virt_ctxt->offload_virt,
                                                        OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG);
                    if (virt_ctxt->hw_acc_fd > 0)
                        hw_acc_virtualizer_send_params(virt_ctxt->hw_acc_fd,
                                                       &virt_ctxt->offload_virt,
                                                       OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG);
                }
                virt_ctxt->temp_disabled = true;
                ALOGI("%s: ctxt %p, disabled based on device", __func__, virt_ctxt);
            }
        } else {
            if (virt_ctxt->temp_disabled) {
                if (effect_is_active(&virt_ctxt->common) && virt_ctxt->enabled_by_client) {
                    offload_virtualizer_set_enable_flag(&(virt_ctxt->offload_virt), true);
                    if (virt_ctxt->ctl)
                        offload_virtualizer_send_params(virt_ctxt->ctl,
                                                        &virt_ctxt->offload_virt,
                                                        OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG);
                    if (virt_ctxt->hw_acc_fd > 0)
                        hw_acc_virtualizer_send_params(virt_ctxt->hw_acc_fd,
                                                       &virt_ctxt->offload_virt,
                                                       OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG);
                }
                virt_ctxt->temp_disabled = false;
            }
        }
    }
    // else virtualization mode is forced to a certain device, nothing to do

    offload_virtualizer_set_device(&(virt_ctxt->offload_virt), device);
    return 0;
}

int virtualizer_reset(effect_context_t *context)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;

    return 0;
}

int virtualizer_init(effect_context_t *context)
{
    ALOGV("%s: ctxt %p", __func__, context);
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;

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

    virt_ctxt->enabled_by_client = false;
    virt_ctxt->temp_disabled = false;
    virt_ctxt->hw_acc_fd = -1;
    virt_ctxt->forced_device = AUDIO_DEVICE_NONE;
    virt_ctxt->device = AUDIO_DEVICE_NONE;
    memset(&(virt_ctxt->offload_virt), 0, sizeof(struct virtualizer_params));

    return 0;
}

int virtualizer_enable(effect_context_t *context)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;

    ALOGV("%s: ctxt %p, strength %d", __func__, virt_ctxt, virt_ctxt->strength);

    virt_ctxt->enabled_by_client = true;
    if (!offload_virtualizer_get_enable_flag(&(virt_ctxt->offload_virt)) &&
        !(virt_ctxt->temp_disabled)) {
        offload_virtualizer_set_enable_flag(&(virt_ctxt->offload_virt), true);
        if (virt_ctxt->ctl && virt_ctxt->strength)
            offload_virtualizer_send_params(virt_ctxt->ctl,
                                          &virt_ctxt->offload_virt,
                                          OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG |
                                          OFFLOAD_SEND_VIRTUALIZER_STRENGTH);
        if ((virt_ctxt->hw_acc_fd > 0) && virt_ctxt->strength)
            hw_acc_virtualizer_send_params(virt_ctxt->hw_acc_fd,
                                           &virt_ctxt->offload_virt,
                                           OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG |
                                           OFFLOAD_SEND_VIRTUALIZER_STRENGTH);
    }
    return 0;
}

int virtualizer_disable(effect_context_t *context)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, virt_ctxt);

    virt_ctxt->enabled_by_client = false;
    if (offload_virtualizer_get_enable_flag(&(virt_ctxt->offload_virt))) {
        offload_virtualizer_set_enable_flag(&(virt_ctxt->offload_virt), false);
        if (virt_ctxt->ctl)
            offload_virtualizer_send_params(virt_ctxt->ctl,
                                          &virt_ctxt->offload_virt,
                                          OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG);
        if (virt_ctxt->hw_acc_fd > 0)
            hw_acc_virtualizer_send_params(virt_ctxt->hw_acc_fd,
                                           &virt_ctxt->offload_virt,
                                           OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG);
    }
    return 0;
}

int virtualizer_start(effect_context_t *context, output_context_t *output)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;

    ALOGV("%s: ctxt %p, ctl %p", __func__, virt_ctxt, output->ctl);
    virt_ctxt->ctl = output->ctl;
    if (offload_virtualizer_get_enable_flag(&(virt_ctxt->offload_virt))) {
        if (virt_ctxt->ctl)
            offload_virtualizer_send_params(virt_ctxt->ctl, &virt_ctxt->offload_virt,
                                          OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG |
                                          OFFLOAD_SEND_VIRTUALIZER_STRENGTH);
        if (virt_ctxt->hw_acc_fd > 0)
            hw_acc_virtualizer_send_params(virt_ctxt->hw_acc_fd,
                                           &virt_ctxt->offload_virt,
                                           OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG |
                                           OFFLOAD_SEND_VIRTUALIZER_STRENGTH);
    }
    return 0;
}

int virtualizer_stop(effect_context_t *context, output_context_t *output __unused)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, virt_ctxt);
    if (offload_virtualizer_get_enable_flag(&(virt_ctxt->offload_virt)) &&
        virt_ctxt->ctl) {
        struct virtualizer_params virt;
        virt.enable_flag = false;
        offload_virtualizer_send_params(virt_ctxt->ctl, &virt,
                                        OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG);
    }
    virt_ctxt->ctl = NULL;
    return 0;
}

int virtualizer_set_mode(effect_context_t *context, int32_t hw_acc_fd)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;

    ALOGV("%s: ctxt %p", __func__, virt_ctxt);
    virt_ctxt->hw_acc_fd = hw_acc_fd;
    if ((virt_ctxt->hw_acc_fd > 0) &&
        (offload_virtualizer_get_enable_flag(&(virt_ctxt->offload_virt))))
        hw_acc_virtualizer_send_params(virt_ctxt->hw_acc_fd,
                                       &virt_ctxt->offload_virt,
                                       OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG |
                                       OFFLOAD_SEND_VIRTUALIZER_STRENGTH);
    return 0;
}
