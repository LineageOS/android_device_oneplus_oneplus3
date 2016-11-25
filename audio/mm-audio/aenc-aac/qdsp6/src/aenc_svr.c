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
void *omx_aac_msg(void *info)
{
    struct aac_ipc_info *aac_info = (struct aac_ipc_info*)info;
    unsigned char id;
    ssize_t n;

    DEBUG_DETAIL("\n%s: message thread start\n", __FUNCTION__);
    while (!aac_info->dead)
    {
        n = read(aac_info->pipe_in, &id, 1);
        if (0 == n) break;
        if (1 == n)
        {
          DEBUG_DETAIL("\n%s-->pipe_in=%d pipe_out=%d\n",
                                               aac_info->thread_name,
                                               aac_info->pipe_in,
                                               aac_info->pipe_out);

            aac_info->process_msg_cb(aac_info->client_data, id);
        }
        if ((n < 0) && (errno != EINTR)) break;
    }
    DEBUG_DETAIL("%s: message thread stop\n", __FUNCTION__);

    return 0;
}

void *omx_aac_events(void *info)
{
    struct aac_ipc_info *aac_info = (struct aac_ipc_info*)info;
    unsigned char id = 0;

    DEBUG_DETAIL("%s: message thread start\n", aac_info->thread_name);
    aac_info->process_msg_cb(aac_info->client_data, id);
    DEBUG_DETAIL("%s: message thread stop\n", aac_info->thread_name);
    return 0;
}

/**
 @brief This function starts command server

 @param cb pointer to callback function from the client
 @param client_data reference client wants to get back
  through callback
 @return handle to msging thread
 */
struct aac_ipc_info *omx_aac_thread_create(
                                    message_func cb,
                                    void* client_data,
                                    char* th_name)
{
    int r;
    int fds[2];
    struct aac_ipc_info *aac_info;

    aac_info = calloc(1, sizeof(struct aac_ipc_info));
    if (!aac_info)
    {
        return 0;
    }

    aac_info->client_data = client_data;
    aac_info->process_msg_cb = cb;
    strlcpy(aac_info->thread_name, th_name, sizeof(aac_info->thread_name));

    if (pipe(fds))
    {
        DEBUG_PRINT_ERROR("\n%s: pipe creation failed\n", __FUNCTION__);
        goto fail_pipe;
    }

    aac_info->pipe_in = fds[0];
    aac_info->pipe_out = fds[1];
   
    r = pthread_create(&aac_info->thr, 0, omx_aac_msg, aac_info);
    if (r < 0) goto fail_thread;

    DEBUG_DETAIL("Created thread for %s \n", aac_info->thread_name);
    return aac_info;


fail_thread:
    close(aac_info->pipe_in);
    close(aac_info->pipe_out);

fail_pipe:
    free(aac_info);

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
struct aac_ipc_info *omx_aac_event_thread_create(
                                    message_func cb,
                                    void* client_data,
                                    char* th_name)
{
    int r;
    int fds[2];
    struct aac_ipc_info *aac_info;

    aac_info = calloc(1, sizeof(struct aac_ipc_info));
    if (!aac_info)
    {
        return 0;
    }

    aac_info->client_data = client_data;
    aac_info->process_msg_cb = cb;
    strlcpy(aac_info->thread_name, th_name, sizeof(aac_info->thread_name));

    if (pipe(fds))
    {
        DEBUG_PRINT("\n%s: pipe creation failed\n", __FUNCTION__);
        goto fail_pipe;
    }

    aac_info->pipe_in = fds[0];
    aac_info->pipe_out = fds[1];

    r = pthread_create(&aac_info->thr, 0, omx_aac_events, aac_info);
    if (r < 0) goto fail_thread;

    DEBUG_DETAIL("Created thread for %s \n", aac_info->thread_name);
    return aac_info;


fail_thread:
    close(aac_info->pipe_in);
    close(aac_info->pipe_out);

fail_pipe:
    free(aac_info);

    return 0;
}

void omx_aac_thread_stop(struct aac_ipc_info *aac_info) {
    DEBUG_DETAIL("%s stop server\n", __FUNCTION__);
    aac_info->dead = 1;
    close(aac_info->pipe_in);
    close(aac_info->pipe_out);
    pthread_join(aac_info->thr,NULL);
    aac_info->pipe_out = -1;
    aac_info->pipe_in = -1;
    DEBUG_DETAIL("%s: message thread close fds%d %d\n", aac_info->thread_name,
        aac_info->pipe_in,aac_info->pipe_out);
    free(aac_info);
}

void omx_aac_post_msg(struct aac_ipc_info *aac_info, unsigned char id) {
    DEBUG_DETAIL("\n%s id=%d\n", __FUNCTION__,id);
 
    write(aac_info->pipe_out, &id, 1);
}
