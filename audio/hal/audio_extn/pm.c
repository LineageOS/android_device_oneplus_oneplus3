/* Copyright (c) 2014, The Linux Foundation. All rights reserved.
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
 *
 */

#define LOG_TAG "audio_hw_pm"
/*#define LOG_NDEBUG 0*/

#include "pm.h"
#include <cutils/log.h>

static s_audio_subsys audio_ss;

int audio_extn_pm_vote(void)
{
    int err, intfd, ret;
    FILE *fd;
    enum pm_event subsys_state;
    char halPropVal[PROPERTY_VALUE_MAX];
    bool prop_unload_image = false;
    bool pm_reg = false;
    bool pm_supp = false;

    platform_get_subsys_image_name((char *)&audio_ss.img_name);
    ALOGD("%s: register with peripheral manager for %s",__func__, audio_ss.img_name);
    ret = pm_client_register(audio_extn_pm_event_notifier,
                      &audio_ss,
                      audio_ss.img_name,
                      PM_CLIENT_NAME,
                      &subsys_state,
                      &audio_ss.pm_handle);
    if (ret == PM_RET_SUCCESS) {
        pm_reg = true;
        pm_supp = true;
        ALOGV("%s: registered with peripheral manager for %s",
                  __func__, audio_ss.img_name);
    } else if (ret == PM_RET_UNSUPPORTED) {
        pm_reg = true;
        pm_supp = false;
        ALOGV("%s: peripheral mgr unsupported for %s",
              __func__, audio_ss.img_name);
        return ret;
    } else {
       return ret;
    }
    if (pm_supp == true &&
       pm_reg == true) {
       ALOGD("%s: Voting for subsystem power up", __func__);
       pm_client_connect(audio_ss.pm_handle);

       if (property_get("sys.audio.init", halPropVal, NULL)) {
           prop_unload_image = !(strncmp("false", halPropVal, sizeof("false")));
       }
       /*
        * adsp-loader loads modem/adsp image at boot up to play boot tone,
        * before peripheral manager service is up. Once PM is up, vote to PM
        * and unload the image to give control to PM to load/unload image
        */
       if (prop_unload_image) {
           intfd = open(BOOT_IMG_SYSFS_PATH, O_WRONLY);
           if (intfd == -1) {
               ALOGE("failed to open fd in write mode, %d", errno);
           } else {
               ALOGD("%s: write to sysfs to unload image", __func__);
               err = write(intfd, UNLOAD_IMAGE, 1);
               close(intfd);
               property_set("sys.audio.init", "true");
          }
       }
    }
    return 0;
}

void audio_extn_pm_unvote(void)
{
    ALOGD("%s", __func__);
    if (audio_ss.pm_handle) {
        pm_client_disconnect(audio_ss.pm_handle);
        pm_client_unregister(audio_ss.pm_handle);
    }
}

void audio_extn_pm_set_parameters(struct str_parms *parms)
{
    int ret;
    char value[32];

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_DEV_SHUTDOWN, value, sizeof(value));
    if (ret >= 0) {
        if (strstr(value, "true")) {
            ALOGD("Device shutdown notification received, unregister with PM");
            audio_extn_pm_unvote();
        }
    }
}

void audio_extn_pm_event_notifier(void *client_data, enum pm_event event)
{
    pm_client_event_acknowledge(audio_ss.pm_handle, event);

    /* Closing and re-opening of session is done based on snd card status given
     * by AudioDaemon during SS offline/online (legacy code). Just return for now.
     */
    switch (event) {
    case EVENT_PERIPH_GOING_OFFLINE:
        ALOGV("%s: %s is going offline", __func__, audio_ss.img_name);
    break;

    case EVENT_PERIPH_IS_OFFLINE:
        ALOGV("%s: %s is offline", __func__, audio_ss.img_name);
    break;

    case EVENT_PERIPH_GOING_ONLINE:
        ALOGV("%s: %s is going online", __func__, audio_ss.img_name);
    break;

    case EVENT_PERIPH_IS_ONLINE:
        ALOGV("%s: %s is online", __func__, audio_ss.img_name);
    break;

    default:
        ALOGV("%s: invalid event received from PM", __func__);
    break;
    }
}
