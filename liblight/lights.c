/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2014 The Linux Foundation. All rights reserved.
 * Copyright (C) 2015 The CyanogenMod Project
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

#define LOG_TAG "lights"
#include <cutils/log.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <sys/types.h>

#include <hardware/lights.h>

/******************************************************************************/

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static struct light_state_t g_attention;
static struct light_state_t g_notification;
static struct light_state_t g_battery;

char const*const RED_LED_FILE
        = "/sys/class/leds/red/brightness";

char const*const GREEN_LED_FILE
        = "/sys/class/leds/green/brightness";

char const*const BLUE_LED_FILE
        = "/sys/class/leds/blue/brightness";

char const*const LCD_FILE
        = "/sys/class/leds/lcd-backlight/brightness";

const char*const BUTTONS_FILE
        = "/sys/class/leds/button-backlight/brightness";

char const*const RED_DUTY_PCTS_FILE
        = "/sys/class/leds/red/duty_pcts";

char const*const GREEN_DUTY_PCTS_FILE
        = "/sys/class/leds/green/duty_pcts";

char const*const BLUE_DUTY_PCTS_FILE
        = "/sys/class/leds/blue/duty_pcts";

char const*const RED_START_IDX_FILE
        = "/sys/class/leds/red/start_idx";

char const*const GREEN_START_IDX_FILE
        = "/sys/class/leds/green/start_idx";

char const*const BLUE_START_IDX_FILE
        = "/sys/class/leds/blue/start_idx";

char const*const RED_PAUSE_LO_FILE
        = "/sys/class/leds/red/pause_lo";

char const*const GREEN_PAUSE_LO_FILE
        = "/sys/class/leds/green/pause_lo";

char const*const BLUE_PAUSE_LO_FILE
        = "/sys/class/leds/blue/pause_lo";

char const*const RED_PAUSE_HI_FILE
        = "/sys/class/leds/red/pause_hi";

char const*const GREEN_PAUSE_HI_FILE
        = "/sys/class/leds/green/pause_hi";

char const*const BLUE_PAUSE_HI_FILE
        = "/sys/class/leds/blue/pause_hi";

char const*const RED_RAMP_STEP_MS_FILE
        = "/sys/class/leds/red/ramp_step_ms";

char const*const GREEN_RAMP_STEP_MS_FILE
        = "/sys/class/leds/green/ramp_step_ms";

char const*const BLUE_RAMP_STEP_MS_FILE
        = "/sys/class/leds/blue/ramp_step_ms";

char const*const RED_BLINK_FILE
        = "/sys/class/leds/red/blink";

char const*const GREEN_BLINK_FILE
        = "/sys/class/leds/green/blink";

char const*const BLUE_BLINK_FILE
        = "/sys/class/leds/blue/blink";

char const*const RGB_BLINK_FILE
        = "/sys/class/leds/rgb/rgb_blink";

#define RAMP_SIZE 8
static int BRIGHTNESS_RAMP[RAMP_SIZE]
        = { 0, 12, 25, 37, 50, 72, 85, 100 };
#define RAMP_STEP_DURATION 50

/**
 * device methods
 */

static int
write_int(char const* path, int value)
{
    int fd;
    static int already_warned = 0;

    fd = open(path, O_RDWR);
    if (fd >= 0) {
        char buffer[20];
        int bytes = snprintf(buffer, sizeof(buffer), "%d\n", value);
        ssize_t amt = write(fd, buffer, (size_t)bytes);
        close(fd);
        return amt == -1 ? -errno : 0;
    } else {
        if (already_warned == 0) {
            ALOGE("write_int failed to open %s\n", path);
            already_warned = 1;
        }
        return -errno;
    }
}

static int
write_str(char const* path, char* value)
{
    int fd;
    static int already_warned = 0;

    fd = open(path, O_RDWR);
    if (fd >= 0) {
        char buffer[1024];
        int bytes = snprintf(buffer, sizeof(buffer), "%s\n", value);
        ssize_t amt = write(fd, buffer, (size_t)bytes);
        close(fd);
        return amt == -1 ? -errno : 0;
    } else {
        if (already_warned == 0) {
            ALOGE("write_int failed to open %s\n", path);
            already_warned = 1;
        }
        return -errno;
    }
}

void init_globals(void)
{
    // init the mutex
    pthread_mutex_init(&g_lock, NULL);
}

static int
is_lit(struct light_state_t const* state)
{
    return state->color & 0x00ffffff;
}

static int
rgb_to_brightness(struct light_state_t const* state)
{
    int color = state->color & 0x00ffffff;
    return ((77*((color>>16)&0x00ff))
            + (150*((color>>8)&0x00ff)) + (29*(color&0x00ff))) >> 8;
}

static int
set_light_backlight(struct light_device_t* dev,
        struct light_state_t const* state)
{
    int err = 0;
    int brightness = rgb_to_brightness(state);
    if(!dev) {
        return -1;
    }
    pthread_mutex_lock(&g_lock);
    err = write_int(LCD_FILE, brightness);
    pthread_mutex_unlock(&g_lock);
    return err;
}

static int
set_light_buttons(struct light_device_t *dev,
        const struct light_state_t *state)
{
    int err = 0;
    int brightness = rgb_to_brightness(state);
    if(!dev) {
        return -1;
    }
    pthread_mutex_lock(&g_lock);
    err = write_int(BUTTONS_FILE, brightness);
    pthread_mutex_unlock(&g_lock);
    return err;
}

static char*
get_scaled_duty_pcts(int brightness)
{
    char *buf = malloc(5 * RAMP_SIZE * sizeof(char));
    char *pad = "";
    int i = 0;

    memset(buf, 0, 5 * RAMP_SIZE * sizeof(char));

    for (i = 0; i < RAMP_SIZE; i++) {
        char temp[5] = "";
        snprintf(temp, sizeof(temp), "%s%d", pad, (BRIGHTNESS_RAMP[i] * brightness / 255));
        strcat(buf, temp);
        pad = ",";
    }
    ALOGV("%s: brightness=%d duty=%s", __func__, brightness, buf);
    return buf;
}

static int
set_speaker_light_locked(struct light_device_t* dev,
        struct light_state_t const* state)
{
    int red, green, blue, blink;
    int onMS, offMS, stepDuration, pauseHi;
    unsigned int colorRGB;
    char *duty;

    if(!dev) {
        return -1;
    }

    switch (state->flashMode) {
        case LIGHT_FLASH_TIMED:
            onMS = state->flashOnMS;
            offMS = state->flashOffMS;
            break;
        case LIGHT_FLASH_NONE:
        default:
            onMS = 0;
            offMS = 0;
            break;
    }

    colorRGB = state->color;

    ALOGV("set_speaker_light_locked mode %d, colorRGB=%08X, onMS=%d, offMS=%d\n",
            state->flashMode, colorRGB, onMS, offMS);

    red = (colorRGB >> 16) & 0xFF;
    green = (colorRGB >> 8) & 0xFF;
    blue = colorRGB & 0xFF;
    blink = onMS > 0 && offMS > 0;

    // disable all blinking to start
    write_int(RGB_BLINK_FILE, 0);

    if (blink) {
        stepDuration = RAMP_STEP_DURATION;
        pauseHi = onMS - (stepDuration * RAMP_SIZE * 2);
        if (stepDuration * RAMP_SIZE * 2 > onMS) {
            stepDuration = onMS / (RAMP_SIZE * 2);
            pauseHi = 0;
        }

        // red
        write_int(RED_START_IDX_FILE, 0);
        duty = get_scaled_duty_pcts(red);    
        write_str(RED_DUTY_PCTS_FILE, duty);
        write_int(RED_PAUSE_LO_FILE, offMS);
        // The led driver is configured to ramp up then ramp
        // down the lut. This effectively doubles the ramp duration.
        write_int(RED_PAUSE_HI_FILE, pauseHi);
        write_int(RED_RAMP_STEP_MS_FILE, stepDuration);
        free(duty);

        // green
        write_int(GREEN_START_IDX_FILE, RAMP_SIZE);
        duty = get_scaled_duty_pcts(green);
        write_str(GREEN_DUTY_PCTS_FILE, duty);
        write_int(GREEN_PAUSE_LO_FILE, offMS);
        // The led driver is configured to ramp up then ramp
        // down the lut. This effectively doubles the ramp duration.
        write_int(GREEN_PAUSE_HI_FILE, pauseHi);
        write_int(GREEN_RAMP_STEP_MS_FILE, stepDuration);
        free(duty);

        // blue
        write_int(BLUE_START_IDX_FILE, RAMP_SIZE * 2);
        duty = get_scaled_duty_pcts(blue);
        write_str(BLUE_DUTY_PCTS_FILE, duty);
        write_int(BLUE_PAUSE_LO_FILE, offMS);
        // The led driver is configured to ramp up then ramp
        // down the lut. This effectively doubles the ramp duration.
        write_int(BLUE_PAUSE_HI_FILE, pauseHi);
        write_int(BLUE_RAMP_STEP_MS_FILE, stepDuration);
        free(duty);

        // start the party
        write_int(RGB_BLINK_FILE, 1);

    } else {
        if (red == 0 && green == 0 && blue == 0) {
            write_int(RED_BLINK_FILE, 0);
            write_int(GREEN_BLINK_FILE, 0);
            write_int(BLUE_BLINK_FILE, 0);
        }
        write_int(RED_LED_FILE, red);
        write_int(GREEN_LED_FILE, green);
        write_int(BLUE_LED_FILE, blue);
    }


    return 0;
}

static void
handle_speaker_light_locked(struct light_device_t* dev)
{
    if (is_lit(&g_attention)) {
        set_speaker_light_locked(dev, &g_attention);
    } else if (is_lit(&g_notification)) {
        set_speaker_light_locked(dev, &g_notification);
    } else {
        set_speaker_light_locked(dev, &g_battery);
    }
}

static int
set_light_battery(struct light_device_t* dev,
        struct light_state_t const* state)
{
    pthread_mutex_lock(&g_lock);
    g_battery = *state;
    handle_speaker_light_locked(dev);
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int
set_light_notifications(struct light_device_t* dev,
        struct light_state_t const* state)
{
    pthread_mutex_lock(&g_lock);

    unsigned int brightness;
    unsigned int color;
    unsigned int rgb[3];

    g_notification = *state;

    // If a brightness has been applied by the user
    brightness = (g_notification.color & 0xFF000000) >> 24;
    if (brightness > 0 && brightness < 0xFF) {

        // Retrieve each of the RGB colors
        color = g_notification.color & 0x00FFFFFF;
        rgb[0] = (color >> 16) & 0xFF;
        rgb[1] = (color >> 8) & 0xFF;
        rgb[2] = color & 0xFF;

        // Apply the brightness level
        if (rgb[0] > 0)
            rgb[0] = (rgb[0] * brightness) / 0xFF;
        if (rgb[1] > 0)
            rgb[1] = (rgb[1] * brightness) / 0xFF;
        if (rgb[2] > 0)
            rgb[2] = (rgb[2] * brightness) / 0xFF;

        // Update with the new color
        g_notification.color = (rgb[0] << 16) + (rgb[1] << 8) + rgb[2];
    }

    handle_speaker_light_locked(dev);
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int
set_light_attention(struct light_device_t* dev,
        struct light_state_t const* state)
{
    pthread_mutex_lock(&g_lock);
    g_attention = *state;
    handle_speaker_light_locked(dev);
    pthread_mutex_unlock(&g_lock);
    return 0;
}

/** Close the lights device */
static int
close_lights(struct light_device_t *dev)
{
    if (dev) {
        free(dev);
    }
    return 0;
}


/******************************************************************************/

/**
 * module methods
 */

/** Open a new instance of a lights device using name */
static int open_lights(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
    int (*set_light)(struct light_device_t* dev,
            struct light_state_t const* state);

    if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
        set_light = set_light_backlight;
    else if (0 == strcmp(LIGHT_ID_BUTTONS, name))
        set_light = set_light_buttons;
    else if (0 == strcmp(LIGHT_ID_BATTERY, name))
        set_light = set_light_battery;
    else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name))
        set_light = set_light_notifications;
    else if (0 == strcmp(LIGHT_ID_ATTENTION, name))
        set_light = set_light_attention;
    else
        return -EINVAL;

    pthread_once(&g_init, init_globals);

    struct light_device_t *dev = malloc(sizeof(struct light_device_t));

    if(!dev)
        return -ENOMEM;

    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*)module;
    dev->common.close = (int (*)(struct hw_device_t*))close_lights;
    dev->set_light = set_light;

    *device = (struct hw_device_t*)dev;
    return 0;
}

static struct hw_module_methods_t lights_module_methods = {
    .open =  open_lights,
};

/*
 * The lights Module
 */
struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "Lights Module",
    .author = "The CyanogenMod Project",
    .methods = &lights_module_methods,
};
