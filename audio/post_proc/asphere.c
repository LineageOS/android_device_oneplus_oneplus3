/* Copyright (c) 2015, The Linux Foundation. All rights reserved.
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
#define LOG_TAG "audio_pp_asphere"
/*#define LOG_NDEBUG 0*/

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <cutils/log.h>
#include <cutils/list.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>
#include <hardware/audio_effect.h>
#include "bundle.h"
#include "equalizer.h"
#include "bass_boost.h"
#include "virtualizer.h"
#include "reverb.h"
#include "asphere.h"

#define ASPHERE_MIXER_NAME  "MSM ASphere Set Param"

#define AUDIO_PARAMETER_KEY_ASPHERE_STATUS  "asphere_status"
#define AUDIO_PARAMETER_KEY_ASPHERE_ENABLE   "asphere_enable"
#define AUDIO_PARAMETER_KEY_ASPHERE_STRENGTH "asphere_strength"

#define AUDIO_ASPHERE_EVENT_NODE "/data/misc/audio_pp/event_node"

enum {
    ASPHERE_ACTIVE = 0,
    ASPHERE_SUSPENDED,
    ASPHERE_ERROR
};

struct asphere_module {
    bool enabled;
    int status;
    int strength;
    pthread_mutex_t lock;
    int init_status;
};

static struct asphere_module asphere;
pthread_once_t asphere_once = PTHREAD_ONCE_INIT;

static int asphere_create_app_notification_node(void)
{
    int fd;
    if ((fd = open(AUDIO_ASPHERE_EVENT_NODE, O_CREAT|O_TRUNC|O_WRONLY,
                            S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0) {
        ALOGE("creating notification node failed %d", errno);
        return -EINVAL;
    }
    chmod(AUDIO_ASPHERE_EVENT_NODE, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH);
    close(fd);
    ALOGD("%s: successfully created notification node %s",
                               __func__, AUDIO_ASPHERE_EVENT_NODE);
    return 0;
}

static int asphere_notify_app(void)
{
    int fd;
    if ((fd = open(AUDIO_ASPHERE_EVENT_NODE, O_TRUNC|O_WRONLY)) < 0) {
        ALOGE("opening notification node failed %d", errno);
        return -EINVAL;
    }
    close(fd);
    ALOGD("%s: successfully opened notification node", __func__);
    return 0;
}

static int asphere_get_values_from_mixer(void)
{
    int ret = 0, val[2] = {-1, -1};
    struct mixer_ctl *ctl = NULL;
    struct mixer *mixer = mixer_open(MIXER_CARD);
    if (mixer)
        ctl = mixer_get_ctl_by_name(mixer, ASPHERE_MIXER_NAME);
    if (!ctl) {
        ALOGE("%s: could not get ctl for mixer cmd - %s",
              __func__, ASPHERE_MIXER_NAME);
        return -EINVAL;
    }
    ret = mixer_ctl_get_array(ctl, val, sizeof(val)/sizeof(val[0]));
    if (!ret) {
        asphere.enabled = (val[0] == 0) ? false : true;
        asphere.strength = val[1];
    }
    ALOGD("%s: returned %d, enabled:%d, strength:%d",
          __func__, ret, val[0], val[1]);

    return ret;
}

static int asphere_set_values_to_mixer(void)
{
    int ret = 0, val[2] = {-1, -1};
    struct mixer_ctl *ctl = NULL;
    struct mixer *mixer = mixer_open(MIXER_CARD);
    if (mixer)
        ctl = mixer_get_ctl_by_name(mixer, ASPHERE_MIXER_NAME);
    if (!ctl) {
        ALOGE("%s: could not get ctl for mixer cmd - %s",
              __func__, ASPHERE_MIXER_NAME);
        return -EINVAL;
    }
    val[0] = ((asphere.status == ASPHERE_ACTIVE) && asphere.enabled) ? 1 : 0;
    val[1] = asphere.strength;

    ret = mixer_ctl_set_array(ctl, val, sizeof(val)/sizeof(val[0]));
    ALOGD("%s: returned %d, enabled:%d, strength:%d",
          __func__, ret, val[0], val[1]);

    return ret;
}

static void asphere_init_once() {
    ALOGD("%s", __func__);
    pthread_mutex_init(&asphere.lock, NULL);
    asphere.init_status = 1;
    asphere_get_values_from_mixer();
    asphere_create_app_notification_node();
}

static int asphere_init() {
    pthread_once(&asphere_once, asphere_init_once);
    return asphere.init_status;
}

void asphere_set_parameters(struct str_parms *parms)
{
    int ret = 0;
    bool enable = false;
    int strength = -1;
    char value[32] = {0};
    char propValue[PROPERTY_VALUE_MAX] = {0};
    bool set_enable = false, set_strength = false;

    if (!property_get("audio.pp.asphere.enabled", propValue, "false") ||
        (strncmp("true", propValue, 4) != 0)) {
        ALOGV("%s: property not set!!! not doing anything", __func__);
        return;
    }
    if (asphere_init() != 1) {
        ALOGW("%s: init check failed!!!", __func__);
        return;
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_ASPHERE_ENABLE,
                                                  value, sizeof(value));
    if (ret > 0) {
        enable = (atoi(value) == 1) ? true : false;
        set_enable = true;
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_ASPHERE_STRENGTH,
                                                   value, sizeof(value));
    if (ret > 0) {
        strength = atoi(value);
        if (strength >= 0 && strength <= 1000)
            set_strength = true;
    }

    if (set_enable || set_strength) {
        pthread_mutex_lock(&asphere.lock);
        asphere.enabled = set_enable ? enable : asphere.enabled;
        asphere.strength = set_strength ? strength : asphere.strength;
        ret = asphere_set_values_to_mixer();
        pthread_mutex_unlock(&asphere.lock);
        ALOGV("%s: exit ret %d", __func__, ret);
    }
}

void asphere_get_parameters(struct str_parms *query,
                                      struct str_parms *reply)
{
    char value[32] = {0};
    char propValue[PROPERTY_VALUE_MAX] = {0};
    int get_status, get_enable, get_strength, ret;

    if (!property_get("audio.pp.asphere.enabled", propValue, "false") ||
        (strncmp("true", propValue, 4) != 0)) {
        ALOGV("%s: property not set!!! not doing anything", __func__);
        return;
    }
    if (asphere_init() != 1) {
        ALOGW("%s: init check failed!!!", __func__);
        return;
    }

    ret = str_parms_get_str(query, AUDIO_PARAMETER_KEY_ASPHERE_STATUS,
                                                 value, sizeof(value));
    if (ret >= 0) {
        str_parms_add_int(reply, AUDIO_PARAMETER_KEY_ASPHERE_STATUS,
                                                     asphere.status);
    }

    ret = str_parms_get_str(query, AUDIO_PARAMETER_KEY_ASPHERE_ENABLE,
                                                 value, sizeof(value));
    if (ret >= 0) {
        str_parms_add_int(reply, AUDIO_PARAMETER_KEY_ASPHERE_ENABLE,
                                              asphere.enabled ? 1 : 0);
    }

    ret = str_parms_get_str(query, AUDIO_PARAMETER_KEY_ASPHERE_STRENGTH,
                                                  value, sizeof(value));
    if (ret >= 0) {
        str_parms_add_int(reply, AUDIO_PARAMETER_KEY_ASPHERE_STRENGTH,
                                                     asphere.strength);
    }
}

static bool effect_needs_asphere_concurrency_handling(effect_context_t *context)
{
    if (memcmp(&context->desc->type,
                &equalizer_descriptor.type, sizeof(effect_uuid_t)) == 0 ||
        memcmp(&context->desc->type,
                &bassboost_descriptor.type, sizeof(effect_uuid_t)) == 0 ||
        memcmp(&context->desc->type,
                &virtualizer_descriptor.type, sizeof(effect_uuid_t)) == 0 ||
        memcmp(&context->desc->type,
                &ins_preset_reverb_descriptor.type, sizeof(effect_uuid_t)) == 0 ||
        memcmp(&context->desc->type,
                &ins_env_reverb_descriptor.type, sizeof(effect_uuid_t)) == 0)
        return true;

    return false;
}

void handle_asphere_on_effect_enabled(bool enable,
                                      effect_context_t *context,
                                      struct listnode *created_effects)
{
    struct listnode *node;
    char propValue[PROPERTY_VALUE_MAX] = {0};

    ALOGV("%s: effect %0x", __func__, context->desc->type.timeLow);
    if (!property_get("audio.pp.asphere.enabled", propValue, "false") ||
        (strncmp("true", propValue, 4) != 0)) {
        ALOGV("%s: property not set!!! not doing anything", __func__);
        return;
    }
    if (asphere_init() != 1) {
        ALOGW("%s: init check failed!!!", __func__);
        return;
    }

    if (!effect_needs_asphere_concurrency_handling(context)) {
        ALOGV("%s: effect %0x, do not need concurrency handling",
                                 __func__, context->desc->type.timeLow);
        return;
    }

    list_for_each(node, created_effects) {
        effect_context_t *fx_ctxt = node_to_item(node,
                                                 effect_context_t,
                                                 effects_list_node);
        if (fx_ctxt != NULL &&
            effect_needs_asphere_concurrency_handling(fx_ctxt) == true &&
            fx_ctxt != context && effect_is_active(fx_ctxt) == true) {
            ALOGV("%s: found another effect %0x, skip processing %0x", __func__,
                      fx_ctxt->desc->type.timeLow, context->desc->type.timeLow);
            return;
        }
    }
    pthread_mutex_lock(&asphere.lock);
    asphere.status = enable ? ASPHERE_SUSPENDED : ASPHERE_ACTIVE;
    asphere_set_values_to_mixer();
    asphere_notify_app();
    pthread_mutex_unlock(&asphere.lock);
}
