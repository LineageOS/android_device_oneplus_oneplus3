/* Copyright (c) 2011-2014,2016-2017 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_CtxBase"

#include <dlfcn.h>
#include <cutils/sched_policy.h>
#include <unistd.h>
#include <ContextBase.h>
#include <msg_q.h>
#include <loc_target.h>
#include <platform_lib_includes.h>
#include <loc_log.h>

namespace loc_core {

loc_gps_cfg_s_type ContextBase::mGps_conf {};
loc_sap_cfg_s_type ContextBase::mSap_conf {};

const loc_param_s_type ContextBase::mGps_conf_table[] =
{
  {"GPS_LOCK",                       &mGps_conf.GPS_LOCK,                       NULL, 'n'},
  {"SUPL_VER",                       &mGps_conf.SUPL_VER,                       NULL, 'n'},
  {"LPP_PROFILE",                    &mGps_conf.LPP_PROFILE,                    NULL, 'n'},
  {"A_GLONASS_POS_PROTOCOL_SELECT",  &mGps_conf.A_GLONASS_POS_PROTOCOL_SELECT,  NULL, 'n'},
  {"LPPE_CP_TECHNOLOGY",             &mGps_conf.LPPE_CP_TECHNOLOGY,             NULL, 'n'},
  {"LPPE_UP_TECHNOLOGY",             &mGps_conf.LPPE_UP_TECHNOLOGY,             NULL, 'n'},
  {"AGPS_CERT_WRITABLE_MASK",        &mGps_conf.AGPS_CERT_WRITABLE_MASK,        NULL, 'n'},
  {"SUPL_MODE",                      &mGps_conf.SUPL_MODE,                      NULL, 'n'},
  {"SUPL_ES",                        &mGps_conf.SUPL_ES,                        NULL, 'n'},
  {"INTERMEDIATE_POS",               &mGps_conf.INTERMEDIATE_POS,               NULL, 'n'},
  {"ACCURACY_THRES",                 &mGps_conf.ACCURACY_THRES,                 NULL, 'n'},
  {"NMEA_PROVIDER",                  &mGps_conf.NMEA_PROVIDER,                  NULL, 'n'},
  {"CAPABILITIES",                   &mGps_conf.CAPABILITIES,                   NULL, 'n'},
  {"XTRA_VERSION_CHECK",             &mGps_conf.XTRA_VERSION_CHECK,             NULL, 'n'},
  {"XTRA_SERVER_1",                  &mGps_conf.XTRA_SERVER_1,                  NULL, 's'},
  {"XTRA_SERVER_2",                  &mGps_conf.XTRA_SERVER_2,                  NULL, 's'},
  {"XTRA_SERVER_3",                  &mGps_conf.XTRA_SERVER_3,                  NULL, 's'},
  {"USE_EMERGENCY_PDN_FOR_EMERGENCY_SUPL",  &mGps_conf.USE_EMERGENCY_PDN_FOR_EMERGENCY_SUPL,          NULL, 'n'},
  {"AGPS_CONFIG_INJECT",             &mGps_conf.AGPS_CONFIG_INJECT,             NULL, 'n'},
  {"EXTERNAL_DR_ENABLED",            &mGps_conf.EXTERNAL_DR_ENABLED,                  NULL, 'n'},
};

const loc_param_s_type ContextBase::mSap_conf_table[] =
{
  {"GYRO_BIAS_RANDOM_WALK",          &mSap_conf.GYRO_BIAS_RANDOM_WALK,          &mSap_conf.GYRO_BIAS_RANDOM_WALK_VALID, 'f'},
  {"ACCEL_RANDOM_WALK_SPECTRAL_DENSITY",     &mSap_conf.ACCEL_RANDOM_WALK_SPECTRAL_DENSITY,    &mSap_conf.ACCEL_RANDOM_WALK_SPECTRAL_DENSITY_VALID, 'f'},
  {"ANGLE_RANDOM_WALK_SPECTRAL_DENSITY",     &mSap_conf.ANGLE_RANDOM_WALK_SPECTRAL_DENSITY,    &mSap_conf.ANGLE_RANDOM_WALK_SPECTRAL_DENSITY_VALID, 'f'},
  {"RATE_RANDOM_WALK_SPECTRAL_DENSITY",      &mSap_conf.RATE_RANDOM_WALK_SPECTRAL_DENSITY,     &mSap_conf.RATE_RANDOM_WALK_SPECTRAL_DENSITY_VALID, 'f'},
  {"VELOCITY_RANDOM_WALK_SPECTRAL_DENSITY",  &mSap_conf.VELOCITY_RANDOM_WALK_SPECTRAL_DENSITY, &mSap_conf.VELOCITY_RANDOM_WALK_SPECTRAL_DENSITY_VALID, 'f'},
  {"SENSOR_ACCEL_BATCHES_PER_SEC",   &mSap_conf.SENSOR_ACCEL_BATCHES_PER_SEC,   NULL, 'n'},
  {"SENSOR_ACCEL_SAMPLES_PER_BATCH", &mSap_conf.SENSOR_ACCEL_SAMPLES_PER_BATCH, NULL, 'n'},
  {"SENSOR_GYRO_BATCHES_PER_SEC",    &mSap_conf.SENSOR_GYRO_BATCHES_PER_SEC,    NULL, 'n'},
  {"SENSOR_GYRO_SAMPLES_PER_BATCH",  &mSap_conf.SENSOR_GYRO_SAMPLES_PER_BATCH,  NULL, 'n'},
  {"SENSOR_ACCEL_BATCHES_PER_SEC_HIGH",   &mSap_conf.SENSOR_ACCEL_BATCHES_PER_SEC_HIGH,   NULL, 'n'},
  {"SENSOR_ACCEL_SAMPLES_PER_BATCH_HIGH", &mSap_conf.SENSOR_ACCEL_SAMPLES_PER_BATCH_HIGH, NULL, 'n'},
  {"SENSOR_GYRO_BATCHES_PER_SEC_HIGH",    &mSap_conf.SENSOR_GYRO_BATCHES_PER_SEC_HIGH,    NULL, 'n'},
  {"SENSOR_GYRO_SAMPLES_PER_BATCH_HIGH",  &mSap_conf.SENSOR_GYRO_SAMPLES_PER_BATCH_HIGH,  NULL, 'n'},
  {"SENSOR_CONTROL_MODE",            &mSap_conf.SENSOR_CONTROL_MODE,            NULL, 'n'},
  {"SENSOR_USAGE",                   &mSap_conf.SENSOR_USAGE,                   NULL, 'n'},
  {"SENSOR_ALGORITHM_CONFIG_MASK",   &mSap_conf.SENSOR_ALGORITHM_CONFIG_MASK,   NULL, 'n'},
  {"SENSOR_PROVIDER",                &mSap_conf.SENSOR_PROVIDER,                NULL, 'n'}
};

void ContextBase::readConfig()
{
   /*Defaults for gps.conf*/
   mGps_conf.INTERMEDIATE_POS = 0;
   mGps_conf.ACCURACY_THRES = 0;
   mGps_conf.NMEA_PROVIDER = 0;
   mGps_conf.GPS_LOCK = 0;
   mGps_conf.SUPL_VER = 0x10000;
   mGps_conf.SUPL_MODE = 0x1;
   mGps_conf.SUPL_ES = 0;
   mGps_conf.CAPABILITIES = 0x7;
   /* LTE Positioning Profile configuration is disable by default*/
   mGps_conf.LPP_PROFILE = 0;
   /*By default no positioning protocol is selected on A-GLONASS system*/
   mGps_conf.A_GLONASS_POS_PROTOCOL_SELECT = 0;
   /*XTRA version check is disabled by default*/
   mGps_conf.XTRA_VERSION_CHECK=0;
   /*Use emergency PDN by default*/
   mGps_conf.USE_EMERGENCY_PDN_FOR_EMERGENCY_SUPL = 1;
   /* By default no LPPe CP technology is enabled*/
   mGps_conf.LPPE_CP_TECHNOLOGY = 0;
   /* By default no LPPe UP technology is enabled*/
   mGps_conf.LPPE_UP_TECHNOLOGY = 0;

   /*Defaults for sap.conf*/
   mSap_conf.GYRO_BIAS_RANDOM_WALK = 0;
   mSap_conf.SENSOR_ACCEL_BATCHES_PER_SEC = 2;
   mSap_conf.SENSOR_ACCEL_SAMPLES_PER_BATCH = 5;
   mSap_conf.SENSOR_GYRO_BATCHES_PER_SEC = 2;
   mSap_conf.SENSOR_GYRO_SAMPLES_PER_BATCH = 5;
   mSap_conf.SENSOR_ACCEL_BATCHES_PER_SEC_HIGH = 4;
   mSap_conf.SENSOR_ACCEL_SAMPLES_PER_BATCH_HIGH = 25;
   mSap_conf.SENSOR_GYRO_BATCHES_PER_SEC_HIGH = 4;
   mSap_conf.SENSOR_GYRO_SAMPLES_PER_BATCH_HIGH = 25;
   mSap_conf.SENSOR_CONTROL_MODE = 0; /* AUTO */
   mSap_conf.SENSOR_USAGE = 0; /* Enabled */
   mSap_conf.SENSOR_ALGORITHM_CONFIG_MASK = 0; /* INS Disabled = FALSE*/
   /* Values MUST be set by OEMs in configuration for sensor-assisted
      navigation to work. There are NO default values */
   mSap_conf.ACCEL_RANDOM_WALK_SPECTRAL_DENSITY = 0;
   mSap_conf.ANGLE_RANDOM_WALK_SPECTRAL_DENSITY = 0;
   mSap_conf.RATE_RANDOM_WALK_SPECTRAL_DENSITY = 0;
   mSap_conf.VELOCITY_RANDOM_WALK_SPECTRAL_DENSITY = 0;
   mSap_conf.GYRO_BIAS_RANDOM_WALK_VALID = 0;
   mSap_conf.ACCEL_RANDOM_WALK_SPECTRAL_DENSITY_VALID = 0;
   mSap_conf.ANGLE_RANDOM_WALK_SPECTRAL_DENSITY_VALID = 0;
   mSap_conf.RATE_RANDOM_WALK_SPECTRAL_DENSITY_VALID = 0;
   mSap_conf.VELOCITY_RANDOM_WALK_SPECTRAL_DENSITY_VALID = 0;
   /* default provider is SSC */
   mSap_conf.SENSOR_PROVIDER = 1;

   /* None of the 10 slots for agps certificates are writable by default */
   mGps_conf.AGPS_CERT_WRITABLE_MASK = 0;

   /* inject supl config to modem with config values from config.xml or gps.conf, default 1 */
   mGps_conf.AGPS_CONFIG_INJECT = 1;

   UTIL_READ_CONF(LOC_PATH_GPS_CONF, mGps_conf_table);
   UTIL_READ_CONF(LOC_PATH_SAP_CONF, mSap_conf_table);
}

uint32_t ContextBase::getCarrierCapabilities() {
    #define carrierMSA (uint32_t)0x2
    #define carrierMSB (uint32_t)0x1
    #define gpsConfMSA (uint32_t)0x4
    #define gpsConfMSB (uint32_t)0x2
    uint32_t capabilities = mGps_conf.CAPABILITIES;
    if ((mGps_conf.SUPL_MODE & carrierMSA) != carrierMSA) {
        capabilities &= ~gpsConfMSA;
    }
    if ((mGps_conf.SUPL_MODE & carrierMSB) != carrierMSB) {
        capabilities &= ~gpsConfMSB;
    }

    LOC_LOGV("getCarrierCapabilities: CAPABILITIES %x, SUPL_MODE %x, carrier capabilities %x",
             mGps_conf.CAPABILITIES, mGps_conf.SUPL_MODE, capabilities);
    return capabilities;
}

LBSProxyBase* ContextBase::getLBSProxy(const char* libName)
{
    LBSProxyBase* proxy = NULL;
    LOC_LOGD("%s:%d]: getLBSProxy libname: %s\n", __func__, __LINE__, libName);
    void* lib = dlopen(libName, RTLD_NOW);

    if ((void*)NULL != lib) {
        getLBSProxy_t* getter = (getLBSProxy_t*)dlsym(lib, "getLBSProxy");
        if (NULL != getter) {
            proxy = (*getter)();
        }
    }
    else
    {
        LOC_LOGW("%s:%d]: FAILED TO LOAD libname: %s\n", __func__, __LINE__, libName);
    }
    if (NULL == proxy) {
        proxy = new LBSProxyBase();
    }
    LOC_LOGD("%s:%d]: Exiting\n", __func__, __LINE__);
    return proxy;
}

LocApiBase* ContextBase::createLocApi(LOC_API_ADAPTER_EVENT_MASK_T exMask)
{
    LocApiBase* locApi = NULL;

    // Check the target
    if (TARGET_NO_GNSS != loc_get_target()){

        if (NULL == (locApi = mLBSProxy->getLocApi(mMsgTask, exMask, this))) {
            void *handle = NULL;
            //try to see if LocApiV02 is present
            if ((handle = dlopen("libloc_api_v02.so", RTLD_NOW)) != NULL) {
                LOC_LOGD("%s:%d]: libloc_api_v02.so is present", __func__, __LINE__);
                getLocApi_t* getter = (getLocApi_t*) dlsym(handle, "getLocApi");
                if (getter != NULL) {
                    LOC_LOGD("%s:%d]: getter is not NULL for LocApiV02", __func__,
                            __LINE__);
                    locApi = (*getter)(mMsgTask, exMask, this);
                }
            }
            // only RPC is the option now
            else {
                LOC_LOGD("%s:%d]: libloc_api_v02.so is NOT present. Trying RPC",
                        __func__, __LINE__);
                handle = dlopen("libloc_api-rpc-qc.so", RTLD_NOW);
                if (NULL != handle) {
                    getLocApi_t* getter = (getLocApi_t*) dlsym(handle, "getLocApi");
                    if (NULL != getter) {
                        LOC_LOGD("%s:%d]: getter is not NULL in RPC", __func__,
                                __LINE__);
                        locApi = (*getter)(mMsgTask, exMask, this);
                    }
                }
            }
        }
    }

    // locApi could still be NULL at this time
    // we would then create a dummy one
    if (NULL == locApi) {
        locApi = new LocApiBase(mMsgTask, exMask, this);
    }

    return locApi;
}

ContextBase::ContextBase(const MsgTask* msgTask,
                         LOC_API_ADAPTER_EVENT_MASK_T exMask,
                         const char* libName) :
    mLBSProxy(getLBSProxy(libName)),
    mMsgTask(msgTask),
    mLocApi(createLocApi(exMask)),
    mLocApiProxy(mLocApi->getLocApiProxy())
{
}

}
