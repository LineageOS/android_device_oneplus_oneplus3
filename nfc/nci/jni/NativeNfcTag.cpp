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

#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include "OverrideLog.h"
#include "NfcJniUtil.h"
#include "NfcTag.h"
#include "config.h"
#include "Mutex.h"
#include "IntervalTimer.h"
#include "JavaClassConstants.h"
#include "Pn544Interop.h"
#include <ScopedLocalRef.h>
#include <ScopedPrimitiveArray.h>
#include <string>

extern "C"
{
    #include "nfa_api.h"
    #include "nfa_rw_api.h"
    #include "ndef_utils.h"
    #include "rw_api.h"
    #include "phNxpExtns.h"
}
namespace android
{
    extern nfc_jni_native_data* getNative(JNIEnv *e, jobject o);
    extern bool nfcManager_isNfcActive();
    extern int gGeneralTransceiveTimeout;
    extern UINT16 getrfDiscoveryDuration();
}

extern bool         gActivated;
extern SyncEvent    gDeactivatedEvent;

/*****************************************************************************
**
** public variables and functions
**
*****************************************************************************/
namespace android
{
    bool    gIsTagDeactivating = false;    // flag for nfa callback indicating we are deactivating for RF interface switch
    bool    gIsSelectingRfInterface = false; // flag for nfa callback indicating we are selecting for RF interface switch
    bool    fNeedToSwitchBack = false;
    void    acquireRfInterfaceMutexLock();
    void    releaseRfInterfaceMutexLock();
}


/*****************************************************************************
**
** private variables and functions
**
*****************************************************************************/
namespace android
{


// Pre-defined tag type values. These must match the values in
// framework Ndef.java for Google public NFC API.
#define NDEF_UNKNOWN_TYPE          -1
#define NDEF_TYPE1_TAG             1
#define NDEF_TYPE2_TAG             2
#define NDEF_TYPE3_TAG             3
#define NDEF_TYPE4_TAG             4
#define NDEF_MIFARE_CLASSIC_TAG    101

/*Below #defines are made to make libnfc-nci as AOSP*/
#ifndef NCI_INTERFACE_MIFARE
#define NCI_INTERFACE_MIFARE       0x80
#endif
#undef  NCI_PROTOCOL_MIFARE
#define NCI_PROTOCOL_MIFARE        0x80


#define STATUS_CODE_TARGET_LOST    146  // this error code comes from the service

static uint32_t     sCheckNdefCurrentSize = 0;
static tNFA_STATUS  sCheckNdefStatus = 0; //whether tag already contains a NDEF message
static bool         sCheckNdefCapable = false; //whether tag has NDEF capability
static tNFA_HANDLE  sNdefTypeHandlerHandle = NFA_HANDLE_INVALID;
tNFA_INTF_TYPE   sCurrentRfInterface = NFA_INTERFACE_ISO_DEP;
static std::basic_string<UINT8> sRxDataBuffer;
static tNFA_STATUS  sRxDataStatus = NFA_STATUS_OK;
static bool         sWaitingForTransceive = false;
static bool         sTransceiveRfTimeout = false;
static Mutex        sRfInterfaceMutex;
static uint32_t     sReadDataLen = 0;
static tNFA_STATUS  sReadStatus;
static uint8_t*     sReadData = NULL;
static bool         sIsReadingNdefMessage = false;
static SyncEvent    sReadEvent;
static sem_t        sWriteSem;
static sem_t        sFormatSem;
static SyncEvent    sTransceiveEvent;
static SyncEvent    sReconnectEvent;
static sem_t        sCheckNdefSem;
static SyncEvent    sPresenceCheckEvent;
static sem_t        sMakeReadonlySem;
static IntervalTimer sSwitchBackTimer; // timer used to tell us to switch back to ISO_DEP frame interface
static IntervalTimer sPresenceCheckTimer; // timer used for presence cmd notification timeout.
static IntervalTimer sReconnectNtfTimer ;
static jboolean     sWriteOk = JNI_FALSE;
static jboolean     sWriteWaitingForComplete = JNI_FALSE;
static bool         sFormatOk = false;
static bool         sReadOnlyOk = false;
#if(NXP_EXTNS == TRUE && NFC_NXP_NON_STD_CARD == TRUE)
static bool         sNeedToSwitchRf = false;
#endif
static jboolean     sConnectOk = JNI_FALSE;
static jboolean     sConnectWaitingForComplete = JNI_FALSE;
static bool         sGotDeactivate = false;
static uint32_t     sCheckNdefMaxSize = 0;
static bool         sCheckNdefCardReadOnly = false;
static jboolean     sCheckNdefWaitingForComplete = JNI_FALSE;
static bool         sIsTagPresent = true;
static tNFA_STATUS  sMakeReadonlyStatus = NFA_STATUS_FAILED;
static jboolean     sMakeReadonlyWaitingForComplete = JNI_FALSE;
static int          sCurrentConnectedTargetType = TARGET_TYPE_UNKNOWN;
static int          sCurrentConnectedHandle;
static SyncEvent    sNfaVSCResponseEvent;
static SyncEvent    sNfaVSCNotificationEvent;
static bool         sIsTagInField;
static bool         sVSCRsp;
static bool         sReconnectFlag = false;
static int reSelect (tNFA_INTF_TYPE rfInterface, bool fSwitchIfNeeded);
static bool switchRfInterface(tNFA_INTF_TYPE rfInterface);
static bool setNdefDetectionTimeoutIfTagAbsent (JNIEnv *e, jobject o, tNFC_PROTOCOL protocol);
static void setNdefDetectionTimeout ();
static jboolean nativeNfcTag_doPresenceCheck (JNIEnv*, jobject);
#if(NXP_EXTNS == TRUE)
uint8_t key1[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t key2[6] = {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7};
bool isMifare = false;
static uint8_t Presence_check_TypeB[] =  {0xB2};
#if(NFC_NXP_NON_STD_CARD == TRUE)
static UINT16 NON_NCI_CARD_TIMER_OFFSET =700;
static IntervalTimer sNonNciCardDetectionTimer;
static IntervalTimer sNonNciMultiCardDetectionTimer;
struct sNonNciCard{
    bool chinaTransp_Card;
    bool Changan_Card;
    UINT8 sProtocolType;
    UINT8 srfInterfaceType;
    UINT32 uidlen;
    UINT8 uid[12];
} sNonNciCard_t;
bool scoreGenericNtf = false;
void nativeNfcTag_cacheNonNciCardDetection();
void nativeNfcTag_handleNonNciCardDetection(tNFA_CONN_EVT_DATA* eventData);
void nativeNfcTag_handleNonNciMultiCardDetection(UINT8 connEvent, tNFA_CONN_EVT_DATA* eventData);
static void nonNciCardTimerProc(union sigval);
static void nonNciMultiCardTimerProc(union sigval);
uint8_t checkTagNtf = 0;
uint8_t checkCmdSent = 0;
#endif
#endif
static bool sIsReconnecting = false;
static int doReconnectFlag = 0x00;
static bool sIsCheckingNDef = false;

static void nfaVSCCallback(UINT8 event, UINT16 param_len, UINT8 *p_param);
static void nfaVSCNtfCallback(UINT8 event, UINT16 param_len, UINT8 *p_param);
static void presenceCheckTimerProc (union sigval);
static void sReconnectTimerProc(union sigval);

static void nfaVSCNtfCallback(UINT8 event, UINT16 param_len, UINT8 *p_param)
{
    (void)event;
    ALOGD ("%s", __FUNCTION__);
    if(param_len == 4 && p_param[3] == 0x01)
    {
        sIsTagInField = true;
    }
    else
    {
        sIsTagInField = false;
    }

    ALOGD ("%s is Tag in Field = %d", __FUNCTION__, sIsTagInField);
    usleep(100*1000);
    SyncEventGuard guard (sNfaVSCNotificationEvent);
    sNfaVSCNotificationEvent.notifyOne ();
}

static void nfaVSCCallback(UINT8 event, UINT16 param_len, UINT8 *p_param)
{
    (void)event;
    ALOGD ("%s", __FUNCTION__);
    ALOGD ("%s param_len = %d ", __FUNCTION__, param_len);
    ALOGD ("%s p_param = %d ", __FUNCTION__, *p_param);

    if(param_len == 4 && p_param[3] == 0x00)
    {
        ALOGD ("%s sVSCRsp = true", __FUNCTION__);

        sVSCRsp = true;
    }
    else
    {
        ALOGD ("%s sVSCRsp = false", __FUNCTION__);

        sVSCRsp = false;
    }

    ALOGD ("%s sVSCRsp = %d", __FUNCTION__, sVSCRsp);


    SyncEventGuard guard (sNfaVSCResponseEvent);
    sNfaVSCResponseEvent.notifyOne ();
}

/*******************************************************************************
**
** Function:        nativeNfcTag_abortWaits
**
** Description:     Unblock all thread synchronization objects.
**
** Returns:         None
**
*******************************************************************************/
void nativeNfcTag_abortWaits ()
{
    ALOGD ("%s", __FUNCTION__);
    {
        SyncEventGuard g (sReadEvent);
        sReadEvent.notifyOne ();
    }
    sem_post (&sWriteSem);
    sem_post (&sFormatSem);
    {
        SyncEventGuard g (sTransceiveEvent);
        sTransceiveEvent.notifyOne ();
    }
    {
        SyncEventGuard g (sReconnectEvent);
        sReconnectEvent.notifyOne ();
    }

    sem_post (&sCheckNdefSem);
    {
        SyncEventGuard guard (sPresenceCheckEvent);
        sPresenceCheckEvent.notifyOne ();
    }
    sem_post (&sMakeReadonlySem);
    sCurrentRfInterface = NFA_INTERFACE_ISO_DEP;
    sCurrentConnectedTargetType = TARGET_TYPE_UNKNOWN;
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doReadCompleted
**
** Description:     Receive the completion status of read operation.  Called by
**                  NFA_READ_CPLT_EVT.
**                  status: Status of operation.
**
** Returns:         None
**
*******************************************************************************/
void nativeNfcTag_doReadCompleted (tNFA_STATUS status)
{
    ALOGD ("%s: status=0x%X; is reading=%u", __FUNCTION__, status, sIsReadingNdefMessage);

    if (sIsReadingNdefMessage == false)
        return; //not reading NDEF message right now, so just return

    sReadStatus = status;
    if (status != NFA_STATUS_OK)
    {
        sReadDataLen = 0;
        if (sReadData)
            free (sReadData);
        sReadData = NULL;
    }
    SyncEventGuard g (sReadEvent);
    sReadEvent.notifyOne ();
}


/*******************************************************************************
**
** Function:        ndefHandlerCallback
**
** Description:     Receive NDEF-message related events from stack.
**                  event: Event code.
**                  p_data: Event data.
**
** Returns:         None
**
*******************************************************************************/
static void ndefHandlerCallback (tNFA_NDEF_EVT event, tNFA_NDEF_EVT_DATA *eventData)
{
    ALOGD ("%s: event=%u, eventData=%p", __FUNCTION__, event, eventData);

    switch (event)
    {
    case NFA_NDEF_REGISTER_EVT:
        {
            tNFA_NDEF_REGISTER& ndef_reg = eventData->ndef_reg;
            ALOGD ("%s: NFA_NDEF_REGISTER_EVT; status=0x%X; h=0x%X", __FUNCTION__, ndef_reg.status, ndef_reg.ndef_type_handle);
            sNdefTypeHandlerHandle = ndef_reg.ndef_type_handle;
        }
        break;

    case NFA_NDEF_DATA_EVT:
        {
            ALOGD ("%s: NFA_NDEF_DATA_EVT; data_len = %lu", __FUNCTION__, eventData->ndef_data.len);
            sReadDataLen = eventData->ndef_data.len;
            sReadData = (uint8_t*) malloc (sReadDataLen);
            if(sReadData != NULL)
            memcpy (sReadData, eventData->ndef_data.p_data, eventData->ndef_data.len);
        }
        break;

    default:
        ALOGE ("%s: Unknown event %u ????", __FUNCTION__, event);
        break;
    }
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doRead
**
** Description:     Read the NDEF message on the tag.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         NDEF message.
**
*******************************************************************************/
static jbyteArray nativeNfcTag_doRead (JNIEnv* e, jobject o)
{
    ALOGD ("%s: enter", __FUNCTION__);
    tNFA_STATUS status = NFA_STATUS_FAILED;
    jbyteArray buf = NULL;
    int handle = sCurrentConnectedHandle;

    sReadStatus = NFA_STATUS_OK;
    sReadDataLen = 0;
    if (sReadData != NULL)
    {
        free (sReadData);
        sReadData = NULL;
    }

    if (sCheckNdefCurrentSize > 0)
    {
        {
            SyncEventGuard g (sReadEvent);
            sIsReadingNdefMessage = true;
            if (NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_MIFARE)
            {
                status = EXTNS_MfcReadNDef();
            }
            else
            {
                status = NFA_RwReadNDef ();
            }
            sReadEvent.wait (); //wait for NFA_READ_CPLT_EVT
        }
        sIsReadingNdefMessage = false;

        if (sReadDataLen > 0) //if stack actually read data from the tag
        {
            ALOGD ("%s: read %u bytes", __FUNCTION__, sReadDataLen);
            buf = e->NewByteArray (sReadDataLen);
            e->SetByteArrayRegion (buf, 0, sReadDataLen, (jbyte*) sReadData);
        }
        if (sReadStatus == NFA_STATUS_TIMEOUT)
            setNdefDetectionTimeout();
        else if (sReadStatus == NFA_STATUS_FAILED)
            (void)setNdefDetectionTimeoutIfTagAbsent(e, o, NFA_PROTOCOL_ISO15693);
    }
    else
    {
        ALOGD ("%s: create empty buffer", __FUNCTION__);
        sReadDataLen = 0;
        sReadData = (uint8_t*) malloc (1);
        buf = e->NewByteArray (sReadDataLen);
        e->SetByteArrayRegion (buf, 0, sReadDataLen, (jbyte*) sReadData);
    }

    if (sReadData)
    {
        free (sReadData);
        sReadData = NULL;
    }
    sReadDataLen = 0;

    ALOGD ("%s: exit", __FUNCTION__);
    return buf;
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doWriteStatus
**
** Description:     Receive the completion status of write operation.  Called
**                  by NFA_WRITE_CPLT_EVT.
**                  isWriteOk: Status of operation.
**
** Returns:         None
**
*******************************************************************************/
void nativeNfcTag_doWriteStatus (jboolean isWriteOk)
{
    if (sWriteWaitingForComplete != JNI_FALSE)
    {
        sWriteWaitingForComplete = JNI_FALSE;
        sWriteOk = isWriteOk;
        sem_post (&sWriteSem);
    }
}
#if(NXP_EXTNS == TRUE && NFC_NXP_NON_STD_CARD == TRUE)
/*******************************************************************************
**
** Function:        nonNciCardTimerProc
**
** Description:     CallBack timer for Non nci card detection.
**
**
**
** Returns:         None
**
*******************************************************************************/
void nonNciCardTimerProc(union sigval)
{
    ALOGD ("%s: enter ", __FUNCTION__);
    memset(&sNonNciCard_t,0,sizeof(sNonNciCard));
    scoreGenericNtf = false;
}

/*******************************************************************************
**
** Function:        nonNciMultiCardTimerProc
**
** Description:     CallBack timer for Non nci Multi card detection.
**
**
**
** Returns:         None
**
*******************************************************************************/
void nonNciMultiCardTimerProc(union sigval)
{
    ALOGD ("%s: enter ", __FUNCTION__);
    checkTagNtf = 0;
    checkCmdSent = 0;
}

/*******************************************************************************
**
** Function:        nativeNfcTag_cacheChinaBeijingCardDetection
**
** Description:     Store the  China Beijing Card detection parameters
**
** Returns:         None
**
*******************************************************************************/
void nativeNfcTag_cacheNonNciCardDetection()
{
    NfcTag& natTag = NfcTag::getInstance ();
    static UINT32 cardDetectTimeout = 0;
    static UINT8 *uid;
    scoreGenericNtf = true;
    NfcTag::getInstance().getTypeATagUID(&uid ,&sNonNciCard_t.uidlen);
    memcpy(sNonNciCard_t.uid ,uid ,sNonNciCard_t.uidlen);
    sNonNciCard_t.sProtocolType = natTag.mTechLibNfcTypes[sCurrentConnectedHandle];
    sNonNciCard_t.srfInterfaceType = sCurrentRfInterface;
    cardDetectTimeout =  NON_NCI_CARD_TIMER_OFFSET + android::getrfDiscoveryDuration();
    ALOGD ("%s: cardDetectTimeout = %d", __FUNCTION__,cardDetectTimeout);
    sNonNciCardDetectionTimer.set(cardDetectTimeout, nonNciCardTimerProc);
    ALOGD ("%s: sNonNciCard_t.sProtocolType=0x%x sNonNciCard_t.srfInterfaceType =0x%x ", __FUNCTION__,sNonNciCard_t.sProtocolType, sNonNciCard_t.srfInterfaceType);
}
/*******************************************************************************
**
** Function:        nativeNfcTag_handleChinaBeijingCardDetection
**
** Description:     China Beijing Card activation
**
** Returns:         None
**
*******************************************************************************/
void nativeNfcTag_handleNonNciCardDetection(tNFA_CONN_EVT_DATA* eventData)
{
    ALOGD ("%s: enter ", __FUNCTION__);
    sNonNciCardDetectionTimer.kill();
    static UINT32 tempUidLen = 0x00;
    static UINT8  *tempUid;
    NfcTag::getInstance().getTypeATagUID(&tempUid, &tempUidLen);
    if((eventData->activated.activate_ntf.intf_param.type == sNonNciCard_t.srfInterfaceType)  && ( eventData->activated.activate_ntf.protocol == sNonNciCard_t.sProtocolType))
    {
        if((tempUidLen == sNonNciCard_t.uidlen) && (memcmp(tempUid, sNonNciCard_t.uid, tempUidLen) == 0x00) )
        {
            sNonNciCard_t.chinaTransp_Card = true;
            ALOGD ("%s:  sNonNciCard_t.chinaTransp_Card = true", __FUNCTION__);
        }
    }
    else if((sNonNciCard_t.srfInterfaceType == NFC_INTERFACE_FRAME) && ( eventData->activated.activate_ntf.protocol == sNonNciCard_t.sProtocolType))
    {
        if((tempUidLen == sNonNciCard_t.uidlen) && (memcmp(tempUid, sNonNciCard_t.uid, tempUidLen) == 0x00) )
        {
            sNonNciCard_t.Changan_Card = true;
            ALOGD ("%s:   sNonNciCard_t.Changan_Card = true", __FUNCTION__);
        }
    }
    ALOGD ("%s: eventData->activated.activate_ntf.protocol =0x%x eventData->activated.activate_ntf.intf_param.type =0x%x", __FUNCTION__,eventData->activated.activate_ntf.protocol, eventData->activated.activate_ntf.intf_param.type);
}

/*******************************************************************************
**
** Function:        nativeNfcTag_handleChinaMultiCardDetection
**
** Description:     Multiprotocol Card activation
**
** Returns:         None
**
*******************************************************************************/
void nativeNfcTag_handleNonNciMultiCardDetection(UINT8 connEvent, tNFA_CONN_EVT_DATA* eventData)
{
    ALOGD ("%s: enter ", __FUNCTION__);
    if(NfcTag::getInstance ().mNumDiscNtf)
    {
        ALOGD("%s: check_tag_ntf = %d, check_cmd_sent = %d", __FUNCTION__ ,checkTagNtf,checkCmdSent);
        if(checkTagNtf == 0)
        {
            NfcTag::getInstance().connectionEventHandler (connEvent, eventData);
            NFA_Deactivate (TRUE);
            checkCmdSent = 1;
            sNonNciMultiCardDetectionTimer.set(NON_NCI_CARD_TIMER_OFFSET, nonNciCardTimerProc);
        }
        else if(checkTagNtf == 1)
        {
            NfcTag::getInstance ().mNumDiscNtf = 0;
            checkTagNtf = 0;
            checkCmdSent = 0;
            NfcTag::getInstance().connectionEventHandler (connEvent, eventData);
        }
    }
    else
    {
        NfcTag::getInstance().connectionEventHandler (connEvent, eventData);
    }
}
/*******************************************************************************
**
** Function:        switchBackTimerProc
**
** Description:     Callback function for interval timer.
**
** Returns:         None
**
*******************************************************************************/
static void switchBackTimerProc (union sigval)
{
    ALOGD ("%s", __FUNCTION__);
    switchRfInterface(NFA_INTERFACE_ISO_DEP);
}
#endif

/*******************************************************************************
**
** Function:        nativeNfcTag_formatStatus
**
** Description:     Receive the completion status of format operation.  Called
**                  by NFA_FORMAT_CPLT_EVT.
**                  isOk: Status of operation.
**
** Returns:         None
**
*******************************************************************************/
void nativeNfcTag_formatStatus (bool isOk)
{
    sFormatOk = isOk;
    sem_post (&sFormatSem);
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doWrite
**
** Description:     Write a NDEF message to the tag.
**                  e: JVM environment.
**                  o: Java object.
**                  buf: Contains a NDEF message.
**
** Returns:         True if ok.
**
*******************************************************************************/
static jboolean nativeNfcTag_doWrite (JNIEnv* e, jobject, jbyteArray buf)
{
    jboolean result = JNI_FALSE;
    tNFA_STATUS status = 0;
    const int maxBufferSize = 1024;
    UINT8 buffer[maxBufferSize] = { 0 };
    UINT32 curDataSize = 0;
    int handle = sCurrentConnectedHandle;

    ScopedByteArrayRO bytes(e, buf);
    UINT8* p_data = const_cast<UINT8*>(reinterpret_cast<const UINT8*>(&bytes[0])); // TODO: const-ness API bug in NFA_RwWriteNDef!

    ALOGD ("%s: enter; len = %zu", __FUNCTION__, bytes.size());

    /* Create the write semaphore */
    if (sem_init (&sWriteSem, 0, 0) == -1)
    {
        ALOGE ("%s: semaphore creation failed (errno=0x%08x)", __FUNCTION__, errno);
        return JNI_FALSE;
    }

    sWriteWaitingForComplete = JNI_TRUE;
    if (sCheckNdefStatus == NFA_STATUS_FAILED)
    {
        //if tag does not contain a NDEF message
        //and tag is capable of storing NDEF message
        if (sCheckNdefCapable)
        {
#if(NXP_EXTNS == TRUE)
            isMifare = false;
#endif
            ALOGD ("%s: try format", __FUNCTION__);
            sem_init (&sFormatSem, 0, 0);
            sFormatOk = false;
            if (NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_MIFARE)
            {
#if(NXP_EXTNS == TRUE)
                isMifare = true;
                status = EXTNS_MfcFormatTag(key1,sizeof(key1));
#endif
            }
            else
            {
                status = NFA_RwFormatTag ();
            }
            sem_wait (&sFormatSem);
            sem_destroy (&sFormatSem);

#if(NXP_EXTNS == TRUE)
            if(isMifare == true && sFormatOk != true)
            {
            sem_init (&sFormatSem, 0, 0);

            status = EXTNS_MfcFormatTag(key2,sizeof(key2));
                sem_wait (&sFormatSem);
                sem_destroy (&sFormatSem);
            }
#endif

            if (sFormatOk == false) //if format operation failed
                goto TheEnd;
        }
        ALOGD ("%s: try write", __FUNCTION__);
        status = NFA_RwWriteNDef (p_data, bytes.size());
    }
    else if (bytes.size() == 0)
    {
        //if (NXP TagWriter wants to erase tag) then create and write an empty ndef message
        NDEF_MsgInit (buffer, maxBufferSize, &curDataSize);
        status = NDEF_MsgAddRec (buffer, maxBufferSize, &curDataSize, NDEF_TNF_EMPTY, NULL, 0, NULL, 0, NULL, 0);
        ALOGD ("%s: create empty ndef msg; status=%u; size=%lu", __FUNCTION__, status, curDataSize);
        if (NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_MIFARE)
        {
            status = EXTNS_MfcWriteNDef(buffer, curDataSize);
        }
        else
        {
            status = NFA_RwWriteNDef (buffer, curDataSize);
        }
    }
    else
    {
        ALOGD ("%s: NFA_RwWriteNDef", __FUNCTION__);
        if (NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_MIFARE)
        {
            status = EXTNS_MfcWriteNDef(p_data, bytes.size());
        }
        else
        {
            status = NFA_RwWriteNDef (p_data, bytes.size());
        }
    }

    if (status != NFA_STATUS_OK)
    {
        ALOGE ("%s: write/format error=%d", __FUNCTION__, status);
        goto TheEnd;
    }

    /* Wait for write completion status */
    sWriteOk = false;
    if (sem_wait (&sWriteSem))
    {
        ALOGE ("%s: wait semaphore (errno=0x%08x)", __FUNCTION__, errno);
        goto TheEnd;
    }

    result = sWriteOk;

TheEnd:
    /* Destroy semaphore */
    if (sem_destroy (&sWriteSem))
    {
        ALOGE ("%s: failed destroy semaphore (errno=0x%08x)", __FUNCTION__, errno);
    }
    sWriteWaitingForComplete = JNI_FALSE;
    ALOGD ("%s: exit; result=%d", __FUNCTION__, result);
    return result;
}

/*******************************************************************************
**
** Function:        setNdefDetectionTimeoutIfTagAbsent
**
** Description:     Check protocol / presence of a tag which cannot detect tag lost during
**                  NDEF check. If it is absent, set NDEF detection timed out state.
**
** Returns:         True if a tag is absent and a current protocol matches the given protocols.
**
*******************************************************************************/
static bool setNdefDetectionTimeoutIfTagAbsent (JNIEnv *e, jobject o, tNFC_PROTOCOL protocol)
{
    if (!(NfcTag::getInstance().getProtocol() & protocol))
        return false;

    if (nativeNfcTag_doPresenceCheck(e, o))
        return false;

    ALOGD ("%s: tag is not present. set NDEF detection timed out", __FUNCTION__);
    setNdefDetectionTimeout();
    return true;
}

/*******************************************************************************
**
** Function:        setNdefDetectionTimeout
**
** Description:     Set the flag which indicates whether NDEF detection algorithm
**                  timed out so that the tag is regarded as lost.
**
** Returns:         None
**
*******************************************************************************/
static void setNdefDetectionTimeout ()
{
    tNFA_CONN_EVT_DATA conn_evt_data;

    conn_evt_data.status               = NFA_STATUS_TIMEOUT;
    conn_evt_data.ndef_detect.cur_size = 0;
    conn_evt_data.ndef_detect.max_size = 0;
    conn_evt_data.ndef_detect.flags    = RW_NDEF_FL_UNKNOWN;

    NfcTag::getInstance().connectionEventHandler(NFA_NDEF_DETECT_EVT, &conn_evt_data);
}

/*******************************************************************************
**
** Function:        nativeNfcTag_doConnectStatus
**
** Description:     Receive the completion status of connect operation.
**                  isConnectOk: Status of the operation.
**
** Returns:         None
**
*******************************************************************************/
void nativeNfcTag_doConnectStatus (jboolean isConnectOk)
{
    if (EXTNS_GetConnectFlag() == TRUE)
    {
       // int status = (isConnectOk == JNI_FALSE)?0xFF:0x00; /*commented to eliminate unused variable warning*/
        EXTNS_MfcActivated();
        EXTNS_SetConnectFlag (FALSE);
        return;
    }

    if (sConnectWaitingForComplete != JNI_FALSE)
    {
        sConnectWaitingForComplete = JNI_FALSE;
        sConnectOk = isConnectOk;
        SyncEventGuard g (sReconnectEvent);
        sReconnectEvent.notifyOne ();
    }
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doDeactivateStatus
**
** Description:     Receive the completion status of deactivate operation.
**
** Returns:         None
**
*******************************************************************************/
void nativeNfcTag_doDeactivateStatus (int status)
{
    if(EXTNS_GetDeactivateFlag() == TRUE)
    {
        EXTNS_MfcDisconnect();
        EXTNS_SetDeactivateFlag(FALSE);
        return;
    }

    sGotDeactivate = (status == 0);

    SyncEventGuard g (sReconnectEvent);
    sReconnectEvent.notifyOne ();
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doConnect
**
** Description:     Connect to the tag in RF field.
**                  e: JVM environment.
**                  o: Java object.
**                  targetHandle: Handle of the tag.
**
** Returns:         Must return NXP status code, which NFC service expects.
**
*******************************************************************************/
static jint nativeNfcTag_doConnect (JNIEnv*, jobject, jint targetHandle)
{
    ALOGD ("%s: targetHandle = %d", __FUNCTION__, targetHandle);
    int i = targetHandle;
    NfcTag& natTag = NfcTag::getInstance ();
    int retCode = NFCSTATUS_SUCCESS;

    if (i >= NfcTag::MAX_NUM_TECHNOLOGY)
    {
        ALOGE ("%s: Handle not found", __FUNCTION__);
        retCode = NFCSTATUS_FAILED;
        goto TheEnd;
    }
#if(NXP_EXTNS == TRUE && NFC_NXP_NON_STD_CARD == TRUE)
    sNeedToSwitchRf = false;
#endif
    if (natTag.getActivationState() != NfcTag::Active)
    {
        ALOGE ("%s: tag already deactivated", __FUNCTION__);
        retCode = NFCSTATUS_FAILED;
        goto TheEnd;
    }
#if(NXP_EXTNS == TRUE)
    sCurrentConnectedHandle = targetHandle;
    if(natTag.mTechLibNfcTypes[i] == NFC_PROTOCOL_T3BT)
    {
        goto TheEnd;
    }
#endif
    sCurrentConnectedTargetType = natTag.mTechList[i];
    if (natTag.mTechLibNfcTypes[i] != NFC_PROTOCOL_ISO_DEP)
    {
        ALOGD ("%s() Nfc type = %d, do nothing for non ISO_DEP", __FUNCTION__, natTag.mTechLibNfcTypes[i]);
        retCode = NFCSTATUS_SUCCESS;
        goto TheEnd;
    }
    /* Switching is required for CTS protocol paramter test case.*/
    if (natTag.mTechList[i] == TARGET_TYPE_ISO14443_3A || natTag.mTechList[i] == TARGET_TYPE_ISO14443_3B)
    {
        ALOGD ("%s: switching to tech: %d need to switch rf intf to frame", __FUNCTION__, natTag.mTechList[i]);
#if(NXP_EXTNS == TRUE && NFC_NXP_NON_STD_CARD == TRUE)
        if(sNonNciCard_t.Changan_Card == true)
            sNeedToSwitchRf = true;
        else
#endif
            retCode = switchRfInterface(NFA_INTERFACE_FRAME) ? NFA_STATUS_OK : NFA_STATUS_FAILED;
    }
    else
    {
        retCode = switchRfInterface(NFA_INTERFACE_ISO_DEP) ? NFA_STATUS_OK : NFA_STATUS_FAILED;
    }

TheEnd:
    ALOGD ("%s: exit 0x%X", __FUNCTION__, retCode);
    return retCode;
}
void setReconnectState(bool flag)
{
    sReconnectFlag = flag;
    ALOGE("setReconnectState = 0x%x",sReconnectFlag );
}
bool getReconnectState(void)
{
    ALOGE("getReconnectState = 0x%x",sReconnectFlag );
    return sReconnectFlag;
}
/*******************************************************************************
**
** Function:        reSelect
**
** Description:     Deactivates the tag and re-selects it with the specified
**                  rf interface.
**
** Returns:         status code, 0 on success, 1 on failure,
**                  146 (defined in service) on tag lost
**
*******************************************************************************/
static int reSelect (tNFA_INTF_TYPE rfInterface, bool fSwitchIfNeeded)
{
    int handle = sCurrentConnectedHandle;
    ALOGD ("%s: enter; rf intf = %d, current intf = %d", __FUNCTION__, rfInterface, sCurrentRfInterface);

    sRfInterfaceMutex.lock ();

    if (fSwitchIfNeeded && (rfInterface == sCurrentRfInterface))
    {
        // already in the requested interface
        sRfInterfaceMutex.unlock ();
        return 0;   // success
    }

    NfcTag& natTag = NfcTag::getInstance ();

    tNFA_STATUS status;
    int rVal = 1;
#if(NFC_NXP_NON_STD_CARD == TRUE)
    unsigned char retry_cnt = 1;
#endif
    do
    {
        //if tag has shutdown, abort this method
        if (NfcTag::getInstance ().isNdefDetectionTimedOut())
        {
            ALOGD ("%s: ndef detection timeout; break", __FUNCTION__);
            rVal = STATUS_CODE_TARGET_LOST;
            break;
        }
#if(NFC_NXP_NON_STD_CARD == TRUE)
        if(!retry_cnt && (natTag.mTechLibNfcTypes[handle] != NFA_PROTOCOL_MIFARE))
            NfcTag::getInstance ().mCashbeeDetected = true;
#endif
        {
            SyncEventGuard g (sReconnectEvent);
            gIsTagDeactivating = true;
            sGotDeactivate = false;
            setReconnectState(false);
            NFA_SetReconnectState(TRUE);
            if (NfcTag::getInstance ().isCashBeeActivated() == true || NfcTag::getInstance ().isEzLinkTagActivated() == true
#if(NXP_EXTNS == TRUE && NFC_NXP_NON_STD_CARD == TRUE)
            || sNonNciCard_t.chinaTransp_Card == true
#endif
            )
            {
                setReconnectState(true);
                /* send deactivate to Idle command */
                ALOGD ("%s: deactivate to Idle", __FUNCTION__);
                if (NFA_STATUS_OK != (status = NFA_StopRfDiscovery ())) //deactivate to sleep state
                {
                    ALOGE ("%s: deactivate failed, status = %d", __FUNCTION__, status);
                    break;
                }
            }
            else
            {
                ALOGD ("%s: deactivate to sleep", __FUNCTION__);
                if (NFA_STATUS_OK != (status = NFA_Deactivate (TRUE))) //deactivate to sleep state
                {
                    ALOGE ("%s: deactivate failed, status = %d", __FUNCTION__, status);
                    break;
                }
            }
            if (sReconnectEvent.wait (1000) == false) //if timeout occurred
            {
                ALOGE ("%s: timeout waiting for deactivate", __FUNCTION__);
            }
        }

        /* if (!sGotDeactivate)
        {
            rVal = STATUS_CODE_TARGET_LOST;
            break;
         }*/
        if(NfcTag::getInstance().getActivationState() == NfcTag::Idle)
        {
            ALOGD("%s:tag is in idle", __FUNCTION__);
            if((NfcTag::getInstance().mActivationParams_t.mTechLibNfcTypes == NFC_PROTOCOL_ISO_DEP))
            {
                if(NfcTag::getInstance().mActivationParams_t.mTechParams == NFC_DISCOVERY_TYPE_POLL_A)
                {
                    NfcTag::getInstance ().mCashbeeDetected = true;
                }
                else if(NfcTag::getInstance().mActivationParams_t.mTechParams == NFC_DISCOVERY_TYPE_POLL_B)
                {
                    NfcTag::getInstance ().mEzLinkTypeTag = true;
                }
            }
        }


        if (!(NfcTag::getInstance ().isCashBeeActivated() == true || NfcTag::getInstance ().isEzLinkTagActivated() == true
#if(NXP_EXTNS == TRUE && NFC_NXP_NON_STD_CARD == TRUE)
           || sNonNciCard_t.chinaTransp_Card == true
#endif
        ))
        {
            if (NfcTag::getInstance ().getActivationState () != NfcTag::Sleep)
            {
                ALOGD ("%s: tag is not in sleep", __FUNCTION__);
                rVal = STATUS_CODE_TARGET_LOST;
#if(NFC_NXP_NON_STD_CARD == TRUE)
                if(!retry_cnt)
#endif
                    break;
#if(NFC_NXP_NON_STD_CARD == TRUE)
                else continue;
#endif
            }
        }
        else
        {
            setReconnectState(false);
        }
        gIsTagDeactivating = false;

        {
            SyncEventGuard g2 (sReconnectEvent);
            gIsSelectingRfInterface = true;
            sConnectWaitingForComplete = JNI_TRUE;
            if (NfcTag::getInstance ().isCashBeeActivated() == true || NfcTag::getInstance ().isEzLinkTagActivated() == true
#if(NXP_EXTNS == TRUE && NFC_NXP_NON_STD_CARD == TRUE)
               || sNonNciCard_t.chinaTransp_Card == true
#endif
            )
            {
                setReconnectState(true);
                ALOGD ("%s: Discover map cmd", __FUNCTION__);
                if (NFA_STATUS_OK != (status = NFA_StartRfDiscovery ())) //deactivate to sleep state
                {
                    ALOGE ("%s: deactivate failed, status = %d", __FUNCTION__, status);
                    break;
                }
            }
            else
            {
                ALOGD ("%s: select interface %u", __FUNCTION__, rfInterface);

                if (NFA_STATUS_OK != (status = NFA_Select (natTag.mTechHandles[handle], natTag.mTechLibNfcTypes[handle], rfInterface)))
                {
                    ALOGE ("%s: NFA_Select failed, status = %d", __FUNCTION__, status);
                    break;
                }
            }
            sConnectOk = false;
            if (sReconnectEvent.wait (1000) == false) //if timeout occured
            {
                ALOGE ("%s: timeout waiting for select", __FUNCTION__);
#if(NXP_EXTNS == TRUE)
                if (!(NfcTag::getInstance ().isCashBeeActivated() == true || NfcTag::getInstance ().isEzLinkTagActivated() == true
        #if(NFC_NXP_NON_STD_CARD == TRUE)
                   || sNonNciCard_t.chinaTransp_Card == true
        #endif
                ))
                {
                    status = NFA_Deactivate (FALSE);
                    if (status != NFA_STATUS_OK)
                        ALOGE ("%s: deactivate failed; error=0x%X", __FUNCTION__, status);
                }
                break;
#endif
            }
        }

        ALOGD("%s: select completed; sConnectOk=%d", __FUNCTION__, sConnectOk);
        if (NfcTag::getInstance ().getActivationState () != NfcTag::Active)
        {
            ALOGD("%s: tag is not active", __FUNCTION__);
            rVal = STATUS_CODE_TARGET_LOST;
#if(NFC_NXP_NON_STD_CARD == TRUE)
            if(!retry_cnt)
#endif
            break;
        }
        if(NfcTag::getInstance ().isEzLinkTagActivated() == true)
        {
            NfcTag::getInstance ().mEzLinkTypeTag = false;
        }
#if(NFC_NXP_NON_STD_CARD == TRUE)
        if(NfcTag::getInstance ().isCashBeeActivated() == true)
        {
            NfcTag::getInstance ().mCashbeeDetected = false;
        }
#endif
        if (sConnectOk)
        {
            rVal = 0;   // success
            sCurrentRfInterface = rfInterface;
#if(NFC_NXP_NON_STD_CARD == TRUE)
            break;
#endif
        }
        else
        {
            rVal = 1;
        }
    }
#if(NFC_NXP_NON_STD_CARD == TRUE)
    while (retry_cnt--);
#else
    while(0);
#endif
    setReconnectState(false);
    NFA_SetReconnectState(FALSE);
    sConnectWaitingForComplete = JNI_FALSE;
    gIsTagDeactivating = false;
    gIsSelectingRfInterface = false;
    sRfInterfaceMutex.unlock ();
    ALOGD ("%s: exit; status=%d", __FUNCTION__, rVal);
    return rVal;
}

/*******************************************************************************
**
** Function:        switchRfInterface
**
** Description:     Switch controller's RF interface to frame, ISO-DEP, or NFC-DEP.
**                  rfInterface: Type of RF interface.
**
** Returns:         True if ok.
**
*******************************************************************************/
static bool switchRfInterface (tNFA_INTF_TYPE rfInterface)
{
    int handle = sCurrentConnectedHandle;
    ALOGD ("%s: rf intf = %d", __FUNCTION__, rfInterface);
    NfcTag& natTag = NfcTag::getInstance ();

    if (natTag.mTechLibNfcTypes[handle] != NFC_PROTOCOL_ISO_DEP)
    {
        ALOGD ("%s: protocol: %d not ISO_DEP, do nothing", __FUNCTION__, natTag.mTechLibNfcTypes[0]);
        return true;
    }

    ALOGD ("%s: new rf intf = %d, cur rf intf = %d", __FUNCTION__, rfInterface, sCurrentRfInterface);

    bool rVal = true;
    if (rfInterface != sCurrentRfInterface)
    {
        if (rVal = (0 == reSelect(rfInterface, true)))
        {
            sCurrentRfInterface = rfInterface;
        }
    }

    return rVal;
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doReconnect
**
** Description:     Re-connect to the tag in RF field.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         Status code.
**
*******************************************************************************/
static jint nativeNfcTag_doReconnect (JNIEnv*, jobject)
{
    ALOGD ("%s: enter", __FUNCTION__);
    int retCode = NFCSTATUS_SUCCESS;
    NfcTag& natTag = NfcTag::getInstance ();
    int handle = sCurrentConnectedHandle;

    UINT8* uid;
    UINT32 uid_len;
    ALOGD ("%s: enter; handle=%x", __FUNCTION__, handle);
    natTag.getTypeATagUID(&uid,&uid_len);

    if(natTag.mNfcDisableinProgress)
    {
        ALOGE ("%s: NFC disabling in progress", __FUNCTION__);
        retCode = NFCSTATUS_FAILED;
        goto TheEnd;
    }

    if (natTag.getActivationState() != NfcTag::Active)
    {
        ALOGE ("%s: tag already deactivated", __FUNCTION__);
        retCode = NFCSTATUS_FAILED;
        goto TheEnd;
    }

    // special case for Kovio
    if (NfcTag::getInstance ().mTechList [handle] == TARGET_TYPE_KOVIO_BARCODE)
    {
        ALOGD ("%s: fake out reconnect for Kovio", __FUNCTION__);
        goto TheEnd;
    }

    if (natTag.isNdefDetectionTimedOut())
    {
        ALOGD ("%s: ndef detection timeout", __FUNCTION__);
        retCode = STATUS_CODE_TARGET_LOST;
        goto TheEnd;
    }

    //special case for TypeB and TypeA random UID
    if ( (sCurrentRfInterface != NCI_INTERFACE_FRAME) &&
            ((natTag.mTechLibNfcTypes[handle] == NFA_PROTOCOL_ISO_DEP &&
            true == natTag.isTypeBTag() ) ||
            ( NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_ISO_DEP &&
                    uid_len > 0 && uid[0] == 0x08))
         )
    {
        ALOGD ("%s: reconnect for TypeB / TypeA random uid", __FUNCTION__);
        sReconnectNtfTimer.set(500, sReconnectTimerProc);

        tNFC_STATUS stat = NFA_RegVSCback (true,nfaVSCNtfCallback); //Register CallBack for VS NTF
        if(NFA_STATUS_OK != stat)
        {
            retCode = 0x01;
            goto TheEnd;
        }

        SyncEventGuard guard (sNfaVSCResponseEvent);
        stat = NFA_SendVsCommand (0x11,0x00,NULL,nfaVSCCallback);
        if(NFA_STATUS_OK == stat)
        {
            sIsReconnecting = true;
            ALOGD ("%s: reconnect for TypeB - wait for NFA VS command to finish", __FUNCTION__);
            sNfaVSCResponseEvent.wait(); //wait for NFA VS command to finish
            ALOGD ("%s: reconnect for TypeB - Got RSP", __FUNCTION__);
        }

        if(false == sVSCRsp)
        {
            retCode = 0x01;
            sIsReconnecting = false;
        }
        else
        {
            {
                ALOGD ("%s: reconnect for TypeB - wait for NFA VS NTF to come", __FUNCTION__);
                SyncEventGuard guard (sNfaVSCNotificationEvent);
                sNfaVSCNotificationEvent.wait(); //wait for NFA VS NTF to come
                ALOGD ("%s: reconnect for TypeB - GOT NFA VS NTF", __FUNCTION__);
                sReconnectNtfTimer.kill();
                sIsReconnecting = false;
            }

            if(false == sIsTagInField)
            {
                ALOGD ("%s: NxpNci: TAG OUT OF FIELD", __FUNCTION__);
                retCode = STATUS_CODE_TARGET_LOST;

                SyncEventGuard g (gDeactivatedEvent);

                //Tag not present, deactivate the TAG.
                stat = NFA_Deactivate (FALSE);
                if (stat == NFA_STATUS_OK)
                {
                    gDeactivatedEvent.wait ();
                }
                else
                {
                    ALOGE ("%s: deactivate failed; error=0x%X", __FUNCTION__, stat);
                }
            }

            else
            {
                retCode = 0x00;
            }
        }

        stat = NFA_RegVSCback (false,nfaVSCNtfCallback); //DeRegister CallBack for VS NTF
        if(NFA_STATUS_OK != stat)
        {
            retCode = 0x01;
        }
        ALOGD ("%s: reconnect for TypeB - return", __FUNCTION__);

        goto TheEnd;
    }
     // this is only supported for type 2 or 4 (ISO_DEP) tags
    if (natTag.mTechLibNfcTypes[handle] == NFA_PROTOCOL_ISO_DEP)
        retCode = reSelect(NFA_INTERFACE_ISO_DEP, false);
    else if (natTag.mTechLibNfcTypes[handle] == NFA_PROTOCOL_T2T)
        retCode = reSelect(NFA_INTERFACE_FRAME, false);
    else if (natTag.mTechLibNfcTypes[handle] == NFA_PROTOCOL_MIFARE)
        retCode = reSelect(NFA_INTERFACE_MIFARE, false);

TheEnd:
    ALOGD ("%s: exit 0x%X", __FUNCTION__, retCode);
    return retCode;
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doHandleReconnect
**
** Description:     Re-connect to the tag in RF field.
**                  e: JVM environment.
**                  o: Java object.
**                  targetHandle: Handle of the tag.
**
** Returns:         Status code.
**
*******************************************************************************/
static jint nativeNfcTag_doHandleReconnect (JNIEnv *e, jobject o, jint targetHandle)
{
    ALOGD ("%s: targetHandle = %d", __FUNCTION__, targetHandle);
    if(NfcTag::getInstance ().mNfcDisableinProgress)
        return STATUS_CODE_TARGET_LOST;
    return nativeNfcTag_doConnect (e, o, targetHandle);
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doDisconnect
**
** Description:     Deactivate the RF field.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         True if ok.
**
*******************************************************************************/
static jboolean nativeNfcTag_doDisconnect (JNIEnv*, jobject)
{
    ALOGD ("%s: enter", __FUNCTION__);
    tNFA_STATUS nfaStat = NFA_STATUS_OK;

    NfcTag::getInstance().resetAllTransceiveTimeouts ();
#if(NXP_EXTNS == TRUE && NFC_NXP_NON_STD_CARD == TRUE)
    if(sNonNciCard_t.Changan_Card == true || sNonNciCard_t.chinaTransp_Card == true)
    {
        memset(&sNonNciCard_t,0,sizeof(sNonNciCard));
        scoreGenericNtf = false;
    }
#endif
    if (NfcTag::getInstance ().getActivationState () != NfcTag::Active)
    {
        ALOGE ("%s: tag already deactivated", __FUNCTION__);
        goto TheEnd;
    }

    nfaStat = NFA_Deactivate (FALSE);
    if (nfaStat != NFA_STATUS_OK)
        ALOGE ("%s: deactivate failed; error=0x%X", __FUNCTION__, nfaStat);

TheEnd:
    ALOGD ("%s: exit", __FUNCTION__);
    return (nfaStat == NFA_STATUS_OK) ? JNI_TRUE : JNI_FALSE;
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doTransceiveStatus
**
** Description:     Receive the completion status of transceive operation.
**                  status: operation status.
**                  buf: Contains tag's response.
**                  bufLen: Length of buffer.
**
** Returns:         None
**
*******************************************************************************/
void nativeNfcTag_doTransceiveStatus (tNFA_STATUS status, uint8_t* buf, uint32_t bufLen)
{
    int handle = sCurrentConnectedHandle;
    SyncEventGuard g (sTransceiveEvent);
    ALOGD ("%s: data len=%d", __FUNCTION__, bufLen);

    if (NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_MIFARE)
    {
       if (EXTNS_GetCallBackFlag() == FALSE)
       {
           EXTNS_MfcCallBack(buf, bufLen);
           return;
       }
    }

    if (!sWaitingForTransceive)
    {
        ALOGE ("%s: drop data", __FUNCTION__);
        return;
    }
    sRxDataStatus = status;
    if (sRxDataStatus == NFA_STATUS_OK || sRxDataStatus == NFA_STATUS_CONTINUE)
        sRxDataBuffer.append (buf, bufLen);

    if (sRxDataStatus == NFA_STATUS_OK)
        sTransceiveEvent.notifyOne ();
}


void nativeNfcTag_notifyRfTimeout ()
{
    SyncEventGuard g (sTransceiveEvent);
    ALOGD ("%s: waiting for transceive: %d", __FUNCTION__, sWaitingForTransceive);
    if (!sWaitingForTransceive)
        return;

    sTransceiveRfTimeout = true;

    sTransceiveEvent.notifyOne ();
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doTransceive
**
** Description:     Send raw data to the tag; receive tag's response.
**                  e: JVM environment.
**                  o: Java object.
**                  raw: Not used.
**                  statusTargetLost: Whether tag responds or times out.
**
** Returns:         Response from tag.
**
*******************************************************************************/
static jbyteArray nativeNfcTag_doTransceive (JNIEnv* e, jobject o, jbyteArray data, jboolean raw, jintArray statusTargetLost)
{
    int timeout = NfcTag::getInstance ().getTransceiveTimeout (sCurrentConnectedTargetType);
    ALOGD ("%s: enter; raw=%u; timeout = %d", __FUNCTION__, raw, timeout);

    int handle = sCurrentConnectedHandle;
    bool waitOk = false;
    bool isNack = false;
    jint *targetLost = NULL;
    tNFA_STATUS status;
#if(NXP_EXTNS == TRUE && NFC_NXP_NON_STD_CARD == TRUE)
    bool fNeedToSwitchBack = false;
#endif
    if (NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_MIFARE)
    {
        if( doReconnectFlag == 0)
        {
            int retCode = NFCSTATUS_SUCCESS;
            retCode = nativeNfcTag_doReconnect (e, o);
            doReconnectFlag = 0x01;
        }
    }

    if (NfcTag::getInstance ().getActivationState () != NfcTag::Active)
    {
        if (statusTargetLost)
        {
            targetLost = e->GetIntArrayElements (statusTargetLost, 0);
            if (targetLost)
                *targetLost = 1; //causes NFC service to throw TagLostException
            e->ReleaseIntArrayElements (statusTargetLost, targetLost, 0);
        }
        ALOGD ("%s: tag not active", __FUNCTION__);
        return NULL;
    }

    NfcTag& natTag = NfcTag::getInstance ();

    // get input buffer and length from java call
    ScopedByteArrayRO bytes(e, data);
    uint8_t* buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&bytes[0])); // TODO: API bug; NFA_SendRawFrame should take const*!
    size_t bufLen = bytes.size();

    if (statusTargetLost)
    {
        targetLost = e->GetIntArrayElements (statusTargetLost, 0);
        if (targetLost)
            *targetLost = 0; //success, tag is still present
    }

    sSwitchBackTimer.kill ();
    ScopedLocalRef<jbyteArray> result(e, NULL);
    do
    {
#if(NXP_EXTNS == TRUE && NFC_NXP_NON_STD_CARD == TRUE)
        if (sNeedToSwitchRf)
        {
            if (!switchRfInterface (NFA_INTERFACE_FRAME)) //NFA_INTERFACE_ISO_DEP
            {
                break;
            }
            fNeedToSwitchBack = true;
        }
#endif
        {
            SyncEventGuard g (sTransceiveEvent);
            sTransceiveRfTimeout = false;
            sWaitingForTransceive = true;
            sRxDataStatus = NFA_STATUS_OK;
            sRxDataBuffer.clear ();
            if (NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_MIFARE)
            {
                status = EXTNS_MfcTransceive(buf, bufLen);
            }
            else
            {
                status = NFA_SendRawFrame (buf, bufLen,
                        NFA_DM_DEFAULT_PRESENCE_CHECK_START_DELAY);
            }

            if (status != NFA_STATUS_OK)
            {
                ALOGE ("%s: fail send; error=%d", __FUNCTION__, status);
                break;
            }
            waitOk = sTransceiveEvent.wait (gGeneralTransceiveTimeout);
        }

        if (waitOk == false || sTransceiveRfTimeout) //if timeout occurred
        {
            ALOGE ("%s: wait response timeout", __FUNCTION__);
            if (targetLost)
                *targetLost = 1; //causes NFC service to throw TagLostException
            break;
        }

        if (NfcTag::getInstance ().getActivationState () != NfcTag::Active)
        {
            ALOGE ("%s: already deactivated", __FUNCTION__);
            if (targetLost)
                *targetLost = 1; //causes NFC service to throw TagLostException
            break;
        }

        ALOGD ("%s: response %d bytes", __FUNCTION__, sRxDataBuffer.size());

        if ((natTag.getProtocol () == NFA_PROTOCOL_T2T) &&
            natTag.isT2tNackResponse (sRxDataBuffer.data(), sRxDataBuffer.size()))
        {
            isNack = true;
        }

        if (sRxDataBuffer.size() > 0)
        {
            if (isNack)
            {
                //Some Mifare Ultralight C tags enter the HALT state after it
                //responds with a NACK.  Need to perform a "reconnect" operation
                //to wake it.
                ALOGD ("%s: try reconnect", __FUNCTION__);
                nativeNfcTag_doReconnect (NULL, NULL);
                ALOGD ("%s: reconnect finish", __FUNCTION__);
            }
            else if (NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_MIFARE)
            {
                uint32_t transDataLen = sRxDataBuffer.size();
                uint8_t *transData = (uint8_t *)sRxDataBuffer.data();
                if (EXTNS_CheckMfcResponse(&transData, &transDataLen) == NFCSTATUS_FAILED)
                {
                        int retCode = NFCSTATUS_SUCCESS;
                        retCode = nativeNfcTag_doReconnect (e, o);
                }
                else
                {
                    if(transDataLen != 0)
                    {
                        result.reset(e->NewByteArray(transDataLen));
                    }
                    if (result.get() != NULL)
                    {
                        e->SetByteArrayRegion(result.get(), 0, transDataLen, (const jbyte *) transData);
                    }
                    else
                        ALOGE ("%s: Failed to allocate java byte array", __FUNCTION__);
                }
            }
            else
            {
                // marshall data to java for return
                result.reset(e->NewByteArray(sRxDataBuffer.size()));
                if (result.get() != NULL)
                {
                    e->SetByteArrayRegion(result.get(), 0, sRxDataBuffer.size(), (const jbyte *) sRxDataBuffer.data());
                }
                else
                    ALOGE ("%s: Failed to allocate java byte array", __FUNCTION__);
            } // else a nack is treated as a transceive failure to the upper layers

            sRxDataBuffer.clear();
        }
    } while (0);

    sWaitingForTransceive = false;
    if (targetLost)
        e->ReleaseIntArrayElements (statusTargetLost, targetLost, 0);
#if(NXP_EXTNS == TRUE && NFC_NXP_NON_STD_CARD == TRUE)
    if (fNeedToSwitchBack)
    {
        sSwitchBackTimer.set (1500, switchBackTimerProc);
    }
#endif
    ALOGD ("%s: exit", __FUNCTION__);
    return result.release();
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doGetNdefType
**
** Description:     Retrieve the type of tag.
**                  e: JVM environment.
**                  o: Java object.
**                  libnfcType: Type of tag represented by JNI.
**                  javaType: Not used.
**
** Returns:         Type of tag represented by NFC Service.
**
*******************************************************************************/
static jint nativeNfcTag_doGetNdefType (JNIEnv*, jobject, jint libnfcType, jint javaType)
{
    ALOGD ("%s: enter; libnfc type=%d; java type=%d", __FUNCTION__, libnfcType, javaType);
    jint ndefType = NDEF_UNKNOWN_TYPE;

    // For NFA, libnfcType is mapped to the protocol value received
    // in the NFA_ACTIVATED_EVT and NFA_DISC_RESULT_EVT event.
    switch (libnfcType) {
    case NFA_PROTOCOL_T1T:
        ndefType = NDEF_TYPE1_TAG;
        break;
    case NFA_PROTOCOL_T2T:
        ndefType = NDEF_TYPE2_TAG;;
        break;
    case NFA_PROTOCOL_T3T:
        ndefType = NDEF_TYPE3_TAG;
        break;
    case NFA_PROTOCOL_ISO_DEP:
        ndefType = NDEF_TYPE4_TAG;
        break;
    case NFA_PROTOCOL_ISO15693:
        ndefType = NDEF_UNKNOWN_TYPE;
        break;
    case NFA_PROTOCOL_MIFARE:
        ndefType = NDEF_MIFARE_CLASSIC_TAG;
        break;
    case NFA_PROTOCOL_INVALID:
        ndefType = NDEF_UNKNOWN_TYPE;
        break;
    default:
        ndefType = NDEF_UNKNOWN_TYPE;
        break;
    }
    ALOGD ("%s: exit; ndef type=%d", __FUNCTION__, ndefType);
    return ndefType;
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doCheckNdefResult
**
** Description:     Receive the result of checking whether the tag contains a NDEF
**                  message.  Called by the NFA_NDEF_DETECT_EVT.
**                  status: Status of the operation.
**                  maxSize: Maximum size of NDEF message.
**                  currentSize: Current size of NDEF message.
**                  flags: Indicate various states.
**
** Returns:         None
**
*******************************************************************************/
void nativeNfcTag_doCheckNdefResult (tNFA_STATUS status, uint32_t maxSize, uint32_t currentSize, uint8_t flags)
{
    //this function's flags parameter is defined using the following macros
    //in nfc/include/rw_api.h;
    //#define RW_NDEF_FL_READ_ONLY  0x01    /* Tag is read only              */
    //#define RW_NDEF_FL_FORMATED   0x02    /* Tag formated for NDEF         */
    //#define RW_NDEF_FL_SUPPORTED  0x04    /* NDEF supported by the tag     */
    //#define RW_NDEF_FL_UNKNOWN    0x08    /* Unable to find if tag is ndef capable/formated/read only */
    //#define RW_NDEF_FL_FORMATABLE 0x10    /* Tag supports format operation */

    if (!sCheckNdefWaitingForComplete)
    {
        ALOGE ("%s: not waiting", __FUNCTION__);
        return;
    }

    if (flags & RW_NDEF_FL_READ_ONLY)
        ALOGD ("%s: flag read-only", __FUNCTION__);
    if (flags & RW_NDEF_FL_FORMATED)
        ALOGD ("%s: flag formatted for ndef", __FUNCTION__);
    if (flags & RW_NDEF_FL_SUPPORTED)
        ALOGD ("%s: flag ndef supported", __FUNCTION__);
    if (flags & RW_NDEF_FL_UNKNOWN)
        ALOGD ("%s: flag all unknown", __FUNCTION__);
    if (flags & RW_NDEF_FL_FORMATABLE)
        ALOGD ("%s: flag formattable", __FUNCTION__);

    sCheckNdefWaitingForComplete = JNI_FALSE;
    sCheckNdefStatus = status;
    if (sCheckNdefStatus != NFA_STATUS_OK && sCheckNdefStatus != NFA_STATUS_TIMEOUT)
        sCheckNdefStatus = NFA_STATUS_FAILED;
    sCheckNdefCapable = false; //assume tag is NOT ndef capable
    if (sCheckNdefStatus == NFA_STATUS_OK)
    {
        //NDEF content is on the tag
        sCheckNdefMaxSize = maxSize;
        sCheckNdefCurrentSize = currentSize;
        sCheckNdefCardReadOnly = flags & RW_NDEF_FL_READ_ONLY;
        sCheckNdefCapable = true;
    }
    else if (sCheckNdefStatus == NFA_STATUS_FAILED)
    {
        //no NDEF content on the tag
        sCheckNdefMaxSize = 0;
        sCheckNdefCurrentSize = 0;
        sCheckNdefCardReadOnly = flags & RW_NDEF_FL_READ_ONLY;
        if ((flags & RW_NDEF_FL_UNKNOWN) == 0) //if stack understands the tag
        {
            if (flags & RW_NDEF_FL_SUPPORTED) //if tag is ndef capable
                sCheckNdefCapable = true;
        }
    }
    else if (sCheckNdefStatus == NFA_STATUS_TIMEOUT)
    {
        ALOGE("%s: timeout", __FUNCTION__);

        sCheckNdefMaxSize = 0;
        sCheckNdefCurrentSize = 0;
        sCheckNdefCardReadOnly = false;
    }
    else
    {
        ALOGE ("%s: unknown status=0x%X", __FUNCTION__, status);
        sCheckNdefMaxSize = 0;
        sCheckNdefCurrentSize = 0;
        sCheckNdefCardReadOnly = false;
    }
    sem_post (&sCheckNdefSem);
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doCheckNdef
**
** Description:     Does the tag contain a NDEF message?
**                  e: JVM environment.
**                  o: Java object.
**                  ndefInfo: NDEF info.
**
** Returns:         Status code; 0 is success.
**
*******************************************************************************/
static jint nativeNfcTag_doCheckNdef (JNIEnv* e, jobject o, jintArray ndefInfo)
{
    tNFA_STATUS status = NFA_STATUS_FAILED;
    jint* ndef = NULL;
    int handle = sCurrentConnectedHandle;
    ALOGD ("%s: enter; handle=%x", __FUNCTION__, handle);

    ALOGD ("%s: enter", __FUNCTION__);
    sIsCheckingNDef = true;
#if (NXP_EXTNS == TRUE)
    if (NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_T3BT)
    {
        ndef = e->GetIntArrayElements (ndefInfo, 0);
        ndef[0] = 0;
        ndef[1] = NDEF_MODE_READ_ONLY;
        e->ReleaseIntArrayElements (ndefInfo, ndef, 0);
        sIsCheckingNDef = false;
        return NFA_STATUS_FAILED;
    }
#endif

    // special case for Kovio
    if (NfcTag::getInstance ().mTechList [handle] == TARGET_TYPE_KOVIO_BARCODE)
    {
        ALOGD ("%s: Kovio tag, no NDEF", __FUNCTION__);
        ndef = e->GetIntArrayElements (ndefInfo, 0);
        ndef[0] = 0;
        ndef[1] = NDEF_MODE_READ_ONLY;
        e->ReleaseIntArrayElements (ndefInfo, ndef, 0);
        sIsCheckingNDef = false;
        return NFA_STATUS_FAILED;
    }
    if (NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_MIFARE)
    {
        int retCode = NFCSTATUS_SUCCESS;
        retCode = nativeNfcTag_doReconnect (e, o);
    }

    doReconnectFlag = 0;

    /* Create the write semaphore */
    if (sem_init (&sCheckNdefSem, 0, 0) == -1)
    {
        ALOGE ("%s: Check NDEF semaphore creation failed (errno=0x%08x)", __FUNCTION__, errno);
        sIsCheckingNDef = false;
        return JNI_FALSE;
    }

    if (NfcTag::getInstance ().getActivationState () != NfcTag::Active)
    {
        ALOGE ("%s: tag already deactivated", __FUNCTION__);
        goto TheEnd;
    }

    ALOGD ("%s: try NFA_RwDetectNDef", __FUNCTION__);
    sCheckNdefWaitingForComplete = JNI_TRUE;

    ALOGD ("%s: NfcTag::getInstance ().mTechLibNfcTypes[%d]=%d", __FUNCTION__, NfcTag::getInstance ().mTechLibNfcTypes[handle]);

    if (NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_MIFARE)
    {
        status = EXTNS_MfcCheckNDef ();
    }else{
        status = NFA_RwDetectNDef ();
    }

    if (status != NFA_STATUS_OK)
    {
        ALOGE ("%s: NFA_RwDetectNDef failed, status = 0x%X", __FUNCTION__, status);
        goto TheEnd;
    }

    /* Wait for check NDEF completion status */
    if (sem_wait (&sCheckNdefSem))
    {
        ALOGE ("%s: Failed to wait for check NDEF semaphore (errno=0x%08x)", __FUNCTION__, errno);
        goto TheEnd;
    }

    if (sCheckNdefStatus == NFA_STATUS_OK)
    {
        //stack found a NDEF message on the tag
        ndef = e->GetIntArrayElements (ndefInfo, 0);
        if (NfcTag::getInstance ().getProtocol () == NFA_PROTOCOL_T1T)
            ndef[0] = NfcTag::getInstance ().getT1tMaxMessageSize ();
        else
            ndef[0] = sCheckNdefMaxSize;
        if (sCheckNdefCardReadOnly)
            ndef[1] = NDEF_MODE_READ_ONLY;
        else
            ndef[1] = NDEF_MODE_READ_WRITE;
        e->ReleaseIntArrayElements (ndefInfo, ndef, 0);
        status = NFA_STATUS_OK;
    }
    else if (sCheckNdefStatus == NFA_STATUS_FAILED)
    {
        //stack did not find a NDEF message on the tag;
        ndef = e->GetIntArrayElements (ndefInfo, 0);
        if (NfcTag::getInstance ().getProtocol () == NFA_PROTOCOL_T1T)
            ndef[0] = NfcTag::getInstance ().getT1tMaxMessageSize ();
        else
            ndef[0] = sCheckNdefMaxSize;
        if (sCheckNdefCardReadOnly)
            ndef[1] = NDEF_MODE_READ_ONLY;
        else
            ndef[1] = NDEF_MODE_READ_WRITE;
        e->ReleaseIntArrayElements (ndefInfo, ndef, 0);
        status = NFA_STATUS_FAILED;
        if (setNdefDetectionTimeoutIfTagAbsent(e, o, NFA_PROTOCOL_T3T | NFA_PROTOCOL_ISO15693))
            status = STATUS_CODE_TARGET_LOST;
    }
    else if ((sCheckNdefStatus == NFA_STATUS_TIMEOUT) && (NfcTag::getInstance ().getProtocol() == NFC_PROTOCOL_ISO_DEP))
    {
        pn544InteropStopPolling ();
        status = STATUS_CODE_TARGET_LOST;
    }
    else
    {
        ALOGD ("%s: unknown status 0x%X", __FUNCTION__, sCheckNdefStatus);
        status = sCheckNdefStatus;
    }

TheEnd:
    /* Destroy semaphore */
    if (sem_destroy (&sCheckNdefSem))
    {
        ALOGE ("%s: Failed to destroy check NDEF semaphore (errno=0x%08x)", __FUNCTION__, errno);
    }
    sCheckNdefWaitingForComplete = JNI_FALSE;
    sIsCheckingNDef = false;
    ALOGD ("%s: exit; status=0x%X", __FUNCTION__, status);
    return status;
}


/*******************************************************************************
**
** Function:        nativeNfcTag_resetPresenceCheck
**
** Description:     Reset variables related to presence-check.
**
** Returns:         None
**
*******************************************************************************/
void nativeNfcTag_resetPresenceCheck ()
{
    sIsTagPresent = true;
    NfcTag::getInstance ().mCashbeeDetected = false;
    NfcTag::getInstance ().mEzLinkTypeTag = false;
    MfcResetPresenceCheckStatus();
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doPresenceCheckResult
**
** Description:     Receive the result of presence-check.
**                  status: Result of presence-check.
**
** Returns:         None
**
*******************************************************************************/
void nativeNfcTag_doPresenceCheckResult (tNFA_STATUS status)
{
    SyncEventGuard guard (sPresenceCheckEvent);
    sIsTagPresent = status == NFA_STATUS_OK;
    sPresenceCheckEvent.notifyOne ();
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doPresenceCheck
**
** Description:     Check if the tag is in the RF field.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         True if tag is in RF field.
**
*******************************************************************************/
static jboolean nativeNfcTag_doPresenceCheck (JNIEnv*, jobject)
{
    ALOGD ("%s", __FUNCTION__);
    tNFA_STATUS status = NFA_STATUS_OK;
    jboolean isPresent = JNI_FALSE;
    UINT8* uid;
    UINT32 uid_len;
    NfcTag::getInstance ().getTypeATagUID(&uid,&uid_len);
    int handle = sCurrentConnectedHandle;

    if(NfcTag::getInstance().mNfcDisableinProgress)
    {
        ALOGD("%s, Nfc disable in progress",__FUNCTION__);
        return JNI_FALSE;
    }

    if (sIsCheckingNDef == true)
    {
        ALOGD("%s: Ndef is being checked", __FUNCTION__);
        return JNI_TRUE;
    }
    if (fNeedToSwitchBack)
    {
        sSwitchBackTimer.kill ();
    }
    if (nfcManager_isNfcActive() == false)
    {
        ALOGD ("%s: NFC is no longer active.", __FUNCTION__);
        return JNI_FALSE;
    }

    if (!sRfInterfaceMutex.tryLock())
    {
        ALOGD ("%s: tag is being reSelected assume it is present", __FUNCTION__);
        return JNI_TRUE;
    }

    sRfInterfaceMutex.unlock();

    if (NfcTag::getInstance ().isActivated () == false)
    {
        ALOGD ("%s: tag already deactivated", __FUNCTION__);
        return JNI_FALSE;
    }

    /*Presence check for Kovio - RF Deactive command with type Discovery*/
    ALOGD ("%s: handle=%d", __FUNCTION__, handle);
    if (NfcTag::getInstance ().mTechList [handle] == TARGET_TYPE_KOVIO_BARCODE)
    {
        SyncEventGuard guard (sPresenceCheckEvent);
        status = NFA_RwPresenceCheck (NfcTag::getInstance().getPresenceCheckAlgorithm());
        if (status == NFA_STATUS_OK)
        {
            sPresenceCheckEvent.wait ();
            isPresent = sIsTagPresent ? JNI_TRUE : JNI_FALSE;
        }
        if (isPresent == JNI_FALSE)
            ALOGD ("%s: tag absent", __FUNCTION__);
        return isPresent;
#if 0
        ALOGD ("%s: Kovio, force deactivate handling", __FUNCTION__);
        tNFA_DEACTIVATED deactivated = {NFA_DEACTIVATE_TYPE_IDLE};
        {
            SyncEventGuard g (gDeactivatedEvent);
            gActivated = false; //guard this variable from multi-threaded access
            gDeactivatedEvent.notifyOne ();
        }

        NfcTag::getInstance().setDeactivationState (deactivated);
        nativeNfcTag_resetPresenceCheck();
        NfcTag::getInstance().connectionEventHandler (NFA_DEACTIVATED_EVT, NULL);
        nativeNfcTag_abortWaits();
        NfcTag::getInstance().abort ();

        return JNI_FALSE;
#endif
    }

    /*
     * This fix is made because NFA_RwPresenceCheck cmd is not woking for ISO-DEP in CEFH mode
     * Hence used the Properitary presence check cmd
     * */

    if (NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_ISO_DEP)
    {
        if (sIsReconnecting == true)
        {
            ALOGD("%s: Reconnecting Tag", __FUNCTION__);
            return JNI_TRUE;
        }
        ALOGD ("%s: presence check for TypeB / TypeA random uid", __FUNCTION__);
        sPresenceCheckTimer.set(500, presenceCheckTimerProc);

        tNFC_STATUS stat = NFA_RegVSCback (true,nfaVSCNtfCallback); //Register CallBack for VS NTF
        if(NFA_STATUS_OK != stat)
        {
            goto TheEnd;
        }

        SyncEventGuard guard (sNfaVSCResponseEvent);
        stat = NFA_SendVsCommand (0x11,0x00,NULL,nfaVSCCallback);
        if(NFA_STATUS_OK == stat)
        {
            ALOGD ("%s: presence check for TypeB - wait for NFA VS RSP to come", __FUNCTION__);
            sNfaVSCResponseEvent.wait(); //wait for NFA VS command to finish
            ALOGD ("%s: presence check for TypeB - GOT NFA VS RSP", __FUNCTION__);
        }

        if(true == sVSCRsp)
        {
            {
                SyncEventGuard guard (sNfaVSCNotificationEvent);
                ALOGD ("%s: presence check for TypeB - wait for NFA VS NTF to come", __FUNCTION__);
                sNfaVSCNotificationEvent.wait(); //wait for NFA VS NTF to come
                ALOGD ("%s: presence check for TypeB - GOT NFA VS NTF", __FUNCTION__);
                sPresenceCheckTimer.kill();
            }

            if(false == sIsTagInField)
            {
                isPresent = JNI_FALSE;
            }
            else
            {
                isPresent =  JNI_TRUE;
            }
        }
        NFA_RegVSCback (false,nfaVSCNtfCallback); //DeRegister CallBack for VS NTF
        ALOGD ("%s: presence check for TypeB - return", __FUNCTION__);
        goto TheEnd;
    }

#if (NXP_EXTNS == TRUE)
    if(NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_T3BT)
    {
        UINT8 *pbuf = NULL;
        UINT8 bufLen = 0x00;
        bool waitOk = false;

        SyncEventGuard g (sTransceiveEvent);
        sTransceiveRfTimeout = false;
        sWaitingForTransceive = true;
        //sTransceiveDataLen = 0;
        bufLen = (UINT8) sizeof(Presence_check_TypeB);
        pbuf = Presence_check_TypeB;
        //memcpy(pbuf, Attrib_cmd_TypeB, bufLen);
        status = NFA_SendRawFrame (pbuf, bufLen,NFA_DM_DEFAULT_PRESENCE_CHECK_START_DELAY);
        if (status != NFA_STATUS_OK)
        {
            ALOGE ("%s: fail send; error=%d", __FUNCTION__, status);
        }
        else
            waitOk = sTransceiveEvent.wait (gGeneralTransceiveTimeout);

        if (waitOk == false || sTransceiveRfTimeout) //if timeout occurred
        {
            return JNI_FALSE;;
        }
        else
        {
            return JNI_TRUE;
        }
    }
#endif

    if (NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_MIFARE)
    {
        ALOGE ("Calling EXTNS_MfcPresenceCheck");
        status = EXTNS_MfcPresenceCheck();
        if (status == NFCSTATUS_SUCCESS)
        {
            if (NFCSTATUS_SUCCESS == EXTNS_GetPresenceCheckStatus())
            {
                return JNI_TRUE;
            }
            else
            {
                return JNI_FALSE;
            }
        }
    }
    {
        SyncEventGuard guard (sPresenceCheckEvent);
        status = NFA_RwPresenceCheck (NfcTag::getInstance().getPresenceCheckAlgorithm());
        if (status == NFA_STATUS_OK)
        {
            sPresenceCheckEvent.wait ();
            isPresent = sIsTagPresent ? JNI_TRUE : JNI_FALSE;
        }
    }

    TheEnd:

    if (isPresent == JNI_FALSE)
        ALOGD ("%s: tag absent", __FUNCTION__);
    return isPresent;
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doIsNdefFormatable
**
** Description:     Can tag be formatted to store NDEF message?
**                  e: JVM environment.
**                  o: Java object.
**                  libNfcType: Type of tag.
**                  uidBytes: Tag's unique ID.
**                  pollBytes: Data from activation.
**                  actBytes: Data from activation.
**
** Returns:         True if formattable.
**
*******************************************************************************/
static jboolean nativeNfcTag_doIsNdefFormatable (JNIEnv* e,
        jobject o, jint /*libNfcType*/, jbyteArray, jbyteArray,
        jbyteArray)
{
    jboolean isFormattable = JNI_FALSE;

    switch (NfcTag::getInstance().getProtocol())
    {
    case NFA_PROTOCOL_T1T:
    case NFA_PROTOCOL_ISO15693:
    case NFA_PROTOCOL_MIFARE:
        isFormattable = JNI_TRUE;
        break;

    case NFA_PROTOCOL_T3T:
        isFormattable = NfcTag::getInstance().isFelicaLite() ? JNI_TRUE : JNI_FALSE;
        break;

    case NFA_PROTOCOL_T2T:
        isFormattable = ( NfcTag::getInstance().isMifareUltralight() |
                          NfcTag::getInstance().isInfineonMyDMove() |
                          NfcTag::getInstance().isKovioType2Tag() )
                        ? JNI_TRUE : JNI_FALSE;
        break;
    case NFA_PROTOCOL_ISO_DEP:
        /**
         * Determines whether this is a formatable IsoDep tag - currectly only NXP DESFire
         * is supported.
         */
        uint8_t  cmd[] = {0x90, 0x60, 0x00, 0x00, 0x00};

        if(NfcTag::getInstance().isMifareDESFire())
        {
            /* Identifies as DESfire, use get version cmd to be sure */
            jbyteArray versionCmd = e->NewByteArray(5);
            e->SetByteArrayRegion(versionCmd, 0, 5, (jbyte*)cmd);
            jbyteArray respBytes = nativeNfcTag_doTransceive(e, o,
                        versionCmd, JNI_TRUE, NULL);
            if (respBytes != NULL)
            {
                // Check whether the response matches a typical DESfire
                // response.
                // libNFC even does more advanced checking than we do
                // here, and will only format DESfire's with a certain
                // major/minor sw version and NXP as a manufacturer.
                // We don't want to do such checking here, to avoid
                // having to change code in multiple places.
                // A succesful (wrapped) DESFire getVersion command returns
                // 9 bytes, with byte 7 0x91 and byte 8 having status
                // code 0xAF (these values are fixed and well-known).
                int respLength = e->GetArrayLength(respBytes);
                uint8_t* resp = (uint8_t*)e->GetByteArrayElements(respBytes, NULL);
                if (respLength == 9 && resp[7] == 0x91 && resp[8] == 0xAF)
                {
                    isFormattable = JNI_TRUE;
                }
                e->ReleaseByteArrayElements(respBytes, (jbyte *)resp, JNI_ABORT);
            }
        }
        break;
    }
    ALOGD("%s: is formattable=%u", __FUNCTION__, isFormattable);
    return isFormattable;
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doIsIsoDepNdefFormatable
**
** Description:     Is ISO-DEP tag formattable?
**                  e: JVM environment.
**                  o: Java object.
**                  pollBytes: Data from activation.
**                  actBytes: Data from activation.
**
** Returns:         True if formattable.
**
*******************************************************************************/
static jboolean nativeNfcTag_doIsIsoDepNdefFormatable (JNIEnv *e, jobject o, jbyteArray pollBytes, jbyteArray actBytes)
{
    uint8_t uidFake[] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
    ALOGD ("%s", __FUNCTION__);
    jbyteArray uidArray = e->NewByteArray (8);
    e->SetByteArrayRegion (uidArray, 0, 8, (jbyte*) uidFake);
    return nativeNfcTag_doIsNdefFormatable (e, o, 0, uidArray, pollBytes, actBytes);
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doNdefFormat
**
** Description:     Format a tag so it can store NDEF message.
**                  e: JVM environment.
**                  o: Java object.
**                  key: Not used.
**
** Returns:         True if ok.
**
*******************************************************************************/
static jboolean nativeNfcTag_doNdefFormat (JNIEnv *e, jobject o, jbyteArray)
{
    ALOGD ("%s: enter", __FUNCTION__);
    tNFA_STATUS status = NFA_STATUS_OK;
    int handle = sCurrentConnectedHandle;
#if(NXP_EXTNS == TRUE)
    isMifare = false;
#endif

    // Do not try to format if tag is already deactivated.
    if (NfcTag::getInstance ().isActivated () == false)
    {
        ALOGD ("%s: tag already deactivated(no need to format)", __FUNCTION__);
        return JNI_FALSE;
    }

    sem_init (&sFormatSem, 0, 0);
    sFormatOk = false;
    if (NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_MIFARE)
    {
        status = nativeNfcTag_doReconnect (e, o);
#if(NXP_EXTNS == TRUE)
        isMifare = true;
        ALOGD("Format with First Key");
        status = EXTNS_MfcFormatTag(key1,sizeof(key1));
#endif
    }
    else
    {
        status = NFA_RwFormatTag ();
    }
    if (status == NFA_STATUS_OK)
    {
        ALOGD ("%s: wait for completion", __FUNCTION__);
        sem_wait (&sFormatSem);
        status = sFormatOk ? NFA_STATUS_OK : NFA_STATUS_FAILED;
#if(NXP_EXTNS == TRUE)
        if(sFormatOk == true && isMifare == true)
            ALOGD ("Formay with First Key Success");
#endif
    }
    else
        ALOGE ("%s: error status=%u", __FUNCTION__, status);
    sem_destroy (&sFormatSem);

#if(NXP_EXTNS == TRUE)
    if(isMifare == true && sFormatOk != true)
    {
    ALOGD ("Format with First Key Failed");

        sem_init (&sFormatSem, 0, 0);

        status = nativeNfcTag_doReconnect (e, o);
        ALOGD ("Format with Second Key");
        status |= EXTNS_MfcFormatTag(key2,sizeof(key2));
        if (status == NFA_STATUS_OK)
        {
            ALOGD ("%s:2nd try wait for completion", __FUNCTION__);
            sem_wait (&sFormatSem);
            status = sFormatOk ? NFA_STATUS_OK : NFA_STATUS_FAILED;
        }
        /*When tryng with the second key ensure that the target reconnect is successful,
          if not update the connect flag accordingly */
        else
        {
            ALOGE ("%s: error status=%u", __FUNCTION__, status);
            EXTNS_SetConnectFlag (FALSE);
        }
        sem_destroy (&sFormatSem);

        if(sFormatOk)
            ALOGD ("Formay with Second Key Success");
        else
            ALOGD ("Formay with Second Key Failed");
    }
#endif

    if (NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_ISO_DEP)
    {
        int retCode = NFCSTATUS_SUCCESS;
        retCode = nativeNfcTag_doReconnect (e, o);
    }
    ALOGD ("%s: exit", __FUNCTION__);
    return (status == NFA_STATUS_OK) ? JNI_TRUE : JNI_FALSE;
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doMakeReadonlyResult
**
** Description:     Receive the result of making a tag read-only. Called by the
**                  NFA_SET_TAG_RO_EVT.
**                  status: Status of the operation.
**
** Returns:         None
**
*******************************************************************************/
void nativeNfcTag_doMakeReadonlyResult (tNFA_STATUS status)
{
    if (sMakeReadonlyWaitingForComplete != JNI_FALSE)
    {
        sMakeReadonlyWaitingForComplete = JNI_FALSE;
        sMakeReadonlyStatus = status;

        sem_post (&sMakeReadonlySem);
    }
}


/*******************************************************************************
**
** Function:        nativeNfcTag_doMakeReadonly
**
** Description:     Make the tag read-only.
**                  e: JVM environment.
**                  o: Java object.
**                  key: Key to access the tag.
**
** Returns:         True if ok.
**
*******************************************************************************/
static jboolean nativeNfcTag_doMakeReadonly (JNIEnv *e, jobject o, jbyteArray)
{
    jboolean result = JNI_FALSE;
    tNFA_STATUS status = NFA_STATUS_OK;
    int handle = sCurrentConnectedHandle;

    ALOGD ("%s", __FUNCTION__);

    isMifare = false;
    /* Create the make_readonly semaphore */
    if (sem_init (&sMakeReadonlySem, 0, 0) == -1)
    {
        ALOGE ("%s: Make readonly semaphore creation failed (errno=0x%08x)", __FUNCTION__, errno);
        return JNI_FALSE;
    }

    sMakeReadonlyWaitingForComplete = JNI_TRUE;

    sReadOnlyOk = false;
    if (NfcTag::getInstance ().mTechLibNfcTypes[handle] == NFA_PROTOCOL_MIFARE)
    {
        status = nativeNfcTag_doReconnect (e, o);
        ALOGD ("Calling EXTNS_MfcSetReadOnly");
        ALOGD ("SetReadOnly with First Key");
        if(status == NFA_STATUS_OK)
        {
            isMifare = true;
            status = EXTNS_MfcSetReadOnly(key1,sizeof(key1));
        }
    }
    else
    {
        // Hard-lock the tag (cannot be reverted)
        status = NFA_RwSetTagReadOnly(TRUE);
        if (status == NFA_STATUS_REJECTED)
        {
            status = NFA_RwSetTagReadOnly (FALSE); //try soft lock
            if (status != NFA_STATUS_OK)
            {
                ALOGE ("%s: fail soft lock, status=%d", __FUNCTION__, status);
                goto TheEnd;
            }
        }
    }
    if(status == NFA_STATUS_OK)
    {
        ALOGD ("%s: wait for completion for First Key", __FUNCTION__);
        sem_wait(&sMakeReadonlySem);

        ALOGD("ReadOnly Value = %d",sMakeReadonlyStatus);
        if(sMakeReadonlyStatus == NFA_STATUS_OK)
        {
            sReadOnlyOk = true;
            ALOGD ("ReadOnly with First Key Success status  = 0x%0x",sMakeReadonlyStatus);
        }
        else
        {
            sReadOnlyOk = false;
            ALOGD ("ReadOnly with First Key Failed status  = 0x%0x",sMakeReadonlyStatus);
        }
    }

    if(sReadOnlyOk != true && isMifare == true)
    {
        /* Destroy semaphore */
        if (sem_destroy (&sMakeReadonlySem))
        {
            ALOGE ("%s: Failed to destroy read_only semaphore (errno=0x%08x)", __FUNCTION__, errno);
        }
        /* Create the make_readonly semaphore */
        if (sem_init (&sMakeReadonlySem, 0, 0) == -1)
        {
            ALOGE ("%s: Make readonly semaphore creation failed (errno=0x%08x)", __FUNCTION__, errno);
            return JNI_FALSE;
        }
        sMakeReadonlyWaitingForComplete = JNI_TRUE;
        ALOGE ("ReadOnly with First Key Failed");
        ALOGD ("Calling EXTNS_MfcSetReadOnly with second Key");

        status = nativeNfcTag_doReconnect (e, o);
        if(status == NFA_STATUS_OK)
        {
            status = EXTNS_MfcSetReadOnly(key2,sizeof(key2));
        }
        else
        {
            ALOGE ("nativeNfcTag_doReconnect for Second time Failed");
        }


       if(status == NFA_STATUS_OK)
       {
          ALOGD ("%s: wait for completion for Second Key", __FUNCTION__);
          sem_wait(&sMakeReadonlySem);

          ALOGD("ReadOnly Value for second key = %d",sMakeReadonlyStatus);
          if(sMakeReadonlyStatus == NFA_STATUS_OK)
          {
              sReadOnlyOk = true;
              ALOGD ("ReadOnly with second Key Success");
          }
          else
          {
              sReadOnlyOk = false;
              ALOGE ("%s: error  ReadOnly with second Key also status=%u", __FUNCTION__, sMakeReadonlyStatus);
          }
       }
       else
       {
            ALOGE (" Sending EXTNS_MfcSetReadOnly with Second Key-- FAILED");
       }
    }
    else
    {
        ALOGE ("Sending EXTNS_MfcSetReadOnly with Second Key-- FAILED");
    }

    if(sReadOnlyOk == true)
    {
        sMakeReadonlyStatus = NFA_STATUS_OK;
        result = JNI_TRUE;
    }

TheEnd:
    /* Destroy semaphore */
    if (sem_destroy (&sMakeReadonlySem))
    {
        ALOGE ("%s: Failed to destroy read_only semaphore (errno=0x%08x)", __FUNCTION__, errno);
    }
    sMakeReadonlyWaitingForComplete = JNI_FALSE;
    return result;
}


/*******************************************************************************
**
** Function:        nativeNfcTag_registerNdefTypeHandler
**
** Description:     Register a callback to receive NDEF message from the tag
**                  from the NFA_NDEF_DATA_EVT.
**
** Returns:         None
**
*******************************************************************************/
//register a callback to receive NDEF message from the tag
//from the NFA_NDEF_DATA_EVT;
void nativeNfcTag_registerNdefTypeHandler ()
{
    ALOGD ("%s", __FUNCTION__);
    sNdefTypeHandlerHandle = NFA_HANDLE_INVALID;
    NFA_RegisterNDefTypeHandler (TRUE, NFA_TNF_DEFAULT, (UINT8 *) "", 0, ndefHandlerCallback);
    EXTNS_MfcRegisterNDefTypeHandler(ndefHandlerCallback);
}


/*******************************************************************************
**
** Function:        nativeNfcTag_deregisterNdefTypeHandler
**
** Description:     No longer need to receive NDEF message from the tag.
**
** Returns:         None
**
*******************************************************************************/
void nativeNfcTag_deregisterNdefTypeHandler ()
{
    ALOGD ("%s", __FUNCTION__);
    NFA_DeregisterNDefTypeHandler (sNdefTypeHandlerHandle);
    sNdefTypeHandlerHandle = NFA_HANDLE_INVALID;
}


/*******************************************************************************
**
** Function:        presenceCheckTimerProc
**
** Description:     Callback function for presence check timer.
**
** Returns:         None
**
*******************************************************************************/
static void presenceCheckTimerProc (union sigval)
{
    ALOGD ("%s", __FUNCTION__);
    sIsTagInField = false;
    sIsReconnecting = false;
    SyncEventGuard guard (sNfaVSCNotificationEvent);
    sNfaVSCNotificationEvent.notifyOne ();
}

/*******************************************************************************
**
** Function:        sReconnectTimerProc
**
** Description:     Callback function for reconnect timer.
**
** Returns:         None
**
*******************************************************************************/
static void sReconnectTimerProc (union sigval)
{
    ALOGD ("%s", __FUNCTION__);
    SyncEventGuard guard (sNfaVSCNotificationEvent);
    sNfaVSCNotificationEvent.notifyOne ();
}

/*******************************************************************************
**
** Function:        acquireRfInterfaceMutexLock
**
** Description:     acquire lock
**
** Returns:         None
**
*******************************************************************************/
void acquireRfInterfaceMutexLock()
{
    ALOGD ("%s: try to acquire lock", __FUNCTION__);
    sRfInterfaceMutex.lock();
    ALOGD ("%s: sRfInterfaceMutex lock", __FUNCTION__);
}

/*******************************************************************************
**
** Function:       releaseRfInterfaceMutexLock
**
** Description:    release the lock
**
** Returns:        None
**
*******************************************************************************/
void releaseRfInterfaceMutexLock()
{
    sRfInterfaceMutex.unlock();
    ALOGD ("%s: sRfInterfaceMutex unlock", __FUNCTION__);
}

/*****************************************************************************
**
** JNI functions for Android 4.0.3
**
*****************************************************************************/
static JNINativeMethod gMethods[] =
{
   {"doConnect", "(I)I", (void *)nativeNfcTag_doConnect},
   {"doDisconnect", "()Z", (void *)nativeNfcTag_doDisconnect},
   {"doReconnect", "()I", (void *)nativeNfcTag_doReconnect},
   {"doHandleReconnect", "(I)I", (void *)nativeNfcTag_doHandleReconnect},
   {"doTransceive", "([BZ[I)[B", (void *)nativeNfcTag_doTransceive},
   {"doGetNdefType", "(II)I", (void *)nativeNfcTag_doGetNdefType},
   {"doCheckNdef", "([I)I", (void *)nativeNfcTag_doCheckNdef},
   {"doRead", "()[B", (void *)nativeNfcTag_doRead},
   {"doWrite", "([B)Z", (void *)nativeNfcTag_doWrite},
   {"doPresenceCheck", "()Z", (void *)nativeNfcTag_doPresenceCheck},
   {"doIsIsoDepNdefFormatable", "([B[B)Z", (void *)nativeNfcTag_doIsIsoDepNdefFormatable},
   {"doNdefFormat", "([B)Z", (void *)nativeNfcTag_doNdefFormat},
   {"doMakeReadonly", "([B)Z", (void *)nativeNfcTag_doMakeReadonly},
};


/*******************************************************************************
**
** Function:        register_com_android_nfc_NativeNfcTag
**
** Description:     Regisgter JNI functions with Java Virtual Machine.
**                  e: Environment of JVM.
**
** Returns:         Status of registration.
**
*******************************************************************************/
int register_com_android_nfc_NativeNfcTag (JNIEnv *e)
{
    ALOGD ("%s", __FUNCTION__);
    return jniRegisterNativeMethods (e, gNativeNfcTagClassName, gMethods, NELEM (gMethods));
}


} /* namespace android */
