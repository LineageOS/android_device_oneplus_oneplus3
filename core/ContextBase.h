/* Copyright (c) 2011-2017, The Linux Foundation. All rights reserved.
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
#ifndef __LOC_CONTEXT_BASE__
#define __LOC_CONTEXT_BASE__

#include <stdbool.h>
#include <ctype.h>
#include <MsgTask.h>
#include <LocApiBase.h>
#include <LBSProxyBase.h>
#include <loc_cfg.h>

/* GPS.conf support */
/* NOTE: the implementaiton of the parser casts number
   fields to 32 bit. To ensure all 'n' fields working,
   they must all be 32 bit fields. */
typedef struct loc_gps_cfg_s
{
    uint32_t       INTERMEDIATE_POS;
    uint32_t       ACCURACY_THRES;
    uint32_t       SUPL_VER;
    uint32_t       SUPL_MODE;
    uint32_t       SUPL_ES;
    uint32_t       CAPABILITIES;
    uint32_t       LPP_PROFILE;
    uint32_t       XTRA_VERSION_CHECK;
    char           XTRA_SERVER_1[LOC_MAX_PARAM_STRING];
    char           XTRA_SERVER_2[LOC_MAX_PARAM_STRING];
    char           XTRA_SERVER_3[LOC_MAX_PARAM_STRING];
    uint32_t       USE_EMERGENCY_PDN_FOR_EMERGENCY_SUPL;
    uint32_t       NMEA_PROVIDER;
    GnssConfigGpsLock   GPS_LOCK;
    uint32_t       A_GLONASS_POS_PROTOCOL_SELECT;
    uint32_t       AGPS_CERT_WRITABLE_MASK;
    uint32_t       AGPS_CONFIG_INJECT;
    uint32_t       LPPE_CP_TECHNOLOGY;
    uint32_t       LPPE_UP_TECHNOLOGY;
    uint32_t       EXTERNAL_DR_ENABLED;
    char           SUPL_HOST[LOC_MAX_PARAM_STRING];
    uint32_t       SUPL_PORT;
    uint32_t       MODEM_TYPE;
    char           MO_SUPL_HOST[LOC_MAX_PARAM_STRING];
    uint32_t       MO_SUPL_PORT;
    uint32_t       CONSTRAINED_TIME_UNCERTAINTY_ENABLED;
    double         CONSTRAINED_TIME_UNCERTAINTY_THRESHOLD;
    uint32_t       CONSTRAINED_TIME_UNCERTAINTY_ENERGY_BUDGET;
    uint32_t       POSITION_ASSISTED_CLOCK_ESTIMATOR_ENABLED;
    char           PROXY_APP_PACKAGE_NAME[LOC_MAX_PARAM_STRING];
    uint32_t       CP_MTLR_ES;
    uint32_t       GNSS_DEPLOYMENT;
    uint32_t       CUSTOM_NMEA_GGA_FIX_QUALITY_ENABLED;
} loc_gps_cfg_s_type;

/* NOTE: the implementaiton of the parser casts number
   fields to 32 bit. To ensure all 'n' fields working,
   they must all be 32 bit fields. */
/* Meanwhile, *_valid fields are 8 bit fields, and 'f'
   fields are double. Rigid as they are, it is the
   the status quo, until the parsing mechanism is
   change, that is. */
typedef struct
{
    uint8_t        GYRO_BIAS_RANDOM_WALK_VALID;
    double         GYRO_BIAS_RANDOM_WALK;
    uint32_t       SENSOR_ACCEL_BATCHES_PER_SEC;
    uint32_t       SENSOR_ACCEL_SAMPLES_PER_BATCH;
    uint32_t       SENSOR_GYRO_BATCHES_PER_SEC;
    uint32_t       SENSOR_GYRO_SAMPLES_PER_BATCH;
    uint32_t       SENSOR_ACCEL_BATCHES_PER_SEC_HIGH;
    uint32_t       SENSOR_ACCEL_SAMPLES_PER_BATCH_HIGH;
    uint32_t       SENSOR_GYRO_BATCHES_PER_SEC_HIGH;
    uint32_t       SENSOR_GYRO_SAMPLES_PER_BATCH_HIGH;
    uint32_t       SENSOR_CONTROL_MODE;
    uint32_t       SENSOR_ALGORITHM_CONFIG_MASK;
    uint8_t        ACCEL_RANDOM_WALK_SPECTRAL_DENSITY_VALID;
    double         ACCEL_RANDOM_WALK_SPECTRAL_DENSITY;
    uint8_t        ANGLE_RANDOM_WALK_SPECTRAL_DENSITY_VALID;
    double         ANGLE_RANDOM_WALK_SPECTRAL_DENSITY;
    uint8_t        RATE_RANDOM_WALK_SPECTRAL_DENSITY_VALID;
    double         RATE_RANDOM_WALK_SPECTRAL_DENSITY;
    uint8_t        VELOCITY_RANDOM_WALK_SPECTRAL_DENSITY_VALID;
    double         VELOCITY_RANDOM_WALK_SPECTRAL_DENSITY;
} loc_sap_cfg_s_type;

namespace loc_core {

class LocAdapterBase;

class ContextBase {
    static LBSProxyBase* getLBSProxy(const char* libName);
    LocApiBase* createLocApi(LOC_API_ADAPTER_EVENT_MASK_T excludedMask);
    static const loc_param_s_type mGps_conf_table[];
    static const loc_param_s_type mSap_conf_table[];
protected:
    const LBSProxyBase* mLBSProxy;
    const MsgTask* mMsgTask;
    LocApiBase* mLocApi;
    LocApiProxyBase *mLocApiProxy;

public:
    ContextBase(const MsgTask* msgTask,
                LOC_API_ADAPTER_EVENT_MASK_T exMask,
                const char* libName);
    inline virtual ~ContextBase() {
        if (nullptr != mLocApi) {
            mLocApi->destroy();
            mLocApi = nullptr;
        }
        if (nullptr != mLBSProxy) {
            delete mLBSProxy;
            mLBSProxy = nullptr;
        }
    }

    inline const MsgTask* getMsgTask() { return mMsgTask; }
    inline LocApiBase* getLocApi() { return mLocApi; }
    inline LocApiProxyBase* getLocApiProxy() { return mLocApiProxy; }
    inline bool hasAgpsExtendedCapabilities() { return mLBSProxy->hasAgpsExtendedCapabilities(); }
    inline bool hasNativeXtraClient() { return mLBSProxy->hasNativeXtraClient(); }
    inline void modemPowerVote(bool power) const { return mLBSProxy->modemPowerVote(power); }
    inline IzatDevId_t getIzatDevId() const {
        return mLBSProxy->getIzatDevId();
    }
    inline void sendMsg(const LocMsg *msg) { getMsgTask()->sendMsg(msg); }

    static loc_gps_cfg_s_type mGps_conf;
    static loc_sap_cfg_s_type mSap_conf;
    static bool sIsEngineCapabilitiesKnown;
    static uint64_t sSupportedMsgMask;
    static uint8_t sFeaturesSupported[MAX_FEATURE_LENGTH];
    static bool sGnssMeasurementSupported;

    void readConfig();
    static uint32_t getCarrierCapabilities();
    void setEngineCapabilities(uint64_t supportedMsgMask,
            uint8_t *featureList, bool gnssMeasurementSupported);

    static inline bool isEngineCapabilitiesKnown() {
        return sIsEngineCapabilitiesKnown;
    }

    static inline bool isMessageSupported(LocCheckingMessagesID msgID) {

        // confirm if msgID is not larger than the number of bits in
        // mSupportedMsg
        if ((uint64_t)msgID > (sizeof(sSupportedMsgMask) << 3)) {
            return false;
        } else {
            uint32_t messageChecker = 1 << msgID;
            return (messageChecker & sSupportedMsgMask) == messageChecker;
        }
    }

    /*
        Check if a feature is supported
    */
    static bool isFeatureSupported(uint8_t featureVal);

    /*
        Check if gnss measurement is supported
    */
    static bool gnssConstellationConfig();

};

struct LocApiResponse: LocMsg {
    private:
        ContextBase& mContext;
        std::function<void (LocationError err)> mProcImpl;
        inline virtual void proc() const {
            mProcImpl(mLocationError);
        }
    protected:
        LocationError mLocationError;
    public:
        inline LocApiResponse(ContextBase& context,
                              std::function<void (LocationError err)> procImpl ) :
                              mContext(context), mProcImpl(procImpl) {}

        void returnToSender(const LocationError err) {
            mLocationError = err;
            mContext.sendMsg(this);
        }
};

struct LocApiCollectiveResponse: LocMsg {
    private:
        ContextBase& mContext;
        std::function<void (std::vector<LocationError> errs)> mProcImpl;
        inline virtual void proc() const {
            mProcImpl(mLocationErrors);
        }
    protected:
        std::vector<LocationError> mLocationErrors;
    public:
        inline LocApiCollectiveResponse(ContextBase& context,
                              std::function<void (std::vector<LocationError> errs)> procImpl ) :
                              mContext(context), mProcImpl(procImpl) {}
        inline virtual ~LocApiCollectiveResponse() {
        }

        void returnToSender(std::vector<LocationError>& errs) {
            mLocationErrors = errs;
            mContext.sendMsg(this);
        }
};


template <typename DATA>
struct LocApiResponseData: LocMsg {
    private:
        ContextBase& mContext;
        std::function<void (LocationError err, DATA data)> mProcImpl;
        inline virtual void proc() const {
            mProcImpl(mLocationError, mData);
        }
    protected:
        LocationError mLocationError;
        DATA mData;
    public:
        inline LocApiResponseData(ContextBase& context,
                              std::function<void (LocationError err, DATA data)> procImpl ) :
                              mContext(context), mProcImpl(procImpl) {}

        void returnToSender(const LocationError err, const DATA data) {
            mLocationError = err;
            mData = data;
            mContext.sendMsg(this);
        }
};


} // namespace loc_core

#endif //__LOC_CONTEXT_BASE__
