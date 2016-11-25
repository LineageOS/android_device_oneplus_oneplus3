/*
 *  (C) 2014 DTS, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EFFECT_UTIL_H_
#define EFFECT_UTIL_H_

#ifdef DTS_EAGLE

#include <cutils/properties.h>
#include <sys/stat.h>
#include <fcntl.h>

enum {
    EFFECT_TYPE_EQ = 0,
    EFFECT_TYPE_VIRT,
    EFFECT_TYPE_BB,
};

enum {
    EFFECT_SET_PARAM = 0,
    EFFECT_ENABLE_PARAM,
};


#define EFFECT_NO_OP 0
#define PCM_DEV_ID 9

void create_effect_state_node(int device_id);
void update_effects_node(int device_id, int effect_type, int enable_or_set, int enable_disable, int strength, int band, int level);
void remove_effect_state_node(int device_id);

#endif /*DTS_EAGLE*/

#endif /*EFFECT_UTIL_H_*/
