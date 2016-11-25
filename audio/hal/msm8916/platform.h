/*
 * Copyright (c) 2013-2016, The Linux Foundation. All rights reserved.
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

#ifndef QCOM_AUDIO_PLATFORM_H
#define QCOM_AUDIO_PLATFORM_H
#include <sound/voice_params.h>

enum {
    FLUENCE_NONE,
    FLUENCE_DUAL_MIC = 0x1,
    FLUENCE_QUAD_MIC = 0x2,
};

enum {
    FLUENCE_ENDFIRE = 0x1,
    FLUENCE_BROADSIDE = 0x2,
};

enum {
    SOURCE_MONO_MIC  = 0x1,            /* Target contains 1 mic */
    SOURCE_DUAL_MIC  = 0x2,            /* Target contains 2 mics */
    SOURCE_THREE_MIC = 0x4,            /* Target contains 3 mics */
    SOURCE_QUAD_MIC  = 0x8,            /* Target contains 4 mics */
};

#define PLATFORM_IMAGE_NAME "modem"

/*
 * Below are the devices for which is back end is same, SLIMBUS_0_RX.
 * All these devices are handled by the internal HW codec. We can
 * enable any one of these devices at any time. An exception here is
 * 44.1k headphone which uses different backend. This is filtered
 * as different hal internal device in the code but remains same
 * as standard android device AUDIO_DEVICE_OUT_WIRED_HEADPHONE
 * for other layers.
 */
#define AUDIO_DEVICE_OUT_ALL_CODEC_BACKEND \
    (AUDIO_DEVICE_OUT_EARPIECE | AUDIO_DEVICE_OUT_SPEAKER | \
     AUDIO_DEVICE_OUT_WIRED_HEADSET | AUDIO_DEVICE_OUT_WIRED_HEADPHONE|\
     AUDIO_DEVICE_OUT_LINE)

/*
 * Below are the input devices for which back end is same, SLIMBUS_0_TX.
 * All these devices are handled by the internal HW codec. We can
 * enable any one of these devices at any time
 */
#define AUDIO_DEVICE_IN_ALL_CODEC_BACKEND \
    (AUDIO_DEVICE_IN_BUILTIN_MIC | AUDIO_DEVICE_IN_BACK_MIC | \
     AUDIO_DEVICE_IN_WIRED_HEADSET | AUDIO_DEVICE_IN_VOICE_CALL) & ~AUDIO_DEVICE_BIT_IN

/* Sound devices specific to the platform
 * The DEVICE_OUT_* and DEVICE_IN_* should be mapped to these sound
 * devices to enable corresponding mixer paths
 */
enum {
    SND_DEVICE_NONE = 0,

    /* Playback devices */
    SND_DEVICE_MIN,
    SND_DEVICE_OUT_BEGIN = SND_DEVICE_MIN,
    SND_DEVICE_OUT_HANDSET = SND_DEVICE_OUT_BEGIN,
    SND_DEVICE_OUT_SPEAKER,
    SND_DEVICE_OUT_SPEAKER_EXTERNAL_1,
    SND_DEVICE_OUT_SPEAKER_EXTERNAL_2,
    SND_DEVICE_OUT_SPEAKER_REVERSE,
    SND_DEVICE_OUT_SPEAKER_WSA,
    SND_DEVICE_OUT_SPEAKER_VBAT,
    SND_DEVICE_OUT_LINE,
    SND_DEVICE_OUT_HEADPHONES,
    SND_DEVICE_OUT_HEADPHONES_44_1,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES,
    SND_DEVICE_OUT_SPEAKER_AND_LINE,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES_EXTERNAL_1,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES_EXTERNAL_2,
    SND_DEVICE_OUT_VOICE_HANDSET,
    SND_DEVICE_OUT_VOICE_SPEAKER,
    SND_DEVICE_OUT_VOICE_SPEAKER_WSA,
    SND_DEVICE_OUT_VOICE_SPEAKER_VBAT,
    SND_DEVICE_OUT_VOICE_HEADPHONES,
    SND_DEVICE_OUT_VOICE_LINE,
    SND_DEVICE_OUT_HDMI,
    SND_DEVICE_OUT_SPEAKER_AND_HDMI,
    SND_DEVICE_OUT_BT_SCO,
    SND_DEVICE_OUT_BT_SCO_WB,
    SND_DEVICE_OUT_BT_A2DP,
    SND_DEVICE_OUT_SPEAKER_AND_BT_A2DP,
    SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES,
    SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES,
    SND_DEVICE_OUT_VOICE_TTY_HCO_HANDSET,
    SND_DEVICE_OUT_VOICE_TX,
    SND_DEVICE_OUT_AFE_PROXY,
    SND_DEVICE_OUT_USB_HEADSET,
    SND_DEVICE_OUT_SPEAKER_AND_USB_HEADSET,
    SND_DEVICE_OUT_TRANSMISSION_FM,
    SND_DEVICE_OUT_ANC_HEADSET,
    SND_DEVICE_OUT_ANC_FB_HEADSET,
    SND_DEVICE_OUT_VOICE_ANC_HEADSET,
    SND_DEVICE_OUT_VOICE_ANC_FB_HEADSET,
    SND_DEVICE_OUT_SPEAKER_AND_ANC_HEADSET,
    SND_DEVICE_OUT_ANC_HANDSET,
    SND_DEVICE_OUT_SPEAKER_PROTECTED,
    SND_DEVICE_OUT_VOICE_SPEAKER_PROTECTED,
    SND_DEVICE_OUT_SPEAKER_PROTECTED_VBAT,
    SND_DEVICE_OUT_VOICE_SPEAKER_PROTECTED_VBAT,
#ifdef RECORD_PLAY_CONCURRENCY
    SND_DEVICE_OUT_VOIP_HANDSET,
    SND_DEVICE_OUT_VOIP_SPEAKER,
    SND_DEVICE_OUT_VOIP_HEADPHONES,
#endif
    SND_DEVICE_OUT_END,

    /*
     * Note: IN_BEGIN should be same as OUT_END because total number of devices
     * SND_DEVICES_MAX should not exceed MAX_RX + MAX_TX devices.
     */
    /* Capture devices */
    SND_DEVICE_IN_BEGIN = SND_DEVICE_OUT_END,
    SND_DEVICE_IN_HANDSET_MIC  = SND_DEVICE_IN_BEGIN,
    SND_DEVICE_IN_HANDSET_MIC_EXTERNAL,
    SND_DEVICE_IN_HANDSET_MIC_AEC,
    SND_DEVICE_IN_HANDSET_MIC_NS,
    SND_DEVICE_IN_HANDSET_MIC_AEC_NS,
    SND_DEVICE_IN_HANDSET_DMIC,
    SND_DEVICE_IN_HANDSET_DMIC_AEC,
    SND_DEVICE_IN_HANDSET_DMIC_NS,
    SND_DEVICE_IN_HANDSET_DMIC_AEC_NS,
    SND_DEVICE_IN_SPEAKER_MIC,
    SND_DEVICE_IN_SPEAKER_MIC_AEC,
    SND_DEVICE_IN_SPEAKER_MIC_NS,
    SND_DEVICE_IN_SPEAKER_MIC_AEC_NS,
    SND_DEVICE_IN_SPEAKER_DMIC,
    SND_DEVICE_IN_SPEAKER_DMIC_AEC,
    SND_DEVICE_IN_SPEAKER_DMIC_NS,
    SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS,
    SND_DEVICE_IN_HEADSET_MIC,
    SND_DEVICE_IN_HEADSET_MIC_FLUENCE,
    SND_DEVICE_IN_VOICE_SPEAKER_MIC,
    SND_DEVICE_IN_VOICE_HEADSET_MIC,
    SND_DEVICE_IN_HDMI_MIC,
    SND_DEVICE_IN_BT_SCO_MIC,
    SND_DEVICE_IN_BT_SCO_MIC_NREC,
    SND_DEVICE_IN_BT_SCO_MIC_WB,
    SND_DEVICE_IN_BT_SCO_MIC_WB_NREC,
    SND_DEVICE_IN_CAMCORDER_MIC,
    SND_DEVICE_IN_VOICE_DMIC,
    SND_DEVICE_IN_VOICE_SPEAKER_DMIC,
    SND_DEVICE_IN_VOICE_SPEAKER_QMIC,
    SND_DEVICE_IN_VOICE_TTY_FULL_HEADSET_MIC,
    SND_DEVICE_IN_VOICE_TTY_VCO_HANDSET_MIC,
    SND_DEVICE_IN_VOICE_TTY_HCO_HEADSET_MIC,
    SND_DEVICE_IN_VOICE_REC_MIC,
    SND_DEVICE_IN_VOICE_REC_MIC_NS,
    SND_DEVICE_IN_VOICE_REC_DMIC_STEREO,
    SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE,
    SND_DEVICE_IN_VOICE_RX,
    SND_DEVICE_IN_USB_HEADSET_MIC,
    SND_DEVICE_IN_CAPTURE_FM,
    SND_DEVICE_IN_AANC_HANDSET_MIC,
    SND_DEVICE_IN_QUAD_MIC,
    SND_DEVICE_IN_HANDSET_STEREO_DMIC,
    SND_DEVICE_IN_SPEAKER_STEREO_DMIC,
    SND_DEVICE_IN_CAPTURE_VI_FEEDBACK,
    SND_DEVICE_IN_VOICE_SPEAKER_DMIC_BROADSIDE,
    SND_DEVICE_IN_SPEAKER_DMIC_BROADSIDE,
    SND_DEVICE_IN_SPEAKER_DMIC_AEC_BROADSIDE,
    SND_DEVICE_IN_SPEAKER_DMIC_NS_BROADSIDE,
    SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS_BROADSIDE,
    SND_DEVICE_IN_VOICE_FLUENCE_DMIC_AANC,
    SND_DEVICE_IN_HANDSET_QMIC,
    SND_DEVICE_IN_SPEAKER_QMIC_AEC,
    SND_DEVICE_IN_SPEAKER_QMIC_NS,
    SND_DEVICE_IN_SPEAKER_QMIC_AEC_NS,
    SND_DEVICE_IN_THREE_MIC,
    SND_DEVICE_IN_HANDSET_TMIC,
    SND_DEVICE_IN_UNPROCESSED_MIC,
    SND_DEVICE_IN_UNPROCESSED_STEREO_MIC,
    SND_DEVICE_IN_UNPROCESSED_THREE_MIC,
    SND_DEVICE_IN_UNPROCESSED_QUAD_MIC,
    SND_DEVICE_IN_UNPROCESSED_HEADSET_MIC,
    SND_DEVICE_IN_END,

    SND_DEVICE_MAX = SND_DEVICE_IN_END,

};

#define DEFAULT_OUTPUT_SAMPLING_RATE 48000
#define OUTPUT_SAMPLING_RATE_44100      44100
#define MAX_PORT                        6
#define ALL_CODEC_BACKEND_PORT          0
#define HEADPHONE_44_1_BACKEND_PORT     5
#define MAX_CODEC_TX_BACKENDS           1
enum {
    DEFAULT_CODEC_BACKEND,
    SLIMBUS_0_RX = DEFAULT_CODEC_BACKEND,
    HEADPHONE_44_1_BACKEND,
    SLIMBUS_5_RX = HEADPHONE_44_1_BACKEND,
    HEADPHONE_BACKEND,
    SLIMBUS_6_RX = HEADPHONE_BACKEND,
    HDMI_RX_BACKEND,
    USB_AUDIO_RX_BACKEND,
    MAX_CODEC_BACKENDS
};
#define AUDIO_PARAMETER_KEY_NATIVE_AUDIO "audio.nat.codec.enabled"
#define AUDIO_PARAMETER_KEY_NATIVE_AUDIO_MODE "native_audio_mode"

#define AUDIO_PARAMETER_KEY_TRUE_32_BIT "true_32_bit"

#define ALL_SESSION_VSID                0xFFFFFFFF
#define DEFAULT_MUTE_RAMP_DURATION_MS   20
#define DEFAULT_VOLUME_RAMP_DURATION_MS 20
#define MIXER_PATH_MAX_LENGTH 100
#define SND_CARD_MAX_LENGTH 100
#define CODEC_VERSION_MAX_LENGTH 100

#define MAX_VOL_INDEX 5
#define MIN_VOL_INDEX 0
#define percent_to_index(val, min, max) \
            ((val) * ((max) - (min)) * 0.01 + (min) + .5)

/*
 * tinyAlsa library interprets period size as number of frames
 * one frame = channel_count * sizeof (pcm sample)
 * so if format = 16-bit PCM and channels = Stereo, frame size = 2 ch * 2 = 4 bytes
 * DEEP_BUFFER_OUTPUT_PERIOD_SIZE = 1024 means 1024 * 4 = 4096 bytes
 * We should take care of returning proper size when AudioFlinger queries for
 * the buffer size of an input/output stream
 */
#define DEEP_BUFFER_OUTPUT_PERIOD_SIZE 1920
#define DEEP_BUFFER_OUTPUT_PERIOD_COUNT 2
#define LOW_LATENCY_OUTPUT_PERIOD_SIZE 240
#define LOW_LATENCY_OUTPUT_PERIOD_COUNT 2

#define LOW_LATENCY_CAPTURE_SAMPLE_RATE 48000
#define LOW_LATENCY_CAPTURE_PERIOD_SIZE 240
#define LOW_LATENCY_CAPTURE_USE_CASE 1

#define HDMI_MULTI_PERIOD_SIZE  336
#define HDMI_MULTI_PERIOD_COUNT 8
#define HDMI_MULTI_DEFAULT_CHANNEL_COUNT 6
#define HDMI_MULTI_PERIOD_BYTES (HDMI_MULTI_PERIOD_SIZE * HDMI_MULTI_DEFAULT_CHANNEL_COUNT * 2)


/* Used in calculating fragment size for pcm offload */
#define PCM_OFFLOAD_BUFFER_DURATION 40 /* 40 millisecs */

/* MAX PCM fragment size cannot be increased  further due
 * to flinger's cblk size of 1mb,and it has to be a multiple of
 * 24 - lcm of channels supported by DSP
 */
#define MAX_PCM_OFFLOAD_FRAGMENT_SIZE (240 * 1024)
#define MIN_PCM_OFFLOAD_FRAGMENT_SIZE  512

#define AUDIO_CAPTURE_PERIOD_DURATION_MSEC 20
#define AUDIO_CAPTURE_PERIOD_COUNT 2

#define DEVICE_NAME_MAX_SIZE 128
#define HW_INFO_ARRAY_MAX_SIZE 32

#define DEEP_BUFFER_PCM_DEVICE 0
#define AUDIO_RECORD_PCM_DEVICE 0
#define MULTIMEDIA2_PCM_DEVICE 1
#define MULTIMEDIA3_PCM_DEVICE 4
#define FM_PLAYBACK_PCM_DEVICE 5
#define FM_CAPTURE_PCM_DEVICE  6
#define HFP_PCM_RX 5
#define HFP_SCO_RX 17
#define HFP_ASM_RX_TX 18

#define INCALL_MUSIC_UPLINK_PCM_DEVICE 1
#define INCALL_MUSIC_UPLINK2_PCM_DEVICE 16
#define SPKR_PROT_CALIB_RX_PCM_DEVICE 5
#define SPKR_PROT_CALIB_TX_PCM_DEVICE 26
#define PLAYBACK_OFFLOAD_DEVICE 9
#define PLAYBACK_OFFLOAD_DEVICE2 24
#define COMPRESS_VOIP_CALL_PCM_DEVICE 3

/* Define macro for Internal FM volume mixer */
#ifdef PLATFORM_MSMFALCON
#define FM_RX_VOLUME "SLIMBUS_8 LOOPBACK Volume"
#else
#define FM_RX_VOLUME "Internal FM RX Volume"
#endif

#define LOWLATENCY_PCM_DEVICE 12
#define EC_REF_RX "I2S_RX"
#define COMPRESS_CAPTURE_DEVICE 19

#define VOICE_CALL_PCM_DEVICE 2
#define VOICE2_CALL_PCM_DEVICE 13
#define VOLTE_CALL_PCM_DEVICE 15
#define QCHAT_CALL_PCM_DEVICE 37
#define VOWLAN_CALL_PCM_DEVICE 16

#define AFE_PROXY_PLAYBACK_PCM_DEVICE 7
#define AFE_PROXY_RECORD_PCM_DEVICE 8

#define PLATFORM_MAX_MIC_COUNT "input_mic_max_count"
#define PLATFORM_DEFAULT_MIC_COUNT 2

#define LIB_CSD_CLIENT "libcsd-client.so"
/* CSD-CLIENT related functions */
typedef int (*init_t)();
typedef int (*deinit_t)();
typedef int (*disable_device_t)();
typedef int (*enable_device_config_t)(int, int);
typedef int (*enable_device_t)(int, int, uint32_t);
typedef int (*volume_t)(uint32_t, int, uint16_t);
typedef int (*mic_mute_t)(uint32_t, int, uint16_t);
typedef int (*slow_talk_t)(uint32_t, uint8_t);
typedef int (*start_voice_t)(uint32_t);
typedef int (*stop_voice_t)(uint32_t);
typedef int (*start_playback_t)(uint32_t);
typedef int (*stop_playback_t)(uint32_t);
typedef int (*set_lch_t)(uint32_t, enum voice_lch_mode);
typedef int (*start_record_t)(uint32_t, int);
typedef int (*stop_record_t)(uint32_t);
/* CSD Client structure */
struct csd_data {
    void *csd_client;
    init_t init;
    deinit_t deinit;
    disable_device_t disable_device;
    enable_device_config_t enable_device_config;
    enable_device_t enable_device;
    volume_t volume;
    mic_mute_t mic_mute;
    slow_talk_t slow_talk;
    start_voice_t start_voice;
    stop_voice_t stop_voice;
    start_playback_t start_playback;
    stop_playback_t stop_playback;
    set_lch_t set_lch;
    start_record_t start_record;
    stop_record_t stop_record;
};

int platform_get_subsys_image_name (char *buf);

/* HDMI Passthrough defines */
enum {
    LEGACY_PCM = 0,
    PASSTHROUGH,
    PASSTHROUGH_CONVERT
};
/*
 * ID for setting mute and lateny on the device side
 * through Device PP Params mixer control.
 */
#define DEVICE_PARAM_MUTE_ID    0
#define DEVICE_PARAM_LATENCY_ID 1

#define ENUM_TO_STRING(X) #X

struct audio_device_to_audio_interface {
    audio_devices_t device;
    char device_name[100];
    char interface_name[100];
};

struct audio_backend_cfg {
    unsigned int   sample_rate;
    unsigned int   channels;
    unsigned int   bit_width;
    bool           passthrough_enabled;
    audio_format_t format;
};

#endif // QCOM_AUDIO_PLATFORM_H
