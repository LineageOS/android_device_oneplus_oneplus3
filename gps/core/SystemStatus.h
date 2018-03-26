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

    SystemStatusItemBase() {
        timeval tv;
        gettimeofday(&tv, NULL);
        mUtcTime.tv_sec  = tv.tv_sec;
        mUtcTime.tv_nsec = tv.tv_usec *1000ULL;
        mUtcReported = mUtcTime;
    };
    virtual ~SystemStatusItemBase() { };
    virtual void dump(void) { };
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
        mLocationEx(locationEx) { }
    bool equals(SystemStatusLocation& peer);
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
    bool equals(SystemStatusTimeAndClock& peer);
    void dump(void);
};

class SystemStatusXoState : public SystemStatusItemBase
{
public:
    uint8_t  mXoState;
    inline SystemStatusXoState() :
        mXoState(0) {}
    inline SystemStatusXoState(const SystemStatusPQWM1& nmea);
    bool equals(SystemStatusXoState& peer);
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
    bool equals(SystemStatusRfAndParams& peer);
    void dump(void);
};

class SystemStatusErrRecovery : public SystemStatusItemBase
{
public:
    uint32_t mRecErrorRecovery;
    inline SystemStatusErrRecovery() :
        mRecErrorRecovery(0) {};
    inline SystemStatusErrRecovery(const SystemStatusPQWM1& nmea);
    bool equals(SystemStatusErrRecovery& peer);
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
    bool equals(SystemStatusInjectedPosition& peer);
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
    bool equals(SystemStatusBestPosition& peer);
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
    bool equals(SystemStatusXtra& peer);
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
    bool equals(SystemStatusEphemeris& peer);
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
    bool equals(SystemStatusSvHealth& peer);
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
    bool equals(SystemStatusPdr& peer);
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
    bool equals(SystemStatusNavData& peer);
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
    bool equals(SystemStatusPositionFailure& peer);
    void dump(void);
};

/******************************************************************************
 SystemStatus report data structure - from DataItem observer
******************************************************************************/
class SystemStatusGpsState : public SystemStatusItemBase, public IDataItemCore
{
public:
    inline SystemStatusGpsState() :
            mEnabled(false) {}
    inline SystemStatusGpsState(bool enabled) :
            mEnabled(enabled) {}

    bool mEnabled;

    inline bool equals(SystemStatusGpsState& peer) {
        return (mEnabled == peer.mEnabled);
    }
    inline void dump(void) {
        LOC_LOGD("GpsState: state=%u", mEnabled);
    }
    inline DataItemId getId() {
        return GPSSTATE_DATA_ITEM_ID;
    }
    inline void stringify(string& valueStr) {
        valueStr.clear();
        valueStr += "GpsState: enabled=";
        valueStr += to_string(mEnabled);
    }
    inline int32_t copy(IDataItemCore* src, bool* dataItemCopied = nullptr) {
        SystemStatusGpsState* gpsstate = static_cast<SystemStatusGpsState*>(src);
        mEnabled = gpsstate->mEnabled;
        if (dataItemCopied) {
            *dataItemCopied = true;
        }
        return 1;
    }
};

class SystemStatusNetworkInfo : public SystemStatusItemBase, public IDataItemCore
{
public:
    inline SystemStatusNetworkInfo() :
            mType(0),
            mTypeName(""),
            mSubTypeName(""),
            mAvailable(false),
            mConnected(false),
            mRoaming(false) {}
    inline SystemStatusNetworkInfo(
            uint32_t type,
            std::string typeName,
            std::string subTypeName,
            bool available,
            bool connected,
            bool roaming) :
            mType(type),
            mTypeName(typeName),
            mSubTypeName(subTypeName),
            mAvailable(available),
            mConnected(connected),
            mRoaming(roaming) {}

    uint32_t mType;
    std::string mTypeName;
    std::string mSubTypeName;
    bool mAvailable;
    bool mConnected;
    bool mRoaming;

    inline bool equals(SystemStatusNetworkInfo& peer) {
        if ((mType != peer.mType) ||
            (mTypeName != peer.mTypeName) ||
            (mSubTypeName != peer.mSubTypeName) ||
            (mAvailable != peer.mAvailable) ||
            (mConnected != peer.mConnected) ||
            (mRoaming != peer.mRoaming)) {
            return false;
        }
        return true;
    }
    inline void dump(void) {
        LOC_LOGD("NetworkInfo: type=%u connected=%u", mType, mConnected);
    }
    inline DataItemId getId() {
        return NETWORKINFO_DATA_ITEM_ID;
    }
    inline void stringify(string& /*valueStr*/) { }
    inline int32_t copy(IDataItemCore* src, bool* dataItemCopied = nullptr) {
        SystemStatusNetworkInfo* networkinfo = static_cast<SystemStatusNetworkInfo*>(src);
        mType = networkinfo->mType;
        mTypeName = networkinfo->mTypeName;
        mSubTypeName = networkinfo->mSubTypeName;
        mAvailable = networkinfo->mAvailable;
        mConnected = networkinfo->mConnected;
        mRoaming = networkinfo->mRoaming;
        if (dataItemCopied) {
            *dataItemCopied = true;
        }
        return 1;
    }
};

class SystemStatusTac : public SystemStatusItemBase, public IDataItemCore
{
public:
    inline SystemStatusTac() :
            mValue("") {}
    inline SystemStatusTac(std::string value) :
            mValue(value) {}

    std::string mValue;

    inline bool equals(SystemStatusTac& peer) {
        return (mValue == peer.mValue);
    }
    inline void dump(void) {
        LOC_LOGD("Tac: value=%s", mValue.c_str());
    }
    inline DataItemId getId() {
        return TAC_DATA_ITEM_ID;
    }
    inline void stringify(string& /*valueStr*/) { }
    inline int32_t copy(IDataItemCore* src, bool* dataItemCopied = nullptr) {
        SystemStatusTac* tac = static_cast<SystemStatusTac*>(src);
        mValue = tac->mValue;
        if (dataItemCopied) {
            *dataItemCopied = true;
        }
        return 1;
    }
};

class SystemStatusMccMnc : public SystemStatusItemBase, public IDataItemCore
{
public:
    inline SystemStatusMccMnc() :
            mValue("") {}
    inline SystemStatusMccMnc(std::string value) :
            mValue(value) {}

    std::string mValue;

    inline bool equals(SystemStatusMccMnc& peer) {
        return (mValue == peer.mValue);
    }
    inline void dump(void) {
        LOC_LOGD("TacMccMnc value=%s", mValue.c_str());
    }
    inline DataItemId getId() {
        return MCCMNC_DATA_ITEM_ID;
    }
    inline void stringify(string& /*valueStr*/) { }
    inline int32_t copy(IDataItemCore* src, bool* dataItemCopied = nullptr) {
        SystemStatusMccMnc* mccmnc = static_cast<SystemStatusMccMnc*>(src);
        mValue = mccmnc->mValue;
        if (dataItemCopied) {
            *dataItemCopied = true;
        }
        return 1;
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
    std::vector<SystemStatusGpsState>         mGpsState;
    std::vector<SystemStatusNetworkInfo>      mNetworkInfo;
    std::vector<SystemStatusTac>              mTac;
    std::vector<SystemStatusMccMnc>           mMccMnc;
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

    static const uint32_t                     maxLocation = 5;

    static const uint32_t                     maxTimeAndClock = 5;
    static const uint32_t                     maxXoState = 5;
    static const uint32_t                     maxRfAndParams = 5;
    static const uint32_t                     maxErrRecovery = 5;

    static const uint32_t                     maxInjectedPosition = 5;
    static const uint32_t                     maxBestPosition = 5;
    static const uint32_t                     maxXtra = 5;
    static const uint32_t                     maxEphemeris = 5;
    static const uint32_t                     maxSvHealth = 5;
    static const uint32_t                     maxPdr = 5;
    static const uint32_t                     maxNavData = 5;

    static const uint32_t                     maxPositionFailure = 5;

    static const uint32_t                     maxGpsState = 5;
    static const uint32_t                     maxNetworkInfo = 5;
    static const uint32_t                     maxTac = 5;
    static const uint32_t                     maxMccMnc = 5;

    SystemStatusReports mCache;
    bool mConnected;

    bool setLocation(const UlpLocation& location);

    bool setTimeAndCLock(const SystemStatusPQWM1& nmea);
    bool setXoState(const SystemStatusPQWM1& nmea);
    bool setRfAndParams(const SystemStatusPQWM1& nmea);
    bool setErrRecovery(const SystemStatusPQWM1& nmea);

    bool setInjectedPosition(const SystemStatusPQWP1& nmea);
    bool setBestPosition(const SystemStatusPQWP2& nmea);
    bool setXtra(const SystemStatusPQWP3& nmea);
    bool setEphemeris(const SystemStatusPQWP4& nmea);
    bool setSvHealth(const SystemStatusPQWP5& nmea);
    bool setPdr(const SystemStatusPQWP6& nmea);
    bool setNavData(const SystemStatusPQWP7& nmea);

    bool setPositionFailure(const SystemStatusPQWS1& nmea);

    bool setNetworkInfo(IDataItemCore* dataitem);

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
    bool eventConnectionStatus(bool connected, uint8_t type);
};

} // namespace loc_core

#endif //__SYSTEM_STATUS__

