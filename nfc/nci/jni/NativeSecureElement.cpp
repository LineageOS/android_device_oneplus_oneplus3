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
#include "OverrideLog.h"
#include "SecureElement.h"
#include "JavaClassConstants.h"
#include "PowerSwitch.h"
#include "NfcTag.h"
#include "RoutingManager.h"
#include <ScopedPrimitiveArray.h>

extern bool hold_the_transceive;
extern int dual_mode_current_state;
namespace android
{

extern void startRfDiscovery (bool isStart);
extern bool isDiscoveryStarted();

extern void com_android_nfc_NfcManager_disableDiscovery (JNIEnv* e, jobject o);
extern void com_android_nfc_NfcManager_enableDiscovery (JNIEnv* e, jobject o, jint mode);
extern int gGeneralTransceiveTimeout;
static SyncEvent            sNfaVSCResponseEvent;
//static bool sRfEnabled;           /*commented to eliminate warning defined but not used*/

static void nfaVSCCallback(UINT8 event, UINT16 param_len, UINT8 *p_param);

inline static void nfaVSCCallback(UINT8 event, UINT16 param_len, UINT8 *p_param)    /*defined as inline to eliminate warning defined but not used*/
{
    (void)event;
    (void)param_len;
    (void)p_param;
    SyncEventGuard guard (sNfaVSCResponseEvent);
    sNfaVSCResponseEvent.notifyOne ();
}

// These must match the EE_ERROR_ types in NfcService.java
static const int EE_ERROR_IO = -1;
static const int EE_ERROR_ALREADY_OPEN = -2;
static const int EE_ERROR_INIT = -3;
static const int EE_ERROR_LISTEN_MODE = -4;
static const int EE_ERROR_EXT_FIELD = -5;
static const int EE_ERROR_NFC_DISABLED = -6;
static bool is_wired_mode_open = false;
/*******************************************************************************
**
** Function:        nativeNfcSecureElement_doOpenSecureElementConnection
**
** Description:     Connect to the secure element.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         Handle of secure element.  values < 0 represent failure.
**
*******************************************************************************/
static jint nativeNfcSecureElement_doOpenSecureElementConnection (JNIEnv*, jobject)
{
    ALOGD("%s: enter", __FUNCTION__);
    bool stat = false;
    jint secElemHandle = EE_ERROR_INIT;
#if((NFC_NXP_ESE == TRUE)&&(NXP_EXTNS == TRUE))
    long ret_val = -1;
    NFCSTATUS status = NFCSTATUS_FAILED;
    p61_access_state_t p61_current_state = P61_STATE_INVALID;
#endif
    SecureElement &se = SecureElement::getInstance();
    if (se.isBusy())
    {
        goto TheEnd;
    }
#if (NFC_NXP_ESE ==  TRUE && NFC_NXP_CHIP_TYPE != PN547C2)
    if((RoutingManager::getInstance().is_ee_recovery_ongoing()))
    {
        ALOGD ("ee recovery ongoing!!!");
        goto TheEnd;
    }
    stat = se.isWiredModeAllowedInRfState();
    if(stat == false)
    {
        ALOGD("Denying SE open due to SE listen mode active");
        secElemHandle = EE_ERROR_LISTEN_MODE;
        goto TheEnd;
    }
#else
    if (se.isActivatedInListenMode()) {
        ALOGD("Denying SE open due to SE listen mode active");
        secElemHandle = EE_ERROR_LISTEN_MODE;
        goto TheEnd;
    }

    if (se.isRfFieldOn()) {
        ALOGD("Denying SE open due to SE in active RF field");
        secElemHandle = EE_ERROR_EXT_FIELD;
        goto TheEnd;
    }
#endif

#if((NFC_NXP_ESE == TRUE)&&(NXP_EXTNS == TRUE))
    ret_val = NFC_GetP61Status ((void *)&p61_current_state);
    if (ret_val < 0)
    {
        ALOGD("NFC_GetP61Status failed");
        goto TheEnd;
    }
    ALOGD("P61 Status is: %x", p61_current_state);
#if(NFC_NXP_ESE_VER == JCOP_VER_3_1)
    if (!(p61_current_state & P61_STATE_SPI))
    {
#endif
    if(p61_current_state & (P61_STATE_SPI)||(p61_current_state & (P61_STATE_SPI_PRIO)))
    {
        dual_mode_current_state |= SPI_ON;
    }
    if(p61_current_state & (P61_STATE_SPI_PRIO))
    {
        hold_the_transceive = true;
    }

    secElemHandle = NFC_ReqWiredAccess ((void *)&status);
    if (secElemHandle < 0)
    {
        ALOGD("Denying SE open due to NFC_ReqWiredAccess failed");
        goto TheEnd;
    }
    else
    {
        if (status != NFCSTATUS_SUCCESS)
        {
            ALOGD("Denying SE open due to SE is being used by SPI");
            secElemHandle = EE_ERROR_IO;
            goto TheEnd;
        }
        else
        {
            is_wired_mode_open = true;
            ALOGD("SE Access granted");
        }
    }
#if(NFC_NXP_ESE_VER == JCOP_VER_3_1)
    }
    else
    {
        ALOGD("Denying SE open because SPI is already open");
        goto TheEnd;

    }
#endif
#endif
    //tell the controller to power up to get ready for sec elem operations
    PowerSwitch::getInstance ().setLevel (PowerSwitch::FULL_POWER);
    PowerSwitch::getInstance ().setModeOn (PowerSwitch::SE_CONNECTED);
#if 0
    {

        sRfEnabled = isDiscoveryStarted();
        if (sRfEnabled) {
            // Stop RF Discovery if we were polling
            startRfDiscovery (false);
        }

        UINT8 param[] = {0x00}; //Disable standby
        SyncEventGuard guard (sNfaVSCResponseEvent);
        tNFA_STATUS stat = NFA_SendVsCommand (0x00,0x01,param,nfaVSCCallback);
        if(NFA_STATUS_OK == stat)
        {
            sNfaVSCResponseEvent.wait(); //wait for NFA VS command to finish

        }

        startRfDiscovery (true);
    }
#endif

        stat = se.activate(SecureElement::ESE_ID);

    if (stat)
    {
        //establish a pipe to sec elem
        stat = se.connectEE();
        if (stat)
        {
            secElemHandle = se.mActiveEeHandle;
        }
        else
        {
            se.deactivate (0);
        }
    }

    //if code fails to connect to the secure element, and nothing is active, then
    //tell the controller to power down
    if ((!stat) && (! PowerSwitch::getInstance ().setModeOff (PowerSwitch::SE_CONNECTED)))
    {
        PowerSwitch::getInstance ().setLevel (PowerSwitch::LOW_POWER);
    }

TheEnd:
    ALOGD("%s: exit; return handle=0x%X", __FUNCTION__, secElemHandle);
    return secElemHandle;
}


/*******************************************************************************
**
** Function:        nativeNfcSecureElement_doDisconnectSecureElementConnection
**
** Description:     Disconnect from the secure element.
**                  e: JVM environment.
**                  o: Java object.
**                  handle: Handle of secure element.
**
** Returns:         True if ok.
**
*******************************************************************************/
static jboolean nativeNfcSecureElement_doDisconnectSecureElementConnection (JNIEnv*, jobject, jint handle)
{
    ALOGD("%s: enter; handle=0x%04x", __FUNCTION__, handle);
    bool stat = false;
#if((NFC_NXP_ESE == TRUE)&&(NXP_EXTNS == TRUE))
    long ret_val = -1;
    NFCSTATUS status = NFCSTATUS_FAILED;
#endif

    //Send the EVT_END_OF_APDU_TRANSFER event at the end of wired mode session.
    stat = SecureElement::getInstance().sendEvent(SecureElement::EVT_END_OF_APDU_TRANSFER);

    if(stat == false)
        goto TheEnd;
#if 0
    {
        sRfEnabled = isDiscoveryStarted();
        if (sRfEnabled) {
            // Stop RF Discovery if we were polling
            startRfDiscovery (false);
        }

        UINT8 param[] = {0x01};//Enable standby
        SyncEventGuard guard (sNfaVSCResponseEvent);
        tNFA_STATUS stat = NFA_SendVsCommand (0x00,0x01,param,nfaVSCCallback);
        if(NFA_STATUS_OK == stat)
        {
            sNfaVSCResponseEvent.wait(); //wait for NFA VS command to finish

        }

        startRfDiscovery (true);
    }
#endif
    stat = SecureElement::getInstance().disconnectEE (handle);

    //if controller is not routing AND there is no pipe connected,
    //then turn off the sec elem
#if(NFC_NXP_ESE == TRUE)
//Do Nothing
#else
    if (! SecureElement::getInstance().isBusy())
        SecureElement::getInstance().deactivate (handle);
#endif
    //if nothing is active after this, then tell the controller to power down
    if (! PowerSwitch::getInstance ().setModeOff (PowerSwitch::SE_CONNECTED))
        PowerSwitch::getInstance ().setLevel (PowerSwitch::LOW_POWER);
#if((NFC_NXP_ESE == TRUE)&&(NXP_EXTNS == TRUE))
    ret_val = NFC_RelWiredAccess ((void *)&status);
    if (ret_val < 0)
    {
        ALOGD("Denying SE Release due to NFC_RelWiredAccess failed");
        goto TheEnd;
    }
    else
    {
        if (status != NFCSTATUS_SUCCESS)
        {
            ALOGD("Denying SE close due to SE is not being released by Pn54x driver");
            stat = false;
        }
        is_wired_mode_open = false;
    }
#endif
TheEnd:
#if((NFC_NXP_ESE == TRUE)&&(NXP_EXTNS == TRUE))
    ALOGD("%s: exit stat = %d", __FUNCTION__, stat);
#else
    ALOGD("%s: exit", __FUNCTION__);
#endif
    return stat ? JNI_TRUE : JNI_FALSE;
}
#if((NFC_NXP_ESE == TRUE)&&(NXP_EXTNS == TRUE))
static int checkP61Status(void)
{
    jint ret_val = -1;
    p61_access_state_t p61_current_state = P61_STATE_INVALID;
    ret_val = NFC_GetP61Status ((void *)&p61_current_state);
    if (ret_val < 0)
    {
        ALOGD("NFC_GetP61Status failed");
        return -1;
    }
    if(p61_current_state & (P61_STATE_SPI)||(p61_current_state & (P61_STATE_SPI_PRIO)))
    {
        ALOGD("No gpio change");
        ret_val = 0;
    }
    else
    {
        ret_val = -1;
    }
    return ret_val;
}
#endif
/*******************************************************************************
**
** Function:        nativeNfcSecureElement_doResetSecureElement
**
** Description:     Reset the secure element.
**                  e: JVM environment.
**                  o: Java object.
**                  handle: Handle of secure element.
**
** Returns:         True if ok.
**
*******************************************************************************/
static jboolean nativeNfcSecureElement_doResetSecureElement (JNIEnv*, jobject, jint handle)
{
    bool stat = false;
#if (NFC_NXP_ESE == TRUE)
    SecureElement &se = SecureElement::getInstance();
    ALOGD("%s: enter; handle=0x%04x", __FUNCTION__, handle);
    if(!is_wired_mode_open)
    {
        ALOGD("wired mode is not open");
        return stat;
    }

    stat = se.SecEle_Modeset(0x00);
    if (handle == 0x4C0)
    {
        if(checkP61Status())
            se.NfccStandByOperation(STANDBY_GPIO_LOW);
    }
    usleep(100 * 1000);
    if (handle == 0x4C0)
    {
        if(checkP61Status() && (is_wired_mode_open == true))
            se.NfccStandByOperation(STANDBY_GPIO_HIGH);
    }
    stat = se.SecEle_Modeset(0x01);
    usleep(2000 * 1000);

#endif
    ALOGD("%s: exit", __FUNCTION__);
    return stat ? JNI_TRUE : JNI_FALSE;
}

/*******************************************************************************
**
** Function:        nativeNfcSecureElement_doGetAtr
**
** Description:     GetAtr from the connected eSE.
**                  e: JVM environment.
**                  o: Java object.
**                  handle: Handle of secure element.
**
** Returns:         Buffer of received data.
**
*******************************************************************************/
static jbyteArray nativeNfcSecureElement_doGetAtr (JNIEnv* e, jobject, jint handle)
{
    bool stat = false;
    const INT32 recvBufferMaxSize = 1024;
    UINT8 recvBuffer [recvBufferMaxSize];
    INT32 recvBufferActualSize = 0;
#if (NFC_NXP_ESE == TRUE)
    ALOGD("%s: enter; handle=0x%04x", __FUNCTION__, handle);

    stat = SecureElement::getInstance().getAtr(handle, recvBuffer, &recvBufferActualSize);

    //copy results back to java
#endif
    jbyteArray result = e->NewByteArray(recvBufferActualSize);
    if (result != NULL) {
        e->SetByteArrayRegion(result, 0, recvBufferActualSize, (jbyte *) recvBuffer);
    }

    ALOGD("%s: exit: recv len=%ld", __FUNCTION__, recvBufferActualSize);

    return result;
}

/*******************************************************************************
**
** Function:        nativeNfcSecureElement_doTransceive
**
** Description:     Send data to the secure element; retrieve response.
**                  e: JVM environment.
**                  o: Java object.
**                  handle: Secure element's handle.
**                  data: Data to send.
**
** Returns:         Buffer of received data.
**
*******************************************************************************/
static jbyteArray nativeNfcSecureElement_doTransceive (JNIEnv* e, jobject, jint handle, jbyteArray data)
{
    const INT32 recvBufferMaxSize = 0x8800;//1024; 34k
    UINT8 recvBuffer [recvBufferMaxSize];
    INT32 recvBufferActualSize = 0;

    ScopedByteArrayRW bytes(e, data);
#if(NXP_EXTNS == TRUE)
    ALOGD("%s: enter; handle=0x%X; buf len=%zu", __FUNCTION__, handle, bytes.size());
    SecureElement::getInstance().transceive(reinterpret_cast<UINT8*>(&bytes[0]), bytes.size(), recvBuffer, recvBufferMaxSize, recvBufferActualSize, gGeneralTransceiveTimeout);

    //copy results back to java
    jbyteArray result = e->NewByteArray(recvBufferActualSize);
    if (result != NULL)
         {
        e->SetByteArrayRegion(result, 0, recvBufferActualSize, (jbyte *) recvBuffer);
         }
    ALOGD("%s: exit: recv len=%ld", __FUNCTION__, recvBufferActualSize);
    return result;
#else
    jbyteArray result = e->NewByteArray(0);
    return result;
#endif
}

/*****************************************************************************
**
** Description:     JNI functions
**
*****************************************************************************/
static JNINativeMethod gMethods[] =
{
   {"doNativeOpenSecureElementConnection", "()I", (void *) nativeNfcSecureElement_doOpenSecureElementConnection},
   {"doNativeDisconnectSecureElementConnection", "(I)Z", (void *) nativeNfcSecureElement_doDisconnectSecureElementConnection},
   {"doNativeResetSecureElement", "(I)Z", (void *) nativeNfcSecureElement_doResetSecureElement},
   {"doTransceive", "(I[B)[B", (void *) nativeNfcSecureElement_doTransceive},
   {"doNativeGetAtr", "(I)[B", (void *) nativeNfcSecureElement_doGetAtr},
};


/*******************************************************************************
**
** Function:        register_com_android_nfc_NativeNfcSecureElement
**
** Description:     Regisgter JNI functions with Java Virtual Machine.
**                  e: Environment of JVM.
**
** Returns:         Status of registration.
**
*******************************************************************************/
int register_com_android_nfc_NativeNfcSecureElement(JNIEnv *e)
{
    return jniRegisterNativeMethods(e, gNativeNfcSecureElementClassName,
            gMethods, NELEM(gMethods));
}


} // namespace android
