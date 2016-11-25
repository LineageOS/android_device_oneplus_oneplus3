/*
 * Copyright (c) 2013-2016, The Linux Foundation. All rights reserved.
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
 *
 * This file was modified by DTS, Inc. The portions of the
 * code modified by DTS, Inc are copyrighted and
 * licensed separately, as follows:
 *
 * (C) 2014 DTS, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "audio_hw_extn"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0

#include <stdlib.h>
#include <errno.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <cutils/properties.h>
#include <cutils/log.h>

#include "audio_hw.h"
#include "audio_extn.h"
#include "platform.h"
#include "platform_api.h"
#include "edid.h"

#include "sound/compress_params.h"

#define MAX_SLEEP_RETRY 100
#define WIFI_INIT_WAIT_SLEEP 50

struct audio_extn_module {
    bool anc_enabled;
    bool aanc_enabled;
    bool custom_stereo_enabled;
    uint32_t proxy_channel_num;
    bool hpx_enabled;
    bool vbat_enabled;
    bool hifi_audio_enabled;
};

static struct audio_extn_module aextnmod = {
    .anc_enabled = 0,
    .aanc_enabled = 0,
    .custom_stereo_enabled = 0,
    .proxy_channel_num = 2,
    .hpx_enabled = 0,
    .vbat_enabled = 0,
    .hifi_audio_enabled = 0,
};

#define AUDIO_PARAMETER_KEY_ANC        "anc_enabled"
#define AUDIO_PARAMETER_KEY_WFD        "wfd_channel_cap"
#define AUDIO_PARAMETER_CAN_OPEN_PROXY "can_open_proxy"
#define AUDIO_PARAMETER_CUSTOM_STEREO  "stereo_as_dual_mono"
/* Query offload playback instances count */
#define AUDIO_PARAMETER_OFFLOAD_NUM_ACTIVE "offload_num_active"
#define AUDIO_PARAMETER_HPX            "HPX"

/*
* update sysfs node hdmi_audio_cb to enable notification acknowledge feature
* bit(5) set to 1 to enable this feature
* bit(4) set to 1 to enable acknowledgement
* this is done only once at the first connect event
*
* bit(0) set to 1 when HDMI cable is connected
* bit(0) set to 0 when HDMI cable is disconnected
* this is done when device switch happens by setting audioparamter
*/

#define HDMI_PLUG_STATUS_NOTIFY_ENABLE 0x30

static ssize_t update_sysfs_node(const char *path, const char *data, size_t len)
{
    ssize_t err = 0;
    int fd = -1;

    err = access(path, W_OK);
    if (!err) {
        fd = open(path, O_WRONLY);
        errno = 0;
        err = write(fd, data, len);
        if (err < 0) {
            err = -errno;
        }
        close(fd);
    } else {
        ALOGE("%s: Failed to access path: %s error: %s",
                __FUNCTION__, path, strerror(errno));
        err = -errno;
    }

    return err;
}

static int get_hdmi_sysfs_node_index()
{
    static int node_index = -1;
    char fbvalue[80] = {0};
    char fbpath[80] = {0};
    int i = 0;
    FILE *hdmi_fp = NULL;

    if(node_index >= 0) {
        //hdmi sysfs node will not change so we just need to get the index once.
        ALOGV("HDMI sysfs node is at fb%d", node_index);
        return node_index;
    }

    for(i = 0; i < 3; i++) {
        snprintf(fbpath, sizeof(fbpath),
                  "/sys/class/graphics/fb%d/msm_fb_type", i);
        hdmi_fp = fopen(fbpath, "r");
        if(hdmi_fp) {
            fread(fbvalue, sizeof(char), 80, hdmi_fp);
            if(strncmp(fbvalue, "dtv panel", strlen("dtv panel")) == 0) {
                node_index = i;
                ALOGV("HDMI is at fb%d",i);
                fclose(hdmi_fp);
                return node_index;
            }
            fclose(hdmi_fp);
        } else {
            ALOGE("Failed to open fb node %d",i);
        }
    }

    return -1;
}

static int update_hdmi_sysfs_node(int node_value)
{
    char hdmi_ack_path[80] = {0};
    char hdmi_ack_value[3] = {0};
    int index, ret = -1;

    index = get_hdmi_sysfs_node_index();

    if (index >= 0) {
        snprintf(hdmi_ack_value, sizeof(hdmi_ack_value), "%d", node_value);
        snprintf(hdmi_ack_path, sizeof(hdmi_ack_path),
                  "/sys/class/graphics/fb%d/hdmi_audio_cb", index);

        ret = update_sysfs_node(hdmi_ack_path, hdmi_ack_value,
                sizeof(hdmi_ack_value));

        ALOGI("update hdmi_audio_cb at fb[%d] to:[%d] %s",
            index, node_value, (ret >= 0) ? "success":"fail");
    }

    return ret;
}

static void check_and_set_hdmi_connection_status(struct str_parms *parms)
{
    char value[32] = {0};
    static bool is_hdmi_sysfs_node_init = false;

    if (str_parms_get_str(parms, "connect", value, sizeof(value)) >= 0
            && (atoi(value) & AUDIO_DEVICE_OUT_HDMI)) {
        //params = "connect=1024" for HDMI connection.
        if (is_hdmi_sysfs_node_init == false) {
            is_hdmi_sysfs_node_init = true;
            update_hdmi_sysfs_node(HDMI_PLUG_STATUS_NOTIFY_ENABLE);
        }
        update_hdmi_sysfs_node(1);
    } else if(str_parms_get_str(parms, "disconnect", value, sizeof(value)) >= 0
            && (atoi(value) & AUDIO_DEVICE_OUT_HDMI)){
        //params = "disconnect=1024" for HDMI disconnection.
        update_hdmi_sysfs_node(0);
    } else {
        // handle hdmi devices only
        return;
    }
}


#ifndef FM_POWER_OPT
#define audio_extn_fm_set_parameters(adev, parms) (0)
#else
void audio_extn_fm_set_parameters(struct audio_device *adev,
                                   struct str_parms *parms);
#endif
#ifndef HFP_ENABLED
#define audio_extn_hfp_set_parameters(adev, parms) (0)
#else
void audio_extn_hfp_set_parameters(struct audio_device *adev,
                                           struct str_parms *parms);
#endif

#ifndef SOURCE_TRACKING_ENABLED
#define audio_extn_source_track_set_parameters(adev, parms) (0)
#define audio_extn_source_track_get_parameters(adev, query, reply) (0)
#else
void audio_extn_source_track_set_parameters(struct audio_device *adev,
                                            struct str_parms *parms);
void audio_extn_source_track_get_parameters(const struct audio_device *adev,
                                            struct str_parms *query,
                                            struct str_parms *reply);
#endif

#ifndef CUSTOM_STEREO_ENABLED
#define audio_extn_customstereo_set_parameters(adev, parms)         (0)
#else
void audio_extn_customstereo_set_parameters(struct audio_device *adev,
                                           struct str_parms *parms)
{
    int ret = 0;
    char value[32]={0};
    bool custom_stereo_state = false;
    const char *mixer_ctl_name = "Set Custom Stereo OnOff";
    struct mixer_ctl *ctl;

    ALOGV("%s", __func__);
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_CUSTOM_STEREO, value,
                            sizeof(value));
    if (ret >= 0) {
        if (!strncmp("true", value, sizeof("true")) || atoi(value))
            custom_stereo_state = true;

        if (custom_stereo_state == aextnmod.custom_stereo_enabled)
            return;

        ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
        if (!ctl) {
            ALOGE("%s: Could not get ctl for mixer cmd - %s",
                  __func__, mixer_ctl_name);
            return;
        }
        if (mixer_ctl_set_value(ctl, 0, custom_stereo_state) < 0) {
            ALOGE("%s: Could not set custom stereo state %d",
                  __func__, custom_stereo_state);
            return;
        }
        aextnmod.custom_stereo_enabled = custom_stereo_state;
        ALOGV("%s: Setting custom stereo state success", __func__);
    }
}
#endif /* CUSTOM_STEREO_ENABLED */

#ifndef DTS_EAGLE
#define audio_extn_hpx_set_parameters(adev, parms)         (0)
#define audio_extn_hpx_get_parameters(query, reply)  (0)
#define audio_extn_check_and_set_dts_hpx_state(adev)       (0)
#else
void audio_extn_hpx_set_parameters(struct audio_device *adev,
                                   struct str_parms *parms)
{
    int ret = 0;
    char value[32]={0};
    char prop[PROPERTY_VALUE_MAX] = "false";
    bool hpx_state = false;
    const char *mixer_ctl_name = "Set HPX OnOff";
    struct mixer_ctl *ctl = NULL;
    ALOGV("%s", __func__);

    property_get("use.dts_eagle", prop, "0");
    if (strncmp("true", prop, sizeof("true")))
        return;

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_HPX, value,
                            sizeof(value));
    if (ret >= 0) {
        if (!strncmp("ON", value, sizeof("ON")))
            hpx_state = true;

        if (hpx_state == aextnmod.hpx_enabled)
            return;

        aextnmod.hpx_enabled = hpx_state;
        /* set HPX state on stream pp */
        if (adev->offload_effects_set_hpx_state != NULL)
            adev->offload_effects_set_hpx_state(hpx_state);

        audio_extn_dts_eagle_fade(adev, aextnmod.hpx_enabled, NULL);
        /* set HPX state on device pp */
        ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
        if (ctl)
            mixer_ctl_set_value(ctl, 0, aextnmod.hpx_enabled);
    }
}

static int audio_extn_hpx_get_parameters(struct str_parms *query,
                                       struct str_parms *reply)
{
    int ret;
    char value[32]={0};

    ALOGV("%s: hpx %d", __func__, aextnmod.hpx_enabled);
    ret = str_parms_get_str(query, AUDIO_PARAMETER_HPX, value,
                            sizeof(value));
    if (ret >= 0) {
        if (aextnmod.hpx_enabled)
            str_parms_add_str(reply, AUDIO_PARAMETER_HPX, "ON");
        else
            str_parms_add_str(reply, AUDIO_PARAMETER_HPX, "OFF");
    }
    return ret;
}

void audio_extn_check_and_set_dts_hpx_state(const struct audio_device *adev)
{
    char prop[PROPERTY_VALUE_MAX];
    property_get("use.dts_eagle", prop, "0");
    if (strncmp("true", prop, sizeof("true")))
        return;
    if (adev->offload_effects_set_hpx_state)
        adev->offload_effects_set_hpx_state(aextnmod.hpx_enabled);
}
#endif

#ifdef HIFI_AUDIO_ENABLED
bool audio_extn_is_hifi_audio_enabled(void)
{
    ALOGV("%s: status: %d", __func__, aextnmod.hifi_audio_enabled);
    return (aextnmod.hifi_audio_enabled ? true: false);
}

bool audio_extn_is_hifi_audio_supported(void)
{
    /*
     * for internal codec, check for hifiaudio property to enable hifi audio
     */
    if (property_get_bool("persist.audio.hifi.int_codec", false))
    {
        ALOGD("%s: hifi audio supported on internal codec", __func__);
        aextnmod.hifi_audio_enabled = 1;
    }

    return (aextnmod.hifi_audio_enabled ? true: false);
}
#endif

#ifdef VBAT_MONITOR_ENABLED
bool audio_extn_is_vbat_enabled(void)
{
    ALOGD("%s: status: %d", __func__, aextnmod.vbat_enabled);
    return (aextnmod.vbat_enabled ? true: false);
}

bool audio_extn_can_use_vbat(void)
{
    char prop_vbat_enabled[PROPERTY_VALUE_MAX] = "false";

    property_get("persist.audio.vbat.enabled", prop_vbat_enabled, "0");
    if (!strncmp("true", prop_vbat_enabled, 4)) {
        aextnmod.vbat_enabled = 1;
    }

    ALOGD("%s: vbat.enabled property is set to %s", __func__, prop_vbat_enabled);
    return (aextnmod.vbat_enabled ? true: false);
}
#endif

#ifndef ANC_HEADSET_ENABLED
#define audio_extn_set_anc_parameters(adev, parms)       (0)
#else
bool audio_extn_get_anc_enabled(void)
{
    ALOGD("%s: anc_enabled:%d", __func__, aextnmod.anc_enabled);
    return (aextnmod.anc_enabled ? true: false);
}

bool audio_extn_should_use_handset_anc(int in_channels)
{
    char prop_aanc[PROPERTY_VALUE_MAX] = "false";

    property_get("persist.aanc.enable", prop_aanc, "0");
    if (!strncmp("true", prop_aanc, 4)) {
        ALOGD("%s: AANC enabled in the property", __func__);
        aextnmod.aanc_enabled = 1;
    }

    return (aextnmod.aanc_enabled && aextnmod.anc_enabled
            && (in_channels == 1));
}

bool audio_extn_should_use_fb_anc(void)
{
  char prop_anc[PROPERTY_VALUE_MAX] = "feedforward";

  property_get("persist.headset.anc.type", prop_anc, "0");
  if (!strncmp("feedback", prop_anc, sizeof("feedback"))) {
    ALOGD("%s: FB ANC headset type enabled\n", __func__);
    return true;
  }
  return false;
}

void audio_extn_set_anc_parameters(struct audio_device *adev,
                                   struct str_parms *parms)
{
    int ret;
    char value[32] ={0};
    struct listnode *node;
    struct audio_usecase *usecase;
    struct str_parms *query_44_1;
    struct str_parms *reply_44_1;
    struct str_parms *parms_disable_44_1;

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_ANC, value,
                            sizeof(value));
    if (ret >= 0) {
        if (strcmp(value, "true") == 0)
            aextnmod.anc_enabled = true;
        else
            aextnmod.anc_enabled = false;

        /* Store current 44.1 configuration and disable it temporarily before
         * changing ANC state.
         * Since 44.1 playback is not allowed with anc on.
         * If ANC switch is done when 44.1 is active three devices would need
         * sequencing 1. "headphones-44.1", 2. "headphones-anc" and
         * 3. "headphones".
         * Note: Enable/diable of anc would affect other two device's state.
         */
        query_44_1 = str_parms_create_str(AUDIO_PARAMETER_KEY_NATIVE_AUDIO);
        reply_44_1 = str_parms_create();
        if (!query_44_1 || !reply_44_1) {
            if (query_44_1) {
                str_parms_destroy(query_44_1);
            }
            if (reply_44_1) {
                str_parms_destroy(reply_44_1);
            }

            ALOGE("%s: param creation failed", __func__);
            return;
        }

        platform_get_parameters(adev->platform, query_44_1, reply_44_1);

        parms_disable_44_1 = str_parms_create();
        if (!parms_disable_44_1) {
            str_parms_destroy(query_44_1);
            str_parms_destroy(reply_44_1);
            ALOGE("%s: param creation failed for parms_disable_44_1", __func__);
            return;
        }

        str_parms_add_str(parms_disable_44_1, AUDIO_PARAMETER_KEY_NATIVE_AUDIO, "false");
        platform_set_parameters(adev->platform, parms_disable_44_1);
        str_parms_destroy(parms_disable_44_1);

        // Refresh device selection for anc playback
        list_for_each(node, &adev->usecase_list) {
            usecase = node_to_item(node, struct audio_usecase, list);
            if (usecase->type != PCM_CAPTURE) {
                if (usecase->stream.out->devices == \
                    AUDIO_DEVICE_OUT_WIRED_HEADPHONE ||
                    usecase->stream.out->devices ==  \
                    AUDIO_DEVICE_OUT_WIRED_HEADSET ||
                    usecase->stream.out->devices ==  \
                    AUDIO_DEVICE_OUT_EARPIECE) {
                        select_devices(adev, usecase->id);
                        ALOGV("%s: switching device completed", __func__);
                        break;
                }
            }
        }

        // Restore 44.1 configuration on top of updated anc state
        platform_set_parameters(adev->platform, reply_44_1);
        str_parms_destroy(query_44_1);
        str_parms_destroy(reply_44_1);
    }

    ALOGD("%s: anc_enabled:%d", __func__, aextnmod.anc_enabled);
}
#endif /* ANC_HEADSET_ENABLED */

#ifndef FLUENCE_ENABLED
#define audio_extn_set_fluence_parameters(adev, parms) (0)
#define audio_extn_get_fluence_parameters(adev, query, reply) (0)
#else
void audio_extn_set_fluence_parameters(struct audio_device *adev,
                                            struct str_parms *parms)
{
    int ret = 0, err;
    char value[32];
    struct listnode *node;
    struct audio_usecase *usecase;

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_FLUENCE,
                                 value, sizeof(value));
    ALOGV_IF(err >= 0, "%s: Set Fluence Type to %s", __func__, value);
    if (err >= 0) {
        ret = platform_set_fluence_type(adev->platform, value);
        if (ret != 0) {
            ALOGE("platform_set_fluence_type returned error: %d", ret);
        } else {
            /*
             *If the fluence is manually set/reset, devices
             *need to get updated for all the usecases
             *i.e. audio and voice.
             */
             list_for_each(node, &adev->usecase_list) {
                 usecase = node_to_item(node, struct audio_usecase, list);
                 select_devices(adev, usecase->id);
             }
        }
    }
}

int audio_extn_get_fluence_parameters(const struct audio_device *adev,
                       struct str_parms *query, struct str_parms *reply)
{
    int ret = 0, err;
    char value[256] = {0};

    err = str_parms_get_str(query, AUDIO_PARAMETER_KEY_FLUENCE, value,
                                                          sizeof(value));
    if (err >= 0) {
        ret = platform_get_fluence_type(adev->platform, value, sizeof(value));
        if (ret >= 0) {
            ALOGV("%s: Fluence Type is %s", __func__, value);
            str_parms_add_str(reply, AUDIO_PARAMETER_KEY_FLUENCE, value);
        } else
            goto done;
    }
done:
    return ret;
}
#endif /* FLUENCE_ENABLED */

#ifndef AFE_PROXY_ENABLED
#define audio_extn_set_afe_proxy_parameters(adev, parms)  (0)
#define audio_extn_get_afe_proxy_parameters(adev, query, reply) (0)
#else
static int32_t afe_proxy_set_channel_mapping(struct audio_device *adev,
                                                     int channel_count)
{
    struct mixer_ctl *ctl;
    const char *mixer_ctl_name = "Playback Device Channel Map";
    int set_values[8] = {0};
    int ret;
    ALOGV("%s channel_count:%d",__func__, channel_count);

    switch (channel_count) {
    case 2:
        set_values[0] = PCM_CHANNEL_FL;
        set_values[1] = PCM_CHANNEL_FR;
        break;
    case 6:
        set_values[0] = PCM_CHANNEL_FL;
        set_values[1] = PCM_CHANNEL_FR;
        set_values[2] = PCM_CHANNEL_FC;
        set_values[3] = PCM_CHANNEL_LFE;
        set_values[4] = PCM_CHANNEL_LS;
        set_values[5] = PCM_CHANNEL_RS;
        break;
    case 8:
        set_values[0] = PCM_CHANNEL_FL;
        set_values[1] = PCM_CHANNEL_FR;
        set_values[2] = PCM_CHANNEL_FC;
        set_values[3] = PCM_CHANNEL_LFE;
        set_values[4] = PCM_CHANNEL_LS;
        set_values[5] = PCM_CHANNEL_RS;
        set_values[6] = PCM_CHANNEL_LB;
        set_values[7] = PCM_CHANNEL_RB;
        break;
    default:
        ALOGE("unsupported channels(%d) for setting channel map",
                                                    channel_count);
        return -EINVAL;
    }

    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        return -EINVAL;
    }
    ALOGV("AFE: set mapping(%d %d %d %d %d %d %d %d) for channel:%d",
        set_values[0], set_values[1], set_values[2], set_values[3], set_values[4],
        set_values[5], set_values[6], set_values[7], channel_count);
    ret = mixer_ctl_set_array(ctl, set_values, channel_count);
    return ret;
}

int32_t audio_extn_set_afe_proxy_channel_mixer(struct audio_device *adev,
                                    int channel_count)
{
    int32_t ret = 0;
    const char *channel_cnt_str = NULL;
    struct mixer_ctl *ctl = NULL;
    const char *mixer_ctl_name = "PROXY_RX Channels";

    ALOGD("%s: entry", __func__);
    /* use the existing channel count set by hardware params to
    configure the back end for stereo as usb/a2dp would be
    stereo by default */
    ALOGD("%s: channels = %d", __func__, channel_count);
    switch (channel_count) {
    case 8: channel_cnt_str = "Eight"; break;
    case 7: channel_cnt_str = "Seven"; break;
    case 6: channel_cnt_str = "Six"; break;
    case 5: channel_cnt_str = "Five"; break;
    case 4: channel_cnt_str = "Four"; break;
    case 3: channel_cnt_str = "Three"; break;
    default: channel_cnt_str = "Two"; break;
    }

    if(channel_count >= 2 && channel_count <= 8) {
       ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
       if (!ctl) {
            ALOGE("%s: could not get ctl for mixer cmd - %s",
                  __func__, mixer_ctl_name);
        return -EINVAL;
       }
    }
    mixer_ctl_set_enum_by_string(ctl, channel_cnt_str);

    if (channel_count == 6 || channel_count == 8 || channel_count == 2) {
        ret = afe_proxy_set_channel_mapping(adev, channel_count);
    } else {
        ALOGE("%s: set unsupported channel count(%d)",  __func__, channel_count);
        ret = -EINVAL;
    }

    ALOGD("%s: exit", __func__);
    return ret;
}

void audio_extn_set_afe_proxy_parameters(struct audio_device *adev,
                                         struct str_parms *parms)
{
    int ret, val;
    char value[32]={0};

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_WFD, value,
                            sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        aextnmod.proxy_channel_num = val;
        adev->cur_wfd_channels = val;
        ALOGD("%s: channel capability set to: %d", __func__,
               aextnmod.proxy_channel_num);
    }
}

int audio_extn_get_afe_proxy_parameters(const struct audio_device *adev,
                                        struct str_parms *query,
                                        struct str_parms *reply)
{
    int ret, val = 0;
    char value[32]={0};

    ret = str_parms_get_str(query, AUDIO_PARAMETER_CAN_OPEN_PROXY, value,
                            sizeof(value));
    if (ret >= 0) {
        val = (adev->allow_afe_proxy_usage ? 1: 0);
        str_parms_add_int(reply, AUDIO_PARAMETER_CAN_OPEN_PROXY, val);
    }
    ALOGV("%s: called ... can_use_proxy %d", __func__, val);
    return 0;
}

/* must be called with hw device mutex locked */
int32_t audio_extn_read_afe_proxy_channel_masks(struct stream_out *out)
{
    int ret = 0;
    int channels = aextnmod.proxy_channel_num;

    switch (channels) {
        /*
         * Do not handle stereo output in Multi-channel cases
         * Stereo case is handled in normal playback path
         */
    case 6:
        ALOGV("%s: AFE PROXY supports 5.1", __func__);
        out->supported_channel_masks[0] = AUDIO_CHANNEL_OUT_5POINT1;
        break;
    case 8:
        ALOGV("%s: AFE PROXY supports 5.1 and 7.1 channels", __func__);
        out->supported_channel_masks[0] = AUDIO_CHANNEL_OUT_5POINT1;
        out->supported_channel_masks[1] = AUDIO_CHANNEL_OUT_7POINT1;
        break;
    default:
        ALOGE("AFE PROXY does not support multi channel playback");
        ret = -ENOSYS;
        break;
    }
    return ret;
}

int32_t audio_extn_get_afe_proxy_channel_count()
{
    return aextnmod.proxy_channel_num;
}

#endif /* AFE_PROXY_ENABLED */

static int get_active_offload_usecases(const struct audio_device *adev,
                                       struct str_parms *query,
                                       struct str_parms *reply)
{
    int ret, count = 0;
    char value[32]={0};
    struct listnode *node;
    struct audio_usecase *usecase;

    ALOGV("%s", __func__);
    ret = str_parms_get_str(query, AUDIO_PARAMETER_OFFLOAD_NUM_ACTIVE, value,
                            sizeof(value));
    if (ret >= 0) {
        list_for_each(node, &adev->usecase_list) {
            usecase = node_to_item(node, struct audio_usecase, list);
            if (is_offload_usecase(usecase->id))
                count++;
        }
        ALOGV("%s, number of active offload usecases: %d", __func__, count);
        str_parms_add_int(reply, AUDIO_PARAMETER_OFFLOAD_NUM_ACTIVE, count);
    }
    return ret;
}

void audio_extn_set_parameters(struct audio_device *adev,
                               struct str_parms *parms)
{
   audio_extn_set_anc_parameters(adev, parms);
   audio_extn_set_fluence_parameters(adev, parms);
   audio_extn_set_afe_proxy_parameters(adev, parms);
   audio_extn_fm_set_parameters(adev, parms);
   audio_extn_sound_trigger_set_parameters(adev, parms);
   audio_extn_listen_set_parameters(adev, parms);
   audio_extn_ssr_set_parameters(adev, parms);
   audio_extn_hfp_set_parameters(adev, parms);
   audio_extn_dts_eagle_set_parameters(adev, parms);
   audio_extn_a2dp_set_parameters(parms);
   audio_extn_ddp_set_parameters(adev, parms);
   audio_extn_ds2_set_parameters(adev, parms);
   audio_extn_customstereo_set_parameters(adev, parms);
   audio_extn_hpx_set_parameters(adev, parms);
   audio_extn_pm_set_parameters(parms);
   audio_extn_source_track_set_parameters(adev, parms);
   audio_extn_fbsp_set_parameters(parms);
   audio_extn_keep_alive_set_parameters(adev, parms);
   check_and_set_hdmi_connection_status(parms);
   if (adev->offload_effects_set_parameters != NULL)
       adev->offload_effects_set_parameters(parms);
}

void audio_extn_get_parameters(const struct audio_device *adev,
                              struct str_parms *query,
                              struct str_parms *reply)
{
    char *kv_pairs = NULL;
    audio_extn_get_afe_proxy_parameters(adev, query, reply);
    audio_extn_get_fluence_parameters(adev, query, reply);
    audio_extn_ssr_get_parameters(adev, query, reply);
    get_active_offload_usecases(adev, query, reply);
    audio_extn_dts_eagle_get_parameters(adev, query, reply);
    audio_extn_hpx_get_parameters(query, reply);
    audio_extn_source_track_get_parameters(adev, query, reply);
    audio_extn_fbsp_get_parameters(query, reply);
    if (adev->offload_effects_get_parameters != NULL)
        adev->offload_effects_get_parameters(query, reply);

    kv_pairs = str_parms_to_str(reply);
    ALOGD_IF(kv_pairs != NULL, "%s: returns %s", __func__, kv_pairs);
    free(kv_pairs);
}

#ifndef COMPRESS_METADATA_NEEDED
#define audio_extn_parse_compress_metadata(out, parms) (0)
#else
int audio_extn_parse_compress_metadata(struct stream_out *out,
                                       struct str_parms *parms)
{
    int ret = 0;
    char value[32];

    if (out->format == AUDIO_FORMAT_FLAC) {
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_FLAC_MIN_BLK_SIZE, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.flac_dec.min_blk_size = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_FLAC_MAX_BLK_SIZE, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.flac_dec.max_blk_size = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_FLAC_MIN_FRAME_SIZE, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.flac_dec.min_frame_size = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_FLAC_MAX_FRAME_SIZE, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.flac_dec.max_frame_size = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ALOGV("FLAC metadata: min_blk_size %d, max_blk_size %d min_frame_size %d max_frame_size %d",
              out->compr_config.codec->options.flac_dec.min_blk_size,
              out->compr_config.codec->options.flac_dec.max_blk_size,
              out->compr_config.codec->options.flac_dec.min_frame_size,
              out->compr_config.codec->options.flac_dec.max_frame_size);
    }

    else if (out->format == AUDIO_FORMAT_ALAC) {
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_FRAME_LENGTH, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.alac.frame_length = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_COMPATIBLE_VERSION, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.alac.compatible_version = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_BIT_DEPTH, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.alac.bit_depth = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_PB, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.alac.pb = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_MB, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.alac.mb = atoi(value);
            out->is_compr_metadata_avail = true;
        }

        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_KB, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.alac.kb = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_NUM_CHANNELS, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.alac.num_channels = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_MAX_RUN, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.alac.max_run = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_MAX_FRAME_BYTES, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.alac.max_frame_bytes = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_AVG_BIT_RATE, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.alac.avg_bit_rate = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_SAMPLING_RATE, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.alac.sample_rate = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_CHANNEL_LAYOUT_TAG, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.alac.channel_layout_tag = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ALOGV("ALAC CSD values: frameLength %d bitDepth %d numChannels %d"
                " maxFrameBytes %d, avgBitRate %d, sampleRate %d",
                out->compr_config.codec->options.alac.frame_length,
                out->compr_config.codec->options.alac.bit_depth,
                out->compr_config.codec->options.alac.num_channels,
                out->compr_config.codec->options.alac.max_frame_bytes,
                out->compr_config.codec->options.alac.avg_bit_rate,
                out->compr_config.codec->options.alac.sample_rate);
    }

    else if (out->format == AUDIO_FORMAT_APE) {
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_COMPATIBLE_VERSION, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.ape.compatible_version = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_COMPRESSION_LEVEL, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.ape.compression_level = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_FORMAT_FLAGS, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.ape.format_flags = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_BLOCKS_PER_FRAME, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.ape.blocks_per_frame = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_FINAL_FRAME_BLOCKS, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.ape.final_frame_blocks = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_TOTAL_FRAMES, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.ape.total_frames = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_BITS_PER_SAMPLE, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.ape.bits_per_sample = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_NUM_CHANNELS, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.ape.num_channels = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_SAMPLE_RATE, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.ape.sample_rate = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_SEEK_TABLE_PRESENT, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.ape.seek_table_present = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ALOGV("APE CSD values: compatibleVersion %d compressionLevel %d"
                " formatFlags %d blocksPerFrame %d finalFrameBlocks %d"
                " totalFrames %d bitsPerSample %d numChannels %d"
                " sampleRate %d seekTablePresent %d",
                out->compr_config.codec->options.ape.compatible_version,
                out->compr_config.codec->options.ape.compression_level,
                out->compr_config.codec->options.ape.format_flags,
                out->compr_config.codec->options.ape.blocks_per_frame,
                out->compr_config.codec->options.ape.final_frame_blocks,
                out->compr_config.codec->options.ape.total_frames,
                out->compr_config.codec->options.ape.bits_per_sample,
                out->compr_config.codec->options.ape.num_channels,
                out->compr_config.codec->options.ape.sample_rate,
                out->compr_config.codec->options.ape.seek_table_present);
    }

    else if (out->format == AUDIO_FORMAT_VORBIS) {
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_VORBIS_BITSTREAM_FMT, value, sizeof(value));
        if (ret >= 0) {
        // transcoded bitstream mode
            out->compr_config.codec->options.vorbis_dec.bit_stream_fmt = (atoi(value) > 0) ? 1 : 0;
            out->is_compr_metadata_avail = true;
        }
    }

    else if (out->format == AUDIO_FORMAT_WMA || out->format == AUDIO_FORMAT_WMA_PRO) {
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_WMA_FORMAT_TAG, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->format = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_AVG_BIT_RATE, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.wma.avg_bit_rate = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_WMA_BLOCK_ALIGN, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.wma.super_block_align = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_WMA_BIT_PER_SAMPLE, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.wma.bits_per_sample = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_WMA_CHANNEL_MASK, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.wma.channelmask = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_WMA_ENCODE_OPTION, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.wma.encodeopt = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_WMA_ENCODE_OPTION1, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.wma.encodeopt1 = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_WMA_ENCODE_OPTION2, value, sizeof(value));
        if (ret >= 0) {
            out->compr_config.codec->options.wma.encodeopt2 = atoi(value);
            out->is_compr_metadata_avail = true;
        }
        ALOGV("WMA params: fmt %x, bit rate %x, balgn %x, sr %d, chmsk %x"
                " encop %x, op1 %x, op2 %x",
                out->compr_config.codec->format,
                out->compr_config.codec->options.wma.avg_bit_rate,
                out->compr_config.codec->options.wma.super_block_align,
                out->compr_config.codec->options.wma.bits_per_sample,
                out->compr_config.codec->options.wma.channelmask,
                out->compr_config.codec->options.wma.encodeopt,
                out->compr_config.codec->options.wma.encodeopt1,
                out->compr_config.codec->options.wma.encodeopt2);
    }

    return ret;
}
#endif

#ifdef AUXPCM_BT_ENABLED
int32_t audio_extn_read_xml(struct audio_device *adev, uint32_t mixer_card,
                            const char* mixer_xml_path,
                            const char* mixer_xml_path_auxpcm)
{
    char bt_soc[128];
    bool wifi_init_complete = false;
    int sleep_retry = 0;

    while (!wifi_init_complete && sleep_retry < MAX_SLEEP_RETRY) {
        property_get("qcom.bluetooth.soc", bt_soc, NULL);
        if (strncmp(bt_soc, "unknown", sizeof("unknown"))) {
            wifi_init_complete = true;
        } else {
            usleep(WIFI_INIT_WAIT_SLEEP*1000);
            sleep_retry++;
        }
    }

    if (!strncmp(bt_soc, "ath3k", sizeof("ath3k")))
        adev->audio_route = audio_route_init(mixer_card, mixer_xml_path_auxpcm);
    else
        adev->audio_route = audio_route_init(mixer_card, mixer_xml_path);

    return 0;
}
#endif /* AUXPCM_BT_ENABLED */

#ifdef KPI_OPTIMIZE_ENABLED
typedef int (*perf_lock_acquire_t)(int, int, int*, int);
typedef int (*perf_lock_release_t)(int);

static void *qcopt_handle;
static perf_lock_acquire_t perf_lock_acq;
static perf_lock_release_t perf_lock_rel;

char opt_lib_path[512] = {0};

int audio_extn_perf_lock_init(void)
{
    int ret = 0;
    if (qcopt_handle == NULL) {
        if (property_get("ro.vendor.extension_library",
                         opt_lib_path, NULL) <= 0) {
            ALOGE("%s: Failed getting perf property \n", __func__);
            ret = -EINVAL;
            goto err;
        }
        if ((qcopt_handle = dlopen(opt_lib_path, RTLD_NOW)) == NULL) {
            ALOGE("%s: Failed to open perf handle \n", __func__);
            ret = -EINVAL;
            goto err;
        } else {
            perf_lock_acq = (perf_lock_acquire_t)dlsym(qcopt_handle,
                                                       "perf_lock_acq");
            if (perf_lock_acq == NULL) {
                ALOGE("%s: Perf lock Acquire NULL \n", __func__);
                ret = -EINVAL;
                goto err;
            }
            perf_lock_rel = (perf_lock_release_t)dlsym(qcopt_handle,
                                                       "perf_lock_rel");
            if (perf_lock_rel == NULL) {
                ALOGE("%s: Perf lock Release NULL \n", __func__);
                ret = -EINVAL;
                goto err;
            }
            ALOGE("%s: Perf lock handles Success \n", __func__);
        }
    }
err:
    return ret;
}

void audio_extn_perf_lock_acquire(int *handle, int duration,
                                 int *perf_lock_opts, int size)
{

    if (!perf_lock_opts || !size || !perf_lock_acq || !handle) {
        ALOGE("%s: Incorrect params, Failed to acquire perf lock, err ",
              __func__);
        return;
    }
    /*
     * Acquire performance lock for 1 sec during device path bringup.
     * Lock will be released either after 1 sec or when perf_lock_release
     * function is executed.
     */
    *handle = perf_lock_acq(*handle, duration, perf_lock_opts, size);
    if (*handle <= 0)
        ALOGE("%s: Failed to acquire perf lock, err: %d\n",
              __func__, *handle);
}

void audio_extn_perf_lock_release(int *handle)
{
    if (perf_lock_rel && handle && (*handle > 0)) {
        perf_lock_rel(*handle);
        *handle = 0;
    } else {
        ALOGE("%s: Perf lock release error \n", __func__);
    }
}
#endif /* KPI_OPTIMIZE_ENABLED */
