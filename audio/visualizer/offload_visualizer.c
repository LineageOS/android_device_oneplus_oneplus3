/*
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

#define LOG_TAG "offload_visualizer"
/*#define LOG_NDEBUG 0*/
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/prctl.h>
#include <dlfcn.h>

#include <cutils/list.h>
#include <cutils/log.h>
#include <system/thread_defs.h>
#include <tinyalsa/asoundlib.h>
#include <audio_effects/effect_visualizer.h>

#define LIB_ACDB_LOADER "libacdbloader.so"
#define ACDB_DEV_TYPE_OUT 1
#define AFE_PROXY_ACDB_ID 45

static void* acdb_handle;

typedef void (*acdb_send_audio_cal_t)(int, int);

acdb_send_audio_cal_t acdb_send_audio_cal;

enum {
    EFFECT_STATE_UNINITIALIZED,
    EFFECT_STATE_INITIALIZED,
    EFFECT_STATE_ACTIVE,
};

typedef struct effect_context_s effect_context_t;
typedef struct output_context_s output_context_t;

/* effect specific operations. Only the init() and process() operations must be defined.
 * Others are optional.
 */
typedef struct effect_ops_s {
    int (*init)(effect_context_t *context);
    int (*release)(effect_context_t *context);
    int (*reset)(effect_context_t *context);
    int (*enable)(effect_context_t *context);
    int (*disable)(effect_context_t *context);
    int (*start)(effect_context_t *context, output_context_t *output);
    int (*stop)(effect_context_t *context, output_context_t *output);
    int (*process)(effect_context_t *context, audio_buffer_t *in, audio_buffer_t *out);
    int (*set_parameter)(effect_context_t *context, effect_param_t *param, uint32_t size);
    int (*get_parameter)(effect_context_t *context, effect_param_t *param, uint32_t *size);
    int (*command)(effect_context_t *context, uint32_t cmdCode, uint32_t cmdSize,
            void *pCmdData, uint32_t *replySize, void *pReplyData);
} effect_ops_t;

struct effect_context_s {
    const struct effect_interface_s *itfe;
    struct listnode effects_list_node;  /* node in created_effects_list */
    struct listnode output_node;  /* node in output_context_t.effects_list */
    effect_config_t config;
    const effect_descriptor_t *desc;
    audio_io_handle_t out_handle;  /* io handle of the output the effect is attached to */
    uint32_t state;
    bool offload_enabled;  /* when offload is enabled we process VISUALIZER_CMD_CAPTURE command.
                              Otherwise non offloaded visualizer has already processed the command
                              and we must not overwrite the reply. */
    effect_ops_t ops;
};

typedef struct output_context_s {
    struct listnode outputs_list_node;  /* node in active_outputs_list */
    audio_io_handle_t handle; /* io handle */
    struct listnode effects_list; /* list of effects attached to this output */
} output_context_t;


/* maximum time since last capture buffer update before resetting capture buffer. This means
  that the framework has stopped playing audio and we must start returning silence */
#define MAX_STALL_TIME_MS 1000

#define CAPTURE_BUF_SIZE 65536 /* "64k should be enough for everyone" */

#define DISCARD_MEASUREMENTS_TIME_MS 2000 /* discard measurements older than this number of ms */

/* maximum number of buffers for which we keep track of the measurements */
#define MEASUREMENT_WINDOW_MAX_SIZE_IN_BUFFERS 25 /* note: buffer index is stored in uint8_t */

typedef struct buffer_stats_s {
    bool is_valid;
    uint16_t peak_u16; /* the positive peak of the absolute value of the samples in a buffer */
    float rms_squared; /* the average square of the samples in a buffer */
} buffer_stats_t;

typedef struct visualizer_context_s {
    effect_context_t common;

    uint32_t capture_idx;
    uint32_t capture_size;
    uint32_t scaling_mode;
    uint32_t last_capture_idx;
    uint32_t latency;
    struct timespec buffer_update_time;
    uint8_t capture_buf[CAPTURE_BUF_SIZE];
    /* for measurements */
    uint8_t channel_count; /* to avoid recomputing it every time a buffer is processed */
    uint32_t meas_mode;
    uint8_t meas_wndw_size_in_buffers;
    uint8_t meas_buffer_idx;
    buffer_stats_t past_meas[MEASUREMENT_WINDOW_MAX_SIZE_IN_BUFFERS];
} visualizer_context_t;


extern const struct effect_interface_s effect_interface;

/* Offload visualizer UUID: 7a8044a0-1a71-11e3-a184-0002a5d5c51b */
const effect_descriptor_t visualizer_descriptor = {
        {0xe46b26a0, 0xdddd, 0x11db, 0x8afd, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}},
        {0x7a8044a0, 0x1a71, 0x11e3, 0xa184, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}},
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_HW_ACC_TUNNEL ),
        0, /* TODO */
        1,
        "QCOM MSM offload visualizer",
        "The Android Open Source Project",
};

const effect_descriptor_t *descriptors[] = {
        &visualizer_descriptor,
        NULL,
};


pthread_once_t once = PTHREAD_ONCE_INIT;
int init_status;

/* list of created effects. Updated by visualizer_hal_start_output()
 * and visualizer_hal_stop_output() */
struct listnode created_effects_list;
/* list of active output streams. Updated by visualizer_hal_start_output()
 * and visualizer_hal_stop_output() */
struct listnode active_outputs_list;

/* thread capturing PCM from Proxy port and calling the process function on each enabled effect
 * attached to an active output stream */
pthread_t capture_thread;
/* lock must be held when modifying or accessing created_effects_list or active_outputs_list */
pthread_mutex_t lock;
/* thread_lock must be held when starting or stopping the capture thread.
 * Locking order: thread_lock -> lock */
pthread_mutex_t thread_lock;
/* cond is signaled when an output is started or stopped or an effect is enabled or disable: the
 * capture thread will reevaluate the capture and effect rocess conditions. */
pthread_cond_t cond;
/* true when requesting the capture thread to exit */
bool exit_thread;
/* 0 if the capture thread was created successfully */
int thread_status;


#define DSP_OUTPUT_LATENCY_MS 0 /* Fudge factor for latency after capture point in audio DSP */

/* Retry for delay for mixer open */
#define RETRY_NUMBER 10
#define RETRY_US 500000

#define MIXER_CARD 0
#define SOUND_CARD 0
#define CAPTURE_DEVICE 8

/* Proxy port supports only MMAP read and those fixed parameters*/
#define AUDIO_CAPTURE_CHANNEL_COUNT 2
#define AUDIO_CAPTURE_SMP_RATE 48000
#define AUDIO_CAPTURE_PERIOD_SIZE (768)
#define AUDIO_CAPTURE_PERIOD_COUNT 32

struct pcm_config pcm_config_capture = {
    .channels = AUDIO_CAPTURE_CHANNEL_COUNT,
    .rate = AUDIO_CAPTURE_SMP_RATE,
    .period_size = AUDIO_CAPTURE_PERIOD_SIZE,
    .period_count = AUDIO_CAPTURE_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = AUDIO_CAPTURE_PERIOD_SIZE / 4,
    .stop_threshold = INT_MAX,
    .avail_min = AUDIO_CAPTURE_PERIOD_SIZE / 4,
};


/*
 *  Local functions
 */

static void init_once() {
    list_init(&created_effects_list);
    list_init(&active_outputs_list);

    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&thread_lock, NULL);
    pthread_cond_init(&cond, NULL);
    exit_thread = false;
    thread_status = -1;

    init_status = 0;
}

int lib_init() {
    pthread_once(&once, init_once);
    return init_status;
}

bool effect_exists(effect_context_t *context) {
    struct listnode *node;

    list_for_each(node, &created_effects_list) {
        effect_context_t *fx_ctxt = node_to_item(node,
                                                     effect_context_t,
                                                     effects_list_node);
        if (fx_ctxt == context) {
            return true;
        }
    }
    return false;
}

output_context_t *get_output(audio_io_handle_t output) {
    struct listnode *node;

    list_for_each(node, &active_outputs_list) {
        output_context_t *out_ctxt = node_to_item(node,
                                                  output_context_t,
                                                  outputs_list_node);
        if (out_ctxt->handle == output) {
            return out_ctxt;
        }
    }
    return NULL;
}

void add_effect_to_output(output_context_t * output, effect_context_t *context) {
    struct listnode *fx_node;

    list_for_each(fx_node, &output->effects_list) {
        effect_context_t *fx_ctxt = node_to_item(fx_node,
                                                     effect_context_t,
                                                     output_node);
        if (fx_ctxt == context)
            return;
    }
    list_add_tail(&output->effects_list, &context->output_node);
    if (context->ops.start)
        context->ops.start(context, output);
}

void remove_effect_from_output(output_context_t * output, effect_context_t *context) {
    struct listnode *fx_node;

    list_for_each(fx_node, &output->effects_list) {
        effect_context_t *fx_ctxt = node_to_item(fx_node,
                                                     effect_context_t,
                                                     output_node);
        if (fx_ctxt == context) {
            if (context->ops.stop)
                context->ops.stop(context, output);
            list_remove(&context->output_node);
            return;
        }
    }
}

bool effects_enabled() {
    struct listnode *out_node;

    list_for_each(out_node, &active_outputs_list) {
        struct listnode *fx_node;
        output_context_t *out_ctxt = node_to_item(out_node,
                                                  output_context_t,
                                                  outputs_list_node);

        list_for_each(fx_node, &out_ctxt->effects_list) {
            effect_context_t *fx_ctxt = node_to_item(fx_node,
                                                         effect_context_t,
                                                         output_node);
            if (fx_ctxt->state == EFFECT_STATE_ACTIVE && fx_ctxt->ops.process != NULL)
                return true;
        }
    }
    return false;
}

int set_control(const char* name, struct mixer *mixer, int value) {
    struct mixer_ctl *ctl;

    ctl = mixer_get_ctl_by_name(mixer, name);
    if (ctl == NULL) {
        ALOGW("%s: could not get %s ctl", __func__, name);
        return -EINVAL;
    }
    if (mixer_ctl_set_value(ctl, 0, value) != 0) {
        ALOGW("%s: error setting value %d on %s ", __func__, value, name);
        return -EINVAL;
    }

    return 0;
}

int configure_proxy_capture(struct mixer *mixer, int value) {
    int retval = 0;

    if (value && acdb_send_audio_cal)
        acdb_send_audio_cal(AFE_PROXY_ACDB_ID, ACDB_DEV_TYPE_OUT);

    retval = set_control("AFE_PCM_RX Audio Mixer MultiMedia4", mixer, value);

    if (retval != 0)
        return retval;

    // Extending visualizer to capture for compress2 path as well.
    // for extending it to multiple offload either this needs to be extended
    // or need to find better solution to enable only active offload sessions

    retval = set_control("AFE_PCM_RX Audio Mixer MultiMedia7", mixer, value);
    if (retval != 0)
        return retval;

    return 0;
}


void *capture_thread_loop(void *arg)
{
    int16_t data[AUDIO_CAPTURE_PERIOD_SIZE * AUDIO_CAPTURE_CHANNEL_COUNT * sizeof(int16_t)];
    audio_buffer_t buf;
    buf.frameCount = AUDIO_CAPTURE_PERIOD_SIZE;
    buf.s16 = data;
    bool capture_enabled = false;
    struct mixer *mixer;
    struct pcm *pcm = NULL;
    int ret;
    int retry_num = 0;

    ALOGD("thread enter");

    prctl(PR_SET_NAME, (unsigned long)"visualizer capture", 0, 0, 0);

    pthread_mutex_lock(&lock);

    mixer = mixer_open(MIXER_CARD);
    while (mixer == NULL && retry_num < RETRY_NUMBER) {
        usleep(RETRY_US);
        mixer = mixer_open(MIXER_CARD);
        retry_num++;
    }
    if (mixer == NULL) {
        pthread_mutex_unlock(&lock);
        return NULL;
    }

    for (;;) {
        if (exit_thread) {
            break;
        }
        if (effects_enabled()) {
            if (!capture_enabled) {
                ret = configure_proxy_capture(mixer, 1);
                if (ret == 0) {
                    pcm = pcm_open(SOUND_CARD, CAPTURE_DEVICE,
                                   PCM_IN|PCM_MMAP|PCM_NOIRQ, &pcm_config_capture);
                    if (pcm && !pcm_is_ready(pcm)) {
                        ALOGW("%s: %s", __func__, pcm_get_error(pcm));
                        pcm_close(pcm);
                        pcm = NULL;
                        configure_proxy_capture(mixer, 0);
                    } else {
                        capture_enabled = true;
                        ALOGD("%s: capture ENABLED", __func__);
                    }
                }
            }
        } else {
            if (capture_enabled) {
                if (pcm != NULL)
                    pcm_close(pcm);
                configure_proxy_capture(mixer, 0);
                ALOGD("%s: capture DISABLED", __func__);
                capture_enabled = false;
            }
            pthread_cond_wait(&cond, &lock);
        }
        if (!capture_enabled)
            continue;

        pthread_mutex_unlock(&lock);
        ret = pcm_mmap_read(pcm, data, sizeof(data));
        pthread_mutex_lock(&lock);

        if (ret == 0) {
            struct listnode *out_node;

            list_for_each(out_node, &active_outputs_list) {
                output_context_t *out_ctxt = node_to_item(out_node,
                                                          output_context_t,
                                                          outputs_list_node);
                struct listnode *fx_node;

                list_for_each(fx_node, &out_ctxt->effects_list) {
                    effect_context_t *fx_ctxt = node_to_item(fx_node,
                                                                effect_context_t,
                                                                output_node);
                    if (fx_ctxt->ops.process != NULL)
                        fx_ctxt->ops.process(fx_ctxt, &buf, &buf);
                }
            }
        } else {
            ALOGW("%s: read status %d %s", __func__, ret, pcm_get_error(pcm));
        }
    }

    if (capture_enabled) {
        if (pcm != NULL)
            pcm_close(pcm);
        configure_proxy_capture(mixer, 0);
    }
    mixer_close(mixer);
    pthread_mutex_unlock(&lock);

    ALOGD("thread exit");

    return NULL;
}

/*
 * Interface from audio HAL
 */

__attribute__ ((visibility ("default")))
int visualizer_hal_start_output(audio_io_handle_t output, int pcm_id) {
    int ret;
    struct listnode *node;

    ALOGV("%s output %d pcm_id %d", __func__, output, pcm_id);

    if (lib_init() != 0)
        return init_status;

    pthread_mutex_lock(&thread_lock);
    pthread_mutex_lock(&lock);
    if (get_output(output) != NULL) {
        ALOGW("%s output already started", __func__);
        ret = -ENOSYS;
        goto exit;
    }

    output_context_t *out_ctxt = (output_context_t *)malloc(sizeof(output_context_t));
    if (out_ctxt == NULL) {
        ALOGE("%s fail to allocate memory", __func__);
        ret = -ENOMEM;
        goto exit;
    }
    out_ctxt->handle = output;
    list_init(&out_ctxt->effects_list);

    list_for_each(node, &created_effects_list) {
        effect_context_t *fx_ctxt = node_to_item(node,
                                                     effect_context_t,
                                                     effects_list_node);
        if (fx_ctxt->out_handle == output) {
            if (fx_ctxt->ops.start)
                fx_ctxt->ops.start(fx_ctxt, out_ctxt);
            list_add_tail(&out_ctxt->effects_list, &fx_ctxt->output_node);
        }
    }
    if (list_empty(&active_outputs_list)) {
        exit_thread = false;
        thread_status = pthread_create(&capture_thread, (const pthread_attr_t *) NULL,
                        capture_thread_loop, NULL);
    }
    list_add_tail(&active_outputs_list, &out_ctxt->outputs_list_node);
    pthread_cond_signal(&cond);

exit:
    pthread_mutex_unlock(&lock);
    pthread_mutex_unlock(&thread_lock);
    return ret;
}

__attribute__ ((visibility ("default")))
int visualizer_hal_stop_output(audio_io_handle_t output, int pcm_id) {
    int ret;
    struct listnode *node;
    struct listnode *fx_node;
    output_context_t *out_ctxt;

    ALOGV("%s output %d pcm_id %d", __func__, output, pcm_id);

    if (lib_init() != 0)
        return init_status;

    pthread_mutex_lock(&thread_lock);
    pthread_mutex_lock(&lock);

    out_ctxt = get_output(output);
    if (out_ctxt == NULL) {
        ALOGW("%s output not started", __func__);
        ret = -ENOSYS;
        goto exit;
    }
    list_for_each(fx_node, &out_ctxt->effects_list) {
        effect_context_t *fx_ctxt = node_to_item(fx_node,
                                                 effect_context_t,
                                                 output_node);
        if (fx_ctxt->ops.stop)
            fx_ctxt->ops.stop(fx_ctxt, out_ctxt);
    }
    list_remove(&out_ctxt->outputs_list_node);
    pthread_cond_signal(&cond);

    if (list_empty(&active_outputs_list)) {
        if (thread_status == 0) {
            exit_thread = true;
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&lock);
            pthread_join(capture_thread, (void **) NULL);
            pthread_mutex_lock(&lock);
            thread_status = -1;
        }
    }

    free(out_ctxt);

exit:
    pthread_mutex_unlock(&lock);
    pthread_mutex_unlock(&thread_lock);
    return ret;
}


/*
 * Effect operations
 */

int set_config(effect_context_t *context, effect_config_t *config)
{
    if (config->inputCfg.samplingRate != config->outputCfg.samplingRate) return -EINVAL;
    if (config->inputCfg.channels != config->outputCfg.channels) return -EINVAL;
    if (config->inputCfg.format != config->outputCfg.format) return -EINVAL;
    if (config->inputCfg.channels != AUDIO_CHANNEL_OUT_STEREO) return -EINVAL;
    if (config->outputCfg.accessMode != EFFECT_BUFFER_ACCESS_WRITE &&
            config->outputCfg.accessMode != EFFECT_BUFFER_ACCESS_ACCUMULATE) return -EINVAL;
    if (config->inputCfg.format != AUDIO_FORMAT_PCM_16_BIT) return -EINVAL;

    context->config = *config;

    if (context->ops.reset)
        context->ops.reset(context);

    return 0;
}

void get_config(effect_context_t *context, effect_config_t *config)
{
    *config = context->config;
}


/*
 * Visualizer operations
 */

uint32_t visualizer_get_delta_time_ms_from_updated_time(visualizer_context_t* visu_ctxt) {
    uint32_t delta_ms = 0;
    if (visu_ctxt->buffer_update_time.tv_sec != 0) {
        struct timespec ts;
        if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
            time_t secs = ts.tv_sec - visu_ctxt->buffer_update_time.tv_sec;
            long nsec = ts.tv_nsec - visu_ctxt->buffer_update_time.tv_nsec;
            if (nsec < 0) {
                --secs;
                nsec += 1000000000;
            }
            delta_ms = secs * 1000 + nsec / 1000000;
        }
    }
    return delta_ms;
}

int visualizer_reset(effect_context_t *context)
{
    visualizer_context_t * visu_ctxt = (visualizer_context_t *)context;

    visu_ctxt->capture_idx = 0;
    visu_ctxt->last_capture_idx = 0;
    visu_ctxt->buffer_update_time.tv_sec = 0;
    visu_ctxt->latency = DSP_OUTPUT_LATENCY_MS;
    memset(visu_ctxt->capture_buf, 0x80, CAPTURE_BUF_SIZE);
    return 0;
}

int visualizer_init(effect_context_t *context)
{
    int32_t i;

    visualizer_context_t * visu_ctxt = (visualizer_context_t *)context;

    context->config.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
    context->config.inputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    context->config.inputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    context->config.inputCfg.samplingRate = 44100;
    context->config.inputCfg.bufferProvider.getBuffer = NULL;
    context->config.inputCfg.bufferProvider.releaseBuffer = NULL;
    context->config.inputCfg.bufferProvider.cookie = NULL;
    context->config.inputCfg.mask = EFFECT_CONFIG_ALL;
    context->config.outputCfg.accessMode = EFFECT_BUFFER_ACCESS_ACCUMULATE;
    context->config.outputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    context->config.outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    context->config.outputCfg.samplingRate = 44100;
    context->config.outputCfg.bufferProvider.getBuffer = NULL;
    context->config.outputCfg.bufferProvider.releaseBuffer = NULL;
    context->config.outputCfg.bufferProvider.cookie = NULL;
    context->config.outputCfg.mask = EFFECT_CONFIG_ALL;

    visu_ctxt->capture_size = VISUALIZER_CAPTURE_SIZE_MAX;
    visu_ctxt->scaling_mode = VISUALIZER_SCALING_MODE_NORMALIZED;

    // measurement initialization
    visu_ctxt->channel_count = popcount(context->config.inputCfg.channels);
    visu_ctxt->meas_mode = MEASUREMENT_MODE_NONE;
    visu_ctxt->meas_wndw_size_in_buffers = MEASUREMENT_WINDOW_MAX_SIZE_IN_BUFFERS;
    visu_ctxt->meas_buffer_idx = 0;
    for (i=0 ; i<visu_ctxt->meas_wndw_size_in_buffers ; i++) {
        visu_ctxt->past_meas[i].is_valid = false;
        visu_ctxt->past_meas[i].peak_u16 = 0;
        visu_ctxt->past_meas[i].rms_squared = 0;
    }

    set_config(context, &context->config);

    if (acdb_handle == NULL) {
        acdb_handle = dlopen(LIB_ACDB_LOADER, RTLD_NOW);
        if (acdb_handle == NULL) {
            ALOGE("%s: DLOPEN failed for %s", __func__, LIB_ACDB_LOADER);
        } else {
            acdb_send_audio_cal = (acdb_send_audio_cal_t)dlsym(acdb_handle,
                                                    "acdb_loader_send_audio_cal");
            if (!acdb_send_audio_cal)
                ALOGE("%s: Could not find the symbol acdb_send_audio_cal from %s",
                      __func__, LIB_ACDB_LOADER);
            }
    }

    return 0;
}

int visualizer_get_parameter(effect_context_t *context, effect_param_t *p, uint32_t *size)
{
    visualizer_context_t *visu_ctxt = (visualizer_context_t *)context;

    p->status = 0;
    *size = sizeof(effect_param_t) + sizeof(uint32_t);
    if (p->psize != sizeof(uint32_t)) {
        p->status = -EINVAL;
        return 0;
    }
    switch (*(uint32_t *)p->data) {
    case VISUALIZER_PARAM_CAPTURE_SIZE:
        ALOGV("%s get capture_size = %d", __func__, visu_ctxt->capture_size);
        *((uint32_t *)p->data + 1) = visu_ctxt->capture_size;
        p->vsize = sizeof(uint32_t);
        *size += sizeof(uint32_t);
        break;
    case VISUALIZER_PARAM_SCALING_MODE:
        ALOGV("%s get scaling_mode = %d", __func__, visu_ctxt->scaling_mode);
        *((uint32_t *)p->data + 1) = visu_ctxt->scaling_mode;
        p->vsize = sizeof(uint32_t);
        *size += sizeof(uint32_t);
        break;
    case VISUALIZER_PARAM_MEASUREMENT_MODE:
        ALOGV("%s get meas_mode = %d", __func__, visu_ctxt->meas_mode);
        *((uint32_t *)p->data + 1) = visu_ctxt->meas_mode;
        p->vsize = sizeof(uint32_t);
        *size += sizeof(uint32_t);
        break;
    default:
        p->status = -EINVAL;
    }
    return 0;
}

int visualizer_set_parameter(effect_context_t *context, effect_param_t *p, uint32_t size)
{
    visualizer_context_t *visu_ctxt = (visualizer_context_t *)context;

    if (p->psize != sizeof(uint32_t) || p->vsize != sizeof(uint32_t))
        return -EINVAL;

    switch (*(uint32_t *)p->data) {
    case VISUALIZER_PARAM_CAPTURE_SIZE:
        visu_ctxt->capture_size = *((uint32_t *)p->data + 1);
        ALOGV("%s set capture_size = %d", __func__, visu_ctxt->capture_size);
        break;
    case VISUALIZER_PARAM_SCALING_MODE:
        visu_ctxt->scaling_mode = *((uint32_t *)p->data + 1);
        ALOGV("%s set scaling_mode = %d", __func__, visu_ctxt->scaling_mode);
        break;
    case VISUALIZER_PARAM_LATENCY:
        /* Ignore latency as we capture at DSP output
         * visu_ctxt->latency = *((uint32_t *)p->data + 1); */
        ALOGV("%s set latency = %d", __func__, visu_ctxt->latency);
        break;
    case VISUALIZER_PARAM_MEASUREMENT_MODE:
        visu_ctxt->meas_mode = *((uint32_t *)p->data + 1);
        ALOGV("%s set meas_mode = %d", __func__, visu_ctxt->meas_mode);
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

/* Real process function called from capture thread. Called with lock held */
int visualizer_process(effect_context_t *context,
                       audio_buffer_t *inBuffer,
                       audio_buffer_t *outBuffer)
{
    visualizer_context_t *visu_ctxt = (visualizer_context_t *)context;

    if (!effect_exists(context))
        return -EINVAL;

    if (inBuffer == NULL || inBuffer->raw == NULL ||
        outBuffer == NULL || outBuffer->raw == NULL ||
        inBuffer->frameCount != outBuffer->frameCount ||
        inBuffer->frameCount == 0) {
        return -EINVAL;
    }

    // perform measurements if needed
    if (visu_ctxt->meas_mode & MEASUREMENT_MODE_PEAK_RMS) {
        // find the peak and RMS squared for the new buffer
        uint32_t inIdx;
        int16_t max_sample = 0;
        float rms_squared_acc = 0;
        for (inIdx = 0 ; inIdx < inBuffer->frameCount * visu_ctxt->channel_count ; inIdx++) {
            if (inBuffer->s16[inIdx] > max_sample) {
                max_sample = inBuffer->s16[inIdx];
            } else if (-inBuffer->s16[inIdx] > max_sample) {
                max_sample = -inBuffer->s16[inIdx];
            }
            rms_squared_acc += (inBuffer->s16[inIdx] * inBuffer->s16[inIdx]);
        }
        // store the measurement
        visu_ctxt->past_meas[visu_ctxt->meas_buffer_idx].peak_u16 = (uint16_t)max_sample;
        visu_ctxt->past_meas[visu_ctxt->meas_buffer_idx].rms_squared =
                rms_squared_acc / (inBuffer->frameCount * visu_ctxt->channel_count);
        visu_ctxt->past_meas[visu_ctxt->meas_buffer_idx].is_valid = true;
        if (++visu_ctxt->meas_buffer_idx >= visu_ctxt->meas_wndw_size_in_buffers) {
            visu_ctxt->meas_buffer_idx = 0;
        }
    }

    /* all code below assumes stereo 16 bit PCM output and input */
    int32_t shift;

    if (visu_ctxt->scaling_mode == VISUALIZER_SCALING_MODE_NORMALIZED) {
        /* derive capture scaling factor from peak value in current buffer
         * this gives more interesting captures for display. */
        shift = 32;
        int len = inBuffer->frameCount * 2;
        int i;
        for (i = 0; i < len; i++) {
            int32_t smp = inBuffer->s16[i];
            if (smp < 0) smp = -smp - 1; /* take care to keep the max negative in range */
            int32_t clz = __builtin_clz(smp);
            if (shift > clz) shift = clz;
        }
        /* A maximum amplitude signal will have 17 leading zeros, which we want to
         * translate to a shift of 8 (for converting 16 bit to 8 bit) */
        shift = 25 - shift;
        /* Never scale by less than 8 to avoid returning unaltered PCM signal. */
        if (shift < 3) {
            shift = 3;
        }
        /* add one to combine the division by 2 needed after summing
         * left and right channels below */
        shift++;
    } else {
        assert(visu_ctxt->scaling_mode == VISUALIZER_SCALING_MODE_AS_PLAYED);
        shift = 9;
    }

    uint32_t capt_idx;
    uint32_t in_idx;
    uint8_t *buf = visu_ctxt->capture_buf;
    for (in_idx = 0, capt_idx = visu_ctxt->capture_idx;
         in_idx < inBuffer->frameCount;
         in_idx++, capt_idx++) {
        if (capt_idx >= CAPTURE_BUF_SIZE) {
            /* wrap around */
            capt_idx = 0;
        }
        int32_t smp = inBuffer->s16[2 * in_idx] + inBuffer->s16[2 * in_idx + 1];
        smp = smp >> shift;
        buf[capt_idx] = ((uint8_t)smp)^0x80;
    }

    /* XXX the following two should really be atomic, though it probably doesn't
     * matter much for visualization purposes */
    visu_ctxt->capture_idx = capt_idx;
    /* update last buffer update time stamp */
    if (clock_gettime(CLOCK_MONOTONIC, &visu_ctxt->buffer_update_time) < 0) {
        visu_ctxt->buffer_update_time.tv_sec = 0;
    }

    if (context->state != EFFECT_STATE_ACTIVE) {
        ALOGV("%s DONE inactive", __func__);
        return -ENODATA;
    }

    return 0;
}

int visualizer_command(effect_context_t * context, uint32_t cmdCode, uint32_t cmdSize,
        void *pCmdData, uint32_t *replySize, void *pReplyData)
{
    visualizer_context_t * visu_ctxt = (visualizer_context_t *)context;

    switch (cmdCode) {
    case VISUALIZER_CMD_CAPTURE:
        if (pReplyData == NULL || *replySize != visu_ctxt->capture_size) {
            ALOGV("%s VISUALIZER_CMD_CAPTURE error *replySize %d context->capture_size %d",
                  __func__, *replySize, visu_ctxt->capture_size);
            return -EINVAL;
        }

        if (!context->offload_enabled)
            break;

        if (context->state == EFFECT_STATE_ACTIVE) {
            int32_t latency_ms = visu_ctxt->latency;
            const uint32_t delta_ms = visualizer_get_delta_time_ms_from_updated_time(visu_ctxt);
            latency_ms -= delta_ms;
            if (latency_ms < 0) {
                latency_ms = 0;
            }
            const uint32_t delta_smp = context->config.inputCfg.samplingRate * latency_ms / 1000;

            int32_t capture_point = visu_ctxt->capture_idx - visu_ctxt->capture_size - delta_smp;
            int32_t capture_size = visu_ctxt->capture_size;
            if (capture_point < 0) {
                int32_t size = -capture_point;
                if (size > capture_size)
                    size = capture_size;

                memcpy(pReplyData,
                       visu_ctxt->capture_buf + CAPTURE_BUF_SIZE + capture_point,
                       size);
                pReplyData = (void *)((size_t)pReplyData + size);
                capture_size -= size;
                capture_point = 0;
            }
            memcpy(pReplyData,
                   visu_ctxt->capture_buf + capture_point,
                   capture_size);


            /* if audio framework has stopped playing audio although the effect is still
             * active we must clear the capture buffer to return silence */
            if ((visu_ctxt->last_capture_idx == visu_ctxt->capture_idx) &&
                    (visu_ctxt->buffer_update_time.tv_sec != 0)) {
                if (delta_ms > MAX_STALL_TIME_MS) {
                    ALOGV("%s capture going to idle", __func__);
                    visu_ctxt->buffer_update_time.tv_sec = 0;
                    memset(pReplyData, 0x80, visu_ctxt->capture_size);
                }
            }
            visu_ctxt->last_capture_idx = visu_ctxt->capture_idx;
        } else {
            memset(pReplyData, 0x80, visu_ctxt->capture_size);
        }
        break;

    case VISUALIZER_CMD_MEASURE: {
        uint16_t peak_u16 = 0;
        float sum_rms_squared = 0.0f;
        uint8_t nb_valid_meas = 0;
        /* reset measurements if last measurement was too long ago (which implies stored
         * measurements aren't relevant anymore and shouldn't bias the new one) */
        const int32_t delay_ms = visualizer_get_delta_time_ms_from_updated_time(visu_ctxt);
        if (delay_ms > DISCARD_MEASUREMENTS_TIME_MS) {
            uint32_t i;
            ALOGV("Discarding measurements, last measurement is %dms old", delay_ms);
            for (i=0 ; i<visu_ctxt->meas_wndw_size_in_buffers ; i++) {
                visu_ctxt->past_meas[i].is_valid = false;
                visu_ctxt->past_meas[i].peak_u16 = 0;
                visu_ctxt->past_meas[i].rms_squared = 0;
            }
            visu_ctxt->meas_buffer_idx = 0;
        } else {
            /* only use actual measurements, otherwise the first RMS measure happening before
             * MEASUREMENT_WINDOW_MAX_SIZE_IN_BUFFERS have been played will always be artificially
             * low */
            uint32_t i;
            for (i=0 ; i < visu_ctxt->meas_wndw_size_in_buffers ; i++) {
                if (visu_ctxt->past_meas[i].is_valid) {
                    if (visu_ctxt->past_meas[i].peak_u16 > peak_u16) {
                        peak_u16 = visu_ctxt->past_meas[i].peak_u16;
                    }
                    sum_rms_squared += visu_ctxt->past_meas[i].rms_squared;
                    nb_valid_meas++;
                }
            }
        }
        float rms = nb_valid_meas == 0 ? 0.0f : sqrtf(sum_rms_squared / nb_valid_meas);
        int32_t* p_int_reply_data = (int32_t*)pReplyData;
        /* convert from I16 sample values to mB and write results */
        if (rms < 0.000016f) {
            p_int_reply_data[MEASUREMENT_IDX_RMS] = -9600; //-96dB
        } else {
            p_int_reply_data[MEASUREMENT_IDX_RMS] = (int32_t) (2000 * log10(rms / 32767.0f));
        }
        if (peak_u16 == 0) {
            p_int_reply_data[MEASUREMENT_IDX_PEAK] = -9600; //-96dB
        } else {
            p_int_reply_data[MEASUREMENT_IDX_PEAK] = (int32_t) (2000 * log10(peak_u16 / 32767.0f));
        }
        ALOGV("VISUALIZER_CMD_MEASURE peak=%d (%dmB), rms=%.1f (%dmB)",
                peak_u16, p_int_reply_data[MEASUREMENT_IDX_PEAK],
                rms, p_int_reply_data[MEASUREMENT_IDX_RMS]);
        }
        break;

    default:
        ALOGW("%s invalid command %d", __func__, cmdCode);
        return -EINVAL;
    }
    return 0;
}


/*
 * Effect Library Interface Implementation
 */

int effect_lib_create(const effect_uuid_t *uuid,
                         int32_t sessionId,
                         int32_t ioId,
                         effect_handle_t *pHandle) {
    int ret;
    int i;

    if (lib_init() != 0)
        return init_status;

    if (pHandle == NULL || uuid == NULL)
        return -EINVAL;

    for (i = 0; descriptors[i] != NULL; i++) {
        if (memcmp(uuid, &descriptors[i]->uuid, sizeof(effect_uuid_t)) == 0)
            break;
    }

    if (descriptors[i] == NULL)
        return -EINVAL;

    effect_context_t *context;
    if (memcmp(uuid, &visualizer_descriptor.uuid, sizeof(effect_uuid_t)) == 0) {
        visualizer_context_t *visu_ctxt = (visualizer_context_t *)calloc(1,
                                                                     sizeof(visualizer_context_t));
        if (visu_ctxt == NULL) {
            ALOGE("%s fail to allocate memory", __func__);
            return -ENOMEM;
        }
        context = (effect_context_t *)visu_ctxt;
        context->ops.init = visualizer_init;
        context->ops.reset = visualizer_reset;
        context->ops.process = visualizer_process;
        context->ops.set_parameter = visualizer_set_parameter;
        context->ops.get_parameter = visualizer_get_parameter;
        context->ops.command = visualizer_command;
        context->desc = &visualizer_descriptor;
    } else {
        return -EINVAL;
    }

    context->itfe = &effect_interface;
    context->state = EFFECT_STATE_UNINITIALIZED;
    context->out_handle = (audio_io_handle_t)ioId;

    ret = context->ops.init(context);
    if (ret < 0) {
        ALOGW("%s init failed", __func__);
        free(context);
        return ret;
    }

    context->state = EFFECT_STATE_INITIALIZED;

    pthread_mutex_lock(&lock);
    list_add_tail(&created_effects_list, &context->effects_list_node);
    output_context_t *out_ctxt = get_output(ioId);
    if (out_ctxt != NULL)
        add_effect_to_output(out_ctxt, context);
    pthread_mutex_unlock(&lock);

    *pHandle = (effect_handle_t)context;

    ALOGV("%s created context %p", __func__, context);

    return 0;

}

int effect_lib_release(effect_handle_t handle) {
    effect_context_t *context = (effect_context_t *)handle;
    int status;

    if (lib_init() != 0)
        return init_status;

    ALOGV("%s context %p", __func__, handle);
    pthread_mutex_lock(&lock);
    status = -EINVAL;
    if (effect_exists(context)) {
        output_context_t *out_ctxt = get_output(context->out_handle);
        if (out_ctxt != NULL)
            remove_effect_from_output(out_ctxt, context);
        list_remove(&context->effects_list_node);
        if (context->ops.release)
            context->ops.release(context);
        free(context);
        status = 0;
    }
    pthread_mutex_unlock(&lock);

    return status;
}

int effect_lib_get_descriptor(const effect_uuid_t *uuid,
                                effect_descriptor_t *descriptor) {
    int i;

    if (lib_init() != 0)
        return init_status;

    if (descriptor == NULL || uuid == NULL) {
        ALOGV("%s called with NULL pointer", __func__);
        return -EINVAL;
    }

    for (i = 0; descriptors[i] != NULL; i++) {
        if (memcmp(uuid, &descriptors[i]->uuid, sizeof(effect_uuid_t)) == 0) {
            *descriptor = *descriptors[i];
            return 0;
        }
    }

    return  -EINVAL;
}

/*
 * Effect Control Interface Implementation
 */

 /* Stub function for effect interface: never called for offloaded effects */
int effect_process(effect_handle_t self,
                       audio_buffer_t *inBuffer,
                       audio_buffer_t *outBuffer)
{
    effect_context_t * context = (effect_context_t *)self;
    int status = 0;

    ALOGW("%s Called ?????", __func__);

    pthread_mutex_lock(&lock);
    if (!effect_exists(context)) {
        status = -EINVAL;
        goto exit;
    }

    if (context->state != EFFECT_STATE_ACTIVE) {
        status = -EINVAL;
        goto exit;
    }

exit:
    pthread_mutex_unlock(&lock);
    return status;
}

int effect_command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize,
        void *pCmdData, uint32_t *replySize, void *pReplyData)
{

    effect_context_t * context = (effect_context_t *)self;
    int retsize;
    int status = 0;

    pthread_mutex_lock(&lock);

    if (!effect_exists(context)) {
        status = -EINVAL;
        goto exit;
    }

    if (context == NULL || context->state == EFFECT_STATE_UNINITIALIZED) {
        status = -EINVAL;
        goto exit;
    }

//    ALOGV_IF(cmdCode != VISUALIZER_CMD_CAPTURE,
//             "%s command %d cmdSize %d", __func__, cmdCode, cmdSize);

    switch (cmdCode) {
    case EFFECT_CMD_INIT:
        if (pReplyData == NULL || *replySize != sizeof(int)) {
            status = -EINVAL;
            goto exit;
        }
        if (context->ops.init)
            *(int *) pReplyData = context->ops.init(context);
        else
            *(int *) pReplyData = 0;
        break;
    case EFFECT_CMD_SET_CONFIG:
        if (pCmdData == NULL || cmdSize != sizeof(effect_config_t)
                || pReplyData == NULL || *replySize != sizeof(int)) {
            status = -EINVAL;
            goto exit;
        }
        *(int *) pReplyData = set_config(context, (effect_config_t *) pCmdData);
        break;
    case EFFECT_CMD_GET_CONFIG:
        if (pReplyData == NULL ||
            *replySize != sizeof(effect_config_t)) {
            status = -EINVAL;
            goto exit;
        }
        if (!context->offload_enabled) {
            status = -EINVAL;
            goto exit;
        }

        get_config(context, (effect_config_t *)pReplyData);
        break;
    case EFFECT_CMD_RESET:
        if (context->ops.reset)
            context->ops.reset(context);
        break;
    case EFFECT_CMD_ENABLE:
        if (pReplyData == NULL || *replySize != sizeof(int)) {
            status = -EINVAL;
            goto exit;
        }
        if (context->state != EFFECT_STATE_INITIALIZED) {
            status = -ENOSYS;
            goto exit;
        }
        context->state = EFFECT_STATE_ACTIVE;
        if (context->ops.enable)
            context->ops.enable(context);
        pthread_cond_signal(&cond);
        ALOGV("%s EFFECT_CMD_ENABLE", __func__);
        *(int *)pReplyData = 0;
        break;
    case EFFECT_CMD_DISABLE:
        if (pReplyData == NULL || *replySize != sizeof(int)) {
            status = -EINVAL;
            goto exit;
        }
        if (context->state != EFFECT_STATE_ACTIVE) {
            status = -ENOSYS;
            goto exit;
        }
        context->state = EFFECT_STATE_INITIALIZED;
        if (context->ops.disable)
            context->ops.disable(context);
        pthread_cond_signal(&cond);
        ALOGV("%s EFFECT_CMD_DISABLE", __func__);
        *(int *)pReplyData = 0;
        break;
    case EFFECT_CMD_GET_PARAM: {
        if (pCmdData == NULL ||
            cmdSize != (int)(sizeof(effect_param_t) + sizeof(uint32_t)) ||
            pReplyData == NULL ||
            *replySize < (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(uint32_t))) {
            status = -EINVAL;
            goto exit;
        }
        if (!context->offload_enabled) {
            status = -EINVAL;
            goto exit;
        }
        memcpy(pReplyData, pCmdData, sizeof(effect_param_t) + sizeof(uint32_t));
        effect_param_t *p = (effect_param_t *)pReplyData;
        if (context->ops.get_parameter)
            context->ops.get_parameter(context, p, replySize);
        } break;
    case EFFECT_CMD_SET_PARAM: {
        if (pCmdData == NULL ||
            cmdSize != (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(uint32_t)) ||
            pReplyData == NULL || *replySize != sizeof(int32_t)) {
            status = -EINVAL;
            goto exit;
        }
        *(int32_t *)pReplyData = 0;
        effect_param_t *p = (effect_param_t *)pCmdData;
        if (context->ops.set_parameter)
            *(int32_t *)pReplyData = context->ops.set_parameter(context, p, *replySize);

        } break;
    case EFFECT_CMD_SET_DEVICE:
    case EFFECT_CMD_SET_VOLUME:
    case EFFECT_CMD_SET_AUDIO_MODE:
        break;

    case EFFECT_CMD_OFFLOAD: {
        output_context_t *out_ctxt;

        if (cmdSize != sizeof(effect_offload_param_t) || pCmdData == NULL
                || pReplyData == NULL || *replySize != sizeof(int)) {
            ALOGV("%s EFFECT_CMD_OFFLOAD bad format", __func__);
            status = -EINVAL;
            break;
        }

        effect_offload_param_t* offload_param = (effect_offload_param_t*)pCmdData;

        ALOGV("%s EFFECT_CMD_OFFLOAD offload %d output %d",
              __func__, offload_param->isOffload, offload_param->ioHandle);

        *(int *)pReplyData = 0;

        context->offload_enabled = offload_param->isOffload;
        if (context->out_handle == offload_param->ioHandle)
            break;

        out_ctxt = get_output(context->out_handle);
        if (out_ctxt != NULL)
            remove_effect_from_output(out_ctxt, context);

        context->out_handle = offload_param->ioHandle;
        out_ctxt = get_output(offload_param->ioHandle);
        if (out_ctxt != NULL)
            add_effect_to_output(out_ctxt, context);

        } break;


    default:
        if (cmdCode >= EFFECT_CMD_FIRST_PROPRIETARY && context->ops.command)
            status = context->ops.command(context, cmdCode, cmdSize,
                                          pCmdData, replySize, pReplyData);
        else {
            ALOGW("%s invalid command %d", __func__, cmdCode);
            status = -EINVAL;
        }
        break;
    }

exit:
    pthread_mutex_unlock(&lock);

//    ALOGV_IF(cmdCode != VISUALIZER_CMD_CAPTURE,"%s DONE", __func__);
    return status;
}

/* Effect Control Interface Implementation: get_descriptor */
int effect_get_descriptor(effect_handle_t   self,
                                    effect_descriptor_t *descriptor)
{
    effect_context_t *context = (effect_context_t *)self;

    if (!effect_exists(context))
        return -EINVAL;

    if (descriptor == NULL)
        return -EINVAL;

    *descriptor = *context->desc;

    return 0;
}

/* effect_handle_t interface implementation for visualizer effect */
const struct effect_interface_s effect_interface = {
        effect_process,
        effect_command,
        effect_get_descriptor,
        NULL,
};

__attribute__ ((visibility ("default")))
audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
    tag : AUDIO_EFFECT_LIBRARY_TAG,
    version : EFFECT_LIBRARY_API_VERSION,
    name : "Visualizer Library",
    implementor : "The Android Open Source Project",
    create_effect : effect_lib_create,
    release_effect : effect_lib_release,
    get_descriptor : effect_lib_get_descriptor,
};
