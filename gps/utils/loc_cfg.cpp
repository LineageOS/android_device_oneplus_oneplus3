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

#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_utils_cfg"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <grp.h>
#include <errno.h>
#include <loc_cfg.h>
#include <loc_pla.h>
#include <loc_target.h>
#include <loc_misc_utils.h>
#ifdef USE_GLIB
#include <glib.h>
#endif
#include "log_util.h"

/*=============================================================================
 *
 *                          GLOBAL DATA DECLARATION
 *
 *============================================================================*/

/* Parameter data */
static uint32_t DEBUG_LEVEL = 0xff;
static uint32_t TIMESTAMP = 0;
static uint32_t DATUM_TYPE = 0;
static bool sVendorEnhanced = true;

/* Parameter spec table */
static const loc_param_s_type loc_param_table[] =
{
    {"DEBUG_LEVEL",        &DEBUG_LEVEL,        NULL,    'n'},
    {"TIMESTAMP",          &TIMESTAMP,          NULL,    'n'},
    {"DATUM_TYPE",         &DATUM_TYPE,         NULL,    'n'},
};
static const int loc_param_num = sizeof(loc_param_table) / sizeof(loc_param_s_type);

typedef struct loc_param_v_type
{
    char* param_name;
    char* param_str_value;
    int param_int_value;
    double param_double_value;
}loc_param_v_type;

// Reference below arrays wherever needed to avoid duplicating
// same conf path string over and again in location code.
const char LOC_PATH_GPS_CONF[] = LOC_PATH_GPS_CONF_STR;
const char LOC_PATH_IZAT_CONF[] = LOC_PATH_IZAT_CONF_STR;
const char LOC_PATH_FLP_CONF[] = LOC_PATH_FLP_CONF_STR;
const char LOC_PATH_LOWI_CONF[] = LOC_PATH_LOWI_CONF_STR;
const char LOC_PATH_SAP_CONF[] = LOC_PATH_SAP_CONF_STR;
const char LOC_PATH_APDR_CONF[] = LOC_PATH_APDR_CONF_STR;
const char LOC_PATH_XTWIFI_CONF[] = LOC_PATH_XTWIFI_CONF_STR;
const char LOC_PATH_QUIPC_CONF[] = LOC_PATH_QUIPC_CONF_STR;

bool isVendorEnhanced() {
    return sVendorEnhanced;
}
void setVendorEnhanced(bool vendorEnhanced) {
    sVendorEnhanced = vendorEnhanced;
}

/*===========================================================================
FUNCTION loc_get_datum_type

DESCRIPTION
   get datum type

PARAMETERS:
   N/A

DEPENDENCIES
   N/A

RETURN VALUE
   DATUM TYPE

SIDE EFFECTS
   N/A
===========================================================================*/
int loc_get_datum_type()
{
    return DATUM_TYPE;
}

/*===========================================================================
FUNCTION loc_set_config_entry

DESCRIPTION
   Potentially sets a given configuration table entry based on the passed in
   configuration value. This is done by using a string comparison of the
   parameter names and those found in the configuration file.

PARAMETERS:
   config_entry: configuration entry in the table to possibly set
   config_value: value to store in the entry if the parameter names match

DEPENDENCIES
   N/A

RETURN VALUE
   None

SIDE EFFECTS
   N/A
===========================================================================*/
int loc_set_config_entry(const loc_param_s_type* config_entry, loc_param_v_type* config_value)
{
    int ret=-1;
    if(NULL == config_entry || NULL == config_value)
    {
        LOC_LOGE("%s: INVALID config entry or parameter", __FUNCTION__);
        return ret;
    }

    if (strcmp(config_entry->param_name, config_value->param_name) == 0 &&
        config_entry->param_ptr)
    {
        switch (config_entry->param_type)
        {
        case 's':
            if (strcmp(config_value->param_str_value, "NULL") == 0)
            {
                *((char*)config_entry->param_ptr) = '\0';
            }
            else {
                strlcpy((char*) config_entry->param_ptr,
                        config_value->param_str_value,
                        LOC_MAX_PARAM_STRING);
            }
            /* Log INI values */
            LOC_LOGD("%s: PARAM %s = %s", __FUNCTION__,
                     config_entry->param_name, (char*)config_entry->param_ptr);

            if(NULL != config_entry->param_set)
            {
                *(config_entry->param_set) = 1;
            }
            ret = 0;
            break;
        case 'n':
            *((int *)config_entry->param_ptr) = config_value->param_int_value;
            /* Log INI values */
            LOC_LOGD("%s: PARAM %s = %d", __FUNCTION__,
                     config_entry->param_name, config_value->param_int_value);

            if(NULL != config_entry->param_set)
            {
                *(config_entry->param_set) = 1;
            }
            ret = 0;
            break;
        case 'f':
            *((double *)config_entry->param_ptr) = config_value->param_double_value;
            /* Log INI values */
            LOC_LOGD("%s: PARAM %s = %f", __FUNCTION__,
                     config_entry->param_name, config_value->param_double_value);

            if(NULL != config_entry->param_set)
            {
                *(config_entry->param_set) = 1;
            }
            ret = 0;
            break;
        default:
            LOC_LOGE("%s: PARAM %s parameter type must be n, f, or s",
                     __FUNCTION__, config_entry->param_name);
        }
    }
    return ret;
}

/*===========================================================================
FUNCTION loc_fill_conf_item

DESCRIPTION
   Takes a line of configuration item and sets defined values based on
   the passed in configuration table. This table maps strings to values to
   set along with the type of each of these values.

PARAMETERS:
   input_buf : buffer contanis config item
   config_table: table definition of strings to places to store information
   table_length: length of the configuration table

DEPENDENCIES
   N/A

RETURN VALUE
   0: Number of records in the config_table filled with input_buf

SIDE EFFECTS
   N/A
===========================================================================*/
int loc_fill_conf_item(char* input_buf,
                       const loc_param_s_type* config_table, uint32_t table_length)
{
    int ret = 0;

    if (input_buf && config_table) {
        char *lasts;
        loc_param_v_type config_value;
        memset(&config_value, 0, sizeof(config_value));

        /* Separate variable and value */
        config_value.param_name = strtok_r(input_buf, "=", &lasts);
        /* skip lines that do not contain "=" */
        if (config_value.param_name) {
            config_value.param_str_value = strtok_r(NULL, "=", &lasts);

            /* skip lines that do not contain two operands */
            if (config_value.param_str_value) {
                /* Trim leading and trailing spaces */
                loc_util_trim_space(config_value.param_name);
                loc_util_trim_space(config_value.param_str_value);

                /* Parse numerical value */
                if ((strlen(config_value.param_str_value) >=3) &&
                    (config_value.param_str_value[0] == '0') &&
                    (tolower(config_value.param_str_value[1]) == 'x'))
                {
                    /* hex */
                    config_value.param_int_value = (int) strtol(&config_value.param_str_value[2],
                                                                (char**) NULL, 16);
                }
                else {
                    config_value.param_double_value = (double) atof(config_value.param_str_value); /* float */
                    config_value.param_int_value = atoi(config_value.param_str_value); /* dec */
                }

                for(uint32_t i = 0; NULL != config_table && i < table_length; i++)
                {
                    if(!loc_set_config_entry(&config_table[i], &config_value)) {
                        ret += 1;
                    }
                }
            }
        }
    }

    return ret;
}

/*===========================================================================
FUNCTION loc_read_conf_r (repetitive)

DESCRIPTION
   Reads the specified configuration file and sets defined values based on
   the passed in configuration table. This table maps strings to values to
   set along with the type of each of these values.
   The difference between this and loc_read_conf is that this function returns
   the file pointer position at the end of filling a config table. Also, it
   reads a fixed number of parameters at a time which is equal to the length
   of the configuration table. This functionality enables the caller to
   repeatedly call the function to read data from the same file.

PARAMETERS:
   conf_fp : file pointer
   config_table: table definition of strings to places to store information
   table_length: length of the configuration table

DEPENDENCIES
   N/A

RETURN VALUE
   0: Table filled successfully
   1: No more parameters to read
  -1: Error filling table

SIDE EFFECTS
   N/A
===========================================================================*/
int loc_read_conf_r(FILE *conf_fp, const loc_param_s_type* config_table, uint32_t table_length)
{
    int ret=0;

    unsigned int num_params=table_length;
    if(conf_fp == NULL) {
        LOC_LOGE("%s:%d]: ERROR: File pointer is NULL\n", __func__, __LINE__);
        ret = -1;
        goto err;
    }

    /* Clear all validity bits */
    for(uint32_t i = 0; NULL != config_table && i < table_length; i++)
    {
        if(NULL != config_table[i].param_set)
        {
            *(config_table[i].param_set) = 0;
        }
    }

    char input_buf[LOC_MAX_PARAM_LINE];  /* declare a char array */

    LOC_LOGD("%s:%d]: num_params: %d\n", __func__, __LINE__, num_params);
    while(num_params)
    {
        if(!fgets(input_buf, LOC_MAX_PARAM_LINE, conf_fp)) {
            LOC_LOGD("%s:%d]: fgets returned NULL\n", __func__, __LINE__);
            break;
        }

        num_params -= loc_fill_conf_item(input_buf, config_table, table_length);
    }

err:
    return ret;
}

/*===========================================================================
FUNCTION loc_udpate_conf

DESCRIPTION
   Parses the passed in buffer for configuration items, and update the table
   that is also passed in.

Reads the specified configuration file and sets defined values based on
   the passed in configuration table. This table maps strings to values to
   set along with the type of each of these values.

PARAMETERS:
   conf_data: configuration items in bufferas a string
   length: strlen(conf_data)
   config_table: table definition of strings to places to store information
   table_length: length of the configuration table

DEPENDENCIES
   N/A

RETURN VALUE
   number of the records in the table that is updated at time of return.

SIDE EFFECTS
   N/A
===========================================================================*/
int loc_update_conf(const char* conf_data, int32_t length,
                    const loc_param_s_type* config_table, uint32_t table_length)
{
    int ret = -1;

    if (conf_data && length && config_table && table_length) {
        // make a copy, so we do not tokenize the original data
        char* conf_copy = (char*)malloc(length+1);

        if (conf_copy != NULL)
        {
            memcpy(conf_copy, conf_data, length);
            // we hard NULL the end of string to be safe
            conf_copy[length] = 0;

            // start with one record off
            uint32_t num_params = table_length - 1;
            char* saveptr = NULL;
            char* input_buf = strtok_r(conf_copy, "\n", &saveptr);
            ret = 0;

            LOC_LOGD("%s:%d]: num_params: %d\n", __func__, __LINE__, num_params);
            while(num_params && input_buf) {
                ret++;
                num_params -= loc_fill_conf_item(input_buf, config_table, table_length);
                input_buf = strtok_r(NULL, "\n", &saveptr);
            }
            free(conf_copy);
        }
    }

    return ret;
}

/*===========================================================================
FUNCTION loc_read_conf

DESCRIPTION
   Reads the specified configuration file and sets defined values based on
   the passed in configuration table. This table maps strings to values to
   set along with the type of each of these values.

PARAMETERS:
   conf_file_name: configuration file to read
   config_table: table definition of strings to places to store information
   table_length: length of the configuration table

DEPENDENCIES
   N/A

RETURN VALUE
   None

SIDE EFFECTS
   N/A
===========================================================================*/
void loc_read_conf(const char* conf_file_name, const loc_param_s_type* config_table,
                   uint32_t table_length)
{
    FILE *conf_fp = NULL;

    if((conf_fp = fopen(conf_file_name, "r")) != NULL)
    {
        LOC_LOGD("%s: using %s", __FUNCTION__, conf_file_name);
        if(table_length && config_table) {
            loc_read_conf_r(conf_fp, config_table, table_length);
            rewind(conf_fp);
        }
        loc_read_conf_r(conf_fp, loc_param_table, loc_param_num);
        fclose(conf_fp);
    }
    /* Initialize logging mechanism with parsed data */
    loc_logger_init(DEBUG_LEVEL, TIMESTAMP);
}

/*=============================================================================
 *
 *   Define and Structures for Parsing Location Process Configuration File
 *
 *============================================================================*/
#define MAX_NUM_STRINGS   20

//We can have 8 masks for now
#define CONFIG_MASK_TARGET_ALL           0X01
#define CONFIG_MASK_TARGET_FOUND         0X02
#define CONFIG_MASK_TARGET_CHECK         0X03
#define CONFIG_MASK_BASEBAND_ALL         0X04
#define CONFIG_MASK_BASEBAND_FOUND       0X08
#define CONFIG_MASK_BASEBAND_CHECK       0x0c
#define CONFIG_MASK_AUTOPLATFORM_ALL     0x10
#define CONFIG_MASK_AUTOPLATFORM_FOUND   0x20
#define CONFIG_MASK_AUTOPLATFORM_CHECK   0x30
#define CONFIG_MASK_SOCID_ALL            0x40
#define CONFIG_MASK_SOCID_FOUND          0x80
#define CONFIG_MASK_SOCID_CHECK          0xc0

#define LOC_FEATURE_MASK_GTP_WIFI_BASIC            0x01
#define LOC_FEATURE_MASK_GTP_WIFI_PREMIUM          0X02
#define LOC_FEATURE_MASK_GTP_CELL_BASIC            0X04
#define LOC_FEATURE_MASK_GTP_CELL_PREMIUM          0X08
#define LOC_FEATURE_MASK_SAP_BASIC                 0x40
#define LOC_FEATURE_MASK_SAP_PREMIUM               0X80
#define LOC_FEATURE_MASK_GTP_WAA_BASIC             0X100
#define LOC_FEATURE_MASK_GTP_MODEM_CELL_BASIC      0X400
#define LOC_FEATURE_MASK_ODCPI                     0x1000
#define LOC_FEATURE_MASK_FREE_WIFI_SCAN_INJECT     0x2000
#define LOC_FEATURE_MASK_SUPL_WIFI                 0x4000
#define LOC_FEATURE_MASK_WIFI_SUPPLICANT_INFO      0x8000

typedef struct {
    char proc_name[LOC_MAX_PARAM_STRING];
    char proc_argument[LOC_MAX_PARAM_STRING];
    char proc_status[LOC_MAX_PARAM_STRING];
    char group_list[LOC_MAX_PARAM_STRING];
    unsigned int premium_feature;
    unsigned int loc_feature_mask;
    char platform_list[LOC_MAX_PARAM_STRING];
    char baseband[LOC_MAX_PARAM_STRING];
    char low_ram_targets[LOC_MAX_PARAM_STRING];
    char soc_id_list[LOC_MAX_PARAM_STRING];
    unsigned int sglte_target;
    char feature_gtp_mode[LOC_MAX_PARAM_STRING];
    char feature_gtp_waa[LOC_MAX_PARAM_STRING];
    char feature_sap[LOC_MAX_PARAM_STRING];
    char feature_odcpi[LOC_MAX_PARAM_STRING];
    char feature_free_wifi_scan_inject[LOC_MAX_PARAM_STRING];
    char feature_supl_wifi[LOC_MAX_PARAM_STRING];
    char feature_wifi_supplicant_info[LOC_MAX_PARAM_STRING];
    char auto_platform[LOC_MAX_PARAM_STRING];
    unsigned int vendor_enhanced_process;
} loc_launcher_conf;

/* process configuration parameters */
static loc_launcher_conf conf;

/* gps.conf Parameter spec table */
static const loc_param_s_type gps_conf_parameter_table[] = {
    {"SGLTE_TARGET",        &conf.sglte_target,           NULL, 'n'},
};

/* location feature conf, e.g.: izat.conf feature mode table*/
static const loc_param_s_type loc_feature_conf_table[] = {
    {"GTP_MODE",              &conf.feature_gtp_mode,               NULL, 's'},
    {"GTP_WAA",               &conf.feature_gtp_waa,                NULL, 's'},
    {"SAP",                   &conf.feature_sap,                    NULL, 's'},
    {"ODCPI",                 &conf.feature_odcpi,                  NULL, 's'},
    {"FREE_WIFI_SCAN_INJECT", &conf.feature_free_wifi_scan_inject,  NULL, 's'},
    {"SUPL_WIFI",             &conf.feature_supl_wifi,              NULL, 's'},
    {"WIFI_SUPPLICANT_INFO",  &conf.feature_wifi_supplicant_info,   NULL, 's'},
};

/* location process conf, e.g.: izat.conf Parameter spec table */
static const loc_param_s_type loc_process_conf_parameter_table[] = {
    {"PROCESS_NAME",               &conf.proc_name,                NULL, 's'},
    {"PROCESS_ARGUMENT",           &conf.proc_argument,            NULL, 's'},
    {"PROCESS_STATE",              &conf.proc_status,              NULL, 's'},
    {"PROCESS_GROUPS",             &conf.group_list,               NULL, 's'},
    {"PREMIUM_FEATURE",            &conf.premium_feature,          NULL, 'n'},
    {"IZAT_FEATURE_MASK",          &conf.loc_feature_mask,         NULL, 'n'},
    {"PLATFORMS",                  &conf.platform_list,            NULL, 's'},
    {"SOC_IDS",                    &conf.soc_id_list,            NULL, 's'},
    {"BASEBAND",                   &conf.baseband,                 NULL, 's'},
    {"LOW_RAM_TARGETS",            &conf.low_ram_targets,          NULL, 's'},
    {"HARDWARE_TYPE",              &conf.auto_platform,            NULL, 's'},
    {"VENDOR_ENHANCED_PROCESS",    &conf.vendor_enhanced_process,  NULL, 'n'},
};

/*===========================================================================
FUNCTION loc_read_process_conf

DESCRIPTION
   Parse the specified conf file and return info for the processes defined.
   The format of the file should conform with izat.conf.

PARAMETERS:
   conf_file_name: configuration file to read
   process_count_ptr: pointer to store number of processes defined in the conf file.
   process_info_table_ptr: pointer to store the process info table.

DEPENDENCIES
   The file must be in izat.conf format.

RETURN VALUE
   0: success
   none-zero: failure

SIDE EFFECTS
   N/A

NOTES:
   On success, memory pointed by (*process_info_table_ptr) must be freed.
===========================================================================*/
int loc_read_process_conf(const char* conf_file_name, uint32_t * process_count_ptr,
                          loc_process_info_s_type** process_info_table_ptr) {
    loc_process_info_s_type *child_proc = nullptr;
    volatile int i=0;
    unsigned int j=0;
    gid_t gid_list[LOC_PROCESS_MAX_NUM_GROUPS];
    char *split_strings[MAX_NUM_STRINGS];
    int name_length=0, group_list_length=0, platform_length=0, baseband_length=0, ngroups=0, ret=0;
    int auto_platform_length = 0, soc_id_list_length=0;
    int group_index=0, nstrings=0, status_length=0;
    FILE* conf_fp = nullptr;
    char platform_name[PROPERTY_VALUE_MAX], baseband_name[PROPERTY_VALUE_MAX];
    int low_ram_target=0;
    char autoplatform_name[PROPERTY_VALUE_MAX], socid_value[PROPERTY_VALUE_MAX];
    unsigned int loc_service_mask=0;
    unsigned char config_mask = 0;
    unsigned char proc_list_length=0;
    int gtp_cell_ap_enabled = 0;
    char arg_gtp_waa[LOC_PROCESS_MAX_ARG_STR_LENGTH] = "--";
    char arg_gtp_modem_cell[LOC_PROCESS_MAX_ARG_STR_LENGTH] = "--";
    char arg_gtp_wifi[LOC_PROCESS_MAX_ARG_STR_LENGTH] = "--";
    char arg_sap[LOC_PROCESS_MAX_ARG_STR_LENGTH] = "--";
    char arg_disabled[LOC_PROCESS_MAX_ARG_STR_LENGTH] = LOC_FEATURE_MODE_DISABLED;
    char arg_basic[LOC_PROCESS_MAX_ARG_STR_LENGTH] = LOC_FEATURE_MODE_BASIC;
    char arg_premium[LOC_PROCESS_MAX_ARG_STR_LENGTH] = LOC_FEATURE_MODE_PREMIUM;

    if (process_count_ptr == NULL || process_info_table_ptr == NULL) {
        return -1;
    }

    //Read gps.conf and fill parameter table
    UTIL_READ_CONF(LOC_PATH_GPS_CONF, gps_conf_parameter_table);

    //Form argument strings
    strlcat(arg_gtp_waa, LOC_FEATURE_GTP_WAA, LOC_PROCESS_MAX_ARG_STR_LENGTH-3);
    strlcat(arg_gtp_modem_cell, LOC_FEATURE_GTP_MODEM_CELL, LOC_PROCESS_MAX_ARG_STR_LENGTH-3);
    strlcat(arg_gtp_wifi, LOC_FEATURE_GTP_WIFI, LOC_PROCESS_MAX_ARG_STR_LENGTH-3);
    strlcat(arg_sap, LOC_FEATURE_SAP, LOC_PROCESS_MAX_ARG_STR_LENGTH-3);

    //Get platform name from ro.board.platform property
    loc_get_platform_name(platform_name, sizeof(platform_name));
    //Get baseband name from ro.baseband property
    loc_get_target_baseband(baseband_name, sizeof(baseband_name));
    //Identify if this is an automotive platform
    loc_get_auto_platform_name(autoplatform_name,sizeof(autoplatform_name));
    //Identify if this is a low ram target from ro.config.low_ram property
    low_ram_target = loc_identify_low_ram_target();
    // Get the soc-id for this device.
    loc_get_device_soc_id(socid_value, sizeof(socid_value));

    UTIL_READ_CONF(conf_file_name, loc_feature_conf_table);

    //Set service mask for GTP_MODE
    if(strcmp(conf.feature_gtp_mode, "DISABLED") == 0) {
        LOC_LOGD("%s:%d]: GTP MODE DISABLED", __func__, __LINE__);
    }
    else if(strcmp(conf.feature_gtp_mode, "LEGACY_WWAN") == 0) {
        LOC_LOGD("%s:%d]: Setting GTP MODE to mode: LEGACY_WWAN", __func__, __LINE__);
        loc_service_mask |= LOC_FEATURE_MASK_GTP_MODEM_CELL_BASIC;
    }
    else if(strcmp(conf.feature_gtp_mode, "SDK") == 0) {
        LOC_LOGD("%s:%d]: Setting GTP MODE to mode: SDK", __func__, __LINE__);
        loc_service_mask |= LOC_FEATURE_MASK_GTP_WIFI_BASIC;
    }
    //conf file has a garbage value
    else {
        LOC_LOGE("%s:%d]: Unrecognized value for GTP MODE Mode."\
                 " Setting GTP WIFI to default mode: DISABLED", __func__, __LINE__);
    }
    //Set service mask for GTP_WAA
    if(strcmp(conf.feature_gtp_waa, "BASIC") == 0) {
      LOC_LOGD("%s:%d]: Setting GTP WAA to mode: BASIC", __func__, __LINE__);
      loc_service_mask |= LOC_FEATURE_MASK_GTP_WAA_BASIC;
    }
    else if(strcmp(conf.feature_gtp_waa, "DISABLED") == 0) {
      LOC_LOGD("%s:%d]: GTP WAA DISABLED", __func__, __LINE__);
    }
    //conf file has a garbage value
    else {
      LOC_LOGE("%s:%d]: Unrecognized value for GTP WAA Mode."\
               " Setting GTP WAA to default mode: DISABLED", __func__, __LINE__);
    }

    //Set service mask for SAP
    if(strcmp(conf.feature_sap, "PREMIUM") == 0) {
        LOC_LOGD("%s:%d]: Setting SAP to mode: PREMIUM", __func__, __LINE__);
        loc_service_mask |= LOC_FEATURE_MASK_SAP_PREMIUM;
    }
    else if(strcmp(conf.feature_sap, "BASIC") == 0) {
        LOC_LOGD("%s:%d]: Setting SAP to mode: BASIC", __func__, __LINE__);
        loc_service_mask |= LOC_FEATURE_MASK_SAP_BASIC;
    }
    else if(strcmp(conf.feature_sap, "MODEM_DEFAULT") == 0) {
        LOC_LOGD("%s:%d]: Setting SAP to mode: MODEM_DEFAULT", __func__, __LINE__);
    }
    else if(strcmp(conf.feature_sap, "DISABLED") == 0) {
        LOC_LOGD("%s:%d]: Setting SAP to mode: DISABLED", __func__, __LINE__);
    }
    else {
       LOC_LOGE("%s:%d]: Unrecognized value for SAP Mode."\
                " Setting SAP to default mode: BASIC", __func__, __LINE__);
       loc_service_mask |= LOC_FEATURE_MASK_SAP_BASIC;
    }

    // Set service mask for ODCPI
    if(strcmp(conf.feature_odcpi, "BASIC") == 0) {
        LOC_LOGD("%s:%d]: Setting ODCPI to mode: BASIC", __func__, __LINE__);
        loc_service_mask |= LOC_FEATURE_MASK_ODCPI;
    }
    else if(strcmp(conf.feature_odcpi, "DISABLED") == 0) {
        LOC_LOGD("%s:%d]: Setting ODCPI to mode: DISABLED", __func__, __LINE__);
    }
    else if(strcmp(conf.feature_odcpi, "PREMIUM") == 0) {
        LOC_LOGD("%s:%d]: Unrecognized value for ODCPI mode."\
            "Setting ODCPI to default mode: BASIC", __func__, __LINE__);
        loc_service_mask |= LOC_FEATURE_MASK_ODCPI;
    }

    // Set service mask for FREE_WIFI_SCAN_INJECT
    if(strcmp(conf.feature_free_wifi_scan_inject, "BASIC") == 0) {
        LOC_LOGD("%s:%d]: Setting FREE_WIFI_SCAN_INJECT to mode: BASIC", __func__, __LINE__);
        loc_service_mask |= LOC_FEATURE_MASK_FREE_WIFI_SCAN_INJECT;
    }
    else if(strcmp(conf.feature_free_wifi_scan_inject, "DISABLED") == 0) {
        LOC_LOGD("%s:%d]: Setting FREE_WIFI_SCAN_INJECT to mode: DISABLED", __func__, __LINE__);
    }
    else if(strcmp(conf.feature_free_wifi_scan_inject, "PREMIUM") == 0) {
        LOC_LOGD("%s:%d]: Unrecognized value for FREE_WIFI_SCAN_INJECT mode."\
            "Setting FREE_WIFI_SCAN_INJECT to default mode: BASIC", __func__, __LINE__);
        loc_service_mask |= LOC_FEATURE_MASK_FREE_WIFI_SCAN_INJECT;
    }

    // Set service mask for SUPL_WIFI
    if(strcmp(conf.feature_supl_wifi, "BASIC") == 0) {
        LOC_LOGD("%s:%d]: Setting SUPL_WIFI to mode: BASIC", __func__, __LINE__);
        loc_service_mask |= LOC_FEATURE_MASK_SUPL_WIFI;
    }
    else if(strcmp(conf.feature_supl_wifi, "DISABLED") == 0) {
        LOC_LOGD("%s:%d]: Setting SUPL_WIFI to mode: DISABLED", __func__, __LINE__);
    }
    else if(strcmp(conf.feature_supl_wifi, "PREMIUM") == 0) {
        LOC_LOGD("%s:%d]: Unrecognized value for SUPL_WIFI mode."\
            "Setting SUPL_WIFI to default mode: BASIC", __func__, __LINE__);
        loc_service_mask |= LOC_FEATURE_MASK_SUPL_WIFI;
    }

    // Set service mask for WIFI_SUPPLICANT_INFO
    if(strcmp(conf.feature_wifi_supplicant_info, "BASIC") == 0) {
        LOC_LOGD("%s:%d]: Setting WIFI_SUPPLICANT_INFO to mode: BASIC", __func__, __LINE__);
        loc_service_mask |= LOC_FEATURE_MASK_WIFI_SUPPLICANT_INFO;
    }
    else if(strcmp(conf.feature_wifi_supplicant_info, "DISABLED") == 0) {
        LOC_LOGD("%s:%d]: Setting WIFI_SUPPLICANT_INFO to mode: DISABLED", __func__, __LINE__);
    }
    else if(strcmp(conf.feature_wifi_supplicant_info, "PREMIUM") == 0) {
        LOC_LOGD("%s:%d]: Unrecognized value for WIFI_SUPPLICANT_INFO mode."\
            "Setting LOC_FEATURE_MASK_WIFI_SUPPLICANT_INFO to default mode: BASIC", __func__, __LINE__);
        loc_service_mask |= LOC_FEATURE_MASK_WIFI_SUPPLICANT_INFO;
    }

    LOC_LOGD("%s:%d]: loc_service_mask: %x\n", __func__, __LINE__, loc_service_mask);

    if((conf_fp = fopen(conf_file_name, "r")) == NULL) {
        LOC_LOGE("%s:%d]: Error opening %s %s\n", __func__,
                 __LINE__, conf_file_name, strerror(errno));
        ret = -1;
        goto err;
    }

    //Parse through the file to find out how many processes are to be launched
    proc_list_length = 0;
    do {
        conf.proc_name[0] = 0;
        //Here note that the 3rd parameter is passed as 1.
        //This is so that only the first parameter in the table which is "PROCESS_NAME"
        //is read. We do not want to read the entire block of parameters at this time
        //since we are only counting the number of processes to launch.
        //Therefore, only counting the occurrences of PROCESS_NAME parameter
        //should suffice
        if(loc_read_conf_r(conf_fp, loc_process_conf_parameter_table, 1)) {
            LOC_LOGE("%s:%d]: Unable to read conf file. Failing\n", __func__, __LINE__);
            ret = -1;
            goto err;
        }
        name_length=(int)strlen(conf.proc_name);
        if(name_length) {
            proc_list_length++;
            LOC_LOGD("Process name:%s", conf.proc_name);
        }
    } while(name_length);
    LOC_LOGD("Process cnt = %d", proc_list_length);

    child_proc = (loc_process_info_s_type *)calloc(proc_list_length, sizeof(loc_process_info_s_type));
    if(child_proc == NULL) {
        LOC_LOGE("%s:%d]: ERROR: Malloc returned NULL\n", __func__, __LINE__);
        ret = -1;
        goto err;
    }

    //Move file descriptor to the beginning of the file
    //so that the parameters can be read
    rewind(conf_fp);

    for(j=0; j<proc_list_length; j++) {
        //Set defaults for all the child process structs
        child_proc[j].proc_status = DISABLED;
        memset(child_proc[j].group_list, 0, sizeof(child_proc[j].group_list));
        config_mask=0;
        if(loc_read_conf_r(conf_fp, loc_process_conf_parameter_table,
                           sizeof(loc_process_conf_parameter_table)/sizeof(loc_process_conf_parameter_table[0]))) {
            LOC_LOGE("%s:%d]: Unable to read conf file. Failing\n", __func__, __LINE__);
            ret = -1;
            goto err;
        }

        name_length=(int)strlen(conf.proc_name);
        group_list_length=(int)strlen(conf.group_list);
        platform_length = (int)strlen(conf.platform_list);
        baseband_length = (int)strlen(conf.baseband);
        status_length = (int)strlen(conf.proc_status);
        auto_platform_length = (int)strlen(conf.auto_platform);
        soc_id_list_length = (int)strlen(conf.soc_id_list);

        if(!name_length || !group_list_length || !platform_length ||
           !baseband_length || !status_length || !auto_platform_length || !soc_id_list_length) {
            LOC_LOGE("%s:%d]: Error: i: %d; One of the parameters not specified in conf file",
                     __func__, __LINE__, i);
            continue;
        }

        if (!isVendorEnhanced() && (conf.vendor_enhanced_process != 0)) {
            LOC_LOGD("%s:%d]: Process %s is disabled via vendor enhanced process check",
                     __func__, __LINE__, conf.proc_name);
            child_proc[j].proc_status = DISABLED_VIA_VENDOR_ENHANCED_CHECK;
            continue;
        }

        if(strcmp(conf.proc_status, "DISABLED") == 0) {
            LOC_LOGD("%s:%d]: Process %s is disabled in conf file",
                     __func__, __LINE__, conf.proc_name);
            child_proc[j].proc_status = DISABLED_FROM_CONF;
            continue;
        }
        else if(strcmp(conf.proc_status, "ENABLED") == 0) {
            LOC_LOGD("%s:%d]: Process %s is enabled in conf file",
                     __func__, __LINE__, conf.proc_name);
        }

        //Since strlcpy copies length-1 characters, we add 1 to name_length
        if((name_length+1) > LOC_MAX_PARAM_STRING) {
            LOC_LOGE("%s:%d]: i: %d; Length of name parameter too long. Max length: %d",
                     __func__, __LINE__, i, LOC_MAX_PARAM_STRING);
            continue;
        }
        strlcpy(child_proc[j].name[0], conf.proc_name, sizeof (child_proc[j].name[0]));

        child_proc[j].num_groups = 0;
        ngroups = loc_util_split_string(conf.group_list, split_strings, MAX_NUM_STRINGS, ' ');
        for(i=0; i<ngroups; i++) {
            struct group* grp = getgrnam(split_strings[i]);
            if (grp) {
                child_proc[j].group_list[child_proc[j].num_groups] = grp->gr_gid;
                child_proc[j].num_groups++;
                LOC_LOGd("Group %s = %d", split_strings[i], grp->gr_gid);
            }
        }

        nstrings = loc_util_split_string(conf.platform_list, split_strings, MAX_NUM_STRINGS, ' ');
        if(strcmp("all", split_strings[0]) == 0) {
            if (nstrings == 1 || (nstrings == 2 && (strcmp("exclude", split_strings[1]) == 0))) {
                LOC_LOGD("%s:%d]: Enabled for all targets\n", __func__, __LINE__);
                config_mask |= CONFIG_MASK_TARGET_ALL;
            }
            else if (nstrings > 2 && (strcmp("exclude", split_strings[1]) == 0)) {
                config_mask |= CONFIG_MASK_TARGET_FOUND;
                for (i=2; i<nstrings; i++) {
                    if(strcmp(platform_name, split_strings[i]) == 0) {
                        LOC_LOGD("%s:%d]: Disabled platform %s\n", __func__, __LINE__, platform_name);
                        config_mask &= ~CONFIG_MASK_TARGET_FOUND;
                        break;
                    }
                }
            }
        }
        else {
            for(i=0; i<nstrings; i++) {
                if(strcmp(platform_name, split_strings[i]) == 0) {
                    LOC_LOGD("%s:%d]: Matched platform: %s\n",
                             __func__, __LINE__, split_strings[i]);
                    config_mask |= CONFIG_MASK_TARGET_FOUND;
                    break;
                }
            }
        }

        // SOC Id's check
        nstrings = loc_util_split_string(conf.soc_id_list, split_strings, MAX_NUM_STRINGS, ' ');
        if (strcmp("all", split_strings[0]) == 0) {
            if (nstrings == 1 || (nstrings == 2 && (strcmp("exclude", split_strings[1]) == 0))) {
                LOC_LOGd("Enabled for all SOC ids\n");
                config_mask |= CONFIG_MASK_SOCID_ALL;
            }
            else if (nstrings > 2 && (strcmp("exclude", split_strings[1]) == 0)) {
                config_mask |= CONFIG_MASK_SOCID_FOUND;
                for (i = 2; i < nstrings; i++) {
                    if (strcmp(socid_value, split_strings[i]) == 0) {
                        LOC_LOGd("Disabled for SOC id %s\n", socid_value);
                        config_mask &= ~CONFIG_MASK_SOCID_FOUND;
                        break;
                    }
                }
            }
        }
        else {
            for (i = 0; i < nstrings; i++) {
                if (strcmp(socid_value, split_strings[i]) == 0) {
                    LOC_LOGd("Matched SOC id : %s\n", split_strings[i]);
                    config_mask |= CONFIG_MASK_SOCID_FOUND;
                    break;
                }
            }
        }

        nstrings = loc_util_split_string(conf.baseband, split_strings, MAX_NUM_STRINGS, ' ');
        if(strcmp("all", split_strings[0]) == 0) {
            if (nstrings == 1 || (nstrings == 2 && (strcmp("exclude", split_strings[1]) == 0))) {
                LOC_LOGD("%s:%d]: Enabled for all basebands\n", __func__, __LINE__);
                config_mask |= CONFIG_MASK_BASEBAND_ALL;
            }
            else if (nstrings > 2 && (strcmp("exclude", split_strings[1]) == 0)) {
                config_mask |= CONFIG_MASK_BASEBAND_FOUND;
                for (i=2; i<nstrings; i++) {
                    if(strcmp(baseband_name, split_strings[i]) == 0) {
                        LOC_LOGD("%s:%d]: Disabled band %s\n", __func__, __LINE__, baseband_name);
                        config_mask &= ~CONFIG_MASK_BASEBAND_FOUND;
                        break;
                    }
                }
            }
        }
        else {
            for(i=0; i<nstrings; i++) {
                if(strcmp(baseband_name, split_strings[i]) == 0) {
                    LOC_LOGD("%s:%d]: Matched baseband: %s\n",
                             __func__, __LINE__, split_strings[i]);
                    config_mask |= CONFIG_MASK_BASEBAND_FOUND;
                    break;
                }
                //Since ro.baseband is not a reliable source for detecting sglte
                //the alternative is to read the SGLTE_TARGET parameter from gps.conf
                //this parameter is read into conf_sglte_target
                else if((strcmp("sglte", split_strings[i]) == 0 ) && conf.sglte_target) {
                    LOC_LOGD("%s:%d]: Matched baseband SGLTE\n", __func__, __LINE__);
                    config_mask |= CONFIG_MASK_BASEBAND_FOUND;
                    break;
                }
            }
        }

        nstrings = loc_util_split_string(conf.auto_platform, split_strings, MAX_NUM_STRINGS, ' ');
        if(strcmp("all", split_strings[0]) == 0) {
            LOC_LOGD("%s:%d]: Enabled for all auto platforms\n", __func__, __LINE__);
            config_mask |= CONFIG_MASK_AUTOPLATFORM_ALL;
        }
        else {
            for(i=0; i<nstrings; i++) {
                if(strcmp(autoplatform_name, split_strings[i]) == 0) {
                    LOC_LOGD("%s:%d]: Matched auto platform: %s\n",
                             __func__, __LINE__, split_strings[i]);
                    config_mask |= CONFIG_MASK_AUTOPLATFORM_FOUND;
                    break;
                }
            }
        }

        nstrings = loc_util_split_string(conf.low_ram_targets, split_strings, MAX_NUM_STRINGS, ' ');
        if (!strcmp("DISABLED", split_strings[0]) && low_ram_target) {
            LOC_LOGd("Disabled for low ram targets\n");
            child_proc[j].proc_status = DISABLED;
            continue;
        }

        if((config_mask & CONFIG_MASK_TARGET_CHECK) &&
           (config_mask & CONFIG_MASK_BASEBAND_CHECK) &&
           (config_mask & CONFIG_MASK_AUTOPLATFORM_CHECK) &&
           (config_mask & CONFIG_MASK_SOCID_CHECK) &&
           (child_proc[j].proc_status != DISABLED_FROM_CONF) &&
           (child_proc[j].proc_status != DISABLED_VIA_VENDOR_ENHANCED_CHECK)) {

            //Set args
            //The first argument passed through argv is usually the name of the
            //binary when started from commandline.
            //getopt() seems to ignore this first argument and hence we assign it
            //to the process name for consistency with command line args
            i = 0;
            char* temp_arg = ('/' == child_proc[j].name[0][0]) ?
                (strrchr(child_proc[j].name[0], '/') + 1) : child_proc[j].name[0];
            strlcpy (child_proc[j].args[i++], temp_arg, sizeof (child_proc[j].args[i++]));

            if(conf.premium_feature) {
               if(conf.loc_feature_mask & loc_service_mask) {
                    LOC_LOGD("%s:%d]: Enabled. %s has service mask: %x\n",
                             __func__, __LINE__, child_proc[j].name[0], conf.loc_feature_mask);
                    child_proc[j].proc_status = ENABLED;

                    if(conf.loc_feature_mask &
                       (LOC_FEATURE_MASK_GTP_WIFI_BASIC | LOC_FEATURE_MASK_GTP_WIFI_PREMIUM)) {
                        if(loc_service_mask & LOC_FEATURE_MASK_GTP_WIFI_BASIC) {
                            strlcpy(child_proc[j].args[i++], arg_gtp_wifi,
                                    LOC_PROCESS_MAX_ARG_STR_LENGTH);
                            strlcpy(child_proc[j].args[i++], arg_basic,
                                    LOC_PROCESS_MAX_ARG_STR_LENGTH);
                        }
                        else if(loc_service_mask & LOC_FEATURE_MASK_GTP_WIFI_PREMIUM) {
                            strlcpy(child_proc[j].args[i++], arg_gtp_wifi,
                                    LOC_PROCESS_MAX_ARG_STR_LENGTH);
                            strlcpy(child_proc[j].args[i++], arg_premium,
                                    LOC_PROCESS_MAX_ARG_STR_LENGTH);
                        }
                        else
                        {
                            strlcpy(child_proc[j].args[i++], arg_gtp_wifi,
                                    LOC_PROCESS_MAX_ARG_STR_LENGTH);
                            strlcpy(child_proc[j].args[i++], arg_disabled,
                                    LOC_PROCESS_MAX_ARG_STR_LENGTH);
                        }
                    }
                    if(conf.loc_feature_mask &
                       (LOC_FEATURE_MASK_GTP_CELL_BASIC | LOC_FEATURE_MASK_GTP_CELL_PREMIUM )) {
                        if(loc_service_mask & LOC_FEATURE_MASK_GTP_MODEM_CELL_BASIC) {
                            strlcpy(child_proc[j].args[i++], arg_gtp_modem_cell,
                                    LOC_PROCESS_MAX_ARG_STR_LENGTH);
                            strlcpy(child_proc[j].args[i++], arg_basic,
                                    LOC_PROCESS_MAX_ARG_STR_LENGTH);
                        }
                        else {
                             strlcpy(child_proc[j].args[i++], arg_gtp_modem_cell,
                                     LOC_PROCESS_MAX_ARG_STR_LENGTH);
                             strlcpy(child_proc[j].args[i++], arg_disabled,
                                     LOC_PROCESS_MAX_ARG_STR_LENGTH);
                       }
                    }
                    if(conf.loc_feature_mask &
                       (LOC_FEATURE_MASK_SAP_BASIC | LOC_FEATURE_MASK_SAP_PREMIUM)) {
                        if(loc_service_mask & LOC_FEATURE_MASK_SAP_BASIC) {
                            strlcpy(child_proc[j].args[i++], arg_sap,
                                    LOC_PROCESS_MAX_ARG_STR_LENGTH);
                            strlcpy(child_proc[j].args[i++], arg_basic,
                                    LOC_PROCESS_MAX_ARG_STR_LENGTH);
                        }
                        else if(loc_service_mask & LOC_FEATURE_MASK_SAP_PREMIUM) {
                            strlcpy(child_proc[j].args[i++], arg_sap,
                                    LOC_PROCESS_MAX_ARG_STR_LENGTH);
                            strlcpy(child_proc[j].args[i++], arg_premium,
                                    LOC_PROCESS_MAX_ARG_STR_LENGTH);
                        }
                        else
                        {
                            strlcpy(child_proc[j].args[i++], arg_sap,
                                    LOC_PROCESS_MAX_ARG_STR_LENGTH);
                            strlcpy(child_proc[j].args[i++], arg_disabled,
                                    LOC_PROCESS_MAX_ARG_STR_LENGTH);
                        }
                    }

                    if(conf.loc_feature_mask & LOC_FEATURE_MASK_GTP_WAA_BASIC) {
                      if(loc_service_mask & LOC_FEATURE_MASK_GTP_WAA_BASIC) {
                        strlcpy(child_proc[j].args[i++], arg_gtp_waa,
                                LOC_PROCESS_MAX_ARG_STR_LENGTH);
                        strlcpy(child_proc[j].args[i++], arg_basic,
                                LOC_PROCESS_MAX_ARG_STR_LENGTH);
                      }
                      else
                      {
                        strlcpy(child_proc[j].args[i++], arg_gtp_waa,
                                LOC_PROCESS_MAX_ARG_STR_LENGTH);
                        strlcpy(child_proc[j].args[i++], arg_disabled,
                                LOC_PROCESS_MAX_ARG_STR_LENGTH);
                      }
                    }
                    IF_LOC_LOGD {
                        LOC_LOGD("%s:%d]: %s args\n", __func__, __LINE__, child_proc[j].name[0]);
                        for(unsigned int k=0; k<LOC_PROCESS_MAX_NUM_ARGS; k++) {
                            if(child_proc[j].args[k][0] != '\0') {
                                LOC_LOGD("%s:%d]: k: %d, %s\n", __func__, __LINE__, k,
                                         child_proc[j].args[k]);
                            }
                        }
                        LOC_LOGD("%s:%d]: \n", __func__, __LINE__);
                    }
                }
                else {
                    LOC_LOGD("%s:%d]: Disabled. %s has service mask:  %x \n",
                             __func__, __LINE__, child_proc[j].name[0], conf.loc_feature_mask);
                }
            }
            else {
                LOC_LOGD("%s:%d]: %s not a premium feature. Enabled\n",
                         __func__, __LINE__, child_proc[j].name[0]);
                child_proc[j].proc_status = ENABLED;
            }

            /*Fill up the remaining arguments from configuration file*/
            LOC_LOGD("%s] Parsing Process_Arguments from Configuration: %s \n",
                      __func__, conf.proc_argument);
            if(0 != conf.proc_argument[0])
            {
                /**************************************
                ** conf_proc_argument is shared by all the programs getting launched,
                ** hence copy to process specific argument string and parse the same.
                ***************************************/
                strlcpy(child_proc[j].argumentString, conf.proc_argument,
                        sizeof(child_proc[j].argumentString));
                char *temp_args[LOC_PROCESS_MAX_NUM_ARGS];
                memset (temp_args, 0, sizeof (temp_args));
                loc_util_split_string(child_proc[j].argumentString, &temp_args[i],
                                      (LOC_PROCESS_MAX_NUM_ARGS - i), ' ');
                // copy argument from the pointer to the memory
                for (unsigned int index = i; index < LOC_PROCESS_MAX_NUM_ARGS; index++) {
                    if (temp_args[index] == NULL) {
                        break;
                    }
                    strlcpy (child_proc[j].args[index], temp_args[index],
                             sizeof (child_proc[j].args[index]));
                }
            }
        }
        else {
            LOC_LOGD("%s:%d]: Process %s is disabled\n",
                     __func__, __LINE__, child_proc[j].name[0]);
        }
    }

err:
    if (conf_fp) {
        fclose(conf_fp);
    }
    if (ret != 0) {
        LOC_LOGE("%s:%d]: ret: %d", __func__, __LINE__, ret);
        if (child_proc) {
            free (child_proc);
            child_proc = nullptr;
        }
        *process_count_ptr = 0;
        *process_info_table_ptr = nullptr;

    }
    else {
        *process_count_ptr = proc_list_length;
        *process_info_table_ptr = child_proc;
    }

    return ret;
}
