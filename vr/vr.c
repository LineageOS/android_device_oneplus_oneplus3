/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "VrHALImpl"

#include <cutils/log.h>

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <hardware/vr.h>
#include <hardware/hardware.h>

#include "thermal_client.h"


static void *dlhandle;
static int (*p_thermal_client_config_query)(char *, struct config_instance **);
static int (*p_thermal_client_config_set)(struct config_instance *, unsigned int);
static void (*p_thermal_client_config_cleanup)(struct config_instance *, unsigned int);

static int max_string_size = 36;
static int error_state = 0; //global error state - don't do anything if set!

// List thermal configs in format {name, algo_type}
// This list is manually synced with the thermal config
#define NUM_NON_VR_CONFIGS 4
static char *non_vr_thermal_configs[NUM_NON_VR_CONFIGS][2] =
    {{"SKIN-HIGH-FLOOR",     "ss"},
     {"SKIN-MID-FLOOR",      "ss"},
     {"SKIN-LOW-FLOOR",      "ss"},
     {"VIRTUAL-SS-GPU-SKIN", "ss"}};
#define NUM_VR_CONFIGS 1
static char *vr_thermal_configs[NUM_VR_CONFIGS][2] =
    {{"VR-EMMC",     "monitor"}};

#define DEBUG 0

/**
 * Debug function for printing out a log instance
 */
static void log_config_instance(struct config_instance *instance ){
    ALOGI("logging config_instance 0x%p", instance);
    ALOGI("config_instance: cfg_desc = %s", instance->cfg_desc);
    ALOGI("config_instance: algo_type = %s", instance->algo_type);
    ALOGI("config_instance: fields_mask = 0x%x", instance->fields_mask);
    ALOGI("config_instance: num_fields = %u", instance->num_fields);
    for (uint32_t i = 0; i < instance->num_fields; i++) {
        ALOGI("config_instance: field_data[%d]", i);
        ALOGI("\tfield_name = %s", instance->fields[i].field_name);
        ALOGI("\tdata_type = %u", instance->fields[i].data_type);
        ALOGI("\tnum_data = %u", instance->fields[i].num_data);
        switch (instance->fields[i].data_type){
          case FIELD_INT: ALOGI("\tdata = %d", *(int*)(instance->fields[i].data)); break;
          case FIELD_STR: ALOGI("\tdata = %s", (char*)(instance->fields[i].data)); break;
          default: ALOGI("\tdata = 0x%p", instance->fields[i].data); break;
        }
    }
}

/**
 * Debug function for printing out all instances of "ss" and "monitor" algos
 */
static void query_thermal_config(){
    struct config_instance *instances;

    int num_configs = (*p_thermal_client_config_query)("ss", &instances);
    if (num_configs <= 0) {
        return;
    }
    for (int i = 0; i < num_configs; i++) {
        log_config_instance(&(instances[i]));
    }
    if (num_configs > 0) {
        (*p_thermal_client_config_cleanup)(instances,num_configs);
    }

    num_configs = (*p_thermal_client_config_query)("monitor", &instances);
    if (num_configs <= 0) {
        return;
    }
    for (int i = 0; i < num_configs; i++) {
        log_config_instance(&(instances[i]));
    }
    if (num_configs > 0) {
        (*p_thermal_client_config_cleanup)(instances,num_configs);
    }
}

/**
 * Load the thermal client library
 * returns 0 on success
 */
static int load_thermal_client(void)
{
    char *thermal_client_so = "vendor/lib64/libthermalclient.so";

    dlhandle = dlopen(thermal_client_so, RTLD_NOW | RTLD_LOCAL);
    if (dlhandle) {
        dlerror();
        p_thermal_client_config_query = (int (*) (char *, struct config_instance **))
            dlsym(dlhandle, "thermal_client_config_query");
        if (dlerror()) {
            ALOGE("Unable to load thermal_client_config_query");
            goto error_handle;
        }

        p_thermal_client_config_set = (int (*) (struct config_instance *, unsigned int))
            dlsym(dlhandle, "thermal_client_config_set");
        if (dlerror()) {
            ALOGE("Unable to load thermal_client_config_set");
            goto error_handle;
        }

        p_thermal_client_config_cleanup = (void (*) (struct config_instance *, unsigned int))
            dlsym(dlhandle, "thermal_client_config_cleanup");
        if (dlerror()) {
            ALOGE("Unable to load thermal_client_config_cleanup");
            goto error_handle;
        }
    } else {
        ALOGE("unable to open %s", thermal_client_so);
        return -1;
    }

    return 0;

error_handle:
    ALOGE("Error opening functions from %s", thermal_client_so);
    p_thermal_client_config_query = NULL;
    p_thermal_client_config_set = NULL;
    p_thermal_client_config_cleanup = NULL;
    dlclose(dlhandle);
    dlhandle = NULL;
    return -1;
}

/**
 *  Allocate a new struct config_instance for modifying the disable field
 */
static struct config_instance *allocate_config_instance(){
    struct config_instance *config = (struct config_instance *)malloc(sizeof(struct config_instance));
    memset(config, 0, sizeof(*config));

    config->cfg_desc = (char *)malloc(sizeof(char)*max_string_size);
    memset(config->cfg_desc, 0, sizeof(char)*max_string_size);

    config->algo_type = (char *)malloc(sizeof(char)*max_string_size);
    memset(config->algo_type, 0, sizeof(char) * max_string_size);

    config->fields = (struct field_data *)malloc(sizeof(struct field_data));
    memset(config->fields, 0, sizeof(*config->fields));

    config->fields[0].field_name = (char*)malloc(sizeof(char)*max_string_size);
    memset(config->fields[0].field_name, 0, sizeof(char)*max_string_size);

    config->fields[0].data = (void*)malloc(sizeof(int));

    return config;
}
/**
 *  Free the config_instance as allocated in allocate_config_instance
 */
static void free_config_instance(struct config_instance *config){

    free(config->fields[0].data);
    free(config->fields[0].field_name);
    free(config->fields);
    free(config->algo_type);
    free(config->cfg_desc);
    free(config);
}

/**
 *  disable a thermal config
 *  returns 1 on success, anything else is a failure
 */
static int disable_config(char *config_name, char *algo_type){
    int result = 0;
    if (error_state) {
        return 0;
    }
    struct config_instance *config = allocate_config_instance();
    strlcpy(config->cfg_desc, config_name, max_string_size);
    strlcpy(config->algo_type, algo_type, max_string_size);
    strlcpy(config->fields[0].field_name, "disable", max_string_size);

    config->fields_mask |= DISABLE_FIELD;
    config->num_fields = 1;
    config->fields[0].data_type = FIELD_INT;
    config->fields[0].num_data = 1;
    *(int*)(config->fields[0].data) = 1; //DISABLE


    result = (*p_thermal_client_config_set)(config, 1);
    if (DEBUG) {
        ALOGE("disable profile: name = %s, algo_type = %s, success = %d", config_name, algo_type, result);
    }
    free_config_instance(config);

    return result;
}

/**
 *  enable a thermal config
 *  returns 1 on success, anything else is failure
 */
static int enable_config(char *config_name, char *algo_type){
    int result = 0;
    if (error_state) {
        return 0;
    }
    struct config_instance *config = allocate_config_instance();
    strlcpy(config->cfg_desc, config_name, max_string_size);
    strlcpy(config->algo_type, algo_type, max_string_size);
    strlcpy(config->fields[0].field_name, "disable", max_string_size);

    config->fields_mask |= DISABLE_FIELD;
    config->num_fields = 1;
    config->fields[0].data_type = FIELD_INT;
    config->fields[0].num_data = 1;
    *(int*)(config->fields[0].data) = 0;  //ENABLE

    result = (*p_thermal_client_config_set)(config, 1);
    if (DEBUG) {
        ALOGE("enable profile: name = %s, algo_type = %s, success = %d",
          config_name, algo_type, result);
    }

    free_config_instance(config);

    return result;
}

/**
 * Call this if there is a compoenent-fatal error
 * Attempts to clean up any outstanding thermal config state
 */
static void error_cleanup(){
    //disable VR configs, best-effort so ignore return values
    for (unsigned int i = 0; i < NUM_VR_CONFIGS; i++) {
        disable_config(vr_thermal_configs[i][0], vr_thermal_configs[i][1]);
    }

    // enable non-VR profile, best-effort so ignore return values
    for (unsigned int i = 0; i < NUM_NON_VR_CONFIGS; i++) {
        enable_config(non_vr_thermal_configs[i][0], non_vr_thermal_configs[i][1]);
    }

    // set global error flag
    error_state = 1;
}

/*
 * Set global display/GPU/scheduler configuration to used for VR apps.
 */
static void set_vr_thermal_configuration() {
    int result = 1;
    if (error_state) {
        return;
    }

    //disable non-VR configs
    for (unsigned int i = 0; i < NUM_NON_VR_CONFIGS; i++) {
        result = disable_config(non_vr_thermal_configs[i][0], non_vr_thermal_configs[i][1]);
        if (result != 1) {
            goto error;
        }
    }

    //enable VR configs
    for (unsigned int i = 0; i < NUM_VR_CONFIGS; i++) {
        result = enable_config(vr_thermal_configs[i][0], vr_thermal_configs[i][1]);
        if (result != 1) {
            goto error;
        }
    }

    if (DEBUG) {
        query_thermal_config();
    }

    return;

error:
    error_cleanup();
    return;
}

/*
 * Reset to default global display/GPU/scheduler configuration.
 */
static void unset_vr_thermal_configuration() {
    int result = 1;
    if (error_state) {
        return;
    }

    //disable VR configs
    for (unsigned int i = 0; i < NUM_VR_CONFIGS; i++) {
        result = disable_config(vr_thermal_configs[i][0], vr_thermal_configs[i][1]);
        if (result != 1) {
            goto error;
        }
    }

    // enable non-VR profile
    for (unsigned int i = 0; i < NUM_NON_VR_CONFIGS; i++) {
        result = enable_config(non_vr_thermal_configs[i][0], non_vr_thermal_configs[i][1]);
        if (result != 1) {
            goto error;
        }
    }

    if (DEBUG) {
        query_thermal_config();
    }

    return;

error:
    error_cleanup();
    return;
}

static void vr_init(struct vr_module *module) {
    int success = load_thermal_client();
    if (success != 0) {
        ALOGE("failed to load thermal client");
        error_state = 1;
    }
}

static void vr_set_vr_mode(struct vr_module *module, bool enabled) {
    if (enabled) {
        set_vr_thermal_configuration();
    } else {
        unset_vr_thermal_configuration();
    }
}

static struct hw_module_methods_t vr_module_methods = {
    .open = NULL, // There are no devices for this HAL interface.
};


vr_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag                = HARDWARE_MODULE_TAG,
        .module_api_version = VR_MODULE_API_VERSION_1_0,
        .hal_api_version    = HARDWARE_HAL_API_VERSION,
        .id                 = VR_HARDWARE_MODULE_ID,
        .name               = "OnePlus 3 VR HAL",
        .author             = "The Android Open Source Project",
        .methods            = &vr_module_methods,
    },

    .init = vr_init,
    .set_vr_mode = vr_set_vr_mode,
};
