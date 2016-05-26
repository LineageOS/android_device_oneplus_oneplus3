/*
 * Copyright (C) 2013 The Android Open Source Project
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
/******************************************************************************
 *
 *  The original Work has been changed by NXP Semiconductors.
 *
 *  Copyright (C) 2015 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
/*
 *  Manage the listen-mode routing table.
 */
#pragma once
#include "SyncEvent.h"
#include "NfcJniUtil.h"
#include "RouteDataSet.h"
#include "SecureElement.h"
#include <vector>
extern "C"
{
    #include "nfa_api.h"
    #include "nfa_ee_api.h"
}
#if(NXP_EXTNS == TRUE)
#define TECHNOLOGY_BASED_ROUTING        0x00
#define PROTOCOL_BASED_ROUTING          0x01
#define AID_BASED_ROUTING               0x02

/*Size of type and length Fields : No of bytes*/
#define TYPE_LENGTH_SIZE                0x02

#define MAX_GET_ROUTING_BUFFER_SIZE     740
#endif

//FelicaOnHost
typedef struct{

    UINT8 nfcid2Len;
    UINT8 sysCodeLen;
    UINT8 optParamLen;
    UINT16 nfcid2Handle;
    UINT8 sysCode[2];
    UINT8 nfcid2[8];
    UINT8* optParam;
    UINT8 InUse;

   // Mutex mMutex; /*add if it is required */
}NfcID2_info_t;

typedef struct{
    NfcID2_info_t NfcID2_info[4];
    IntervalTimer nfcID2_req_timer;
    UINT8 NfcID2_ReqCount;
    Mutex mMutex;
}NfcID2_add_req_info_t;

typedef struct{
    NfcID2_info_t NfcID2_info[4];
    IntervalTimer nfcID2_rmv_req_timer;
    UINT8 NfcID2_Rmv_ReqCount;
    Mutex mMutex;
}NfcID2_rmv_req_info_t;


class RoutingManager
{
public:
#if(NXP_EXTNS == TRUE)
    static const int NFA_SET_AID_ROUTING = 4;
    static const int NFA_SET_TECHNOLOGY_ROUTING = 1;
    static const int NFA_SET_PROTOCOL_ROUTING = 2;
    nfc_jni_native_data* mNativeData;
#endif

    static const int ROUTE_HOST = 0;
    static const int ROUTE_ESE = 1;

    static RoutingManager& getInstance ();
    bool initialize(nfc_jni_native_data* native);
    void enableRoutingToHost();
    void disableRoutingToHost();
#if(NXP_EXTNS == TRUE)
    bool setRoutingEntry(int type, int value, int route, int power);
    bool clearRoutingEntry(int type);
    void setRouting(bool);
    bool setDefaultRoute(const UINT8 defaultRoute, const UINT8 protoRoute, const UINT8 techRoute);
    bool clearAidTable ();
    void HandleAddNfcID2_Req();
    void HandleRmvNfcID2_Req();
    void setCeRouteStrictDisable(UINT32 state);
    bool is_ee_recovery_ongoing();
    se_rd_req_state_t getEtsiReaederState();
    Rdr_req_ntf_info_t getSwpRrdReqInfo();
    void setEtsiReaederState(se_rd_req_state_t newState);
    void setDefaultTechRouting (int seId, int tech_switchon,int tech_switchoff);
    void setDefaultProtoRouting (int seId, int proto_switchon,int proto_switchoff);
    int addNfcid2Routing(UINT8* nfcid2, UINT8 aidLen,const UINT8* syscode,
            int syscodelen,const UINT8* optparam, int optparamlen);
    bool removeNfcid2Routing(UINT8* nfcID2);

    void getRouting();
    void handleSERemovedNtf();
    void processGetRoutingRsp(tNFA_DM_CBACK_DATA* eventData, UINT8* sRoutingBuff);
    bool addAidRouting(const UINT8* aid, UINT8 aidLen, int route, int power, bool isprefix);
#else
    bool addAidRouting(const UINT8* aid, UINT8 aidLen, int route);
#endif
    bool removeAidRouting(const UINT8* aid, UINT8 aidLen);
    bool commitRouting();
    void onNfccShutdown();
    int registerJniFunctions (JNIEnv* e);
    void ee_removed_disc_ntf_handler(tNFA_HANDLE handle, tNFA_EE_STATUS status);
    SyncEvent mLmrtEvent;
    SyncEvent mCeRegisterEvent;//FelicaOnHost
    SyncEvent mCeDeRegisterEvent;
    Mutex  mResetHandlerMutex;
private:
    RoutingManager();
    ~RoutingManager();
    RoutingManager(const RoutingManager&);
    RoutingManager& operator=(const RoutingManager&);

    void cleanRouting();
    void handleData (const UINT8* data, UINT32 dataLen, tNFA_STATUS status);
    void notifyActivated ();
    void notifyDeactivated ();
    void notifyLmrtFull();

    // See AidRoutingManager.java for corresponding
    // AID_MATCHING_ constants

    // Every routing table entry is matched exact (BCM20793)
    static const int AID_MATCHING_EXACT_ONLY = 0x00;
    // Every routing table entry can be matched either exact or prefix
    static const int AID_MATCHING_EXACT_OR_PREFIX = 0x01;
    // Every routing table entry is matched as a prefix
    static const int AID_MATCHING_PREFIX_ONLY = 0x02;

    // See AidRoutingManager.java for corresponding
    // AID_MATCHING_ platform constants

    //Behavior as per Android-L, supporting prefix match and full
    //match for both OnHost and OffHost apps.
    static const int AID_MATCHING_L = 0x01;
    //Behavior as per Android-KitKat by NXP, supporting prefix match for
    //OffHost and prefix and full both for OnHost apps.
    static const int AID_MATCHING_K = 0x02;
    static void nfaEeCallback (tNFA_EE_EVT event, tNFA_EE_CBACK_DATA* eventData);
    static void stackCallback (UINT8 event, tNFA_CONN_EVT_DATA* eventData);
    static int com_android_nfc_cardemulation_doGetDefaultRouteDestination (JNIEnv* e);
    static int com_android_nfc_cardemulation_doGetDefaultOffHostRouteDestination (JNIEnv* e);
    static int com_android_nfc_cardemulation_doGetAidMatchingMode (JNIEnv* e);
    static int com_android_nfc_cardemulation_doGetAidMatchingPlatform (JNIEnv* e);

    std::vector<UINT8> mRxDataBuffer;

    // Fields below are final after initialize()
    //int mDefaultEe;
    int mOffHostEe;
    int mActiveSe;
    int mAidMatchingMode;
    int mAidMatchingPlatform;
    tNFA_TECHNOLOGY_MASK mSeTechMask;
    static const JNINativeMethod sMethods [];
    int mDefaultEe; //since this variable is used in both cases moved out of compiler switch
    int mHostListnEnable;
    int mFwdFuntnEnable;
    static int mChipId;
    SyncEvent mEeRegisterEvent;
    SyncEvent mRoutingEvent;
    SyncEvent mEeUpdateEvent;
    SyncEvent mEeSetModeEvent;
#if(NXP_EXTNS == TRUE)
     UINT32 mCeRouteStrictDisable;
     int defaultSeID ;
     bool mIsDirty;
     int defaultPowerstate;
     int defaultProtoSeID;
     int defaultProtoPowerstate;
     int defaultTechSeID;
     int defaultTechAPowerstate;
     int DefaultTechType;
     int mAddAid;
#endif
};
