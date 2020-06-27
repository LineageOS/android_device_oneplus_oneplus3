/* Copyright (c) 2011-2014, 2016-2017 The Linux Foundation. All rights reserved.
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
#define LOG_NDEBUG 0 //Define to enable LOGV
#define LOG_TAG "LocSvc_LocApiBase"

#include <dlfcn.h>
#include <inttypes.h>
#include <LocApiBase.h>
#include <LocAdapterBase.h>
#include <log_util.h>
#include <LocDualContext.h>

namespace loc_core {

#define TO_ALL_LOCADAPTERS(call) TO_ALL_ADAPTERS(mLocAdapters, (call))
#define TO_1ST_HANDLING_LOCADAPTERS(call) TO_1ST_HANDLING_ADAPTER(mLocAdapters, (call))

int hexcode(char *hexstring, int string_size,
            const char *data, int data_size)
{
   int i;
   for (i = 0; i < data_size; i++)
   {
      char ch = data[i];
      if (i*2 + 3 <= string_size)
      {
         snprintf(&hexstring[i*2], 3, "%02X", ch);
      }
      else {
         break;
      }
   }
   return i;
}

int decodeAddress(char *addr_string, int string_size,
                   const char *data, int data_size)
{
    const char addr_prefix = 0x91;
    int i, idxOutput = 0;

    if (!data || !addr_string) { return 0; }

    if (data[0] != addr_prefix)
    {
        LOC_LOGW("decodeAddress: address prefix is not 0x%x but 0x%x", addr_prefix, data[0]);
        addr_string[0] = '\0';
        return 0; // prefix not correct
    }

    for (i = 1; i < data_size; i++)
    {
        unsigned char ch = data[i], low = ch & 0x0F, hi = ch >> 4;
        if (low <= 9 && idxOutput < string_size - 1) { addr_string[idxOutput++] = low + '0'; }
        if (hi <= 9 && idxOutput < string_size - 1) { addr_string[idxOutput++] = hi + '0'; }
    }

    addr_string[idxOutput] = '\0'; // Terminates the string

    return idxOutput;
}

struct LocSsrMsg : public LocMsg {
    LocApiBase* mLocApi;
    inline LocSsrMsg(LocApiBase* locApi) :
        LocMsg(), mLocApi(locApi)
    {
        locallog();
    }
    inline virtual void proc() const {
        mLocApi->close();
        mLocApi->open(mLocApi->getEvtMask());
    }
    inline void locallog() const {
        LOC_LOGV("LocSsrMsg");
    }
    inline virtual void log() const {
        locallog();
    }
};

struct LocOpenMsg : public LocMsg {
    LocApiBase* mLocApi;
    inline LocOpenMsg(LocApiBase* locApi) :
            LocMsg(), mLocApi(locApi)
    {
        locallog();
    }
    inline virtual void proc() const {
        mLocApi->open(mLocApi->getEvtMask());
    }
    inline void locallog() const {
        LOC_LOGv("LocOpen Mask: %" PRIx64 "\n", mLocApi->getEvtMask());
    }
    inline virtual void log() const {
        locallog();
    }
};

LocApiBase::LocApiBase(const MsgTask* msgTask,
                       LOC_API_ADAPTER_EVENT_MASK_T excludedMask,
                       ContextBase* context) :
    mMsgTask(msgTask), mContext(context), mSupportedMsg(0),
    mMask(0), mExcludedMask(excludedMask)
{
    memset(mLocAdapters, 0, sizeof(mLocAdapters));
    memset(mFeaturesSupported, 0, sizeof(mFeaturesSupported));
}

LOC_API_ADAPTER_EVENT_MASK_T LocApiBase::getEvtMask()
{
    LOC_API_ADAPTER_EVENT_MASK_T mask = 0;

    TO_ALL_LOCADAPTERS(mask |= mLocAdapters[i]->getEvtMask());

    return mask & ~mExcludedMask;
}

bool LocApiBase::isInSession()
{
    bool inSession = false;

    for (int i = 0;
         !inSession && i < MAX_ADAPTERS && NULL != mLocAdapters[i];
         i++) {
        inSession = mLocAdapters[i]->isInSession();
    }

    return inSession;
}

void LocApiBase::addAdapter(LocAdapterBase* adapter)
{
    for (int i = 0; i < MAX_ADAPTERS && mLocAdapters[i] != adapter; i++) {
        if (mLocAdapters[i] == NULL) {
            mLocAdapters[i] = adapter;
            mMsgTask->sendMsg(new LocOpenMsg(this));
            break;
        }
    }
}

void LocApiBase::removeAdapter(LocAdapterBase* adapter)
{
    for (int i = 0;
         i < MAX_ADAPTERS && NULL != mLocAdapters[i];
         i++) {
        if (mLocAdapters[i] == adapter) {
            mLocAdapters[i] = NULL;

            // shift the rest of the adapters up so that the pointers
            // in the array do not have holes.  This should be more
            // performant, because the array maintenance is much much
            // less frequent than event handlings, which need to linear
            // search all the adapters
            int j = i;
            while (++i < MAX_ADAPTERS && mLocAdapters[i] != NULL);

            // i would be MAX_ADAPTERS or point to a NULL
            i--;
            // i now should point to a none NULL adapter within valid
            // range although i could be equal to j, but it won't hurt.
            // No need to check it, as it gains nothing.
            mLocAdapters[j] = mLocAdapters[i];
            // this makes sure that we exit the for loop
            mLocAdapters[i] = NULL;

            // if we have an empty list of adapters
            if (0 == i) {
                close();
            } else {
                // else we need to remove the bit
                mMsgTask->sendMsg(new LocOpenMsg(this));
            }
        }
    }
}

void LocApiBase::updateEvtMask()
{
    open(getEvtMask());
}

void LocApiBase::handleEngineUpEvent()
{
    // This will take care of renegotiating the loc handle
    mMsgTask->sendMsg(new LocSsrMsg(this));

    LocDualContext::injectFeatureConfig(mContext);

    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->handleEngineUpEvent());
}

void LocApiBase::handleEngineDownEvent()
{
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->handleEngineDownEvent());
}

void LocApiBase::reportPosition(UlpLocation& location,
                                GpsLocationExtended& locationExtended,
                                enum loc_sess_status status,
                                LocPosTechMask loc_technology_mask)
{
    // print the location info before delivering
    LOC_LOGD("flags: %d\n  source: %d\n  latitude: %f\n  longitude: %f\n  "
             "altitude: %f\n  speed: %f\n  bearing: %f\n  accuracy: %f\n  "
             "timestamp: %" PRId64 "\n  rawDataSize: %d\n  rawData: %p\n  "
             "Session status: %d\n Technology mask: %u\n "
             "SV used in fix (gps/glo/bds/gal/qzss) : \
             (%" PRIx64 "/%" PRIx64 "/%" PRIx64 "/%" PRIx64 "/%" PRIx64 ")",
             location.gpsLocation.flags, location.position_source,
             location.gpsLocation.latitude, location.gpsLocation.longitude,
             location.gpsLocation.altitude, location.gpsLocation.speed,
             location.gpsLocation.bearing, location.gpsLocation.accuracy,
             location.gpsLocation.timestamp, location.rawDataSize,
             location.rawData, status, loc_technology_mask,
             locationExtended.gnss_sv_used_ids.gps_sv_used_ids_mask,
             locationExtended.gnss_sv_used_ids.glo_sv_used_ids_mask,
             locationExtended.gnss_sv_used_ids.bds_sv_used_ids_mask,
             locationExtended.gnss_sv_used_ids.gal_sv_used_ids_mask,
             locationExtended.gnss_sv_used_ids.qzss_sv_used_ids_mask);
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(
        mLocAdapters[i]->reportPositionEvent(location, locationExtended,
                                             status, loc_technology_mask)
    );
}

void LocApiBase::reportWwanZppFix(LocGpsLocation &zppLoc)
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->reportWwanZppFix(zppLoc));
}

void LocApiBase::reportOdcpiRequest(OdcpiRequestInfo& request)
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->reportOdcpiRequestEvent(request));
}

void LocApiBase::reportSv(GnssSvNotification& svNotify)
{
    const char* constellationString[] = { "Unknown", "GPS", "SBAS", "GLONASS",
        "QZSS", "BEIDOU", "GALILEO" };

    // print the SV info before delivering
    LOC_LOGV("num sv: %zu\n"
        "      sv: constellation svid         cN0"
        "    elevation    azimuth    flags",
        svNotify.count);
    for (size_t i = 0; i < svNotify.count && i < LOC_GNSS_MAX_SVS; i++) {
        if (svNotify.gnssSvs[i].type >
            sizeof(constellationString) / sizeof(constellationString[0]) - 1) {
            svNotify.gnssSvs[i].type = GNSS_SV_TYPE_UNKNOWN;
        }
        LOC_LOGV("   %03zu: %*s  %02d    %f    %f    %f   0x%02X",
            i,
            13,
            constellationString[svNotify.gnssSvs[i].type],
            svNotify.gnssSvs[i].svId,
            svNotify.gnssSvs[i].cN0Dbhz,
            svNotify.gnssSvs[i].elevation,
            svNotify.gnssSvs[i].azimuth,
            svNotify.gnssSvs[i].gnssSvOptionsMask);
    }
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(
        mLocAdapters[i]->reportSvEvent(svNotify)
        );
}

void LocApiBase::reportSvMeasurement(GnssSvMeasurementSet &svMeasurementSet)
{
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(
        mLocAdapters[i]->reportSvMeasurementEvent(svMeasurementSet)
    );
}

void LocApiBase::reportSvPolynomial(GnssSvPolynomial &svPolynomial)
{
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(
        mLocAdapters[i]->reportSvPolynomialEvent(svPolynomial)
    );
}

void LocApiBase::reportStatus(LocGpsStatusValue status)
{
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportStatus(status));
}

void LocApiBase::reportNmea(const char* nmea, int length)
{
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportNmeaEvent(nmea, length));
}

void LocApiBase::reportXtraServer(const char* url1, const char* url2,
                                  const char* url3, const int maxlength)
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->reportXtraServer(url1, url2, url3, maxlength));

}

void LocApiBase::requestXtraData()
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->requestXtraData());
}

void LocApiBase::requestTime()
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->requestTime());
}

void LocApiBase::requestLocation()
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->requestLocation());
}

void LocApiBase::requestATL(int connHandle, LocAGpsType agps_type)
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->requestATL(connHandle, agps_type));
}

void LocApiBase::releaseATL(int connHandle)
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->releaseATL(connHandle));
}

void LocApiBase::requestSuplES(int connHandle)
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->requestSuplES(connHandle));
}

void LocApiBase::reportDataCallOpened()
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->reportDataCallOpened());
}

void LocApiBase::reportDataCallClosed()
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->reportDataCallClosed());
}

void LocApiBase::requestNiNotify(GnssNiNotification &notify, const void* data)
{
    // loop through adapters, and deliver to the first handling adapter.
    TO_1ST_HANDLING_LOCADAPTERS(mLocAdapters[i]->requestNiNotifyEvent(notify, data));
}

void LocApiBase::saveSupportedMsgList(uint64_t supportedMsgList)
{
    mSupportedMsg = supportedMsgList;
}

void LocApiBase::saveSupportedFeatureList(uint8_t *featureList)
{
    memcpy((void *)mFeaturesSupported, (void *)featureList, sizeof(mFeaturesSupported));
}

void* LocApiBase :: getSibling()
    DEFAULT_IMPL(NULL)

LocApiProxyBase* LocApiBase :: getLocApiProxy()
    DEFAULT_IMPL(NULL)

void LocApiBase::reportGnssMeasurementData(GnssMeasurementsNotification& measurements,
                                           int msInWeek)
{
    // loop through adapters, and deliver to all adapters.
    TO_ALL_LOCADAPTERS(mLocAdapters[i]->reportGnssMeasurementDataEvent(measurements, msInWeek));
}

enum loc_api_adapter_err LocApiBase::
   open(LOC_API_ADAPTER_EVENT_MASK_T /*mask*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    close()
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    startFix(const LocPosMode& /*posMode*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    stopFix()
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

LocationError LocApiBase::
    deleteAidingData(const GnssAidingData& /*data*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    enableData(int /*enable*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    setAPN(char* /*apn*/, int /*len*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    injectPosition(double /*latitude*/, double /*longitude*/, float /*accuracy*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    injectPosition(const Location& /*location*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    setTime(LocGpsUtcTime /*time*/, int64_t /*timeReference*/, int /*uncertainty*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    setXtraData(char* /*data*/, int /*length*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    requestXtraServer()
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
   atlOpenStatus(int /*handle*/, int /*is_succ*/, char* /*apn*/,
                 AGpsBearerType /*bear*/, LocAGpsType /*agpsType*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    atlCloseStatus(int /*handle*/, int /*is_succ*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    setPositionMode(const LocPosMode& /*posMode*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

LocationError LocApiBase::
    setServer(const char* /*url*/, int /*len*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

LocationError LocApiBase::
    setServer(unsigned int /*ip*/, int /*port*/, LocServerType /*type*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

LocationError LocApiBase::
    informNiResponse(GnssNiResponse /*userResponse*/, const void* /*passThroughData*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

LocationError LocApiBase::
    setSUPLVersion(GnssConfigSuplVersion /*version*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    setNMEATypes (uint32_t /*typesMask*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

LocationError LocApiBase::
    setLPPConfig(GnssConfigLppProfile /*profile*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    setSensorControlConfig(int /*sensorUsage*/,
                           int /*sensorProvider*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    setSensorProperties(bool /*gyroBiasVarianceRandomWalk_valid*/,
                        float /*gyroBiasVarianceRandomWalk*/,
                        bool /*accelBiasVarianceRandomWalk_valid*/,
                        float /*accelBiasVarianceRandomWalk*/,
                        bool /*angleBiasVarianceRandomWalk_valid*/,
                        float /*angleBiasVarianceRandomWalk*/,
                        bool /*rateBiasVarianceRandomWalk_valid*/,
                        float /*rateBiasVarianceRandomWalk*/,
                        bool /*velocityBiasVarianceRandomWalk_valid*/,
                        float /*velocityBiasVarianceRandomWalk*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
    setSensorPerfControlConfig(int /*controlMode*/,
                               int /*accelSamplesPerBatch*/,
                               int /*accelBatchesPerSec*/,
                               int /*gyroSamplesPerBatch*/,
                               int /*gyroBatchesPerSec*/,
                               int /*accelSamplesPerBatchHigh*/,
                               int /*accelBatchesPerSecHigh*/,
                               int /*gyroSamplesPerBatchHigh*/,
                               int /*gyroBatchesPerSecHigh*/,
                               int /*algorithmConfig*/)
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

LocationError LocApiBase::
    setAGLONASSProtocol(GnssConfigAGlonassPositionProtocolMask /*aGlonassProtocol*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

LocationError LocApiBase::
    setLPPeProtocolCp(GnssConfigLppeControlPlaneMask /*lppeCP*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

LocationError LocApiBase::
    setLPPeProtocolUp(GnssConfigLppeUserPlaneMask /*lppeUP*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
   getWwanZppFix()
DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)

enum loc_api_adapter_err LocApiBase::
   getBestAvailableZppFix(LocGpsLocation& zppLoc)
{
   memset(&zppLoc, 0, sizeof(zppLoc));
   DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)
}

enum loc_api_adapter_err LocApiBase::
   getBestAvailableZppFix(LocGpsLocation & zppLoc, GpsLocationExtended & locationExtended,
           LocPosTechMask & tech_mask)
{
   memset(&zppLoc, 0, sizeof(zppLoc));
   memset(&tech_mask, 0, sizeof(tech_mask));
   memset(&locationExtended, 0, sizeof (locationExtended));
   DEFAULT_IMPL(LOC_API_ADAPTER_ERR_SUCCESS)
}

int LocApiBase::
    initDataServiceClient(bool /*isDueToSsr*/)
DEFAULT_IMPL(-1)

int LocApiBase::
    openAndStartDataCall()
DEFAULT_IMPL(-1)

void LocApiBase::
    stopDataCall()
DEFAULT_IMPL()

void LocApiBase::
    closeDataCall()
DEFAULT_IMPL()

void LocApiBase::
    releaseDataServiceClient()
DEFAULT_IMPL()

LocationError LocApiBase::
    setGpsLock(GnssConfigGpsLock /*lock*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

void LocApiBase::
    installAGpsCert(const LocDerEncodedCertificate* /*pData*/,
                    size_t /*length*/,
                    uint32_t /*slotBitMask*/)
DEFAULT_IMPL()

int LocApiBase::
    getGpsLock()
DEFAULT_IMPL(-1)

LocationError LocApiBase::
    setXtraVersionCheck(uint32_t /*check*/)
DEFAULT_IMPL(LOCATION_ERROR_SUCCESS)

bool LocApiBase::
    gnssConstellationConfig()
DEFAULT_IMPL(false)

bool LocApiBase::
    isFeatureSupported(uint8_t featureVal)
{
    uint8_t arrayIndex = featureVal >> 3;
    uint8_t bitPos = featureVal & 7;

    if (arrayIndex >= MAX_FEATURE_LENGTH) return false;
    return ((mFeaturesSupported[arrayIndex] >> bitPos ) & 0x1);
}

} // namespace loc_core
