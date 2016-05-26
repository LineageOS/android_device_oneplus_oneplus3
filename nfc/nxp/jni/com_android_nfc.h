/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef __COM_ANDROID_NFC_JNI_H__
#define __COM_ANDROID_NFC_JNI_H__

#undef LOG_TAG
#define LOG_TAG "NFCJNI"

#include <JNIHelp.h>
#include <jni.h>
#include <ScopedLocalRef.h>

#include <pthread.h>
#include <sys/queue.h>

extern "C" {
#include <phNfcStatus.h>
#include <phNfcTypes.h>
#include <phNfcIoctlCode.h>
#include <phLibNfc.h>
#include <phDal4Nfc_messageQueueLib.h>
#include <phFriNfc_NdefMap.h>
#include <cutils/log.h>
#include <com_android_nfc_list.h>
#include <semaphore.h>

}
#include <cutils/properties.h> // for property_get


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

/* Pre-defined card read/write state values. These must match the values in
 * Ndef.java in the framework.
 */

#define NDEF_UNKNOWN_TYPE                -1
#define NDEF_TYPE1_TAG                   1
#define NDEF_TYPE2_TAG                   2
#define NDEF_TYPE3_TAG                   3
#define NDEF_TYPE4_TAG                   4
#define NDEF_MIFARE_CLASSIC_TAG          101
#define NDEF_ICODE_SLI_TAG               102

/* Pre-defined tag type values. These must match the values in
 * Ndef.java in the framework.
 */

#define NDEF_MODE_READ_ONLY              1
#define NDEF_MODE_READ_WRITE             2
#define NDEF_MODE_UNKNOWN                3


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

#define SMX_SECURE_ELEMENT_ID   11259375


/* These must match the EE_ERROR_ types in NfcService.java */
#define EE_ERROR_IO                 -1
#define EE_ERROR_ALREADY_OPEN       -2
#define EE_ERROR_INIT               -3
#define EE_ERROR_LISTEN_MODE        -4
#define EE_ERROR_EXT_FIELD          -5
#define EE_ERROR_NFC_DISABLED       -6

/* Maximum byte length of an AID. */
#define AID_MAXLEN                        16

/* Utility macros for logging */
#define GET_LEVEL(status) ((status)==NFCSTATUS_SUCCESS)?ANDROID_LOG_DEBUG:ANDROID_LOG_WARN

#if 0
  #define LOG_CALLBACK(funcName, status)  LOG_PRI(GET_LEVEL(status), LOG_TAG, "Callback: %s() - status=0x%04x[%s]", funcName, status, nfc_jni_get_status_name(status));
  #define TRACE(...) ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__)
  #define TRACE_ENABLED 1
#else
  #define LOG_CALLBACK(...)
  #define TRACE(...)
  #define TRACE_ENABLED 0
#endif

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

   /* Target discovery configuration */
   int discovery_modes_state[DISCOVERY_MODE_TABLE_SIZE];
   phLibNfc_sADD_Cfg_t discovery_cfg;
   phLibNfc_Registry_Info_t registry_info;

   /* Secure Element selected */
   int seId;

   /* LLCP params */
   int lto;
   int miu;
   int wks;
   int opt;

   /* Tag detected */
   jobject tag;

   /* Lib Status */
   NFCSTATUS status;

   /* p2p modes */
   int p2p_initiator_modes;
   int p2p_target_modes;

};

typedef struct nfc_jni_native_monitor
{
   /* Mutex protecting native library against reentrance */
   pthread_mutex_t reentrance_mutex;

   /* Mutex protecting native library against concurrency */
   pthread_mutex_t concurrency_mutex;

   /* List used to track pending semaphores waiting for callback */
   struct listHead sem_list;

   /* List used to track incoming socket requests (and associated sync variables) */
   LIST_HEAD(, nfc_jni_listen_data) incoming_socket_head;
   pthread_mutex_t incoming_socket_mutex;
   pthread_cond_t  incoming_socket_cond;

} nfc_jni_native_monitor_t;

typedef struct nfc_jni_callback_data
{
   /* Semaphore used to wait for callback */
   sem_t sem;

   /* Used to store the status sent by the callback */
   NFCSTATUS status;

   /* Used to provide a local context to the callback */
   void* pContext;

} nfc_jni_callback_data_t;

typedef struct nfc_jni_listen_data
{
   /* LLCP server socket receiving the connection request */
   phLibNfc_Handle pServerSocket;

   /* LLCP socket created from the connection request */
   phLibNfc_Handle pIncomingSocket;

   /* List entries */
   LIST_ENTRY(nfc_jni_listen_data) entries;

} nfc_jni_listen_data_t;

/* TODO: treat errors and add traces */
#define REENTRANCE_LOCK()        pthread_mutex_lock(&nfc_jni_get_monitor()->reentrance_mutex)
#define REENTRANCE_UNLOCK()      pthread_mutex_unlock(&nfc_jni_get_monitor()->reentrance_mutex)
#define CONCURRENCY_LOCK()       pthread_mutex_lock(&nfc_jni_get_monitor()->concurrency_mutex)
#define CONCURRENCY_UNLOCK()     pthread_mutex_unlock(&nfc_jni_get_monitor()->concurrency_mutex)

namespace android {

extern JavaVM *vm;

JNIEnv *nfc_get_env();

bool nfc_cb_data_init(nfc_jni_callback_data* pCallbackData, void* pContext);
void nfc_cb_data_deinit(nfc_jni_callback_data* pCallbackData);
void nfc_cb_data_releaseAll();

const char* nfc_jni_get_status_name(NFCSTATUS status);
int nfc_jni_cache_object(JNIEnv *e, const char *clsname,
   jobject *cached_obj);
struct nfc_jni_native_data* nfc_jni_get_nat(JNIEnv *e, jobject o);
struct nfc_jni_native_data* nfc_jni_get_nat_ext(JNIEnv *e);
nfc_jni_native_monitor_t* nfc_jni_init_monitor(void);
nfc_jni_native_monitor_t* nfc_jni_get_monitor(void);

int get_technology_type(phNfc_eRemDevType_t type, uint8_t sak);
void nfc_jni_get_technology_tree(JNIEnv* e, phLibNfc_RemoteDevList_t* devList, uint8_t count,
                        ScopedLocalRef<jintArray>* techList,
                        ScopedLocalRef<jintArray>* handleList,
                        ScopedLocalRef<jintArray>* typeList);

/* P2P */
phLibNfc_Handle nfc_jni_get_p2p_device_handle(JNIEnv *e, jobject o);
jshort nfc_jni_get_p2p_device_mode(JNIEnv *e, jobject o);

/* TAG */
jint nfc_jni_get_connected_technology(JNIEnv *e, jobject o);
jint nfc_jni_get_connected_technology_libnfc_type(JNIEnv *e, jobject o);
phLibNfc_Handle nfc_jni_get_connected_handle(JNIEnv *e, jobject o);
jintArray nfc_jni_get_nfc_tag_type(JNIEnv *e, jobject o);

/* LLCP */
phLibNfc_Handle nfc_jni_get_nfc_socket_handle(JNIEnv *e, jobject o);

int register_com_android_nfc_NativeNfcManager(JNIEnv *e);
int register_com_android_nfc_NativeNfcTag(JNIEnv *e);
int register_com_android_nfc_NativeP2pDevice(JNIEnv *e);
int register_com_android_nfc_NativeLlcpConnectionlessSocket(JNIEnv *e);
int register_com_android_nfc_NativeLlcpServiceSocket(JNIEnv *e);
int register_com_android_nfc_NativeLlcpSocket(JNIEnv *e);
int register_com_android_nfc_NativeNfcSecureElement(JNIEnv *e);

} // namespace android

#endif
