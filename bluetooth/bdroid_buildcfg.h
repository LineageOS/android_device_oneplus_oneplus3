/*
 *
 *  Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *  Not a Contribution, Apache license notifications and license are retained
 *  for attribution purposes only.
 *
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (C) 2017 The LineageOS Project
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

#ifndef _BDROID_BUILDCFG_H
#define _BDROID_BUILDCFG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
int32_t property_get_int32(const char *key, int32_t default_value);
#ifdef __cplusplus
}
#endif

static inline const char* BtmGetDefaultName()
{
    int32_t rf_version = property_get_int32("ro.boot.rf_version", 0);

    switch (rf_version) {
    case 11:
    case 21:
    case 31:
        return "OnePlus 3";
    case 12:
    case 22:
    case 32:
        return "OnePlus 3T";
    }

    return "";
}

#define BTM_DEF_LOCAL_NAME BtmGetDefaultName()
#define BLUETOOTH_QTI_SW TRUE
// Disables read remote device feature
#define MAX_ACL_CONNECTIONS   16
#define MAX_L2CAP_CHANNELS    16
#define BLE_VND_INCLUDED   TRUE
// skips conn update at conn completion
#define BT_CLEAN_TURN_ON_DISABLED 1

/* Increasing SEPs to 12 from 6 to support SHO/MCast i.e. two streams per codec */
#define AVDT_NUM_SEPS 12
#endif
