/*
 * Copyright (C) 2016 The CyanogenMod Project
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

package com.android.internal.telephony;

import static com.android.internal.telephony.RILConstants.*;

import android.content.Context;

/**
 * Custom Qualcomm RIL for OnePlus 3
 *
 * {@hide}
 */
public class OP3RIL extends RIL implements CommandsInterface {

    static final int RIL_REQUEST_FACTORY_MODE_NV_PROCESS = 138;
    static final int RIL_REQUEST_FACTORY_MODE_MODEM_GPIO = 139;
    static final int RIL_REQUEST_GET_BAND_MODE = 140;
    static final int RIL_REQUEST_REPORT_BOOTUPNVRESTOR_STATE = 141;
    static final int RIL_REQUEST_GET_RFFE_DEV_INFO = 142;
    static final int RIL_REQUEST_SIM_TRANSMIT_BASIC = 144;
    static final int RIL_REQUEST_SIM_TRANSMIT_CHANNEL = 147;
    static final int RIL_REQUEST_GO_TO_ERROR_FATAL = 148;
    static final int RIL_REQUEST_GET_MDM_BASEBAND = 149;
    static final int RIL_REQUEST_SET_TDD_LTE = 150;

    static final int RIL_UNSOL_OEM_NV_BACKUP_RESPONSE = 1046;
    static final int RIL_UNSOL_RAC_UPDATE = 1047;

    public OP3RIL(Context context, int preferredNetworkType, int cdmaSubscription) {
        super(context, preferredNetworkType, cdmaSubscription, null);
    }

    public OP3RIL(Context context, int preferredNetworkType,
            int cdmaSubscription, Integer instanceId) {
        super(context, preferredNetworkType, cdmaSubscription, instanceId);
    }

    @Override
    static String
    requestToString(int request) {
/*
 cat libs/telephony/ril_commands.h \
 | egrep "^ *{RIL_" \
 | sed -re 's/\{RIL_([^,]+),[^,]+,([^}]+).+/case RIL_\1: return "\1";/'
*/
        switch(request) {
            case RIL_REQUEST_FACTORY_MODE_NV_PROCESS: return "RIL_REQUEST_FACTORY_MODE_NV_PROCESS";
            case RIL_REQUEST_FACTORY_MODE_MODEM_GPIO: return "RIL_REQUEST_FACTORY_MODE_MODEM_GPIO";
            case RIL_REQUEST_GET_BAND_MODE: return "RIL_REQUEST_GET_BAND_MODE";
            case RIL_REQUEST_GET_RFFE_DEV_INFO: return "GET_RFFE_DEV_INFO";
            case RIL_REQUEST_GO_TO_ERROR_FATAL: return "RIL_REQUEST_GO_TO_ERROR_FATAL";
            case RIL_REQUEST_GET_MDM_BASEBAND: return "RIL_REQUEST_GET_MDM_BASEBAND";
            default: return RIL.requestToString(request);
        }
    }
}
