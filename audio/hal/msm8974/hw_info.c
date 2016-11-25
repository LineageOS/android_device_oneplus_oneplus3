/*
 * Copyright (c) 2013-2016, The Linux Foundation. All rights reserved.
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

#define LOG_TAG "hardware_info"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0

#include <stdlib.h>
#include <dlfcn.h>
#include <cutils/log.h>
#include <cutils/str_parms.h>
#include "audio_hw.h"
#include "platform.h"
#include "platform_api.h"


struct hardware_info {
    char name[HW_INFO_ARRAY_MAX_SIZE];
    char type[HW_INFO_ARRAY_MAX_SIZE];
    /* variables for handling target variants */
    uint32_t num_snd_devices;
    char dev_extn[HW_INFO_ARRAY_MAX_SIZE];
    snd_device_t  *snd_devices;
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static const snd_device_t taiko_fluid_variant_devices[] = {
    SND_DEVICE_OUT_SPEAKER,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES,
    SND_DEVICE_OUT_SPEAKER_AND_ANC_HEADSET,
};

static const snd_device_t taiko_CDP_variant_devices[] = {
    SND_DEVICE_OUT_SPEAKER,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES,
    SND_DEVICE_OUT_SPEAKER_AND_ANC_HEADSET,
    SND_DEVICE_IN_QUAD_MIC,
};

static const snd_device_t taiko_apq8084_CDP_variant_devices[] = {
    SND_DEVICE_IN_HANDSET_MIC,
};

static const snd_device_t taiko_liquid_variant_devices[] = {
    SND_DEVICE_OUT_SPEAKER,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES,
    SND_DEVICE_OUT_SPEAKER_AND_ANC_HEADSET,
    SND_DEVICE_IN_SPEAKER_MIC,
    SND_DEVICE_IN_HEADSET_MIC,
    SND_DEVICE_IN_VOICE_DMIC,
    SND_DEVICE_IN_VOICE_SPEAKER_DMIC,
    SND_DEVICE_IN_VOICE_REC_DMIC_STEREO,
    SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE,
    SND_DEVICE_IN_QUAD_MIC,
    SND_DEVICE_IN_HANDSET_STEREO_DMIC,
    SND_DEVICE_IN_SPEAKER_STEREO_DMIC,
};

static const snd_device_t tomtom_msm8994_CDP_variant_devices[] = {
    SND_DEVICE_IN_HANDSET_MIC,
};

static const snd_device_t tomtom_liquid_variant_devices[] = {
    SND_DEVICE_OUT_SPEAKER,
    SND_DEVICE_OUT_SPEAKER_EXTERNAL_1,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES_EXTERNAL_1,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES,
    SND_DEVICE_OUT_SPEAKER_AND_ANC_HEADSET,
    SND_DEVICE_IN_SPEAKER_MIC,
    SND_DEVICE_IN_HEADSET_MIC,
    SND_DEVICE_IN_VOICE_DMIC,
    SND_DEVICE_IN_VOICE_SPEAKER_DMIC,
    SND_DEVICE_IN_VOICE_REC_DMIC_STEREO,
    SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE,
    SND_DEVICE_IN_QUAD_MIC,
    SND_DEVICE_IN_HANDSET_STEREO_DMIC,
    SND_DEVICE_IN_SPEAKER_STEREO_DMIC,
};

static const snd_device_t tomtom_stp_variant_devices[] = {
    SND_DEVICE_OUT_SPEAKER,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES,
    SND_DEVICE_OUT_SPEAKER_AND_ANC_HEADSET,
};

static const snd_device_t taiko_DB_variant_devices[] = {
    SND_DEVICE_OUT_SPEAKER,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES,
    SND_DEVICE_OUT_SPEAKER_AND_ANC_HEADSET,
    SND_DEVICE_IN_SPEAKER_MIC,
    SND_DEVICE_IN_HEADSET_MIC,
    SND_DEVICE_IN_QUAD_MIC,
};

static const snd_device_t tomtom_DB_variant_devices[] = {
    SND_DEVICE_OUT_SPEAKER,
    SND_DEVICE_OUT_SPEAKER_EXTERNAL_1,
    SND_DEVICE_OUT_SPEAKER_EXTERNAL_2,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES_EXTERNAL_1,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES_EXTERNAL_2,
    SND_DEVICE_OUT_VOICE_SPEAKER,
    SND_DEVICE_IN_VOICE_SPEAKER_MIC,
    SND_DEVICE_IN_HANDSET_MIC,
    SND_DEVICE_IN_HANDSET_MIC_EXTERNAL
};

static const snd_device_t tasha_DB_variant_devices[] = {
    SND_DEVICE_OUT_SPEAKER
};

static const snd_device_t taiko_apq8084_sbc_variant_devices[] = {
    SND_DEVICE_IN_HANDSET_MIC,
    SND_DEVICE_IN_SPEAKER_MIC,
};

static const snd_device_t tapan_lite_variant_devices[] = {
    SND_DEVICE_OUT_SPEAKER,
    SND_DEVICE_OUT_HEADPHONES,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES,
    SND_DEVICE_OUT_VOICE_HEADPHONES,
    SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES,
    SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES,
};

static const snd_device_t tapan_skuf_variant_devices[] = {
    SND_DEVICE_OUT_SPEAKER,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES,
    SND_DEVICE_OUT_SPEAKER_AND_ANC_HEADSET,
    /*SND_DEVICE_OUT_SPEAKER_AND_ANC_FB_HEADSET,*/
};

static const snd_device_t tapan_lite_skuf_variant_devices[] = {
    SND_DEVICE_OUT_SPEAKER,
    SND_DEVICE_OUT_HEADPHONES,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES,
    SND_DEVICE_OUT_VOICE_HEADPHONES,
    SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES,
    SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES,
};

static const snd_device_t helicon_skuab_variant_devices[] = {
    SND_DEVICE_OUT_SPEAKER,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES,
    SND_DEVICE_OUT_SPEAKER_AND_ANC_HEADSET,
    SND_DEVICE_OUT_VOICE_SPEAKER,
};

static const snd_device_t tasha_fluid_variant_devices[] = {
    SND_DEVICE_OUT_SPEAKER,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES,
    SND_DEVICE_OUT_SPEAKER_AND_ANC_HEADSET,
    SND_DEVICE_OUT_VOICE_SPEAKER,
    SND_DEVICE_OUT_SPEAKER_AND_HDMI,
    SND_DEVICE_OUT_SPEAKER_AND_USB_HEADSET,
    SND_DEVICE_OUT_SPEAKER_PROTECTED,
    SND_DEVICE_OUT_VOICE_SPEAKER_PROTECTED,
};

static const snd_device_t tasha_liquid_variant_devices[] = {
    SND_DEVICE_OUT_SPEAKER,
    SND_DEVICE_OUT_SPEAKER_EXTERNAL_1,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES_EXTERNAL_1,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES,
    SND_DEVICE_OUT_SPEAKER_AND_ANC_HEADSET,
    SND_DEVICE_IN_SPEAKER_MIC,
    SND_DEVICE_IN_HEADSET_MIC,
    SND_DEVICE_IN_VOICE_DMIC,
    SND_DEVICE_IN_VOICE_SPEAKER_DMIC,
    SND_DEVICE_IN_VOICE_REC_DMIC_STEREO,
    SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE,
    SND_DEVICE_IN_QUAD_MIC,
    SND_DEVICE_IN_HANDSET_STEREO_DMIC,
    SND_DEVICE_IN_SPEAKER_STEREO_DMIC,
};

static void  update_hardware_info_8084(struct hardware_info *hw_info, const char *snd_card_name)
{
    if (!strcmp(snd_card_name, "apq8084-taiko-mtp-snd-card") ||
        !strncmp(snd_card_name, "apq8084-taiko-i2s-mtp-snd-card",
                 sizeof("apq8084-taiko-i2s-mtp-snd-card")) ||
        !strncmp(snd_card_name, "apq8084-tomtom-mtp-snd-card",
                 sizeof("apq8084-tomtom-mtp-snd-card"))) {
        strlcpy(hw_info->type, "mtp", sizeof(hw_info->type));
        strlcpy(hw_info->name, "apq8084", sizeof(hw_info->name));
        hw_info->snd_devices = NULL;
        hw_info->num_snd_devices = 0;
        strlcpy(hw_info->dev_extn, "", sizeof(hw_info->dev_extn));
    } else if ((!strcmp(snd_card_name, "apq8084-taiko-cdp-snd-card")) ||
        !strncmp(snd_card_name, "apq8084-tomtom-cdp-snd-card",
                 sizeof("apq8084-tomtom-cdp-snd-card"))) {
        strlcpy(hw_info->type, " cdp", sizeof(hw_info->type));
        strlcpy(hw_info->name, "apq8084", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)taiko_apq8084_CDP_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(taiko_apq8084_CDP_variant_devices);
        strlcpy(hw_info->dev_extn, "-cdp", sizeof(hw_info->dev_extn));
    } else if (!strncmp(snd_card_name, "apq8084-taiko-i2s-cdp-snd-card",
                        sizeof("apq8084-taiko-i2s-cdp-snd-card"))) {
        strlcpy(hw_info->type, " cdp", sizeof(hw_info->type));
        strlcpy(hw_info->name, "apq8084", sizeof(hw_info->name));
        hw_info->snd_devices = NULL;
        hw_info->num_snd_devices = 0;
        strlcpy(hw_info->dev_extn, "", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "apq8084-taiko-liquid-snd-card")) {
        strlcpy(hw_info->type , " liquid", sizeof(hw_info->type));
        strlcpy(hw_info->name, "apq8084", sizeof(hw_info->type));
        hw_info->snd_devices = (snd_device_t *)taiko_liquid_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(taiko_liquid_variant_devices);
        strlcpy(hw_info->dev_extn, "-liquid", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "apq8084-taiko-sbc-snd-card")) {
        strlcpy(hw_info->type, " sbc", sizeof(hw_info->type));
        strlcpy(hw_info->name, "apq8084", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)taiko_apq8084_sbc_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(taiko_apq8084_sbc_variant_devices);
        strlcpy(hw_info->dev_extn, "-sbc", sizeof(hw_info->dev_extn));
    } else {
        ALOGW("%s: Not an 8084 device", __func__);
    }
}

static void  update_hardware_info_8994(struct hardware_info *hw_info, const char *snd_card_name)
{
    if (!strcmp(snd_card_name, "msm8994-tomtom-mtp-snd-card")) {
        strlcpy(hw_info->type, " mtp", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8994", sizeof(hw_info->name));
        hw_info->snd_devices = NULL;
        hw_info->num_snd_devices = 0;
        strlcpy(hw_info->dev_extn, "", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "msm8994-tomtom-cdp-snd-card")) {
        strlcpy(hw_info->type, " cdp", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8994", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)tomtom_msm8994_CDP_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(tomtom_msm8994_CDP_variant_devices);
        strlcpy(hw_info->dev_extn, "-cdp", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "msm8994-tomtom-stp-snd-card")) {
        strlcpy(hw_info->type, " stp", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8994", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)tomtom_stp_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(tomtom_stp_variant_devices);
        strlcpy(hw_info->dev_extn, "-stp", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "msm8994-tomtom-liquid-snd-card")) {
        strlcpy(hw_info->type, " liquid", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8994", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)tomtom_liquid_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(tomtom_liquid_variant_devices);
        strlcpy(hw_info->dev_extn, "-liquid", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "msm8994-tomtom-db-snd-card")) {
        strlcpy(hw_info->type, " dragon-board", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8994", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)tomtom_DB_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(tomtom_DB_variant_devices);
        strlcpy(hw_info->dev_extn, "-db", sizeof(hw_info->dev_extn));
    } else {
        ALOGW("%s: Not an 8994 device", __func__);
    }
}

static void  update_hardware_info_8996(struct hardware_info *hw_info, const char *snd_card_name)
{
    if (!strcmp(snd_card_name, "msm8996-tasha-fluid-snd-card")) {
        strlcpy(hw_info->type, " fluid", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8996", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)tasha_fluid_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(tasha_fluid_variant_devices);
        strlcpy(hw_info->dev_extn, "-fluid", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "msm8996-tasha-liquid-snd-card")) {
        strlcpy(hw_info->type, " liquid", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8996", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)tasha_liquid_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(tasha_liquid_variant_devices);
        strlcpy(hw_info->dev_extn, "-liquid", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "msm8996-tasha-db-snd-card")) {
        strlcpy(hw_info->type, " dragon-board", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8996", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)tasha_DB_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(tasha_DB_variant_devices);
        strlcpy(hw_info->dev_extn, "-db", sizeof(hw_info->dev_extn));
    } else {
        ALOGW("%s: Not a 8996 device", __func__);
    }
}

static void  update_hardware_info_msmcobalt(struct hardware_info *hw_info, const char *snd_card_name)
{
    if (!strcmp(snd_card_name, "msmcobalt-tasha-fluid-snd-card")) {
        strlcpy(hw_info->type, " fluid", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msmcobalt", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)tasha_fluid_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(tasha_fluid_variant_devices);
        strlcpy(hw_info->dev_extn, "-fluid", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "msmcobalt-tasha-liquid-snd-card")) {
        strlcpy(hw_info->type, " liquid", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msmcobalt", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)tasha_liquid_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(tasha_liquid_variant_devices);
        strlcpy(hw_info->dev_extn, "-liquid", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "msmcobalt-tasha-db-snd-card")) {
        strlcpy(hw_info->type, " dragon-board", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msmcobalt", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)tasha_DB_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(tasha_DB_variant_devices);
        strlcpy(hw_info->dev_extn, "-db", sizeof(hw_info->dev_extn));
    } else {
        ALOGW("%s: Not a msmcobalt device", __func__);
    }
}

static void  update_hardware_info_8974(struct hardware_info *hw_info, const char *snd_card_name)
{
    if (!strcmp(snd_card_name, "msm8974-taiko-mtp-snd-card")) {
        strlcpy(hw_info->type, " mtp", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8974", sizeof(hw_info->name));
        hw_info->snd_devices = NULL;
        hw_info->num_snd_devices = 0;
        strlcpy(hw_info->dev_extn, "", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "msm8974-taiko-cdp-snd-card")) {
        strlcpy(hw_info->type, " cdp", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8974", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)taiko_CDP_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(taiko_CDP_variant_devices);
        strlcpy(hw_info->dev_extn, "-cdp", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "msm8974-taiko-fluid-snd-card")) {
        strlcpy(hw_info->type, " fluid", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8974", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *) taiko_fluid_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(taiko_fluid_variant_devices);
        strlcpy(hw_info->dev_extn, "-fluid", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "msm8974-taiko-liquid-snd-card")) {
        strlcpy(hw_info->type, " liquid", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8974", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)taiko_liquid_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(taiko_liquid_variant_devices);
        strlcpy(hw_info->dev_extn, "-liquid", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "apq8074-taiko-db-snd-card")) {
        strlcpy(hw_info->type, " dragon-board", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8974", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)taiko_DB_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(taiko_DB_variant_devices);
        strlcpy(hw_info->dev_extn, "-DB", sizeof(hw_info->dev_extn));
    } else {
        ALOGW("%s: Not an 8974 device", __func__);
    }
}

static void update_hardware_info_8610(struct hardware_info *hw_info, const char *snd_card_name)
{
    if (!strcmp(snd_card_name, "msm8x10-snd-card")) {
        strlcpy(hw_info->type, "", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8x10", sizeof(hw_info->name));
        hw_info->snd_devices = NULL;
        hw_info->num_snd_devices = 0;
        strlcpy(hw_info->dev_extn, "", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "msm8x10-skuab-snd-card")) {
        strlcpy(hw_info->type, "skuab", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8x10", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)helicon_skuab_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(helicon_skuab_variant_devices);
        strlcpy(hw_info->dev_extn, "-skuab", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "msm8x10-skuaa-snd-card")) {
        strlcpy(hw_info->type, " skuaa", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8x10", sizeof(hw_info->name));
        hw_info->snd_devices = NULL;
        hw_info->num_snd_devices = 0;
        strlcpy(hw_info->dev_extn, "", sizeof(hw_info->dev_extn));
    } else {
        ALOGW("%s: Not an  8x10 device", __func__);
    }
}

static void update_hardware_info_8226(struct hardware_info *hw_info, const char *snd_card_name)
{
    if (!strcmp(snd_card_name, "msm8226-tapan-snd-card")) {
        strlcpy(hw_info->type, "", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8226", sizeof(hw_info->name));
        hw_info->snd_devices = NULL;
        hw_info->num_snd_devices = 0;
        strlcpy(hw_info->dev_extn, "", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "msm8226-tapan9302-snd-card")) {
        strlcpy(hw_info->type, "tapan_lite", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8226", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)tapan_lite_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(tapan_lite_variant_devices);
        strlcpy(hw_info->dev_extn, "-lite", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "msm8226-tapan-skuf-snd-card")) {
        strlcpy(hw_info->type, " skuf", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8226", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *) tapan_skuf_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(tapan_skuf_variant_devices);
        strlcpy(hw_info->dev_extn, "-skuf", sizeof(hw_info->dev_extn));
    } else if (!strcmp(snd_card_name, "msm8226-tapan9302-skuf-snd-card")) {
        strlcpy(hw_info->type, " tapan9302-skuf", sizeof(hw_info->type));
        strlcpy(hw_info->name, "msm8226", sizeof(hw_info->name));
        hw_info->snd_devices = (snd_device_t *)tapan_lite_skuf_variant_devices;
        hw_info->num_snd_devices = ARRAY_SIZE(tapan_lite_skuf_variant_devices);
        strlcpy(hw_info->dev_extn, "-skuf-lite", sizeof(hw_info->dev_extn));
    } else {
        ALOGW("%s: Not an  8x26 device", __func__);
    }
}

void *hw_info_init(const char *snd_card_name)
{
    struct hardware_info *hw_info;

    hw_info = malloc(sizeof(struct hardware_info));
    if (!hw_info) {
        ALOGE("failed to allocate mem for hardware info");
        return NULL;
    }

    hw_info->snd_devices = NULL;
    hw_info->num_snd_devices = 0;
    strlcpy(hw_info->dev_extn, "", sizeof(hw_info->dev_extn));
    strlcpy(hw_info->type, "", sizeof(hw_info->type));
    strlcpy(hw_info->name, "", sizeof(hw_info->name));

    if(strstr(snd_card_name, "msm8974") ||
              strstr(snd_card_name, "apq8074")) {
        ALOGV("8974 - variant soundcard");
        update_hardware_info_8974(hw_info, snd_card_name);
    } else if(strstr(snd_card_name, "msm8226")) {
        ALOGV("8x26 - variant soundcard");
        update_hardware_info_8226(hw_info, snd_card_name);
    } else if(strstr(snd_card_name, "msm8x10")) {
        ALOGV("8x10 - variant soundcard");
        update_hardware_info_8610(hw_info, snd_card_name);
    } else if(strstr(snd_card_name, "apq8084")) {
        ALOGV("8084 - variant soundcard");
        update_hardware_info_8084(hw_info, snd_card_name);
    } else if(strstr(snd_card_name, "msm8994")) {
        ALOGV("8994 - variant soundcard");
        update_hardware_info_8994(hw_info, snd_card_name);
    } else if(strstr(snd_card_name, "msm8996")) {
        ALOGV("8996 - variant soundcard");
        update_hardware_info_8996(hw_info, snd_card_name);
    } else if(strstr(snd_card_name, "msmcobalt")) {
        ALOGV("MSMCOBALT - variant soundcard");
        update_hardware_info_msmcobalt(hw_info, snd_card_name);
    } else {
        ALOGE("%s: Unsupported target %s:",__func__, snd_card_name);
        free(hw_info);
        hw_info = NULL;
    }

    return hw_info;
}

void hw_info_deinit(void *hw_info)
{
    struct hardware_info *my_data = (struct hardware_info*) hw_info;

    if(!my_data)
        free(my_data);
}

void hw_info_append_hw_type(void *hw_info, snd_device_t snd_device,
                            char *device_name)
{
    struct hardware_info *my_data = (struct hardware_info*) hw_info;
    uint32_t i = 0;

    snd_device_t *snd_devices =
            (snd_device_t *) my_data->snd_devices;

    if(snd_devices != NULL) {
        for (i = 0; i <  my_data->num_snd_devices; i++) {
            if (snd_device == (snd_device_t)snd_devices[i]) {
                ALOGV("extract dev_extn device %d, extn = %s",
                        (snd_device_t)snd_devices[i],  my_data->dev_extn);
                CHECK(strlcat(device_name,  my_data->dev_extn,
                        DEVICE_NAME_MAX_SIZE) < DEVICE_NAME_MAX_SIZE);
                break;
            }
        }
    }
    ALOGD("%s : device_name = %s", __func__,device_name);
}
