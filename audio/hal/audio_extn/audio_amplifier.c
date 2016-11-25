/*
 * Copyright (c) 2016 The CyanogenMod Project
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

#define LOG_TAG "audio_amplifier"

#include <stdbool.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <cutils/log.h>

#include "audio_hw.h"
#include "platform.h"

struct amplifier_data {
    struct audio_device *adev;
    amplifier_device_t *hw;
};

struct amplifier_data amp;

int amplifier_open(void* adev)
{
    int rc;
    amplifier_module_t *module;
    amp.adev = (struct audio_device *)adev;

    rc = hw_get_module(AMPLIFIER_HARDWARE_MODULE_ID,
            (const hw_module_t **) &module);
    if (rc) {
        ALOGV("%s: Failed to obtain reference to amplifier module: %s\n",
                __func__, strerror(-rc));
        return -ENODEV;
    }

    rc = amplifier_device_open((const hw_module_t *) module, &amp.hw);
    if (rc) {
        ALOGV("%s: Failed to open amplifier hardware device: %s\n",
                __func__, strerror(-rc));
        amp.hw = NULL;

        return -ENODEV;
    }

    return 0;
}

int amplifier_set_input_devices(uint32_t devices)
{
    if (amp.hw && amp.hw->set_input_devices)
        return amp.hw->set_input_devices(amp.hw, devices);

    return 0;
}

int amplifier_set_output_devices(uint32_t devices)
{
    if (amp.hw && amp.hw->set_output_devices)
        return amp.hw->set_output_devices(amp.hw, devices);

    return 0;
}

int amplifier_enable_devices(uint32_t devices, bool enable)
{
    bool is_output = devices < SND_DEVICE_OUT_END;

    if (amp.hw && amp.hw->enable_output_devices && is_output)
        return amp.hw->enable_output_devices(amp.hw, devices, enable);

    if (amp.hw && amp.hw->enable_input_devices && !is_output)
        return amp.hw->enable_input_devices(amp.hw, devices, enable);

    return 0;
}

int amplifier_set_mode(audio_mode_t mode)
{
    if (amp.hw && amp.hw->set_mode)
        return amp.hw->set_mode(amp.hw, mode);

    return 0;
}

int amplifier_output_stream_start(struct audio_stream_out *stream,
        bool offload)
{
    if (amp.hw && amp.hw->output_stream_start)
        return amp.hw->output_stream_start(amp.hw, stream, offload);

    return 0;
}

int amplifier_input_stream_start(struct audio_stream_in *stream)
{
    if (amp.hw && amp.hw->input_stream_start)
        return amp.hw->input_stream_start(amp.hw, stream);

    return 0;
}

int amplifier_output_stream_standby(struct audio_stream_out *stream)
{
    if (amp.hw && amp.hw->output_stream_standby)
        return amp.hw->output_stream_standby(amp.hw, stream);

    return 0;
}

int amplifier_input_stream_standby(struct audio_stream_in *stream)
{
    if (amp.hw && amp.hw->input_stream_standby)
        return amp.hw->input_stream_standby(amp.hw, stream);

    return 0;
}

int amplifier_set_parameters(struct str_parms *parms)
{
    if (amp.hw && amp.hw->set_parameters)
        return amp.hw->set_parameters(amp.hw, parms);

    return 0;
}

int amplifier_close(void)
{
    if (amp.hw)
        amplifier_device_close(amp.hw);

    amp.hw = NULL;

    return 0;
}
