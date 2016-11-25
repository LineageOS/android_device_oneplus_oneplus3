/*
* Copyright (c) 2014-2016, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
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

#define LOG_TAG "keep_alive"
/*#define LOG_NDEBUG 0*/
#include <stdlib.h>
#include <cutils/log.h>
#include "audio_hw.h"
#include "audio_extn.h"
#include "platform_api.h"
#include <platform.h>

#define SILENCE_MIXER_PATH "silence-playback hdmi"
#define SILENCE_DEV_ID 32           /* index into machine driver */
#define SILENCE_INTERVAL 2 /*In secs*/

typedef enum {
    STATE_DEINIT = -1,
    STATE_IDLE,
    STATE_ACTIVE,
} state_t;

typedef enum {
    REQUEST_WRITE,
} request_t;

typedef struct {
    pthread_mutex_t lock;
    pthread_mutex_t sleep_lock;
    pthread_cond_t  cond;
    pthread_cond_t  wake_up_cond;
    pthread_t thread;
    state_t state;
    struct listnode cmd_list;
    struct pcm *pcm;
    bool done;
    void * userdata;
} keep_alive_t;

struct keep_alive_cmd {
    struct listnode node;
    request_t req;
};

static keep_alive_t ka;

static struct pcm_config silence_config = {
    .channels = 2,
    .rate = DEFAULT_OUTPUT_SAMPLING_RATE,
    .period_size = DEEP_BUFFER_OUTPUT_PERIOD_SIZE,
    .period_count = DEEP_BUFFER_OUTPUT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = DEEP_BUFFER_OUTPUT_PERIOD_SIZE / 4,
    .stop_threshold = INT_MAX,
    .avail_min = DEEP_BUFFER_OUTPUT_PERIOD_SIZE / 4,
};

static void * keep_alive_loop(void * context);

void audio_extn_keep_alive_init(struct audio_device *adev)
{
    ka.userdata = adev;
    ka.state = STATE_IDLE;
    ka.pcm = NULL;
    pthread_mutex_init(&ka.lock, (const pthread_mutexattr_t *) NULL);
    pthread_cond_init(&ka.cond, (const pthread_condattr_t *) NULL);
    pthread_cond_init(&ka.wake_up_cond, (const pthread_condattr_t *) NULL);
    pthread_mutex_init(&ka.sleep_lock, (const pthread_mutexattr_t *) NULL);
    list_init(&ka.cmd_list);
    if (pthread_create(&ka.thread,  (const pthread_attr_t *) NULL,
                       keep_alive_loop, NULL) < 0) {
        ALOGW("Failed to create keep_alive_thread");
        /* can continue without keep alive */
        ka.state = STATE_DEINIT;
    }
}

static void send_cmd_l(request_t r)
{
    if (ka.state == STATE_DEINIT)
        return;

    struct keep_alive_cmd *cmd =
        (struct keep_alive_cmd *)calloc(1, sizeof(struct keep_alive_cmd));

    cmd->req = r;
    list_add_tail(&ka.cmd_list, &cmd->node);
    pthread_cond_signal(&ka.cond);
}

static int close_silence_stream()
{
    if (!ka.pcm)
        return -ENODEV;

    pcm_close(ka.pcm);
    ka.pcm = NULL;
    return 0;
}

static int open_silence_stream()
{
    unsigned int flags = PCM_OUT|PCM_MONOTONIC;

    if (ka.pcm)
        return -EEXIST;

    ALOGD("opening silence device %d", SILENCE_DEV_ID);
    struct audio_device * adev = (struct audio_device *)ka.userdata;
    ka.pcm = pcm_open(adev->snd_card, SILENCE_DEV_ID,
                      flags, &silence_config);
    ALOGD("opened silence device %d", SILENCE_DEV_ID);
    if (ka.pcm == NULL || !pcm_is_ready(ka.pcm)) {
        ALOGE("%s: %s", __func__, pcm_get_error(ka.pcm));
        if (ka.pcm != NULL) {
            pcm_close(ka.pcm);
            ka.pcm = NULL;
        }
        return -1;
    }
    return 0;
}


static int set_mixer_control(struct mixer *mixer,
                             const char * mixer_ctl_name,
                             const char *mixer_val)
{
    struct mixer_ctl *ctl;
    if ((mixer == NULL) || (mixer_ctl_name == NULL) || (mixer_val == NULL)) {
       ALOGE("%s: Invalid input", __func__);
       return -EINVAL;
    }
    ALOGD("setting mixer ctl %s with value %s", mixer_ctl_name, mixer_val);
    ctl = mixer_get_ctl_by_name(mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        return -EINVAL;
    }

    return mixer_ctl_set_enum_by_string(ctl, mixer_val);
}

/* must be called with adev lock held */
void audio_extn_keep_alive_start()
{
    struct audio_device * adev = (struct audio_device *)ka.userdata;
    char mixer_ctl_name[MAX_LENGTH_MIXER_CONTROL_IN_INT];
    int app_type_cfg[MAX_LENGTH_MIXER_CONTROL_IN_INT], len = 0, rc;
    struct mixer_ctl *ctl;
    int acdb_dev_id, snd_device;
    struct listnode *node;
    struct audio_usecase *usecase;
    int32_t sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;

    pthread_mutex_lock(&ka.lock);

    if (ka.state == STATE_DEINIT) {
        ALOGE(" %s : Invalid state ",__func__);
        goto exit;
    }

    if (audio_extn_passthru_is_active()) {
        ALOGE(" %s : Pass through is already active", __func__);
        goto exit;
    }

    if (ka.state == STATE_ACTIVE) {
        ALOGV(" %s : Keep alive state is already Active",__func__ );
        goto exit;
    }

    /* Dont start keep_alive if any other PCM session is routed to HDMI*/
    list_for_each(node, &adev->usecase_list) {
         usecase = node_to_item(node, struct audio_usecase, list);
         if (usecase->type == PCM_PLAYBACK &&
                 usecase->devices & AUDIO_DEVICE_OUT_AUX_DIGITAL)
             goto exit;
    }

    ka.done = false;

    /*configure app type */
    snprintf(mixer_ctl_name, sizeof(mixer_ctl_name),
             "Audio Stream %d App Type Cfg",SILENCE_DEV_ID);

    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s", __func__,
              mixer_ctl_name);
        rc = -EINVAL;
        goto exit;
    }

    snd_device = SND_DEVICE_OUT_HDMI;
    acdb_dev_id = platform_get_snd_device_acdb_id(snd_device);
    if (acdb_dev_id < 0) {
        ALOGE("%s: Couldn't get the acdb dev id", __func__);
        rc = -EINVAL;
        goto exit;
    }

    sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
    app_type_cfg[len++] = platform_get_default_app_type(adev->platform);
    app_type_cfg[len++] = acdb_dev_id;
    app_type_cfg[len++] = sample_rate;

    ALOGI("%s:%d PLAYBACK app_type %d, acdb_dev_id %d, sample_rate %d",
          __func__, __LINE__,
          platform_get_default_app_type(adev->platform),
          acdb_dev_id, sample_rate);
    mixer_ctl_set_array(ctl, app_type_cfg, len);
    /*Configure HDMI Backend with default values, this as well
     *helps reconfigure HDMI backend after passthrough
     */
    set_mixer_control(adev->mixer, "HDMI RX Format", "LPCM");
    set_mixer_control(adev->mixer, "HDMI_RX SampleRate", "KHZ_48");
    set_mixer_control(adev->mixer, "HDMI_RX Channels", "Two");

    /*send calibration*/
    usecase = calloc(1, sizeof(struct audio_usecase));
    usecase->type = PCM_PLAYBACK;
    usecase->out_snd_device = SND_DEVICE_OUT_HDMI;

    platform_send_audio_calibration(adev->platform, usecase,
                platform_get_default_app_type(adev->platform), sample_rate);

    /*apply audio route */
    audio_route_apply_and_update_path(adev->audio_route, SILENCE_MIXER_PATH);

    if (open_silence_stream() == 0) {
        send_cmd_l(REQUEST_WRITE);
        while (ka.state != STATE_ACTIVE) {
            pthread_cond_wait(&ka.cond, &ka.lock);
        }
    }

exit:
    pthread_mutex_unlock(&ka.lock);
}

/* must be called with adev lock held */
void audio_extn_keep_alive_stop()
{
    struct audio_device * adev = (struct audio_device *)ka.userdata;

    pthread_mutex_lock(&ka.lock);

    if ((ka.state == STATE_DEINIT) || (ka.state == STATE_IDLE))
        goto exit;

    pthread_mutex_lock(&ka.sleep_lock);
    ka.done = true;
    pthread_cond_signal(&ka.wake_up_cond);
    pthread_mutex_unlock(&ka.sleep_lock);
    while (ka.state != STATE_IDLE) {
        pthread_cond_wait(&ka.cond, &ka.lock);
    }
    close_silence_stream();
    audio_route_reset_and_update_path(adev->audio_route, SILENCE_MIXER_PATH);

exit:
    pthread_mutex_unlock(&ka.lock);
}

bool audio_extn_keep_alive_is_active()
{
    return ka.state == STATE_ACTIVE;
}

int audio_extn_keep_alive_set_parameters(struct audio_device *adev __unused,
                                         struct str_parms *parms)
{
    char value[32];
    int ret;

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_DEVICE_CONNECT, value, sizeof(value));
    if (ret >= 0) {
        int val = atoi(value);
        if (val & AUDIO_DEVICE_OUT_AUX_DIGITAL) {
            if (!audio_extn_passthru_is_active()) {
                ALOGV("start keep alive");
                audio_extn_keep_alive_start();
            }
        }
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_DEVICE_DISCONNECT, value,
                            sizeof(value));
    if (ret >= 0) {
        int val = atoi(value);
        if (val & AUDIO_DEVICE_OUT_AUX_DIGITAL) {
            ALOGV("stop keep_alive");
            audio_extn_keep_alive_stop();
        }
    }
    return 0;
}


static void * keep_alive_loop(void * context __unused)
{
    struct keep_alive_cmd *cmd = NULL;
    struct listnode *item;
    uint8_t * silence = NULL;
    int32_t bytes = 0;
    struct timespec ts;

    while (true) {
        pthread_mutex_lock(&ka.lock);
        if (list_empty(&ka.cmd_list)) {
            pthread_cond_wait(&ka.cond, &ka.lock);
            pthread_mutex_unlock(&ka.lock);
            continue;
        }

        item = list_head(&ka.cmd_list);
        cmd = node_to_item(item, struct keep_alive_cmd, node);
        list_remove(item);

        if (cmd->req != REQUEST_WRITE) {
            free(cmd);
            pthread_mutex_unlock(&ka.lock);
            continue;
        }

        free(cmd);
        ka.state = STATE_ACTIVE;
        pthread_cond_signal(&ka.cond);
        pthread_mutex_unlock(&ka.lock);

        if (!silence) {
            /* 50 ms */
            bytes =
                (silence_config.rate * silence_config.channels * sizeof(int16_t)) / 20;
            silence = (uint8_t *)calloc(1, bytes);
        }

        while (!ka.done) {
            ALOGV("write %d bytes of silence", bytes);
            pcm_write(ka.pcm, (void *)silence, bytes);
            /* This thread does not have to write silence continuously.
             * Just something to keep the connection alive is sufficient.
             * Hence a short burst of silence periodically.
             */
            pthread_mutex_lock(&ka.sleep_lock);
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += SILENCE_INTERVAL;
            ts.tv_nsec = 0;

            if (!ka.done)
              pthread_cond_timedwait(&ka.wake_up_cond,
                            &ka.sleep_lock, &ts);

            pthread_mutex_unlock(&ka.sleep_lock);
        }
        pthread_mutex_lock(&ka.lock);
        ka.state = STATE_IDLE;
        pthread_cond_signal(&ka.cond);
        pthread_mutex_unlock(&ka.lock);
    }
    return 0;
}
