/*
 * Copyright (c) 2013-2016, The Linux Foundation. All rights reserved.
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

#ifndef VOICE_H
#define VOICE_H

#define BASE_SESS_IDX       0
#define VOICE_SESS_IDX     (BASE_SESS_IDX)

#ifdef MULTI_VOICE_SESSION_ENABLED
#define MAX_VOICE_SESSIONS 7
#else
#define MAX_VOICE_SESSIONS 1
#endif

#define BASE_CALL_STATE     1
#define CALL_INACTIVE       (BASE_CALL_STATE)
#define CALL_ACTIVE         (BASE_CALL_STATE + 1)

#define VOICE_VSID  0x10C01000

#define AUDIO_PARAMETER_KEY_INCALLMUSIC "incall_music_enabled"
#define AUDIO_PARAMETER_VALUE_TRUE "true"

struct audio_device;
struct str_parms;
struct stream_in;
struct stream_out;
typedef int audio_usecase_t;
typedef int snd_device_t;

struct call_state {
    int current;
    int new;
};

struct voice_session {
    struct pcm *pcm_rx;
    struct pcm *pcm_tx;
    struct call_state state;
    uint32_t vsid;
};

struct voice {
    struct voice_session session[MAX_VOICE_SESSIONS];
    int tty_mode;
    bool mic_mute;
    float volume;
    bool in_call;
};

enum {
    INCALL_REC_NONE = -1,
    INCALL_REC_UPLINK,
    INCALL_REC_DOWNLINK,
    INCALL_REC_UPLINK_AND_DOWNLINK,
};

int voice_start_usecase(struct audio_device *adev, audio_usecase_t usecase_id);
int voice_stop_usecase(struct audio_device *adev, audio_usecase_t usecase_id);

int voice_start_call(struct audio_device *adev);
int voice_stop_call(struct audio_device *adev);
int voice_set_parameters(struct audio_device *adev, struct str_parms *parms);
void voice_get_parameters(struct audio_device *adev, struct str_parms *query,
                          struct str_parms *reply);
void voice_init(struct audio_device *adev);
bool voice_is_in_call(const struct audio_device *adev);
bool voice_is_in_call_rec_stream(const struct stream_in *in);
int voice_set_mic_mute(struct audio_device *dev, bool state);
bool voice_get_mic_mute(struct audio_device *dev);
int voice_set_volume(struct audio_device *adev, float volume);
int voice_check_and_set_incall_rec_usecase(struct audio_device *adev,
                                           struct stream_in *in);
int voice_check_and_set_incall_music_usecase(struct audio_device *adev,
                                             struct stream_out *out);
int voice_check_and_stop_incall_rec_usecase(struct audio_device *adev,
                                            struct stream_in *in);
void voice_update_devices_for_all_voice_usecases(struct audio_device *adev);
snd_device_t voice_get_incall_rec_snd_device(snd_device_t in_snd_device);
void voice_set_sidetone(struct audio_device *adev,
                       snd_device_t out_snd_device,
                       bool enable);
bool voice_is_call_state_active(struct audio_device *adev);
#endif //VOICE_H
