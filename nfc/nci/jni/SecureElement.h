/*
 * Copyright (C) 2012 The Android Open Source Project
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
 *  Communicate with secure elements that are attached to the NFC
 *  controller.
 */
#pragma once
#include "SyncEvent.h"
#include "DataQueue.h"
#include "NfcJniUtil.h"
#include "RouteDataSet.h"
#include "IntervalTimer.h"
extern "C"
{
    #include "nfa_ee_api.h"
    #include "nfa_hci_api.h"
    #include "nfa_hci_defs.h"
    #include "nfa_ce_api.h"
    #include "phNxpExtns.h"
#if((NFC_NXP_ESE == TRUE)&&(NXP_EXTNS == TRUE))
    #include "phNfcTypes.h"
#endif
}
#if((NFC_NXP_ESE == TRUE)&&(NXP_EXTNS == TRUE))
#define SIG_NFC 44
#endif
typedef enum dual_mode{
 SPI_DWPCL_NOT_ACTIVE = 0x00,
 CL_ACTIVE = 0x01,
 SPI_ON = 0x02,
 SPI_DWPCL_BOTH_ACTIVE = 0x03,
}dual_mode_state;

typedef enum reset_management{
 TRANS_IDLE = 0x00,
 TRANS_WIRED_ONGOING = 0x01,
 TRANS_CL_ONGOING = 0x02,
 RESET_BLOCKED = 0x04,
}ese_reset_control;
typedef struct {
    tNFA_HANDLE src;
    tNFA_TECHNOLOGY_MASK tech_mask;
    bool reCfg;
}rd_swp_req_t;

typedef enum
{
    STATE_SE_RDR_MODE_INVALID =0x00,
    STATE_SE_RDR_MODE_START_CONFIG,
    STATE_SE_RDR_MODE_START_IN_PROGRESS,
    STATE_SE_RDR_MODE_STARTED,
    STATE_SE_RDR_MODE_ACTIVATED,
    STATE_SE_RDR_MODE_STOP_CONFIG,
    STATE_SE_RDR_MODE_STOP_IN_PROGRESS,
    STATE_SE_RDR_MODE_STOPPED,

}se_rd_req_state_t;

typedef enum
{   STATE_SE_RDR_FAILURE_NOT_SUPPORTED ,
    STATE_SE_RDR_FAILURE_NOT_ALLOWED

}se_rd_req_failures_t;


typedef struct{
    rd_swp_req_t swp_rd_req_info ;
    rd_swp_req_t swp_rd_req_current_info ;
    se_rd_req_state_t swp_rd_state;
    se_rd_req_failures_t swp_rd_req_fail_cause;
    Mutex mMutex;
}Rdr_req_ntf_info_t;

#if((NFC_NXP_ESE == TRUE)&&(NXP_EXTNS == TRUE))
typedef enum operation{
    STANDBY_TIMER_START,
    STANDBY_TIMER_STOP,
    STANDBY_TIMER_TIMEOUT,
    STANDBY_GPIO_HIGH,
    STANDBY_GPIO_LOW
}nfcc_standby_operation_t;
void spi_prio_signal_handler (int signum, siginfo_t *info, void *unused);
#endif

namespace android {
extern SyncEvent sNfaEnableDisablePollingEvent;
extern void startStopPolling (bool isStartPolling);

}  // namespace android

class SecureElement
{
public:
    tNFA_HANDLE  mActiveEeHandle;
#if(NXP_EXTNS == TRUE)
#define MAX_NFCEE 5

    struct mNfceeData{
        tNFA_HANDLE mNfceeHandle[MAX_NFCEE];
        tNFA_EE_STATUS mNfceeStatus[MAX_NFCEE];
        UINT8 mNfceePresent;
    };
    mNfceeData  mNfceeData_t;
#endif

#if(NFC_NXP_ESE == TRUE && NFC_NXP_CHIP_TYPE == PN548C2)
    IntervalTimer sSwpReaderTimer; // timer swp reader timeout.
#endif

    static const int MAX_NUM_EE = 5;    //max number of EE's

    /*******************************************************************************
    **
    ** Function:        getInstance
    **
    ** Description:     Get the SecureElement singleton object.
    **
    ** Returns:         SecureElement object.
    **
    *******************************************************************************/
    static SecureElement& getInstance ();


    /*******************************************************************************
    **
    ** Function:        initialize
    **
    ** Description:     Initialize all member variables.
    **                  native: Native data.
    **
    ** Returns:         True if ok.
    **
    *******************************************************************************/
    bool initialize (nfc_jni_native_data* native);


    /*******************************************************************************
    **
    ** Function:        finalize
    **
    ** Description:     Release all resources.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void finalize ();


    /*******************************************************************************
    **
    ** Function:        getListOfEeHandles
    **
    ** Description:     Get the list of handles of all execution environments.
    **                  e: Java Virtual Machine.
    **
    ** Returns:         List of handles of all execution environments.
    **
    *******************************************************************************/
    jintArray getListOfEeHandles (JNIEnv* e);

    /*******************************************************************************
    **
    ** Function:        getActiveSecureElementList
    **
    ** Description:     Get the list of handles of all execution environments.
    **                  e: Java Virtual Machine.
    **
    ** Returns:         List of handles of all execution environments.
    **
    *******************************************************************************/
    jintArray getActiveSecureElementList (JNIEnv* e);


    /*******************************************************************************
    **
    ** Function:        activate
    **
    ** Description:     Turn on the secure element.
    **                  seID: ID of secure element.
    **
    ** Returns:         True if ok.
    **
    *******************************************************************************/
    bool activate (jint seID);


    /*******************************************************************************
    **
    ** Function:        deactivate
    **
    ** Description:     Turn off the secure element.
    **                  seID: ID of secure element.
    **
    ** Returns:         True if ok.
    **
    *******************************************************************************/
    bool deactivate (jint seID);


    /*******************************************************************************
    **
    ** Function:        connectEE
    **
    ** Description:     Connect to the execution environment.
    **
    ** Returns:         True if ok.
    **
    *******************************************************************************/
    bool connectEE ();


    /*******************************************************************************
    **
    ** Function:        disconnectEE
    **
    ** Description:     Disconnect from the execution environment.
    **                  seID: ID of secure element.
    **
    ** Returns:         True if ok.
    **
    *******************************************************************************/
    bool disconnectEE (jint seID);


    /*******************************************************************************
    **
    ** Function:        transceive
    **
    ** Description:     Send data to the secure element; read it's response.
    **                  xmitBuffer: Data to transmit.
    **                  xmitBufferSize: Length of data.
    **                  recvBuffer: Buffer to receive response.
    **                  recvBufferMaxSize: Maximum size of buffer.
    **                  recvBufferActualSize: Actual length of response.
    **                  timeoutMillisec: timeout in millisecond
    **
    ** Returns:         True if ok.
    **
    *******************************************************************************/
    bool transceive (UINT8* xmitBuffer, INT32 xmitBufferSize, UINT8* recvBuffer,
                     INT32 recvBufferMaxSize, INT32& recvBufferActualSize, INT32 timeoutMillisec);

    void notifyModeSet (tNFA_HANDLE eeHandle, bool success, tNFA_EE_STATUS eeStatus);

    /*******************************************************************************
    **
    ** Function:        notifyListenModeState
    **
    ** Description:     Notify the NFC service about whether the SE was activated
    **                  in listen mode.
    **                  isActive: Whether the secure element is activated.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void notifyListenModeState (bool isActivated);

    /*******************************************************************************
    **
    ** Function:        notifyRfFieldEvent
    **
    ** Description:     Notify the NFC service about RF field events from the stack.
    **                  isActive: Whether any secure element is activated.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void notifyRfFieldEvent (bool isActive);

#if(NXP_EXTNS == TRUE)
    /*******************************************************************************
    **
    ** Function:        notifyEEReaderEvent
    **
    ** Description:     Notify the NFC service about Reader over SWP events from the stack.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void notifyEEReaderEvent (int evt, int data);

#endif

    /*******************************************************************************
    **
    ** Function:        resetRfFieldStatus ();
    **
    ** Description:     Resets the field status.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void resetRfFieldStatus ();

    /*******************************************************************************
    **
    ** Function:        storeUiccInfo
    **
    ** Description:     Store a copy of the execution environment information from the stack.
    **                  info: execution environment information.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void storeUiccInfo (tNFA_EE_DISCOVER_REQ& info);


    /*******************************************************************************
    **
    ** Function:        getUiccId
    **
    ** Description:     Get the ID of the secure element.
    **                  eeHandle: Handle to the secure element.
    **                  uid: Array to receive the ID.
    **
    ** Returns:         True if ok.
    **
    *******************************************************************************/
    bool getUiccId (tNFA_HANDLE eeHandle, jbyteArray& uid);

    /*******************************************************************************
    **
    ** Function:        notifyTransactionListenersOfAid
    **
    ** Description:     Notify the NFC service about a transaction event from secure element.
    **                  aid: Buffer contains application ID.
    **                  aidLen: Length of application ID.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void notifyTransactionListenersOfAid (const UINT8* aid, UINT8 aidLen, const UINT8* data, UINT32 dataLen,UINT32 evtSrc);

    /*******************************************************************************
    **
    ** Function:        notifyConnectivityListeners
    **
    ** Description:     Notify the NFC service about a connectivity event from secure element.
    **                  evtSrc: source of event UICC/eSE.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void notifyConnectivityListeners (UINT8 evtSrc);

    /*******************************************************************************
    **
    ** Function:        notifyEmvcoMultiCardDetectedListeners
    **
    ** Description:     Notify the NFC service about a multiple card presented to
    **                  Emvco reader.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void notifyEmvcoMultiCardDetectedListeners ();

    /*******************************************************************************
    **
    ** Function:        notifyTransactionListenersOfTlv
    **
    ** Description:     Notify the NFC service about a transaction event from secure element.
    **                  The type-length-value contains AID and parameter.
    **                  tlv: type-length-value encoded in Basic Encoding Rule.
    **                  tlvLen: Length tlv.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void notifyTransactionListenersOfTlv (const UINT8* tlv, UINT8 tlvLen);


    /*******************************************************************************
    **
    ** Function:        connectionEventHandler
    **
    ** Description:     Receive card-emulation related events from stack.
    **                  event: Event code.
    **                  eventData: Event data.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void connectionEventHandler (UINT8 event, tNFA_CONN_EVT_DATA* eventData);


    /*******************************************************************************
    **
    ** Function:        applyRoutes
    **
    ** Description:     Read route data from XML and apply them again
    **                  to every secure element.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void applyRoutes ();


    /*******************************************************************************
    **
    ** Function:        setActiveSeOverride
    **
    ** Description:     Specify which secure element to turn on.
    **                  activeSeOverride: ID of secure element
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void setActiveSeOverride (UINT8 activeSeOverride);

    bool SecEle_Modeset(UINT8 type);
    /*******************************************************************************
    **
    ** Function:        routeToSecureElement
    **
    ** Description:     Adjust controller's listen-mode routing table so transactions
    **                  are routed to the secure elements as specified in route.xml.
    **
    ** Returns:         True if ok.
    **
    *******************************************************************************/
    bool routeToSecureElement ();


    /*******************************************************************************
    **
    ** Function:        isBusy
    **
    ** Description:     Whether NFC controller is routing listen-mode events or a pipe is connected.
    **
    ** Returns:         True if either case is true.
    **
    *******************************************************************************/
    bool isBusy ();


    /*******************************************************************************
    **
    ** Function         getActualNumEe
    **
    ** Description      Returns number of secure elements we know about.
    **
    ** Returns          Number of secure elements we know about.
    **
    *******************************************************************************/
    UINT8 getActualNumEe();


    /*******************************************************************************
    **
    ** Function         getSeVerInfo
    **
    ** Description      Gets version information and id for a secure element.  The
    **                  seIndex parmeter is the zero based index of the secure
    **                  element to get verion info for.  The version infommation
    **                  is returned as a string int the verInfo parameter.
    **
    ** Returns          ture on success, false on failure
    **
    *******************************************************************************/
    bool getSeVerInfo(int seIndex, char * verInfo, int verInfoSz, UINT8 * seid);


    /*******************************************************************************
    **
    ** Function:        isActivatedInListenMode
    **
    ** Description:     Can be used to determine if the SE is activated in listen mode
    **
    ** Returns:         True if the SE is activated in listen mode
    **
    *******************************************************************************/
    bool isActivatedInListenMode();

    /*******************************************************************************
    **
    ** Function:        isRfFieldOn
    **
    ** Description:     Can be used to determine if the SE is in an RF field
    **
    ** Returns:         True if the SE is activated in an RF field
    **
    *******************************************************************************/
    bool isRfFieldOn();

    /*******************************************************************************
    **
    ** Function:        setEseListenTechMask
    **
    ** Description:     Can be used to force ESE to only listen the specific
    **                  Technologies.
    **                  NFA_TECHNOLOGY_MASK_A       0x01
    **                  NFA_TECHNOLOGY_MASK_B       0x02
    **
    ** Returns:         True if listening is configured.
    **
    *******************************************************************************/
    bool setEseListenTechMask(UINT8 tech_mask);

    bool sendEvent(UINT8 event);
    /*******************************************************************************
    **
    ** Function:        getAtr
    **
    ** Description:     Can be used to get the ATR response from connected eSE
    **
    ** Returns:         True if ATR response is returned successfully
    **
    *******************************************************************************/
    bool getAtr(jint seID, UINT8* recvBuffer, INT32 *recvBufferSize);
#if(NFC_NXP_ESE == TRUE && NFC_NXP_CHIP_TYPE == PN548C2)
    void etsiInitConfig();
    tNFC_STATUS etsiReaderConfig(int eeHandle);
    tNFC_STATUS etsiResetReaderConfig();
#endif
    jint getSETechnology(tNFA_HANDLE eeHandle);
    static const UINT8 UICC_ID = 0x02;
    static const UINT8 ESE_ID = 0x01;
    static const UINT8 DH_ID = 0x00;

    void getEeHandleList(tNFA_HANDLE *list, UINT8* count);

    tNFA_HANDLE getEseHandleFromGenericId(jint eseId);

    jint getGenericEseId(tNFA_HANDLE handle);

    tNFA_STATUS reconfigureEseHciInit();
#if((NFC_NXP_ESE == TRUE)&&(NXP_EXTNS == TRUE))
    void setCPTimeout();
    void NfccStandByOperation(nfcc_standby_operation_t value);
#endif
    bool isWiredModeAllowedInRfState();
    bool mRecvdTransEvt;
    bool mAllowWiredMode;

    SyncEvent       mRoutingEvent;
    SyncEvent       mAidAddRemoveEvent;
    SyncEvent       mUiccListenEvent;
    SyncEvent       mEseListenEvent;
    SyncEvent       mAllowWiredModeEvent;
    SyncEvent       mEeSetModeEvent;

#if(NXP_EXTNS == TRUE)
    tNFA_STATUS SecElem_EeModeSet(uint16_t handle, uint8_t mode);
    SyncEvent       mEEdatapacketEvent;
    static const UINT8 EVT_END_OF_APDU_TRANSFER = 0x21;    //NXP Propritory
    void setCLState(bool mState);
#endif

private:
    static const unsigned int MAX_RESPONSE_SIZE = 0x8800;//1024; //34K
    enum RouteSelection {NoRoute, DefaultRoute, SecElemRoute};
#ifdef GEMATO_SE_SUPPORT
    static const UINT8 STATIC_PIPE_0x70 = 0x19; //PN54X Gemalto's proprietary static pipe
#else
    static const UINT8 STATIC_PIPE_0x70 = 0x70; //Broadcom's proprietary static pipe
#endif
    static const UINT8 STATIC_PIPE_0x71 = 0x71; //Broadcom's proprietary static pipe
    static const UINT8 EVT_SEND_DATA = 0x10;    //see specification ETSI TS 102 622 v9.0.0 (Host Controller Interface); section 9.3.3.3
#if(NXP_EXTNS == TRUE)
    static const tNFA_HANDLE EE_HANDLE_0xF3 = 0x4C0;//0x401; //handle to secure element in slot 0
#ifdef NXP_UICC_ENABLE
    static const tNFA_HANDLE EE_HANDLE_0xF4 = 0x402; //handle to secure element in slot 1
#else
    static const tNFA_HANDLE EE_HANDLE_0xF4 = 0x0F4;//0x4C0; //handle to secure element in slot 1
#endif
#else
    static const tNFA_HANDLE EE_HANDLE_0xF3 = 0x4F3; //handle to secure element in slot 0
    static const tNFA_HANDLE EE_HANDLE_0xF4 = 0x4F4; //handle to secure element in slot 1
#endif

    static SecureElement sSecElem;
    static const char* APP_NAME;

    UINT8           mDestinationGate;       //destination gate of the UICC
    tNFA_HANDLE     mNfaHciHandle;          //NFA handle to NFA's HCI component
    nfc_jni_native_data* mNativeData;
    bool    mIsInit;                // whether EE is initialized
    UINT8   mActualNumEe;           // actual number of EE's reported by the stack
    UINT8   mNumEePresent;          // actual number of usable EE's
    bool    mbNewEE;
    UINT8   mNewPipeId;
    UINT8   mNewSourceGate;
    UINT16  mActiveSeOverride;      // active "enable" seid, 0 means activate all SEs
    tNFA_STATUS mCommandStatus;     //completion status of the last command
    bool    mIsPiping;              //is a pipe connected to the controller?
    RouteSelection mCurrentRouteSelection;
    int     mActualResponseSize;         //number of bytes in the response received from secure element
    int     mAtrInfolen;
    UINT8   mAtrStatus;
    bool    mUseOberthurWarmReset;  //whether to use warm-reset command
    bool    mActivatedInListenMode; // whether we're activated in listen mode
    UINT8   mOberthurWarmResetCommand; //warm-reset command byte
    tNFA_EE_INFO mEeInfo [MAX_NUM_EE];  //actual size stored in mActualNumEe
    tNFA_EE_DISCOVER_REQ mUiccInfo;
    tNFA_HCI_GET_GATE_PIPE_LIST mHciCfg;
    SyncEvent       mEeRegisterEvent;
    SyncEvent       mHciRegisterEvent;

    SyncEvent       mResetEvent;
    SyncEvent       mResetOngoingEvent;
    SyncEvent       mPipeListEvent;
    SyncEvent       mCreatePipeEvent;
    SyncEvent       mPipeOpenedEvent;
    SyncEvent       mAllocateGateEvent;
    SyncEvent       mDeallocateGateEvent;
//    SyncEvent       mRoutingEvent;
    SyncEvent       mUiccInfoEvent;
//    SyncEvent       mAidAddRemoveEvent;
    SyncEvent       mTransceiveEvent;
    SyncEvent       mGetRegisterEvent;
    SyncEvent       mVerInfoEvent;
    SyncEvent       mRegistryEvent;
    SyncEvent       mDiscMapEvent;
    UINT8           mVerInfo [3];
    UINT8           mAtrInfo[40];
    bool            mGetAtrRspwait;
    UINT8           mResponseData [MAX_RESPONSE_SIZE];
    RouteDataSet    mRouteDataSet; //routing data
    std::vector<std::string> mUsedAids; //AID's that are used in current routes
    UINT8           mAidForEmptySelect[NCI_MAX_AID_LEN+1];
    Mutex           mMutex; // protects fields below
    bool            mRfFieldIsOn; // last known RF field state
    struct timespec mLastRfFieldToggle; // last time RF field went off
    IntervalTimer   mTransceiveTimer;
    bool            mTransceiveWaitOk;
    int mWiredModeRfFiledEnable;
    /*******************************************************************************
    **
    ** Function:        SecureElement
    **
    ** Description:     Initialize member variables.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    SecureElement ();


    /*******************************************************************************
    **
    ** Function:        ~SecureElement
    **
    ** Description:     Release all resources.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    ~SecureElement ();


    /*******************************************************************************
    **
    ** Function:        nfaEeCallback
    **
    ** Description:     Receive execution environment-related events from stack.
    **                  event: Event code.
    **                  eventData: Event data.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    static void nfaEeCallback (tNFA_EE_EVT event, tNFA_EE_CBACK_DATA* eventData);


    /*******************************************************************************
    **
    ** Function:        nfaHciCallback
    **
    ** Description:     Receive Host Controller Interface-related events from stack.
    **                  event: Event code.
    **                  eventData: Event data.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    static void nfaHciCallback (tNFA_HCI_EVT event, tNFA_HCI_EVT_DATA* eventData);


    /*******************************************************************************
    **
    ** Function:        findEeByHandle
    **
    ** Description:     Find information about an execution environment.
    **                  eeHandle: Handle to execution environment.
    **
    ** Returns:         Information about an execution environment.
    **
    *******************************************************************************/
    tNFA_EE_INFO *findEeByHandle (tNFA_HANDLE eeHandle);


    /*******************************************************************************
    **
    ** Function:        findUiccByHandle
    **
    ** Description:     Find information about an execution environment.
    **                  eeHandle: Handle of the execution environment.
    **
    ** Returns:         Information about the execution environment.
    **
    *******************************************************************************/
    tNFA_EE_DISCOVER_INFO *findUiccByHandle (tNFA_HANDLE eeHandle);


    /*******************************************************************************
    **
    ** Function:        getDefaultEeHandle
    **
    ** Description:     Get the handle to the execution environment.
    **
    ** Returns:         Handle to the execution environment.
    **
    *******************************************************************************/
    tNFA_HANDLE getDefaultEeHandle ();


    /*******************************************************************************
    **
    ** Function:        adjustRoutes
    **
    ** Description:     Adjust routes in the controller's listen-mode routing table.
    **                  selection: which set of routes to configure the controller.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void adjustRoutes (RouteSelection selection);


    /*******************************************************************************
    **
    ** Function:        adjustProtocolRoutes
    **
    ** Description:     Adjust default routing based on protocol in NFC listen mode.
    **                  isRouteToEe: Whether routing to EE (true) or host (false).
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void adjustProtocolRoutes (RouteDataSet::Database* db, RouteSelection routeSelection);


    /*******************************************************************************
    **
    ** Function:        adjustTechnologyRoutes
    **
    ** Description:     Adjust default routing based on technology in NFC listen mode.
    **                  isRouteToEe: Whether routing to EE (true) or host (false).
    **
    ** Returns:         None
    **
    *******************************************************************************/
    void adjustTechnologyRoutes (RouteDataSet::Database* db, RouteSelection routeSelection);


    /*******************************************************************************
    **
    ** Function:        getEeInfo
    **
    ** Description:     Get latest information about execution environments from stack.
    **
    ** Returns:         True if at least 1 EE is available.
    **
    *******************************************************************************/
    bool getEeInfo ();

    /*******************************************************************************
    **
    ** Function:        eeStatusToString
    **
    ** Description:     Convert status code to status text.
    **                  status: Status code
    **
    ** Returns:         None
    **
    *******************************************************************************/
    static const char* eeStatusToString (UINT8 status);


    /*******************************************************************************
    **
    ** Function:        encodeAid
    **
    ** Description:     Encode AID in type-length-value using Basic Encoding Rule.
    **                  tlv: Buffer to store TLV.
    **                  tlvMaxLen: TLV buffer's maximum length.
    **                  tlvActualLen: TLV buffer's actual length.
    **                  aid: Buffer of Application ID.
    **                  aidLen: Aid buffer's actual length.
    **
    ** Returns:         True if ok.
    **
    *******************************************************************************/
    bool encodeAid (UINT8* tlv, UINT16 tlvMaxLen, UINT16& tlvActualLen, const UINT8* aid, UINT8 aidLen);

    static int decodeBerTlvLength(UINT8* data,int index, int data_length );

    static void discovery_map_cb (tNFC_DISCOVER_EVT event, tNFC_DISCOVER *p_data);


};
