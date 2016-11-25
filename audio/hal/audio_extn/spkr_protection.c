/*
 * Copyright (c) 2013 - 2016, The Linux Foundation. All rights reserved.
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

#define LOG_TAG "audio_hw_spkr_prot"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0

#include <errno.h>
#include <math.h>
#include <cutils/log.h>
#include <fcntl.h>
#include <dirent.h>
#include "audio_hw.h"
#include "platform.h"
#include "platform_api.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <math.h>
#include <cutils/properties.h>
#include "audio_extn.h"
#include <linux/msm_audio_calibration.h>

#ifdef SPKR_PROT_ENABLED

/*Range of spkr temparatures -30C to 80C*/
#define MIN_SPKR_TEMP_Q6 (-30 * (1 << 6))
#define MAX_SPKR_TEMP_Q6 (80 * (1 << 6))
#define VI_FEED_CHANNEL "VI_FEED_TX Channels"

/*Set safe temp value to 40C*/
#define SAFE_SPKR_TEMP 40
#define SAFE_SPKR_TEMP_Q6 (SAFE_SPKR_TEMP * (1 << 6))

/*Bongo Spkr temp range*/
#define TZ_TEMP_MIN_THRESHOLD    (5)
#define TZ_TEMP_MAX_THRESHOLD    (45)

/*Range of resistance values 2ohms to 40 ohms*/
#define MIN_RESISTANCE_SPKR_Q24 (2 * (1 << 24))
#define MAX_RESISTANCE_SPKR_Q24 (40 * (1 << 24))

/*Path where the calibration file will be stored*/
#define CALIB_FILE "/data/misc/audio/audio.cal"

/*Time between retries for calibartion or intial wait time
  after boot up*/
#define WAIT_TIME_SPKR_CALIB (60 * 1000 * 1000)

#define MIN_SPKR_IDLE_SEC (60 * 30)
#define WAKEUP_MIN_IDLE_CHECK 30

/*Once calibration is started sleep for 1 sec to allow
  the calibration to kick off*/
#define SLEEP_AFTER_CALIB_START (3000)

/*If calibration is in progress wait for 200 msec before querying
  for status again*/
#define WAIT_FOR_GET_CALIB_STATUS (200 * 1000)

/*Speaker states*/
#define SPKR_NOT_CALIBRATED -1
#define SPKR_CALIBRATED 1

/*Speaker processing state*/
#define SPKR_PROCESSING_IN_PROGRESS 1
#define SPKR_PROCESSING_IN_IDLE 0

/* In wsa analog mode vi feedback DAI supports at max 2 channels*/
#define WSA_ANALOG_MODE_CHANNELS 2

#define MAX_PATH             (256)
#define MAX_STR_SIZE         (1024)
#define THERMAL_SYSFS "/sys/class/thermal"
#define TZ_TYPE "/sys/class/thermal/thermal_zone%d/type"
#define TZ_WSA "/sys/class/thermal/thermal_zone%d/temp"

#define AUDIO_PARAMETER_KEY_SPKR_TZ_1     "spkr_1_tz_name"
#define AUDIO_PARAMETER_KEY_SPKR_TZ_2     "spkr_2_tz_name"

#define AUDIO_PARAMETER_KEY_FBSP_TRIGGER_SPKR_CAL   "trigger_spkr_cal"
#define AUDIO_PARAMETER_KEY_FBSP_GET_SPKR_CAL       "get_spkr_cal"
#define AUDIO_PARAMETER_KEY_FBSP_CFG_WAIT_TIME      "fbsp_cfg_wait_time"
#define AUDIO_PARAMETER_KEY_FBSP_CFG_FTM_TIME       "fbsp_cfg_ftm_time"
#define AUDIO_PARAMETER_KEY_FBSP_GET_FTM_PARAM      "get_ftm_param"

/*Modes of Speaker Protection*/
enum speaker_protection_mode {
    SPKR_PROTECTION_DISABLED = -1,
    SPKR_PROTECTION_MODE_PROCESSING = 0,
    SPKR_PROTECTION_MODE_CALIBRATE = 1,
};

struct speaker_prot_session {
    int spkr_prot_mode;
    int spkr_processing_state;
    int thermal_client_handle;
    pthread_mutex_t mutex_spkr_prot;
    pthread_t spkr_calibration_thread;
    pthread_mutex_t spkr_prot_thermalsync_mutex;
    pthread_cond_t spkr_prot_thermalsync;
    int cancel_spkr_calib;
    pthread_cond_t spkr_calib_cancel;
    pthread_mutex_t spkr_calib_cancelack_mutex;
    pthread_cond_t spkr_calibcancel_ack;
    pthread_t speaker_prot_threadid;
    void *thermal_handle;
    void *adev_handle;
    int spkr_prot_t0;
    struct pcm *pcm_rx;
    struct pcm *pcm_tx;
    int (*client_register_callback)
    (char *client_name, int (*callback)(int), void *data);
    void (*thermal_client_unregister_callback)(int handle);
    int (*thermal_client_request)(char *client_name, int req_data);
    bool spkr_prot_enable;
    bool spkr_in_use;
    struct timespec spkr_last_time_used;
    bool wsa_found;
    int spkr_1_tzn;
    int spkr_2_tzn;
    bool trigger_cal;
    pthread_mutex_t cal_wait_cond_mutex;
    pthread_cond_t cal_wait_condition;
};

static struct pcm_config pcm_config_skr_prot = {
    .channels = 4,
    .rate = 48000,
    .period_size = 256,
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = 0,
    .stop_threshold = INT_MAX,
    .avail_min = 0,
};

struct spkr_tz_names {
    char *spkr_1_name;
    char *spkr_2_name;
};

static struct speaker_prot_session handle;
static int vi_feed_no_channels;
static struct spkr_tz_names tz_names;

/*===========================================================================
FUNCTION get_tzn

Utility function to match a sensor name with thermal zone id.

ARGUMENTS
	sensor_name - name of sensor to match

RETURN VALUE
	Thermal zone id on success,
	-1 on failure.
===========================================================================*/
int get_tzn(const char *sensor_name)
{
    DIR *tdir = NULL;
    struct dirent *tdirent = NULL;
    int found = -1;
    int tzn = 0;
    char name[MAX_PATH] = {0};
    char cwd[MAX_PATH] = {0};

    if (!sensor_name)
        return found;

    if (!getcwd(cwd, sizeof(cwd)))
        return found;

    chdir(THERMAL_SYSFS); /* Change dir to read the entries. Doesnt work
                             otherwise */
    tdir = opendir(THERMAL_SYSFS);
    if (!tdir) {
        ALOGE("Unable to open %s\n", THERMAL_SYSFS);
        return found;
    }

    while ((tdirent = readdir(tdir))) {
        char buf[50];
        struct dirent *tzdirent;
        DIR *tzdir = NULL;

        tzdir = opendir(tdirent->d_name);
        if (!tzdir)
            continue;
        while ((tzdirent = readdir(tzdir))) {
            if (strcmp(tzdirent->d_name, "type"))
                continue;
            snprintf(name, MAX_PATH, TZ_TYPE, tzn);
            ALOGV("Opening %s\n", name);
            read_line_from_file(name, buf, sizeof(buf));
            if (strlen(buf) > 0)
                buf[strlen(buf) - 1] = '\0';
            if (!strcmp(buf, sensor_name)) {
                ALOGD(" spkr tz name found, %s\n", name);
                found = 1;
                break;
            }
            tzn++;
        }
        closedir(tzdir);
        if (found == 1)
            break;
    }
    closedir(tdir);
    chdir(cwd); /* Restore current working dir */
    if (found == 1) {
        found = tzn;
        ALOGE("Sensor %s found at tz: %d\n", sensor_name, tzn);
    }
    return found;
}

static void spkr_prot_set_spkrstatus(bool enable)
{
    if (enable)
       handle.spkr_in_use = true;
    else {
       handle.spkr_in_use = false;
       clock_gettime(CLOCK_BOOTTIME, &handle.spkr_last_time_used);
   }
}

void audio_extn_spkr_prot_calib_cancel(void *adev)
{
    pthread_t threadid;
    struct audio_usecase *uc_info;
    threadid = pthread_self();
    ALOGV("%s: Entry", __func__);
    if (pthread_equal(handle.speaker_prot_threadid, threadid) || !adev) {
        ALOGE("%s: Invalid params", __func__);
        return;
    }
    uc_info = get_usecase_from_list(adev, USECASE_AUDIO_SPKR_CALIB_RX);
    if (uc_info) {
            pthread_mutex_lock(&handle.mutex_spkr_prot);
            pthread_mutex_lock(&handle.spkr_calib_cancelack_mutex);
            handle.cancel_spkr_calib = 1;
            pthread_cond_signal(&handle.spkr_calib_cancel);
            pthread_mutex_unlock(&handle.mutex_spkr_prot);
            pthread_cond_wait(&handle.spkr_calibcancel_ack,
            &handle.spkr_calib_cancelack_mutex);
            pthread_mutex_unlock(&handle.spkr_calib_cancelack_mutex);
    }
    ALOGV("%s: Exit", __func__);
}

static bool is_speaker_in_use(unsigned long *sec)
{
    struct timespec temp;
    if (!sec) {
        ALOGE("%s: Invalid params", __func__);
        return true;
    }
    if (handle.spkr_in_use) {
        *sec = 0;
        handle.trigger_cal = false;
        return true;
    } else {
        clock_gettime(CLOCK_BOOTTIME, &temp);
        *sec = temp.tv_sec - handle.spkr_last_time_used.tv_sec;
        return false;
    }
}


static int get_spkr_prot_cal(int cal_fd,
				struct audio_cal_info_msm_spk_prot_status *status)
{
    int ret = 0;
    struct audio_cal_fb_spk_prot_status    cal_data;

    if (cal_fd < 0) {
        ALOGE("%s: Error: cal_fd = %d", __func__, cal_fd);
        ret = -EINVAL;
        goto done;
    }

    if (status == NULL) {
        ALOGE("%s: Error: status NULL", __func__);
        ret = -EINVAL;
        goto done;
    }

    cal_data.hdr.data_size = sizeof(cal_data);
    cal_data.hdr.version = VERSION_0_0;
    cal_data.hdr.cal_type = AFE_FB_SPKR_PROT_CAL_TYPE;
    cal_data.hdr.cal_type_size = sizeof(cal_data.cal_type);
    cal_data.cal_type.cal_hdr.version = VERSION_0_0;
    cal_data.cal_type.cal_hdr.buffer_number = 0;
    cal_data.cal_type.cal_data.mem_handle = -1;

    if (ioctl(cal_fd, AUDIO_GET_CALIBRATION, &cal_data)) {
        ALOGE("%s: Error: AUDIO_GET_CALIBRATION failed!",
            __func__);
        ret = -ENODEV;
        goto done;
    }

    status->r0[SP_V2_SPKR_1] = cal_data.cal_type.cal_info.r0[SP_V2_SPKR_1];
    status->r0[SP_V2_SPKR_2] = cal_data.cal_type.cal_info.r0[SP_V2_SPKR_2];
    status->status = cal_data.cal_type.cal_info.status;
done:
    return ret;
}

static int set_spkr_prot_cal(int cal_fd,
				struct audio_cal_info_spk_prot_cfg *protCfg)
{
    int ret = 0;
    struct audio_cal_fb_spk_prot_cfg    cal_data;
    char value[PROPERTY_VALUE_MAX];

    if (cal_fd < 0) {
        ALOGE("%s: Error: cal_fd = %d", __func__, cal_fd);
        ret = -EINVAL;
        goto done;
    }

    if (protCfg == NULL) {
        ALOGE("%s: Error: status NULL", __func__);
        ret = -EINVAL;
        goto done;
    }

    memset(&cal_data, 0, sizeof(cal_data));
    cal_data.hdr.data_size = sizeof(cal_data);
    cal_data.hdr.version = VERSION_0_0;
    cal_data.hdr.cal_type = AFE_FB_SPKR_PROT_CAL_TYPE;
    cal_data.hdr.cal_type_size = sizeof(cal_data.cal_type);
    cal_data.cal_type.cal_hdr.version = VERSION_0_0;
    cal_data.cal_type.cal_hdr.buffer_number = 0;
    cal_data.cal_type.cal_info.r0[SP_V2_SPKR_1] = protCfg->r0[SP_V2_SPKR_1];
    cal_data.cal_type.cal_info.r0[SP_V2_SPKR_2] = protCfg->r0[SP_V2_SPKR_2];
    cal_data.cal_type.cal_info.t0[SP_V2_SPKR_1] = protCfg->t0[SP_V2_SPKR_1];
    cal_data.cal_type.cal_info.t0[SP_V2_SPKR_2] = protCfg->t0[SP_V2_SPKR_2];
    cal_data.cal_type.cal_info.mode = protCfg->mode;
    property_get("persist.spkr.cal.duration", value, "0");
    if (atoi(value) > 0) {
        ALOGD("%s: quick calibration enabled", __func__);
        cal_data.cal_type.cal_info.quick_calib_flag = 1;
    } else {
        ALOGD("%s: quick calibration disabled", __func__);
        cal_data.cal_type.cal_info.quick_calib_flag = 0;
    }

    cal_data.cal_type.cal_data.mem_handle = -1;

    if (ioctl(cal_fd, AUDIO_SET_CALIBRATION, &cal_data)) {
        ALOGE("%s: Error: AUDIO_SET_CALIBRATION failed!",
            __func__);
        ret = -ENODEV;
        goto done;
    }
done:
    return ret;
}

static int vi_feed_get_channels(struct audio_device *adev)
{
    struct mixer_ctl *ctl;
    const char *mixer_ctl_name = VI_FEED_CHANNEL;
    int value;

    ALOGV("%s: entry", __func__);
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        goto error;
    }
    value = mixer_ctl_get_value(ctl, 0);
    if (value < 0)
        goto error;
    else
        return value+1;
error:
     return -EINVAL;
}

static int spkr_calibrate(int t0_spk_1, int t0_spk_2)
{
    struct audio_device *adev = handle.adev_handle;
    struct audio_cal_info_spk_prot_cfg protCfg;
    struct audio_cal_info_msm_spk_prot_status status;
    bool cleanup = false, disable_rx = false, disable_tx = false;
    int acdb_fd = -1;
    struct audio_usecase *uc_info_rx = NULL, *uc_info_tx = NULL;
    int32_t pcm_dev_rx_id = -1, pcm_dev_tx_id = -1;
    struct timespec ts;
    bool acquire_device = false;

    status.status = 0;
    if (!adev) {
        ALOGE("%s: Invalid params", __func__);
        return -EINVAL;
    }
    if (!list_empty(&adev->usecase_list)) {
        ALOGD("%s: Usecase present retry speaker protection", __func__);
        return -EAGAIN;
    }
    acdb_fd = open("/dev/msm_audio_cal",O_RDWR | O_NONBLOCK);
    if (acdb_fd < 0) {
        ALOGE("%s: spkr_prot_thread open msm_acdb failed", __func__);
        return -ENODEV;
    } else {
        protCfg.mode = MSM_SPKR_PROT_CALIBRATION_IN_PROGRESS;
        protCfg.t0[SP_V2_SPKR_1] = t0_spk_1;
        protCfg.t0[SP_V2_SPKR_2] = t0_spk_2;
        if (set_spkr_prot_cal(acdb_fd, &protCfg)) {
            ALOGE("%s: spkr_prot_thread set failed AUDIO_SET_SPEAKER_PROT",
            __func__);
            status.status = -ENODEV;
            goto exit;
        }
    }
    uc_info_rx = (struct audio_usecase *)calloc(1, sizeof(struct audio_usecase));
    if (!uc_info_rx) {
        return -ENOMEM;
    }
    uc_info_rx->id = USECASE_AUDIO_SPKR_CALIB_RX;
    uc_info_rx->type = PCM_PLAYBACK;
    uc_info_rx->in_snd_device = SND_DEVICE_NONE;
    uc_info_rx->stream.out = adev->primary_output;
    if (audio_extn_is_vbat_enabled())
        uc_info_rx->out_snd_device = SND_DEVICE_OUT_SPEAKER_PROTECTED_VBAT;
    else
        uc_info_rx->out_snd_device = SND_DEVICE_OUT_SPEAKER_PROTECTED;
    disable_rx = true;
    list_add_tail(&adev->usecase_list, &uc_info_rx->list);
    platform_check_and_set_codec_backend_cfg(adev, uc_info_rx,
                                             uc_info_rx->out_snd_device);
    if (audio_extn_is_vbat_enabled())
         enable_snd_device(adev, SND_DEVICE_OUT_SPEAKER_PROTECTED_VBAT);
    else
         enable_snd_device(adev, SND_DEVICE_OUT_SPEAKER_PROTECTED);
    enable_audio_route(adev, uc_info_rx);

    pcm_dev_rx_id = platform_get_pcm_device_id(uc_info_rx->id, PCM_PLAYBACK);
    ALOGV("%s: pcm device id %d", __func__, pcm_dev_rx_id);
    if (pcm_dev_rx_id < 0) {
        ALOGE("%s: Invalid pcm device for usecase (%d)",
              __func__, uc_info_rx->id);
        status.status = -ENODEV;
        goto exit;
    }
    handle.pcm_rx = handle.pcm_tx = NULL;
    handle.pcm_rx = pcm_open(adev->snd_card,
                             pcm_dev_rx_id,
                             PCM_OUT, &pcm_config_skr_prot);
    if (handle.pcm_rx && !pcm_is_ready(handle.pcm_rx)) {
        ALOGE("%s: %s", __func__, pcm_get_error(handle.pcm_rx));
        status.status = -EIO;
        goto exit;
    }
    uc_info_tx = (struct audio_usecase *)
    calloc(1, sizeof(struct audio_usecase));
    if (!uc_info_tx) {
        status.status = -ENOMEM;
        goto exit;
    }
    uc_info_tx->id = USECASE_AUDIO_SPKR_CALIB_TX;
    uc_info_tx->type = PCM_CAPTURE;
    uc_info_tx->in_snd_device = SND_DEVICE_IN_CAPTURE_VI_FEEDBACK;
    uc_info_tx->out_snd_device = SND_DEVICE_NONE;

    disable_tx = true;
    list_add_tail(&adev->usecase_list, &uc_info_tx->list);
    enable_snd_device(adev, SND_DEVICE_IN_CAPTURE_VI_FEEDBACK);
    enable_audio_route(adev, uc_info_tx);

    pcm_dev_tx_id = platform_get_pcm_device_id(uc_info_tx->id, PCM_CAPTURE);
    ALOGV("%s: pcm device id %d", __func__, pcm_dev_tx_id);
    if (pcm_dev_tx_id < 0) {
        ALOGE("%s: Invalid pcm device for usecase (%d)",
              __func__, uc_info_tx->id);
        status.status = -ENODEV;
        goto exit;
    }
    handle.pcm_tx = pcm_open(adev->snd_card,
                             pcm_dev_tx_id,
                             PCM_IN, &pcm_config_skr_prot);
    if (handle.pcm_tx && !pcm_is_ready(handle.pcm_tx)) {
        ALOGE("%s: %s", __func__, pcm_get_error(handle.pcm_tx));
        status.status = -EIO;
        goto exit;
    }
    if (pcm_start(handle.pcm_rx) < 0) {
        ALOGE("%s: pcm start for RX failed", __func__);
        status.status = -EINVAL;
        goto exit;
    }
    if (pcm_start(handle.pcm_tx) < 0) {
        ALOGE("%s: pcm start for TX failed", __func__);
        status.status = -EINVAL;
        goto exit;
    }
    cleanup = true;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += (SLEEP_AFTER_CALIB_START/1000);
    ts.tv_nsec = 0;
    pthread_mutex_lock(&handle.mutex_spkr_prot);
    pthread_mutex_unlock(&adev->lock);
    acquire_device = true;
    (void)pthread_cond_timedwait(&handle.spkr_calib_cancel,
        &handle.mutex_spkr_prot, &ts);
    ALOGD("%s: Speaker calibration done", __func__);
    cleanup = true;
    pthread_mutex_lock(&handle.spkr_calib_cancelack_mutex);
    if (handle.cancel_spkr_calib) {
        status.status = -EAGAIN;
        goto exit;
    }
    if (acdb_fd > 0) {
        status.status = -EINVAL;
        while (!get_spkr_prot_cal(acdb_fd, &status)) {
            /*sleep for 200 ms to check for status check*/
            if (!status.status) {
                ALOGD("%s: spkr_prot_thread calib Success R0 %d %d",
                 __func__, status.r0[SP_V2_SPKR_1], status.r0[SP_V2_SPKR_2]);
                FILE *fp;
                fp = fopen(CALIB_FILE,"wb");
                if (!fp) {
                    ALOGE("%s: spkr_prot_thread File open failed %s",
                    __func__, strerror(errno));
                    status.status = -ENODEV;
                } else {
                    int i;
                    /* HAL for speaker protection is always calibrating for stereo usecase*/
                    for (i = 0; i < vi_feed_no_channels; i++) {
                        fwrite(&status.r0[i], sizeof(status.r0[i]), 1, fp);
                        fwrite(&protCfg.t0[i], sizeof(protCfg.t0[i]), 1, fp);
                    }
                    fclose(fp);
                }
                break;
            } else if (status.status == -EAGAIN) {
                  ALOGV("%s: spkr_prot_thread try again", __func__);
                  usleep(WAIT_FOR_GET_CALIB_STATUS);
            } else {
                ALOGE("%s: spkr_prot_thread get failed status %d",
                __func__, status.status);
                break;
            }
        }
exit:
        if (handle.pcm_rx)
            pcm_close(handle.pcm_rx);
        handle.pcm_rx = NULL;
        if (handle.pcm_tx)
            pcm_close(handle.pcm_tx);
        handle.pcm_tx = NULL;
        /* Clear TX calibration to handset mic */
        if (disable_tx) {
            uc_info_tx->in_snd_device = SND_DEVICE_IN_HANDSET_MIC;
            uc_info_tx->out_snd_device = SND_DEVICE_NONE;
            platform_send_audio_calibration(adev->platform,
              uc_info_tx,
              platform_get_default_app_type(adev->platform), 8000);
        }
        if (!status.status) {
            protCfg.mode = MSM_SPKR_PROT_CALIBRATED;
            protCfg.r0[SP_V2_SPKR_1] = status.r0[SP_V2_SPKR_1];
            protCfg.r0[SP_V2_SPKR_2] = status.r0[SP_V2_SPKR_2];
            if (set_spkr_prot_cal(acdb_fd, &protCfg))
                ALOGE("%s: spkr_prot_thread disable calib mode", __func__);
            else
                handle.spkr_prot_mode = MSM_SPKR_PROT_CALIBRATED;
        } else {
            protCfg.mode = MSM_SPKR_PROT_NOT_CALIBRATED;
            handle.spkr_prot_mode = MSM_SPKR_PROT_NOT_CALIBRATED;
            if (set_spkr_prot_cal(acdb_fd, &protCfg))
                ALOGE("%s: spkr_prot_thread disable calib mode failed", __func__);
        }
        if (acdb_fd > 0)
            close(acdb_fd);

        if (!handle.cancel_spkr_calib && cleanup) {
            pthread_mutex_unlock(&handle.spkr_calib_cancelack_mutex);
            pthread_cond_wait(&handle.spkr_calib_cancel,
            &handle.mutex_spkr_prot);
            pthread_mutex_lock(&handle.spkr_calib_cancelack_mutex);
        }
        if (disable_rx) {
            list_remove(&uc_info_rx->list);
            if (audio_extn_is_vbat_enabled())
                disable_snd_device(adev, SND_DEVICE_OUT_SPEAKER_PROTECTED_VBAT);
            else
                disable_snd_device(adev, SND_DEVICE_OUT_SPEAKER_PROTECTED);
            disable_audio_route(adev, uc_info_rx);
        }
        if (disable_tx) {
            list_remove(&uc_info_tx->list);
            disable_snd_device(adev, SND_DEVICE_IN_CAPTURE_VI_FEEDBACK);
            disable_audio_route(adev, uc_info_tx);
        }
        if (uc_info_rx) free(uc_info_rx);
        if (uc_info_tx) free(uc_info_tx);
        if (cleanup) {
            if (handle.cancel_spkr_calib)
                pthread_cond_signal(&handle.spkr_calibcancel_ack);
            handle.cancel_spkr_calib = 0;
            pthread_mutex_unlock(&handle.spkr_calib_cancelack_mutex);
            pthread_mutex_unlock(&handle.mutex_spkr_prot);
        }
    }
    if (acquire_device)
        pthread_mutex_lock(&adev->lock);
    return status.status;
}

static void spkr_calibrate_wait()
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += WAKEUP_MIN_IDLE_CHECK;
    ts.tv_nsec = 0;
    pthread_mutex_lock(&handle.cal_wait_cond_mutex);
    pthread_cond_timedwait(&handle.cal_wait_condition,
                           &handle.cal_wait_cond_mutex, &ts);
    pthread_mutex_unlock(&handle.cal_wait_cond_mutex);
}

static void* spkr_calibration_thread()
{
    unsigned long sec = 0;
    int t0;
    int t0_spk_1 = 0;
    int t0_spk_2 = 0;
    bool goahead = false;
    struct audio_cal_info_spk_prot_cfg protCfg;
    FILE *fp;
    int acdb_fd, thermal_fd;
    struct audio_device *adev = handle.adev_handle;
    unsigned long min_idle_time = MIN_SPKR_IDLE_SEC;
    char value[PROPERTY_VALUE_MAX];
    char wsa_path[MAX_PATH] = {0};
    int spk_1_tzn, spk_2_tzn;
    char buf[32] = {0};
    int ret;

    /* If the value of this persist.spkr.cal.duration is 0
     * then it means it will take 30min to calibrate
     * and if the value is greater than zero then it would take
     * that much amount of time to calibrate.
     */
    property_get("persist.spkr.cal.duration", value, "0");
    if (atoi(value) > 0)
        min_idle_time = atoi(value);
    handle.speaker_prot_threadid = pthread_self();
    ALOGD("spkr_prot_thread enable prot Entry");
    acdb_fd = open("/dev/msm_audio_cal",O_RDWR | O_NONBLOCK);
    if (acdb_fd > 0) {
        /*Set processing mode with t0/r0*/
        protCfg.mode = MSM_SPKR_PROT_NOT_CALIBRATED;
        if (set_spkr_prot_cal(acdb_fd, &protCfg)) {
            ALOGE("%s: spkr_prot_thread enable prot failed", __func__);
            handle.spkr_prot_mode = MSM_SPKR_PROT_DISABLED;
            close(acdb_fd);
        } else
            handle.spkr_prot_mode = MSM_SPKR_PROT_NOT_CALIBRATED;
    } else {
        handle.spkr_prot_mode = MSM_SPKR_PROT_DISABLED;
        ALOGE("%s: Failed to open acdb node", __func__);
    }
    if (handle.spkr_prot_mode == MSM_SPKR_PROT_DISABLED) {
        ALOGD("%s: Speaker protection disabled", __func__);
        pthread_exit(0);
        return NULL;
    }

    fp = fopen(CALIB_FILE,"rb");
    if (fp) {
        int i;
        bool spkr_calibrated = true;
        for (i = 0; i < vi_feed_no_channels; i++) {
            fread(&protCfg.r0[i], sizeof(protCfg.r0[i]), 1, fp);
            fread(&protCfg.t0[i], sizeof(protCfg.t0[i]), 1, fp);
        }
        ALOGD("%s: spkr_prot_thread r0 value %d %d",
               __func__, protCfg.r0[SP_V2_SPKR_1], protCfg.r0[SP_V2_SPKR_2]);
        ALOGD("%s: spkr_prot_thread t0 value %d %d",
               __func__, protCfg.t0[SP_V2_SPKR_1], protCfg.t0[SP_V2_SPKR_2]);
        fclose(fp);
        /*Valid tempature range: -30C to 80C(in q6 format)
          Valid Resistance range: 2 ohms to 40 ohms(in q24 format)*/
        for (i = 0; i < vi_feed_no_channels; i++) {
            if (!((protCfg.t0[i] > MIN_SPKR_TEMP_Q6) && (protCfg.t0[i] < MAX_SPKR_TEMP_Q6)
                && (protCfg.r0[i] >= MIN_RESISTANCE_SPKR_Q24)
                && (protCfg.r0[i] < MAX_RESISTANCE_SPKR_Q24))) {
                spkr_calibrated = false;
                break;
            }
        }
        if (spkr_calibrated) {
            ALOGD("%s: Spkr calibrated", __func__);
            protCfg.mode = MSM_SPKR_PROT_CALIBRATED;
            if (set_spkr_prot_cal(acdb_fd, &protCfg)) {
                ALOGE("%s: enable prot failed", __func__);
                handle.spkr_prot_mode = MSM_SPKR_PROT_DISABLED;
            } else
                handle.spkr_prot_mode = MSM_SPKR_PROT_CALIBRATED;
            close(acdb_fd);
            pthread_exit(0);
            return NULL;
        }
        close(acdb_fd);
    }

    ALOGV("%s: start calibration", __func__);
    while (1) {
        if (handle.wsa_found) {
            spk_1_tzn = handle.spkr_1_tzn;
            spk_2_tzn = handle.spkr_2_tzn;
            goahead = false;
            pthread_mutex_lock(&adev->lock);
            if (is_speaker_in_use(&sec)) {
                ALOGV("%s: WSA Speaker in use retry calibration", __func__);
                pthread_mutex_unlock(&adev->lock);
                spkr_calibrate_wait();
                continue;
            } else {
                ALOGD("%s: wsa speaker idle %ld,minimum time %ld", __func__, sec, min_idle_time);
                if (!adev->primary_output ||
                    ((sec < min_idle_time) && !handle.trigger_cal)) {
                    pthread_mutex_unlock(&adev->lock);
                    spkr_calibrate_wait();
                    continue;
               }
               goahead = true;
           }
           if (!list_empty(&adev->usecase_list)) {
                ALOGD("%s: Usecase active re-try calibration", __func__);
                pthread_mutex_unlock(&adev->lock);
                spkr_calibrate_wait();
                continue;
           }
           if (goahead) {
               if (spk_1_tzn >= 0) {
                   snprintf(wsa_path, MAX_PATH, TZ_WSA, spk_1_tzn);
                   ALOGV("%s: wsa_path: %s\n", __func__, wsa_path);
                   thermal_fd = -1;
                   thermal_fd = open(wsa_path, O_RDONLY);
                   if (thermal_fd > 0) {
                       if ((ret = read(thermal_fd, buf, sizeof(buf))) >= 0)
                            t0_spk_1 = atoi(buf);
                       else
                           ALOGE("%s: read fail for %s err:%d\n", __func__, wsa_path, ret);
                       close(thermal_fd);
                   } else {
                       ALOGE("%s: fd for %s is NULL\n", __func__, wsa_path);
                   }
                   if (t0_spk_1 < TZ_TEMP_MIN_THRESHOLD ||
                       t0_spk_1 > TZ_TEMP_MAX_THRESHOLD) {
                       pthread_mutex_unlock(&adev->lock);
                       spkr_calibrate_wait();
                       continue;
                   }
                   ALOGD("%s: temp T0 for spkr1 %d\n", __func__, t0_spk_1);
                   /*Convert temp into q6 format*/
                   t0_spk_1 = (t0_spk_1 * (1 << 6));
               }
               if (spk_2_tzn >= 0) {
                   snprintf(wsa_path, MAX_PATH, TZ_WSA, spk_2_tzn);
                   ALOGV("%s: wsa_path: %s\n", __func__, wsa_path);
                   thermal_fd = open(wsa_path, O_RDONLY);
                   if (thermal_fd > 0) {
                       if ((ret = read(thermal_fd, buf, sizeof(buf))) >= 0)
                           t0_spk_2 = atoi(buf);
                       else
                           ALOGE("%s: read fail for %s err:%d\n", __func__, wsa_path, ret);
                       close(thermal_fd);
                   } else {
                       ALOGE("%s: fd for %s is NULL\n", __func__, wsa_path);
                   }
                   if (t0_spk_2 < TZ_TEMP_MIN_THRESHOLD ||
                       t0_spk_2 > TZ_TEMP_MAX_THRESHOLD) {
                       pthread_mutex_unlock(&adev->lock);
                       spkr_calibrate_wait();
                       continue;
                   }
                   ALOGD("%s: temp T0 for spkr2 %d\n", __func__, t0_spk_2);
                   /*Convert temp into q6 format*/
                   t0_spk_2 = (t0_spk_2 * (1 << 6));
               }
           }
           pthread_mutex_unlock(&adev->lock);
        } else if (!handle.thermal_client_request("spkr",1)) {
            ALOGD("%s: wait for callback from thermal daemon", __func__);
            pthread_mutex_lock(&handle.spkr_prot_thermalsync_mutex);
            pthread_cond_wait(&handle.spkr_prot_thermalsync,
            &handle.spkr_prot_thermalsync_mutex);
            /*Convert temp into q6 format*/
            t0 = (handle.spkr_prot_t0 * (1 << 6));
            pthread_mutex_unlock(&handle.spkr_prot_thermalsync_mutex);
            if (t0 < MIN_SPKR_TEMP_Q6 || t0 > MAX_SPKR_TEMP_Q6) {
                ALOGE("%s: Calibration temparature error %d", __func__,
                      handle.spkr_prot_t0);
                continue;
            }
            t0_spk_1 = t0;
            t0_spk_2 = t0;
            ALOGD("%s: Request t0 success value %d", __func__,
            handle.spkr_prot_t0);
        } else {
            ALOGE("%s: Request t0 failed", __func__);
            /*Assume safe value for temparature*/
            t0_spk_1 = SAFE_SPKR_TEMP_Q6;
            t0_spk_2 = SAFE_SPKR_TEMP_Q6;
        }
        goahead = false;
        pthread_mutex_lock(&adev->lock);
        if (is_speaker_in_use(&sec)) {
            ALOGV("%s: Speaker in use retry calibration", __func__);
            pthread_mutex_unlock(&adev->lock);
            spkr_calibrate_wait();
            continue;
        } else {
            if (!(sec > min_idle_time || handle.trigger_cal)) {
                pthread_mutex_unlock(&adev->lock);
                spkr_calibrate_wait();
                continue;
            }
            goahead = true;
        }
        if (!list_empty(&adev->usecase_list)) {
            ALOGD("%s: Usecase active re-try calibration", __func__);
            goahead = false;
            pthread_mutex_unlock(&adev->lock);
            spkr_calibrate_wait();
            continue;
        }
        if (goahead) {
                int status;
                status = spkr_calibrate(t0_spk_1, t0_spk_2);
                pthread_mutex_unlock(&adev->lock);
                if (status == -EAGAIN) {
                    ALOGE("%s: failed to calibrate try again %s",
                    __func__, strerror(status));
                    continue;
                } else {
                    ALOGE("%s: calibrate status %s", __func__, strerror(status));
                }
                ALOGD("%s: spkr_prot_thread end calibration", __func__);
                break;
        }
    }
    if (handle.thermal_client_handle)
        handle.thermal_client_unregister_callback(handle.thermal_client_handle);
    handle.thermal_client_handle = 0;
    if (handle.thermal_handle)
        dlclose(handle.thermal_handle);
    handle.thermal_handle = NULL;
    pthread_exit(0);
    return NULL;
}

static int thermal_client_callback(int temp)
{
    pthread_mutex_lock(&handle.spkr_prot_thermalsync_mutex);
    ALOGD("%s: spkr_prot set t0 %d and signal", __func__, temp);
    if (handle.spkr_prot_mode == MSM_SPKR_PROT_NOT_CALIBRATED)
        handle.spkr_prot_t0 = temp;
    pthread_cond_signal(&handle.spkr_prot_thermalsync);
    pthread_mutex_unlock(&handle.spkr_prot_thermalsync_mutex);
    return 0;
}

static bool is_wsa_present(void)
{
   ALOGD("%s: tz1: %s, tz2: %s", __func__,
          tz_names.spkr_1_name, tz_names.spkr_2_name);
   handle.spkr_1_tzn = get_tzn(tz_names.spkr_1_name);
   handle.spkr_2_tzn = get_tzn(tz_names.spkr_2_name);
   if ((handle.spkr_1_tzn >= 0) || (handle.spkr_2_tzn >= 0))
        handle.wsa_found = true;
   return handle.wsa_found;
}

void audio_extn_spkr_prot_set_parameters(struct str_parms *parms,
                                         char *value, int len)
{
    int err;

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_SPKR_TZ_1,
                            value, len);
    if (err >= 0) {
        tz_names.spkr_1_name = strdup(value);
        str_parms_del(parms, AUDIO_PARAMETER_KEY_SPKR_TZ_1);
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_SPKR_TZ_2,
                            value, len);
    if (err >= 0) {
        tz_names.spkr_2_name = strdup(value);
        str_parms_del(parms, AUDIO_PARAMETER_KEY_SPKR_TZ_2);
    }

    ALOGV("%s: tz1: %s, tz2: %s", __func__,
          tz_names.spkr_1_name, tz_names.spkr_2_name);
}

static int spkr_vi_channels(struct audio_device *adev)
{
    int vi_channels;

    vi_channels = vi_feed_get_channels(adev);
    ALOGD("%s: vi_channels %d", __func__, vi_channels);
    if (vi_channels < 0 || vi_channels > SP_V2_NUM_MAX_SPKRS) {
        /* limit the number of channels to SP_V2_NUM_MAX_SPKRS */
        vi_channels = SP_V2_NUM_MAX_SPKRS;
    }
    return vi_channels;
}

static void get_spkr_prot_thermal_cal(char *param)
{
    int i, status = 0;
    int r0[SP_V2_NUM_MAX_SPKRS] = {0}, t0[SP_V2_NUM_MAX_SPKRS] = {0};
    double dr0[SP_V2_NUM_MAX_SPKRS] = {0}, dt0[SP_V2_NUM_MAX_SPKRS] = {0};

    FILE *fp = fopen(CALIB_FILE,"rb");
    if (fp) {
        for (i = 0; i < vi_feed_no_channels; i++) {
            fread(&r0[i], sizeof(int), 1, fp);
            fread(&t0[i], sizeof(int), 1, fp);
            /* Convert from ADSP format to readable format */
            dr0[i] = ((double)r0[i])/(1 << 24);
            dt0[i] = ((double)t0[i])/(1 << 6);
        }
        ALOGV("%s: R0= %lf, %lf, T0= %lf, %lf",
              __func__, dr0[0], dr0[1], dt0[0], dt0[1]);
        fclose(fp);
    } else {
        ALOGE("%s: failed to open cal file\n", __func__);
        status = -EINVAL;
    }
    snprintf(param, MAX_STR_SIZE - strlen(param) - 1,
            "SpkrCalStatus: %d; R0: %lf, %lf; T0: %lf, %lf",
            status, dr0[SP_V2_SPKR_1], dr0[SP_V2_SPKR_2],
            dt0[SP_V2_SPKR_1], dt0[SP_V2_SPKR_2]);
    ALOGD("%s:: param = %s\n", __func__, param);

    return;
}

#ifdef MSM_SPKR_PROT_IN_FTM_MODE

static int set_spkr_prot_ftm_cfg(int wait_time, int ftm_time)
{
    int ret = 0;
    struct audio_cal_sp_th_vi_ftm_cfg th_cal_data;
    struct audio_cal_sp_ex_vi_ftm_cfg ex_cal_data;

    int cal_fd = open("/dev/msm_audio_cal",O_RDWR | O_NONBLOCK);
    if (cal_fd < 0) {
        ALOGE("%s: open msm_acdb failed", __func__);
        ret = -ENODEV;
        goto done;
    }

    memset(&th_cal_data, 0, sizeof(th_cal_data));
    th_cal_data.hdr.data_size = sizeof(th_cal_data);
    th_cal_data.hdr.version = VERSION_0_0;
    th_cal_data.hdr.cal_type = AFE_FB_SPKR_PROT_TH_VI_CAL_TYPE;
    th_cal_data.hdr.cal_type_size = sizeof(th_cal_data.cal_type);
    th_cal_data.cal_type.cal_hdr.version = VERSION_0_0;
    th_cal_data.cal_type.cal_hdr.buffer_number = 0;
    th_cal_data.cal_type.cal_info.wait_time[SP_V2_SPKR_1] = wait_time;
    th_cal_data.cal_type.cal_info.wait_time[SP_V2_SPKR_2] = wait_time;
    th_cal_data.cal_type.cal_info.ftm_time[SP_V2_SPKR_1] = ftm_time;
    th_cal_data.cal_type.cal_info.ftm_time[SP_V2_SPKR_2] = ftm_time;
    th_cal_data.cal_type.cal_info.mode = MSM_SPKR_PROT_IN_FTM_MODE; // FTM mode
    th_cal_data.cal_type.cal_data.mem_handle = -1;

    if (ioctl(cal_fd, AUDIO_SET_CALIBRATION, &th_cal_data))
        ALOGE("%s: failed to set TH VI FTM_CFG, errno = %d", __func__, errno);

    memset(&ex_cal_data, 0, sizeof(ex_cal_data));
    ex_cal_data.hdr.data_size = sizeof(ex_cal_data);
    ex_cal_data.hdr.version = VERSION_0_0;
    ex_cal_data.hdr.cal_type = AFE_FB_SPKR_PROT_EX_VI_CAL_TYPE;
    ex_cal_data.hdr.cal_type_size = sizeof(ex_cal_data.cal_type);
    ex_cal_data.cal_type.cal_hdr.version = VERSION_0_0;
    ex_cal_data.cal_type.cal_hdr.buffer_number = 0;
    ex_cal_data.cal_type.cal_info.wait_time[SP_V2_SPKR_1] = wait_time;
    ex_cal_data.cal_type.cal_info.wait_time[SP_V2_SPKR_2] = wait_time;
    ex_cal_data.cal_type.cal_info.ftm_time[SP_V2_SPKR_1] = ftm_time;
    ex_cal_data.cal_type.cal_info.ftm_time[SP_V2_SPKR_2] = ftm_time;
    ex_cal_data.cal_type.cal_info.mode = MSM_SPKR_PROT_IN_FTM_MODE; // FTM mode
    ex_cal_data.cal_type.cal_data.mem_handle = -1;

    if (ioctl(cal_fd, AUDIO_SET_CALIBRATION, &ex_cal_data))
        ALOGE("%s: failed to set EX VI FTM_CFG, ret = %d", __func__, errno);

    if (cal_fd > 0)
        close(cal_fd);
done:
    return ret;
}

static void get_spkr_prot_ftm_param(char *param)
{
    struct audio_cal_sp_th_vi_param th_vi_cal_data;
    struct audio_cal_sp_ex_vi_param ex_vi_cal_data;
    int i;
    int ftm_status[SP_V2_NUM_MAX_SPKRS];
    double rdc[SP_V2_NUM_MAX_SPKRS], temp[SP_V2_NUM_MAX_SPKRS];
    double f[SP_V2_NUM_MAX_SPKRS], r[SP_V2_NUM_MAX_SPKRS], q[SP_V2_NUM_MAX_SPKRS];

    int cal_fd = open("/dev/msm_audio_cal",O_RDWR | O_NONBLOCK);
    if (cal_fd < 0) {
        ALOGE("%s: open msm_acdb failed", __func__);
        goto done;
    }

    memset(&th_vi_cal_data, 0, sizeof(th_vi_cal_data));
    th_vi_cal_data.cal_type.cal_info.status[SP_V2_SPKR_1] = -EINVAL;
    th_vi_cal_data.cal_type.cal_info.status[SP_V2_SPKR_2] = -EINVAL;
    th_vi_cal_data.hdr.data_size = sizeof(th_vi_cal_data);
    th_vi_cal_data.hdr.version = VERSION_0_0;
    th_vi_cal_data.hdr.cal_type = AFE_FB_SPKR_PROT_TH_VI_CAL_TYPE;
    th_vi_cal_data.hdr.cal_type_size = sizeof(th_vi_cal_data.cal_type);
    th_vi_cal_data.cal_type.cal_hdr.version = VERSION_0_0;
    th_vi_cal_data.cal_type.cal_hdr.buffer_number = 0;
    th_vi_cal_data.cal_type.cal_data.mem_handle = -1;

    if (ioctl(cal_fd, AUDIO_GET_CALIBRATION, &th_vi_cal_data))
        ALOGE("%s: Error %d in getting th_vi_cal_data", __func__, errno);

    memset(&ex_vi_cal_data, 0, sizeof(ex_vi_cal_data));
    ex_vi_cal_data.cal_type.cal_info.status[SP_V2_SPKR_1] = -EINVAL;
    ex_vi_cal_data.cal_type.cal_info.status[SP_V2_SPKR_2] = -EINVAL;
    ex_vi_cal_data.hdr.data_size = sizeof(ex_vi_cal_data);
    ex_vi_cal_data.hdr.version = VERSION_0_0;
    ex_vi_cal_data.hdr.cal_type = AFE_FB_SPKR_PROT_EX_VI_CAL_TYPE;
    ex_vi_cal_data.hdr.cal_type_size = sizeof(ex_vi_cal_data.cal_type);
    ex_vi_cal_data.cal_type.cal_hdr.version = VERSION_0_0;
    ex_vi_cal_data.cal_type.cal_hdr.buffer_number = 0;
    ex_vi_cal_data.cal_type.cal_data.mem_handle = -1;

    if (ioctl(cal_fd, AUDIO_GET_CALIBRATION, &ex_vi_cal_data))
        ALOGE("%s: Error %d in getting ex_vi_cal_data", __func__, errno);

    for (i = 0; i < vi_feed_no_channels; i++) {
        /* Convert from ADSP format to readable format */
        rdc[i] = ((double)th_vi_cal_data.cal_type.cal_info.r_dc_q24[i])/(1<<24);
        temp[i] = ((double)th_vi_cal_data.cal_type.cal_info.temp_q22[i])/(1<<22);
        f[i] = ((double)ex_vi_cal_data.cal_type.cal_info.freq_q20[i])/(1<<20);
        r[i] = ((double)ex_vi_cal_data.cal_type.cal_info.resis_q24[i])/(1<<24);
        q[i] = ((double)ex_vi_cal_data.cal_type.cal_info.qmct_q24[i])/(1<<24);

        if (th_vi_cal_data.cal_type.cal_info.status[i] == 0 &&
            ex_vi_cal_data.cal_type.cal_info.status[i] == 0) {
            ftm_status[i] = 0;
        } else if (th_vi_cal_data.cal_type.cal_info.status[i] == -EAGAIN &&
                   ex_vi_cal_data.cal_type.cal_info.status[i] == -EAGAIN) {
            ftm_status[i] = -EAGAIN;
        } else {
            ftm_status[i] = -EINVAL;
        }
    }
    snprintf(param, MAX_STR_SIZE - strlen(param) - 1,
            "SpkrParamStatus: %d, %d; Rdc: %lf, %lf; Temp: %lf, %lf;"
            " Freq: %lf, %lf; Rect: %lf, %lf; Qmct: %lf, %lf",
            ftm_status[SP_V2_SPKR_1], ftm_status[SP_V2_SPKR_2],
            rdc[SP_V2_SPKR_1], rdc[SP_V2_SPKR_2], temp[SP_V2_SPKR_1],
            temp[SP_V2_SPKR_2], f[SP_V2_SPKR_1], f[SP_V2_SPKR_2],
            r[SP_V2_SPKR_1], r[SP_V2_SPKR_2], q[SP_V2_SPKR_1], q[SP_V2_SPKR_2]);
    ALOGD("%s:: param = %s\n", __func__, param);

    if (cal_fd > 0)
        close(cal_fd);
done:
    return;
}

#else

static void get_spkr_prot_ftm_param(char *param __unused)
{

    ALOGD("%s: not supported", __func__);
    return;
}

static int set_spkr_prot_ftm_cfg(int wait_time __unused, int ftm_time __unused)
{
    ALOGD("%s: not supported", __func__);
    return -ENOSYS;
}
#endif

static void spkr_calibrate_signal()
{
    pthread_mutex_lock(&handle.cal_wait_cond_mutex);
    pthread_cond_signal(&handle.cal_wait_condition);
    pthread_mutex_unlock(&handle.cal_wait_cond_mutex);
}

int audio_extn_fbsp_set_parameters(struct str_parms *parms)
{
    int ret= 0 , err;
    char *value = NULL;
    int len;
    char *test_r = NULL;
    char *cfg_str;
    int wait_time, ftm_time;
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
        ALOGE("[%s] failed to allocate memory",__func__);
        goto done;
    }
    if (!handle.spkr_prot_enable) {
        ALOGD("%s: Speaker protection disabled", __func__);
        goto done;
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_FBSP_TRIGGER_SPKR_CAL, value,
                            len);
    if (err >= 0) {
        str_parms_del(parms, AUDIO_PARAMETER_KEY_FBSP_TRIGGER_SPKR_CAL);
        if ((strcmp(value, "true") == 0) || (strcmp(value, "yes") == 0)) {
            handle.trigger_cal = true;
            spkr_calibrate_signal();
        }
        goto done;
    }

    /* Expected key value pair is in below format:
     * AUDIO_PARAM_FBSP_CFG_WAIT_TIME=waittime;AUDIO_PARAM_FBSP_CFG_FTM_TIME=ftmtime;
     * Parse waittime and ftmtime from it.
     */
    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_FBSP_CFG_WAIT_TIME,
                            value, len);
    if (err >= 0) {
        str_parms_del(parms, AUDIO_PARAMETER_KEY_FBSP_CFG_WAIT_TIME);
        cfg_str = strtok_r(value, ";", &test_r);
        if (cfg_str == NULL) {
            ALOGE("%s: incorrect wait time cfg_str", __func__);
            ret = -EINVAL;
            goto done;
        }
        wait_time = atoi(cfg_str);
        ALOGV(" %s: cfg_str = %s, wait_time = %d", __func__, cfg_str, wait_time);

        err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_FBSP_CFG_FTM_TIME,
                                value, len);
        if (err >= 0) {
            str_parms_del(parms, AUDIO_PARAMETER_KEY_FBSP_CFG_FTM_TIME);
            cfg_str = strtok_r(value, ";", &test_r);
            if (cfg_str == NULL) {
                ALOGE("%s: incorrect ftm time cfg_str", __func__);
                ret = -EINVAL;
                goto done;
            }
            ftm_time = atoi(cfg_str);
            ALOGV(" %s: cfg_str = %s, ftm_time = %d", __func__, cfg_str, ftm_time);

            ret = set_spkr_prot_ftm_cfg(wait_time, ftm_time);
            if (ret < 0) {
                ALOGE("%s: set_spkr_prot_ftm_cfg failed", __func__);
                goto done;
            }
        }
    }

done:
    ALOGV("%s: exit with code(%d)", __func__, ret);

    if(kv_pairs != NULL)
        free(kv_pairs);
    if(value != NULL)
        free(value);

    return ret;
}

int audio_extn_fbsp_get_parameters(struct str_parms *query,
                                   struct str_parms *reply)
{
    int err = 0;
    char value[MAX_STR_SIZE] = {0};

    if (!handle.spkr_prot_enable) {
        ALOGD("%s: Speaker protection disabled", __func__);
        return -EINVAL;
    }

    err = str_parms_get_str(query, AUDIO_PARAMETER_KEY_FBSP_GET_SPKR_CAL, value,
                                                          sizeof(value));
    if (err >= 0) {
        get_spkr_prot_thermal_cal(value);
        str_parms_add_str(reply, AUDIO_PARAMETER_KEY_FBSP_GET_SPKR_CAL, value);
    }
    err = str_parms_get_str(query, AUDIO_PARAMETER_KEY_FBSP_GET_FTM_PARAM, value,
                            sizeof(value));
    if (err >= 0) {
        get_spkr_prot_ftm_param(value);
        str_parms_add_str(reply, AUDIO_PARAMETER_KEY_FBSP_GET_FTM_PARAM, value);
    }
    return err;
}

void audio_extn_spkr_prot_init(void *adev)
{
    char value[PROPERTY_VALUE_MAX];
    ALOGD("%s: Initialize speaker protection module", __func__);
    memset(&handle, 0, sizeof(handle));
    if (!adev) {
        ALOGE("%s: Invalid params", __func__);
        return;
    }
    property_get("persist.speaker.prot.enable", value, "");
    handle.spkr_prot_enable = false;
    if (!strncmp("true", value, 4))
       handle.spkr_prot_enable = true;
    if (!handle.spkr_prot_enable) {
        ALOGD("%s: Speaker protection disabled", __func__);
        return;
    }
    handle.adev_handle = adev;
    handle.spkr_prot_mode = MSM_SPKR_PROT_DISABLED;
    handle.spkr_processing_state = SPKR_PROCESSING_IN_IDLE;
    handle.spkr_prot_t0 = -1;
    handle.trigger_cal = false;
    /* HAL for speaker protection is always calibrating for stereo usecase*/
    vi_feed_no_channels = spkr_vi_channels(adev);
    pthread_cond_init(&handle.cal_wait_condition, NULL);
    pthread_mutex_init(&handle.cal_wait_cond_mutex, NULL);

    if (is_wsa_present()) {
        if (platform_spkr_prot_is_wsa_analog_mode(adev) == 1) {
            ALOGD("%s: WSA analog mode", __func__);
            pcm_config_skr_prot.channels = WSA_ANALOG_MODE_CHANNELS;
        }
        pthread_cond_init(&handle.spkr_calib_cancel, NULL);
        pthread_cond_init(&handle.spkr_calibcancel_ack, NULL);
        pthread_mutex_init(&handle.mutex_spkr_prot, NULL);
        pthread_mutex_init(&handle.spkr_calib_cancelack_mutex, NULL);
        ALOGD("%s:WSA Create calibration thread", __func__);
        (void)pthread_create(&handle.spkr_calibration_thread,
        (const pthread_attr_t *) NULL, spkr_calibration_thread, &handle);
        return;
    } else {
        ALOGD("%s: WSA spkr calibration thread is not created", __func__);
    }
    pthread_cond_init(&handle.spkr_prot_thermalsync, NULL);
    pthread_cond_init(&handle.spkr_calib_cancel, NULL);
    pthread_cond_init(&handle.spkr_calibcancel_ack, NULL);
    pthread_mutex_init(&handle.mutex_spkr_prot, NULL);
    pthread_mutex_init(&handle.spkr_calib_cancelack_mutex, NULL);
    pthread_mutex_init(&handle.spkr_prot_thermalsync_mutex, NULL);
    handle.thermal_handle = dlopen("/vendor/lib/libthermalclient.so",
            RTLD_NOW);
    if (!handle.thermal_handle) {
        ALOGE("%s: DLOPEN for thermal client failed", __func__);
    } else {
        /*Query callback function symbol*/
        handle.client_register_callback =
       (int (*)(char *, int (*)(int),void *))
        dlsym(handle.thermal_handle, "thermal_client_register_callback");
        handle.thermal_client_unregister_callback =
        (void (*)(int) )
        dlsym(handle.thermal_handle, "thermal_client_unregister_callback");
        if (!handle.client_register_callback ||
            !handle.thermal_client_unregister_callback) {
            ALOGE("%s: DLSYM thermal_client_register_callback failed", __func__);
        } else {
            /*Register callback function*/
            handle.thermal_client_handle =
            handle.client_register_callback("spkr", thermal_client_callback, NULL);
            if (!handle.thermal_client_handle) {
                ALOGE("%s: client_register_callback failed", __func__);
            } else {
                ALOGD("%s: spkr_prot client_register_callback success", __func__);
                handle.thermal_client_request = (int (*)(char *, int))
                dlsym(handle.thermal_handle, "thermal_client_request");
            }
        }
    }
    if (handle.thermal_client_request) {
        ALOGD("%s: Create calibration thread", __func__);
        (void)pthread_create(&handle.spkr_calibration_thread,
        (const pthread_attr_t *) NULL, spkr_calibration_thread, &handle);
    } else {
        ALOGE("%s: thermal_client_request failed", __func__);
        if (handle.thermal_client_handle &&
            handle.thermal_client_unregister_callback)
            handle.thermal_client_unregister_callback(handle.thermal_client_handle);
        if (handle.thermal_handle)
            dlclose(handle.thermal_handle);
        handle.thermal_handle = NULL;
        handle.spkr_prot_enable = false;
    }

    if (handle.spkr_prot_enable) {
        char platform[PROPERTY_VALUE_MAX];
        property_get("ro.board.platform", platform, "");
        if (!strncmp("apq8084", platform, sizeof("apq8084"))) {
            platform_set_snd_device_backend(SND_DEVICE_OUT_VOICE_SPEAKER,
                                            "speaker-protected",
                                            "SLIMBUS_0_RX");
        }
    }
}

int audio_extn_spkr_prot_start_processing(snd_device_t snd_device)
{
    struct audio_usecase *uc_info_tx;
    struct audio_device *adev = handle.adev_handle;
    int32_t pcm_dev_tx_id = -1, ret = 0;
    bool disable_tx = false;

    ALOGV("%s: Entry", __func__);
    /* cancel speaker calibration */
    if (!adev) {
       ALOGE("%s: Invalid params", __func__);
       return -EINVAL;
    }
    snd_device = platform_get_spkr_prot_snd_device(snd_device);
    spkr_prot_set_spkrstatus(true);
    uc_info_tx = (struct audio_usecase *)calloc(1, sizeof(struct audio_usecase));
    if (!uc_info_tx) {
        return -ENOMEM;
    }
    ALOGD("%s: snd_device(%d: %s)", __func__, snd_device,
           platform_get_snd_device_name(snd_device));
    audio_route_apply_and_update_path(adev->audio_route,
           platform_get_snd_device_name(snd_device));

    pthread_mutex_lock(&handle.mutex_spkr_prot);
    if (handle.spkr_processing_state == SPKR_PROCESSING_IN_IDLE) {
        uc_info_tx->id = USECASE_AUDIO_SPKR_CALIB_TX;
        uc_info_tx->type = PCM_CAPTURE;
        uc_info_tx->in_snd_device = SND_DEVICE_IN_CAPTURE_VI_FEEDBACK;
        uc_info_tx->out_snd_device = SND_DEVICE_NONE;
        handle.pcm_tx = NULL;
        list_add_tail(&adev->usecase_list, &uc_info_tx->list);
        disable_tx = true;
        enable_snd_device(adev, SND_DEVICE_IN_CAPTURE_VI_FEEDBACK);
        enable_audio_route(adev, uc_info_tx);

        pcm_dev_tx_id = platform_get_pcm_device_id(uc_info_tx->id, PCM_CAPTURE);
        if (pcm_dev_tx_id < 0) {
            ALOGE("%s: Invalid pcm device for usecase (%d)",
                  __func__, uc_info_tx->id);
            ret = -ENODEV;
            goto exit;
        }
        handle.pcm_tx = pcm_open(adev->snd_card,
                                 pcm_dev_tx_id,
                                 PCM_IN, &pcm_config_skr_prot);
        if (handle.pcm_tx && !pcm_is_ready(handle.pcm_tx)) {
            ALOGE("%s: %s", __func__, pcm_get_error(handle.pcm_tx));
            ret = -EIO;
            goto exit;
        }
        if (pcm_start(handle.pcm_tx) < 0) {
            ALOGE("%s: pcm start for TX failed", __func__);
            ret = -EINVAL;
        }
    }

exit:
   /* Clear VI feedback cal and replace with handset MIC  */
    if (disable_tx) {
        uc_info_tx->in_snd_device = SND_DEVICE_IN_HANDSET_MIC;
        uc_info_tx->out_snd_device = SND_DEVICE_NONE;
        platform_send_audio_calibration(adev->platform,
          uc_info_tx,
          platform_get_default_app_type(adev->platform), 8000);
     }
     if (ret) {
        if (handle.pcm_tx)
            pcm_close(handle.pcm_tx);
        handle.pcm_tx = NULL;
        list_remove(&uc_info_tx->list);
        uc_info_tx->id = USECASE_AUDIO_SPKR_CALIB_TX;
        uc_info_tx->type = PCM_CAPTURE;
        uc_info_tx->in_snd_device = SND_DEVICE_IN_CAPTURE_VI_FEEDBACK;
        uc_info_tx->out_snd_device = SND_DEVICE_NONE;
        disable_snd_device(adev, SND_DEVICE_IN_CAPTURE_VI_FEEDBACK);
        disable_audio_route(adev, uc_info_tx);
        free(uc_info_tx);
    } else
        handle.spkr_processing_state = SPKR_PROCESSING_IN_PROGRESS;
    pthread_mutex_unlock(&handle.mutex_spkr_prot);
    ALOGV("%s: Exit", __func__);
    return ret;
}

void audio_extn_spkr_prot_stop_processing(snd_device_t snd_device)
{
    struct audio_usecase *uc_info_tx;
    struct audio_device *adev = handle.adev_handle;

    ALOGV("%s: Entry", __func__);
    snd_device = platform_get_spkr_prot_snd_device(snd_device);
    spkr_prot_set_spkrstatus(false);
    pthread_mutex_lock(&handle.mutex_spkr_prot);
    if (adev && handle.spkr_processing_state == SPKR_PROCESSING_IN_PROGRESS) {
        uc_info_tx = get_usecase_from_list(adev, USECASE_AUDIO_SPKR_CALIB_TX);
        if (handle.pcm_tx)
            pcm_close(handle.pcm_tx);
        handle.pcm_tx = NULL;
        disable_snd_device(adev, SND_DEVICE_IN_CAPTURE_VI_FEEDBACK);
        if (uc_info_tx) {
            list_remove(&uc_info_tx->list);
            disable_audio_route(adev, uc_info_tx);
            free(uc_info_tx);
        }
    }
    handle.spkr_processing_state = SPKR_PROCESSING_IN_IDLE;
    pthread_mutex_unlock(&handle.mutex_spkr_prot);
    if (adev)
        audio_route_reset_and_update_path(adev->audio_route,
                                      platform_get_snd_device_name(snd_device));
    ALOGV("%s: Exit", __func__);
}

bool audio_extn_spkr_prot_is_enabled()
{
    return handle.spkr_prot_enable;
}
#endif /*SPKR_PROT_ENABLED*/
