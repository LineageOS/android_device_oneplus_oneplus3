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
#pragma once
#undef LOG_TAG
#define LOG_TAG "BrcmNfcJni"
#include <JNIHelp.h>
#include <jni.h>
#include <pthread.h>
#include <sys/queue.h>
#include <semaphore.h>


/* Discovery modes -- keep in sync with NFCManager.DISCOVERY_MODE_* */
#define DISCOVERY_MODE_TAG_READER         0
#define DISCOVERY_MODE_NFCIP1             1
#define DISCOVERY_MODE_CARD_EMULATION     2
#define DISCOVERY_MODE_TABLE_SIZE         3

#define DISCOVERY_MODE_DISABLED           0
#define DISCOVERY_MODE_ENABLED            1

#define MODE_P2P_TARGET                   0
#define MODE_P2P_INITIATOR                1


/* Properties values */
#define PROPERTY_LLCP_LTO                 0
#define PROPERTY_LLCP_MIU                 1
#define PROPERTY_LLCP_WKS                 2
#define PROPERTY_LLCP_OPT                 3
#define PROPERTY_NFC_DISCOVERY_A          4
#define PROPERTY_NFC_DISCOVERY_B          5
#define PROPERTY_NFC_DISCOVERY_F          6
#define PROPERTY_NFC_DISCOVERY_15693      7
#define PROPERTY_NFC_DISCOVERY_NCFIP      8


/* Error codes */
#define ERROR_BUFFER_TOO_SMALL            -12
#define ERROR_INSUFFICIENT_RESOURCES      -9


/* Pre-defined tag type values. These must match the values in
 * Ndef.java in the framework.
 */
#define NDEF_UNKNOWN_TYPE                -1
#define NDEF_TYPE1_TAG                   1
#define NDEF_TYPE2_TAG                   2
#define NDEF_TYPE3_TAG                   3
#define NDEF_TYPE4_TAG                   4
#define NDEF_MIFARE_CLASSIC_TAG          101


/* Pre-defined card read/write state values. These must match the values in
 * Ndef.java in the framework.
 */
#define NDEF_MODE_READ_ONLY              1
#define NDEF_MODE_READ_WRITE             2
#define NDEF_MODE_UNKNOWN                3

#if(NXP_EXTNS == TRUE)
#define VEN_POWER_STATE_ON                   6
#define VEN_POWER_STATE_OFF                  7
// ESE Suppored Technologies
#define TARGET_TYPE_ISO14443_3A_3B        11
#endif
/* Name strings for target types. These *must* match the values in TagTechnology.java */
#define TARGET_TYPE_UNKNOWN               -1
#define TARGET_TYPE_ISO14443_3A           1
#define TARGET_TYPE_ISO14443_3B           2
#define TARGET_TYPE_ISO14443_4            3
#define TARGET_TYPE_FELICA                4
#define TARGET_TYPE_ISO15693              5
#define TARGET_TYPE_NDEF                  6
#define TARGET_TYPE_NDEF_FORMATABLE       7
#define TARGET_TYPE_MIFARE_CLASSIC        8
#define TARGET_TYPE_MIFARE_UL             9
#define TARGET_TYPE_KOVIO_BARCODE         10
#define TARGET_TYPE_ISO14443_4A           11
#define TARGET_TYPE_ISO14443_4B           12

/* Setting VEN_CFG  */
#define NFC_MODE_ON           3
#define NFC_MODE_OFF          2

//define a few NXP error codes that NFC service expects;
//see external/libnfc-nxp/src/phLibNfcStatus.h;
//see external/libnfc-nxp/inc/phNfcStatus.h
#define NFCSTATUS_SUCCESS (0x0000)
#define NFCSTATUS_FAILED (0x00FF)

//default general trasceive timeout in millisecond
#define DEFAULT_GENERAL_TRANS_TIMEOUT  2000

struct nfc_jni_native_data
{
   /* Thread handle */
   pthread_t thread;
   int running;

   /* Our VM */
   JavaVM *vm;
   int env_version;

   /* Reference to the NFCManager instance */
   jobject manager;

   /* Cached objects */
   jobject cached_NfcTag;
   jobject cached_P2pDevice;

   /* Secure Element selected */
   int seId;

   /* LLCP params */
   int lto;
   int miu;
   int wks;
   int opt;

   int tech_mask;
   int discovery_duration;

   /* Tag detected */
   jobject tag;

   int tHandle;
   int tProtocols[16];
   int handles[16];
};


class ScopedAttach
{
public:
    ScopedAttach(JavaVM* vm, JNIEnv** env) : vm_(vm)
    {
        vm_->AttachCurrentThread(env, NULL);
    }

    ~ScopedAttach()
    {
        vm_->DetachCurrentThread();
    }

private:
        JavaVM* vm_;
};


extern "C"
{
    jint JNI_OnLoad(JavaVM *jvm, void *reserved);
}


namespace android
{
    int nfc_jni_cache_object (JNIEnv *e, const char *clsname, jobject *cached_obj);
    int nfc_jni_cache_object_local (JNIEnv *e, const char *className, jobject *cachedObj);
    int nfc_jni_get_nfc_socket_handle (JNIEnv *e, jobject o);
    struct nfc_jni_native_data* nfc_jni_get_nat (JNIEnv *e, jobject o);
    int register_com_android_nfc_NativeNfcManager (JNIEnv *e);
    int register_com_android_nfc_NativeNfcTag (JNIEnv *e);
    int register_com_android_nfc_NativeP2pDevice (JNIEnv *e);
    int register_com_android_nfc_NativeLlcpConnectionlessSocket (JNIEnv *e);
    int register_com_android_nfc_NativeLlcpServiceSocket (JNIEnv *e);
    int register_com_android_nfc_NativeLlcpSocket (JNIEnv *e);
    int register_com_android_nfc_NativeNfcSecureElement (JNIEnv *e);
    int register_com_android_nfc_NativeNfcAla(JNIEnv *e);
} // namespace android
