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

#include <utils/Log.h>
#include <stdlib.h>
#include <string.h>
#include "effect_util.h"
#include <string.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "effect_util"

/*#define LOG_NDEBUG 0*/

enum {
    EQUALIZER,
    VIRTUALIZER,
    BASSBOOST,
};

static const char *paramList[10] = {
                              "eq_enable",
                              "virt_enable",
                              "bb_enable",
                              "eq_param_level0",
                              "eq_param_level1",
                              "eq_param_level2",
                              "eq_param_level3",
                              "eq_param_level4",
                              "virt_param_strength",
                              "bassboost_param_strength"
};

#define EFFECT_FILE "/data/misc/dts/effect"
#define MAX_LENGTH_OF_INTEGER_IN_STRING 13

#ifdef DTS_EAGLE
void create_effect_state_node(int device_id)
{
    char prop[PROPERTY_VALUE_MAX];
    int fd;
    char buf[1024];
    char path[PATH_MAX];
    char value[MAX_LENGTH_OF_INTEGER_IN_STRING];

    property_get("use.dts_eagle", prop, "0");
    if (!strncmp("true", prop, sizeof("true")) || atoi(prop)) {
        ALOGV("create_effect_node for - device_id: %d", device_id);
        strlcpy(path, EFFECT_FILE, sizeof(path));
        snprintf(value, sizeof(value), "%d", device_id);
        strlcat(path, value, sizeof(path));
        if ((fd=open(path, O_RDONLY)) < 0) {
            ALOGV("No File exist");
        } else {
            ALOGV("A file with the same name exist. So, not creating again");
            return;
        }
        if ((fd=creat(path, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0) {
            ALOGE("opening effect state node failed returned");
            return;
        }
        chmod(path, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH);
        snprintf(buf, sizeof(buf), "eq_enable=%d;virt_enable=%d;bb_enable=%d;eq_param_level0=%d;eq_param_level1=%d;eq_param_level2=%d;eq_param_level3=%d;eq_param_level4=%d;virt_param_strength=%d;bassboost_param_strength=%d", 0,0,0,0,0,0,0,0,0,0);
        int n = write(fd, buf, strlen(buf));
        ALOGV("number of bytes written: %d", n);
        close(fd);
    }
}

void update_effects_node(int device_id, int effect_type, int enable_or_set, int enable_disable, int strength, int eq_band, int eq_level)
{
    char prop[PROPERTY_VALUE_MAX];
    char buf[1024];
    int fd = 0;
    int paramValue = 0;
    char path[PATH_MAX];
    char value[MAX_LENGTH_OF_INTEGER_IN_STRING];
    char parameterValue[MAX_LENGTH_OF_INTEGER_IN_STRING];
    int keyParamIndex = -1; //index in the paramlist array which has to be updated
    char *s1, *s2;
    char resultBuf[1024];
    int index1 = -1;
  //ALOGV("value of device_id and effect_type is %d and %d", device_id, effect_type);
    property_get("use.dts_eagle", prop, "0");
    if (!strncmp("true", prop, sizeof("true")) || atoi(prop)) {
        strlcpy(path, EFFECT_FILE, sizeof(path));
        snprintf(value, sizeof(value), "%d", device_id);
        strlcat(path, value, sizeof(path));
        switch (effect_type)
        {
        case EQUALIZER:
            if (enable_or_set) {
                keyParamIndex = 0;
                paramValue = enable_disable;
        } else {
            switch (eq_band) {
            case 0:
                keyParamIndex = 3;
                break;
            case 1:
                keyParamIndex = 4;
                break;
            case 2:
                keyParamIndex = 5;
                break;
            case 3:
                keyParamIndex = 6;
                break;
            case 4:
                keyParamIndex = 7;
                break;
            default:
                break;
            }
            paramValue = eq_level;
        }
        break;
        case VIRTUALIZER:
            if(enable_or_set) {
                keyParamIndex = 1;
                paramValue = enable_disable;
            } else {
                 keyParamIndex = 8;
                 paramValue = strength;
            }
            break;
        case BASSBOOST:
            if (enable_or_set) {
                keyParamIndex = 2;
                paramValue = enable_disable;
            } else {
                keyParamIndex = 9;
                paramValue = strength;
            }
            break;
         default:
            break;
        }
        if(keyParamIndex !=-1) {
            FILE *fp;
            fp = fopen(path,"r");
            if (fp != NULL) {
                memset(buf, 0, 1024);
                memset(resultBuf, 0, 1024);
                if (fgets(buf, 1024, fp) != NULL) {
                    s1 = strstr(buf, paramList[keyParamIndex]);
                    s2 = strstr(s1,";");
                    index1 = s1 - buf;
                    strncpy(resultBuf, buf, index1);
                    strncat(resultBuf, paramList[keyParamIndex], sizeof(resultBuf)-strlen(resultBuf)-1);
                    strncat(resultBuf, "=", sizeof(resultBuf)-strlen(resultBuf)-1);
                    snprintf(parameterValue, sizeof(parameterValue), "%d", paramValue);
                    strncat(resultBuf, parameterValue, sizeof(resultBuf)-strlen(resultBuf)-1);
                    if (s2)
                        strncat(resultBuf, s2, sizeof(resultBuf)-strlen(resultBuf)-1);
                    fclose(fp);
                    if ((fd=open(path, O_TRUNC|O_WRONLY)) < 0) {
                       ALOGV("opening file for writing failed");
                       return;
                    }
                    int n = write(fd, resultBuf, strlen(resultBuf));
                    close(fd);
                    ALOGV("number of bytes written: %d", n);
                } else {
                    ALOGV("file could not be read");
                    fclose(fp);
                }
            } else
                ALOGV("file could not be opened");
        }
    }
}

void remove_effect_state_node(int device_id)
{
    char prop[PROPERTY_VALUE_MAX];
    int fd;
    char path[PATH_MAX];
    char value[MAX_LENGTH_OF_INTEGER_IN_STRING];

    property_get("use.dts_eagle", prop, "0");
    if (!strncmp("true", prop, sizeof("true")) || atoi(prop)) {
        ALOGV("remove_state_notifier_node: device_id - %d", device_id);
        strlcpy(path, EFFECT_FILE, sizeof(path));
        snprintf(value, sizeof(value), "%d", device_id);
        strlcat(path, value, sizeof(path));
        if ((fd=open(path, O_RDONLY)) < 0) {
            ALOGV("open effect state node failed");
        } else {
            ALOGV("open effect state node successful");
            ALOGV("Remove the file");
            close(fd);
            remove(path);
        }
    }
}
#endif
