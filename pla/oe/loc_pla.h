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
 */

#ifndef __LOC_PLA__
#define __LOC_PLA__

#ifdef __cplusplus
#ifndef FEATURE_EXTERNAL_AP
#include <utils/SystemClock.h>
#endif /* FEATURE_EXTERNAL_AP */
#include <inttypes.h>
#include <sys/time.h>
#include <time.h>

inline int64_t uptimeMillis()
{
    struct timespec ts;
    int64_t time_ms = 0;
    clock_gettime(CLOCK_BOOTTIME, &ts);
    time_ms += (ts.tv_sec * 1000000000LL);
    time_ms += ts.tv_nsec + 500000LL;
    return time_ms / 1000000LL;
}

extern "C" {
#endif

#ifndef FEATURE_EXTERNAL_AP
#include <cutils/properties.h>
#include <cutils/threads.h>
#include <cutils/sched_policy.h>
#endif /* FEATURE_EXTERNAL_AP */
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#ifndef OFF_TARGET
#include <glib.h>
#define strlcat g_strlcat
#define strlcpy g_strlcpy
#else
#define strlcat strncat
#define strlcpy strncpy
#endif

#define UID_GPS (1021)
#define GID_GPS (1021)
#define UID_LOCCLIENT (4021)
#define GID_LOCCLIENT (4021)

#define LOC_PATH_GPS_CONF_STR      "/etc/gps.conf"
#define LOC_PATH_IZAT_CONF_STR     "/etc/izat.conf"
#define LOC_PATH_FLP_CONF_STR      "/etc/flp.conf"
#define LOC_PATH_LOWI_CONF_STR     "/etc/lowi.conf"
#define LOC_PATH_SAP_CONF_STR      "/etc/sap.conf"
#define LOC_PATH_APDR_CONF_STR     "/etc/apdr.conf"
#define LOC_PATH_XTWIFI_CONF_STR   "/etc/xtwifi.conf"
#define LOC_PATH_QUIPC_CONF_STR    "/etc/quipc.conf"

#ifdef FEATURE_EXTERNAL_AP
#define PROPERTY_VALUE_MAX 92

inline int property_get(const char* key, char* value, const char* default_value)
{
    strlcpy(value, default_value, PROPERTY_VALUE_MAX - 1);
    return strlen(value);
}
#endif /* FEATURE_EXTERNAL_AP */

#ifdef __cplusplus
}
#endif /*__cplusplus */

#endif /* __LOC_PLA__ */
