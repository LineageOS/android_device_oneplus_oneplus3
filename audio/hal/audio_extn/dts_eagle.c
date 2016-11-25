/*
 *  (C) 2014 DTS, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "audio_hw_dts_eagle"
/*#define LOG_NDEBUG 0*/

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <fcntl.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/str_parms.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sound/asound.h>
#include <sound/audio_effects.h>
#include <sound/devdep_params.h>
#include "audio_hw.h"
#include "platform.h"
#include "platform_api.h"

#ifdef DTS_EAGLE

#define AUDIO_PARAMETER_KEY_DTS_EAGLE   "DTS_EAGLE"
#define STATE_NOTIFY_FILE               "/data/misc/dts/stream"
#define FADE_NOTIFY_FILE                "/data/misc/dts/fade"
#define DTS_EAGLE_KEY                   "DTS_EAGLE"
#define DEVICE_NODE                     "/dev/snd/hwC0D3"
#define MAX_LENGTH_OF_INTEGER_IN_STRING 13
#define PARAM_GET_MAX_SIZE              512

struct dts_eagle_param_desc_alsa {
    int alsa_effect_ID;
    struct dts_eagle_param_desc d;
};

static struct dts_eagle_param_desc_alsa *fade_in_data = NULL;
static struct dts_eagle_param_desc_alsa *fade_out_data = NULL;
static int32_t mDevices = 0;
static int32_t mCurrDevice = 0;
static const char* DTS_EAGLE_STR = DTS_EAGLE_KEY;

static int do_DTS_Eagle_params_stream(const struct stream_out *out, struct dts_eagle_param_desc_alsa *t, bool get) {
    char mixer_string[128];
    char mixer_str_query[128];
    struct mixer_ctl *ctl;
    struct mixer_ctl *query_ctl;
    int pcm_device_id = platform_get_pcm_device_id(out->usecase, PCM_PLAYBACK);

    ALOGV("DTS_EAGLE_HAL (%s): enter", __func__);
    snprintf(mixer_string, sizeof(mixer_string), "%s %d", "Audio Effects Config", pcm_device_id);
    ctl = mixer_get_ctl_by_name(out->dev->mixer, mixer_string);
    if (!ctl) {
        ALOGE("DTS_EAGLE_HAL (%s): failed to open mixer %s", __func__, mixer_string);
    } else if (t) {
        int size = t->d.size + sizeof(struct dts_eagle_param_desc_alsa);
        ALOGD("DTS_EAGLE_HAL (%s): opened mixer %s", __func__, mixer_string);
        if (get) {
            ALOGD("DTS_EAGLE_HAL (%s): get request", __func__);
            snprintf(mixer_str_query, sizeof(mixer_str_query), "%s %d", "Query Audio Effect Param", pcm_device_id);
            query_ctl = mixer_get_ctl_by_name(out->dev->mixer, mixer_str_query);
            if (!query_ctl) {
                ALOGE("DTS_EAGLE_HAL (%s): failed to open mixer %s", __func__, mixer_str_query);
                return -EINVAL;
            }
            mixer_ctl_set_array(query_ctl, t, size);
            return mixer_ctl_get_array(ctl, t, size);
        }
        ALOGD("DTS_EAGLE_HAL (%s): set request", __func__);
        return mixer_ctl_set_array(ctl, t, size);
    } else {
        ALOGD("DTS_EAGLE_HAL (%s): parameter data NULL", __func__);
    }
    return -EINVAL;
}

static int do_DTS_Eagle_params(const struct audio_device *adev, struct dts_eagle_param_desc_alsa *t, bool get, const struct stream_out *out) {
    struct listnode *node;
    struct audio_usecase *usecase;
    int ret = 0, sent = 0, tret = 0;

    ALOGV("DTS_EAGLE_HAL (%s): enter", __func__);

    if (out) {
        /* if valid out stream is given, then send params to this stream only */
        tret = do_DTS_Eagle_params_stream(out, t, get);
        if (tret < 0)
            ret = tret;
        else
            sent = 1;
    } else {
        list_for_each(node, &adev->usecase_list) {
            usecase = node_to_item(node, struct audio_usecase, list);
            /* set/get eagle params for offload usecases only */
            if ((usecase->type == PCM_PLAYBACK) && is_offload_usecase(usecase->id)) {
                tret = do_DTS_Eagle_params_stream(usecase->stream.out, t, get);
                if (tret < 0)
                    ret = tret;
                else
                    sent = 1;
            }
        }
    }

    if (!sent) {
        int fd = open(DEVICE_NODE, O_RDWR);

        if (get) {
            ALOGD("DTS_EAGLE_HAL (%s): no stream opened, attempting to retrieve directly from cache", __func__);
            t->d.device &= ~DTS_EAGLE_FLAG_ALSA_GET;
        } else {
            ALOGD("DTS_EAGLE_HAL (%s): no stream opened, attempting to send directly to cache", __func__);
            t->d.device |= DTS_EAGLE_FLAG_IOCTL_JUSTSETCACHE;
        }

        if (fd > 0) {
            int cmd = get ? DTS_EAGLE_IOCTL_GET_PARAM : DTS_EAGLE_IOCTL_SET_PARAM;
            if (ioctl(fd, cmd, &t->d) < 0) {
                ALOGE("DTS_EAGLE_HAL (%s): error sending/getting param\n", __func__);
                ret = -EINVAL;
            } else {
                ALOGD("DTS_EAGLE_HAL (%s): sent/retrieved param\n", __func__);
            }
            close(fd);
        } else {
            ALOGE("DTS_EAGLE_HAL (%s): couldn't open device %s\n", __func__, DEVICE_NODE);
            ret = -EINVAL;
        }
    }
    return ret;
}

static void fade_node(bool need_data) {
    char prop[PROPERTY_VALUE_MAX];
    property_get("use.dts_eagle", prop, "0");
    if (strncmp("true", prop, sizeof("true")))
        return;
    int fd, n = 0;
    if ((fd = open(FADE_NOTIFY_FILE, O_TRUNC|O_WRONLY)) < 0) {
        ALOGV("No fade node, create one");
        fd = creat(FADE_NOTIFY_FILE, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        if (fd < 0) {
            ALOGE("DTS_EAGLE_HAL (%s): Creating fade notifier node failed", __func__);
            return;
        }
        chmod(FADE_NOTIFY_FILE, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH);
    }
    char *str = need_data ? "need" : "have";
    n = write(fd, str, strlen(str));
    close(fd);
    if (n > 0)
        ALOGI("DTS_EAGLE_HAL (%s): fade notifier node set to \"%s\", %i bytes written", __func__, str, n);
    else
        ALOGE("DTS_EAGLE_HAL (%s): error writing to fade notifier node", __func__);
}

int audio_extn_dts_eagle_fade(const struct audio_device *adev, bool fade_in, const struct stream_out *out) {
    char prop[PROPERTY_VALUE_MAX];

    ALOGV("DTS_EAGLE_HAL (%s): enter with fade %s requested", __func__, fade_in ? "in" : "out");

    property_get("use.dts_eagle", prop, "0");
    if (strncmp("true", prop, sizeof("true")))
        return 0;

    if (!fade_in_data || !fade_out_data)
        fade_node(true);

    if (fade_in) {
        if (fade_in_data)
            return do_DTS_Eagle_params(adev, fade_in_data, false, out);
    } else {
        if (fade_out_data)
            return do_DTS_Eagle_params(adev, fade_out_data, false, out);
    }
    return 0;
}

void audio_extn_dts_eagle_send_lic() {
    char prop[PROPERTY_VALUE_MAX] = {0};
    bool enabled;
    property_get("use.dts_eagle", prop, "0");
    enabled = !strncmp("true", prop, sizeof("true")) || atoi(prop);
    if (!enabled)
        return;
    int fd = open(DEVICE_NODE, O_RDWR);
    int index = 1;
    if (fd >= 0) {
        if (ioctl(fd, DTS_EAGLE_IOCTL_SEND_LICENSE, &index) < 0) {
            ALOGE("DTS_EAGLE_HAL: error sending license after adsp ssr");
        } else {
            ALOGD("DTS_EAGLE_HAL: sent license after adsp ssr");
        }
        close(fd);
    } else {
        ALOGE("DTS_EAGLE_HAL: error opening eagle");
    }
    return;
}

void audio_extn_dts_eagle_set_parameters(struct audio_device *adev, struct str_parms *parms) {
    int ret, val;
    char value[32] = { 0 }, prop[PROPERTY_VALUE_MAX];

    ALOGV("DTS_EAGLE_HAL (%s): enter", __func__);

    property_get("use.dts_eagle", prop, "0");
    if (strncmp("true", prop, sizeof("true")))
        return;

    memset(value, 0, sizeof(value));
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_DTS_EAGLE, value, sizeof(value));
    if (ret >= 0) {
        int *data = NULL, id, size, offset, count, dev, dts_found = 0, fade_in = 0;
        struct dts_eagle_param_desc_alsa *t2 = NULL, **t = &t2;

        ret = str_parms_get_str(parms, "fade", value, sizeof(value));
        if (ret >= 0) {
            fade_in = atoi(value);
            if (fade_in > 0) {
                t = (fade_in == 1) ? &fade_in_data : &fade_out_data;
            }
        }

        ret = str_parms_get_str(parms, "count", value, sizeof(value));
        if (ret >= 0) {
            count = atoi(value);
            if (count > 1) {
                int tmp_size = count * 32;
                char *tmp = malloc(tmp_size+1);
                data = malloc(sizeof(int) * count);
                ALOGV("DTS_EAGLE_HAL (%s): multi count param detected, count: %d", __func__, count);
                if (data && tmp) {
                    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_DTS_EAGLE, tmp, tmp_size);
                    if (ret >= 0) {
                        int idx = 0, tidx, tcnt = 0;
                        dts_found = 1;
                        do {
                            sscanf(&tmp[idx], "%i", &data[tcnt]);
                            tidx = strcspn(&tmp[idx], ",");
                            if (idx + tidx >= ret && tcnt < count-1) {
                                ALOGE("DTS_EAGLE_HAL (%s): malformed multi value string.", __func__);
                                dts_found = 0;
                                break;
                            }
                            ALOGD("DTS_EAGLE_HAL (%s): %i:%i (next %s)", __func__, tcnt, data[tcnt], &tmp[idx+tidx]);
                            idx += tidx + 1;
                            tidx = 0;
                            tcnt++;
                        } while (tcnt < count);
                    }
                } else {
                    ALOGE("DTS_EAGLE_HAL (%s): mem alloc for multi count param parse failed.", __func__);
                }
                free(tmp);
            }
        }

        if (!dts_found) {
            data = malloc(sizeof(int));
            if (data) {
                ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_DTS_EAGLE, value, sizeof(value));
                if (ret >= 0) {
                    *data = atoi(value);
                    dts_found = 1;
                    count = 1;
                } else {
                    ALOGE("DTS_EAGLE_HAL (%s): malformed value string.", __func__);
                }
            } else {
                ALOGE("DTS_EAGLE_HAL (%s): mem alloc for param parse failed.", __func__);
            }
        }

        if (dts_found) {
            dts_found = 0;
            ret = str_parms_get_str(parms, "id", value, sizeof(value));
            if (ret >= 0) {
                if (sscanf(value, "%x", &id) == 1) {
                    ret = str_parms_get_str(parms, "size", value, sizeof(value));
                    if (ret >= 0) {
                        size = atoi(value);
                        ret = str_parms_get_str(parms, "offset", value, sizeof(value));
                        if (ret >= 0) {
                            offset = atoi(value);
                            ret = str_parms_get_str(parms, "device", value, sizeof(value));
                            if (ret >= 0) {
                                dev = atoi(value);
                                dts_found = 1;
                            }
                        }
                    }
                }
            }
        }

        if (dts_found && count > 1 && size != (int)(count * sizeof(int))) {
            ALOGE("DTS_EAGLE_HAL (%s): size/count mismatch (size = %i bytes, count = %i integers / %lu bytes).", __func__, size, count, (long unsigned int)count*sizeof(int));
        } else if (dts_found) {
            ALOGI("DTS_EAGLE_HAL (%s): param detected: %s", __func__, str_parms_to_str(parms));
            if (!(*t))
                *t = (struct dts_eagle_param_desc_alsa*)malloc(sizeof(struct dts_eagle_param_desc_alsa) + size);
            if (*t) {
                (*t)->alsa_effect_ID = DTS_EAGLE_MODULE;
                (*t)->d.id = id;
                (*t)->d.size = size;
                (*t)->d.offset = offset;
                (*t)->d.device = dev;
                memcpy((void*)((char*)*t + sizeof(struct dts_eagle_param_desc_alsa)), data, size);
                ALOGD("DTS_EAGLE_HAL (%s): id: 0x%X, size: %d, offset: %d, device: %d", __func__,
                       (*t)->d.id, (*t)->d.size, (*t)->d.offset, (*t)->d.device);
                if (!fade_in) {
                    ret = do_DTS_Eagle_params(adev, *t, false, NULL);
                    if (ret < 0)
                        ALOGE("DTS_EAGLE_HAL (%s): failed setting params in kernel with error %i", __func__, ret);
                }
                free(t2);
            } else {
                ALOGE("DTS_EAGLE_HAL (%s): mem alloc for dsp structure failed.", __func__);
            }
        } else {
            ALOGE("DTS_EAGLE_HAL (%s): param detected but failed parse: %s", __func__, str_parms_to_str(parms));
        }
        free(data);

        if (fade_in > 0 && fade_in_data && fade_out_data)
            fade_node(false);
    }
    ALOGV("DTS_EAGLE_HAL (%s): exit", __func__);
}

int audio_extn_dts_eagle_get_parameters(const struct audio_device *adev,
                  struct str_parms *query, struct str_parms *reply) {
    int ret, val;
    char value[32] = { 0 }, prop[PROPERTY_VALUE_MAX];
    char params[PARAM_GET_MAX_SIZE];

    ALOGV("DTS_EAGLE_HAL (%s): enter", __func__);

    property_get("use.dts_eagle", prop, "0");
    if (strncmp("true", prop, sizeof("true")))
        return 0;

    ret = str_parms_get_str(query, AUDIO_PARAMETER_KEY_DTS_EAGLE, value, sizeof(value));
    if (ret >= 0) {
        int *data = NULL, id = 0, size = 0, offset = 0,
            count = 1, dev = 0, idx = 0, dts_found = 0, i = 0;
        const size_t chars_4_int = 16;
        ret = str_parms_get_str(query, "count", value, sizeof(value));
        if (ret >= 0) {
            count = atoi(value);
            if (count > 1) {
                ALOGV("DTS_EAGLE_HAL (%s): multi count param detected, count: %d", __func__, count);
            } else {
                count = 1;
            }
        }

        ret = str_parms_get_str(query, "id", value, sizeof(value));
        if (ret >= 0) {
            if (sscanf(value, "%x", &id) == 1) {
                ret = str_parms_get_str(query, "size", value, sizeof(value));
                if (ret >= 0) {
                    size = atoi(value);
                    ret = str_parms_get_str(query, "offset", value, sizeof(value));
                    if (ret >= 0) {
                        offset = atoi(value);
                        ret = str_parms_get_str(query, "device", value, sizeof(value));
                        if (ret >= 0) {
                            dev = atoi(value);
                            dts_found = 1;
                        }
                    }
                }
            }
        }

        if (dts_found) {
            ALOGI("DTS_EAGLE_HAL (%s): param (get) detected: %s", __func__, str_parms_to_str(query));
            struct dts_eagle_param_desc_alsa *t = (void *)params;
            if (t) {
                char buf[chars_4_int*count];
                t->alsa_effect_ID = DTS_EAGLE_MODULE;
                t->d.id = id;
                t->d.size = size;
                t->d.offset = offset;
                t->d.device = dev;
                ALOGV("DTS_EAGLE_HAL (%s): id (get): 0x%X, size: %d, offset: %d, device: %d", __func__,
                       t->d.id, t->d.size, t->d.offset, t->d.device & 0x7FFFFFFF);
                if ((sizeof(struct dts_eagle_param_desc_alsa) + size) > PARAM_GET_MAX_SIZE) {
                    ALOGE("%s: requested data too large", __func__);
                    return -1;
                }
                ret = do_DTS_Eagle_params(adev, t, true, NULL);
                if (ret >= 0) {
                    data = (int*)(params + sizeof(struct dts_eagle_param_desc_alsa));
                    for (i = 0; i < count; i++)
                        idx += snprintf(&buf[idx], chars_4_int, "%i,", data[i]);
                    buf[idx > 0 ? idx-1 : 0] = 0;
                    ALOGD("DTS_EAGLE_HAL (%s): get result: %s", __func__, buf);
                    str_parms_add_int(reply, "size", size);
                    str_parms_add_str(reply, AUDIO_PARAMETER_KEY_DTS_EAGLE, buf);
                    str_parms_add_int(reply, "count", count);
                    snprintf(value, sizeof(value), "0x%x", id);
                    str_parms_add_str(reply, "id", value);
                    str_parms_add_int(reply, "device", dev);
                    str_parms_add_int(reply, "offset", offset);
                    ALOGV("DTS_EAGLE_HAL (%s): reply: %s", __func__, str_parms_to_str(reply));
                } else {
                    ALOGE("DTS_EAGLE_HAL (%s): failed getting params from kernel with error %i", __func__, ret);
                    return -1;
                }
            } else {
                ALOGE("DTS_EAGLE_HAL (%s): mem alloc for (get) dsp structure failed.", __func__);
                return -1;
            }
        } else {
            ALOGE("DTS_EAGLE_HAL (%s): param (get) detected but failed parse: %s", __func__, str_parms_to_str(query));
            return -1;
        }
    }

    ALOGV("DTS_EAGLE_HAL (%s): exit", __func__);
    return 0;
}

void audio_extn_dts_create_state_notifier_node(int stream_out)
{
    char prop[PROPERTY_VALUE_MAX];
    char path[PATH_MAX];
    char value[MAX_LENGTH_OF_INTEGER_IN_STRING];
    int fd;
    property_get("use.dts_eagle", prop, "0");
    if ((!strncmp("true", prop, sizeof("true")) || atoi(prop))) {
        ALOGV("DTS_EAGLE_NODE_STREAM (%s): create_state_notifier_node - stream_out: %d", __func__, stream_out);
        strlcpy(path, STATE_NOTIFY_FILE, sizeof(path));
        snprintf(value, sizeof(value), "%d", stream_out);
        strlcat(path, value, sizeof(path));

        if ((fd=open(path, O_RDONLY)) < 0) {
            ALOGV("DTS_EAGLE_NODE_STREAM (%s): no file exists", __func__);
        } else {
            ALOGV("DTS_EAGLE_NODE_STREAM (%s): a file with the same name exists, removing it before creating it", __func__);
            close(fd);
            remove(path);
        }
        if ((fd=creat(path, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0) {
            ALOGE("DTS_EAGLE_NODE_STREAM (%s): opening state notifier node failed returned", __func__);
            return;
        }
        chmod(path, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH);
        ALOGV("DTS_EAGLE_NODE_STREAM (%s): opening state notifier node successful", __func__);
        close(fd);
        if (!fade_in_data || !fade_out_data)
            fade_node(true);
    }
}

void audio_extn_dts_notify_playback_state(int stream_out, int has_video, int sample_rate,
                           int channels, int is_playing) {
    char prop[PROPERTY_VALUE_MAX];
    char path[PATH_MAX];
    char value[MAX_LENGTH_OF_INTEGER_IN_STRING];
    char buf[1024];
    int fd;
    property_get("use.dts_eagle", prop, "0");
    if ((!strncmp("true", prop, sizeof("true")) || atoi(prop))) {
        ALOGV("DTS_EAGLE_NODE_STREAM (%s): notify_playback_state - is_playing: %d", __func__, is_playing);
        strlcpy(path, STATE_NOTIFY_FILE, sizeof(path));
        snprintf(value, sizeof(value), "%d", stream_out);
        strlcat(path, value, sizeof(path));
        if ((fd=open(path, O_TRUNC|O_WRONLY)) < 0) {
            ALOGE("DTS_EAGLE_NODE_STREAM (%s): open state notifier node failed", __func__);
        } else {
            snprintf(buf, sizeof(buf), "has_video=%d;sample_rate=%d;channel_mode=%d;playback_state=%d",
                     has_video, sample_rate, channels, is_playing);
            int n = write(fd, buf, strlen(buf));
            if (n > 0)
                ALOGV("DTS_EAGLE_NODE_STREAM (%s): write to state notifier node successful, bytes written: %d", __func__, n);
            else
                ALOGE("DTS_EAGLE_NODE_STREAM (%s): write state notifier node failed", __func__);
            close(fd);
        }
    }
}

void audio_extn_dts_remove_state_notifier_node(int stream_out)
{
    char prop[PROPERTY_VALUE_MAX];
    char path[PATH_MAX];
    char value[MAX_LENGTH_OF_INTEGER_IN_STRING];
    int fd;
    property_get("use.dts_eagle", prop, "0");
    if ((!strncmp("true", prop, sizeof("true")) || atoi(prop)) && (stream_out)) {
        ALOGV("DTS_EAGLE_NODE_STREAM (%s): remove_state_notifier_node: stream_out - %d", __func__, stream_out);
        strlcpy(path, STATE_NOTIFY_FILE, sizeof(path));
        snprintf(value, sizeof(value), "%d", stream_out);
        strlcat(path, value, sizeof(path));
        if ((fd=open(path, O_RDONLY)) < 0) {
            ALOGV("DTS_EAGLE_NODE_STREAM (%s): open state notifier node failed", __func__);
        } else {
            ALOGV("DTS_EAGLE_NODE_STREAM (%s): open state notifier node successful, removing the file", __func__);
            close(fd);
            remove(path);
        }
    }
}

#endif /* DTS_EAGLE end */
