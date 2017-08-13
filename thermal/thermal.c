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

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "ThermalHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/thermal.h>

#define MAX_LENGTH                    50

#define CPU_USAGE_FILE                "/proc/stat"
#define TEMPERATURE_FILE_FORMAT       "/sys/class/thermal/thermal_zone%d/temp"
#define CPU_ONLINE_FILE_FORMAT        "/sys/devices/system/cpu/cpu%d/online"

#define BATTERY_SENSOR_NUM            29
#define GPU_SENSOR_NUM                14
#define SKIN_SENSOR_NUM               24

const int CPU_SENSORS[] = {4, 6, 9, 11};

#define CPU_NUM                       (sizeof(CPU_SENSORS) / sizeof(int))
// Sum of CPU_NUM + 3 for GPU, BATTERY, and SKIN.
#define TEMPERATURE_NUM               7

//qcom, therm-reset-temp
#define CPU_SHUTDOWN_THRESHOLD        115
//qcom, limit-temp
#define CPU_THROTTLING_THRESHOLD      60
#define BATTERY_SHUTDOWN_THRESHOLD    60
// device/oneplus/oneplus3/configs/thermal-engine.conf
#define SKIN_THROTTLING_THRESHOLD     47
#define SKIN_SHUTDOWN_THRESHOLD       70
#define VR_THROTTLED_BELOW_MIN        58

#define GPU_LABEL                     "GPU"
#define BATTERY_LABEL                 "battery"
#define SKIN_LABEL                    "skin"

const char *CPU_LABEL[] = {"CPU0", "CPU1", "CPU2", "CPU3"};

/**
 * Reads device temperature.
 *
 * @param sensor_num Number of sensor file with temperature.
 * @param type Device temperature type.
 * @param name Device temperature name.
 * @param mult Multiplier used to translate temperature to Celsius.
 * @param throttling_threshold Throttling threshold for the temperature.
 * @param shutdown_threshold Shutdown threshold for the temperature.
 * @param out Pointer to temperature_t structure that will be filled with current
 *     values.
 *
 * @return 0 on success or negative value -errno on error.
 */
static ssize_t read_temperature(int sensor_num, int type, const char *name, float mult,
        float throttling_threshold, float shutdown_threshold, float vr_throttling_threshold,
        temperature_t *out) {
    FILE *file;
    char file_name[MAX_LENGTH];
    float temp;

    sprintf(file_name, TEMPERATURE_FILE_FORMAT, sensor_num);
    file = fopen(file_name, "r");
    if (file == NULL) {
        ALOGE("%s: failed to open: %s", __func__, strerror(errno));
        return -errno;
    }
    if (1 != fscanf(file, "%f", &temp)) {
        fclose(file);
        ALOGE("%s: failed to read a float: %s", __func__, strerror(errno));
        return errno ? -errno : -EIO;
    }

    fclose(file);

    (*out) = (temperature_t) {
        .type = type,
        .name = name,
        .current_value = temp * mult,
        .throttling_threshold = throttling_threshold,
        .shutdown_threshold = shutdown_threshold,
        .vr_throttling_threshold = vr_throttling_threshold
    };

    return 0;
}

static ssize_t get_cpu_temperatures(temperature_t *list, size_t size) {
    size_t cpu;

    for (cpu = 0; cpu < CPU_NUM; cpu++) {
        if (cpu >= size) {
            break;
        }
        // tsens_tz_sensor[4,6,9,11]: temperature in decidegrees Celsius.
        ssize_t result = read_temperature(CPU_SENSORS[cpu], DEVICE_TEMPERATURE_CPU, CPU_LABEL[cpu],
                0.1, CPU_THROTTLING_THRESHOLD, CPU_SHUTDOWN_THRESHOLD, UNKNOWN_TEMPERATURE,
                &list[cpu]);
        if (result != 0) {
            return result;
        }
    }
    return cpu;
}

static ssize_t get_temperatures(thermal_module_t *module, temperature_t *list, size_t size) {
    ssize_t result = 0;
    size_t current_index = 0;

    if (list == NULL) {
        return TEMPERATURE_NUM;
    }

    result = get_cpu_temperatures(list, size);
    if (result < 0) {
        return result;
    }
    current_index += result;

    // GPU temperature.
    if (current_index < size) {
        // tsens_tz_sensor14: temperature in decidegrees Celsius.
        result = read_temperature(GPU_SENSOR_NUM, DEVICE_TEMPERATURE_GPU, GPU_LABEL, 0.1,
                UNKNOWN_TEMPERATURE, UNKNOWN_TEMPERATURE, UNKNOWN_TEMPERATURE,
                &list[current_index]);
        if (result < 0) {
            return result;
        }
        current_index++;
    }

    // Battery temperature.
    if (current_index < size) {
        // tsens_tz_sensor29: battery: temperature in millidegrees Celsius.
        result = read_temperature(BATTERY_SENSOR_NUM, DEVICE_TEMPERATURE_BATTERY, BATTERY_LABEL,
                0.001, UNKNOWN_TEMPERATURE, BATTERY_SHUTDOWN_THRESHOLD, UNKNOWN_TEMPERATURE,
                &list[current_index]);
        if (result < 0) {
            return result;
        }
        current_index++;
    }

    // Skin temperature.
    if (current_index < size) {
        // tsens_tz_sensor24: temperature in Celsius.
        result = read_temperature(SKIN_SENSOR_NUM, DEVICE_TEMPERATURE_SKIN, SKIN_LABEL, 1.,
                SKIN_THROTTLING_THRESHOLD, SKIN_SHUTDOWN_THRESHOLD, VR_THROTTLED_BELOW_MIN,
                &list[current_index]);
        if (result < 0) {
            return result;
        }
        current_index++;
    }
    return TEMPERATURE_NUM;
}

static ssize_t get_cpu_usages(thermal_module_t *module, cpu_usage_t *list) {
    int vals, cpu_num, online;
    ssize_t read;
    uint64_t user, nice, system, idle, active, total;
    char *line = NULL;
    size_t len = 0;
    size_t size = 0;
    char file_name[MAX_LENGTH];
    FILE *file;
    FILE *cpu_file;

    if (list == NULL) {
        return CPU_NUM;
    }

    file = fopen(CPU_USAGE_FILE, "r");
    if (file == NULL) {
        ALOGE("%s: failed to open: %s", __func__, strerror(errno));
        return -errno;
    }

    while ((read = getline(&line, &len, file)) != -1) {
        // Skip non "cpu[0-9]" lines.
        if (strnlen(line, read) < 4 || strncmp(line, "cpu", 3) != 0 || !isdigit(line[3])) {
            free(line);
            line = NULL;
            len = 0;
            continue;
        }

        vals = sscanf(line, "cpu%d %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64, &cpu_num, &user,
                &nice, &system, &idle);

        free(line);
        line = NULL;
        len = 0;

        if (vals != 5 || size == CPU_NUM) {
            if (vals != 5) {
                ALOGE("%s: failed to read CPU information from file: %s", __func__,
                        strerror(errno));
            } else {
                ALOGE("/proc/stat file has incorrect format.");
            }
            fclose(file);
            return errno ? -errno : -EIO;
        }

        active = user + nice + system;
        total = active + idle;

        // Read online CPU information.
        snprintf(file_name, MAX_LENGTH, CPU_ONLINE_FILE_FORMAT, cpu_num);
        cpu_file = fopen(file_name, "r");
        online = 0;
        if (cpu_file == NULL) {
            ALOGE("%s: failed to open file: %s (%s)", __func__, file_name, strerror(errno));
            fclose(file);
            return -errno;
        }
        if (1 != fscanf(cpu_file, "%d", &online)) {
            ALOGE("%s: failed to read CPU online information from file: %s (%s)", __func__,
                    file_name, strerror(errno));
            fclose(file);
            fclose(cpu_file);
            return errno ? -errno : -EIO;
        }
        fclose(cpu_file);

        list[size] = (cpu_usage_t) {
            .name = CPU_LABEL[size],
            .active = active,
            .total = total,
            .is_online = online
        };

        size++;
    }
    fclose(file);

    if (size != CPU_NUM) {
        ALOGE("/proc/stat file has incorrect format.");
        return -EIO;
    }
    return CPU_NUM;
}

static struct hw_module_methods_t thermal_module_methods = {
    .open = NULL,
};

thermal_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = THERMAL_HARDWARE_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = THERMAL_HARDWARE_MODULE_ID,
        .name = "OnePlus 3 Thermal HAL",
        .author = "The Android Open Source Project",
        .methods = &thermal_module_methods,
    },
    .getTemperatures = get_temperatures,
    .getCpuUsages = get_cpu_usages,
};
