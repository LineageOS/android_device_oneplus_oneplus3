/* Copyright (c) 2014, The Linux Foundation. All rights reserved.
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

#ifndef AUDIO_EXTN_PM_H
#define AUDIO_EXTN_PM_H

#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pm-service.h>
#include "audio_hw.h"
#include <platform.h>
#include <cutils/properties.h>
#include <cutils/log.h>


/* Client name to be registered with PM */
#define PM_CLIENT_NAME "audio"
/* Command to sysfs to unload image */
#define UNLOAD_IMAGE "0"
#define MAX_NAME_LEN 32
#define BOOT_IMG_SYSFS_PATH "/sys/kernel/boot_adsp/boot"

typedef struct {
    //MAX_NAME_LEN defined in mdm_detect.h
    char img_name[MAX_NAME_LEN];
    //this handle is used by peripheral mgr
    void *pm_handle;
}s_audio_subsys;

/* Vote to peripheral manager for required subsystem */
int audio_extn_pm_vote (void);

/* Unvote to peripheral manager */
void audio_extn_pm_unvote (void);

/* Get subsytem status notification from PM */
void audio_extn_pm_event_notifier (void *client_data, enum pm_event event);

#endif // AUDIO_EXTN_PM_H
