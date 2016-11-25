/*--------------------------------------------------------------------------
Copyright (c) 2010, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of The Linux Foundation nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/
#ifndef AENC_SVR_H
#define AENC_SVR_H

#ifdef __cplusplus
extern "C" {
#endif
#include <pthread.h>
#include <sched.h>
#include <utils/Log.h>

#ifdef _ANDROID_
#define LOG_TAG "QC_EVRCENC"
#endif

#ifndef LOGE
#define LOGE ALOGE
#endif

#ifndef LOGW
#define LOGW ALOGW
#endif

#ifndef LOGD
#define LOGD ALOGD
#endif

#ifndef LOGV
#define LOGV ALOGV
#endif

#ifndef LOGI
#define LOGI ALOGI
#endif

#define DEBUG_PRINT_ERROR LOGE
#define DEBUG_PRINT       LOGI
#define DEBUG_DETAIL      LOGV

typedef void (*message_func)(void* client_data, unsigned char id);

/**
 @brief audio encoder ipc info structure

 */
struct evrc_ipc_info
{
    pthread_t thr;
    int pipe_in;
    int pipe_out;
    int dead;
    message_func process_msg_cb;
    void         *client_data;
    char         thread_name[128];
};

/**
 @brief This function starts command server

 @param cb pointer to callback function from the client
 @param client_data reference client wants to get back
  through callback
 @return handle to command server
 */
struct evrc_ipc_info *omx_evrc_thread_create(message_func cb,
    void* client_data,
    char *th_name);

struct evrc_ipc_info *omx_evrc_event_thread_create(message_func cb,
    void* client_data,
    char *th_name);
/**
 @brief This function stop command server

 @param svr handle to command server
 @return none
 */
void omx_evrc_thread_stop(struct evrc_ipc_info *evrc_ipc);


/**
 @brief This function post message in the command server

 @param svr handle to command server
 @return none
 */
void omx_evrc_post_msg(struct evrc_ipc_info *evrc_ipc,
                          unsigned char id);

void* omx_evrc_comp_timer_handler(void *);

#ifdef __cplusplus
}
#endif

#endif /* AENC_SVR */
