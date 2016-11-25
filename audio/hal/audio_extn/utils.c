/*
 * Copyright (c) 2014-2016, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 *
 * Copyright (C) 2014 The Android Open Source Project
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

#define LOG_TAG "audio_hw_utils"
/* #define LOG_NDEBUG 0 */

#include <errno.h>
#include <cutils/properties.h>
#include <cutils/config_utils.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <cutils/str_parms.h>
#include <cutils/log.h>
#include <cutils/misc.h>

#include "audio_hw.h"
#include "platform.h"
#include "platform_api.h"
#include "audio_extn.h"
#include "voice.h"

#ifdef AUDIO_EXTERNAL_HDMI_ENABLED
#ifdef HDMI_PASSTHROUGH_ENABLED
#include "audio_parsers.h"
#endif
#endif

#define AUDIO_OUTPUT_POLICY_VENDOR_CONFIG_FILE "/vendor/etc/audio_output_policy.conf"

#define OUTPUTS_TAG "outputs"

#define DYNAMIC_VALUE_TAG "dynamic"
#define FLAGS_TAG "flags"
#define FORMATS_TAG "formats"
#define SAMPLING_RATES_TAG "sampling_rates"
#define BIT_WIDTH_TAG "bit_width"
#define APP_TYPE_TAG "app_type"

#define STRING_TO_ENUM(string) { #string, string }
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define BASE_TABLE_SIZE 64
#define MAX_BASEINDEX_LEN 256

#ifdef AUDIO_EXTERNAL_HDMI_ENABLED
#define PROFESSIONAL        (1<<0)      /* 0 = consumer, 1 = professional */
#define NON_LPCM            (1<<1)      /* 0 = audio, 1 = non-audio */
#define SR_44100            (0<<0)      /* 44.1kHz */
#define SR_NOTID            (1<<0)      /* non indicated */
#define SR_48000            (2<<0)      /* 48kHz */
#define SR_32000            (3<<0)      /* 32kHz */
#define SR_22050            (4<<0)      /* 22.05kHz */
#define SR_24000            (6<<0)      /* 24kHz */
#define SR_88200            (8<<0)      /* 88.2kHz */
#define SR_96000            (10<<0)     /* 96kHz */
#define SR_176400           (12<<0)     /* 176.4kHz */
#define SR_192000           (14<<0)     /* 192kHz */

#endif
struct string_to_enum {
    const char *name;
    uint32_t value;
};

const struct string_to_enum s_flag_name_to_enum_table[] = {
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_DIRECT),
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_DIRECT_PCM),
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_PRIMARY),
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_FAST),
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_DEEP_BUFFER),
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD),
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_NON_BLOCKING),
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_HW_AV_SYNC),
#ifdef INCALL_MUSIC_ENABLED
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_INCALL_MUSIC),
#endif
#ifdef HDMI_PASSTHROUGH_ENABLED
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_COMPRESS_PASSTHROUGH),
#endif
};

const struct string_to_enum s_format_name_to_enum_table[] = {
    STRING_TO_ENUM(AUDIO_FORMAT_PCM_8_BIT),
    STRING_TO_ENUM(AUDIO_FORMAT_PCM_16_BIT),
    STRING_TO_ENUM(AUDIO_FORMAT_PCM_24_BIT_PACKED),
    STRING_TO_ENUM(AUDIO_FORMAT_PCM_8_24_BIT),
    STRING_TO_ENUM(AUDIO_FORMAT_PCM_32_BIT),
    STRING_TO_ENUM(AUDIO_FORMAT_MP3),
    STRING_TO_ENUM(AUDIO_FORMAT_AAC),
    STRING_TO_ENUM(AUDIO_FORMAT_VORBIS),
    STRING_TO_ENUM(AUDIO_FORMAT_AMR_NB),
    STRING_TO_ENUM(AUDIO_FORMAT_AMR_WB),
    STRING_TO_ENUM(AUDIO_FORMAT_AC3),
    STRING_TO_ENUM(AUDIO_FORMAT_E_AC3),
    STRING_TO_ENUM(AUDIO_FORMAT_DTS),
    STRING_TO_ENUM(AUDIO_FORMAT_DTS_HD),
#ifdef AUDIO_EXTN_FORMATS_ENABLED
    STRING_TO_ENUM(AUDIO_FORMAT_E_AC3_JOC),
    STRING_TO_ENUM(AUDIO_FORMAT_WMA),
    STRING_TO_ENUM(AUDIO_FORMAT_WMA_PRO),
    STRING_TO_ENUM(AUDIO_FORMAT_AAC_ADIF),
    STRING_TO_ENUM(AUDIO_FORMAT_AMR_WB_PLUS),
    STRING_TO_ENUM(AUDIO_FORMAT_EVRC),
    STRING_TO_ENUM(AUDIO_FORMAT_EVRCB),
    STRING_TO_ENUM(AUDIO_FORMAT_EVRCWB),
    STRING_TO_ENUM(AUDIO_FORMAT_QCELP),
    STRING_TO_ENUM(AUDIO_FORMAT_MP2),
    STRING_TO_ENUM(AUDIO_FORMAT_EVRCNW),
    STRING_TO_ENUM(AUDIO_FORMAT_FLAC),
    STRING_TO_ENUM(AUDIO_FORMAT_ALAC),
    STRING_TO_ENUM(AUDIO_FORMAT_APE),
    STRING_TO_ENUM(AUDIO_FORMAT_E_AC3_JOC),
    STRING_TO_ENUM(AUDIO_FORMAT_AAC_LC),
    STRING_TO_ENUM(AUDIO_FORMAT_AAC_HE_V1),
    STRING_TO_ENUM(AUDIO_FORMAT_AAC_HE_V2),
    STRING_TO_ENUM(AUDIO_FORMAT_AAC_ADTS),
    STRING_TO_ENUM(AUDIO_FORMAT_AAC_ADTS_LC),
    STRING_TO_ENUM(AUDIO_FORMAT_AAC_ADTS_HE_V1),
    STRING_TO_ENUM(AUDIO_FORMAT_AAC_ADTS_HE_V2),
#endif
};

static char bTable[BASE_TABLE_SIZE] = {
            'A','B','C','D','E','F','G','H','I','J','K','L',
            'M','N','O','P','Q','R','S','T','U','V','W','X',
            'Y','Z','a','b','c','d','e','f','g','h','i','j',
            'k','l','m','n','o','p','q','r','s','t','u','v',
            'w','x','y','z','0','1','2','3','4','5','6','7',
            '8','9','+','/'
};

static uint32_t string_to_enum(const struct string_to_enum *table, size_t size,
                               const char *name)
{
    size_t i;
    for (i = 0; i < size; i++) {
        if (strcmp(table[i].name, name) == 0) {
            ALOGV("%s found %s", __func__, table[i].name);
            return table[i].value;
        }
    }
    return 0;
}

static audio_output_flags_t parse_flag_names(char *name)
{
    uint32_t flag = 0;
    char *last_r;
    char *flag_name = strtok_r(name, "|", &last_r);
    while (flag_name != NULL) {
        if (strlen(flag_name) != 0) {
            flag |= string_to_enum(s_flag_name_to_enum_table,
                               ARRAY_SIZE(s_flag_name_to_enum_table),
                               flag_name);
        }
        flag_name = strtok_r(NULL, "|", &last_r);
    }

    ALOGV("parse_flag_names: flag - %d", flag);
    return (audio_output_flags_t)flag;
}

static void parse_format_names(char *name, struct streams_output_cfg *so_info)
{
    struct stream_format *sf_info = NULL;
    char *last_r;
    char *str = strtok_r(name, "|", &last_r);

    if (str != NULL && strcmp(str, DYNAMIC_VALUE_TAG) == 0)
        return;

    list_init(&so_info->format_list);
    while (str != NULL) {
        audio_format_t format = (audio_format_t)string_to_enum(s_format_name_to_enum_table,
                                              ARRAY_SIZE(s_format_name_to_enum_table), str);
        ALOGV("%s: format - %d", __func__, format);
        if (format != 0) {
            sf_info = (struct stream_format *)calloc(1, sizeof(struct stream_format));
            if (sf_info == NULL)
                break; /* return whatever was parsed */

            sf_info->format = format;
            list_add_tail(&so_info->format_list, &sf_info->list);
        }
        str = strtok_r(NULL, "|", &last_r);
    }
}

static void parse_sample_rate_names(char *name, struct streams_output_cfg *so_info)
{
    struct stream_sample_rate *ss_info = NULL;
    uint32_t sample_rate = 48000;
    char *last_r;
    char *str = strtok_r(name, "|", &last_r);

    if (str != NULL && 0 == strcmp(str, DYNAMIC_VALUE_TAG))
        return;

    list_init(&so_info->sample_rate_list);
    while (str != NULL) {
        sample_rate = (uint32_t)strtol(str, (char **)NULL, 10);
        ALOGV("%s: sample_rate - %d", __func__, sample_rate);
        if (0 != sample_rate) {
            ss_info = (struct stream_sample_rate *)calloc(1, sizeof(struct stream_sample_rate));
            if (!ss_info) {
                ALOGE("%s: memory allocation failure", __func__);
                return;
            }
            ss_info->sample_rate = sample_rate;
            list_add_tail(&so_info->sample_rate_list, &ss_info->list);
        }
        str = strtok_r(NULL, "|", &last_r);
    }
}

static int parse_bit_width_names(char *name)
{
    int bit_width = 16;
    char *last_r;
    char *str = strtok_r(name, "|", &last_r);

    if (str != NULL && strcmp(str, DYNAMIC_VALUE_TAG))
        bit_width = (int)strtol(str, (char **)NULL, 10);

    ALOGV("%s: bit_width - %d", __func__, bit_width);
    return bit_width;
}

static int parse_app_type_names(void *platform, char *name)
{
    int app_type = platform_get_default_app_type(platform);
    char *last_r;
    char *str = strtok_r(name, "|", &last_r);

    if (str != NULL && strcmp(str, DYNAMIC_VALUE_TAG))
        app_type = (int)strtol(str, (char **)NULL, 10);

    ALOGV("%s: app_type - %d", __func__, app_type);
    return app_type;
}

static void update_streams_output_cfg_list(cnode *root, void *platform,
                                           struct listnode *streams_output_cfg_list)
{
    cnode *node = root->first_child;
    struct streams_output_cfg *so_info;

    ALOGV("%s", __func__);
    so_info = (struct streams_output_cfg *)calloc(1, sizeof(struct streams_output_cfg));

    if (!so_info) {
        ALOGE("failed to allocate mem for so_info list element");
        return;
    }

    while (node) {
        if (strcmp(node->name, FLAGS_TAG) == 0) {
            so_info->flags = parse_flag_names((char *)node->value);
        } else if (strcmp(node->name, FORMATS_TAG) == 0) {
            parse_format_names((char *)node->value, so_info);
        } else if (strcmp(node->name, SAMPLING_RATES_TAG) == 0) {
            so_info->app_type_cfg.sample_rate = CODEC_BACKEND_DEFAULT_SAMPLE_RATE;
            parse_sample_rate_names((char *)node->value, so_info);
        } else if (strcmp(node->name, BIT_WIDTH_TAG) == 0) {
            so_info->app_type_cfg.bit_width = parse_bit_width_names((char *)node->value);
        } else if (strcmp(node->name, APP_TYPE_TAG) == 0) {
            so_info->app_type_cfg.app_type = parse_app_type_names(platform, (char *)node->value);
        }
        node = node->next;
    }
    list_add_tail(streams_output_cfg_list, &so_info->list);
}

static void load_output(cnode *root, void *platform,
                        struct listnode *streams_output_cfg_list)
{
    cnode *node = config_find(root, OUTPUTS_TAG);
    if (node == NULL) {
        ALOGE("%s: could not load output, node is NULL", __func__);
        return;
    }

    node = node->first_child;
    while (node) {
        ALOGV("%s: loading output %s", __func__, node->name);
        update_streams_output_cfg_list(node, platform, streams_output_cfg_list);
        node = node->next;
    }
}

static void send_app_type_cfg(void *platform, struct mixer *mixer,
                              struct listnode *streams_output_cfg_list)
{
    int app_type_cfg[MAX_LENGTH_MIXER_CONTROL_IN_INT] = {-1};
    int length = 0, i, num_app_types = 0;
    struct listnode *node;
    bool update;
    struct mixer_ctl *ctl = NULL;
    const char *mixer_ctl_name = "App Type Config";
    struct streams_output_cfg *so_info;

    if (!mixer) {
        ALOGE("%s: mixer is null",__func__);
        return;
    }
    ctl = mixer_get_ctl_by_name(mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",__func__, mixer_ctl_name);
        return;
    }
    if (streams_output_cfg_list == NULL) {
        app_type_cfg[length++] = 1;
        app_type_cfg[length++] = platform_get_default_app_type(platform);
        app_type_cfg[length++] = 48000;
        app_type_cfg[length++] = 16;
        mixer_ctl_set_array(ctl, app_type_cfg, length);
        return;
    }

    app_type_cfg[length++] = num_app_types;
    list_for_each(node, streams_output_cfg_list) {
        so_info = node_to_item(node, struct streams_output_cfg, list);
        update = true;
        for (i=0; i<length; i=i+3) {
            if (app_type_cfg[i+1] == -1)
                break;
            else if (app_type_cfg[i+1] == so_info->app_type_cfg.app_type) {
                update = false;
                break;
            }
        }
        if (update && ((length + 3) <= MAX_LENGTH_MIXER_CONTROL_IN_INT)) {
            num_app_types += 1 ;
            app_type_cfg[length++] = so_info->app_type_cfg.app_type;
            app_type_cfg[length++] = so_info->app_type_cfg.sample_rate;
            app_type_cfg[length++] = so_info->app_type_cfg.bit_width;
        }
    }
    ALOGV("%s: num_app_types: %d", __func__, num_app_types);
    if (num_app_types) {
        app_type_cfg[0] = num_app_types;
        mixer_ctl_set_array(ctl, app_type_cfg, length);
    }
}

void audio_extn_utils_update_streams_output_cfg_list(void *platform,
                                       struct mixer *mixer,
                                       struct listnode *streams_output_cfg_list)
{
    cnode *root;
    char *data;

    ALOGV("%s", __func__);
    list_init(streams_output_cfg_list);
    data = (char *)load_file(AUDIO_OUTPUT_POLICY_VENDOR_CONFIG_FILE, NULL);
    if (data == NULL) {
        send_app_type_cfg(platform, mixer, NULL);
        ALOGE("%s: could not load output policy config file", __func__);
        return;
    }

    root = config_node("", "");
    if (root == NULL) {
        ALOGE("cfg_list, NULL config root");
        free(data);
        return;
    }

    config_load(root, data);
    load_output(root, platform, streams_output_cfg_list);

    send_app_type_cfg(platform, mixer, streams_output_cfg_list);

    config_free(root);
    free(data);
}

void audio_extn_utils_dump_streams_output_cfg_list(
                                       struct listnode *streams_output_cfg_list)
{
    struct listnode *node_i, *node_j;
    struct streams_output_cfg *so_info;
    struct stream_format *sf_info;
    struct stream_sample_rate *ss_info;
    ALOGV("%s", __func__);
    list_for_each(node_i, streams_output_cfg_list) {
        so_info = node_to_item(node_i, struct streams_output_cfg, list);
        ALOGV("%s: flags-%d, output_sample_rate-%d, output_bit_width-%d, app_type-%d",
               __func__, so_info->flags, so_info->app_type_cfg.sample_rate,
               so_info->app_type_cfg.bit_width, so_info->app_type_cfg.app_type);
        list_for_each(node_j, &so_info->format_list) {
            sf_info = node_to_item(node_j, struct stream_format, list);
            ALOGV("format-%x", sf_info->format);
        }
        list_for_each(node_j, &so_info->sample_rate_list) {
            ss_info = node_to_item(node_j, struct stream_sample_rate, list);
            ALOGV("sample rate-%d", ss_info->sample_rate);
        }
    }
}

void audio_extn_utils_release_streams_output_cfg_list(
                                       struct listnode *streams_output_cfg_list)
{
    struct listnode *node_i, *node_j;
    struct streams_output_cfg *so_info;

    ALOGV("%s", __func__);
    while (!list_empty(streams_output_cfg_list)) {
        node_i = list_head(streams_output_cfg_list);
        so_info = node_to_item(node_i, struct streams_output_cfg, list);
        while (!list_empty(&so_info->format_list)) {
            node_j = list_head(&so_info->format_list);
            list_remove(node_j);
            free(node_to_item(node_j, struct stream_format, list));
        }
        while (!list_empty(&so_info->sample_rate_list)) {
            node_j = list_head(&so_info->sample_rate_list);
            list_remove(node_j);
            free(node_to_item(node_j, struct stream_sample_rate, list));
        }
        list_remove(node_i);
        free(node_to_item(node_i, struct streams_output_cfg, list));
    }
}

static bool set_output_cfg(struct streams_output_cfg *so_info,
                           struct stream_app_type_cfg *app_type_cfg,
                           uint32_t sample_rate, uint32_t bit_width)
 {
    struct listnode *node_i;
    struct stream_sample_rate *ss_info;
    list_for_each(node_i, &so_info->sample_rate_list) {
        ss_info = node_to_item(node_i, struct stream_sample_rate, list);
        if ((sample_rate <= ss_info->sample_rate) &&
            (bit_width == so_info->app_type_cfg.bit_width)) {

            app_type_cfg->app_type = so_info->app_type_cfg.app_type;
            app_type_cfg->sample_rate = ss_info->sample_rate;
            app_type_cfg->bit_width = so_info->app_type_cfg.bit_width;
            ALOGV("%s app_type_cfg->app_type %d, app_type_cfg->sample_rate %d, app_type_cfg->bit_width %d",
                   __func__, app_type_cfg->app_type, app_type_cfg->sample_rate, app_type_cfg->bit_width);
            return true;
        }
    }
    /*
     * Reiterate through the list assuming dafault sample rate.
     * Handles scenario where input sample rate is higher
     * than all sample rates in list for the input bit width.
     */
    sample_rate = CODEC_BACKEND_DEFAULT_SAMPLE_RATE;

    list_for_each(node_i, &so_info->sample_rate_list) {
        ss_info = node_to_item(node_i, struct stream_sample_rate, list);
        if ((sample_rate <= ss_info->sample_rate) &&
            (bit_width == so_info->app_type_cfg.bit_width)) {
            app_type_cfg->app_type = so_info->app_type_cfg.app_type;
            app_type_cfg->sample_rate = sample_rate;
            app_type_cfg->bit_width = so_info->app_type_cfg.bit_width;
            ALOGV("%s Assuming sample rate. app_type_cfg->app_type %d, app_type_cfg->sample_rate %d, app_type_cfg->bit_width %d",
                   __func__, app_type_cfg->app_type, app_type_cfg->sample_rate, app_type_cfg->bit_width);
            return true;
        }
    }
    return false;
}

void audio_extn_utils_update_stream_app_type_cfg(void *platform,
                                  struct listnode *streams_output_cfg_list,
                                  audio_devices_t devices,
                                  audio_output_flags_t flags,
                                  audio_format_t format,
                                  uint32_t sample_rate,
                                  uint32_t bit_width,
                                  audio_channel_mask_t channel_mask,
                                  struct stream_app_type_cfg *app_type_cfg)
{
    struct listnode *node_i, *node_j;
    struct streams_output_cfg *so_info;
    struct stream_format *sf_info;
    char value[PROPERTY_VALUE_MAX] = {0};

    if ((24 == bit_width) &&
        (devices & AUDIO_DEVICE_OUT_SPEAKER)) {
        int32_t bw = platform_get_snd_device_bit_width(SND_DEVICE_OUT_SPEAKER);
        if (-ENOSYS != bw)
            bit_width = (uint32_t)bw;
        sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
        ALOGI("%s Allowing 24-bit playback on speaker ONLY at default sampling rate", __func__);
    }

    property_get("audio.playback.mch.downsample",value,"");
    if (!strncmp("true", value, sizeof("true"))) {
        if ((popcount(channel_mask) > 2) &&
                (sample_rate > CODEC_BACKEND_DEFAULT_SAMPLE_RATE) &&
                !(flags & AUDIO_OUTPUT_FLAG_COMPRESS_PASSTHROUGH))  {
                    sample_rate = CODEC_BACKEND_DEFAULT_SAMPLE_RATE;
                    ALOGD("%s: MCH session defaulting sample rate to %d",
                               __func__, sample_rate);
        }
    }
    ALOGV("%s: flags: %x, format: %x sample_rate %d",
           __func__, flags, format, sample_rate);
    list_for_each(node_i, streams_output_cfg_list) {
        so_info = node_to_item(node_i, struct streams_output_cfg, list);
        if (so_info->flags == flags) {
            list_for_each(node_j, &so_info->format_list) {
                sf_info = node_to_item(node_j, struct stream_format, list);
                if (sf_info->format == format) {
                    if (set_output_cfg(so_info, app_type_cfg, sample_rate, bit_width))
                        return;
                }
            }
        }
    }
    list_for_each(node_i, streams_output_cfg_list) {
        so_info = node_to_item(node_i, struct streams_output_cfg, list);
        if (so_info->flags == AUDIO_OUTPUT_FLAG_PRIMARY) {
            ALOGV("Compatible output profile not found.");
            app_type_cfg->app_type = so_info->app_type_cfg.app_type;
            app_type_cfg->sample_rate = so_info->app_type_cfg.sample_rate;
            app_type_cfg->bit_width = so_info->app_type_cfg.bit_width;
            ALOGV("%s Default to primary output: App type: %d sample_rate %d",
                  __func__, so_info->app_type_cfg.app_type, app_type_cfg->sample_rate);
            return;
        }
    }
    ALOGW("%s: App type could not be selected. Falling back to default", __func__);
    app_type_cfg->app_type = platform_get_default_app_type(platform);
    app_type_cfg->sample_rate = CODEC_BACKEND_DEFAULT_SAMPLE_RATE;
    app_type_cfg->bit_width = 16;
}

static bool audio_is_this_native_usecase(struct audio_usecase *uc)
{
    bool native_usecase = false;
    struct stream_out *out = (struct stream_out*) uc->stream.out;

    if (PCM_PLAYBACK == uc->type && out != NULL &&
        NATIVE_AUDIO_MODE_INVALID != platform_get_native_support() &&
        is_offload_usecase(uc->id) &&
        (out->sample_rate == OUTPUT_SAMPLING_RATE_44100))
        native_usecase = true;

    return native_usecase;
}

int audio_extn_utils_send_app_type_cfg(struct audio_device *adev,
                                       struct audio_usecase *usecase)
{
    char mixer_ctl_name[MAX_LENGTH_MIXER_CONTROL_IN_INT];
    int app_type_cfg[MAX_LENGTH_MIXER_CONTROL_IN_INT], len = 0, rc;
    struct mixer_ctl *ctl;
    int pcm_device_id = 0, acdb_dev_id, snd_device = usecase->out_snd_device;
    int32_t sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
    char value[PROPERTY_VALUE_MAX] = {0};

    ALOGV("%s", __func__);

    if (usecase->type != PCM_PLAYBACK && usecase->type != PCM_CAPTURE) {
        ALOGE("%s: not a playback or capture path, no need to cfg app type", __func__);
        rc = 0;
        goto exit_send_app_type_cfg;
    }
    if ((usecase->id != USECASE_AUDIO_PLAYBACK_DEEP_BUFFER) &&
        (usecase->id != USECASE_AUDIO_PLAYBACK_LOW_LATENCY) &&
        (usecase->id != USECASE_AUDIO_PLAYBACK_MULTI_CH) &&
        (!is_offload_usecase(usecase->id)) &&
        (usecase->type != PCM_CAPTURE)) {
        ALOGV("%s: a rx/tx path where app type cfg is not required %d", __func__, usecase->id);
        rc = 0;
        goto exit_send_app_type_cfg;
    }
    if (usecase->type == PCM_PLAYBACK) {
        snd_device = usecase->out_snd_device;
        pcm_device_id = platform_get_pcm_device_id(usecase->id, PCM_PLAYBACK);
    } else if (usecase->type == PCM_CAPTURE) {
        snd_device = usecase->in_snd_device;
        pcm_device_id = platform_get_pcm_device_id(usecase->id, PCM_CAPTURE);
    }

    snprintf(mixer_ctl_name, sizeof(mixer_ctl_name),
             "Audio Stream %d App Type Cfg", pcm_device_id);

    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s", __func__,
              mixer_ctl_name);
        rc = -EINVAL;
        goto exit_send_app_type_cfg;
    }
    snd_device = (snd_device == SND_DEVICE_OUT_SPEAKER) ?
                 platform_get_spkr_prot_snd_device(snd_device) : snd_device;
    acdb_dev_id = platform_get_snd_device_acdb_id(snd_device);
    if (acdb_dev_id < 0) {
        ALOGE("%s: Couldn't get the acdb dev id", __func__);
        rc = -EINVAL;
        goto exit_send_app_type_cfg;
    }
    if ((usecase->type == PCM_PLAYBACK) && (usecase->stream.out == NULL)) {
         sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
         app_type_cfg[len++] = platform_get_default_app_type(adev->platform);
         app_type_cfg[len++] = acdb_dev_id;
         app_type_cfg[len++] = sample_rate;
         ALOGI("%s:%d PLAYBACK app_type %d, acdb_dev_id %d, sample_rate %d",
               __func__, __LINE__,
               platform_get_default_app_type(adev->platform),
               acdb_dev_id, sample_rate);
    } else if (usecase->type == PCM_PLAYBACK) {

         if (usecase->stream.out->devices & AUDIO_DEVICE_OUT_SPEAKER) {
             usecase->stream.out->app_type_cfg.sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
         } else if ((usecase->stream.out->app_type_cfg.sample_rate == OUTPUT_SAMPLING_RATE_44100 &&
                      !(audio_is_this_native_usecase(usecase))) ||
                      (usecase->stream.out->sample_rate < OUTPUT_SAMPLING_RATE_44100)) {
                    usecase->stream.out->app_type_cfg.sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
         }

         sample_rate = usecase->stream.out->app_type_cfg.sample_rate;

         property_get("audio.playback.mch.downsample",value,"");
         if (!strncmp("true", value, sizeof("true"))) {
             if ((popcount(usecase->stream.out->channel_mask) > 2) &&
                    (usecase->stream.out->app_type_cfg.sample_rate > CODEC_BACKEND_DEFAULT_SAMPLE_RATE) &&
                    !(usecase->stream.out->flags & AUDIO_OUTPUT_FLAG_COMPRESS_PASSTHROUGH))
                sample_rate = CODEC_BACKEND_DEFAULT_SAMPLE_RATE;
         }

        if ((24 == usecase->stream.out->bit_width) &&
            (usecase->stream.out->devices & AUDIO_DEVICE_OUT_SPEAKER)) {
            usecase->stream.out->app_type_cfg.sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
        } else if ((snd_device != SND_DEVICE_OUT_HEADPHONES_44_1 &&
            usecase->stream.out->sample_rate == OUTPUT_SAMPLING_RATE_44100) ||
            (usecase->stream.out->sample_rate < OUTPUT_SAMPLING_RATE_44100)) {
            usecase->stream.out->app_type_cfg.sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
        }
        sample_rate = usecase->stream.out->app_type_cfg.sample_rate;

        app_type_cfg[len++] = usecase->stream.out->app_type_cfg.app_type;
        app_type_cfg[len++] = acdb_dev_id;
        if (((usecase->stream.out->format == AUDIO_FORMAT_E_AC3) ||
            (usecase->stream.out->format == AUDIO_FORMAT_E_AC3_JOC))
            && audio_extn_passthru_is_passthrough_stream(usecase->stream.out)) {
            app_type_cfg[len++] = sample_rate * 4;
        } else {
            app_type_cfg[len++] = sample_rate;
        }
        ALOGI("%s PLAYBACK app_type %d, acdb_dev_id %d, sample_rate %d",
              __func__, usecase->stream.out->app_type_cfg.app_type, acdb_dev_id, sample_rate);
    } else if (usecase->type == PCM_CAPTURE) {
         app_type_cfg[len++] = platform_get_default_app_type_v2(adev->platform, usecase->type);
         app_type_cfg[len++] = acdb_dev_id;
         app_type_cfg[len++] = sample_rate;
         ALOGI("%s CAPTURE app_type %d, acdb_dev_id %d, sample_rate %d",
           __func__, platform_get_default_app_type_v2(adev->platform, usecase->type),
           acdb_dev_id, sample_rate);
    }

    mixer_ctl_set_array(ctl, app_type_cfg, len);
    rc = 0;
    ALOGI("%s:becf: adm: app_type %d, acdb_dev_id %d, sample_rate %d",
          __func__,
          platform_get_default_app_type_v2(adev->platform, usecase->type),
          acdb_dev_id, sample_rate);
exit_send_app_type_cfg:
    return rc;
}

int read_line_from_file(const char *path, char *buf, size_t count)
{
    char * fgets_ret;
    FILE * fd;
    int rv;

    fd = fopen(path, "r");
    if (fd == NULL)
        return -1;

    fgets_ret = fgets(buf, (int)count, fd);
    if (NULL != fgets_ret) {
        rv = (int)strlen(buf);
    } else {
        rv = ferror(fd);
    }
    fclose(fd);

   return rv;
}

/*Translates ALSA formats to AOSP PCM formats*/
audio_format_t alsa_format_to_hal(uint32_t alsa_format)
{
    audio_format_t format;

    switch(alsa_format) {
    case SNDRV_PCM_FORMAT_S16_LE:
        format = AUDIO_FORMAT_PCM_16_BIT;
        break;
    case SNDRV_PCM_FORMAT_S24_3LE:
        format = AUDIO_FORMAT_PCM_24_BIT_PACKED;
        break;
    case SNDRV_PCM_FORMAT_S24_LE:
        format = AUDIO_FORMAT_PCM_8_24_BIT;
        break;
    case SNDRV_PCM_FORMAT_S32_LE:
        format = AUDIO_FORMAT_PCM_32_BIT;
        break;
    default:
        ALOGW("Incorrect ALSA format");
        format = AUDIO_FORMAT_INVALID;
    }
    return format;
}

/*Translates hal format (AOSP) to alsa formats*/
uint32_t hal_format_to_alsa(audio_format_t hal_format)
{
    uint32_t alsa_format;

    switch (hal_format) {
    case AUDIO_FORMAT_PCM_32_BIT: {
        if (platform_supports_true_32bit())
            alsa_format = SNDRV_PCM_FORMAT_S32_LE;
        else
            alsa_format = SNDRV_PCM_FORMAT_S24_3LE;
        }
        break;
    case AUDIO_FORMAT_PCM_8_BIT:
        alsa_format = SNDRV_PCM_FORMAT_S8;
        break;
    case AUDIO_FORMAT_PCM_24_BIT_PACKED:
        alsa_format = SNDRV_PCM_FORMAT_S24_3LE;
        break;
    case AUDIO_FORMAT_PCM_8_24_BIT: {
        if (platform_supports_true_32bit())
            alsa_format = SNDRV_PCM_FORMAT_S32_LE;
        else
            alsa_format = SNDRV_PCM_FORMAT_S24_3LE;
        }
        break;
    case AUDIO_FORMAT_PCM_FLOAT:
        alsa_format = SNDRV_PCM_FORMAT_S24_3LE;
        break;
    default:
    case AUDIO_FORMAT_PCM_16_BIT:
        alsa_format = SNDRV_PCM_FORMAT_S16_LE;
        break;
    }
    return alsa_format;
}

/*Translates PCM formats to AOSP formats*/
audio_format_t pcm_format_to_hal(uint32_t pcm_format)
{
    audio_format_t format = AUDIO_FORMAT_INVALID;

    switch(pcm_format) {
    case PCM_FORMAT_S16_LE:
        format = AUDIO_FORMAT_PCM_16_BIT;
        break;
    case PCM_FORMAT_S24_3LE:
        format = AUDIO_FORMAT_PCM_24_BIT_PACKED;
        break;
    case PCM_FORMAT_S24_LE:
        format = AUDIO_FORMAT_PCM_8_24_BIT;
        break;
    case PCM_FORMAT_S32_LE:
        format = AUDIO_FORMAT_PCM_32_BIT;
        break;
    default:
        ALOGW("Incorrect PCM format");
        format = AUDIO_FORMAT_INVALID;
    }
    return format;
}

/*Translates hal format (AOSP) to alsa formats*/
uint32_t hal_format_to_pcm(audio_format_t hal_format)
{
    uint32_t pcm_format;

    switch (hal_format) {
    case AUDIO_FORMAT_PCM_32_BIT:
    case AUDIO_FORMAT_PCM_8_24_BIT:
    case AUDIO_FORMAT_PCM_FLOAT: {
        if (platform_supports_true_32bit())
            pcm_format = PCM_FORMAT_S32_LE;
        else
            pcm_format = PCM_FORMAT_S24_3LE;
        }
        break;
    case AUDIO_FORMAT_PCM_8_BIT:
        pcm_format = PCM_FORMAT_S8;
        break;
    case AUDIO_FORMAT_PCM_24_BIT_PACKED:
        pcm_format = PCM_FORMAT_S24_3LE;
        break;
    default:
    case AUDIO_FORMAT_PCM_16_BIT:
        pcm_format = PCM_FORMAT_S16_LE;
        break;
    }
    return pcm_format;
}

uint32_t get_alsa_fragment_size(uint32_t bytes_per_sample,
                                  uint32_t sample_rate,
                                  uint32_t noOfChannels)
{
    uint32_t fragment_size = 0;
    uint32_t pcm_offload_time = PCM_OFFLOAD_BUFFER_DURATION;

    fragment_size = (pcm_offload_time
                     * sample_rate
                     * bytes_per_sample
                     * noOfChannels)/1000;
    if (fragment_size < MIN_PCM_OFFLOAD_FRAGMENT_SIZE)
        fragment_size = MIN_PCM_OFFLOAD_FRAGMENT_SIZE;
    else if (fragment_size > MAX_PCM_OFFLOAD_FRAGMENT_SIZE)
        fragment_size = MAX_PCM_OFFLOAD_FRAGMENT_SIZE;
    /*To have same PCM samples for all channels, the buffer size requires to
     *be multiple of (number of channels * bytes per sample)
     *For writes to succeed, the buffer must be written at address which is multiple of 32
     */
    fragment_size = ALIGN(fragment_size, (bytes_per_sample * noOfChannels * 32));

    ALOGI("PCM offload Fragment size to %d bytes", fragment_size);
    return fragment_size;
}

/* Calculates the fragment size required to configure compress session.
 * Based on the alsa format selected, decide if conversion is needed in

 * HAL ( e.g. convert AUDIO_FORMAT_PCM_FLOAT input format to
 * AUDIO_FORMAT_PCM_24_BIT_PACKED before writing to the compress driver.
 */
void audio_extn_utils_update_direct_pcm_fragment_size(struct stream_out *out)
{
    audio_format_t dst_format = out->hal_op_format;
    audio_format_t src_format = out->hal_ip_format;
    uint32_t hal_op_bytes_per_sample = audio_bytes_per_sample(dst_format);
    uint32_t hal_ip_bytes_per_sample = audio_bytes_per_sample(src_format);

    out->compr_config.fragment_size =
             get_alsa_fragment_size(hal_op_bytes_per_sample,
                                      out->sample_rate,
                                      popcount(out->channel_mask));

    if ((src_format != dst_format) &&
         hal_op_bytes_per_sample != hal_ip_bytes_per_sample) {

        out->hal_fragment_size =
                  ((out->compr_config.fragment_size * hal_ip_bytes_per_sample) /
                   hal_op_bytes_per_sample);
        ALOGI("enable conversion hal_input_fragment_size is %d src_format %x dst_format %x",
               out->hal_fragment_size, src_format, dst_format);
    } else {
        out->hal_fragment_size = out->compr_config.fragment_size;
    }
}

void audio_extn_utils_send_audio_calibration(struct audio_device *adev,
                                             struct audio_usecase *usecase)
{
    int type = usecase->type;

    if (type == PCM_PLAYBACK) {
        struct stream_out *out = usecase->stream.out;
        int snd_device = usecase->out_snd_device;
        snd_device = (snd_device == SND_DEVICE_OUT_SPEAKER) ?
                     platform_get_spkr_prot_snd_device(snd_device) : snd_device;
        platform_send_audio_calibration(adev->platform, usecase,
                                        out->app_type_cfg.app_type,
                                        usecase->stream.out->app_type_cfg.sample_rate);
    }
    if ((type == PCM_HFP_CALL) || (type == PCM_CAPTURE)) {
        /* when app type is default. the sample rate is not used to send cal */
        platform_send_audio_calibration(adev->platform, usecase,
                 platform_get_default_app_type_v2(adev->platform, usecase->type),
                 48000);
    }
}

// Base64 Encode and Decode
// Not all features supported. This must be used only with following conditions.
// Decode Modes: Support with and without padding
//         CRLF not handling. So no CRLF in string to decode.
// Encode Modes: Supports only padding
int b64decode(char *inp, int ilen, uint8_t* outp)
{
    int i, j, k, ii, num;
    int rem, pcnt;
    uint32_t res=0;
    uint8_t getIndex[MAX_BASEINDEX_LEN];
    uint8_t tmp, cflag;

    if(inp == NULL || outp == NULL || ilen <= 0) {
        ALOGE("[%s] received NULL pointer or zero length",__func__);
        return -1;
    }

    memset(getIndex, MAX_BASEINDEX_LEN-1, sizeof(getIndex));
    for(i=0;i<BASE_TABLE_SIZE;i++) {
        getIndex[(uint8_t)bTable[i]] = (uint8_t)i;
    }
    getIndex[(uint8_t)'=']=0;

    j=0;k=0;
    num = ilen/4;
    rem = ilen%4;
    if(rem==0)
        num = num-1;
    cflag=0;
    for(i=0; i<num; i++) {
        res=0;
        for(ii=0;ii<4;ii++) {
            res = res << 6;
            tmp = getIndex[(uint8_t)inp[j++]];
            res = res | tmp;
            cflag = cflag | tmp;
        }
        outp[k++] = (res >> 16)&0xFF;
        outp[k++] = (res >> 8)&0xFF;
        outp[k++] = res & 0xFF;
    }

    // Handle last bytes special
    pcnt=0;
    if(rem == 0) {
        //With padding or full data
        res = 0;
        for(ii=0;ii<4;ii++) {
            if(inp[j] == '=')
                pcnt++;
            res = res << 6;
            tmp = getIndex[(uint8_t)inp[j++]];
            res = res | tmp;
            cflag = cflag | tmp;
        }
        outp[k++] = res >> 16;
        if(pcnt == 2)
            goto done;
        outp[k++] = (res>>8)&0xFF;
        if(pcnt == 1)
            goto done;
        outp[k++] = res&0xFF;
    } else {
        //without padding
        res = 0;
        for(i=0;i<rem;i++) {
            res = res << 6;
            tmp = getIndex[(uint8_t)inp[j++]];
            res = res | tmp;
            cflag = cflag | tmp;
        }
        for(i=rem;i<4;i++) {
            res = res << 6;
            pcnt++;
        }
        outp[k++] = res >> 16;
        if(pcnt == 2)
            goto done;
        outp[k++] = (res>>8)&0xFF;
        if(pcnt == 1)
            goto done;
        outp[k++] = res&0xFF;
    }
done:
    if(cflag == 0xFF) {
        ALOGE("[%s] base64 decode failed. Invalid character found %s",
            __func__, inp);
        return 0;
    }
    return k;
}

int b64encode(uint8_t *inp, int ilen, char* outp)
{
    int i,j,k, num;
    int rem=0;
    uint32_t res=0;

    if(inp == NULL || outp == NULL || ilen<=0) {
        ALOGE("[%s] received NULL pointer or zero input length",__func__);
        return -1;
    }

    num = ilen/3;
    rem = ilen%3;
    j=0;k=0;
    for(i=0; i<num; i++) {
        //prepare index
        res = inp[j++]<<16;
        res = res | inp[j++]<<8;
        res = res | inp[j++];
        //get output map from index
        outp[k++] = (char) bTable[(res>>18)&0x3F];
        outp[k++] = (char) bTable[(res>>12)&0x3F];
        outp[k++] = (char) bTable[(res>>6)&0x3F];
        outp[k++] = (char) bTable[res&0x3F];
    }

    switch(rem) {
        case 1:
            res = inp[j++]<<16;
            outp[k++] = (char) bTable[res>>18];
            outp[k++] = (char) bTable[(res>>12)&0x3F];
            //outp[k++] = '=';
            //outp[k++] = '=';
            break;
        case 2:
            res = inp[j++]<<16;
            res = res | inp[j++]<<8;
            outp[k++] = (char) bTable[res>>18];
            outp[k++] = (char) bTable[(res>>12)&0x3F];
            outp[k++] = (char) bTable[(res>>6)&0x3F];
            //outp[k++] = '=';
            break;
        default:
            break;
    }
    outp[k] = '\0';
    return k;
}


int audio_extn_utils_get_codec_version(const char *snd_card_name,
                            int card_num,
                            char *codec_version)
{
    char procfs_path[50];
    FILE *fp;

    if (strstr(snd_card_name, "tasha")) {
        snprintf(procfs_path, sizeof(procfs_path),
                 "/proc/asound/card%d/codecs/tasha/version", card_num);
        if ((fp = fopen(procfs_path, "r")) != NULL) {
            fgets(codec_version, CODEC_VERSION_MAX_LENGTH, fp);
            fclose(fp);
        } else {
            ALOGE("%s: ERROR. cannot open %s", __func__, procfs_path);
            return -ENOENT;
        }
        ALOGD("%s: codec version %s", __func__, codec_version);
    }

    return 0;
}


#ifdef AUDIO_EXTERNAL_HDMI_ENABLED

void get_default_compressed_channel_status(
                                  unsigned char *channel_status)
{
     memset(channel_status,0,24);

     /* block start bit in preamble bit 3 */
     channel_status[0] |= PROFESSIONAL;
     //compre out
     channel_status[0] |= NON_LPCM;
     // sample rate; fixed 48K for default/transcode
     channel_status[3] |= SR_48000;
}

#ifdef HDMI_PASSTHROUGH_ENABLED
int32_t get_compressed_channel_status(void *audio_stream_data,
                                                   uint32_t audio_frame_size,
                                                   unsigned char *channel_status,
                                                   enum audio_parser_code_type codec_type)
                                                   // codec_type - AUDIO_PARSER_CODEC_AC3
                                                   //            - AUDIO_PARSER_CODEC_DTS
{
     unsigned char *stream;
     int ret = 0;
     stream = (unsigned char *)audio_stream_data;

     if (audio_stream_data == NULL || audio_frame_size == 0) {
         ALOGW("no buffer to get channel status, return default for compress");
         get_default_compressed_channel_status(channel_status);
         return ret;
     }

     memset(channel_status,0,24);
     if(init_audio_parser(stream, audio_frame_size, codec_type) == -1)
     {
         ALOGE("init audio parser failed");
         return -1;
     }
     ret = get_channel_status(channel_status, codec_type);
     return ret;

}

#endif

void get_lpcm_channel_status(uint32_t sampleRate,
                                                  unsigned char *channel_status)
{
     int32_t status = 0;
     memset(channel_status,0,24);
     /* block start bit in preamble bit 3 */
     channel_status[0] |= PROFESSIONAL;
     //LPCM OUT
     channel_status[0] &= ~NON_LPCM;

     switch (sampleRate) {
         case 8000:
         case 11025:
         case 12000:
         case 16000:
         case 22050:
             channel_status[3] |= SR_NOTID;
             break;
         case 24000:
             channel_status[3] |= SR_24000;
             break;
         case 32000:
             channel_status[3] |= SR_32000;
             break;
         case 44100:
             channel_status[3] |= SR_44100;
             break;
         case 48000:
             channel_status[3] |= SR_48000;
             break;
         case 88200:
             channel_status[3] |= SR_88200;
             break;
         case 96000:
             channel_status[3] |= SR_96000;
             break;
         case 176400:
             channel_status[3] |= SR_176400;
             break;
         case 192000:
            channel_status[3] |= SR_192000;
             break;
         default:
             ALOGV("Invalid sample_rate %u\n", sampleRate);
             status = -1;
             break;
     }
}

void audio_utils_set_hdmi_channel_status(struct stream_out *out, char * buffer, size_t bytes)
{
    unsigned char channel_status[24]={0};
    struct snd_aes_iec958 iec958;
    const char *mixer_ctl_name = "IEC958 Playback PCM Stream";
    struct mixer_ctl *ctl;
    ALOGV("%s: buffer %s bytes %zd", __func__, buffer, bytes);
#ifdef HDMI_PASSTHROUGH_ENABLED
    if (audio_extn_is_dolby_format(out->format) &&
        /*TODO:Extend code to support DTS passthrough*/
        /*set compressed channel status bits*/
        audio_extn_passthru_is_passthrough_stream(out)){
        get_compressed_channel_status(buffer, bytes, channel_status, AUDIO_PARSER_CODEC_AC3);
    } else
#endif
    {
        /*set channel status bit for LPCM*/
        get_lpcm_channel_status(out->sample_rate, channel_status);
    }

    memcpy(iec958.status, channel_status,sizeof(iec958.status));
    ctl = mixer_get_ctl_by_name(out->dev->mixer, mixer_ctl_name);
    if (!ctl) {
            ALOGE("%s: Could not get ctl for mixer cmd - %s",
                  __func__, mixer_ctl_name);
            return;
    }
    if (mixer_ctl_set_array(ctl, &iec958, sizeof(iec958)) < 0) {
        ALOGE("%s: Could not set channel status for ext HDMI ",
              __func__);
        return;
    }

}
#endif
