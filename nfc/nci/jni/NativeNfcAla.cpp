/*
 * Copyright (C) 2015 NXP Semiconductors
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
#include "NfcJniUtil.h"
#include "PowerSwitch.h"
#include "JavaClassConstants.h"
#include "OverrideLog.h"
#include <ScopedPrimitiveArray.h>
#include "DwpChannel.h"

extern "C"
{
#if (NFC_NXP_ESE == TRUE)
    #include "AlaLib.h"
    #include "IChannel.h"
    #include "phNxpConfig.h"
#endif
}
#define LS_DEFAULT_VERSION 0x20
namespace android
{
extern SyncEvent            sNfaVSCResponseEvent;
extern void startRfDiscovery (bool isStart);
extern bool isDiscoveryStarted();
extern void nfaVSCCallback(UINT8 event, UINT16 param_len, UINT8 *p_param);
extern void set_transcation_stat(bool result);
}

namespace android
{

static bool sRfEnabled;
jbyteArray nfcManager_lsExecuteScript(JNIEnv* e, jobject o, jstring src, jstring dest, jbyteArray);
jbyteArray nfcManager_lsGetVersion(JNIEnv* e, jobject o);
int nfcManager_doAppletLoadApplet(JNIEnv* e, jobject o, jstring choice, jbyteArray);
int nfcManager_getLoaderServiceConfVersion(JNIEnv* e, jobject o);
int nfcManager_GetAppletsList(JNIEnv* e, jobject o, jobjectArray list);
jbyteArray nfcManager_GetCertificateKey(JNIEnv* e, jobject);
jbyteArray nfcManager_lsGetStatus(JNIEnv* e, jobject);
jbyteArray nfcManager_lsGetAppletStatus(JNIEnv* e, jobject);
#if (NFC_NXP_ESE == TRUE)
extern void DWPChannel_init(IChannel_t *DWP);
extern IChannel_t Dwp;
#endif

/*******************************************************************************
**
** Function:        nfcManager_GetListofApplets
**
** Description:     provides the list of applets present in the directory.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         None.
**
*******************************************************************************/
int nfcManager_GetAppletsList(JNIEnv* e, jobject o, jobjectArray list)
{
    (void)e;
    (void)o;
    (void)list;
#if (NFC_NXP_ESE == TRUE && NXP_LDR_SVC_VER_2 == FALSE)
    char *name[10];
    UINT8 num =0, xx=0;
    UINT8 list_len = e->GetArrayLength(list);
    ALOGD ("%s: enter", __FUNCTION__);

    sRfEnabled = isDiscoveryStarted();
    if (sRfEnabled) {
        // Stop RF Discovery if we were polling
        startRfDiscovery (false);
    }
    ALOGD("%s: list_len=0x%x", __FUNCTION__, list_len);
    ALA_GetlistofApplets(name, &num);

    if((num != 0) &&
       (list_len >= num))
    {
        while(num > 0)
        {
            jstring tmp = e->NewStringUTF(name[xx]);
            e->SetObjectArrayElement(list, xx, tmp);
            if(name[xx] != NULL)
            {
                free(name[xx]);
            }
            xx++;
            num--;
        }
    }
    else
    {
        ALOGE("%s: No applets found",__FUNCTION__);
    }
    startRfDiscovery (true);
#else
    int xx = -1;
    ALOGD ("%s: No p61", __FUNCTION__);
#endif
    ALOGD ("%s: exit; num_applets =0x%X", __FUNCTION__,xx);
    return xx;
}

/*******************************************************************************
**
** Function:        nfcManager_doAppletLoadApplet
**
** Description:     start jcos download.
**                  e: JVM environment.
**                  o: Java object.
**                  name: path of the ALA script file located
**                  data: SHA encrypted package name
**
** Returns:         True if ok.
**
*******************************************************************************/

int nfcManager_doAppletLoadApplet(JNIEnv* e, jobject o, jstring name, jbyteArray data)
{
    (void)e;
    (void)o;
    (void)name;
#if (NFC_NXP_ESE == TRUE && NXP_LDR_SVC_VER_2 == FALSE)
    ALOGD ("%s: enter", __FUNCTION__);
    tNFA_STATUS wStatus, status;
    IChannel_t Dwp;
    bool stat = false;
    const char *choice = NULL;

    sRfEnabled = isDiscoveryStarted();
    wStatus = status = NFA_STATUS_FAILED;

    set_transcation_stat(true);
    if (sRfEnabled) {
        // Stop RF Discovery if we were polling
        startRfDiscovery (false);
    }
    DWPChannel_init(&Dwp);
    wStatus = ALA_Init(&Dwp);
    if(wStatus != NFA_STATUS_OK)
    {
        ALOGE("%s: ALA initialization failed", __FUNCTION__);
    }
    else
    {
        ALOGE("%s: start Applet load applet", __FUNCTION__);
        choice = e->GetStringUTFChars(name, 0);
        ALOGE("choice= %s", choice);
        ScopedByteArrayRO bytes(e, data);
        uint8_t* buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&bytes[0]));
        size_t bufLen = bytes.size();
        wStatus = ALA_Start(choice, buf, bufLen);
    }
    stat = ALA_DeInit();
    if(choice != NULL)
        e->ReleaseStringUTFChars(name, choice);

    if(dwpChannelForceClose == false)
        startRfDiscovery (true);

    set_transcation_stat(false);

    ALOGD ("%s: exit; status =0x%X", __FUNCTION__,wStatus);
#else

    tNFA_STATUS wStatus = 0x0F;
    ALOGD ("%s: No p61", __FUNCTION__);
#endif
    return wStatus;

}

/*******************************************************************************
**
** Function:        nfcManager_lsExecuteScript
**
** Description:     start jcos download.
**                  e: JVM environment.
**                  o: Java object.
**                  name: path of the ALA script file located
**                  data: SHA encrypted package name
**
** Returns:         True if ok.
**
*******************************************************************************/
jbyteArray nfcManager_lsExecuteScript(JNIEnv* e, jobject o, jstring name, jstring dest, jbyteArray data)
{
    (void)e;
    (void)o;
    (void)name;
    (void)dest;
    const char *destpath = NULL;
    const UINT8 lsExecuteResponseSize = 4;
    uint8_t resSW [4]={0x4e,0x02,0x69,0x87};
    jbyteArray result = e->NewByteArray(0);
#if (NFC_NXP_ESE == TRUE && NXP_LDR_SVC_VER_2 == TRUE)
    ALOGD ("%s: enter", __FUNCTION__);
    tNFA_STATUS wStatus, status;
    IChannel_t Dwp;
    bool stat = false;
    const char *choice = NULL;

    sRfEnabled = isDiscoveryStarted();
    wStatus = status = NFA_STATUS_FAILED;

    set_transcation_stat(true);
    if (sRfEnabled) {
        // Stop RF Discovery if we were polling
        startRfDiscovery (false);
    }
    DWPChannel_init(&Dwp);
    wStatus = ALA_Init(&Dwp);
    if(wStatus != NFA_STATUS_OK)
    {
        ALOGE("%s: ALA initialization failed", __FUNCTION__);
    }
    else
    {
        // Commented the Disabling standby
        /* UINT8 param[] = {0x00}; //Disable standby
        SyncEventGuard guard (sNfaVSCResponseEvent);
        status = NFA_SendVsCommand (0x00,0x01,param,nfaVSCCallback);
        if(NFA_STATUS_OK == status)
        {
            sNfaVSCResponseEvent.wait(); //wait for NFA VS command to finish
            ALOGE("%s: start Applet load applet", __FUNCTION__);
            choice = e->GetStringUTFChars(name, 0);
            ALOGE("choice= %s", choice);
            wStatus = ALA_Start(choice);
        }*/
        destpath = e->GetStringUTFChars(dest, 0);
        ALOGE("destpath= %s", destpath);
        ALOGE("%s: start Applet load applet", __FUNCTION__);
        choice = e->GetStringUTFChars(name, 0);
        ALOGE("choice= %s", choice);
        ScopedByteArrayRO bytes(e, data);
        uint8_t* buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&bytes[0]));
        size_t bufLen = bytes.size();
        wStatus = ALA_Start(choice,destpath, buf, bufLen,resSW);

       //copy results back to java
       result = e->NewByteArray(lsExecuteResponseSize);
       if (result != NULL)
       {
           e->SetByteArrayRegion(result, 0, lsExecuteResponseSize, (jbyte *) resSW);
       }
    }

    // Commented the Enabling standby
    /* UINT8 param[] = {0x01}; //Enable standby
    SyncEventGuard guard (sNfaVSCResponseEvent);
    status = NFA_SendVsCommand (0x00,0x01,param,nfaVSCCallback);
    if(NFA_STATUS_OK == status)
    {
        sNfaVSCResponseEvent.wait(); //wait for NFA VS command to finish
    }*/

    stat = ALA_DeInit();
    if(choice != NULL)
        e->ReleaseStringUTFChars(name, choice);

    if(dwpChannelForceClose == false)
        startRfDiscovery (true);

    set_transcation_stat(false);

    ALOGD ("%s: exit; status =0x%X", __FUNCTION__,wStatus);
#else
    if(destpath != NULL)
        e->ReleaseStringUTFChars(dest, destpath);

    result = e->NewByteArray(0);
    tNFA_STATUS wStatus = 0x0F;
    ALOGD ("%s: No p61", __FUNCTION__);
#endif
    return result;
}

/*******************************************************************************
**
** Function:        nfcManager_GetCertificateKey
**
** Description:     It is ued to get the reference certificate key
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         key value.
**
*******************************************************************************/

jbyteArray nfcManager_GetCertificateKey(JNIEnv* e, jobject)
{
#if (NFC_NXP_ESE == TRUE && NXP_LDR_SVC_VER_2 == FALSE)
    ALOGD ("%s: enter", __FUNCTION__);
    tNFA_STATUS wStatus = NFA_STATUS_FAILED;
    IChannel_t Dwp;
    bool stat = false;
    const INT32 recvBufferMaxSize = 256;
    UINT8 recvBuffer [recvBufferMaxSize];
    INT32 recvBufferActualSize = 0;

    sRfEnabled = isDiscoveryStarted();

    if (sRfEnabled) {
        // Stop RF Discovery if we were polling
        startRfDiscovery (false);
    }
    DWPChannel_init(&Dwp);
    wStatus = ALA_Init(&Dwp);
    if(wStatus != NFA_STATUS_OK)
    {
        ALOGE("%s: ALA initialization failed", __FUNCTION__);
    }
    else
    {
        ALOGE("%s: start Get reference Certificate Key", __FUNCTION__);
        wStatus = ALA_GetCertificateKey(recvBuffer, &recvBufferActualSize);
    }

    //copy results back to java
    jbyteArray result = e->NewByteArray(recvBufferActualSize);
    if (result != NULL)
    {
        e->SetByteArrayRegion(result, 0, recvBufferActualSize, (jbyte *) recvBuffer);
    }

    stat = ALA_DeInit();
    startRfDiscovery (true);

    ALOGD("%s: exit: recv len=%ld", __FUNCTION__, recvBufferActualSize);
#else
    jbyteArray result = e->NewByteArray(0);
    ALOGD ("%s: No p61", __FUNCTION__);
#endif
    return result;
}

/*******************************************************************************
**
** Function:        nfcManager_lsGetVersion
**
** Description:     It is ued to get version of Loader service client & Applet
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         version of Loder service.
**
*******************************************************************************/
jbyteArray nfcManager_lsGetVersion(JNIEnv* e, jobject)
{

#if (NFC_NXP_ESE == TRUE && NXP_LDR_SVC_VER_2 == TRUE)
    ALOGD ("%s: enter", __FUNCTION__);
    tNFA_STATUS wStatus = NFA_STATUS_FAILED;
    IChannel_t Dwp;
    bool stat = false;
    const INT32 recvBufferMaxSize = 4;
    UINT8 recvBuffer [recvBufferMaxSize];

    sRfEnabled = isDiscoveryStarted();
    set_transcation_stat(true);
    if (sRfEnabled) {
        // Stop RF Discovery if we were polling
        startRfDiscovery (false);
    }
    DWPChannel_init(&Dwp);
    wStatus = ALA_Init(&Dwp);
    if(wStatus != NFA_STATUS_OK)
    {
        ALOGE("%s: ALA initialization failed", __FUNCTION__);
    }
    else
    {
        ALOGE("%s: start Get reference Certificate Key", __FUNCTION__);
        wStatus = ALA_lsGetVersion(recvBuffer);
    }

    //copy results back to java
    jbyteArray result = e->NewByteArray(recvBufferMaxSize);
    if (result != NULL)
    {
        e->SetByteArrayRegion(result, 0, recvBufferMaxSize, (jbyte *) recvBuffer);
    }

    stat = ALA_DeInit();
    if(dwpChannelForceClose == false)
        startRfDiscovery (true);

    set_transcation_stat(false);

    ALOGD("%s: exit: recv len=%ld", __FUNCTION__, recvBufferMaxSize);
#else
    jbyteArray result = e->NewByteArray(0);
    ALOGD ("%s: No p61", __FUNCTION__);
#endif
    return result;

}

/*******************************************************************************
**
** Function:        nfcManager_lsGetAppletStatus
**
** Description:     It is ued to get LS Applet status SUCCESS/FAIL
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         LS Previous execution Applet status .
**
*******************************************************************************/
jbyteArray nfcManager_lsGetAppletStatus(JNIEnv* e, jobject)
{

#if ((NFC_NXP_ESE == TRUE) && (NXP_LDR_SVC_VER_2 == TRUE))
    ALOGD ("%s: enter", __FUNCTION__);
    tNFA_STATUS wStatus = NFA_STATUS_FAILED;
    bool stat = false;
    const INT32 recvBufferMaxSize = 2;
    UINT8 recvBuffer [recvBufferMaxSize]={0x63,0x40};
    IChannel_t Dwp;
    sRfEnabled = isDiscoveryStarted();

    if (sRfEnabled) {
        // Stop RF Discovery if we were polling
        startRfDiscovery (false);
    }
    DWPChannel_init(&Dwp);
    wStatus = ALA_Init(&Dwp);
    if(wStatus != NFA_STATUS_OK)
    {
        ALOGE("%s: ALA initialization failed", __FUNCTION__);
    }
    else
    {
        ALOGE("%s: start Get reference Certificate Key", __FUNCTION__);
        wStatus = ALA_lsGetAppletStatus(recvBuffer);
    }

    ALOGD ("%s: lsGetAppletStatus values %x %x", __FUNCTION__, recvBuffer[0], recvBuffer[1]);
    //copy results back to java
    jbyteArray result = e->NewByteArray(recvBufferMaxSize);
    if (result != NULL)
    {
        e->SetByteArrayRegion(result, 0, recvBufferMaxSize, (jbyte *) recvBuffer);
    }
    stat = ALA_DeInit();

    if(dwpChannelForceClose == false)
        startRfDiscovery (true);

    ALOGD("%s: exit: recv len=%ld", __FUNCTION__, recvBufferMaxSize);
#else
    jbyteArray result = e->NewByteArray(0);
    ALOGD ("%s: No p61", __FUNCTION__);
#endif
    return result;
}

/*******************************************************************************
**
** Function:        nfcManager_lsGetStatus
**
** Description:     It is ued to get LS Client status SUCCESS/FAIL
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         version of Loder service.
**
*******************************************************************************/
jbyteArray nfcManager_lsGetStatus(JNIEnv* e, jobject)
{

#if ((NFC_NXP_ESE == TRUE) && (NXP_LDR_SVC_VER_2 == TRUE))
    ALOGD ("%s: enter", __FUNCTION__);
    tNFA_STATUS wStatus = NFA_STATUS_FAILED;
    const INT32 recvBufferMaxSize = 2;
    UINT8 recvBuffer [recvBufferMaxSize] = {0x63,0x40};

    wStatus = ALA_lsGetStatus(recvBuffer);

    ALOGD ("%s: lsGetStatus values %x %x", __FUNCTION__, recvBuffer[0], recvBuffer[1]);
    //copy results back to java
    jbyteArray result = e->NewByteArray(recvBufferMaxSize);
    if (result != NULL)
    {
        e->SetByteArrayRegion(result, 0, recvBufferMaxSize, (jbyte *) recvBuffer);
    }
    ALOGD("%s: exit: recv len=%ld", __FUNCTION__, recvBufferMaxSize);
#else
    jbyteArray result = e->NewByteArray(0);
    ALOGD ("%s: No p61", __FUNCTION__);
#endif
    return result;
}

/*******************************************************************************
**
** Function:        nfcManager_getLoaderServiceConfVersion
**
** Description:     Get current Loader service version from config file.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         Void.
**
*******************************************************************************/
int nfcManager_getLoaderServiceConfVersion(JNIEnv* e, jobject o)
{
    unsigned long num = 0;
    UINT8 ls_version = LS_DEFAULT_VERSION;
    ALOGD ("%s: enter", __FUNCTION__);
#if ((NFC_NXP_ESE == TRUE) && (NXP_LDR_SVC_VER_2 == TRUE))
    if(GetNxpNumValue (NAME_NXP_LOADER_SERICE_VERSION, (void*)&num, sizeof(num))==false)
    {
        ALOGD ("LOADER_SERVICE_VERSION not found");
        num = 0;
    }
    /*If LS version exists in config file*/
    if(num != 0)
    {
        ls_version = num;
    }
#else
    ALOGD ("%s: No P61", __FUNCTION__);
#endif
    ALOGD ("%s: exit", __FUNCTION__);
    return ls_version;
}

/*****************************************************************************
 **
 ** Description:     JNI functions
 **
 *****************************************************************************/
static JNINativeMethod gMethods[] =
{
    {"doLsExecuteScript","(Ljava/lang/String;Ljava/lang/String;[B)[B",
                (void *)nfcManager_lsExecuteScript},
    {"doLsGetVersion","()[B",
                (void *)nfcManager_lsGetVersion},
    {"doLsGetStatus","()[B",
      (void *)nfcManager_lsGetStatus},
    {"doLsGetAppletStatus","()[B",
      (void *)nfcManager_lsGetAppletStatus},
    {"doGetLSConfigVersion", "()I",
       (void *)nfcManager_getLoaderServiceConfVersion},

    {"GetAppletsList", "([Ljava/lang/String;)I",
                (void *)nfcManager_GetAppletsList},

    {"doAppletLoadApplet", "(Ljava/lang/String;[B)I",
                (void *)nfcManager_doAppletLoadApplet},

    {"GetCertificateKey", "()[B",
                (void *)nfcManager_GetCertificateKey},
};

/*******************************************************************************
 **
 ** Function:        register_com_android_nfc_NativeNfcAla
 **
 ** Description:     Regisgter JNI functions with Java Virtual Machine.
 **                  e: Environment of JVM.
 **
 ** Returns:         Status of registration.
 **
 *******************************************************************************/
int register_com_android_nfc_NativeNfcAla(JNIEnv *e)
{
    return jniRegisterNativeMethods(e, gNativeNfcAlaClassName,
            gMethods, NELEM(gMethods));
}
} /*namespace android*/
