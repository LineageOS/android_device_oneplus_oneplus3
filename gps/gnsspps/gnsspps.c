/* Copyright (c) 2011-2015, The Linux Foundation. All rights reserved.
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
 *     * Neither the name of The Linux Foundatoin, nor the names of its
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
#include <platform_lib_log_util.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <timepps.h>
#include <linux/types.h>

//DRsync kernel timestamp
static struct timespec drsyncKernelTs = {0,0};
//DRsync userspace timestamp
static struct timespec drsyncUserTs = {0,0};
//flag to stop fetching timestamp
static int isActive = 0;
static pps_handle handle;

static pthread_mutex_t ts_lock;

  /*  checks the PPS source and opens it */
int check_device(char *path, pps_handle *handle)
{
     int ret;

     /* Try to find the source by using the supplied "path" name */
     ret = open(path, O_RDWR);
     if (ret < 0)
     {
         LOC_LOGV("%s:%d unable to open device %s", __func__, __LINE__, path);
         return ret;
     }

     /* Open the PPS source */
     ret = pps_create(ret, handle);
     if (ret < 0)
     {
         LOC_LOGV( "%s:%d cannot create a PPS source from device %s", __func__, __LINE__, path);
         return -1;
     }
     return 0;
}

/* fetches the timestamp from the PPS source */
int read_pps(pps_handle *handle)
{
    struct timespec timeout;
    pps_info infobuf;
    int ret;
    // 3sec timeout
    timeout.tv_sec = 3;
    timeout.tv_nsec = 0;

       ret = pps_fetch(*handle, PPS_TSFMT_TSPEC, &infobuf,&timeout);

        if (ret < 0 && ret !=-EINTR)
        {
            LOC_LOGV("%s:%d pps_fetch() error %d", __func__, __LINE__,  ret);
            return -1;
        }

        pthread_mutex_lock(&ts_lock);
        drsyncKernelTs.tv_sec = infobuf.tv_sec;
        drsyncKernelTs.tv_nsec = infobuf.tv_nsec;
        ret = clock_gettime(CLOCK_BOOTTIME,&drsyncUserTs);
        pthread_mutex_unlock(&ts_lock);

        if(ret != 0)
        {
            LOC_LOGV("%s:%d clock_gettime() error",__func__,__LINE__);
        }
    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif
/* infinitely calls read_pps() */
void *thread_handle(void *input)
{
    int ret;
    if(input != NULL)
    {
        LOC_LOGV("%s:%d Thread Input is present", __func__, __LINE__);
    }
    while(isActive)
    {
        ret = read_pps(&handle);

        if (ret == -1 && errno != ETIMEDOUT )
        {
            LOC_LOGV("%s:%d Could not fetch PPS source", __func__, __LINE__);
        }
    }
    return NULL;
}

/*  opens the device and fetches from PPS source */
int initPPS(char *devname)
{
    int ret,pid;
    pthread_t thread;
    isActive = 1;

    ret = check_device(devname, &handle);
    if (ret < 0)
    {
        LOC_LOGV("%s:%d Could not find PPS source", __func__, __LINE__);
        return 0;
    }

    pthread_mutex_init(&ts_lock,NULL);

    pid = pthread_create(&thread,NULL,&thread_handle,NULL);
    if(pid != 0)
    {
        LOC_LOGV("%s:%d Could not create thread in InitPPS", __func__, __LINE__);
        return 0;
    }
    return 1;
}

/* stops fetching and closes the device */
void deInitPPS()
{
    pthread_mutex_lock(&ts_lock);
    isActive = 0;
    pthread_mutex_unlock(&ts_lock);

    pthread_mutex_destroy(&ts_lock);
    pps_destroy(handle);
}

/* retrieves DRsync kernel timestamp,DRsync userspace timestamp
   and updates current timestamp */
/* Returns:
 *     1. @Param out DRsync kernel timestamp
 *     2. @Param out DRsync userspace timestamp
 *     3. @Param out current timestamp
 */
int getPPS(struct timespec *fineKernelTs ,struct timespec *currentTs,
           struct timespec *fineUserTs)
{
    int ret;

    pthread_mutex_lock(&ts_lock);
    fineKernelTs->tv_sec = drsyncKernelTs.tv_sec;
    fineKernelTs->tv_nsec = drsyncKernelTs.tv_nsec;

    fineUserTs->tv_sec = drsyncUserTs.tv_sec;
    fineUserTs->tv_nsec = drsyncUserTs.tv_nsec;

    ret = clock_gettime(CLOCK_BOOTTIME,currentTs);
    pthread_mutex_unlock(&ts_lock);
    if(ret != 0)
    {
       LOC_LOGV("%s:%d clock_gettime() error",__func__,__LINE__);
       return 0;
    }
    return 1;
}

#ifdef __cplusplus
}
#endif

