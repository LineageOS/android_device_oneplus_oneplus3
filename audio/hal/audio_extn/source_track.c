/*
 * Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
#define LOG_TAG "source_track"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0

#include <errno.h>
#include <math.h>
#include <cutils/log.h>

#include "audio_hw.h"
#include "platform.h"
#include "platform_api.h"
#include "voice_extn.h"
#include <stdlib.h>
#include <cutils/str_parms.h>

#ifdef SOURCE_TRACKING_ENABLED
/* Audio Paramater Key to identify the list of start angles.
 * Starting angle (in degrees) defines the boundary starting angle for each sector.
 */
#define AUDIO_PARAMETER_KEY_SOUND_FOCUS_START_ANGLES        "SoundFocus.start_angles"
/* Audio Paramater Key to identify the list of enable flags corresponding to each sector.
 */
#define AUDIO_PARAMETER_KEY_SOUND_FOCUS_ENABLE_SECTORS      "SoundFocus.enable_sectors"
/* Audio Paramater Key to identify the gain step value to be applied to all enabled sectors.
 */
#define AUDIO_PARAMETER_KEY_SOUND_FOCUS_GAIN_STEP           "SoundFocus.gain_step"
/* Audio Paramater Key to identify the list of voice activity detector outputs corresponding
 * to each sector.
 */
#define AUDIO_PARAMETER_KEY_SOURCE_TRACK_VAD                "SourceTrack.vad"
/* Audio Paramater Key to identify the direction (in degrees) of arrival for desired talker
 * (dominant source of speech).
 */
#define AUDIO_PARAMETER_KEY_SOURCE_TRACK_DOA_SPEECH         "SourceTrack.doa_speech"
/* Audio Paramater Key to identify the list of directions (in degrees) of arrival for
 * interferers (interfering noise sources).
 */
#define AUDIO_PARAMETER_KEY_SOURCE_TRACK_DOA_NOISE          "SourceTrack.doa_noise"
/* Audio Paramater Key to identify the list of sound strength indicators at each degree
 * of the horizontal plane referred to by a full circle (360 degrees).
 */
#define AUDIO_PARAMETER_KEY_SOURCE_TRACK_POLAR_ACTIVITY     "SourceTrack.polar_activity"

#define BITMASK_AUDIO_PARAMETER_KEY_SOUND_FOCUS_START_ANGLES        0x1
#define BITMASK_AUDIO_PARAMETER_KEY_SOUND_FOCUS_ENABLE_SECTORS      0x2
#define BITMASK_AUDIO_PARAMETER_KEY_SOUND_FOCUS_GAIN_STEP           0x4
#define BITMASK_AUDIO_PARAMETER_KEY_SOURCE_TRACK_VAD                0x8
#define BITMASK_AUDIO_PARAMETER_KEY_SOURCE_TRACK_DOA_SPEECH         0x10
#define BITMASK_AUDIO_PARAMETER_KEY_SOURCE_TRACK_DOA_NOISE          0x20
#define BITMASK_AUDIO_PARAMETER_KEY_SOURCE_TRACK_POLAR_ACTIVITY     0x40

#define BITMASK_AUDIO_PARAMETER_KEYS_SOUND_FOCUS \
     (BITMASK_AUDIO_PARAMETER_KEY_SOUND_FOCUS_START_ANGLES |\
      BITMASK_AUDIO_PARAMETER_KEY_SOUND_FOCUS_ENABLE_SECTORS |\
      BITMASK_AUDIO_PARAMETER_KEY_SOUND_FOCUS_GAIN_STEP)

#define BITMASK_AUDIO_PARAMETER_KEYS_SOURCE_TRACKING \
     (BITMASK_AUDIO_PARAMETER_KEY_SOURCE_TRACK_VAD |\
      BITMASK_AUDIO_PARAMETER_KEY_SOURCE_TRACK_DOA_SPEECH |\
      BITMASK_AUDIO_PARAMETER_KEY_SOURCE_TRACK_DOA_NOISE |\
      BITMASK_AUDIO_PARAMETER_KEY_SOURCE_TRACK_POLAR_ACTIVITY)

#define MAX_SECTORS                                         8
#define MAX_STR_SIZE                                       2048

extern struct audio_device_to_audio_interface audio_device_to_interface_table[];
extern int audio_device_to_interface_table_len;

struct sound_focus_param {
    uint16_t start_angle[MAX_SECTORS];
    uint8_t enable[MAX_SECTORS];
    uint16_t gain_step;
};

struct source_tracking_param {
    uint8_t vad[MAX_SECTORS];
    uint16_t doa_speech;
    uint16_t doa_noise[3];
    uint8_t polar_activity[360];
};

static int add_audio_intf_name_to_mixer_ctl(audio_devices_t device, char *mixer_ctl_name,
                                struct audio_device_to_audio_interface *table, int len)
{
    int ret = 0;
    int i;

    if (table == NULL) {
        ALOGE("%s: table is NULL", __func__);

        ret = -EINVAL;
        goto done;
    }

    if (mixer_ctl_name == NULL) {
        ret = -EINVAL;
        goto done;
    }

    for (i=0; i < len; i++) {
        if (device == table[i].device) {
             strlcat(mixer_ctl_name, " ", MIXER_PATH_MAX_LENGTH);
             strlcat(mixer_ctl_name, table[i].interface_name, MIXER_PATH_MAX_LENGTH);
             break;
        }
    }

    if (i == len) {
        ALOGE("%s: Audio Device not found in the table", __func__);

        ret = -EINVAL;
    }
done:
    return ret;
}

static bool is_stt_supported_snd_device(snd_device_t snd_device)
{
    bool ret = false;

    switch (snd_device) {
    case SND_DEVICE_IN_HANDSET_DMIC:
    case SND_DEVICE_IN_HANDSET_DMIC_AEC:
    case SND_DEVICE_IN_HANDSET_DMIC_NS:
    case SND_DEVICE_IN_HANDSET_DMIC_AEC_NS:
    case SND_DEVICE_IN_HANDSET_STEREO_DMIC:
    case SND_DEVICE_IN_HANDSET_QMIC:
    case SND_DEVICE_IN_VOICE_DMIC:
    case SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE:
    case SND_DEVICE_IN_HEADSET_MIC_FLUENCE:
    case SND_DEVICE_IN_SPEAKER_DMIC:
    case SND_DEVICE_IN_SPEAKER_DMIC_AEC:
    case SND_DEVICE_IN_SPEAKER_DMIC_NS:
    case SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS:
    case SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS_BROADSIDE:
    case SND_DEVICE_IN_SPEAKER_DMIC_AEC_BROADSIDE:
    case SND_DEVICE_IN_SPEAKER_DMIC_NS_BROADSIDE:
    case SND_DEVICE_IN_SPEAKER_QMIC_AEC:
    case SND_DEVICE_IN_SPEAKER_QMIC_NS:
    case SND_DEVICE_IN_SPEAKER_QMIC_AEC_NS:
    case SND_DEVICE_IN_VOICE_SPEAKER_DMIC:
    case SND_DEVICE_IN_VOICE_SPEAKER_DMIC_BROADSIDE:
    case SND_DEVICE_IN_VOICE_SPEAKER_QMIC:
        ret = true;
        break;
    default:
        break;
    }

    return ret;
}

audio_devices_t get_input_audio_device(audio_devices_t device)
{
    audio_devices_t in_device = device;

    switch (device) {
    case AUDIO_DEVICE_OUT_EARPIECE:
    case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
        in_device = AUDIO_DEVICE_IN_BUILTIN_MIC;
        break;
    case AUDIO_DEVICE_OUT_SPEAKER:
        in_device = AUDIO_DEVICE_IN_BACK_MIC;
        break;
    case AUDIO_DEVICE_OUT_WIRED_HEADSET:
        in_device = AUDIO_DEVICE_IN_WIRED_HEADSET;
        break;
    case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
        in_device = AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET;
        break;
    default:
        break;
    }

    return in_device;
}

static int derive_mixer_ctl_from_usecase_intf(const struct audio_device *adev,
                                              char *mixer_ctl_name) {
    struct audio_usecase *usecase = NULL;
    audio_devices_t in_device;
    int ret = 0;

    if (mixer_ctl_name == NULL) {
        ALOGE("%s: mixer_ctl_name is NULL", __func__);

        ret = -EINVAL;
        goto done;
    }

    if (voice_is_in_call(adev)) {
        strlcat(mixer_ctl_name, " ", MIXER_PATH_MAX_LENGTH);
        strlcat(mixer_ctl_name, "Voice Tx", MIXER_PATH_MAX_LENGTH);
        usecase = get_usecase_from_list(adev,
                                        get_usecase_id_from_usecase_type(adev, VOICE_CALL));
    } else if (voice_extn_compress_voip_is_active(adev)) {
        strlcat(mixer_ctl_name, " ", MIXER_PATH_MAX_LENGTH);
        strlcat(mixer_ctl_name, "Voice Tx", MIXER_PATH_MAX_LENGTH);
        usecase = get_usecase_from_list(adev, USECASE_COMPRESS_VOIP_CALL);
    } else {
        strlcat(mixer_ctl_name, " ", MIXER_PATH_MAX_LENGTH);
        strlcat(mixer_ctl_name, "Audio Tx", MIXER_PATH_MAX_LENGTH);
        usecase = get_usecase_from_list(adev, get_usecase_id_from_usecase_type(adev, PCM_CAPTURE));
    }

    if (usecase && (usecase->id != USECASE_AUDIO_SPKR_CALIB_TX)) {
        if (is_stt_supported_snd_device(usecase->in_snd_device)) {
             in_device = get_input_audio_device(usecase->devices);
             ret = add_audio_intf_name_to_mixer_ctl(in_device, mixer_ctl_name,
                audio_device_to_interface_table, audio_device_to_interface_table_len);
        } else {
            ALOGE("%s: Sound Focus/Source Tracking not supported on the input sound device (%s)",
                    __func__, platform_get_snd_device_name(usecase->in_snd_device));

            ret = -EINVAL;
        }
    } else {
        ALOGE("%s: No use case is active which supports Sound Focus/Source Tracking",
               __func__);

        ret = -EINVAL;
    }

done:
    return ret;
}

static int parse_soundfocus_sourcetracking_keys(struct str_parms *parms)
{
    char *value = NULL;
    int len;
    int ret = 0, err;
    char *kv_pairs = str_parms_to_str(parms);

    if(kv_pairs == NULL) {
        ret = -ENOMEM;
        ALOGE("[%s] key-value pair is NULL",__func__);
        goto done;
    }

    ALOGV_IF(kv_pairs != NULL, "%s: enter: %s", __func__, kv_pairs);

    len = strlen(kv_pairs);
    value = (char*)calloc(len, sizeof(char));
    if(value == NULL) {
        ret = -ENOMEM;
        ALOGE("%s: failed to allocate memory", __func__);

        goto done;
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_SOUND_FOCUS_START_ANGLES,
                            value, len);
    if (err >= 0) {
        str_parms_del(parms, AUDIO_PARAMETER_KEY_SOUND_FOCUS_START_ANGLES);
        ret = ret | BITMASK_AUDIO_PARAMETER_KEY_SOUND_FOCUS_START_ANGLES;
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_SOUND_FOCUS_ENABLE_SECTORS,
                            value, len);
    if (err >= 0) {
        str_parms_del(parms, AUDIO_PARAMETER_KEY_SOUND_FOCUS_ENABLE_SECTORS);
        ret = ret | BITMASK_AUDIO_PARAMETER_KEY_SOUND_FOCUS_ENABLE_SECTORS;
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_SOUND_FOCUS_GAIN_STEP,
                            value, len);
    if (err >= 0) {
        str_parms_del(parms, AUDIO_PARAMETER_KEY_SOUND_FOCUS_GAIN_STEP);
        ret = ret | BITMASK_AUDIO_PARAMETER_KEY_SOUND_FOCUS_GAIN_STEP;
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_SOURCE_TRACK_VAD,
                            value, len);
    if (err >= 0) {
        str_parms_del(parms, AUDIO_PARAMETER_KEY_SOURCE_TRACK_VAD);
        ret = ret | BITMASK_AUDIO_PARAMETER_KEY_SOURCE_TRACK_VAD;
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_SOURCE_TRACK_DOA_SPEECH,
                            value, len);
    if (err >= 0) {
        str_parms_del(parms, AUDIO_PARAMETER_KEY_SOURCE_TRACK_DOA_SPEECH);
        ret = ret | BITMASK_AUDIO_PARAMETER_KEY_SOURCE_TRACK_DOA_SPEECH;
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_SOURCE_TRACK_DOA_NOISE,
                            value, len);
    if (err >= 0) {
        str_parms_del(parms, AUDIO_PARAMETER_KEY_SOURCE_TRACK_DOA_NOISE);
        ret = ret | BITMASK_AUDIO_PARAMETER_KEY_SOURCE_TRACK_DOA_NOISE;
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_SOURCE_TRACK_POLAR_ACTIVITY,
                            value, len);
    if (err >= 0) {
        str_parms_del(parms, AUDIO_PARAMETER_KEY_SOURCE_TRACK_POLAR_ACTIVITY);
        ret = ret | BITMASK_AUDIO_PARAMETER_KEY_SOURCE_TRACK_POLAR_ACTIVITY;
    }

done:
    if (kv_pairs)
        free(kv_pairs);
    if(value != NULL)
        free(value);
    ALOGV("%s: returning bitmask = %d", __func__, ret);

    return ret;
}

static int get_soundfocus_sourcetracking_data(const struct audio_device *adev,
                                        const int bitmask,
                                        struct sound_focus_param *sound_focus_data,
                                        struct source_tracking_param *source_tracking_data)
{
    struct mixer_ctl *ctl;
    char sound_focus_mixer_ctl_name[MIXER_PATH_MAX_LENGTH] = "Sound Focus";
    char source_tracking_mixer_ctl_name[MIXER_PATH_MAX_LENGTH] = "Source Tracking";
    int ret = -EINVAL;
    int count;

    if (bitmask & BITMASK_AUDIO_PARAMETER_KEYS_SOUND_FOCUS) {
        /* Derive the mixer control name based on the use case and the audio interface
         * for the corresponding audio device
         */
        ret = derive_mixer_ctl_from_usecase_intf(adev, sound_focus_mixer_ctl_name);
        if (ret != 0) {
            ALOGE("%s: Could not get Sound Focus Params", __func__);

            goto done;
        } else {
            ALOGV("%s: Mixer Ctl name: %s", __func__, sound_focus_mixer_ctl_name);
        }

        ctl = mixer_get_ctl_by_name(adev->mixer, sound_focus_mixer_ctl_name);
        if (!ctl) {
            ALOGE("%s: Could not get ctl for mixer cmd - %s",
                  __func__, sound_focus_mixer_ctl_name);

            ret = -EINVAL;
            goto done;
        } else {
            ALOGV("%s: Getting Sound Focus Params", __func__);

            mixer_ctl_update(ctl);
            count = mixer_ctl_get_num_values(ctl);
            if (count != sizeof(struct sound_focus_param)) {
                ALOGE("%s: mixer_ctl_get_num_values() invalid sound focus data size", __func__);

                ret = -EINVAL;
                goto done;
            }

            ret = mixer_ctl_get_array(ctl, (void *)sound_focus_data, count);
            if (ret != 0) {
                ALOGE("%s: mixer_ctl_get_array() failed to get Sound Focus Params", __func__);

                ret = -EINVAL;
                goto done;
            }
        }
    }

    if (bitmask & BITMASK_AUDIO_PARAMETER_KEYS_SOURCE_TRACKING) {
        /* Derive the mixer control name based on the use case and the audio interface
         * for the corresponding audio device
         */
        ret = derive_mixer_ctl_from_usecase_intf(adev, source_tracking_mixer_ctl_name);
        if (ret != 0) {
            ALOGE("%s: Could not get Source Tracking Params", __func__);

            goto done;
        } else {
            ALOGV("%s: Mixer Ctl name: %s", __func__, source_tracking_mixer_ctl_name);
        }

        ctl = mixer_get_ctl_by_name(adev->mixer, source_tracking_mixer_ctl_name);
        if (!ctl) {
            ALOGE("%s: Could not get ctl for mixer cmd - %s",
                  __func__, source_tracking_mixer_ctl_name);

            ret = -EINVAL;
            goto done;
        } else {
            ALOGV("%s: Getting Source Tracking Params", __func__);

            mixer_ctl_update(ctl);
            count = mixer_ctl_get_num_values(ctl);
            if (count != sizeof(struct source_tracking_param)) {
                ALOGE("%s: mixer_ctl_get_num_values() invalid source tracking data size", __func__);

                ret = -EINVAL;
                goto done;
            }

            ret = mixer_ctl_get_array(ctl, (void *)source_tracking_data, count);
            if (ret != 0) {
                ALOGE("%s: mixer_ctl_get_array() failed to get Source Tracking Params", __func__);

                ret = -EINVAL;
                goto done;
            }
        }
    }

done:
    return ret;
}

static void send_soundfocus_sourcetracking_params(struct str_parms *reply,
                                                const int bitmask,
                                                const struct sound_focus_param sound_focus_data,
                                                const struct source_tracking_param source_tracking_data)
{
    int i = 0;
    char value[MAX_STR_SIZE] = "";

    if (bitmask & BITMASK_AUDIO_PARAMETER_KEY_SOUND_FOCUS_START_ANGLES) {
        for (i = 0; i < MAX_SECTORS; i++) {
            if ((i >=4) && (sound_focus_data.start_angle[i] == 0xFFFF))
                continue;
            if (i)
                snprintf(value + strlen(value), MAX_STR_SIZE - strlen(value) - 1, ",");
            snprintf(value + strlen(value), MAX_STR_SIZE - strlen(value) - 1, "%d", sound_focus_data.start_angle[i]);
        }
        str_parms_add_str(reply, AUDIO_PARAMETER_KEY_SOUND_FOCUS_START_ANGLES, value);
    }

    strlcpy(value, "", sizeof(""));
    if (bitmask & BITMASK_AUDIO_PARAMETER_KEY_SOUND_FOCUS_ENABLE_SECTORS) {
        for (i = 0; i < MAX_SECTORS; i++) {
            if ((i >=4) && (sound_focus_data.enable[i] == 0xFF))
                continue;
            if (i)
                snprintf(value + strlen(value), MAX_STR_SIZE - strlen(value) - 1, ",");
            snprintf(value + strlen(value), MAX_STR_SIZE - strlen(value) - 1, "%d", sound_focus_data.enable[i]);
        }
        str_parms_add_str(reply, AUDIO_PARAMETER_KEY_SOUND_FOCUS_ENABLE_SECTORS, value);
    }

    if (bitmask & BITMASK_AUDIO_PARAMETER_KEY_SOUND_FOCUS_GAIN_STEP)
        str_parms_add_int(reply, AUDIO_PARAMETER_KEY_SOUND_FOCUS_GAIN_STEP, sound_focus_data.gain_step);

    strlcpy(value, "", sizeof(""));
    if (bitmask & BITMASK_AUDIO_PARAMETER_KEY_SOURCE_TRACK_VAD) {
        for (i = 0; i < MAX_SECTORS; i++) {
            if ((i >=4) && (source_tracking_data.vad[i] == 0xFF))
                continue;
            if (i)
                snprintf(value + strlen(value), MAX_STR_SIZE - strlen(value) - 1, ",");
            snprintf(value + strlen(value), MAX_STR_SIZE - strlen(value) - 1, "%d", source_tracking_data.vad[i]);
        }
        str_parms_add_str(reply, AUDIO_PARAMETER_KEY_SOURCE_TRACK_VAD, value);
    }

    if (bitmask & BITMASK_AUDIO_PARAMETER_KEY_SOURCE_TRACK_DOA_SPEECH)
        str_parms_add_int(reply, AUDIO_PARAMETER_KEY_SOURCE_TRACK_DOA_SPEECH, source_tracking_data.doa_speech);

    strlcpy(value, "", sizeof(""));
    if (bitmask & BITMASK_AUDIO_PARAMETER_KEY_SOURCE_TRACK_DOA_NOISE) {
        snprintf(value, MAX_STR_SIZE,
                     "%d,%d,%d", source_tracking_data.doa_noise[0], source_tracking_data.doa_noise[1], source_tracking_data.doa_noise[2]);
        str_parms_add_str(reply, AUDIO_PARAMETER_KEY_SOURCE_TRACK_DOA_NOISE, value);
    }

    strlcpy(value, "", sizeof(""));
    if (bitmask & BITMASK_AUDIO_PARAMETER_KEY_SOURCE_TRACK_POLAR_ACTIVITY) {
        for (i = 0; i < 360; i++) {
            if (i)
                snprintf(value + strlen(value), MAX_STR_SIZE - strlen(value) - 1, ",");
            snprintf(value + strlen(value), MAX_STR_SIZE - strlen(value) - 1, "%d", source_tracking_data.polar_activity[i]);
        }
        str_parms_add_str(reply, AUDIO_PARAMETER_KEY_SOURCE_TRACK_POLAR_ACTIVITY, value);
    }
}

void audio_extn_source_track_get_parameters(const struct audio_device *adev,
                                            struct str_parms *query,
                                            struct str_parms *reply)
{
    int bitmask = 0, ret = 0;
    struct sound_focus_param sound_focus_data;
    struct source_tracking_param source_tracking_data;

    memset(&sound_focus_data, 0xFF, sizeof(struct sound_focus_param));
    memset(&source_tracking_data, 0xFF, sizeof(struct source_tracking_param));

    // Parse the input parameters string for Source Tracking keys
    bitmask = parse_soundfocus_sourcetracking_keys(query);
    if (bitmask) {
        // Get the parameter values from the backend
        ret = get_soundfocus_sourcetracking_data(adev, bitmask, &sound_focus_data, &source_tracking_data);
        if (ret == 0) {
            // Construct the return string with key, value pairs
            send_soundfocus_sourcetracking_params(reply, bitmask, sound_focus_data, source_tracking_data);
        }
    }
}

void audio_extn_source_track_set_parameters(struct audio_device *adev,
                                            struct str_parms *parms)
{
    int len, ret, count;;
    struct mixer_ctl *ctl;
    char sound_focus_mixer_ctl_name[MIXER_PATH_MAX_LENGTH] = "Sound Focus";
    char *value = NULL;
    char *kv_pairs = str_parms_to_str(parms);

    if(kv_pairs == NULL) {
        ret = -ENOMEM;
        ALOGE("[%s] key-value pair is NULL",__func__);
        goto done;
    }

    len = strlen(kv_pairs);
    value = (char*)calloc(len, sizeof(char));
    if(value == NULL) {
        ret = -ENOMEM;
        ALOGE("%s: failed to allocate memory", __func__);

        goto done;
    }

    // Parse the input parameter string for Source Tracking key, value pairs
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_SOUND_FOCUS_START_ANGLES,
                            value, len);
    if (ret >= 0) {
        char *saveptr, *tok;
        int i = 0, val;
        struct sound_focus_param sound_focus_param;

        memset(&sound_focus_param, 0xFF, sizeof(struct sound_focus_param));

        str_parms_del(parms, AUDIO_PARAMETER_KEY_SOUND_FOCUS_START_ANGLES);
        tok = strtok_r(value, ",", &saveptr);
        while ((i < MAX_SECTORS) && (tok != NULL)) {
            if (sscanf(tok, "%d", &val) == 1) {
                sound_focus_param.start_angle[i++] = (uint16_t)val;
            }
            tok = strtok_r(NULL, ",", &saveptr);
        }

        ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_SOUND_FOCUS_ENABLE_SECTORS,
                                value, len);
        if (ret >= 0) {
            str_parms_del(parms, AUDIO_PARAMETER_KEY_SOUND_FOCUS_ENABLE_SECTORS);
            tok = strtok_r(value, ",", &saveptr);
            i = 0;
            while ((i < MAX_SECTORS) && (tok != NULL)) {
                if (sscanf(tok, "%d", &val) == 1) {
                    sound_focus_param.enable[i++] = (uint8_t)val;
                }
                tok = strtok_r(NULL, ",", &saveptr);
            }
        } else {
            ALOGE("%s: SoundFocus.enable_sectors key not found", __func__);

            goto done;
        }

        ret = str_parms_get_int(parms, AUDIO_PARAMETER_KEY_SOUND_FOCUS_GAIN_STEP, &val);
        if (ret >= 0) {
            str_parms_del(parms, AUDIO_PARAMETER_KEY_SOUND_FOCUS_GAIN_STEP);
            sound_focus_param.gain_step = (uint16_t)val;
        } else {
            ALOGE("%s: SoundFocus.gain_step key not found", __func__);

            goto done;
        }

        /* Derive the mixer control name based on the use case and the audio h/w
         * interface name for the corresponding audio device
         */
        ret = derive_mixer_ctl_from_usecase_intf(adev, sound_focus_mixer_ctl_name);
        if (ret != 0) {
            ALOGE("%s: Could not set Sound Focus Params", __func__);

            goto done;
        } else {
            ALOGV("%s: Mixer Ctl name: %s", __func__, sound_focus_mixer_ctl_name);
        }

        ctl = mixer_get_ctl_by_name(adev->mixer, sound_focus_mixer_ctl_name);
        if (!ctl) {
            ALOGE("%s: Could not get ctl for mixer cmd - %s",
                  __func__, sound_focus_mixer_ctl_name);

            goto done;
        } else {
            ALOGV("%s: Setting Sound Focus Params", __func__);

            for (i = 0; i < MAX_SECTORS;i++) {
                ALOGV("%s: start_angles[%d] = %d", __func__, i, sound_focus_param.start_angle[i]);
            }
            for (i = 0; i < MAX_SECTORS;i++) {
                ALOGV("%s: enable_sectors[%d] = %d", __func__, i, sound_focus_param.enable[i]);
            }
            ALOGV("%s: gain_step = %d", __func__, sound_focus_param.gain_step);

            mixer_ctl_update(ctl);
            count = mixer_ctl_get_num_values(ctl);
            if (count != sizeof(struct sound_focus_param)) {
                ALOGE("%s: mixer_ctl_get_num_values() invalid data size", __func__);

                goto done;
            }

            // Set the parameters on the mixer control derived above
            ret = mixer_ctl_set_array(ctl, (void *)&sound_focus_param, count);
            if (ret != 0) {
                ALOGE("%s: mixer_ctl_set_array() failed to set Sound Focus Params", __func__);

                goto done;
            }
       }
    }

done:
    if (kv_pairs)
        free(kv_pairs);
    if(value != NULL)
        free(value);
    return;
}
#endif /* SOURCE_TRACKING_ENABLED end */
