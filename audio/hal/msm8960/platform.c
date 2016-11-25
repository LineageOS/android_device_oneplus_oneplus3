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

#define LOG_TAG "msm8960_platform"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0

#include <stdlib.h>
#include <dlfcn.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <audio_hw.h>
#include <platform_api.h>
#include "platform.h"
#include "audio_extn.h"

#define LIB_ACDB_LOADER "libacdbloader.so"
#define LIB_CSD_CLIENT "libcsd-client.so"

/*
 * This is the sysfs path for the HDMI audio data block
 */
#define AUDIO_DATA_BLOCK_PATH "/sys/class/graphics/fb1/audio_data_block"
#define MIXER_XML_PATH "/system/etc/mixer_paths.xml"

/*
 * This file will have a maximum of 38 bytes:
 *
 * 4 bytes: number of audio blocks
 * 4 bytes: total length of Short Audio Descriptor (SAD) blocks
 * Maximum 10 * 3 bytes: SAD blocks
 */
#define MAX_SAD_BLOCKS      10
#define SAD_BLOCK_SIZE      3

/* EDID format ID for LPCM audio */
#define EDID_FORMAT_LPCM    1

struct audio_block_header
{
    int reserved;
    int length;
};


typedef void (*acdb_deallocate_t)();
typedef int  (*acdb_init_t)();
typedef void (*acdb_send_audio_cal_t)(int, int);
typedef void (*acdb_send_voice_cal_t)(int, int);

typedef int (*csd_client_init_t)();
typedef int (*csd_client_deinit_t)();
typedef int (*csd_disable_device_t)();
typedef int (*csd_enable_device_t)(int, int, uint32_t);
typedef int (*csd_volume_t)(int);
typedef int (*csd_mic_mute_t)(int);
typedef int (*csd_start_voice_t)();
typedef int (*csd_stop_voice_t)();


struct platform_data {
    struct audio_device *adev;
    bool fluence_in_spkr_mode;
    bool fluence_in_voice_call;
    bool fluence_in_voice_rec;
    int  fluence_type;
    int  dualmic_config;
    bool ec_ref_enabled;

    /* Audio calibration related functions */
    void *acdb_handle;
    acdb_init_t acdb_init;
    acdb_deallocate_t acdb_deallocate;
    acdb_send_audio_cal_t acdb_send_audio_cal;
    acdb_send_voice_cal_t acdb_send_voice_cal;

    /* CSD Client related functions for voice call */
    void *csd_client;
    csd_client_init_t csd_client_init;
    csd_client_deinit_t csd_client_deinit;
    csd_disable_device_t csd_disable_device;
    csd_enable_device_t csd_enable_device;
    csd_volume_t csd_volume;
    csd_mic_mute_t csd_mic_mute;
    csd_start_voice_t csd_start_voice;
    csd_stop_voice_t csd_stop_voice;
};

static const int pcm_device_table[AUDIO_USECASE_MAX][2] = {
    [USECASE_AUDIO_PLAYBACK_DEEP_BUFFER] = {0, 0},
    [USECASE_AUDIO_PLAYBACK_LOW_LATENCY] = {14, 14},
    [USECASE_AUDIO_PLAYBACK_MULTI_CH] = {1, 1},
    [USECASE_AUDIO_RECORD] = {0, 0},
    [USECASE_AUDIO_RECORD_LOW_LATENCY] = {14, 14},
    [USECASE_VOICE_CALL] = {12, 12},
};

/* Array to store sound devices */
static const char * const device_table[SND_DEVICE_MAX] = {
    [SND_DEVICE_NONE] = "none",
    /* Playback sound devices */
    [SND_DEVICE_OUT_HANDSET] = "handset",
    [SND_DEVICE_OUT_SPEAKER] = "speaker",
    [SND_DEVICE_OUT_SPEAKER_REVERSE] = "speaker-reverse",
    [SND_DEVICE_OUT_HEADPHONES] = "headphones",
    [SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES] = "speaker-and-headphones",
    [SND_DEVICE_OUT_VOICE_SPEAKER] = "voice-speaker",
    [SND_DEVICE_OUT_VOICE_HEADPHONES] = "voice-headphones",
    [SND_DEVICE_OUT_HDMI] = "hdmi",
    [SND_DEVICE_OUT_SPEAKER_AND_HDMI] = "speaker-and-hdmi",
    [SND_DEVICE_OUT_BT_SCO] = "bt-sco-headset",
    [SND_DEVICE_OUT_BT_SCO_WB] = "bt-sco-headset-wb",
    [SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES] = "voice-tty-full-headphones",
    [SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES] = "voice-tty-vco-headphones",
    [SND_DEVICE_OUT_VOICE_TTY_HCO_HANDSET] = "voice-tty-hco-handset",
    [SND_DEVICE_OUT_USB_HEADSET] = "usb-headphones",
    [SND_DEVICE_OUT_SPEAKER_AND_USB_HEADSET] = "speaker-and-usb-headphones",

    /* Capture sound devices */
    [SND_DEVICE_IN_HANDSET_MIC] = "handset-mic",
    [SND_DEVICE_IN_SPEAKER_MIC] = "speaker-mic",
    [SND_DEVICE_IN_HEADSET_MIC] = "headset-mic",
    [SND_DEVICE_IN_HANDSET_MIC_AEC] = "handset-mic",
    [SND_DEVICE_IN_SPEAKER_MIC_AEC] = "voice-speaker-mic",
    [SND_DEVICE_IN_HEADSET_MIC_AEC] = "headset-mic",
    [SND_DEVICE_IN_VOICE_SPEAKER_MIC] = "voice-speaker-mic",
    [SND_DEVICE_IN_VOICE_HEADSET_MIC] = "voice-headset-mic",
    [SND_DEVICE_IN_HDMI_MIC] = "hdmi-mic",
    [SND_DEVICE_IN_BT_SCO_MIC] = "bt-sco-mic",
    [SND_DEVICE_IN_BT_SCO_MIC_WB] = "bt-sco-mic-wb",
    [SND_DEVICE_IN_CAMCORDER_MIC] = "camcorder-mic",
    [SND_DEVICE_IN_VOICE_DMIC] = "voice-dmic-ef",
    [SND_DEVICE_IN_VOICE_SPEAKER_DMIC] = "voice-speaker-dmic-ef",
    [SND_DEVICE_IN_VOICE_TTY_FULL_HEADSET_MIC] = "voice-tty-full-headset-mic",
    [SND_DEVICE_IN_VOICE_TTY_VCO_HANDSET_MIC] = "voice-tty-vco-handset-mic",
    [SND_DEVICE_IN_VOICE_TTY_HCO_HEADSET_MIC] = "voice-tty-hco-headset-mic",
    [SND_DEVICE_IN_VOICE_REC_MIC] = "voice-rec-mic",
    [SND_DEVICE_IN_VOICE_REC_DMIC] = "voice-rec-dmic-ef",
    [SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE] = "voice-rec-dmic-ef-fluence",
    [SND_DEVICE_IN_USB_HEADSET_MIC] = "usb-headset-mic",
};

/* ACDB IDs (audio DSP path configuration IDs) for each sound device */
static const int acdb_device_table[SND_DEVICE_MAX] = {
    [SND_DEVICE_NONE] = -1,
    [SND_DEVICE_OUT_HANDSET] = 7,
    [SND_DEVICE_OUT_SPEAKER] = 14,
    [SND_DEVICE_OUT_SPEAKER_REVERSE] = 14,
    [SND_DEVICE_OUT_HEADPHONES] = 10,
    [SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES] = 10,
    [SND_DEVICE_OUT_VOICE_SPEAKER] = 14,
    [SND_DEVICE_OUT_VOICE_HEADPHONES] = 10,
    [SND_DEVICE_OUT_HDMI] = 18,
    [SND_DEVICE_OUT_SPEAKER_AND_HDMI] = 14,
    [SND_DEVICE_OUT_BT_SCO] = 22,
    [SND_DEVICE_OUT_BT_SCO_WB] = 39,
    [SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES] = 17,
    [SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES] = 17,
    [SND_DEVICE_OUT_VOICE_TTY_HCO_HANDSET] = 37,
    [SND_DEVICE_OUT_USB_HEADSET] = 45,
    [SND_DEVICE_OUT_SPEAKER_AND_USB_HEADSET] = 14,

    [SND_DEVICE_IN_HANDSET_MIC] = 4,
    [SND_DEVICE_IN_SPEAKER_MIC] = 4,
    [SND_DEVICE_IN_HEADSET_MIC] = 8,
    [SND_DEVICE_IN_HANDSET_MIC_AEC] = 40,
    [SND_DEVICE_IN_SPEAKER_MIC_AEC] = 42,
    [SND_DEVICE_IN_HEADSET_MIC_AEC] = 47,
    [SND_DEVICE_IN_VOICE_SPEAKER_MIC] = 11,
    [SND_DEVICE_IN_VOICE_HEADSET_MIC] = 8,
    [SND_DEVICE_IN_HDMI_MIC] = 4,
    [SND_DEVICE_IN_BT_SCO_MIC] = 21,
    [SND_DEVICE_IN_BT_SCO_MIC_WB] = 38,
    [SND_DEVICE_IN_CAMCORDER_MIC] = 61,
    [SND_DEVICE_IN_VOICE_DMIC] = 6,
    [SND_DEVICE_IN_VOICE_SPEAKER_DMIC] = 13,
    [SND_DEVICE_IN_VOICE_TTY_FULL_HEADSET_MIC] = 16,
    [SND_DEVICE_IN_VOICE_TTY_VCO_HANDSET_MIC] = 36,
    [SND_DEVICE_IN_VOICE_TTY_HCO_HEADSET_MIC] = 16,
    [SND_DEVICE_IN_VOICE_REC_MIC] = 62,
    [SND_DEVICE_IN_USB_HEADSET_MIC] = 44,
    /* TODO: Update with proper acdb ids */
    [SND_DEVICE_IN_VOICE_REC_DMIC] = 62,
    [SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE] = 6,
};

#define DEEP_BUFFER_PLATFORM_DELAY (29*1000LL)
#define LOW_LATENCY_PLATFORM_DELAY (13*1000LL)

void platform_set_echo_reference(void *platform, bool enable)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    struct audio_device *adev = my_data->adev;

    if (enable) {
        my_data->ec_ref_enabled = enable;
        audio_route_apply_and_update_path(adev->audio_route, "echo-reference");
    } else {
        if (my_data->ec_ref_enabled) {
            audio_route_reset_and_update_path(adev->audio_route, "echo-reference");
            my_data->ec_ref_enabled = enable;
        } else {
            ALOGV("EC Reference is already disabled: %d", my_data->ec_ref_enabled);
        }
    }

    ALOGV("Setting EC Reference: %d", enable);
}

void *platform_init(struct audio_device *adev)
{
    char platform[PROPERTY_VALUE_MAX];
    char baseband[PROPERTY_VALUE_MAX];
    char value[PROPERTY_VALUE_MAX];
    struct platform_data *my_data;
    const char *snd_card_name;
    const char *mixer_ctl_name = "Set HPX ActiveBe";
    struct mixer_ctl *ctl = NULL;

    adev->mixer = mixer_open(MIXER_CARD);

    if (!adev->mixer) {
        ALOGE("Unable to open the mixer, aborting.");
        return NULL;
    }

    adev->audio_route = audio_route_init(MIXER_CARD, MIXER_XML_PATH);
    if (!adev->audio_route) {
        ALOGE("%s: Failed to init audio route controls, aborting.", __func__);
        mixer_close(adev->mixer);
        return NULL;
    }

    my_data = calloc(1, sizeof(struct platform_data));

    snd_card_name = mixer_get_name(adev->mixer);

    my_data->adev = adev;
    my_data->fluence_in_spkr_mode = false;
    my_data->fluence_in_voice_call = false;
    my_data->fluence_in_voice_rec = false;
    my_data->fluence_type = FLUENCE_NONE;

    property_get("ro.qc.sdk.audio.fluencetype", value, "");
    if (!strncmp("fluencepro", value, sizeof("fluencepro"))) {
        my_data->fluence_type = FLUENCE_QUAD_MIC;
    } else if (!strncmp("fluence", value, sizeof("fluence"))) {
        my_data->fluence_type = FLUENCE_DUAL_MIC;
    } else {
        my_data->fluence_type = FLUENCE_NONE;
    }

    if (my_data->fluence_type != FLUENCE_NONE) {
        property_get("persist.audio.fluence.voicecall",value,"");
        if (!strncmp("true", value, sizeof("true"))) {
            my_data->fluence_in_voice_call = true;
        }

        property_get("persist.audio.fluence.voicerec",value,"");
        if (!strncmp("true", value, sizeof("true"))) {
            my_data->fluence_in_voice_rec = true;
        }

        property_get("persist.audio.fluence.speaker",value,"");
        if (!strncmp("true", value, sizeof("true"))) {
            my_data->fluence_in_spkr_mode = true;
        }
    }

    my_data->acdb_handle = dlopen(LIB_ACDB_LOADER, RTLD_NOW);
    if (my_data->acdb_handle == NULL) {
        ALOGE("%s: DLOPEN failed for %s", __func__, LIB_ACDB_LOADER);
    } else {
        ALOGV("%s: DLOPEN successful for %s", __func__, LIB_ACDB_LOADER);
        my_data->acdb_deallocate = (acdb_deallocate_t)dlsym(my_data->acdb_handle,
                                                    "acdb_loader_deallocate_ACDB");
        my_data->acdb_send_audio_cal = (acdb_send_audio_cal_t)dlsym(my_data->acdb_handle,
                                                    "acdb_loader_send_audio_cal");
        if (!my_data->acdb_send_audio_cal)
            ALOGW("%s: Could not find the symbol acdb_send_audio_cal from %s",
                  __func__, LIB_ACDB_LOADER);
        my_data->acdb_send_voice_cal = (acdb_send_voice_cal_t)dlsym(my_data->acdb_handle,
                                                    "acdb_loader_send_voice_cal");
        my_data->acdb_init = (acdb_init_t)dlsym(my_data->acdb_handle,
                                                    "acdb_loader_init_ACDB");
        if (my_data->acdb_init == NULL)
            ALOGE("%s: dlsym error %s for acdb_loader_init_ACDB", __func__, dlerror());
        else
            my_data->acdb_init();
    }

    /* If platform is Fusion3, load CSD Client specific symbols
     * Voice call is handled by MDM and apps processor talks to
     * MDM through CSD Client
     */
    property_get("ro.board.platform", platform, "");
    property_get("ro.baseband", baseband, "");
    if (!strcmp("msm8960", platform) && !strcmp("mdm", baseband)) {
        my_data->csd_client = dlopen(LIB_CSD_CLIENT, RTLD_NOW);
        if (my_data->csd_client == NULL)
            ALOGE("%s: DLOPEN failed for %s", __func__, LIB_CSD_CLIENT);
    }

    if (my_data->csd_client) {
        ALOGV("%s: DLOPEN successful for %s", __func__, LIB_CSD_CLIENT);
        my_data->csd_client_deinit = (csd_client_deinit_t)dlsym(my_data->csd_client,
                                                    "csd_client_deinit");
        my_data->csd_disable_device = (csd_disable_device_t)dlsym(my_data->csd_client,
                                                    "csd_client_disable_device");
        my_data->csd_enable_device = (csd_enable_device_t)dlsym(my_data->csd_client,
                                                    "csd_client_enable_device");
        my_data->csd_start_voice = (csd_start_voice_t)dlsym(my_data->csd_client,
                                                    "csd_client_start_voice");
        my_data->csd_stop_voice = (csd_stop_voice_t)dlsym(my_data->csd_client,
                                                    "csd_client_stop_voice");
        my_data->csd_volume = (csd_volume_t)dlsym(my_data->csd_client,
                                                    "csd_client_volume");
        my_data->csd_mic_mute = (csd_mic_mute_t)dlsym(my_data->csd_client,
                                                    "csd_client_mic_mute");
        my_data->csd_client_init = (csd_client_init_t)dlsym(my_data->csd_client,
                                                    "csd_client_init");

        if (my_data->csd_client_init == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_init", __func__, dlerror());
        } else {
            my_data->csd_client_init();
        }
    }

    /* Configure active back end for HPX*/
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (ctl) {
        ALOGI(" sending HPX Active BE information ");
        mixer_ctl_set_value(ctl, 0, false);
    }

    return my_data;
}

void platform_deinit(void *platform)
{
    struct platform_data *my_data = (struct platform_data *)platform;

    free(platform);
}

const char *platform_get_snd_device_name(snd_device_t snd_device)
{
    if (snd_device >= SND_DEVICE_MIN && snd_device < SND_DEVICE_MAX)
        return device_table[snd_device];
    else
        return "";
}

int platform_get_snd_device_name_extn(void *platform, snd_device_t snd_device,
                                      char *device_name)
{
    struct platform_data *my_data = (struct platform_data *)platform;

    if (snd_device >= SND_DEVICE_MIN && snd_device < SND_DEVICE_MAX) {
        strlcpy(device_name, device_table[snd_device], DEVICE_NAME_MAX_SIZE);
    } else {
        strlcpy(device_name, "", DEVICE_NAME_MAX_SIZE);
        return -EINVAL;
    }

    return 0;
}

void platform_add_backend_name(char *mixer_path, snd_device_t snd_device,
                               struct audio_usecase *usecase __unused)
{
    if (snd_device == SND_DEVICE_IN_BT_SCO_MIC)
        strlcat(mixer_path, " bt-sco", MIXER_PATH_MAX_LENGTH);
    else if (snd_device == SND_DEVICE_IN_BT_SCO_MIC_WB)
        strlcat(mixer_path, " bt-sco-wb", MIXER_PATH_MAX_LENGTH);
    else if(snd_device == SND_DEVICE_OUT_BT_SCO)
        strlcat(mixer_path, " bt-sco", MIXER_PATH_MAX_LENGTH);
    else if(snd_device == SND_DEVICE_OUT_BT_SCO_WB)
        strlcat(mixer_path, " bt-sco-wb", MIXER_PATH_MAX_LENGTH);
    else if (snd_device == SND_DEVICE_OUT_HDMI)
        strlcat(mixer_path, " hdmi", MIXER_PATH_MAX_LENGTH);
    else if (snd_device == SND_DEVICE_OUT_SPEAKER_AND_HDMI)
        strlcat(mixer_path, " speaker-and-hdmi", MIXER_PATH_MAX_LENGTH);
}

int platform_get_pcm_device_id(audio_usecase_t usecase, int device_type)
{
    int device_id;
    if (device_type == PCM_PLAYBACK)
        device_id = pcm_device_table[usecase][0];
    else
        device_id = pcm_device_table[usecase][1];
    return device_id;
}

int platform_get_snd_device_index(char *snd_device_index_name __unused)
{
    return -ENODEV;
}

int platform_set_snd_device_acdb_id(snd_device_t snd_device __unused,
                                    unsigned int acdb_id __unused)
{
    return -ENODEV;
}

uint32_t platform_get_compress_offload_buffer_size(audio_offload_info_t* info __unused)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

int platform_get_snd_device_acdb_id(snd_device_t snd_device __unused)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

int platform_set_snd_device_bit_width(snd_device_t snd_device __unused,
                                      unsigned int bit_width __unused)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

int platform_get_snd_device_bit_width(snd_device_t snd_device __unused)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

int platform_switch_voice_call_enable_device_config(void *platform __unused,
                                                    snd_device_t out_snd_device __unused,
                                                    snd_device_t in_snd_device __unused)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

int platform_switch_voice_call_usecase_route_post(void *platform __unused,
                                                  snd_device_t out_snd_device __unused,
                                                  snd_device_t in_snd_device __unused)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

int platform_set_incall_recording_session_id(void *platform __unused,
                                             uint32_t session_id __unused,
                                             int rec_mode __unused)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

int platform_stop_incall_recording_usecase(void *platform __unused)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

int platform_get_sample_rate(void *platform __unused, uint32_t *rate __unused)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

int platform_get_default_app_type(void *platform __unused)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

int platform_send_audio_calibration(void *platform, struct audio_usecase *usecase,
                                    int app_type __unused, int sample_rate __unused)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int acdb_dev_id, acdb_dev_type;
    struct audio_device *adev = my_data->adev;
    int snd_device = SND_DEVICE_OUT_SPEAKER;

    if (usecase->type == PCM_PLAYBACK)
        snd_device = platform_get_output_snd_device(adev->platform,
                                            usecase->stream.out->devices);
    else if ((usecase->type == PCM_CAPTURE) &&
                   voice_is_in_call_rec_stream(usecase->stream.in))
        snd_device = voice_get_incall_rec_snd_device(usecase->in_snd_device);
    else if ((usecase->type == PCM_HFP_CALL) || (usecase->type == PCM_CAPTURE))
        snd_device = platform_get_input_snd_device(adev->platform,
                                            adev->primary_output->devices);
    acdb_dev_id = acdb_device_table[snd_device];
    if (acdb_dev_id < 0) {
        ALOGE("%s: Could not find acdb id for device(%d)",
              __func__, snd_device);
        return -EINVAL;
    }
    if (my_data->acdb_send_audio_cal) {
        ("%s: sending audio calibration for snd_device(%d) acdb_id(%d)",
              __func__, snd_device, acdb_dev_id);
        if (snd_device >= SND_DEVICE_OUT_BEGIN &&
                snd_device < SND_DEVICE_OUT_END)
            acdb_dev_type = ACDB_DEV_TYPE_OUT;
        else
            acdb_dev_type = ACDB_DEV_TYPE_IN;
        my_data->acdb_send_audio_cal(acdb_dev_id, acdb_dev_type);
    }
    return 0;
}

int platform_switch_voice_call_device_pre(void *platform)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int ret = 0;

    if (my_data->csd_client != NULL &&
        voice_is_in_call(my_data->adev)) {
        /* This must be called before disabling the mixer controls on APQ side */
        if (my_data->csd_disable_device == NULL) {
            ALOGE("%s: dlsym error for csd_disable_device", __func__);
        } else {
            ret = my_data->csd_disable_device();
            if (ret < 0) {
                ALOGE("%s: csd_client_disable_device, failed, error %d",
                      __func__, ret);
            }
        }
    }
    return ret;
}

int platform_switch_voice_call_device_post(void *platform,
                                           snd_device_t out_snd_device,
                                           snd_device_t in_snd_device)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int acdb_rx_id, acdb_tx_id;
    int ret = 0;

    if (my_data->csd_client) {
        if (my_data->csd_enable_device == NULL) {
            ALOGE("%s: dlsym error for csd_enable_device",
                  __func__);
        } else {
            acdb_rx_id = acdb_device_table[out_snd_device];
            acdb_tx_id = acdb_device_table[in_snd_device];

            if (acdb_rx_id > 0 || acdb_tx_id > 0) {
                ret = my_data->csd_enable_device(acdb_rx_id, acdb_tx_id,
                                                    my_data->adev->acdb_settings);
                if (ret < 0) {
                    ALOGE("%s: csd_enable_device, failed, error %d",
                          __func__, ret);
                }
            } else {
                ALOGE("%s: Incorrect ACDB IDs (rx: %d tx: %d)", __func__,
                      acdb_rx_id, acdb_tx_id);
            }
        }
    }

    return ret;
}

int platform_start_voice_call(void *platform, uint32_t vsid __unused)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int ret = 0;

    if (my_data->csd_client) {
        if (my_data->csd_start_voice == NULL) {
            ALOGE("dlsym error for csd_client_start_voice");
            ret = -ENOSYS;
        } else {
            ret = my_data->csd_start_voice();
            if (ret < 0) {
                ALOGE("%s: csd_start_voice error %d\n", __func__, ret);
            }
        }
    }

    return ret;
}

int platform_stop_voice_call(void *platform, uint32_t vsid __unused)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int ret = 0;

    if (my_data->csd_client) {
        if (my_data->csd_stop_voice == NULL) {
            ALOGE("dlsym error for csd_stop_voice");
        } else {
            ret = my_data->csd_stop_voice();
            if (ret < 0) {
                ALOGE("%s: csd_stop_voice error %d\n", __func__, ret);
            }
        }
    }

    return ret;
}

int platform_set_voice_volume(void *platform, int volume)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int ret = 0;

    if (my_data->csd_client) {
        if (my_data->csd_volume == NULL) {
            ALOGE("%s: dlsym error for csd_volume", __func__);
        } else {
            ret = my_data->csd_volume(volume);
            if (ret < 0) {
                ALOGE("%s: csd_volume error %d", __func__, ret);
            }
        }
    } else {
        ALOGE("%s: No CSD Client present", __func__);
    }

    return ret;
}

int platform_set_mic_mute(void *platform, bool state)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int ret = 0;

    if (my_data->adev->mode == AUDIO_MODE_IN_CALL) {
        if (my_data->csd_client) {
            if (my_data->csd_mic_mute == NULL) {
                ALOGE("%s: dlsym error for csd_mic_mute", __func__);
            } else {
                ret = my_data->csd_mic_mute(state);
                if (ret < 0) {
                    ALOGE("%s: csd_mic_mute error %d", __func__, ret);
                }
            }
        } else {
            ALOGE("%s: No CSD Client present", __func__);
        }
    }

    return ret;
}

int platform_set_device_mute(void *platform __unused, bool state __unused, char *dir __unused)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

snd_device_t platform_get_output_snd_device(void *platform, audio_devices_t devices)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    struct audio_device *adev = my_data->adev;
    audio_mode_t mode = adev->mode;
    snd_device_t snd_device = SND_DEVICE_NONE;

    ALOGV("%s: enter: output devices(%#x)", __func__, devices);
    if (devices == AUDIO_DEVICE_NONE ||
        devices & AUDIO_DEVICE_BIT_IN) {
        ALOGV("%s: Invalid output devices (%#x)", __func__, devices);
        goto exit;
    }

    if (mode == AUDIO_MODE_IN_CALL) {
        if (devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE ||
            devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) {
            if (adev->voice.tty_mode == TTY_MODE_FULL)
                snd_device = SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES;
            else if (adev->voice.tty_mode == TTY_MODE_VCO)
                snd_device = SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES;
            else if (adev->voice.tty_mode == TTY_MODE_HCO)
                snd_device = SND_DEVICE_OUT_VOICE_TTY_HCO_HANDSET;
            else
                snd_device = SND_DEVICE_OUT_VOICE_HEADPHONES;
        } else if (devices & AUDIO_DEVICE_OUT_ALL_SCO) {
            if (adev->bt_wb_speech_enabled)
                snd_device = SND_DEVICE_OUT_BT_SCO_WB;
            else
                snd_device = SND_DEVICE_OUT_BT_SCO;
        } else if (devices & AUDIO_DEVICE_OUT_SPEAKER) {
            snd_device = SND_DEVICE_OUT_VOICE_SPEAKER;
        } else if (devices & AUDIO_DEVICE_OUT_EARPIECE) {
            snd_device = SND_DEVICE_OUT_HANDSET;
        }
        if (snd_device != SND_DEVICE_NONE) {
            goto exit;
        }
    }

    if (popcount(devices) == 2) {
        if (devices == (AUDIO_DEVICE_OUT_WIRED_HEADPHONE |
                        AUDIO_DEVICE_OUT_SPEAKER)) {
            snd_device = SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES;
        } else if (devices == (AUDIO_DEVICE_OUT_WIRED_HEADSET |
                               AUDIO_DEVICE_OUT_SPEAKER)) {
            snd_device = SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES;
        } else if (devices == (AUDIO_DEVICE_OUT_AUX_DIGITAL |
                               AUDIO_DEVICE_OUT_SPEAKER)) {
            snd_device = SND_DEVICE_OUT_SPEAKER_AND_HDMI;
        } else {
            ALOGE("%s: Invalid combo device(%#x)", __func__, devices);
            goto exit;
        }
        if (snd_device != SND_DEVICE_NONE) {
            goto exit;
        }
    }

    if (popcount(devices) != 1) {
        ALOGE("%s: Invalid output devices(%#x)", __func__, devices);
        goto exit;
    }

    if (devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE ||
        devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) {
        snd_device = SND_DEVICE_OUT_HEADPHONES;
    } else if (devices & AUDIO_DEVICE_OUT_SPEAKER) {
        if (adev->speaker_lr_swap)
            snd_device = SND_DEVICE_OUT_SPEAKER_REVERSE;
        else
            snd_device = SND_DEVICE_OUT_SPEAKER;
    } else if (devices & AUDIO_DEVICE_OUT_ALL_SCO) {
        if (adev->bt_wb_speech_enabled)
            snd_device = SND_DEVICE_OUT_BT_SCO_WB;
        else
            snd_device = SND_DEVICE_OUT_BT_SCO;
    } else if (devices & AUDIO_DEVICE_OUT_AUX_DIGITAL) {
        snd_device = SND_DEVICE_OUT_HDMI ;
    } else if (devices & AUDIO_DEVICE_OUT_EARPIECE) {
        snd_device = SND_DEVICE_OUT_HANDSET;
    } else {
        ALOGE("%s: Unknown device(s) %#x", __func__, devices);
    }
exit:
    ALOGV("%s: exit: snd_device(%s)", __func__, device_table[snd_device]);
    return snd_device;
}

snd_device_t platform_get_input_snd_device(void *platform, audio_devices_t out_device)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    struct audio_device *adev = my_data->adev;
    audio_source_t  source = (adev->active_input == NULL) ?
                                AUDIO_SOURCE_DEFAULT : adev->active_input->source;

    audio_mode_t    mode   = adev->mode;
    audio_devices_t in_device = ((adev->active_input == NULL) ?
                                    AUDIO_DEVICE_NONE : adev->active_input->device)
                                & ~AUDIO_DEVICE_BIT_IN;
    audio_channel_mask_t channel_mask = (adev->active_input == NULL) ?
                                AUDIO_CHANNEL_IN_MONO : adev->active_input->channel_mask;
    snd_device_t snd_device = SND_DEVICE_NONE;

    ALOGV("%s: enter: out_device(%#x) in_device(%#x)",
          __func__, out_device, in_device);
    if ((out_device != AUDIO_DEVICE_NONE) && (mode == AUDIO_MODE_IN_CALL)) {
        if (adev->voice.tty_mode != TTY_MODE_OFF) {
            if (out_device & AUDIO_DEVICE_OUT_WIRED_HEADPHONE ||
                out_device & AUDIO_DEVICE_OUT_WIRED_HEADSET) {
                switch (adev->voice.tty_mode) {
                case TTY_MODE_FULL:
                    snd_device = SND_DEVICE_IN_VOICE_TTY_FULL_HEADSET_MIC;
                    break;
                case TTY_MODE_VCO:
                    snd_device = SND_DEVICE_IN_VOICE_TTY_VCO_HANDSET_MIC;
                    break;
                case TTY_MODE_HCO:
                    snd_device = SND_DEVICE_IN_VOICE_TTY_HCO_HEADSET_MIC;
                    break;
                default:
                    ALOGE("%s: Invalid TTY mode (%#x)", __func__, adev->voice.tty_mode);
                }
                goto exit;
            }
        }
        if (out_device & AUDIO_DEVICE_OUT_EARPIECE ||
            out_device & AUDIO_DEVICE_OUT_WIRED_HEADPHONE) {
            if (my_data->fluence_type == FLUENCE_NONE ||
                my_data->fluence_in_voice_call == false) {
                snd_device = SND_DEVICE_IN_HANDSET_MIC;
            } else {
                snd_device = SND_DEVICE_IN_VOICE_DMIC;
                adev->acdb_settings |= DMIC_FLAG;
            }
        } else if (out_device & AUDIO_DEVICE_OUT_WIRED_HEADSET) {
            snd_device = SND_DEVICE_IN_VOICE_HEADSET_MIC;
        } else if (out_device & AUDIO_DEVICE_OUT_ALL_SCO) {
            if (adev->bt_wb_speech_enabled)
                snd_device = SND_DEVICE_IN_BT_SCO_MIC_WB;
            else
                snd_device = SND_DEVICE_IN_BT_SCO_MIC;
        } else if (out_device & AUDIO_DEVICE_OUT_SPEAKER) {
            if (my_data->fluence_type != FLUENCE_NONE &&
                my_data->fluence_in_voice_call &&
                my_data->fluence_in_spkr_mode) {
                if(my_data->fluence_type == FLUENCE_DUAL_MIC) {
                    adev->acdb_settings |= DMIC_FLAG;
                    snd_device = SND_DEVICE_IN_VOICE_SPEAKER_DMIC;
                } else {
                    adev->acdb_settings |= QMIC_FLAG;
                    snd_device = SND_DEVICE_IN_VOICE_SPEAKER_QMIC;
                }
            } else {
                snd_device = SND_DEVICE_IN_VOICE_SPEAKER_MIC;
            }
        }
    } else if (source == AUDIO_SOURCE_CAMCORDER) {
        if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC ||
            in_device & AUDIO_DEVICE_IN_BACK_MIC) {
            snd_device = SND_DEVICE_IN_CAMCORDER_MIC;
        }
    } else if (source == AUDIO_SOURCE_VOICE_RECOGNITION) {
        if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC) {
            if (channel_mask == AUDIO_CHANNEL_IN_FRONT_BACK)
                snd_device = SND_DEVICE_IN_VOICE_REC_DMIC;
            else if (my_data->fluence_in_voice_rec)
                snd_device = SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE;

            if (snd_device == SND_DEVICE_NONE)
                snd_device = SND_DEVICE_IN_VOICE_REC_MIC;
            else
                adev->acdb_settings |= DMIC_FLAG;
        }
    } else if (source == AUDIO_SOURCE_VOICE_COMMUNICATION) {
        if (out_device & AUDIO_DEVICE_OUT_SPEAKER)
            in_device = AUDIO_DEVICE_IN_BACK_MIC;
        if (adev->active_input) {
            if (adev->active_input->enable_aec) {
                if (in_device & AUDIO_DEVICE_IN_BACK_MIC) {
                    snd_device = SND_DEVICE_IN_SPEAKER_MIC_AEC;
                } else if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC) {
                    snd_device = SND_DEVICE_IN_HANDSET_MIC_AEC;
                } else if (in_device & AUDIO_DEVICE_IN_WIRED_HEADSET) {
                    snd_device = SND_DEVICE_IN_HEADSET_MIC_AEC;
                }
                platform_set_echo_reference(adev->platform, true);
            } else
                platform_set_echo_reference(adev->platform, false);
        }
    } else if (source == AUDIO_SOURCE_DEFAULT) {
        goto exit;
    }


    if (snd_device != SND_DEVICE_NONE) {
        goto exit;
    }

    if (in_device != AUDIO_DEVICE_NONE &&
            !(in_device & AUDIO_DEVICE_IN_VOICE_CALL) &&
            !(in_device & AUDIO_DEVICE_IN_COMMUNICATION)) {
        if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC) {
            snd_device = SND_DEVICE_IN_HANDSET_MIC;
        } else if (in_device & AUDIO_DEVICE_IN_BACK_MIC) {
            snd_device = SND_DEVICE_IN_SPEAKER_MIC;
        } else if (in_device & AUDIO_DEVICE_IN_WIRED_HEADSET) {
            snd_device = SND_DEVICE_IN_HEADSET_MIC;
        } else if (in_device & AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET) {
            if (adev->bt_wb_speech_enabled)
                snd_device = SND_DEVICE_IN_BT_SCO_MIC_WB;
            else
                snd_device = SND_DEVICE_IN_BT_SCO_MIC;
        } else if (in_device & AUDIO_DEVICE_IN_AUX_DIGITAL) {
            snd_device = SND_DEVICE_IN_HDMI_MIC;
        } else {
            ALOGE("%s: Unknown input device(s) %#x", __func__, in_device);
            ALOGW("%s: Using default handset-mic", __func__);
            snd_device = SND_DEVICE_IN_HANDSET_MIC;
        }
    } else {
        if (out_device & AUDIO_DEVICE_OUT_EARPIECE) {
            snd_device = SND_DEVICE_IN_HANDSET_MIC;
        } else if (out_device & AUDIO_DEVICE_OUT_WIRED_HEADSET) {
            snd_device = SND_DEVICE_IN_HEADSET_MIC;
        } else if (out_device & AUDIO_DEVICE_OUT_SPEAKER) {
            snd_device = SND_DEVICE_IN_SPEAKER_MIC;
        } else if (out_device & AUDIO_DEVICE_OUT_WIRED_HEADPHONE) {
            snd_device = SND_DEVICE_IN_HANDSET_MIC;
        } else if (out_device & AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET) {
            if (adev->bt_wb_speech_enabled)
                snd_device = SND_DEVICE_IN_BT_SCO_MIC_WB;
            else
                snd_device = SND_DEVICE_IN_BT_SCO_MIC;
        } else if (out_device & AUDIO_DEVICE_OUT_AUX_DIGITAL) {
            snd_device = SND_DEVICE_IN_HDMI_MIC;
        } else {
            ALOGE("%s: Unknown output device(s) %#x", __func__, out_device);
            ALOGW("%s: Using default handset-mic", __func__);
            snd_device = SND_DEVICE_IN_HANDSET_MIC;
        }
    }
exit:
    ALOGV("%s: exit: in_snd_device(%s)", __func__, device_table[snd_device]);
    return snd_device;
}

int platform_set_hdmi_channels(void *platform,  int channel_count)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    struct audio_device *adev = my_data->adev;
    struct mixer_ctl *ctl;
    const char *channel_cnt_str = NULL;
    const char *mixer_ctl_name = "HDMI_RX Channels";
    switch (channel_count) {
    case 8:
        channel_cnt_str = "Eight"; break;
    case 7:
        channel_cnt_str = "Seven"; break;
    case 6:
        channel_cnt_str = "Six"; break;
    case 5:
        channel_cnt_str = "Five"; break;
    case 4:
        channel_cnt_str = "Four"; break;
    case 3:
        channel_cnt_str = "Three"; break;
    default:
        channel_cnt_str = "Two"; break;
    }
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        return -EINVAL;
    }
    ALOGV("HDMI channel count: %s", channel_cnt_str);
    mixer_ctl_set_enum_by_string(ctl, channel_cnt_str);
    return 0;
}

int platform_edid_get_max_channels(void *platform __unused)
{
    FILE *file;
    struct audio_block_header header;
    char block[MAX_SAD_BLOCKS * SAD_BLOCK_SIZE];
    char *sad = block;
    int num_audio_blocks;
    int channel_count;
    int max_channels = 0;
    int i;

    file = fopen(AUDIO_DATA_BLOCK_PATH, "rb");
    if (file == NULL) {
        ALOGE("Unable to open '%s'", AUDIO_DATA_BLOCK_PATH);
        return 0;
    }

    /* Read audio block header */
    fread(&header, 1, sizeof(header), file);

    /* Read SAD blocks, clamping the maximum size for safety */
    if (header.length > (int)sizeof(block))
        header.length = (int)sizeof(block);
    fread(&block, header.length, 1, file);

    fclose(file);

    /* Calculate the number of SAD blocks */
    num_audio_blocks = header.length / SAD_BLOCK_SIZE;

    for (i = 0; i < num_audio_blocks; i++) {
        /* Only consider LPCM blocks */
        if ((sad[0] >> 3) != EDID_FORMAT_LPCM)
            continue;

        channel_count = (sad[0] & 0x7) + 1;
        if (channel_count > max_channels)
            max_channels = channel_count;

        /* Advance to next block */
        sad += 3;
    }

    return max_channels;
}

void platform_get_parameters(void *platform __unused,
                             struct str_parms *query __unused,
                             struct str_parms *reply __unused)
{
    ALOGE("%s: Not implemented", __func__);
}

int platform_set_parameters(void *platform __unused, struct str_parms *parms __unused)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

int platform_set_incall_recoding_session_id(void *platform __unused,
                                            uint32_t session_id __unused)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

int platform_update_lch(void *platform __unused,
                        struct voice_session *session __unused,
                        enum voice_lch_mode lch_mode __unused)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

int platform_start_incall_music_usecase(void *platform __unused)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

int platform_stop_incall_music_usecase(void *platform __unused)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

/* Delay in Us */
int64_t platform_render_latency(audio_usecase_t usecase)
{
    switch (usecase) {
        case USECASE_AUDIO_PLAYBACK_DEEP_BUFFER:
            return DEEP_BUFFER_PLATFORM_DELAY;
        case USECASE_AUDIO_PLAYBACK_LOW_LATENCY:
            return LOW_LATENCY_PLATFORM_DELAY;
        default:
            return 0;
    }
}

int platform_update_usecase_from_source(int source, int usecase)
{
    ALOGV("%s: input source :%d", __func__, source);
    return usecase;
}

bool platform_listen_device_needs_event(snd_device_t snd_device __unused)
{
    return false;
}

bool platform_listen_usecase_needs_event(audio_usecase_t uc_id __unused)
{
    return false;
}

bool platform_check_and_set_codec_backend_cfg(struct audio_device* adev __unused,
                                              struct audio_usecase *usecase __unused)
{
    return false;
}

bool platform_check_and_set_capture_codec_backend_cfg(struct audio_device* adev __unused,
                                              struct audio_usecase *usecase __unused)
{
    return false;
}

int platform_get_usecase_index(const char * usecase __unused)
{
    return -ENOSYS;
}

int platform_set_usecase_pcm_id(audio_usecase_t usecase __unused, int32_t type __unused,
                                int32_t pcm_id __unused)
{
    return -ENOSYS;
}

int platform_set_snd_device_backend(snd_device_t device __unused,
                                    const char *backend __unused,
                                    const char *hw_interface __unused)
{
    return -ENOSYS;
}

bool platform_sound_trigger_device_needs_event(snd_device_t snd_device __unused)
{
    return false;
}

bool platform_sound_trigger_usecase_needs_event(audio_usecase_t uc_id  __unused)
{
    return false;
}

int platform_set_fluence_type(void *platform __unused, char *value __unused)
{
    return -ENOSYS;
}

int platform_get_fluence_type(void *platform __unused, char *value __unused,
                              uint32_t len __unused)
{
    return -ENOSYS;
}

uint32_t platform_get_pcm_offload_buffer_size(audio_offload_info_t* info __unused)
{
    return 0;
}

int platform_get_edid_info(void *platform __unused)
{
   return -ENOSYS;
}

int platform_set_channel_map(void *platform __unused, int ch_count __unused,
                             char *ch_map __unused, int snd_id __unused)
{
    return -ENOSYS;
}

int platform_set_stream_channel_map(void *platform __unused,
                                    audio_channel_mask_t channel_mask __unused,
                                    int snd_id __unused)
{
    return -ENOSYS;
}

int platform_set_edid_channels_configuration(void *platform __unused,
                                             int channels __unused)
{
    return 0;
}

unsigned char platform_map_to_edid_format(int format __unused)
{
    return 0;
}

bool platform_is_edid_supported_format(void *platform __unused,
                                       int format __unused)
{
    return  false;
}

bool platform_is_edid_supported_sample_rate(void *platform __unused,
                                    int sample_rate __unused)
{
    return false;
}

void platform_cache_edid(void * platform __unused)
{

}

void platform_invalidate_hdmi_config(void * platform __unused)
{

}

int platform_set_hdmi_config(void *platform __unused,
                                    uint32_t channel_count __unused,
                                    uint32_t sample_rate __unused,
                                    bool enable_passthrough __unused)
{
    return 0;
}

int platform_set_device_params(struct stream_out *out __unused,
                                  int param __unused, int value __unused)
{
    return 0;
}

int platform_set_audio_device_interface(const char * device_name __unused,
                                        const char *intf_name __unused,
                                        const char *codec_type __unused)
{
    return -ENOSYS;
}

bool platform_send_gain_dep_cal(void *platform __unused,
                                int level __unused)
{
    return 0;
}

void platform_set_gsm_mode(void *platform __unused, bool enable __unused)
{
    ALOGE("%s: Not implemented", __func__);
}

int platform_set_snd_device_name(snd_device_t snd_device __unused,
                                 const char * name __unused)
{
    return -ENOSYS;
}

bool platform_can_enable_spkr_prot_on_device(snd_device_t snd_device __unused)
{
    /* speaker protection not implemented for this platform*/
    return false;
}

int platform_get_spkr_prot_acdb_id(snd_device_t snd_device __unused)
{
    return -ENOSYS;
}

int platform_get_spkr_prot_snd_device(snd_device_t snd_device __unused)
{
    return -ENOSYS;
}

int platform_spkr_prot_is_wsa_analog_mode(void *adev __unused)
{
    return 0;
bool platform_can_split_snd_device(void *platform __unused,
                                   snd_device_t in_snd_device __unused,
                                   int *num_devices __unused,
                                   snd_device_t *out_snd_devices __unused)
{
    return false;
}

bool platform_check_backends_match(snd_device_t snd_device1 __unused,
                                   snd_device_t snd_device2 __unused)
{
    return true;
}

int platform_set_sidetone(struct audio_device *adev,
                          snd_device_t out_snd_device,
                          bool enable,
                          char *str)
{
    int ret;
    if (out_snd_device == SND_DEVICE_OUT_USB_HEADSET) {
            ret = audio_extn_usb_enable_sidetone(out_snd_device, enable);
            if (ret)
                ALOGI("%s: usb device %d does not support device sidetone\n",
                  __func__, out_snd_device);
    } else {
        ALOGV("%s: sidetone out device(%d) mixer cmd = %s\n",
              __func__, out_snd_device, str);

        if (enable)
            audio_route_apply_and_update_path(adev->audio_route, str);
        else
            audio_route_reset_and_update_path(adev->audio_route, str);
    }
    return 0;
}
