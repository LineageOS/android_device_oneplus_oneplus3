/* Copyright (c) 2011-2015, 2018 The Linux Foundation. All rights reserved.
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
 *     * Neither the name of The Linux Foundation, nor the names of its
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

#ifndef LOC_CFG_H
#define LOC_CFG_H

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>

#define LOC_MAX_PARAM_NAME                 80
#define LOC_MAX_PARAM_STRING               80
#define LOC_MAX_PARAM_LINE    (LOC_MAX_PARAM_NAME + LOC_MAX_PARAM_STRING)

#define LOC_FEATURE_MODE_DISABLED "DISABLED"
#define LOC_FEATURE_MODE_BASIC    "BASIC"
#define LOC_FEATURE_MODE_PREMIUM  "PREMIUM"

#define LOC_FEATURE_GTP_AP_CELL        "gtp-ap-cell"
#define LOC_FEATURE_GTP_MODEM_CELL     "gtp-modem-cell"
#define LOC_FEATURE_GTP_CELL_ENH       "gtp-cell-enh"
#define LOC_FEATURE_GTP_WIFI           "gtp-wifi"
#define LOC_FEATURE_GTP_WAA            "gtp-waa"
#define LOC_FEATURE_SAP                "sap"

#define LOC_PROCESS_MAX_NUM_GROUPS     20
#define LOC_PROCESS_MAX_NUM_ARGS       25
#define LOC_PROCESS_MAX_ARG_STR_LENGTH 32

#define UTIL_UPDATE_CONF(conf_data, len, config_table) \
    loc_update_conf((conf_data), (len), (config_table), \
                    sizeof(config_table) / sizeof(config_table[0]))

#define UTIL_READ_CONF_DEFAULT(filename) \
    loc_read_conf((filename), NULL, 0);

#define UTIL_READ_CONF(filename, config_table) \
    loc_read_conf((filename), (config_table), sizeof(config_table) / sizeof(config_table[0]))

/*=============================================================================
 *
 *                        MODULE TYPE DECLARATION
 *
 *============================================================================*/
typedef struct
{
  const char                    *param_name;
  void                          *param_ptr;
  uint8_t                       *param_set;   /* indicate value set by config file */
  char                           param_type;  /* 'n' for number,
                                                 's' for string,
                                                 'f' for double */
} loc_param_s_type;

typedef enum {
    ENABLED,
    RUNNING,
    DISABLED,
    DISABLED_FROM_CONF
} loc_process_e_status;

typedef struct {
    loc_process_e_status proc_status;
    pid_t                proc_id;
    char                 name[2][LOC_MAX_PARAM_STRING];
    gid_t                group_list[LOC_PROCESS_MAX_NUM_GROUPS];
    unsigned char        num_groups;
    char                 args[LOC_PROCESS_MAX_NUM_ARGS][LOC_PROCESS_MAX_ARG_STR_LENGTH];
    char                 argumentString[LOC_MAX_PARAM_STRING];
} loc_process_info_s_type;

/*=============================================================================
 *
 *                          MODULE EXTERNAL DATA
 *
 *============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 *
 *                       MODULE EXPORTED FUNCTIONS
 *
 *============================================================================*/
void loc_read_conf(const char* conf_file_name,
                   const loc_param_s_type* config_table,
                   uint32_t table_length);
int loc_read_conf_r(FILE *conf_fp, const loc_param_s_type* config_table,
                    uint32_t table_length);
int loc_update_conf(const char* conf_data, int32_t length,
                    const loc_param_s_type* config_table, uint32_t table_length);

// Below are the location conf file paths
extern const char LOC_PATH_GPS_CONF[];
extern const char LOC_PATH_IZAT_CONF[];
extern const char LOC_PATH_FLP_CONF[];
extern const char LOC_PATH_LOWI_CONF[];
extern const char LOC_PATH_SAP_CONF[];
extern const char LOC_PATH_APDR_CONF[];
extern const char LOC_PATH_XTWIFI_CONF[];
extern const char LOC_PATH_QUIPC_CONF[];

int loc_read_process_conf(const char* conf_file_name, uint32_t * process_count_ptr,
                          loc_process_info_s_type** process_info_table_ptr);

uint32_t loc_modem_emulator_enabled();

#ifdef __cplusplus
}
#endif

#endif /* LOC_CFG_H */
