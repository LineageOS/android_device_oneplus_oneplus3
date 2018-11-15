/* Copyright (c) 2017, The Linux Foundation. All rights reserved.
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
#ifndef __SYSTEM_STATUS__
#define __SYSTEM_STATUS__

#include <stdint.h>
#include <string>
#include <vector>
#include <platform_lib_log_util.h>
#include <MsgTask.h>
#include <IDataItemCore.h>
#include <IOsObserver.h>
#include <DataItemConcreteTypesBase.h>
#include <SystemStatusOsObserver.h>

#include <gps_extended_c.h>

#define GPS_MIN  (1)   //1-32
#define SBAS_MIN (33)
#define GLO_MIN  (65)  //65-88
#define QZSS_MIN (193) //193-197
#define BDS_MIN  (201) //201-237
#define GAL_MIN  (301) //301-336

#define GPS_NUM  (32)
#define SBAS_NUM (32)
#define GLO_NUM  (24)
#define QZSS_NUM (5)
#define BDS_NUM  (37)
#define GAL_NUM  (36)
#define SV_ALL_NUM  (GPS_NUM+GLO_NUM+QZSS_NUM+BDS_NUM+GAL_NUM) //=134

namespace loc_core
{

/******************************************************************************
 SystemStatus report data structure
******************************************************************************/
class SystemStatusItemBase
{
public:
    timespec mUtcTime;     // UTC timestamp when this info was last updated
    timespec mUtcReported; // UTC timestamp when this info was reported
    static const uint32_t maxItem = 5;

    SystemStatusItemBase() {
        timeval tv;
        gettimeofday(&tv, NULL);
        mUtcTime.tv_sec  = tv.tv_sec;
        mUtcTime.tv_nsec = tv.tv_usec *1000ULL;
        mUtcReported = mUtcTime;
    };
    virtual ~SystemStatusItemBase() {};
    inline virtual SystemStatusItemBase& collate(SystemStatusItemBase&) {
        return *this;
    }
    virtual void dump(void) {};
};

class SystemStatusLocation : public SystemStatusItemBase
{
public:
    bool mValid;
    UlpLocation mLocation;
    GpsLocationExtended mLocationEx;
    inline SystemStatusLocation() :
        mValid(false) {}
    inline SystemStatusLocation(const UlpLocation& location,
                         const GpsLocationExtended& locationEx) :
        mValid(true),
        mLocation(location),
        mLocationEx(locationEx) {}
    bool equals(const SystemStatusLocation& peer);
    void dump(void);
};

class SystemStatusPQWM1;
class SystemStatusTimeAndClock : public SystemStatusItemBase
{
public:
    uint16_t mGpsWeek;
    uint32_t mGpsTowMs;
    uint8_t  mTimeValid;
    uint8_t  mTimeSource;
    int32_t  mTimeUnc;
    int32_t  mClockFreqBias;
    int32_t  mClockFreqBiasUnc;
    int32_t  mLeapSeconds;
    int32_t  mLeapSecUnc;
    inline SystemStatusTimeAndClock() :
        mGpsWeek(0),
        mGpsTowMs(0),
        mTimeValid(0),
        mTimeSource(0),
        mTimeUnc(0),
        mClockFreqBias(0),
        mClockFreqBiasUnc(0),
        mLeapSeconds(0),
        mLeapSecUnc(0) {}
    inline SystemStatusTimeAndClock(const SystemStatusPQWM1& nmea);
    bool equals(const SystemStatusTimeAndClock& peer);
    void dump(void);
};

class SystemStatusXoState : public SystemStatusItemBase
{
public:
    uint8_t  mXoState;
    inline SystemStatusXoState() :
        mXoState(0) {}
    inline SystemStatusXoState(const SystemStatusPQWM1& nmea);
    bool equals(const SystemStatusXoState& peer);
    void dump(void);
};

class SystemStatusRfAndParams : public SystemStatusItemBase
{
public:
    int32_t  mPgaGain;
    uint32_t mGpsBpAmpI;
    uint32_t mGpsBpAmpQ;
    uint32_t mAdcI;
    uint32_t mAdcQ;
    uint32_t mJammerGps;
    uint32_t mJammerGlo;
    uint32_t mJammerBds;
    uint32_t mJammerGal;
    double   mAgcGps;
    double   mAgcGlo;
    double   mAgcBds;
    double   mAgcGal;
    inline SystemStatusRfAndParams() :
        mPgaGain(0),
        mGpsBpAmpI(0),
        mGpsBpAmpQ(0),
        mAdcI(0),
        mAdcQ(0),
        mJammerGps(0),
        mJammerGlo(0),
        mJammerBds(0),
        mJammerGal(0),
        mAgcGps(0),
        mAgcGlo(0),
        mAgcBds(0),
        mAgcGal(0) {}
    inline SystemStatusRfAndParams(const SystemStatusPQWM1& nmea);
    bool equals(const SystemStatusRfAndParams& peer);
    void dump(void);
};

class SystemStatusErrRecovery : public SystemStatusItemBase
{
public:
    uint32_t mRecErrorRecovery;
    inline SystemStatusErrRecovery() :
        mRecErrorRecovery(0) {};
    inline SystemStatusErrRecovery(const SystemStatusPQWM1& nmea);
    bool equals(const SystemStatusErrRecovery& peer);
    void dump(void);
};

class SystemStatusPQWP1;
class SystemStatusInjectedPosition : public SystemStatusItemBase
{
public:
    uint8_t  mEpiValidity;
    float    mEpiLat;
    float    mEpiLon;
    float    mEpiAlt;
    float    mEpiHepe;
    float    mEpiAltUnc;
    uint8_t  mEpiSrc;
    inline SystemStatusInjectedPosition() :
        mEpiValidity(0),
        mEpiLat(0),
        mEpiLon(0),
        mEpiAlt(0),
        mEpiHepe(0),
        mEpiAltUnc(0),
        mEpiSrc(0) {}
    inline SystemStatusInjectedPosition(const SystemStatusPQWP1& nmea);
    bool equals(const SystemStatusInjectedPosition& peer);
    void dump(void);
};

class SystemStatusPQWP2;
class SystemStatusBestPosition : public SystemStatusItemBase
{
public:
    bool     mValid;
    float    mBestLat;
    float    mBestLon;
    float    mBestAlt;
    float    mBestHepe;
    float    mBestAltUnc;
    inline SystemStatusBestPosition() :
        mValid(false),
        mBestLat(0),
        mBestLon(0),
        mBestAlt(0),
        mBestHepe(0),
        mBestAltUnc(0) {}
    inline SystemStatusBestPosition(const SystemStatusPQWP2& nmea);
    bool equals(const SystemStatusBestPosition& peer);
    void dump(void);
};

class SystemStatusPQWP3;
class SystemStatusXtra : public SystemStatusItemBase
{
public:
    uint8_t   mXtraValidMask;
    uint32_t  mGpsXtraAge;
    uint32_t  mGloXtraAge;
    uint32_t  mBdsXtraAge;
    uint32_t  mGalXtraAge;
    uint32_t  mQzssXtraAge;
    uint32_t  mGpsXtraValid;
    uint32_t  mGloXtraValid;
    uint64_t  mBdsXtraValid;
    uint64_t  mGalXtraValid;
    uint8_t   mQzssXtraValid;
    inline SystemStatusXtra() :
        mXtraValidMask(0),
        mGpsXtraAge(0),
        mGloXtraAge(0),
        mBdsXtraAge(0),
        mGalXtraAge(0),
        mQzssXtraAge(0),
        mGpsXtraValid(0),
        mGloXtraValid(0),
        mBdsXtraValid(0ULL),
        mGalXtraValid(0ULL),
        mQzssXtraValid(0) {}
    inline SystemStatusXtra(const SystemStatusPQWP3& nmea);
    bool equals(const SystemStatusXtra& peer);
    void dump(void);
};

class SystemStatusPQWP4;
class SystemStatusEphemeris : public SystemStatusItemBase
{
public:
    uint32_t  mGpsEpheValid;
    uint32_t  mGloEpheValid;
    uint64_t  mBdsEpheValid;
    uint64_t  mGalEpheValid;
    uint8_t   mQzssEpheValid;
    inline SystemStatusEphemeris() :
        mGpsEpheValid(0),
        mGloEpheValid(0),
        mBdsEpheValid(0ULL),
        mGalEpheValid(0ULL),
        mQzssEpheValid(0) {}
    inline SystemStatusEphemeris(const SystemStatusPQWP4& nmea);
    bool equals(const SystemStatusEphemeris& peer);
    void dump(void);
};

class SystemStatusPQWP5;
class SystemStatusSvHealth : public SystemStatusItemBase
{
public:
    uint32_t  mGpsUnknownMask;
    uint32_t  mGloUnknownMask;
    uint64_t  mBdsUnknownMask;
    uint64_t  mGalUnknownMask;
    uint8_t   mQzssUnknownMask;
    uint32_t  mGpsGoodMask;
    uint32_t  mGloGoodMask;
    uint64_t  mBdsGoodMask;
    uint64_t  mGalGoodMask;
    uint8_t   mQzssGoodMask;
    uint32_t  mGpsBadMask;
    uint32_t  mGloBadMask;
    uint64_t  mBdsBadMask;
    uint64_t  mGalBadMask;
    uint8_t   mQzssBadMask;
    inline SystemStatusSvHealth() :
        mGpsUnknownMask(0),
        mGloUnknownMask(0),
        mBdsUnknownMask(0ULL),
        mGalUnknownMask(0ULL),
        mQzssUnknownMask(0),
        mGpsGoodMask(0),
        mGloGoodMask(0),
        mBdsGoodMask(0ULL),
        mGalGoodMask(0ULL),
        mQzssGoodMask(0),
        mGpsBadMask(0),
        mGloBadMask(0),
        mBdsBadMask(0ULL),
        mGalBadMask(0ULL),
        mQzssBadMask(0) {}
    inline SystemStatusSvHealth(const SystemStatusPQWP5& nmea);
    bool equals(const SystemStatusSvHealth& peer);
    void dump(void);
};

class SystemStatusPQWP6;
class SystemStatusPdr : public SystemStatusItemBase
{
public:
    uint32_t  mFixInfoMask;
    inline SystemStatusPdr() :
        mFixInfoMask(0) {}
    inline SystemStatusPdr(const SystemStatusPQWP6& nmea);
    bool equals(const SystemStatusPdr& peer);
    void dump(void);
};

class SystemStatusPQWP7;
struct SystemStatusNav
{
    GnssEphemerisType   mType;
    GnssEphemerisSource mSource;
    int32_t             mAgeSec;
};

class SystemStatusNavData : public SystemStatusItemBase
{
public:
    SystemStatusNav mNav[SV_ALL_NUM];
    inline SystemStatusNavData() {
        for (uint32_t i=0; i<SV_ALL_NUM; i++) {
            mNav[i].mType = GNSS_EPH_TYPE_UNKNOWN;
            mNav[i].mSource = GNSS_EPH_SOURCE_UNKNOWN;
            mNav[i].mAgeSec = 0;
        }
    }
    inline SystemStatusNavData(const SystemStatusPQWP7& nmea);
    bool equals(const SystemStatusNavData& peer);
    void dump(void);
};

class SystemStatusPQWS1;
class SystemStatusPositionFailure : public SystemStatusItemBase
{
public:
    uint32_t  mFixInfoMask;
    uint32_t  mHepeLimit;
    inline SystemStatusPositionFailure() :
        mFixInfoMask(0),
        mHepeLimit(0) {}
    inline SystemStatusPositionFailure(const SystemStatusPQWS1& nmea);
    bool equals(const SystemStatusPositionFailure& peer);
    void dump(void);
};

/******************************************************************************
 SystemStatus report data structure - from DataItem observer
******************************************************************************/
class SystemStatusAirplaneMode : public SystemStatusItemBase,
        public AirplaneModeDataItemBase
{
public:
    inline SystemStatusAirplaneMode(bool mode=false) :
            AirplaneModeDataItemBase(mode) {}
    inline SystemStatusAirplaneMode(const AirplaneModeDataItemBase& itemBase) :
            AirplaneModeDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusAirplaneMode& peer) {
        return (mMode == peer.mMode);
    }
};

class SystemStatusENH : public SystemStatusItemBase,
        public ENHDataItemBase
{
public:
    inline SystemStatusENH(bool enabled=false) :
            ENHDataItemBase(enabled) {}
    inline SystemStatusENH(const ENHDataItemBase& itemBase) :
            ENHDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusENH& peer) {
        return (mEnabled == peer.mEnabled);
    }
};

class SystemStatusGpsState : public SystemStatusItemBase,
        public GPSStateDataItemBase
{
public:
    inline SystemStatusGpsState(bool enabled=false) :
            GPSStateDataItemBase(enabled) {}
    inline SystemStatusGpsState(const GPSStateDataItemBase& itemBase) :
            GPSStateDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusGpsState& peer) {
        return (mEnabled == peer.mEnabled);
    }
    inline void dump(void) override {
        LOC_LOGD("GpsState: state=%u", mEnabled);
    }
};

class SystemStatusNLPStatus : public SystemStatusItemBase,
        public NLPStatusDataItemBase
{
public:
    inline SystemStatusNLPStatus(bool enabled=false) :
            NLPStatusDataItemBase(enabled) {}
    inline SystemStatusNLPStatus(const NLPStatusDataItemBase& itemBase) :
            NLPStatusDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusNLPStatus& peer) {
        return (mEnabled == peer.mEnabled);
    }
};

class SystemStatusWifiHardwareState : public SystemStatusItemBase,
        public WifiHardwareStateDataItemBase
{
public:
    inline SystemStatusWifiHardwareState(bool enabled=false) :
            WifiHardwareStateDataItemBase(enabled) {}
    inline SystemStatusWifiHardwareState(const WifiHardwareStateDataItemBase& itemBase) :
            WifiHardwareStateDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusWifiHardwareState& peer) {
        return (mEnabled == peer.mEnabled);
    }
};

class SystemStatusNetworkInfo : public SystemStatusItemBase,
        public NetworkInfoDataItemBase
{
    NetworkInfoDataItemBase* mSrcObjPtr;
public:
    inline SystemStatusNetworkInfo(
            int32_t type=0,
            std::string typeName="",
            string subTypeName="",
            bool available=false,
            bool connected=false,
            bool roaming=false) :
            NetworkInfoDataItemBase(
                    (NetworkType)type,
                    type,
                    typeName,
                    subTypeName,
                    available,
                    connected,
                    roaming),
            mSrcObjPtr(nullptr) {}
    inline SystemStatusNetworkInfo(const NetworkInfoDataItemBase& itemBase) :
            NetworkInfoDataItemBase(itemBase),
            mSrcObjPtr((NetworkInfoDataItemBase*)&itemBase) {
        mType = itemBase.getType();
    }
    inline bool equals(const SystemStatusNetworkInfo& peer) {
        if ((mAllTypes == peer.mAllTypes) &&
            (mTypeName == peer.mTypeName) &&
            (mSubTypeName == peer.mSubTypeName) &&
            (mAvailable == peer.mAvailable) &&
            (mConnected == peer.mConnected) &&
            (mRoaming == peer.mRoaming)) {
            return true;
        }
        return false;
    }
    inline virtual SystemStatusItemBase& collate(SystemStatusItemBase& curInfo) {
        uint64_t allTypes = (static_cast<SystemStatusNetworkInfo&>(curInfo)).mAllTypes;
        if (mConnected) {
            mAllTypes |= allTypes;
        } else if (0 != mAllTypes) {
            mAllTypes = (allTypes & (~mAllTypes));
        } // else (mConnected == false && mAllTypes == 0)
          // we keep mAllTypes as 0, which means no more connections.

        if (nullptr != mSrcObjPtr) {
            // this is critical, changing mAllTypes of the original obj
            mSrcObjPtr->mAllTypes = mAllTypes;
        }
        return *this;
    }
    inline void dump(void) override {
        LOC_LOGD("NetworkInfo: mAllTypes=%" PRIx64 " connected=%u mType=%x",
                 mAllTypes, mConnected, mType);
    }
};

class SystemStatusServiceInfo : public SystemStatusItemBase,
        public RilServiceInfoDataItemBase
{
public:
    inline SystemStatusServiceInfo() :
            RilServiceInfoDataItemBase() {}
    inline SystemStatusServiceInfo(const RilServiceInfoDataItemBase& itemBase) :
            RilServiceInfoDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusServiceInfo& peer) {
        return static_cast<const RilServiceInfoDataItemBase&>(peer) ==
                static_cast<const RilServiceInfoDataItemBase&>(*this);
    }
};

class SystemStatusRilCellInfo : public SystemStatusItemBase,
        public RilCellInfoDataItemBase
{
public:
    inline SystemStatusRilCellInfo() :
            RilCellInfoDataItemBase() {}
    inline SystemStatusRilCellInfo(const RilCellInfoDataItemBase& itemBase) :
            RilCellInfoDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusRilCellInfo& peer) {
        return static_cast<const RilCellInfoDataItemBase&>(peer) ==
                static_cast<const RilCellInfoDataItemBase&>(*this);
    }
};

class SystemStatusServiceStatus : public SystemStatusItemBase,
        public ServiceStatusDataItemBase
{
public:
    inline SystemStatusServiceStatus(int32_t mServiceState=0) :
            ServiceStatusDataItemBase(mServiceState) {}
    inline SystemStatusServiceStatus(const ServiceStatusDataItemBase& itemBase) :
            ServiceStatusDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusServiceStatus& peer) {
        return (mServiceState == peer.mServiceState);
    }
};

class SystemStatusModel : public SystemStatusItemBase,
        public ModelDataItemBase
{
public:
    inline SystemStatusModel(string name="") :
            ModelDataItemBase(name) {}
    inline SystemStatusModel(const ModelDataItemBase& itemBase) :
            ModelDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusModel& peer) {
        return (mModel == peer.mModel);
        }
};

class SystemStatusManufacturer : public SystemStatusItemBase,
        public ManufacturerDataItemBase
{
public:
    inline SystemStatusManufacturer(string name="") :
            ManufacturerDataItemBase(name) {}
    inline SystemStatusManufacturer(const ManufacturerDataItemBase& itemBase) :
            ManufacturerDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusManufacturer& peer) {
        return (mManufacturer == peer.mManufacturer);
    }
};

class SystemStatusAssistedGps : public SystemStatusItemBase,
        public AssistedGpsDataItemBase
{
public:
    inline SystemStatusAssistedGps(bool enabled=false) :
            AssistedGpsDataItemBase(enabled) {}
    inline SystemStatusAssistedGps(const AssistedGpsDataItemBase& itemBase) :
            AssistedGpsDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusAssistedGps& peer) {
        return (mEnabled == peer.mEnabled);
    }
};

class SystemStatusScreenState : public SystemStatusItemBase,
        public ScreenStateDataItemBase
{
public:
    inline SystemStatusScreenState(bool state=false) :
            ScreenStateDataItemBase(state) {}
    inline SystemStatusScreenState(const ScreenStateDataItemBase& itemBase) :
            ScreenStateDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusScreenState& peer) {
        return (mState == peer.mState);
    }
};

class SystemStatusPowerConnectState : public SystemStatusItemBase,
        public PowerConnectStateDataItemBase
{
public:
    inline SystemStatusPowerConnectState(bool state=false) :
            PowerConnectStateDataItemBase(state) {}
    inline SystemStatusPowerConnectState(const PowerConnectStateDataItemBase& itemBase) :
            PowerConnectStateDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusPowerConnectState& peer) {
        return (mState == peer.mState);
    }
};

class SystemStatusTimeZoneChange : public SystemStatusItemBase,
        public TimeZoneChangeDataItemBase
{
public:
    inline SystemStatusTimeZoneChange(
            int64_t currTimeMillis=0ULL, int32_t rawOffset=0, int32_t dstOffset=0) :
            TimeZoneChangeDataItemBase(currTimeMillis, rawOffset, dstOffset) {}
    inline SystemStatusTimeZoneChange(const TimeZoneChangeDataItemBase& itemBase) :
            TimeZoneChangeDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusTimeZoneChange& peer) {
        return ((mCurrTimeMillis == peer.mCurrTimeMillis) &&
                (mRawOffsetTZ == peer.mRawOffsetTZ) &&
                (mDstOffsetTZ == peer.mDstOffsetTZ));
    }
};

class SystemStatusTimeChange : public SystemStatusItemBase,
        public TimeChangeDataItemBase
{
public:
    inline SystemStatusTimeChange(
            int64_t currTimeMillis=0ULL, int32_t rawOffset=0, int32_t dstOffset=0) :
            TimeChangeDataItemBase(currTimeMillis, rawOffset, dstOffset) {}
    inline SystemStatusTimeChange(const TimeChangeDataItemBase& itemBase) :
            TimeChangeDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusTimeChange& peer) {
        return ((mCurrTimeMillis == peer.mCurrTimeMillis) &&
                (mRawOffsetTZ == peer.mRawOffsetTZ) &&
                (mDstOffsetTZ == peer.mDstOffsetTZ));
    }
};

class SystemStatusWifiSupplicantStatus : public SystemStatusItemBase,
        public WifiSupplicantStatusDataItemBase
{
public:
    inline SystemStatusWifiSupplicantStatus() :
            WifiSupplicantStatusDataItemBase() {}
    inline SystemStatusWifiSupplicantStatus(const WifiSupplicantStatusDataItemBase& itemBase) :
            WifiSupplicantStatusDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusWifiSupplicantStatus& peer) {
        return ((mState == peer.mState) &&
                (mApMacAddressValid == peer.mApMacAddressValid) &&
                (mWifiApSsidValid == peer.mWifiApSsidValid) &&
                (mWifiApSsid == peer.mWifiApSsid));
        }
};

class SystemStatusShutdownState : public SystemStatusItemBase,
        public ShutdownStateDataItemBase
{
public:
    inline SystemStatusShutdownState(bool state=false) :
            ShutdownStateDataItemBase(state) {}
    inline SystemStatusShutdownState(const ShutdownStateDataItemBase& itemBase) :
            ShutdownStateDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusShutdownState& peer) {
        return (mState == peer.mState);
    }
};

class SystemStatusTac : public SystemStatusItemBase,
        public TacDataItemBase
{
public:
    inline SystemStatusTac(std::string value="") :
            TacDataItemBase(value) {}
    inline SystemStatusTac(const TacDataItemBase& itemBase) :
            TacDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusTac& peer) {
        return (mValue == peer.mValue);
    }
    inline void dump(void) {
        LOC_LOGD("Tac: value=%s", mValue.c_str());
    }
};

class SystemStatusMccMnc : public SystemStatusItemBase,
        public MccmncDataItemBase
{
public:
    inline SystemStatusMccMnc(std::string value="") :
            MccmncDataItemBase(value) {}
    inline SystemStatusMccMnc(const MccmncDataItemBase& itemBase) :
            MccmncDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusMccMnc& peer) {
        return (mValue == peer.mValue);
    }
    inline void dump(void) {
        LOC_LOGD("TacMccMnc value=%s", mValue.c_str());
    }
};

class SystemStatusBtDeviceScanDetail : public SystemStatusItemBase,
        public BtDeviceScanDetailsDataItemBase
{
public:
    inline SystemStatusBtDeviceScanDetail() :
            BtDeviceScanDetailsDataItemBase() {}
    inline SystemStatusBtDeviceScanDetail(const BtDeviceScanDetailsDataItemBase& itemBase) :
            BtDeviceScanDetailsDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusBtDeviceScanDetail& /*peer*/) {
        return true;
    }
};

class SystemStatusBtleDeviceScanDetail : public SystemStatusItemBase,
        public BtLeDeviceScanDetailsDataItemBase
{
public:
    inline SystemStatusBtleDeviceScanDetail() :
            BtLeDeviceScanDetailsDataItemBase() {}
    inline SystemStatusBtleDeviceScanDetail(const BtLeDeviceScanDetailsDataItemBase& itemBase) :
            BtLeDeviceScanDetailsDataItemBase(itemBase) {}
    inline bool equals(const SystemStatusBtleDeviceScanDetail& /*peer*/) {
        return true;
    }
};

/******************************************************************************
 SystemStatusReports
******************************************************************************/
class SystemStatusReports
{
public:
    // from QMI_LOC indication
    std::vector<SystemStatusLocation>         mLocation;

    // from ME debug NMEA
    std::vector<SystemStatusTimeAndClock>     mTimeAndClock;
    std::vector<SystemStatusXoState>          mXoState;
    std::vector<SystemStatusRfAndParams>      mRfAndParams;
    std::vector<SystemStatusErrRecovery>      mErrRecovery;

    // from PE debug NMEA
    std::vector<SystemStatusInjectedPosition> mInjectedPosition;
    std::vector<SystemStatusBestPosition>     mBestPosition;
    std::vector<SystemStatusXtra>             mXtra;
    std::vector<SystemStatusEphemeris>        mEphemeris;
    std::vector<SystemStatusSvHealth>         mSvHealth;
    std::vector<SystemStatusPdr>              mPdr;
    std::vector<SystemStatusNavData>          mNavData;

    // from SM debug NMEA
    std::vector<SystemStatusPositionFailure>  mPositionFailure;

    // from dataitems observer
    std::vector<SystemStatusAirplaneMode>     mAirplaneMode;
    std::vector<SystemStatusENH>              mENH;
    std::vector<SystemStatusGpsState>         mGPSState;
    std::vector<SystemStatusNLPStatus>        mNLPStatus;
    std::vector<SystemStatusWifiHardwareState> mWifiHardwareState;
    std::vector<SystemStatusNetworkInfo>      mNetworkInfo;
    std::vector<SystemStatusServiceInfo>      mRilServiceInfo;
    std::vector<SystemStatusRilCellInfo>      mRilCellInfo;
    std::vector<SystemStatusServiceStatus>    mServiceStatus;
    std::vector<SystemStatusModel>            mModel;
    std::vector<SystemStatusManufacturer>     mManufacturer;
    std::vector<SystemStatusAssistedGps>      mAssistedGps;
    std::vector<SystemStatusScreenState>      mScreenState;
    std::vector<SystemStatusPowerConnectState> mPowerConnectState;
    std::vector<SystemStatusTimeZoneChange>   mTimeZoneChange;
    std::vector<SystemStatusTimeChange>       mTimeChange;
    std::vector<SystemStatusWifiSupplicantStatus> mWifiSupplicantStatus;
    std::vector<SystemStatusShutdownState>    mShutdownState;
    std::vector<SystemStatusTac>              mTac;
    std::vector<SystemStatusMccMnc>           mMccMnc;
    std::vector<SystemStatusBtDeviceScanDetail> mBtDeviceScanDetail;
    std::vector<SystemStatusBtleDeviceScanDetail> mBtLeDeviceScanDetail;
};

/******************************************************************************
 SystemStatus
******************************************************************************/
class SystemStatus
{
private:
    static SystemStatus                       *mInstance;
    SystemStatusOsObserver                    mSysStatusObsvr;
    // ctor
    SystemStatus(const MsgTask* msgTask);
    // dtor
    inline ~SystemStatus() {}

    // Data members
    static pthread_mutex_t                    mMutexSystemStatus;
    SystemStatusReports mCache;

    template <typename TYPE_REPORT, typename TYPE_ITEM>
    bool setIteminReport(TYPE_REPORT& report, TYPE_ITEM&& s);

    // set default dataitem derived item in report cache
    template <typename TYPE_REPORT, typename TYPE_ITEM>
    void setDefaultIteminReport(TYPE_REPORT& report, const TYPE_ITEM& s);

    template <typename TYPE_REPORT, typename TYPE_ITEM>
    void getIteminReport(TYPE_REPORT& reportout, const TYPE_ITEM& c) const;

public:
    // Static methods
    static SystemStatus* getInstance(const MsgTask* msgTask);
    static void destroyInstance();
    IOsObserver* getOsObserver();

    // Helpers
    bool eventPosition(const UlpLocation& location,const GpsLocationExtended& locationEx);
    bool eventDataItemNotify(IDataItemCore* dataitem);
    bool setNmeaString(const char *data, uint32_t len);
    bool getReport(SystemStatusReports& reports, bool isLatestonly = false) const;
    bool setDefaultReport(void);
    bool eventConnectionStatus(bool connected, int8_t type);
};

} // namespace loc_core

#endif //__SYSTEM_STATUS__

