/*
 * Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * *    * Redistributions of source code must retain the above copyright
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
#define LOG_NIDEBUG 0

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdlib.h>

#define LOG_TAG "QCOM PowerHAL"
#include <utils/Log.h>
#include <hardware/hardware.h>
#include <hardware/power.h>

#include "utils.h"
#include "metadata-defs.h"
#include "hint-data.h"
#include "performance.h"
#include "power-common.h"
#include "powerhintparser.h"

pthread_mutex_t camera_hint_mutex = PTHREAD_MUTEX_INITIALIZER;
static int camera_hint_ref_count;
static int current_power_profile = PROFILE_BALANCED;
static int sustained_mode_handle = 0;
static int vr_mode_handle = 0;
int sustained_performance_mode = 0;
int vr_mode = 0;
static pthread_mutex_t s_interaction_lock = PTHREAD_MUTEX_INITIALIZER;
int launch_handle = -1;
int launch_mode;
int cpu_boost_handle = -1;
int cpu_boost_mode;
#define CHECK_HANDLE(x) (((x)>0) && ((x)!=-1))

extern void interaction(int duration, int num_args, int opt_list[]);

int get_number_of_profiles() {
    return 5;
}

static int profile_high_performance[] = {
    SCHED_BOOST_ON_V3, 0x1,
    ALL_CPUS_PWR_CLPS_DIS_V3, 0x1,
    CPUS_ONLINE_MIN_BIG, 0x2,
    CPUS_ONLINE_MIN_LITTLE, 0x2,
    MIN_FREQ_BIG_CORE_0, 0xFFF,
    MIN_FREQ_LITTLE_CORE_0, 0xFFF,
};

static int profile_power_save[] = {
    CPUS_ONLINE_MAX_LIMIT_BIG, 0x1,
    MAX_FREQ_BIG_CORE_0, 0x3E8,
    MAX_FREQ_LITTLE_CORE_0, 0x3E8,
};

static int profile_bias_power[] = {
    MAX_FREQ_BIG_CORE_0, 0x514,
    MAX_FREQ_LITTLE_CORE_0, 0x3E8,
};

static int profile_bias_performance[] = {
    CPUS_ONLINE_MAX_LIMIT_BIG, 0x2,
    CPUS_ONLINE_MAX_LIMIT_LITTLE, 0x2,
    MIN_FREQ_BIG_CORE_0, 0x578,
};

static void set_power_profile(int profile) {

    if (profile == current_power_profile)
        return;

    ALOGV("%s: Profile=%d", __func__, profile);

    if (current_power_profile != PROFILE_BALANCED) {
        undo_hint_action(DEFAULT_PROFILE_HINT_ID);
        ALOGV("%s: Hint undone", __func__);
    }

    if (profile == PROFILE_POWER_SAVE) {
        perform_hint_action(DEFAULT_PROFILE_HINT_ID, profile_power_save,
                ARRAY_SIZE(profile_power_save));
        ALOGD("%s: Set powersave mode", __func__);

    } else if (profile == PROFILE_HIGH_PERFORMANCE) {
        perform_hint_action(DEFAULT_PROFILE_HINT_ID, profile_high_performance,
                ARRAY_SIZE(profile_high_performance));
        ALOGD("%s: Set performance mode", __func__);

    } else if (profile == PROFILE_BIAS_POWER) {
        perform_hint_action(DEFAULT_PROFILE_HINT_ID, profile_bias_power,
                ARRAY_SIZE(profile_bias_power));
        ALOGD("%s: Set bias power mode", __func__);

    } else if (profile == PROFILE_BIAS_PERFORMANCE) {
        perform_hint_action(DEFAULT_PROFILE_HINT_ID, profile_bias_performance,
                ARRAY_SIZE(profile_bias_performance));
        ALOGD("%s: Set bias perf mode", __func__);

    }

    current_power_profile = profile;
}

static int process_video_encode_hint(void *metadata)
{
    char governor[80];
    struct video_encode_metadata_t video_encode_metadata;

    if (get_scaling_governor(governor, sizeof(governor)) == -1) {
        ALOGE("Can't obtain scaling governor.");

        return HINT_NONE;
    }

    /* Initialize encode metadata struct fields */
    memset(&video_encode_metadata, 0, sizeof(struct video_encode_metadata_t));
    video_encode_metadata.state = -1;
    video_encode_metadata.hint_id = DEFAULT_VIDEO_ENCODE_HINT_ID;

    if (metadata) {
        if (parse_video_encode_metadata((char *)metadata, &video_encode_metadata) ==
            -1) {
            ALOGE("Error occurred while parsing metadata.");
            return HINT_NONE;
        }
    } else {
        return HINT_NONE;
    }

    if (video_encode_metadata.state == 1) {
        if ((strncmp(governor, INTERACTIVE_GOVERNOR, strlen(INTERACTIVE_GOVERNOR)) == 0) &&
                (strlen(governor) == strlen(INTERACTIVE_GOVERNOR))) {
            /* 1. cpufreq params
             *    -above_hispeed_delay for LVT - 40ms
             *    -go hispeed load for LVT - 95
             *    -hispeed freq for LVT - 556 MHz
             *    -target load for LVT - 90
             *    -above hispeed delay for sLVT - 40ms
             *    -go hispeed load for sLVT - 95
             *    -hispeed freq for sLVT - 806 MHz
             *    -target load for sLVT - 90
             * 2. bus DCVS set to V2 config:
             *    -low power ceil mpbs - 2500
             *    -low power io percent - 50
             * 3. hysteresis optimization
             *    -bus dcvs hysteresis tuning
             *    -sample_ms of 10 ms
             */
            int resource_values[] = {
                ABOVE_HISPEED_DELAY_BIG, 0x4,
                GO_HISPEED_LOAD_BIG, 0x5F,
                HISPEED_FREQ_BIG, 0x326,
                TARGET_LOADS_BIG, 0x5A,
                ABOVE_HISPEED_DELAY_LITTLE, 0x4,
                GO_HISPEED_LOAD_LITTLE, 0x5F,
                HISPEED_FREQ_LITTLE, 0x22C,
                TARGET_LOADS_LITTLE, 0x5A,
                LOW_POWER_CEIL_MBPS, 0x9C4,
                LOW_POWER_IO_PERCENT, 0x32,
                CPUBW_HWMON_V1, 0x0,
                CPUBW_HWMON_SAMPLE_MS, 0xA,
            };

            pthread_mutex_lock(&camera_hint_mutex);
            camera_hint_ref_count++;
            if (camera_hint_ref_count == 1) {
                perform_hint_action(video_encode_metadata.hint_id,
                        resource_values, ARRAY_SIZE(resource_values));
            }
            pthread_mutex_unlock(&camera_hint_mutex);
            ALOGI("Video Encode hint start");
            return HINT_HANDLED;
        }
    } else if (video_encode_metadata.state == 0) {
        if ((strncmp(governor, INTERACTIVE_GOVERNOR, strlen(INTERACTIVE_GOVERNOR)) == 0) &&
                (strlen(governor) == strlen(INTERACTIVE_GOVERNOR))) {
            pthread_mutex_lock(&camera_hint_mutex);
            camera_hint_ref_count--;
            if (!camera_hint_ref_count) {
                undo_hint_action(video_encode_metadata.hint_id);
            }
            pthread_mutex_unlock(&camera_hint_mutex);

            ALOGI("Video Encode hint stop");
            return HINT_HANDLED;
        }
    }
    return HINT_NONE;
}

int power_hint_override(__unused struct power_module *module,
        power_hint_t hint, void *data)
{
    static struct timespec s_previous_boost_timespec;
    struct timespec cur_boost_timespec;
    long long elapsed_time;
    int duration;

    int resources_launch[] = {
        SCHED_BOOST_ON_V3, 0x1,
        MAX_FREQ_BIG_CORE_0, 0xFFF,
        MAX_FREQ_LITTLE_CORE_0, 0xFFF,
        MIN_FREQ_BIG_CORE_0, 0xFFF,
        MIN_FREQ_LITTLE_CORE_0, 0xFFF,
        CPUBW_HWMON_MIN_FREQ, 0x8C,
        ALL_CPUS_PWR_CLPS_DIS_V3, 0x1,
        STOR_CLK_SCALE_DIS, 0x1,
    };

    int resources_cpu_boost[] = {
        SCHED_BOOST_ON_V3, 0x1,
        MIN_FREQ_BIG_CORE_0, 0x3E8,
    };

    int resources_interaction_fling_boost[] = {
        CPUBW_HWMON_MIN_FREQ, 0x33,
        MIN_FREQ_BIG_CORE_0, 0x3E8,
        MIN_FREQ_LITTLE_CORE_0, 0x3E8,
        SCHED_BOOST_ON_V3, 0x1,
   //   SCHED_GROUP_ON, 0x1,
    };

    int resources_interaction_boost[] = {
        MIN_FREQ_BIG_CORE_0, 0x3E8,
    };

    int *resources_sustained_mode = NULL;
    int *resources_vr_sustained_mode = NULL;
    int *resources_vr_mode = NULL;
    int resources = 0;

    if (hint == POWER_HINT_SET_PROFILE) {
        set_power_profile(*(int32_t *)data);
        return HINT_HANDLED;
    }

    /* Skip other hints in power save mode */
    if (current_power_profile == PROFILE_POWER_SAVE)
        return HINT_HANDLED;

    if (hint == POWER_HINT_INTERACTION) {
        pthread_mutex_lock(&s_interaction_lock);
        if (sustained_performance_mode || vr_mode) {
            pthread_mutex_unlock(&s_interaction_lock);
            return HINT_HANDLED;
        }

        duration = data ? *((int *)data) : 500;

        clock_gettime(CLOCK_MONOTONIC, &cur_boost_timespec);
        elapsed_time = calc_timespan_us(s_previous_boost_timespec, cur_boost_timespec);
        if (elapsed_time > 750000)
            elapsed_time = 750000;
        /**
         * Don't hint if it's been less than 250ms since last boost
         * also detect if we're doing anything resembling a fling
         * support additional boosting in case of flings
         */
        else if (elapsed_time < 250000 && duration <= 750) {
            pthread_mutex_unlock(&s_interaction_lock);
            return HINT_HANDLED;
        }

        s_previous_boost_timespec = cur_boost_timespec;

        if (duration >= 1500) {
            interaction(duration, ARRAY_SIZE(resources_interaction_fling_boost),
                    resources_interaction_fling_boost);
        } else {
            interaction(duration, ARRAY_SIZE(resources_interaction_boost),
                    resources_interaction_boost);
        }
        pthread_mutex_unlock(&s_interaction_lock);
        return HINT_HANDLED;
    }

    if (hint == POWER_HINT_LAUNCH) {
        duration = 2000;
        if (sustained_performance_mode || vr_mode) {
            return HINT_HANDLED;
        }

        if (data && launch_mode == 0) {
            launch_handle = interaction_with_handle(
                launch_handle, duration, ARRAY_SIZE(resources_launch), resources_launch);
            if (CHECK_HANDLE(launch_handle)) {
                launch_mode = 1;
                ALOGI("Activity launch hint handled");
                return HINT_HANDLED;
            } else {
                return HINT_NONE;
            }
        } else if (data == NULL && launch_mode == 1) {
            release_request(launch_handle);
            launch_mode = 0;
            return HINT_HANDLED;
        }
        return HINT_NONE;
    }

    if (hint == POWER_HINT_CPU_BOOST) {
        duration = *(int32_t *)data / 1000;
        if (sustained_performance_mode || vr_mode) {
            return HINT_HANDLED;
        }

        if (data && cpu_boost_mode == 0) {
            cpu_boost_handle = interaction_with_handle(
                cpu_boost_handle, duration, ARRAY_SIZE(resources_cpu_boost), resources_cpu_boost);
            if (CHECK_HANDLE(cpu_boost_handle)) {
                cpu_boost_mode = 1;
                ALOGI("CPU boost hint handled");
                return HINT_HANDLED;
            } else {
                return HINT_NONE;
            }
        } else if (data == NULL && cpu_boost_mode == 1) {
            release_request(cpu_boost_handle);
            cpu_boost_mode = 0;
            return HINT_HANDLED;
        }
        return HINT_NONE;
    }

    if (hint == POWER_HINT_VIDEO_ENCODE)
        return process_video_encode_hint(data);

    if (hint == POWER_HINT_SUSTAINED_PERFORMANCE)
    {
        duration = 0;
        pthread_mutex_lock(&s_interaction_lock);
        if (data && sustained_performance_mode == 0) {
            if (vr_mode == 0) { // Sustained mode only.
                resources_sustained_mode = getPowerhint(SUSTAINED_PERF_HINT_ID, &resources);
                if (!resources_sustained_mode) {
                    ALOGE("Can't get sustained perf hints from xml ");
                    pthread_mutex_unlock(&s_interaction_lock);
                    return HINT_NONE;
                }
                // Ensure that POWER_HINT_LAUNCH is not in progress.
                if (launch_mode == 1) {
                    release_request(launch_handle);
                    launch_mode = 0;
                }
                // Ensure that POWER_HINT_CPU_BOOST is not in progress.
                if (cpu_boost_mode == 1) {
                    release_request(cpu_boost_handle);
                    cpu_boost_mode = 0;
                }
                sustained_mode_handle = interaction_with_handle(
                    sustained_mode_handle, duration, resources, resources_sustained_mode);
                if (!CHECK_HANDLE(sustained_mode_handle)) {
                    ALOGE("Failed interaction_with_handle for sustained_mode_handle");
                    pthread_mutex_unlock(&s_interaction_lock);
                    return HINT_NONE;
                }
            } else if (vr_mode == 1) { // Sustained + VR mode.
                resources_vr_sustained_mode = getPowerhint(VR_MODE_SUSTAINED_PERF_HINT_ID, &resources);
                if (!resources_vr_sustained_mode) {
                    ALOGE("Can't get VR mode sustained perf hints from xml ");
                    pthread_mutex_unlock(&s_interaction_lock);
                    return HINT_NONE;
                }
                release_request(vr_mode_handle);
                sustained_mode_handle = interaction_with_handle(
                    sustained_mode_handle, duration, resources, resources_vr_sustained_mode);
                if (!CHECK_HANDLE(sustained_mode_handle)) {
                    ALOGE("Failed interaction_with_handle for sustained_mode_handle");
                    pthread_mutex_unlock(&s_interaction_lock);
                    return HINT_NONE;
                }
            }
            sustained_performance_mode = 1;
        } else if (sustained_performance_mode == 1) {
            release_request(sustained_mode_handle);
            if (vr_mode == 1) { // Switch back to VR Mode.
                resources_vr_mode = getPowerhint(VR_MODE_HINT_ID, &resources);
                if (!resources_vr_mode) {
                    ALOGE("Can't get VR mode perf hints from xml ");
                    pthread_mutex_unlock(&s_interaction_lock);
                    return HINT_NONE;
                }
                release_request(vr_mode_handle);
                vr_mode_handle = interaction_with_handle(
                    vr_mode_handle, duration, resources, resources_vr_mode);
                if (!CHECK_HANDLE(vr_mode_handle)) {
                    ALOGE("Failed interaction_with_handle for vr_mode_handle");
                    pthread_mutex_unlock(&s_interaction_lock);
                    return HINT_NONE;
                }
            }
            sustained_performance_mode = 0;
        }
        pthread_mutex_unlock(&s_interaction_lock);
        return HINT_HANDLED;
    }

    if (hint == POWER_HINT_VR_MODE)
    {
        duration = 0;
        pthread_mutex_lock(&s_interaction_lock);
        if (data && vr_mode == 0) {
            if (sustained_performance_mode == 0) { // VR mode only.
                resources_vr_mode = getPowerhint(VR_MODE_HINT_ID, &resources);
                if (!resources_vr_mode) {
                    ALOGE("Can't get VR mode perf hints from xml ");
                    pthread_mutex_unlock(&s_interaction_lock);
                    return HINT_NONE;
                }
                // Ensure that POWER_HINT_LAUNCH is not in progress.
                if (launch_mode == 1) {
                    release_request(launch_handle);
                    launch_mode = 0;
                }
                // Ensure that POWER_HINT_CPU_BOOST is not in progress.
                if (cpu_boost_mode == 1) {
                    release_request(cpu_boost_handle);
                    cpu_boost_mode = 0;
                }
                vr_mode_handle = interaction_with_handle(
                    vr_mode_handle, duration, resources, resources_vr_mode);
                if (!CHECK_HANDLE(vr_mode_handle)) {
                    ALOGE("Failed interaction_with_handle for vr_mode_handle");
                    pthread_mutex_unlock(&s_interaction_lock);
                    return HINT_NONE;
                }
            } else if (sustained_performance_mode == 1) { // Sustained + VR mode.
                resources_vr_sustained_mode = getPowerhint(VR_MODE_SUSTAINED_PERF_HINT_ID, &resources);
                if (!resources_vr_sustained_mode) {
                    ALOGE("Can't get VR mode sustained perf hints from xml ");
                    pthread_mutex_unlock(&s_interaction_lock);
                    return HINT_NONE;
                }
                release_request(sustained_mode_handle);
                vr_mode_handle = interaction_with_handle(
                    vr_mode_handle, duration, resources, resources_vr_sustained_mode);
                if (!CHECK_HANDLE(vr_mode_handle)) {
                    ALOGE("Failed interaction_with_handle for vr_mode_handle");
                    pthread_mutex_unlock(&s_interaction_lock);
                    return HINT_NONE;
                }
            }
            vr_mode = 1;
        } else if (vr_mode == 1) {
            release_request(vr_mode_handle);
            if (sustained_performance_mode == 1) { // Switch back to sustained Mode.
                resources_sustained_mode = getPowerhint(SUSTAINED_PERF_HINT_ID, &resources);
                if (!resources_sustained_mode) {
                    ALOGE("Can't get sustained perf hints from xml ");
                    pthread_mutex_unlock(&s_interaction_lock);
                    return HINT_NONE;
                }
                release_request(sustained_mode_handle);
                sustained_mode_handle = interaction_with_handle(
                    sustained_mode_handle, duration, resources, resources_sustained_mode);
                if (!CHECK_HANDLE(sustained_mode_handle)) {
                    ALOGE("Failed interaction_with_handle for sustained_mode_handle");
                    pthread_mutex_unlock(&s_interaction_lock);
                    return HINT_NONE;
                }
            }
            vr_mode = 0;
        }
        pthread_mutex_unlock(&s_interaction_lock);
        return HINT_HANDLED;
    }

    return HINT_NONE;
}

int set_interactive_override(__unused struct power_module *module, int on)
{
    return HINT_HANDLED; /* Don't excecute this code path, not in use */
    char governor[80];

    if (get_scaling_governor(governor, sizeof(governor)) == -1) {
        ALOGE("Can't obtain scaling governor.");

        return HINT_NONE;
    }

    if (!on) {
        /* Display off */
        if ((strncmp(governor, INTERACTIVE_GOVERNOR, strlen(INTERACTIVE_GOVERNOR)) == 0) &&
            (strlen(governor) == strlen(INTERACTIVE_GOVERNOR))) {
            int resource_values[] = {}; /* dummy node */
            perform_hint_action(DISPLAY_STATE_HINT_ID,
                    resource_values, ARRAY_SIZE(resource_values));
            ALOGI("Display Off hint start");
            return HINT_HANDLED;
        }
    } else {
        /* Display on */
        if ((strncmp(governor, INTERACTIVE_GOVERNOR, strlen(INTERACTIVE_GOVERNOR)) == 0) &&
            (strlen(governor) == strlen(INTERACTIVE_GOVERNOR))) {
            undo_hint_action(DISPLAY_STATE_HINT_ID);
            ALOGI("Display Off hint stop");
            return HINT_HANDLED;
        }
    }
    return HINT_NONE;
}
