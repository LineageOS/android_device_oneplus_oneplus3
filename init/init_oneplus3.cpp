/*
   Copyright (c) 2016, The CyanogenMod Project
             (c) 2017-2019, The LineageOS Project

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <android-base/logging.h>
#include <android-base/properties.h>

#include <string>
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

#include "property_service.h"
#include "vendor_init.h"

using ::android::init::property_set;

constexpr const char* RO_PROP_SOURCES[] = {
        nullptr, "product.", "product_services.", "odm.", "vendor.", "system.", "bootimage.",
};

constexpr const char* BUILD_DESCRIPTION[] = {
        "OnePlus3-user 9 PKQ1.181203.001 1907311932 release-keys",
        "OnePlus3-user 9 PKQ1.181203.001 1907311932 release-keys",
};

constexpr const char* BUILD_FINGERPRINT[] = {
        "OnePlus/OnePlus3/OnePlus3:9/PKQ1.181203.001/1907311932:user/release-keys",
        "OnePlus/OnePlus3/OnePlus3T:9/PKQ1.181203.001/1907311932:user/release-keys",
};

void property_override(char const prop[], char const value[]) {
    prop_info* pi = (prop_info*)__system_property_find(prop);
    if (pi) {
        __system_property_update(pi, value, strlen(value));
    }
}

void load_props(const char* model, bool is_3t = false) {
    const auto ro_prop_override = [](const char* source, const char* prop, const char* value,
                                     bool product) {
        std::string prop_name = "ro.";

        if (product) prop_name += "product.";
        if (source != nullptr) prop_name += source;
        if (!product) prop_name += "build.";
        prop_name += prop;

        property_override(prop_name.c_str(), value);
    };

    for (const auto& source : RO_PROP_SOURCES) {
        ro_prop_override(source, "device", is_3t ? "OnePlus3T" : "OnePlus3", true);
        ro_prop_override(source, "model", model, true);
        ro_prop_override(source, "fingerprint", is_3t ? BUILD_FINGERPRINT[1] : BUILD_FINGERPRINT[0],
                         false);
    }
    ro_prop_override(nullptr, "description", is_3t ? BUILD_DESCRIPTION[1] : BUILD_DESCRIPTION[0],
                     false);
    ro_prop_override(nullptr, "product", "OnePlus3", false);

    // ro.build.fingerprint property has not been set
    if (is_3t) {
        property_set("ro.build.fingerprint", BUILD_FINGERPRINT[1]);
        property_set("ro.power_profile.override", "power_profile_3t");
    } else {
        property_set("ro.build.fingerprint", BUILD_FINGERPRINT[0]);
    }
}

void vendor_load_properties() {
    int rf_version = stoi(android::base::GetProperty("ro.boot.rf_version", "0"));

    switch (rf_version) {
        case 11:
        case 31:
            /* China / North America model */
            load_props("ONEPLUS A3000");
            break;
        case 21:
            /* Europe / Asia model */
            load_props("ONEPLUS A3003");
            break;
        case 12:
            /* China model */
            load_props("ONEPLUS A3010", true);
            break;
        case 22:
            /* Europe / Asia model */
            load_props("ONEPLUS A3003", true);
            break;
        case 32:
            /* North America model */
            load_props("ONEPLUS A3000", true);
            break;
        default:
            LOG(ERROR) << __func__ << ": unexcepted rf version!";
    }
}
