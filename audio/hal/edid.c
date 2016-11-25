/*
 * Copyright (c) 2014, 2016, The Linux Foundation. All rights reserved.
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

#define LOG_TAG "audio_hw_edid"
/*#define LOG_NDEBUG 0*/
/*#define LOG_NDDEBUG 0*/

#include <errno.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <cutils/str_parms.h>
#include <cutils/log.h>

#include "audio_hw.h"
#include "platform.h"
#include "platform_api.h"
#include "edid.h"

static const char * edid_format_to_str(unsigned char format)
{
    char * format_str = "??";

    switch (format) {
    case LPCM:
        format_str = "Format:LPCM";
        break;
    case AC3:
        format_str = "Format:AC-3";
        break;
    case MPEG1:
        format_str = "Format:MPEG1 (Layers 1 & 2)";
        break;
    case MP3:
        format_str = "Format:MP3 (MPEG1 Layer 3)";
        break;
    case MPEG2_MULTI_CHANNEL:
        format_str = "Format:MPEG2 (multichannel)";
        break;
    case AAC:
        format_str = "Format:AAC";
        break;
    case DTS:
        format_str = "Format:DTS";
        break;
    case ATRAC:
        format_str = "Format:ATRAC";
        break;
    case SACD:
        format_str = "Format:One-bit audio aka SACD";
        break;
    case DOLBY_DIGITAL_PLUS:
        format_str = "Format:Dolby Digital +";
        break;
    case DTS_HD:
        format_str = "Format:DTS-HD";
        break;
    case MAT:
        format_str = "Format:MAT (MLP)";
        break;
    case DST:
        format_str = "Format:DST";
        break;
    case WMA_PRO:
        format_str = "Format:WMA Pro";
        break;
    default:
        break;
    }
    return format_str;
}

static int get_edid_sf(unsigned char byte)
{
    int nfreq = 0;

    if (byte & BIT(6)) {
        ALOGV("192kHz");
        nfreq = 192000;
    } else if (byte & BIT(5)) {
        ALOGV("176kHz");
        nfreq = 176000;
    } else if (byte & BIT(4)) {
        ALOGV("96kHz");
        nfreq = 96000;
    } else if (byte & BIT(3)) {
        ALOGV("88.2kHz");
        nfreq = 88200;
    } else if (byte & BIT(2)) {
        ALOGV("48kHz");
        nfreq = 48000;
    } else if (byte & BIT(1)) {
        ALOGV("44.1kHz");
        nfreq = 44100;
    } else if (byte & BIT(0)) {
        ALOGV("32kHz");
        nfreq = 32000;
    }
    return nfreq;
}

static int get_edid_bps(unsigned char byte,
                        unsigned char format)
{
    int bits_per_sample = 0;
    if (format == 1) {
        if (byte & BIT(2)) {
            ALOGV("24bit");
            bits_per_sample = 24;
        } else if (byte & BIT(1)) {
            ALOGV("20bit");
            bits_per_sample = 20;
        } else if (byte & BIT(0)) {
            ALOGV("16bit");
            bits_per_sample = 16;
        }
    } else {
        ALOGV("not lpcm format, return 0");
        return 0;
    }
    return bits_per_sample;
}

static void update_channel_map(edid_audio_info* info)
{
    /* HDMI Cable follows CEA standard so SAD is received in CEA
     * Input source file channel map is fed to ASM in WAV standard(audio.h)
     * so upto 7.1 SAD bits are:
     * in CEA convention: RLC/RRC,FLC/FRC,RC,RL/RR,FC,LFE,FL/FR
     * in WAV convention: BL/BR,FLC/FRC,BC,SL/SR,FC,LFE,FL/FR
     * Corresponding ADSP IDs (apr-audio_v2.h):
     * PCM_CHANNEL_FL/PCM_CHANNEL_FR,
     * PCM_CHANNEL_LFE,
     * PCM_CHANNEL_FC,
     * PCM_CHANNEL_LS/PCM_CHANNEL_RS,
     * PCM_CHANNEL_CS,
     * PCM_CHANNEL_FLC/PCM_CHANNEL_FRC
     * PCM_CHANNEL_LB/PCM_CHANNEL_RB
     */
    if (!info)
        return;
    memset(info->channel_map, 0, MAX_CHANNELS_SUPPORTED);
    if(info->speaker_allocation[0] & BIT(0)) {
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
    }
    if(info->speaker_allocation[0] & BIT(1)) {
        info->channel_map[2] = PCM_CHANNEL_LFE;
    }
    if(info->speaker_allocation[0] & BIT(2)) {
        info->channel_map[3] = PCM_CHANNEL_FC;
    }
    if(info->speaker_allocation[0] & BIT(3)) {
    /*
     * As per CEA(HDMI Cable) standard Bit 3 is equivalent
     * to SideLeft/SideRight of WAV standard
     */
        info->channel_map[4] = PCM_CHANNEL_LS;
        info->channel_map[5] = PCM_CHANNEL_RS;
    }
    if(info->speaker_allocation[0] & BIT(4)) {
        if(info->speaker_allocation[0] & BIT(3)) {
            info->channel_map[6] = PCM_CHANNEL_CS;
            info->channel_map[7] = 0;
        } else if (info->speaker_allocation[1] & BIT(1)) {
            info->channel_map[6] = PCM_CHANNEL_CS;
            info->channel_map[7] = PCM_CHANNEL_TS;
        } else if (info->speaker_allocation[1] & BIT(2)) {
            info->channel_map[6] = PCM_CHANNEL_CS;
            info->channel_map[7] = PCM_CHANNEL_CVH;
        } else {
            info->channel_map[4] = PCM_CHANNEL_CS;
            info->channel_map[5] = 0;
        }
    }
    if(info->speaker_allocation[0] & BIT(5)) {
        info->channel_map[6] = PCM_CHANNEL_FLC;
        info->channel_map[7] = PCM_CHANNEL_FRC;
    }
    if(info->speaker_allocation[0] & BIT(6)) {
        // If RLC/RRC is present, RC is invalid as per specification
        info->speaker_allocation[0] &= 0xef;
        /*
         * As per CEA(HDMI Cable) standard Bit 6 is equivalent
         * to BackLeft/BackRight of WAV standard
         */
        info->channel_map[6] = PCM_CHANNEL_LB;
        info->channel_map[7] = PCM_CHANNEL_RB;
    }
    // higher channel are not defined by LPASS
    //info->nSpeakerAllocation[0] &= 0x3f;
    if(info->speaker_allocation[0] & BIT(7)) {
        info->channel_map[6] = 0; // PCM_CHANNEL_FLW; but not defined by LPASS
        info->channel_map[7] = 0; // PCM_CHANNEL_FRW; but not defined by LPASS
    }
    if(info->speaker_allocation[1] & BIT(0)) {
        info->channel_map[6] = 0; // PCM_CHANNEL_FLH; but not defined by LPASS
        info->channel_map[7] = 0; // PCM_CHANNEL_FRH; but not defined by LPASS
    }

    ALOGI("%s channel map updated to [%d %d %d %d %d %d %d %d ]  [%x %x %x]", __func__
        , info->channel_map[0], info->channel_map[1], info->channel_map[2]
        , info->channel_map[3], info->channel_map[4], info->channel_map[5]
        , info->channel_map[6], info->channel_map[7]
        , info->speaker_allocation[0], info->speaker_allocation[1]
        , info->speaker_allocation[2]);
}

static void dump_speaker_allocation(edid_audio_info* info)
{
    if (!info)
        return;

    if (info->speaker_allocation[0] & BIT(7))
        ALOGV("FLW/FRW");
    if (info->speaker_allocation[0] & BIT(6))
        ALOGV("RLC/RRC");
    if (info->speaker_allocation[0] & BIT(5))
        ALOGV("FLC/FRC");
    if (info->speaker_allocation[0] & BIT(4))
        ALOGV("RC");
    if (info->speaker_allocation[0] & BIT(3))
        ALOGV("RL/RR");
    if (info->speaker_allocation[0] & BIT(2))
        ALOGV("FC");
    if (info->speaker_allocation[0] & BIT(1))
        ALOGV("LFE");
    if (info->speaker_allocation[0] & BIT(0))
        ALOGV("FL/FR");
    if (info->speaker_allocation[1] & BIT(2))
        ALOGV("FCH");
    if (info->speaker_allocation[1] & BIT(1))
        ALOGV("TC");
    if (info->speaker_allocation[1] & BIT(0))
        ALOGV("FLH/FRH");
}

static void update_channel_allocation(edid_audio_info* info)
{
    int16_t ca;
    int16_t spkr_alloc;

    if (!info)
        return;

    /* Most common 5.1 SAD is 0xF, ca 0x0b
     * and 7.1 SAD is 0x4F, ca 0x13 */
    spkr_alloc = ((info->speaker_allocation[1]) << 8) |
               (info->speaker_allocation[0]);
    ALOGV("info->nSpeakerAllocation %x %x\n", info->speaker_allocation[0],
                                              info->speaker_allocation[1]);
    ALOGV("spkr_alloc: %x", spkr_alloc);

    /* The below switch case calculates channel allocation values
       as defined in CEA-861 section 6.6.2 */
    switch (spkr_alloc) {
    case BIT(0):                                           ca = 0x00; break;
    case BIT(0)|BIT(1):                                    ca = 0x01; break;
    case BIT(0)|BIT(2):                                    ca = 0x02; break;
    case BIT(0)|BIT(1)|BIT(2):                             ca = 0x03; break;
    case BIT(0)|BIT(4):                                    ca = 0x04; break;
    case BIT(0)|BIT(1)|BIT(4):                             ca = 0x05; break;
    case BIT(0)|BIT(2)|BIT(4):                             ca = 0x06; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(4):                      ca = 0x07; break;
    case BIT(0)|BIT(3):                                    ca = 0x08; break;
    case BIT(0)|BIT(1)|BIT(3):                             ca = 0x09; break;
    case BIT(0)|BIT(2)|BIT(3):                             ca = 0x0A; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3):                      ca = 0x0B; break;
    case BIT(0)|BIT(3)|BIT(4):                             ca = 0x0C; break;
    case BIT(0)|BIT(1)|BIT(3)|BIT(4):                      ca = 0x0D; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(4):                      ca = 0x0E; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4):               ca = 0x0F; break;
    case BIT(0)|BIT(3)|BIT(6):                             ca = 0x10; break;
    case BIT(0)|BIT(1)|BIT(3)|BIT(6):                      ca = 0x11; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(6):                      ca = 0x12; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(6):               ca = 0x13; break;
    case BIT(0)|BIT(5):                                    ca = 0x14; break;
    case BIT(0)|BIT(1)|BIT(5):                             ca = 0x15; break;
    case BIT(0)|BIT(2)|BIT(5):                             ca = 0x16; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(5):                      ca = 0x17; break;
    case BIT(0)|BIT(4)|BIT(5):                             ca = 0x18; break;
    case BIT(0)|BIT(1)|BIT(4)|BIT(5):                      ca = 0x19; break;
    case BIT(0)|BIT(2)|BIT(4)|BIT(5):                      ca = 0x1A; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(4)|BIT(5):               ca = 0x1B; break;
    case BIT(0)|BIT(3)|BIT(5):                             ca = 0x1C; break;
    case BIT(0)|BIT(1)|BIT(3)|BIT(5):                      ca = 0x1D; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(5):                      ca = 0x1E; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(5):               ca = 0x1F; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(10):                     ca = 0x20; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(10):              ca = 0x21; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(9):                      ca = 0x22; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(9):               ca = 0x23; break;
    case BIT(0)|BIT(3)|BIT(8):                             ca = 0x24; break;
    case BIT(0)|BIT(1)|BIT(3)|BIT(8):                      ca = 0x25; break;
    case BIT(0)|BIT(3)|BIT(7):                             ca = 0x26; break;
    case BIT(0)|BIT(1)|BIT(3)|BIT(7):                      ca = 0x27; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(4)|BIT(9):               ca = 0x28; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(9):        ca = 0x29; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(4)|BIT(10):              ca = 0x2A; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(10):       ca = 0x2B; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(9)|BIT(10):              ca = 0x2C; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(9)|BIT(10):       ca = 0x2D; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(8):                      ca = 0x2E; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(8):               ca = 0x2F; break;
    case BIT(0)|BIT(2)|BIT(3)|BIT(7):                      ca = 0x30; break;
    case BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(7):               ca = 0x31; break;
    default:                                               ca = 0x0;  break;
    }
    ALOGD("%s channel allocation: %x", __func__, ca);
    info->channel_allocation = ca;
}

static void update_channel_map_lpass(edid_audio_info* info)
{
    if (!info)
        return;
    if (info->channel_allocation < 0 || info->channel_allocation > 0x1f) {
        ALOGE("Channel allocation out of supported range");
        return;
    }
    ALOGV("channel_allocation 0x%x", info->channel_allocation);
    memset(info->channel_map, 0, MAX_CHANNELS_SUPPORTED);
    switch(info->channel_allocation) {
    case 0x0:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        break;
    case 0x1:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LFE;
        break;
    case 0x2:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_FC;
        break;
    case 0x3:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LFE;
        info->channel_map[3] = PCM_CHANNEL_FC;
        break;
    case 0x4:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_CS;
        break;
    case 0x5:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LFE;
        info->channel_map[3] = PCM_CHANNEL_CS;
        break;
    case 0x6:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_FC;
        info->channel_map[3] = PCM_CHANNEL_CS;
        break;
    case 0x7:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LFE;
        info->channel_map[3] = PCM_CHANNEL_FC;
        info->channel_map[4] = PCM_CHANNEL_CS;
        break;
    case 0x8:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LS;
        info->channel_map[3] = PCM_CHANNEL_RS;
        break;
    case 0x9:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LFE;
        info->channel_map[3] = PCM_CHANNEL_LS;
        info->channel_map[4] = PCM_CHANNEL_RS;
        break;
    case 0xa:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_FC;
        info->channel_map[3] = PCM_CHANNEL_LS;
        info->channel_map[4] = PCM_CHANNEL_RS;
        break;
    case 0xb:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LFE;
        info->channel_map[3] = PCM_CHANNEL_FC;
        info->channel_map[4] = PCM_CHANNEL_LS;
        info->channel_map[5] = PCM_CHANNEL_RS;
        break;
    case 0xc:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LS;
        info->channel_map[3] = PCM_CHANNEL_RS;
        info->channel_map[4] = PCM_CHANNEL_CS;
        break;
    case 0xd:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LFE;
        info->channel_map[3] = PCM_CHANNEL_LS;
        info->channel_map[4] = PCM_CHANNEL_RS;
        info->channel_map[5] = PCM_CHANNEL_CS;
        break;
    case 0xe:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_FC;
        info->channel_map[3] = PCM_CHANNEL_LS;
        info->channel_map[4] = PCM_CHANNEL_RS;
        info->channel_map[5] = PCM_CHANNEL_CS;
        break;
    case 0xf:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LFE;
        info->channel_map[3] = PCM_CHANNEL_FC;
        info->channel_map[4] = PCM_CHANNEL_LS;
        info->channel_map[5] = PCM_CHANNEL_RS;
        info->channel_map[6] = PCM_CHANNEL_CS;
        break;
    case 0x10:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LS;
        info->channel_map[3] = PCM_CHANNEL_RS;
        info->channel_map[4] = PCM_CHANNEL_LB;
        info->channel_map[5] = PCM_CHANNEL_RB;
        break;
    case 0x11:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LFE;
        info->channel_map[3] = PCM_CHANNEL_LS;
        info->channel_map[4] = PCM_CHANNEL_RS;
        info->channel_map[5] = PCM_CHANNEL_LB;
        info->channel_map[6] = PCM_CHANNEL_RB;
        break;
    case 0x12:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_FC;
        info->channel_map[3] = PCM_CHANNEL_LS;
        info->channel_map[4] = PCM_CHANNEL_RS;
        info->channel_map[5] = PCM_CHANNEL_LB;
        info->channel_map[6] = PCM_CHANNEL_RB;
        break;
    case 0x13:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LFE;
        info->channel_map[3] = PCM_CHANNEL_FC;
        info->channel_map[4] = PCM_CHANNEL_LS;
        info->channel_map[5] = PCM_CHANNEL_RS;
        info->channel_map[6] = PCM_CHANNEL_LB;
        info->channel_map[7] = PCM_CHANNEL_RB;
        break;
    case 0x14:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_FLC;
        info->channel_map[3] = PCM_CHANNEL_FRC;
        break;
    case 0x15:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LFE;
        info->channel_map[3] = PCM_CHANNEL_FLC;
        info->channel_map[4] = PCM_CHANNEL_FRC;
        break;
    case 0x16:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_FC;
        info->channel_map[3] = PCM_CHANNEL_FLC;
        info->channel_map[4] = PCM_CHANNEL_FRC;
        break;
    case 0x17:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LFE;
        info->channel_map[3] = PCM_CHANNEL_FC;
        info->channel_map[4] = PCM_CHANNEL_FLC;
        info->channel_map[5] = PCM_CHANNEL_FRC;
        break;
    case 0x18:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_CS;
        info->channel_map[3] = PCM_CHANNEL_FLC;
        info->channel_map[4] = PCM_CHANNEL_FRC;
        break;
    case 0x19:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LFE;
        info->channel_map[3] = PCM_CHANNEL_CS;
        info->channel_map[4] = PCM_CHANNEL_FLC;
        info->channel_map[5] = PCM_CHANNEL_FRC;
        break;
    case 0x1a:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_FC;
        info->channel_map[3] = PCM_CHANNEL_CS;
        info->channel_map[4] = PCM_CHANNEL_FLC;
        info->channel_map[5] = PCM_CHANNEL_FRC;
        break;
    case 0x1b:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LFE;
        info->channel_map[3] = PCM_CHANNEL_FC;
        info->channel_map[4] = PCM_CHANNEL_CS;
        info->channel_map[5] = PCM_CHANNEL_FLC;
        info->channel_map[6] = PCM_CHANNEL_FRC;
        break;
    case 0x1c:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LS;
        info->channel_map[3] = PCM_CHANNEL_RS;
        info->channel_map[4] = PCM_CHANNEL_FLC;
        info->channel_map[5] = PCM_CHANNEL_FRC;
        break;
    case 0x1d:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LFE;
        info->channel_map[3] = PCM_CHANNEL_LS;
        info->channel_map[4] = PCM_CHANNEL_RS;
        info->channel_map[5] = PCM_CHANNEL_FLC;
        info->channel_map[6] = PCM_CHANNEL_FRC;
        break;
    case 0x1e:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_FC;
        info->channel_map[3] = PCM_CHANNEL_LS;
        info->channel_map[4] = PCM_CHANNEL_RS;
        info->channel_map[5] = PCM_CHANNEL_FLC;
        info->channel_map[6] = PCM_CHANNEL_FRC;
        break;
    case 0x1f:
        info->channel_map[0] = PCM_CHANNEL_FL;
        info->channel_map[1] = PCM_CHANNEL_FR;
        info->channel_map[2] = PCM_CHANNEL_LFE;
        info->channel_map[3] = PCM_CHANNEL_FC;
        info->channel_map[4] = PCM_CHANNEL_LS;
        info->channel_map[5] = PCM_CHANNEL_RS;
        info->channel_map[6] = PCM_CHANNEL_FLC;
        info->channel_map[7] = PCM_CHANNEL_FRC;
        break;
    default:
        break;
    }
    ALOGD("%s channel map updated to [%d %d %d %d %d %d %d %d ]", __func__
        , info->channel_map[0], info->channel_map[1], info->channel_map[2]
        , info->channel_map[3], info->channel_map[4], info->channel_map[5]
        , info->channel_map[6], info->channel_map[7]);
}

static void dump_edid_data(edid_audio_info *info)
{

    int i;
    for (i = 0; i < info->audio_blocks && i < MAX_EDID_BLOCKS; i++) {
        ALOGV("%s:FormatId:%d rate:%d bps:%d channels:%d", __func__,
              info->audio_blocks_array[i].format_id,
              info->audio_blocks_array[i].sampling_freq,
              info->audio_blocks_array[i].bits_per_sample,
              info->audio_blocks_array[i].channels);
    }
    ALOGV("%s:no of audio blocks:%d", __func__, info->audio_blocks);
    ALOGV("%s:speaker allocation:[%x %x %x]", __func__,
           info->speaker_allocation[0], info->speaker_allocation[1],
           info->speaker_allocation[2]);
    ALOGV("%s:channel map:[%x %x %x %x %x %x %x %x]", __func__,
           info->channel_map[0], info->channel_map[1],
           info->channel_map[2], info->channel_map[3],
           info->channel_map[4], info->channel_map[5],
           info->channel_map[6], info->channel_map[7]);
    ALOGV("%s:channel allocation:%d", __func__, info->channel_allocation);
    ALOGV("%s:[%d %d %d %d %d %d %d %d ]", __func__,
           info->channel_map[0], info->channel_map[1],
           info->channel_map[2], info->channel_map[3],
           info->channel_map[4], info->channel_map[5],
           info->channel_map[6], info->channel_map[7]);
}

bool edid_get_sink_caps(edid_audio_info* info, char *edid_data)
{
    unsigned char channels[MAX_EDID_BLOCKS];
    unsigned char formats[MAX_EDID_BLOCKS];
    unsigned char frequency[MAX_EDID_BLOCKS];
    unsigned char bitrate[MAX_EDID_BLOCKS];
    int i = 0;
    int length, count_desc;

    if (!info || !edid_data) {
        ALOGE("No valid EDID");
        return false;
    }

    length = (int) *edid_data++;
    ALOGV("Total length is %d",length);

    count_desc = length/MIN_AUDIO_DESC_LENGTH;

    if (!count_desc) {
        ALOGE("insufficient descriptors");
        return false;
    }

    memset(info, 0, sizeof(edid_audio_info));

    info->audio_blocks = count_desc-1;
    if (info->audio_blocks > MAX_EDID_BLOCKS) {
        info->audio_blocks = MAX_EDID_BLOCKS;
    }

    ALOGV("Total # of audio descriptors %d",count_desc);

    for (i=0; i<info->audio_blocks; i++) {
        // last block for speaker allocation;
        channels [i]   = (*edid_data & 0x7) + 1;
        formats  [i]   = (*edid_data++) >> 3;
        frequency[i]   = *edid_data++;
        bitrate  [i]   = *edid_data++;
    }
    info->speaker_allocation[0] = *edid_data++;
    info->speaker_allocation[1] = *edid_data++;
    info->speaker_allocation[2] = *edid_data++;

    update_channel_map(info);
    update_channel_allocation(info);
    update_channel_map_lpass(info);

    for (i=0; i<info->audio_blocks; i++) {
        ALOGV("AUDIO DESC BLOCK # %d\n",i);

        info->audio_blocks_array[i].channels = channels[i];
        ALOGD("info->audio_blocks_array[i].channels %d\n",
              info->audio_blocks_array[i].channels);

        ALOGV("Format Byte %d\n", formats[i]);
        info->audio_blocks_array[i].format_id = (edid_audio_format_id)formats[i];
        ALOGD("info->audio_blocks_array[i].format_id %s",
              edid_format_to_str(formats[i]));

        ALOGV("Frequency Byte %d\n", frequency[i]);
        info->audio_blocks_array[i].sampling_freq = get_edid_sf(frequency[i]);
        ALOGV("info->audio_blocks_array[i].sampling_freq %d",
              info->audio_blocks_array[i].sampling_freq);

        ALOGV("BitsPerSample Byte %d\n", bitrate[i]);
        info->audio_blocks_array[i].bits_per_sample =
                   get_edid_bps(bitrate[i],formats[i]);
        ALOGV("info->audio_blocks_array[i].bits_per_sample %d",
              info->audio_blocks_array[i].bits_per_sample);
    }
    dump_speaker_allocation(info);
    dump_edid_data(info);
    return true;
}

bool edid_is_supported_sr(edid_audio_info* info, int sr)
{
    int i = 0;
    if (info != NULL && sr != 0) {
        for (i = 0; i < info->audio_blocks && i < MAX_EDID_BLOCKS; i++) {
            if (info->audio_blocks_array[i].sampling_freq == sr) {
                ALOGV("%s: returns true for sample rate [%d]",
                      __func__, sr);
                return true;
            }
        }
    }
    ALOGV("%s: returns false for sample rate [%d]",
           __func__, sr);
    return false;
}

bool edid_is_supported_bps(edid_audio_info* info, int bps)
{
    int i = 0;

    if (bps == 16) {
        //16 bit bps is always supported
        //some oem may not update 16bit support in their edid info
        return true;
    }

    if (info != NULL && bps != 0) {
        for (i = 0; i < info->audio_blocks && i < MAX_EDID_BLOCKS; i++) {
            if (info->audio_blocks_array[i].bits_per_sample == bps) {
                ALOGV("%s: returns true for bit width [%d]",
                      __func__, bps);
                return true;
            }
        }
    }
    ALOGV("%s: returns false for bit width [%d]",
           __func__, bps);
    return false;
}
