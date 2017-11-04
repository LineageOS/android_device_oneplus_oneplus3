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
#include "platform_lib_log_util.h"
#include "platform_lib_macros.h"

char * get_timestamp(char *str, unsigned long buf_size)
{
  struct timeval tv;
  struct timezone tz;
  int hh, mm, ss;
  gettimeofday(&tv, &tz);
  hh = tv.tv_sec/3600%24;
  mm = (tv.tv_sec%3600)/60;
  ss = tv.tv_sec%60;
  snprintf(str, buf_size, "%02d:%02d:%02d.%06ld", hh, mm, ss, tv.tv_usec);
  return str;
}

// Below are the location conf file paths
#ifdef __ANDROID__

#define LOC_PATH_GPS_CONF_STR      "/vendor/etc/gps.conf"
#define LOC_PATH_IZAT_CONF_STR     "/vendor/etc/izat.conf"
#define LOC_PATH_FLP_CONF_STR      "/vendor/etc/flp.conf"
#define LOC_PATH_LOWI_CONF_STR     "/vendor/etc/lowi.conf"
#define LOC_PATH_SAP_CONF_STR      "/vendor/etc/sap.conf"
#define LOC_PATH_APDR_CONF_STR     "/vendor/etc/apdr.conf"
#define LOC_PATH_XTWIFI_CONF_STR   "/vendor/etc/xtwifi.conf"
#define LOC_PATH_QUIPC_CONF_STR    "/vendor/etc/quipc.conf"

#else

#define LOC_PATH_GPS_CONF_STR      "/etc/gps.conf"
#define LOC_PATH_IZAT_CONF_STR     "/etc/izat.conf"
#define LOC_PATH_FLP_CONF_STR      "/etc/flp.conf"
#define LOC_PATH_LOWI_CONF_STR     "/etc/lowi.conf"
#define LOC_PATH_SAP_CONF_STR      "/etc/sap.conf"
#define LOC_PATH_APDR_CONF_STR     "/etc/apdr.conf"
#define LOC_PATH_XTWIFI_CONF_STR   "/etc/xtwifi.conf"
#define LOC_PATH_QUIPC_CONF_STR    "/etc/quipc.conf"

#endif // __ANDROID__

// Reference below arrays wherever needed to avoid duplicating
// same conf path string over and again in location code.
const char LOC_PATH_GPS_CONF[]    = LOC_PATH_GPS_CONF_STR;
const char LOC_PATH_IZAT_CONF[]   = LOC_PATH_IZAT_CONF_STR;
const char LOC_PATH_FLP_CONF[]    = LOC_PATH_FLP_CONF_STR;
const char LOC_PATH_LOWI_CONF[]   = LOC_PATH_LOWI_CONF_STR;
const char LOC_PATH_SAP_CONF[]    = LOC_PATH_SAP_CONF_STR;
const char LOC_PATH_APDR_CONF[]   = LOC_PATH_APDR_CONF_STR;
const char LOC_PATH_XTWIFI_CONF[] = LOC_PATH_XTWIFI_CONF_STR;
const char LOC_PATH_QUIPC_CONF[]  = LOC_PATH_QUIPC_CONF_STR;
