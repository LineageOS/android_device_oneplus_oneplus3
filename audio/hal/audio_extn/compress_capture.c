/*
 * Copyright (c) 2013 - 2014, The Linux Foundation. All rights reserved.
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

#define LOG_TAG "audio_hw_compress"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0

#include <errno.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <cutils/str_parms.h>
#include <cutils/log.h>

#include "audio_hw.h"
#include "platform.h"
#include "platform_api.h"

#include "sound/compress_params.h"
#include "sound/compress_offload.h"

#ifdef COMPRESS_CAPTURE_ENABLED

#define COMPRESS_IN_CONFIG_CHANNELS 1
#define COMPRESS_IN_CONFIG_PERIOD_SIZE 2048
#define COMPRESS_IN_CONFIG_PERIOD_COUNT 16


struct compress_in_module {
    uint8_t             *in_buf;
};

static struct compress_in_module c_in_mod = {
    .in_buf = NULL,
};


void audio_extn_compr_cap_init(struct stream_in *in)
{
    in->usecase = USECASE_AUDIO_RECORD_COMPRESS;
    in->config.channels = COMPRESS_IN_CONFIG_CHANNELS;
    in->config.period_size = COMPRESS_IN_CONFIG_PERIOD_SIZE;
    in->config.period_count= COMPRESS_IN_CONFIG_PERIOD_COUNT;
    in->config.format = AUDIO_FORMAT_AMR_WB;
    c_in_mod.in_buf = (uint8_t*)calloc(1, in->config.period_size*2);
}

void audio_extn_compr_cap_deinit()
{
    if (c_in_mod.in_buf) {
        free(c_in_mod.in_buf);
        c_in_mod.in_buf = NULL;
    }
}

bool audio_extn_compr_cap_enabled()
{
    char prop_value[PROPERTY_VALUE_MAX] = {0};
    bool tunnel_encode = false;

    property_get("tunnel.audio.encode",prop_value,"0");
    if (!strncmp("true", prop_value, sizeof("true")))
        return true;
    else
        return false;
}

bool audio_extn_compr_cap_format_supported(audio_format_t format)
{
    if (format == AUDIO_FORMAT_AMR_WB)
        return true;
    else
        return false;
}


bool audio_extn_compr_cap_usecase_supported(audio_usecase_t usecase)
{
    if ((usecase == USECASE_AUDIO_RECORD_COMPRESS) ||
        (usecase == USECASE_INCALL_REC_UPLINK_COMPRESS) ||
        (usecase == USECASE_INCALL_REC_DOWNLINK_COMPRESS) ||
        (usecase == USECASE_INCALL_REC_UPLINK_AND_DOWNLINK_COMPRESS))
        return true;
    else
        return false;
}


size_t audio_extn_compr_cap_get_buffer_size(audio_format_t format)
{
    if (format == AUDIO_FORMAT_AMR_WB)
        /*One AMR WB frame is 61 bytes. Return that to the caller.
        The buffer size is not altered, that is still period size.*/
        return AMR_WB_FRAMESIZE;
    else
        return 0;
}

size_t audio_extn_compr_cap_read(struct stream_in * in,
    void *buffer, size_t bytes)
{
    int ret;
    struct snd_compr_audio_info *header;
    uint32_t c_in_header;
    uint32_t c_in_buf_size;

    c_in_buf_size = in->config.period_size*2;

    if (in->pcm) {
        ret = pcm_read(in->pcm, c_in_mod.in_buf, c_in_buf_size);
        if (ret < 0) {
            ALOGE("pcm_read() returned failure: %d", ret);
            return ret;
        } else {
            header = (struct snd_compr_audio_info *) c_in_mod.in_buf;
            c_in_header = sizeof(*header) + header->reserved[0];
            if (header->frame_size > 0) {
                if (c_in_header  + header->frame_size > c_in_buf_size) {
                    ALOGW("AMR WB read buffer overflow.");
                    header->frame_size =
                        bytes - sizeof(*header) - header->reserved[0];
                }
                ALOGV("c_in_buf: %p, data offset: %p, header size: %zu,"
                    "reserved[0]: %u frame_size: %d", c_in_mod.in_buf,
                        c_in_mod.in_buf + c_in_header,
                        sizeof(*header), header->reserved[0],
                        header->frame_size);
                memcpy(buffer, c_in_mod.in_buf + c_in_header, header->frame_size);
            } else {
                ALOGE("pcm_read() with zero frame size");
                ret = -EINVAL;
            }
        }
    }

    return 0;
}

#endif /* COMPRESS_CAPTURE_ENABLED end */
