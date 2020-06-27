/*
 * Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 * Not a Contribution
 */
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

#define LOG_TAG "LocSvc__AGnssRilInterface"

#include <log_util.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sstream>
#include <string>
#include "Gnss.h"
#include "AGnssRil.h"
#include <DataItemConcreteTypesBase.h>

typedef void* (getLocationInterface)();

namespace android {
namespace hardware {
namespace gnss {
namespace V1_0 {
namespace implementation {


AGnssRil::AGnssRil(Gnss* gnss) : mGnss(gnss) {
    ENTRY_LOG_CALLFLOW();
}

AGnssRil::~AGnssRil() {
    ENTRY_LOG_CALLFLOW();
}

Return<bool> AGnssRil::updateNetworkState(bool connected, NetworkType type, bool /*roaming*/) {
    ENTRY_LOG_CALLFLOW();

    // for XTRA
    if (nullptr != mGnss && ( nullptr != mGnss->getGnssInterface() )) {
        int8_t typeout = loc_core::NetworkInfoDataItemBase::TYPE_UNKNOWN;
        switch(type)
        {
            case IAGnssRil::NetworkType::MOBILE:
                typeout = loc_core::NetworkInfoDataItemBase::TYPE_MOBILE;
                break;
            case IAGnssRil::NetworkType::WIFI:
                typeout = loc_core::NetworkInfoDataItemBase::TYPE_WIFI;
                break;
            case IAGnssRil::NetworkType::MMS:
                typeout = loc_core::NetworkInfoDataItemBase::TYPE_MMS;
                break;
            case IAGnssRil::NetworkType::SUPL:
                typeout = loc_core::NetworkInfoDataItemBase::TYPE_SUPL;
                break;
            case IAGnssRil::NetworkType::DUN:
                typeout = loc_core::NetworkInfoDataItemBase::TYPE_DUN;
                break;
            case IAGnssRil::NetworkType::HIPRI:
                typeout = loc_core::NetworkInfoDataItemBase::TYPE_HIPRI;
                break;
            case IAGnssRil::NetworkType::WIMAX:
                typeout = loc_core::NetworkInfoDataItemBase::TYPE_WIMAX;
                break;
            default:
                typeout = loc_core::NetworkInfoDataItemBase::TYPE_UNKNOWN;
                break;
        }
        mGnss->getGnssInterface()->updateConnectionStatus(connected, typeout);
    }
    return true;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
