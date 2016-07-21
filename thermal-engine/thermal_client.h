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

#include <cutils/properties.h>
#ifndef __THERMAL_CLIENT_H__
#define __THERMAL_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ACTIONS  (32)

/* Enum for supported fields */
enum supported_fields {
	UNKNOWN_FIELD = 0x0,
	DISABLE_FIELD = 0x1,
	SAMPLING_FIELD = 0x2,
	THRESHOLDS_FIELD = 0x4,
	SET_POINT_FIELD = THRESHOLDS_FIELD,
	THRESHOLDS_CLR_FIELD = 0x8,
	SET_POINT_CLR_FIELD = THRESHOLDS_CLR_FIELD,
	ACTION_INFO_FIELD = 0x10,
	SUPPORTED_FIELD_MAX = 0x20,
};

enum field_data_type {
	FIELD_INT = 0,
	FIELD_STR,
	FIELD_INT_ARR,
	FIELD_ARR_STR,
	FIELD_ARR_INT_ARR,
	FIELD_MAX
};

struct action_info_data {
	int info[MAX_ACTIONS];
	uint32_t num_actions;
};

struct field_data {
	char *field_name;
	enum field_data_type data_type;
	uint32_t num_data;
	void *data;
};

struct config_instance {
	char *cfg_desc;
	char *algo_type;
	unsigned int fields_mask;  /* mask set by client to request to adjust supported fields */
	uint32_t num_fields;
	struct field_data *fields;
};

int thermal_client_config_query(char *algo_type, struct config_instance **configs);
void thermal_client_config_cleanup(struct config_instance *configs, unsigned int config_size);
int thermal_client_config_set(struct config_instance *configs, unsigned int config_size);

int thermal_client_register_callback(char *client_name, int (*callback)(int , void *, void *), void *data);
int thermal_client_request(char *client_name, int req_data);
void thermal_client_unregister_callback(int client_cb_handle);

#ifdef __cplusplus
}
#endif

#endif /* __THERMAL_CLIENT_H__ */
