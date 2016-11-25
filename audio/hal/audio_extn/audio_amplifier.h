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

#ifndef EXTN_AMPLIFIER_H
#define EXTN_AMPLIFIER_H

#ifndef EXT_AMPLIFIER_ENABLED
#define amplifier_open(adev) (0)
#define amplifier_set_input_devices(devices) (0)
#define amplifier_set_output_devices(devices) (0)
#define amplifier_enable_devices(devices, enable) (0)
#define amplifier_set_mode(mode) (0)
#define amplifier_output_stream_start(stream, offload) (0)
#define amplifier_input_stream_start(stream) (0)
#define amplifier_output_stream_standby(stream) (0)
#define amplifier_input_stream_standby(stream) (0)
#define amplifier_set_parameters(parms) (0)
#define amplifier_close() (0)
#else

int amplifier_open(void* adev);
int amplifier_set_input_devices(uint32_t devices);
int amplifier_set_output_devices(uint32_t devices);
int amplifier_enable_devices(uint32_t devices, bool enable);
int amplifier_set_mode(audio_mode_t mode);
int amplifier_output_stream_start(struct audio_stream_out *stream,
        bool offload);
int amplifier_input_stream_start(struct audio_stream_in *stream);
int amplifier_output_stream_standby(struct audio_stream_out *stream);
int amplifier_input_stream_standby(struct audio_stream_in *stream);
int amplifier_set_parameters(struct str_parms *parms);
int amplifier_close(void);
#endif

#endif
