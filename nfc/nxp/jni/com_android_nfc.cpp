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

#include <stdlib.h>

#include "errno.h"
#include "com_android_nfc.h"
#include "com_android_nfc_list.h"
#include "phLibNfcStatus.h"
#include <ScopedLocalRef.h>

/*
 * JNI Initialization
 */
jint JNI_OnLoad(JavaVM *jvm, void *reserved)
{
   JNIEnv *e;

   ALOGI("NFC Service: loading nxp JNI");

   // Check JNI version
   if(jvm->GetEnv((void **)&e, JNI_VERSION_1_6))
      return JNI_ERR;

   android::vm = jvm;

   if (android::register_com_android_nfc_NativeNfcManager(e) == -1)
      return JNI_ERR;
   if (android::register_com_android_nfc_NativeNfcTag(e) == -1)
      return JNI_ERR;
   if (android::register_com_android_nfc_NativeP2pDevice(e) == -1)
      return JNI_ERR;
   if (android::register_com_android_nfc_NativeLlcpSocket(e) == -1)
      return JNI_ERR;
   if (android::register_com_android_nfc_NativeLlcpConnectionlessSocket(e) == -1)
      return JNI_ERR;
   if (android::register_com_android_nfc_NativeLlcpServiceSocket(e) == -1)
      return JNI_ERR;
   if (android::register_com_android_nfc_NativeNfcSecureElement(e) == -1)
      return JNI_ERR;

   return JNI_VERSION_1_6;
}

namespace android {

extern struct nfc_jni_native_data *exported_nat;

JavaVM *vm;

/*
 * JNI Utils
 */
JNIEnv *nfc_get_env()
{
    JNIEnv *e;
    if (vm->GetEnv((void **)&e, JNI_VERSION_1_6) != JNI_OK) {
        ALOGE("Current thread is not attached to VM");
        phLibNfc_Mgt_Recovery();
        abort();
    }
    return e;
}

bool nfc_cb_data_init(nfc_jni_callback_data* pCallbackData, void* pContext)
{
   /* Create semaphore */
   if(sem_init(&pCallbackData->sem, 0, 0) == -1)
   {
      ALOGE("Semaphore creation failed (errno=0x%08x)", errno);
      return false;
   }

   /* Set default status value */
   pCallbackData->status = NFCSTATUS_FAILED;

   /* Copy the context */
   pCallbackData->pContext = pContext;

   /* Add to active semaphore list */
   if (!listAdd(&nfc_jni_get_monitor()->sem_list, pCallbackData))
   {
      ALOGE("Failed to add the semaphore to the list");
   }

   return true;
}

void nfc_cb_data_deinit(nfc_jni_callback_data* pCallbackData)
{
   /* Destroy semaphore */
   if (sem_destroy(&pCallbackData->sem))
   {
      ALOGE("Failed to destroy semaphore (errno=0x%08x)", errno);
   }

   /* Remove from active semaphore list */
   if (!listRemove(&nfc_jni_get_monitor()->sem_list, pCallbackData))
   {
      ALOGE("Failed to remove semaphore from the list");
   }

}

void nfc_cb_data_releaseAll()
{
   nfc_jni_callback_data* pCallbackData;

   while (listGetAndRemoveNext(&nfc_jni_get_monitor()->sem_list, (void**)&pCallbackData))
   {
      pCallbackData->status = NFCSTATUS_FAILED;
      sem_post(&pCallbackData->sem);
   }
}

int nfc_jni_cache_object(JNIEnv *e, const char *clsname,
   jobject *cached_obj)
{
   ScopedLocalRef<jclass> cls(e, e->FindClass(clsname));
   if (cls.get() == NULL) {
      ALOGD("Find class error\n");
      return -1;
   }

   jmethodID ctor = e->GetMethodID(cls.get(), "<init>", "()V");
   ScopedLocalRef<jobject> obj(e, e->NewObject(cls.get(), ctor));
   if (obj.get() == NULL) {
      ALOGD("Create object error\n");
      return -1;
   }

   *cached_obj = e->NewGlobalRef(obj.get());
   if (*cached_obj == NULL) {
      ALOGD("Global ref error\n");
      return -1;
   }

   return 0;
}


struct nfc_jni_native_data* nfc_jni_get_nat(JNIEnv *e, jobject o)
{
   /* Retrieve native structure address */
   ScopedLocalRef<jclass> c(e, e->GetObjectClass(o));
   jfieldID f = e->GetFieldID(c.get(), "mNative", "I");
   return (struct nfc_jni_native_data*) e->GetIntField(o, f);
}

struct nfc_jni_native_data* nfc_jni_get_nat_ext(JNIEnv *e)
{
   return exported_nat;
}

static nfc_jni_native_monitor_t *nfc_jni_native_monitor = NULL;

nfc_jni_native_monitor_t* nfc_jni_init_monitor(void)
{

   pthread_mutexattr_t recursive_attr;

   pthread_mutexattr_init(&recursive_attr);
   pthread_mutexattr_settype(&recursive_attr, PTHREAD_MUTEX_RECURSIVE_NP);

   if(nfc_jni_native_monitor == NULL)
   {
      nfc_jni_native_monitor = (nfc_jni_native_monitor_t*)malloc(sizeof(nfc_jni_native_monitor_t));
   }

   if(nfc_jni_native_monitor != NULL)
   {
      memset(nfc_jni_native_monitor, 0, sizeof(nfc_jni_native_monitor_t));

      if(pthread_mutex_init(&nfc_jni_native_monitor->reentrance_mutex, &recursive_attr) == -1)
      {
         ALOGE("NFC Manager Reentrance Mutex creation returned 0x%08x", errno);
         return NULL;
      }

      if(pthread_mutex_init(&nfc_jni_native_monitor->concurrency_mutex, NULL) == -1)
      {
         ALOGE("NFC Manager Concurrency Mutex creation returned 0x%08x", errno);
         return NULL;
      }

      if(!listInit(&nfc_jni_native_monitor->sem_list))
      {
         ALOGE("NFC Manager Semaphore List creation failed");
         return NULL;
      }

      LIST_INIT(&nfc_jni_native_monitor->incoming_socket_head);

      if(pthread_mutex_init(&nfc_jni_native_monitor->incoming_socket_mutex, NULL) == -1)
      {
         ALOGE("NFC Manager incoming socket mutex creation returned 0x%08x", errno);
         return NULL;
      }

      if(pthread_cond_init(&nfc_jni_native_monitor->incoming_socket_cond, NULL) == -1)
      {
         ALOGE("NFC Manager incoming socket condition creation returned 0x%08x", errno);
         return NULL;
      }

}

   return nfc_jni_native_monitor;
}

nfc_jni_native_monitor_t* nfc_jni_get_monitor(void)
{
   return nfc_jni_native_monitor;
}


phLibNfc_Handle nfc_jni_get_p2p_device_handle(JNIEnv *e, jobject o)
{
   ScopedLocalRef<jclass> c(e, e->GetObjectClass(o));
   jfieldID f = e->GetFieldID(c.get(), "mHandle", "I");
   return e->GetIntField(o, f);
}

jshort nfc_jni_get_p2p_device_mode(JNIEnv *e, jobject o)
{
   ScopedLocalRef<jclass> c(e, e->GetObjectClass(o));
   jfieldID f = e->GetFieldID(c.get(), "mMode", "S");
   return e->GetShortField(o, f);
}


int nfc_jni_get_connected_tech_index(JNIEnv *e, jobject o)
{
   ScopedLocalRef<jclass> c(e, e->GetObjectClass(o));
   jfieldID f = e->GetFieldID(c.get(), "mConnectedTechIndex", "I");
   return e->GetIntField(o, f);

}

jint nfc_jni_get_connected_technology(JNIEnv *e, jobject o)
{
   int connectedTech = -1;

   int connectedTechIndex = nfc_jni_get_connected_tech_index(e,o);
   jintArray techTypes = nfc_jni_get_nfc_tag_type(e, o);

   if ((connectedTechIndex != -1) && (techTypes != NULL) &&
           (connectedTechIndex < e->GetArrayLength(techTypes))) {
       jint* technologies = e->GetIntArrayElements(techTypes, 0);
       if (technologies != NULL) {
           connectedTech = technologies[connectedTechIndex];
           e->ReleaseIntArrayElements(techTypes, technologies, JNI_ABORT);
       }
   }

   return connectedTech;

}

jint nfc_jni_get_connected_technology_libnfc_type(JNIEnv *e, jobject o)
{
   jint connectedLibNfcType = -1;

   int connectedTechIndex = nfc_jni_get_connected_tech_index(e,o);
   ScopedLocalRef<jclass> c(e, e->GetObjectClass(o));
   jfieldID f = e->GetFieldID(c.get(), "mTechLibNfcTypes", "[I");
   ScopedLocalRef<jintArray> libNfcTypes(e, (jintArray) e->GetObjectField(o, f));

   if ((connectedTechIndex != -1) && (libNfcTypes.get() != NULL) &&
           (connectedTechIndex < e->GetArrayLength(libNfcTypes.get()))) {
       jint* types = e->GetIntArrayElements(libNfcTypes.get(), 0);
       if (types != NULL) {
           connectedLibNfcType = types[connectedTechIndex];
           e->ReleaseIntArrayElements(libNfcTypes.get(), types, JNI_ABORT);
       }
   }
   return connectedLibNfcType;
}

phLibNfc_Handle nfc_jni_get_connected_handle(JNIEnv *e, jobject o)
{
   ScopedLocalRef<jclass> c(e, e->GetObjectClass(o));
   jfieldID f = e->GetFieldID(c.get(), "mConnectedHandle", "I");
   return e->GetIntField(o, f);
}

phLibNfc_Handle nfc_jni_get_nfc_socket_handle(JNIEnv *e, jobject o)
{
   ScopedLocalRef<jclass> c(e, e->GetObjectClass(o));
   jfieldID f = e->GetFieldID(c.get(), "mHandle", "I");
   return e->GetIntField(o, f);
}

jintArray nfc_jni_get_nfc_tag_type(JNIEnv *e, jobject o)
{
   ScopedLocalRef<jclass> c(e, e->GetObjectClass(o));
   jfieldID f = e->GetFieldID(c.get(), "mTechList","[I");
   return (jintArray) e->GetObjectField(o, f);
}



//Display status code
const char* nfc_jni_get_status_name(NFCSTATUS status)
{
   #define STATUS_ENTRY(status) { status, #status }

   struct status_entry {
      NFCSTATUS   code;
      const char  *name;
   };

   const struct status_entry sNameTable[] = {
      STATUS_ENTRY(NFCSTATUS_SUCCESS),
      STATUS_ENTRY(NFCSTATUS_FAILED),
      STATUS_ENTRY(NFCSTATUS_INVALID_PARAMETER),
      STATUS_ENTRY(NFCSTATUS_INSUFFICIENT_RESOURCES),
      STATUS_ENTRY(NFCSTATUS_TARGET_LOST),
      STATUS_ENTRY(NFCSTATUS_INVALID_HANDLE),
      STATUS_ENTRY(NFCSTATUS_MULTIPLE_TAGS),
      STATUS_ENTRY(NFCSTATUS_ALREADY_REGISTERED),
      STATUS_ENTRY(NFCSTATUS_FEATURE_NOT_SUPPORTED),
      STATUS_ENTRY(NFCSTATUS_SHUTDOWN),
      STATUS_ENTRY(NFCSTATUS_ABORTED),
      STATUS_ENTRY(NFCSTATUS_REJECTED ),
      STATUS_ENTRY(NFCSTATUS_NOT_INITIALISED),
      STATUS_ENTRY(NFCSTATUS_PENDING),
      STATUS_ENTRY(NFCSTATUS_BUFFER_TOO_SMALL),
      STATUS_ENTRY(NFCSTATUS_ALREADY_INITIALISED),
      STATUS_ENTRY(NFCSTATUS_BUSY),
      STATUS_ENTRY(NFCSTATUS_TARGET_NOT_CONNECTED),
      STATUS_ENTRY(NFCSTATUS_MULTIPLE_PROTOCOLS),
      STATUS_ENTRY(NFCSTATUS_DESELECTED),
      STATUS_ENTRY(NFCSTATUS_INVALID_DEVICE),
      STATUS_ENTRY(NFCSTATUS_MORE_INFORMATION),
      STATUS_ENTRY(NFCSTATUS_RF_TIMEOUT),
      STATUS_ENTRY(NFCSTATUS_RF_ERROR),
      STATUS_ENTRY(NFCSTATUS_BOARD_COMMUNICATION_ERROR),
      STATUS_ENTRY(NFCSTATUS_INVALID_STATE),
      STATUS_ENTRY(NFCSTATUS_NOT_REGISTERED),
      STATUS_ENTRY(NFCSTATUS_RELEASED),
      STATUS_ENTRY(NFCSTATUS_NOT_ALLOWED),
      STATUS_ENTRY(NFCSTATUS_INVALID_REMOTE_DEVICE),
      STATUS_ENTRY(NFCSTATUS_SMART_TAG_FUNC_NOT_SUPPORTED),
      STATUS_ENTRY(NFCSTATUS_READ_FAILED),
      STATUS_ENTRY(NFCSTATUS_WRITE_FAILED),
      STATUS_ENTRY(NFCSTATUS_NO_NDEF_SUPPORT),
      STATUS_ENTRY(NFCSTATUS_EOF_NDEF_CONTAINER_REACHED),
      STATUS_ENTRY(NFCSTATUS_INVALID_RECEIVE_LENGTH),
      STATUS_ENTRY(NFCSTATUS_INVALID_FORMAT),
      STATUS_ENTRY(NFCSTATUS_INSUFFICIENT_STORAGE),
      STATUS_ENTRY(NFCSTATUS_FORMAT_ERROR),
   };

   int i = sizeof(sNameTable)/sizeof(status_entry);

   while(i>0)
   {
      i--;
      if (sNameTable[i].code == PHNFCSTATUS(status))
      {
         return sNameTable[i].name;
      }
   }

   return "UNKNOWN";
}

int addTechIfNeeded(int *techList, int* handleList, int* typeList, int listSize,
        int maxListSize, int techToAdd, int handleToAdd, int typeToAdd) {
    bool found = false;
    for (int i = 0; i < listSize; i++) {
        if (techList[i] == techToAdd) {
            found = true;
            break;
        }
    }
    if (!found && listSize < maxListSize) {
        techList[listSize] = techToAdd;
        handleList[listSize] = handleToAdd;
        typeList[listSize] = typeToAdd;
        return listSize + 1;
    }
    else {
        return listSize;
    }
}


#define MAX_NUM_TECHNOLOGIES 32

/*
 *  Utility to get a technology tree and a corresponding handle list from a detected tag.
 */
void nfc_jni_get_technology_tree(JNIEnv* e, phLibNfc_RemoteDevList_t* devList, uint8_t count,
                                 ScopedLocalRef<jintArray>* techList,
                                 ScopedLocalRef<jintArray>* handleList,
                                 ScopedLocalRef<jintArray>* libnfcTypeList)
{
   int technologies[MAX_NUM_TECHNOLOGIES];
   int handles[MAX_NUM_TECHNOLOGIES];
   int libnfctypes[MAX_NUM_TECHNOLOGIES];

   int index = 0;
   // TODO: This counts from up to down because on multi-protocols, the
   // ISO handle is usually the second, and we prefer the ISO. Should implement
   // a method to find the "preferred handle order" and use that instead,
   // since we shouldn't have dependencies on the tech list ordering.
   for (int target = count - 1; target >= 0; target--) {
       int type = devList[target].psRemoteDevInfo->RemDevType;
       int handle = devList[target].hTargetDev;
       switch (type)
       {
          case phNfc_eISO14443_A_PICC:
          case phNfc_eISO14443_4A_PICC:
            {
              index = addTechIfNeeded(technologies, handles, libnfctypes, index,
                      MAX_NUM_TECHNOLOGIES, TARGET_TYPE_ISO14443_4, handle, type);
              break;
            }
          case phNfc_eISO14443_4B_PICC:
            {
              index = addTechIfNeeded(technologies, handles, libnfctypes, index,
                      MAX_NUM_TECHNOLOGIES, TARGET_TYPE_ISO14443_4, handle, type);
              index = addTechIfNeeded(technologies, handles, libnfctypes, index,
                      MAX_NUM_TECHNOLOGIES, TARGET_TYPE_ISO14443_3B, handle, type);
            }break;
          case phNfc_eISO14443_3A_PICC:
            {
              index = addTechIfNeeded(technologies, handles, libnfctypes,
                      index, MAX_NUM_TECHNOLOGIES, TARGET_TYPE_ISO14443_3A, handle, type);
            }break;
          case phNfc_eISO14443_B_PICC:
            {
              // TODO a bug in libnfc will cause 14443-3B only cards
              // to be returned as this type as well, but these cards
              // are very rare. Hence assume it's -4B
              index = addTechIfNeeded(technologies, handles, libnfctypes,
                      index, MAX_NUM_TECHNOLOGIES, TARGET_TYPE_ISO14443_4, handle, type);
              index = addTechIfNeeded(technologies, handles, libnfctypes,
                      index, MAX_NUM_TECHNOLOGIES, TARGET_TYPE_ISO14443_3B, handle, type);
            }break;
          case phNfc_eISO15693_PICC:
            {
              index = addTechIfNeeded(technologies, handles, libnfctypes,
                      index, MAX_NUM_TECHNOLOGIES, TARGET_TYPE_ISO15693, handle, type);
            }break;
          case phNfc_eMifare_PICC:
            {
              // We don't want to be too clever here; libnfc has already determined
              // it's a Mifare, so we only check for UL, for all other tags
              // we assume it's a mifare classic. This should make us more
              // future-proof.
              int sak = devList[target].psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.Sak;
              switch(sak)
              {
                case 0x00:
                  // could be UL or UL-C
                  index = addTechIfNeeded(technologies, handles, libnfctypes,
                          index, MAX_NUM_TECHNOLOGIES, TARGET_TYPE_MIFARE_UL, handle, type);
                  break;
                default:
                  index = addTechIfNeeded(technologies, handles, libnfctypes,
                          index, MAX_NUM_TECHNOLOGIES, TARGET_TYPE_MIFARE_CLASSIC, handle, type);
                  break;
              }
            }break;
          case phNfc_eFelica_PICC:
            {
              index = addTechIfNeeded(technologies, handles, libnfctypes,
                      index, MAX_NUM_TECHNOLOGIES, TARGET_TYPE_FELICA, handle, type);
            }break;
          case phNfc_eJewel_PICC:
            {
              // Jewel represented as NfcA
              index = addTechIfNeeded(technologies, handles, libnfctypes,
                      index, MAX_NUM_TECHNOLOGIES, TARGET_TYPE_ISO14443_3A, handle, type);
            }break;
          default:
            {
              index = addTechIfNeeded(technologies, handles, libnfctypes,
                      index, MAX_NUM_TECHNOLOGIES, TARGET_TYPE_UNKNOWN, handle, type);
            }
        }
   }

   // Build the Java arrays
   if (techList != NULL) {
       techList->reset(e->NewIntArray(index));
       e->SetIntArrayRegion(techList->get(), 0, index, technologies);
   }

   if (handleList != NULL) {
       handleList->reset(e->NewIntArray(index));
       e->SetIntArrayRegion(handleList->get(), 0, index, handles);
   }

   if (libnfcTypeList != NULL) {
       libnfcTypeList->reset(e->NewIntArray(index));
       e->SetIntArrayRegion(libnfcTypeList->get(), 0, index, libnfctypes);
   }
}

} // namespace android
