/*
   Copyright (c) 2016, The CyanogenMod Project
             (c) 2017-2018, The LineageOS Project

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
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

#include "property_service.h"
#include "vendor_init.h"

using android::init::property_set;

void property_override(char const prop[], char const value[])
{
    prop_info *pi;

    pi = (prop_info*) __system_property_find(prop);
    if (pi)
        __system_property_update(pi, value, strlen(value));
    else
        __system_property_add(prop, strlen(prop), value, strlen(value));
}

void property_override_dual(char const system_prop[], char const vendor_prop[], char const value[])
{
    property_override(system_prop, value);
    property_override(vendor_prop, value);
}

void load_op3(const char *model) {
    property_override_dual("ro.product.model", "ro.product.vendor.model", model);
    property_override("ro.build.product", "OnePlus3");
    property_override_dual("ro.product.device", "ro.product.vendor.device", "OnePlus3");
    property_override("ro.build.description", "OnePlus3-user 9 PKQ1.181203.001 1907311932 release-keys");
    property_override_dual("ro.build.fingerprint", "ro.vendor.build.fingerprint", "OnePlus/OnePlus3/OnePlus3:9/PKQ1.181203.001/1907311932:user/release-keys");
}

void load_op3t(const char *model) {
    property_override_dual("ro.product.model", "ro.product.vendor.model", model);
    property_override("ro.build.product", "OnePlus3");
    property_override_dual("ro.product.device", "ro.product.vendor.device", "OnePlus3T");
    property_override("ro.build.description", "OnePlus3-user 9 PKQ1.181203.001 1907311932 release-keys");
    property_override_dual("ro.build.fingerprint", "ro.vendor.build.fingerprint", "OnePlus/OnePlus3/OnePlus3T:9/PKQ1.181203.001/1907311932:user/release-keys");
    property_set("ro.power_profile.override", "power_profile_3t");
}

void vendor_load_properties() {
    int rf_version = stoi(android::base::GetProperty("ro.boot.rf_version", ""));

    switch (rf_version) {
    case 11:
    case 31:
        /* China / North America model */
        load_op3("ONEPLUS A3000");
        break;
    case 21:
        /* Europe / Asia model */
        load_op3("ONEPLUS A3003");
        break;
    case 12:
        /* China model */
        load_op3t("ONEPLUS A3010");
        break;
    case 22:
        /* Europe / Asia model */
        load_op3t("ONEPLUS A3003");
        break;
    case 32:
        /* North America model */
        load_op3t("ONEPLUS A3000");
        break;
    default:
        LOG(ERROR) << __func__ << ": unexcepted rf version!";
    }
}
