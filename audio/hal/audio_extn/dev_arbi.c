/*
 * Copyright (c) 2014, The Linux Foundation. All rights reserved.
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

#define LOG_TAG "audio_hw_dev_arbi"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0

#include <errno.h>
#include <cutils/log.h>
#include <fcntl.h>
#include "audio_hw.h"
#include "platform.h"
#include "platform_api.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <cutils/properties.h>
#include "audio_extn.h"

#ifdef DEV_ARBI_ENABLED

typedef int (init_fn_t)();
typedef int (deinit_fn_t)();
typedef int (acquire_fn_t)(audio_devices_t aud_dev);
typedef int (release_fn_t)(audio_devices_t aud_dev);

typedef struct {
    snd_device_t snd_device;
    audio_devices_t aud_device;
} snd_aud_dev_mapping_t;

static void* lib_handle = NULL;

static init_fn_t *init_fp = NULL;
static deinit_fn_t *deinit_fp = NULL;
static acquire_fn_t *acquire_fp = NULL;
static release_fn_t *release_fp = NULL;

static int load_dev_arbi_lib()
{
    int rc = -EINVAL;

    if (lib_handle != NULL) {
        ALOGE("%s: library already loaded", __func__);
        return rc;
    }

    lib_handle = dlopen("libaudiodevarb.so", RTLD_NOW);
    if (lib_handle != NULL) {
        init_fp = (init_fn_t*)dlsym(lib_handle, "aud_dev_arbi_server_init");
        deinit_fp = (deinit_fn_t*)dlsym(lib_handle, "aud_dev_arbi_server_deinit");
        acquire_fp = (acquire_fn_t*)dlsym(lib_handle, "aud_dev_arbi_server_acquire");
        release_fp = (release_fn_t*)dlsym(lib_handle, "aud_dev_arbi_server_release");

        if ((init_fp == NULL) ||
            (deinit_fp == NULL) ||
            (acquire_fp == NULL) ||
            (release_fp == NULL)) {

            ALOGE("%s: error loading symbols from library", __func__);

            init_fp = NULL;
            deinit_fp = NULL;
            acquire_fp = NULL;
            release_fp = NULL;
        } else
            return 0;
    }

    return rc;
}

int audio_extn_dev_arbi_init()
{
    int rc = load_dev_arbi_lib();
    if (!rc)
        rc = init_fp();

    return rc;
}

int audio_extn_dev_arbi_deinit()
{
    int rc = -EINVAL;

    if(deinit_fp != NULL) {
        rc = deinit_fp();

        init_fp = NULL;
        deinit_fp = NULL;
        acquire_fp = NULL;
        release_fp = NULL;

        dlclose(lib_handle);
        lib_handle = NULL;
    }

    return rc;
}

static audio_devices_t get_audio_device(snd_device_t snd_device)
{
    static snd_aud_dev_mapping_t snd_aud_dev_map[] = {
        {SND_DEVICE_OUT_HANDSET, AUDIO_DEVICE_OUT_EARPIECE},
        {SND_DEVICE_OUT_VOICE_HANDSET, AUDIO_DEVICE_OUT_EARPIECE},
        {SND_DEVICE_OUT_SPEAKER, AUDIO_DEVICE_OUT_SPEAKER},
        {SND_DEVICE_OUT_VOICE_SPEAKER, AUDIO_DEVICE_OUT_SPEAKER},
        {SND_DEVICE_OUT_HEADPHONES, AUDIO_DEVICE_OUT_WIRED_HEADPHONE},
        {SND_DEVICE_OUT_VOICE_HEADPHONES, AUDIO_DEVICE_OUT_WIRED_HEADPHONE},
        {SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES,
            AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_WIRED_HEADPHONE}
    };

    audio_devices_t aud_device = AUDIO_DEVICE_NONE;
    uint32_t ind = 0;

    for (ind = 0; ind < ARRAY_SIZE(snd_aud_dev_map); ++ind) {
        if (snd_device == snd_aud_dev_map[ind].snd_device) {
            aud_device = snd_aud_dev_map[ind].aud_device;
            break;
        }
    }

    return aud_device;
}

int audio_extn_dev_arbi_acquire(snd_device_t snd_device)
{
    int rc = -EINVAL;
    audio_devices_t audio_device = get_audio_device(snd_device);

    if ((acquire_fp != NULL) && (audio_device != AUDIO_DEVICE_NONE))
        rc = acquire_fp(audio_device);

    return rc;
}

int audio_extn_dev_arbi_release(snd_device_t snd_device)
{
    int rc = -EINVAL;
    audio_devices_t audio_device = get_audio_device(snd_device);

    if ((release_fp != NULL) && (audio_device != AUDIO_DEVICE_NONE))
        rc = release_fp(audio_device);

    return rc;
}

#endif /*DEV_ARBI_ENABLED*/
