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
 */

#define LOG_TAG "audio_hw_ssr"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0

#include <errno.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <cutils/str_parms.h>
#include <cutils/log.h>
#include <pthread.h>
#include <cutils/sched_policy.h>
#include <sys/resource.h>
#include <system/thread_defs.h>

#include "audio_hw.h"
#include "audio_extn.h"
#include "platform.h"
#include "platform_api.h"
#include "surround_rec_interface.h"

#ifdef SSR_ENABLED
#define COEFF_ARRAY_SIZE            4
#define FILT_SIZE                   ((512+1)* 6)  /* # ((FFT bins)/2+1)*numOutputs */
#define SSR_CHANNEL_OUTPUT_NUM      6
#define SSR_PERIOD_SIZE             240

#define NUM_IN_BUFS                 4
#define NUM_OUT_BUFS                4
#define NUM_IN_CHANNELS             3

#define LIB_SURROUND_3MIC_PROC  "libsurround_3mic_proc.so"
#define LIB_DRC                 "libdrc.so"

#define AUDIO_PARAMETER_SSRMODE_ON        "ssrOn"


typedef short Word16;
typedef const get_param_data_t* (*surround_rec_get_get_param_data_t)(void);
typedef const set_param_data_t* (*surround_rec_get_set_param_data_t)(void);
typedef int (*surround_rec_init_t)(void **, int, int, int, int, const char *);
typedef void (*surround_rec_deinit_t)(void *);
typedef void (*surround_rec_process_t)(void *, const int16_t *, int16_t *);

typedef int (*drc_init_t)(void **, int, int, const char *);
typedef void (*drc_deinit_t)(void *);
typedef int (*drc_process_t)(void *, const int16_t *, int16_t *);

struct pcm_buffer {
    void *data;
    int length;
};

struct pcm_buffer_queue {
    struct pcm_buffer_queue *next;
    struct pcm_buffer buffer;
};

struct ssr_module {
    int                 ssr_3mic;
    int                 num_out_chan;
    FILE                *fp_input;
    FILE                *fp_output;
    void                *surround_obj;
    Word16              *surround_raw_buffer;
    int                  surround_raw_buffer_size;
    bool                 is_ssr_enabled;
    struct stream_in    *in;
    void                *drc_obj;


    void *surround_rec_handle;
    surround_rec_get_get_param_data_t surround_rec_get_get_param_data;
    surround_rec_get_set_param_data_t surround_rec_get_set_param_data;
    surround_rec_init_t surround_rec_init;
    surround_rec_deinit_t surround_rec_deinit;
    surround_rec_process_t surround_rec_process;

    void *drc_handle;
    drc_init_t drc_init;
    drc_deinit_t drc_deinit;
    drc_process_t drc_process;

    pthread_t ssr_process_thread;
    bool ssr_process_thread_started;
    bool ssr_process_thread_stop;
    struct pcm_buffer_queue in_buf_nodes[NUM_IN_BUFS];
    struct pcm_buffer_queue out_buf_nodes[NUM_OUT_BUFS];
    void *in_buf_data;
    void *out_buf_data;
    struct pcm_buffer_queue *out_buf_free;
    struct pcm_buffer_queue *out_buf;
    struct pcm_buffer_queue *in_buf_free;
    struct pcm_buffer_queue *in_buf;
    pthread_mutex_t ssr_process_lock;
    pthread_cond_t cond_process;
    pthread_cond_t cond_read;
    bool is_ssr_mode_on;
};

static struct ssr_module ssrmod = {
    .fp_input = NULL,
    .fp_output = NULL,
    .surround_obj = NULL,
    .surround_raw_buffer = NULL,
    .surround_raw_buffer_size = 0,
    .is_ssr_enabled = 0,
    .in = NULL,
    .drc_obj = NULL,

    .surround_rec_handle = NULL,
    .surround_rec_get_get_param_data = NULL,
    .surround_rec_get_set_param_data = NULL,
    .surround_rec_init = NULL,
    .surround_rec_deinit = NULL,
    .surround_rec_process = NULL,

    .drc_handle = NULL,
    .drc_init = NULL,
    .drc_deinit = NULL,
    .drc_process = NULL,

    .ssr_process_thread_stop = 0,
    .ssr_process_thread_started = 0,
    .in_buf_data = NULL,
    .out_buf_data = NULL,
    .out_buf_free = NULL,
    .out_buf = NULL,
    .in_buf_free = NULL,
    .in_buf = NULL,
    .cond_process = PTHREAD_COND_INITIALIZER,
    .cond_read = PTHREAD_COND_INITIALIZER,
    .ssr_process_lock = PTHREAD_MUTEX_INITIALIZER,
    .is_ssr_mode_on = false,
};

static void *ssr_process_thread(void *context);

static int32_t drc_init_lib(int num_chan, int sample_rate __unused)
{
    int ret = 0;
    const char *cfgFileName = "";

    if (ssrmod.drc_obj) {
        ALOGE("%s: DRC library is already initialized", __func__);
        return 0;
    }

    ssrmod.drc_handle = dlopen(LIB_DRC, RTLD_NOW);
    if (ssrmod.drc_handle == NULL) {
        ALOGE("%s: DLOPEN failed for %s", __func__, LIB_DRC);
        ret = -ENOSYS;
        goto init_fail;
    }

    ALOGV("%s: DLOPEN successful for %s", __func__, LIB_DRC);
    ssrmod.drc_init = (drc_init_t)
        dlsym(ssrmod.drc_handle, "DRC_init");
    ssrmod.drc_deinit = (drc_deinit_t)
        dlsym(ssrmod.drc_handle, "DRC_deinit");
    ssrmod.drc_process = (drc_process_t)
        dlsym(ssrmod.drc_handle, "DRC_process");

    if (!ssrmod.drc_init ||
        !ssrmod.drc_deinit ||
        !ssrmod.drc_process){
        ALOGW("%s: Could not find one of the symbols from %s",
              __func__, LIB_DRC);
        ret = -ENOSYS;
        goto init_fail;
    }

    /* TO DO: different config files for different sample rates */
    if (num_chan == 6) {
        cfgFileName = "/system/etc/drc/drc_cfg_5.1.txt";
    } else if (num_chan == 2) {
        cfgFileName = "/system/etc/drc/drc_cfg_AZ.txt";
    }

    ALOGV("%s: Calling drc_init: num ch: %d, period: %d, cfg file: %s", __func__, num_chan, SSR_PERIOD_SIZE, cfgFileName);
    ret = ssrmod.drc_init(&ssrmod.drc_obj, num_chan, SSR_PERIOD_SIZE, cfgFileName);
    if (ret) {
        ALOGE("drc_init failed with ret:%d",ret);
        ret = -EINVAL;
        goto init_fail;
    }

    return 0;

init_fail:
    if (ssrmod.drc_obj) {
        ssrmod.drc_obj = NULL;
    }
    if(ssrmod.drc_handle) {
        dlclose(ssrmod.drc_handle);
        ssrmod.drc_handle = NULL;
    }
    return ret;
}

static int32_t ssr_init_surround_sound_3mic_lib(unsigned long buffersize, int num_in_chan, int num_out_chan, int sample_rate)
{
    int ret = 0;
    const char *cfgFileName = NULL;

    if ( ssrmod.surround_obj ) {
        ALOGE("%s: surround sound library is already initialized", __func__);
        return 0;
    }

    ssrmod.surround_rec_handle = dlopen(LIB_SURROUND_3MIC_PROC, RTLD_NOW);
    if (ssrmod.surround_rec_handle == NULL) {
        ALOGE("%s: DLOPEN failed for %s", __func__, LIB_SURROUND_3MIC_PROC);
        goto init_fail;
    } else {
        ALOGV("%s: DLOPEN successful for %s", __func__, LIB_SURROUND_3MIC_PROC);
        ssrmod.surround_rec_get_get_param_data = (surround_rec_get_get_param_data_t)
        dlsym(ssrmod.surround_rec_handle, "surround_rec_get_get_param_data");

        ssrmod.surround_rec_get_set_param_data = (surround_rec_get_set_param_data_t)
        dlsym(ssrmod.surround_rec_handle, "surround_rec_get_set_param_data");
        ssrmod.surround_rec_init = (surround_rec_init_t)
        dlsym(ssrmod.surround_rec_handle, "surround_rec_init");
        ssrmod.surround_rec_deinit = (surround_rec_deinit_t)
        dlsym(ssrmod.surround_rec_handle, "surround_rec_deinit");
        ssrmod.surround_rec_process = (surround_rec_process_t)
        dlsym(ssrmod.surround_rec_handle, "surround_rec_process");

        if (!ssrmod.surround_rec_get_get_param_data ||
            !ssrmod.surround_rec_get_set_param_data ||
            !ssrmod.surround_rec_init ||
            !ssrmod.surround_rec_deinit ||
            !ssrmod.surround_rec_process){
            ALOGW("%s: Could not find the one of the symbols from %s",
                  __func__, LIB_SURROUND_3MIC_PROC);
            ret = -ENOSYS;
            goto init_fail;
        }
    }

    /* Allocate memory for input buffer */
    ssrmod.surround_raw_buffer = (Word16 *) calloc(buffersize,
                                              sizeof(Word16));
    if (!ssrmod.surround_raw_buffer) {
       ALOGE("%s: Memory allocation failure. Not able to allocate "
             "memory for surroundInputBuffer", __func__);
       ret = -ENOMEM;
       goto init_fail;
    }

    ssrmod.surround_raw_buffer_size = buffersize;

    ssrmod.num_out_chan = num_out_chan;

    if (num_out_chan == 6) {
        cfgFileName = "/system/etc/surround_sound_3mic/surround_sound_rec_5.1.cfg";
    } else if (num_out_chan == 2) {
        cfgFileName = "/system/etc/surround_sound_3mic/surround_sound_rec_AZ.cfg";
    } else {
        ALOGE("%s: No cfg file for num_out_chan: %d", __func__, num_out_chan);
    }

    ALOGV("%s: Calling surround_rec_init: in ch: %d, out ch: %d, period: %d, sample rate: %d, cfg file: %s",
          __func__, num_in_chan, num_out_chan, SSR_PERIOD_SIZE, sample_rate, cfgFileName);
     ret = ssrmod.surround_rec_init(&ssrmod.surround_obj,
        num_in_chan, num_out_chan, SSR_PERIOD_SIZE, sample_rate, cfgFileName);
    if (ret) {
        ALOGE("surround_rec_init failed with ret:%d",ret);
        ret = -EINVAL;
        goto init_fail;
    }

    return 0;

init_fail:
    if (ssrmod.surround_obj) {
        ssrmod.surround_obj = NULL;
    }
    if (ssrmod.surround_raw_buffer) {
        free(ssrmod.surround_raw_buffer);
        ssrmod.surround_raw_buffer = NULL;
        ssrmod.surround_raw_buffer_size = 0;
    }
    if(ssrmod.surround_rec_handle) {
        dlclose(ssrmod.surround_rec_handle);
        ssrmod.surround_rec_handle = NULL;
    }
    return ret;
}

void audio_extn_ssr_update_enabled()
{
    char ssr_enabled[PROPERTY_VALUE_MAX] = "false";

    property_get("ro.qc.sdk.audio.ssr",ssr_enabled,"0");
    if (!strncmp("true", ssr_enabled, 4)) {
        ALOGD("%s: surround sound recording is supported", __func__);
        ssrmod.is_ssr_enabled = true;
    } else {
        ALOGD("%s: surround sound recording is not supported", __func__);
        ssrmod.is_ssr_enabled = false;
    }
}

bool audio_extn_ssr_get_enabled()
{
    ALOGV("%s: is_ssr_enabled:%d is_ssr_mode_on:%d ", __func__, ssrmod.is_ssr_enabled, ssrmod.is_ssr_mode_on);

    if(ssrmod.is_ssr_enabled && ssrmod.is_ssr_mode_on)
        return true;

    return false;
}

int audio_extn_ssr_check_and_set_usecase(struct stream_in *in)
{
    int ret = -1;
    int channel_count = audio_channel_count_from_in_mask(in->channel_mask);
    audio_devices_t devices = in->device;
    audio_source_t source = in->source;

    /* validate input params
     * only stereo and 5:1 channel config is supported
     * only AUDIO_DEVICE_IN_BUILTIN_MIC, AUDIO_DEVICE_IN_BACK_MIC supports 3 mics */
    if (audio_extn_ssr_get_enabled() &&
           ((channel_count == 2) || (channel_count == 6)) &&
           ((AUDIO_SOURCE_MIC == source) || (AUDIO_SOURCE_CAMCORDER == source)) &&
           ((AUDIO_DEVICE_IN_BUILTIN_MIC == devices) || (AUDIO_DEVICE_IN_BACK_MIC == devices)) &&
           (in->format == AUDIO_FORMAT_PCM_16_BIT)) {

        ALOGD("%s: Found SSR use case starting SSR lib with channel_count :%d",
                      __func__, channel_count);

        if (!audio_extn_ssr_init(in, channel_count)) {
            ALOGD("%s: Created SSR session succesfully", __func__);
            ret = 0;
        } else {
            ALOGE("%s: Unable to start SSR record session", __func__);
        }
   }
   return ret;
}

static void pcm_buffer_queue_push(struct pcm_buffer_queue **queue,
                                  struct pcm_buffer_queue *node)
{
    struct pcm_buffer_queue *iter;

    node->next = NULL;
    if ((*queue) == NULL) {
        *queue = node;
    } else {
        iter = *queue;
        while (iter->next) {
            iter = iter->next;
        }
        iter->next = node;
    }
}

static struct pcm_buffer_queue *pcm_buffer_queue_pop(struct pcm_buffer_queue **queue)
{
    struct pcm_buffer_queue *node = (*queue);
    if (node != NULL) {
        *queue = node->next;
        node->next = NULL;
    }
    return node;
}

static void deinit_ssr_process_thread()
{
    pthread_mutex_lock(&ssrmod.ssr_process_lock);
    ssrmod.ssr_process_thread_stop = 1;

    if(ssrmod.in_buf_data != NULL)
       free(ssrmod.in_buf_data);
    ssrmod.in_buf_data = NULL;
    ssrmod.in_buf = NULL;
    ssrmod.in_buf_free = NULL;

    if(ssrmod.out_buf_data != NULL)
       free(ssrmod.out_buf_data);
    ssrmod.out_buf_data = NULL;
    ssrmod.out_buf = NULL;
    ssrmod.out_buf_free = NULL;
    pthread_cond_broadcast(&ssrmod.cond_process);
    pthread_cond_broadcast(&ssrmod.cond_read);
    pthread_mutex_unlock(&ssrmod.ssr_process_lock);
    if (ssrmod.ssr_process_thread_started) {
        pthread_join(ssrmod.ssr_process_thread, (void **)NULL);
        ssrmod.ssr_process_thread_started = 0;
    }
}

struct stream_in *audio_extn_ssr_get_stream()
{
    return ssrmod.in;
}

int32_t audio_extn_ssr_init(struct stream_in *in, int num_out_chan)
{
    uint32_t ret = -1;
    char c_multi_ch_dump[128] = {0};
    uint32_t buffer_size;

    ALOGD("%s: ssr case, sample rate %d", __func__, in->config.rate);

    if (ssrmod.surround_obj != NULL) {
        ALOGV("%s: reinitializing surround sound library", __func__);
        audio_extn_ssr_deinit();
    }

    if (audio_extn_ssr_get_enabled()) {
        ssrmod.ssr_3mic = 1;
    } else {
        ALOGE(" Rejecting SSR -- init is called without enabling SSR");
        goto fail;
    }

    /* buffer size equals to  period_size * period_count */     
    buffer_size = SSR_PERIOD_SIZE * NUM_IN_CHANNELS * sizeof(int16_t);
    ALOGV("%s: buffer_size: %d", __func__, buffer_size);

    if (ssrmod.ssr_3mic != 0) {
        ret = ssr_init_surround_sound_3mic_lib(buffer_size, NUM_IN_CHANNELS, num_out_chan, in->config.rate);
        if (0 != ret) {
            ALOGE("%s: ssr_init_surround_sound_3mic_lib failed: %d  "
                  "buffer_size:%d", __func__, ret, buffer_size);
            goto fail;
        }
    }

    /* Initialize DRC if available */
    ret = drc_init_lib(num_out_chan, in->config.rate);
    if (0 != ret) {
        ALOGE("%s: drc_init_lib failed, ret %d", __func__, ret);
    }

    pthread_mutex_lock(&ssrmod.ssr_process_lock);
    if (!ssrmod.ssr_process_thread_started) {
        int i;
        int output_buf_size = SSR_PERIOD_SIZE * sizeof(int16_t) * num_out_chan;

        ssrmod.in_buf_data = (void *)calloc(buffer_size, NUM_IN_BUFS);
        if (ssrmod.in_buf_data == NULL) {
            ALOGE("%s: failed to allocate input buffer", __func__);
            pthread_mutex_unlock(&ssrmod.ssr_process_lock);
            ret = -ENOMEM;
            goto fail;
        }
        ssrmod.out_buf_data = (void *)calloc(output_buf_size, NUM_OUT_BUFS);
        if (ssrmod.out_buf_data == NULL) {
            ALOGE("%s: failed to allocate output buffer", __func__);
            pthread_mutex_unlock(&ssrmod.ssr_process_lock);
            ret = -ENOMEM;
            // ssrmod.in_buf_data will be freed in deinit_ssr_process_thread()
            goto fail;
        }

        ssrmod.in_buf = NULL;
        ssrmod.in_buf_free = NULL;
        ssrmod.out_buf = NULL;
        ssrmod.out_buf_free = NULL;

        for (i=0; i < NUM_IN_BUFS; i++) {
            struct pcm_buffer_queue *buf = &ssrmod.in_buf_nodes[i];
            buf->buffer.data = &(((char *)ssrmod.in_buf_data)[i*buffer_size]);
            buf->buffer.length = buffer_size;
            pcm_buffer_queue_push(&ssrmod.in_buf_free, buf);
        }

        for (i=0; i < NUM_OUT_BUFS; i++) {
            struct pcm_buffer_queue *buf = &ssrmod.out_buf_nodes[i];
            buf->buffer.data = &(((char *)ssrmod.out_buf_data)[i*output_buf_size]);
            buf->buffer.length = output_buf_size;
            pcm_buffer_queue_push(&ssrmod.out_buf, buf);
        }

        ssrmod.ssr_process_thread_stop = 0;
        ALOGV("%s: creating thread", __func__);
        ret = pthread_create(&ssrmod.ssr_process_thread,
                             (const pthread_attr_t *) NULL,
                             ssr_process_thread, NULL);
        if (ret != 0) {
            ALOGE("%s: failed to create thread for surround sound recording.",
                  __func__);
            pthread_mutex_unlock(&ssrmod.ssr_process_lock);
            goto fail;
        }

        ssrmod.ssr_process_thread_started = 1;
        ALOGV("%s: done creating thread", __func__);
    }

    in->config.channels = NUM_IN_CHANNELS;
    in->config.period_size = SSR_PERIOD_SIZE;
    in->config.period_count = in->config.channels * sizeof(int16_t);

    pthread_mutex_unlock(&ssrmod.ssr_process_lock);

    property_get("ssr.pcmdump",c_multi_ch_dump,"0");
    if (0 == strncmp("true", c_multi_ch_dump, sizeof("ssr.dump-pcm"))) {
        /* Remember to change file system permission of data(e.g. chmod 777 data/),
          otherwise, fopen may fail */
        if ( !ssrmod.fp_input) {
            ALOGD("%s: Opening ssr input dump file \n", __func__);
            ssrmod.fp_input = fopen("/data/misc/audio/ssr_input_3ch.pcm", "wb");
        }

        if ( !ssrmod.fp_output) {
            if(ssrmod.num_out_chan == 6) {
                ALOGD("%s: Opening ssr input dump file for 6 channel\n", __func__);
                ssrmod.fp_output = fopen("/data/misc/audio/ssr_output_6ch.pcm", "wb");
            } else {
                ALOGD("%s: Opening ssr input dump file for 2 channel\n", __func__);
                ssrmod.fp_output = fopen("/data/misc/audio/ssr_output_2ch.pcm", "wb");
            }
        }

        if ((!ssrmod.fp_input) || (!ssrmod.fp_output)) {
            ALOGE("%s: input dump or ouput dump open failed: mfp_4ch:%p mfp_6ch:%p",
                  __func__, ssrmod.fp_input, ssrmod.fp_output);
        }
    }

    ssrmod.in = in;

    ALOGV("%s: exit", __func__);
    return 0;

fail:
    (void) audio_extn_ssr_deinit();
    return ret;
}

int32_t audio_extn_ssr_deinit()
{
    ALOGV("%s: entry", __func__);
    deinit_ssr_process_thread();

    if (ssrmod.drc_obj) {
        ssrmod.drc_deinit(ssrmod.drc_obj);
        ssrmod.drc_obj = NULL;
    }

    if (ssrmod.surround_obj) {

        if (ssrmod.ssr_3mic) {
            ssrmod.surround_rec_deinit(ssrmod.surround_obj);
            ssrmod.surround_obj = NULL;
        }
        if (ssrmod.surround_raw_buffer) {
            free(ssrmod.surround_raw_buffer);
            ssrmod.surround_raw_buffer = NULL;
        }
        if (ssrmod.fp_input)
            fclose(ssrmod.fp_input);
        if (ssrmod.fp_output)
            fclose(ssrmod.fp_output);
    }

    if(ssrmod.drc_handle) {
        dlclose(ssrmod.drc_handle);
        ssrmod.drc_handle = NULL;
    }

    if(ssrmod.surround_rec_handle) {
        dlclose(ssrmod.surround_rec_handle);
        ssrmod.surround_rec_handle = NULL;
    }

    ssrmod.in = NULL;
    //SSR session can be closed due to device switch
    //Do not force reset ssr mode

    //ssrmod.is_ssr_mode_on = false;
    ALOGV("%s: exit", __func__);

    return 0;
}

static void *ssr_process_thread(void *context __unused)
{
    int32_t ret;

    ALOGV("%s: enter", __func__);

    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_URGENT_AUDIO);
    set_sched_policy(0, SP_FOREGROUND);

    pthread_mutex_lock(&ssrmod.ssr_process_lock);
    while (!ssrmod.ssr_process_thread_stop) {
        struct pcm_buffer_queue *out_buf;
        struct pcm_buffer_queue *in_buf;

        while ((!ssrmod.ssr_process_thread_stop) &&
               ((ssrmod.out_buf_free == NULL) ||
                (ssrmod.in_buf == NULL))) {
            ALOGV("%s: waiting for buffers", __func__);
            pthread_cond_wait(&ssrmod.cond_process, &ssrmod.ssr_process_lock);
        }
        if (ssrmod.ssr_process_thread_stop) {
            break;
        }
        ALOGV("%s: got buffers", __func__);

        out_buf = pcm_buffer_queue_pop(&ssrmod.out_buf_free);
        in_buf = pcm_buffer_queue_pop(&ssrmod.in_buf);

        pthread_mutex_unlock(&ssrmod.ssr_process_lock);

        /* apply ssr libs to convert 4ch to 6ch */
        if (ssrmod.ssr_3mic) {
            ssrmod.surround_rec_process(ssrmod.surround_obj,
                (int16_t *) in_buf->buffer.data,
                (int16_t *) out_buf->buffer.data);
        }

        /* Run DRC if initialized */
        if (ssrmod.drc_obj != NULL) {
            ALOGV("%s: Running DRC", __func__);
            ret = ssrmod.drc_process(ssrmod.drc_obj, out_buf->buffer.data, out_buf->buffer.data);
            if (ret != 0) {
                ALOGE("%s: drc_process returned %d", __func__, ret);
            }
        }

        /*dump for raw pcm data*/
        if (ssrmod.fp_input)
            fwrite(in_buf->buffer.data, 1, in_buf->buffer.length, ssrmod.fp_input);
        if (ssrmod.fp_output)
            fwrite(out_buf->buffer.data, 1, out_buf->buffer.length, ssrmod.fp_output);

        pthread_mutex_lock(&ssrmod.ssr_process_lock);

        pcm_buffer_queue_push(&ssrmod.out_buf, out_buf);
        pcm_buffer_queue_push(&ssrmod.in_buf_free, in_buf);

        /* Read thread should go on without waiting for condition
         * variable. If it has to wait (due to this thread not keeping
         * up with the read requests), let this thread use the remainder
         * of its buffers before waking up the read thread. */
        if (ssrmod.in_buf == NULL) {
            pthread_cond_signal(&ssrmod.cond_read);
        }
    }
    pthread_mutex_unlock(&ssrmod.ssr_process_lock);

    ALOGV("%s: exit", __func__);

    pthread_exit(NULL);
}

int32_t audio_extn_ssr_read(struct audio_stream_in *stream,
                       void *buffer, size_t bytes)
{
    struct stream_in *in = (struct stream_in *)stream;
    int32_t ret = 0;
    struct pcm_buffer_queue *in_buf;
    struct pcm_buffer_queue *out_buf;

    ALOGV("%s: entry", __func__);

    if (!ssrmod.surround_obj) {
        ALOGE("%s: surround_obj not initialized", __func__);
        return -ENOMEM;
    }

    ret = pcm_read(in->pcm, ssrmod.surround_raw_buffer, ssrmod.surround_raw_buffer_size);
    if (ret < 0) {
        ALOGE("%s: %s ret:%d", __func__, pcm_get_error(in->pcm),ret);
        return ret;
    }

    pthread_mutex_lock(&ssrmod.ssr_process_lock);

    if (!ssrmod.ssr_process_thread_started) {
        pthread_mutex_unlock(&ssrmod.ssr_process_lock);
        ALOGV("%s: ssr_process_thread not initialized", __func__);
        return -EINVAL;
    }

    if ((ssrmod.in_buf_free == NULL) || (ssrmod.out_buf == NULL)) {
        ALOGE("%s: waiting for buffers", __func__);
        pthread_cond_wait(&ssrmod.cond_read, &ssrmod.ssr_process_lock);
        if ((ssrmod.in_buf_free == NULL) || (ssrmod.out_buf == NULL)) {
            pthread_mutex_unlock(&ssrmod.ssr_process_lock);
            ALOGE("%s: failed to acquire buffers", __func__);
            return -EINVAL;
        }
    }

    in_buf = pcm_buffer_queue_pop(&ssrmod.in_buf_free);
    out_buf = pcm_buffer_queue_pop(&ssrmod.out_buf);

    memcpy(in_buf->buffer.data, ssrmod.surround_raw_buffer, in_buf->buffer.length);
    pcm_buffer_queue_push(&ssrmod.in_buf, in_buf);

    memcpy(buffer, out_buf->buffer.data, bytes);
    pcm_buffer_queue_push(&ssrmod.out_buf_free, out_buf);

    pthread_cond_signal(&ssrmod.cond_process);

    pthread_mutex_unlock(&ssrmod.ssr_process_lock);

    ALOGV("%s: exit", __func__);
    return ret;
}

void audio_extn_ssr_set_parameters(struct audio_device *adev __unused,
                                   struct str_parms *parms)
{
    int err;
    char value[4096] = {0};

    //Do not update SSR mode during recording
    if ( !ssrmod.surround_obj) {
        int ret = 0;
        ret = str_parms_get_str(parms, AUDIO_PARAMETER_SSRMODE_ON, value,
                                sizeof(value));
        if (ret >= 0) {
            if (strcmp(value, "true") == 0) {
                ALOGD("Received SSR session request..setting SSR mode to true");
                ssrmod.is_ssr_mode_on = true;
            } else {
                ALOGD("resetting SSR mode to false");
                ssrmod.is_ssr_mode_on = false;
            }
        }
    }
    if (ssrmod.ssr_3mic && ssrmod.surround_obj) {
        const set_param_data_t *set_params = ssrmod.surround_rec_get_set_param_data();
        if (set_params != NULL) {
            while (set_params->name != NULL && set_params->set_param_fn != NULL) {
                err = str_parms_get_str(parms, set_params->name, value, sizeof(value));
                if (err >= 0) {
                    ALOGV("Set %s to %s\n", set_params->name, value);
                    set_params->set_param_fn(ssrmod.surround_obj, value);
                }
                set_params++;
            }
        }
    }
}

void audio_extn_ssr_get_parameters(const struct audio_device *adev __unused,
                                   struct str_parms *parms,
                                   struct str_parms *reply)
{
    int err;
    char value[4096] = {0};

    if (ssrmod.ssr_3mic && ssrmod.surround_obj) {
        const get_param_data_t *get_params = ssrmod.surround_rec_get_get_param_data();
        int get_all = 0;
        err = str_parms_get_str(parms, "ssr.all", value, sizeof(value));
        if (err >= 0) {
            get_all = 1;
        }
        if (get_params != NULL) {
            while (get_params->name != NULL && get_params->get_param_fn != NULL) {
                err = str_parms_get_str(parms, get_params->name, value, sizeof(value));
                if (get_all || (err >= 0)) {
                    ALOGV("Getting parameter %s", get_params->name);
                    char *val = get_params->get_param_fn(ssrmod.surround_obj);
                    if (val != NULL) {
                        str_parms_add_str(reply, get_params->name, val);
                        free(val);
                    }
                }
                get_params++;
            }
        }
    }
}

#endif /* SSR_ENABLED */
