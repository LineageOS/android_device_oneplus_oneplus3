/*--------------------------------------------------------------------------
Copyright (c) 2010, 2016, The Linux Foundation. All rights reserved.

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>

#include <aenc_svr.h>

/**
 @brief This function processes posted messages

 Once thread is being spawned, this function is run to
 start processing commands posted by client

 @param info pointer to context

 */
void *omx_qcelp13_msg(void *info)
{
    struct qcelp13_ipc_info *qcelp13_info = (struct qcelp13_ipc_info*)info;
    unsigned char id;
    ssize_t n;

    DEBUG_DETAIL("\n%s: message thread start\n", __FUNCTION__);
    while (!qcelp13_info->dead)
    {
        n = read(qcelp13_info->pipe_in, &id, 1);
        if (0 == n) break;
        if (1 == n)
        {
          DEBUG_DETAIL("\n%s-->pipe_in=%d pipe_out=%d\n",
                                               qcelp13_info->thread_name,
                                               qcelp13_info->pipe_in,
                                               qcelp13_info->pipe_out);

            qcelp13_info->process_msg_cb(qcelp13_info->client_data, id);
        }
        if ((n < 0) && (errno != EINTR)) break;
    }
    DEBUG_DETAIL("%s: message thread stop\n", __FUNCTION__);

    return 0;
}

void *omx_qcelp13_events(void *info)
{
    struct qcelp13_ipc_info *qcelp13_info = (struct qcelp13_ipc_info*)info;
    unsigned char id = 0;

    DEBUG_DETAIL("%s: message thread start\n", qcelp13_info->thread_name);
    qcelp13_info->process_msg_cb(qcelp13_info->client_data, id);
    DEBUG_DETAIL("%s: message thread stop\n", qcelp13_info->thread_name);
    return 0;
}

/**
 @brief This function starts command server

 @param cb pointer to callback function from the client
 @param client_data reference client wants to get back
  through callback
 @return handle to msging thread
 */
struct qcelp13_ipc_info *omx_qcelp13_thread_create(
                                    message_func cb,
                                    void* client_data,
                                    char* th_name)
{
    int r;
    int fds[2];
    struct qcelp13_ipc_info *qcelp13_info;

    qcelp13_info = calloc(1, sizeof(struct qcelp13_ipc_info));
    if (!qcelp13_info)
    {
        return 0;
    }

    qcelp13_info->client_data = client_data;
    qcelp13_info->process_msg_cb = cb;
    strlcpy(qcelp13_info->thread_name, th_name,
			sizeof(qcelp13_info->thread_name));

    if (pipe(fds))
    {
        DEBUG_PRINT_ERROR("\n%s: pipe creation failed\n", __FUNCTION__);
        goto fail_pipe;
    }

    qcelp13_info->pipe_in = fds[0];
    qcelp13_info->pipe_out = fds[1];

    r = pthread_create(&qcelp13_info->thr, 0, omx_qcelp13_msg, qcelp13_info);
    if (r < 0) goto fail_thread;

    DEBUG_DETAIL("Created thread for %s \n", qcelp13_info->thread_name);
    return qcelp13_info;


fail_thread:
    close(qcelp13_info->pipe_in);
    close(qcelp13_info->pipe_out);

fail_pipe:
    free(qcelp13_info);

    return 0;
}

/**
 *  @brief This function starts command server
 *
 *   @param cb pointer to callback function from the client
 *    @param client_data reference client wants to get back
 *      through callback
 *       @return handle to msging thread
 *        */
struct qcelp13_ipc_info *omx_qcelp13_event_thread_create(
                                    message_func cb,
                                    void* client_data,
                                    char* th_name)
{
    int r;
    int fds[2];
    struct qcelp13_ipc_info *qcelp13_info;

    qcelp13_info = calloc(1, sizeof(struct qcelp13_ipc_info));
    if (!qcelp13_info)
    {
        return 0;
    }

    qcelp13_info->client_data = client_data;
    qcelp13_info->process_msg_cb = cb;
    strlcpy(qcelp13_info->thread_name, th_name,
		sizeof(qcelp13_info->thread_name));

    if (pipe(fds))
    {
        DEBUG_PRINT("\n%s: pipe creation failed\n", __FUNCTION__);
        goto fail_pipe;
    }

    qcelp13_info->pipe_in = fds[0];
    qcelp13_info->pipe_out = fds[1];

    r = pthread_create(&qcelp13_info->thr, 0, omx_qcelp13_events, qcelp13_info);
    if (r < 0) goto fail_thread;

    DEBUG_DETAIL("Created thread for %s \n", qcelp13_info->thread_name);
    return qcelp13_info;


fail_thread:
    close(qcelp13_info->pipe_in);
    close(qcelp13_info->pipe_out);

fail_pipe:
    free(qcelp13_info);

    return 0;
}

void omx_qcelp13_thread_stop(struct qcelp13_ipc_info *qcelp13_info) {
    DEBUG_DETAIL("%s stop server\n", __FUNCTION__);
    qcelp13_info->dead = 1;
    close(qcelp13_info->pipe_in);
    close(qcelp13_info->pipe_out);
    pthread_join(qcelp13_info->thr,NULL);
    qcelp13_info->pipe_out = -1;
    qcelp13_info->pipe_in = -1;
    DEBUG_DETAIL("%s: message thread close fds%d %d\n", qcelp13_info->thread_name,
        qcelp13_info->pipe_in,qcelp13_info->pipe_out);
    free(qcelp13_info);
}

void omx_qcelp13_post_msg(struct qcelp13_ipc_info *qcelp13_info, unsigned char id) {
    DEBUG_DETAIL("\n%s id=%d\n", __FUNCTION__,id);
    
    write(qcelp13_info->pipe_out, &id, 1);
}
