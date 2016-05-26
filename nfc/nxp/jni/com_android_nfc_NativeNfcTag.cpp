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

#include <semaphore.h>
#include <errno.h>
#include <ScopedLocalRef.h>
#include <ScopedPrimitiveArray.h>

#include "com_android_nfc.h"
#include "phNfcHalTypes.h"

static phLibNfc_Data_t nfc_jni_ndef_rw;
static phLibNfc_Handle handle;
uint8_t *nfc_jni_ndef_buf = NULL;
uint32_t nfc_jni_ndef_buf_len = 0;

extern uint8_t device_connected_flag;

namespace android {

extern phLibNfc_Handle storedHandle;

extern void nfc_jni_restart_discovery_locked(struct nfc_jni_native_data *nat);
extern void nfc_jni_reset_timeout_values();

/*
 * Callbacks
 */
 static void nfc_jni_tag_rw_callback(void *pContext, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_tag_rw_callback", status);

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

static void nfc_jni_connect_callback(void *pContext,
   phLibNfc_Handle hRemoteDev,
   phLibNfc_sRemoteDevInformation_t *psRemoteDevInfo, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_connect_callback", status);

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   if (pCallbackData->pContext != NULL) {
       // Store the remote dev info ptr in the callback context
       // Note that this ptr will remain valid, it is tied to a statically
       // allocated buffer in libnfc.
       phLibNfc_sRemoteDevInformation_t** ppRemoteDevInfo =
           (phLibNfc_sRemoteDevInformation_t**)pCallbackData->pContext;
       *ppRemoteDevInfo = psRemoteDevInfo;
   }

   sem_post(&pCallbackData->sem);
}

static void nfc_jni_checkndef_callback(void *pContext,
   phLibNfc_ChkNdef_Info_t info, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_checkndef_callback", status);
   phLibNfc_ChkNdef_Info_t* pNdefInfo = (phLibNfc_ChkNdef_Info_t*) (pCallbackData->pContext);
   if(status == NFCSTATUS_OK)
   {
      if(nfc_jni_ndef_buf)
      {
         free(nfc_jni_ndef_buf);
      }
      nfc_jni_ndef_buf_len = info.MaxNdefMsgLength;
      nfc_jni_ndef_buf = (uint8_t*)malloc(nfc_jni_ndef_buf_len);
      if (pNdefInfo != NULL) *pNdefInfo = info;
   }
   else {
      if (pNdefInfo != NULL) {
        memset(pNdefInfo, 0, sizeof(*pNdefInfo));
      }
   }

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

static void nfc_jni_disconnect_callback(void *pContext,
   phLibNfc_Handle hRemoteDev, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_disconnect_callback", status);

   if(nfc_jni_ndef_buf)
   {
      free(nfc_jni_ndef_buf);
   }
   nfc_jni_ndef_buf = NULL;
   nfc_jni_ndef_buf_len = 0;

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

static void nfc_jni_async_disconnect_callback(void *pContext,
   phLibNfc_Handle hRemoteDev, NFCSTATUS status)
{
   LOG_CALLBACK("nfc_jni_async_disconnect_callback", status);

   if(nfc_jni_ndef_buf)
   {
      free(nfc_jni_ndef_buf);
   }
   nfc_jni_ndef_buf = NULL;
   nfc_jni_ndef_buf_len = 0;
}

static phNfc_sData_t *nfc_jni_transceive_buffer;

static void nfc_jni_transceive_callback(void *pContext,
   phLibNfc_Handle handle, phNfc_sData_t *pResBuffer, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_transceive_callback", status);

   nfc_jni_transceive_buffer = pResBuffer;

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

static void nfc_jni_presencecheck_callback(void *pContext, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_presencecheck_callback", status);

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

static void nfc_jni_formatndef_callback(void *pContext, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_formatndef_callback", status);

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

static void nfc_jni_readonly_callback(void *pContext, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_readonly_callback", status);

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

/* Functions */
static jbyteArray com_android_nfc_NativeNfcTag_doRead(JNIEnv *e,
   jobject o)
{
   NFCSTATUS status;
   phLibNfc_Handle handle = 0;
   jbyteArray buf = NULL;
   struct nfc_jni_callback_data cb_data;

   CONCURRENCY_LOCK();

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, NULL))
   {
      goto clean_and_return;
   }

   handle = nfc_jni_get_connected_handle(e, o);

   nfc_jni_ndef_rw.length = nfc_jni_ndef_buf_len;
   nfc_jni_ndef_rw.buffer = nfc_jni_ndef_buf;

   TRACE("phLibNfc_Ndef_Read()");
   REENTRANCE_LOCK();
   status = phLibNfc_Ndef_Read(handle, &nfc_jni_ndef_rw,
                               phLibNfc_Ndef_EBegin,
                               nfc_jni_tag_rw_callback,
                               (void *)&cb_data);
   REENTRANCE_UNLOCK();
   if(status != NFCSTATUS_PENDING)
   {
      ALOGE("phLibNfc_Ndef_Read() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));
      goto clean_and_return;
   }
   TRACE("phLibNfc_Ndef_Read() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));

   /* Wait for callback response */
   if(sem_wait(&cb_data.sem))
   {
      ALOGE("Failed to wait for semaphore (errno=0x%08x)", errno);
      goto clean_and_return;
   }

   if(cb_data.status != NFCSTATUS_SUCCESS)
   {
      goto clean_and_return;
   }

   buf = e->NewByteArray(nfc_jni_ndef_rw.length);
   e->SetByteArrayRegion(buf, 0, nfc_jni_ndef_rw.length,
      (jbyte *)nfc_jni_ndef_rw.buffer);

clean_and_return:
   nfc_cb_data_deinit(&cb_data);
   CONCURRENCY_UNLOCK();

   return buf;
}


static jboolean com_android_nfc_NativeNfcTag_doWrite(JNIEnv *e,
   jobject o, jbyteArray buf)
{
   NFCSTATUS   status;
   jboolean    result = JNI_FALSE;
   struct nfc_jni_callback_data cb_data;

   phLibNfc_Handle handle = nfc_jni_get_connected_handle(e, o);

   CONCURRENCY_LOCK();

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, NULL))
   {
      goto clean_and_return;
   }

   nfc_jni_ndef_rw.length = (uint32_t)e->GetArrayLength(buf);
   nfc_jni_ndef_rw.buffer = (uint8_t *)e->GetByteArrayElements(buf, NULL);

   TRACE("phLibNfc_Ndef_Write()");
   TRACE("Ndef Handle :0x%x\n",handle);
   TRACE("Ndef buffer length : %d", nfc_jni_ndef_rw.length);
   REENTRANCE_LOCK();
   status = phLibNfc_Ndef_Write(handle, &nfc_jni_ndef_rw,nfc_jni_tag_rw_callback, (void *)&cb_data);
   REENTRANCE_UNLOCK();
   if(status != NFCSTATUS_PENDING)
   {
      ALOGE("phLibNfc_Ndef_Write() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));
      goto clean_and_return;
   }
   TRACE("phLibNfc_Ndef_Write() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));

   /* Wait for callback response */
   if(sem_wait(&cb_data.sem))
   {
      ALOGE("Failed to wait for semaphore (errno=0x%08x)", errno);
      goto clean_and_return;
   }

   if(cb_data.status != NFCSTATUS_SUCCESS)
   {
      goto clean_and_return;
   }

   result = JNI_TRUE;

clean_and_return:
   e->ReleaseByteArrayElements(buf, (jbyte *)nfc_jni_ndef_rw.buffer, JNI_ABORT);

   nfc_cb_data_deinit(&cb_data);
   CONCURRENCY_UNLOCK();
   return result;
}

/*
 *  Utility to recover poll bytes from target infos
 */
void set_target_pollBytes(JNIEnv *e, jobject tag,
        phLibNfc_sRemoteDevInformation_t *psRemoteDevInfo)
{
    ScopedLocalRef<jclass> tag_cls(e, e->GetObjectClass(tag));
    jfieldID f = e->GetFieldID(tag_cls.get(), "mTechPollBytes", "[[B");

    ScopedLocalRef<jobjectArray> existingPollBytes(e, (jobjectArray) e->GetObjectField(tag, f));
    if (existingPollBytes.get() != NULL) {
        return;
    }

    jfieldID techListField = e->GetFieldID(tag_cls.get(), "mTechList", "[I");
    ScopedLocalRef<jintArray> techList(e, (jintArray) e->GetObjectField(tag, techListField));
    ScopedIntArrayRO techIds(e, techList.get());
    int techListLength = techIds.size();

    ScopedLocalRef<jbyteArray> pollBytes(e, e->NewByteArray(0));
    ScopedLocalRef<jclass> byteArrayClass(e, e->GetObjectClass(pollBytes.get()));
    ScopedLocalRef<jobjectArray> techPollBytes(e, e->NewObjectArray(techListLength, byteArrayClass.get(), NULL));

    for (int tech = 0; tech < techListLength; tech++) {
        switch(techIds[tech])
        {
            /* ISO14443-3A: ATQA/SENS_RES */
            case TARGET_TYPE_ISO14443_3A:
                if (psRemoteDevInfo->RemDevType == phNfc_eJewel_PICC) {
                    // Jewel ATQA is not read and stored by the PN544, but it is fixed
                    // at {0x00, 0x0C} in the spec. So eJewel can safely be
                    // translated to {0x00, 0x0C}.
                    const static jbyte JewelAtqA[2] = {0x00, 0x0C};
                    pollBytes.reset(e->NewByteArray(2));
                    e->SetByteArrayRegion(pollBytes.get(), 0, 2, (jbyte*) JewelAtqA);
                } else {
                    pollBytes.reset(e->NewByteArray(sizeof(psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.AtqA)));
                    e->SetByteArrayRegion(pollBytes.get(), 0, sizeof(psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.AtqA),
                                          (jbyte *)psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.AtqA);
                }
                break;
            /* ISO14443-3B: Application data (4 bytes) and Protocol Info (3 bytes) from ATQB/SENSB_RES */
            case TARGET_TYPE_ISO14443_3B:
                pollBytes.reset(e->NewByteArray(sizeof(psRemoteDevInfo->RemoteDevInfo.Iso14443B_Info.AtqB.AtqResInfo.AppData)
                        + sizeof(psRemoteDevInfo->RemoteDevInfo.Iso14443B_Info.AtqB.AtqResInfo.ProtInfo)));
                e->SetByteArrayRegion(pollBytes.get(), 0, sizeof(psRemoteDevInfo->RemoteDevInfo.Iso14443B_Info.AtqB.AtqResInfo.AppData),
                                      (jbyte *)psRemoteDevInfo->RemoteDevInfo.Iso14443B_Info.AtqB.AtqResInfo.AppData);
                e->SetByteArrayRegion(pollBytes.get(), sizeof(psRemoteDevInfo->RemoteDevInfo.Iso14443B_Info.AtqB.AtqResInfo.AppData),
                                      sizeof(psRemoteDevInfo->RemoteDevInfo.Iso14443B_Info.AtqB.AtqResInfo.ProtInfo),
                                      (jbyte *)psRemoteDevInfo->RemoteDevInfo.Iso14443B_Info.AtqB.AtqResInfo.ProtInfo);
                break;
            /* JIS_X_6319_4: PAD0 (2 byte), PAD1 (2 byte), MRTI(2 byte), PAD2 (1 byte), RC (2 byte) */
            case TARGET_TYPE_FELICA:
                pollBytes.reset(e->NewByteArray(sizeof(psRemoteDevInfo->RemoteDevInfo.Felica_Info.PMm)
                        + sizeof(psRemoteDevInfo->RemoteDevInfo.Felica_Info.SystemCode)));
                e->SetByteArrayRegion(pollBytes.get(), 0, sizeof(psRemoteDevInfo->RemoteDevInfo.Felica_Info.PMm),
                                      (jbyte *)psRemoteDevInfo->RemoteDevInfo.Felica_Info.PMm);
                e->SetByteArrayRegion(pollBytes.get(), sizeof(psRemoteDevInfo->RemoteDevInfo.Felica_Info.PMm),
                                      sizeof(psRemoteDevInfo->RemoteDevInfo.Felica_Info.SystemCode),
                                      (jbyte *)psRemoteDevInfo->RemoteDevInfo.Felica_Info.SystemCode);
                break;
            /* ISO15693: response flags (1 byte), DSFID (1 byte) */
            case TARGET_TYPE_ISO15693:
                pollBytes.reset(e->NewByteArray(sizeof(psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Flags)
                        + sizeof(psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Dsfid)));
                e->SetByteArrayRegion(pollBytes.get(), 0, sizeof(psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Flags),
                                      (jbyte *)&psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Flags);
                e->SetByteArrayRegion(pollBytes.get(), sizeof(psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Flags),
                                      sizeof(psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Dsfid),
                                      (jbyte *)&psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Dsfid);
                break;
            default:
                pollBytes.reset(e->NewByteArray(0));
                break;
        }
        e->SetObjectArrayElement(techPollBytes.get(), tech, pollBytes.get());
    }

    e->SetObjectField(tag, f, techPollBytes.get());
}

/*
 *  Utility to recover activation bytes from target infos
 */
void set_target_activationBytes(JNIEnv *e, jobject tag,
        phLibNfc_sRemoteDevInformation_t *psRemoteDevInfo)
{
    ScopedLocalRef<jclass> tag_cls(e, e->GetObjectClass(tag));

    jfieldID f = e->GetFieldID(tag_cls.get(), "mTechActBytes", "[[B");
    ScopedLocalRef<jobjectArray> existingActBytes(e, (jobjectArray) e->GetObjectField(tag, f));
    if (existingActBytes.get() != NULL) {
        return;
    }

    jfieldID techListField = e->GetFieldID(tag_cls.get(), "mTechList", "[I");
    ScopedLocalRef<jintArray> techList(e, (jintArray) e->GetObjectField(tag, techListField));
    ScopedIntArrayRO techIds(e, techList.get());
    int techListLength = techIds.size();

    ScopedLocalRef<jbyteArray> actBytes(e, e->NewByteArray(0));
    ScopedLocalRef<jclass> byteArrayClass(e, e->GetObjectClass(actBytes.get()));
    ScopedLocalRef<jobjectArray> techActBytes(e, e->NewObjectArray(techListLength, byteArrayClass.get(), 0));

    for (int tech = 0; tech < techListLength; tech++) {
        switch(techIds[tech]) {

            /* ISO14443-3A: SAK/SEL_RES */
            case TARGET_TYPE_ISO14443_3A:
                actBytes.reset(e->NewByteArray(sizeof(psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.Sak)));
                e->SetByteArrayRegion(actBytes.get(), 0, sizeof(psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.Sak),
                                      (jbyte *)&psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.Sak);
                break;
            /* ISO14443-3A & ISO14443-4: SAK/SEL_RES, historical bytes from ATS */
            /* ISO14443-3B & ISO14443-4: HiLayerResp */
            case TARGET_TYPE_ISO14443_4:
                // Determine whether -A or -B
                if (psRemoteDevInfo->RemDevType == phNfc_eISO14443_B_PICC ||
                        psRemoteDevInfo->RemDevType == phNfc_eISO14443_4B_PICC) {
                    actBytes.reset(e->NewByteArray(psRemoteDevInfo->RemoteDevInfo.Iso14443B_Info.HiLayerRespLength));
                    e->SetByteArrayRegion(actBytes.get(), 0, psRemoteDevInfo->RemoteDevInfo.Iso14443B_Info.HiLayerRespLength,
                                      (jbyte *)psRemoteDevInfo->RemoteDevInfo.Iso14443B_Info.HiLayerResp);
                } else if (psRemoteDevInfo->RemDevType == phNfc_eISO14443_A_PICC ||
                        psRemoteDevInfo->RemDevType == phNfc_eISO14443_4A_PICC) {
                    actBytes.reset(e->NewByteArray(psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.AppDataLength));
                    e->SetByteArrayRegion(actBytes.get(), 0,
                                          psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.AppDataLength,
                                          (jbyte *)psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.AppData);
                }
                break;
            /* ISO15693: response flags (1 byte), DSFID (1 byte) */
            case TARGET_TYPE_ISO15693:
                actBytes.reset(e->NewByteArray(sizeof(psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Flags)
                        + sizeof(psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Dsfid)));
                e->SetByteArrayRegion(actBytes.get(), 0, sizeof(psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Flags),
                                      (jbyte *)&psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Flags);
                e->SetByteArrayRegion(actBytes.get(), sizeof(psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Flags),
                                      sizeof(psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Dsfid),
                                      (jbyte *)&psRemoteDevInfo->RemoteDevInfo.Iso15693_Info.Dsfid);
                break;
            default:
                actBytes.reset(e->NewByteArray(0));
                break;
        }
        e->SetObjectArrayElement(techActBytes.get(), tech, actBytes.get());
    }
    e->SetObjectField(tag, f, techActBytes.get());
}

static jint com_android_nfc_NativeNfcTag_doConnect(JNIEnv *e,
   jobject o, phLibNfc_Handle handle)
{
   jint status;
   struct nfc_jni_callback_data cb_data;
   phLibNfc_sRemoteDevInformation_t* pRemDevInfo = NULL;

   CONCURRENCY_LOCK();

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, &pRemDevInfo))
   {
      status = NFCSTATUS_NOT_ENOUGH_MEMORY;
      goto clean_and_return;
   }

   TRACE("phLibNfc_RemoteDev_Connect(RW)");
   REENTRANCE_LOCK();
   storedHandle = handle;
   status = phLibNfc_RemoteDev_Connect(handle, nfc_jni_connect_callback,(void *)&cb_data);
   REENTRANCE_UNLOCK();
   if(status != NFCSTATUS_PENDING)
   {
      ALOGE("phLibNfc_RemoteDev_Connect(RW) returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));
      goto clean_and_return;
   }
   TRACE("phLibNfc_RemoteDev_Connect(RW) returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));

   /* Wait for callback response */
   if(sem_wait(&cb_data.sem))
   {
      ALOGE("Failed to wait for semaphore (errno=0x%08x)", errno);
      status = NFCSTATUS_ABORTED;
      goto clean_and_return;
   }

   status = cb_data.status;
   TRACE("phLibNfc_RemoteDev_Connect() - Status code = %d", status);

   /* Connect Status */
   if(status != NFCSTATUS_SUCCESS)
   {
      goto clean_and_return;
   }

   // Success, set poll & act bytes
   set_target_pollBytes(e, o, pRemDevInfo);
   set_target_activationBytes(e, o, pRemDevInfo);

clean_and_return:
   nfc_cb_data_deinit(&cb_data);
   CONCURRENCY_UNLOCK();
   return status;
}

static jint com_android_nfc_NativeNfcTag_doHandleReconnect(JNIEnv *e,
   jobject o, phLibNfc_Handle handle)
{
   jint status;
   struct nfc_jni_callback_data cb_data;
   phLibNfc_sRemoteDevInformation_t* pRemDevInfo = NULL;
   CONCURRENCY_LOCK();

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, &pRemDevInfo))
   {
      status = NFCSTATUS_NOT_ENOUGH_MEMORY;
      goto clean_and_return;
   }

   TRACE("phLibNfc_RemoteDev_ReConnect(RW)");
   REENTRANCE_LOCK();
   storedHandle = handle;
   status = phLibNfc_RemoteDev_ReConnect(handle, nfc_jni_connect_callback,(void *)&cb_data);
   REENTRANCE_UNLOCK();
   if(status != NFCSTATUS_PENDING)
   {
      ALOGE("phLibNfc_RemoteDev_ReConnect(RW) returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));
      goto clean_and_return;
   }
   TRACE("phLibNfc_RemoteDev_ReConnect(RW) returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));

   /* Wait for callback response */
   if(sem_wait(&cb_data.sem))
   {
      ALOGE("Failed to wait for semaphore (errno=0x%08x)", errno);
      status = NFCSTATUS_ABORTED;
      goto clean_and_return;
   }

   status = cb_data.status;

   /* Connect Status */
   if(status != NFCSTATUS_SUCCESS)
   {
      goto clean_and_return;
   }

clean_and_return:
   nfc_cb_data_deinit(&cb_data);
   CONCURRENCY_UNLOCK();
   return status;
}

static jint com_android_nfc_NativeNfcTag_doReconnect(JNIEnv *e,
   jobject o)
{
    // Reconnect is provided by libnfc by just calling connect again
    // on the same handle.
    int libNfcType = nfc_jni_get_connected_technology_libnfc_type(e, o);
    if (libNfcType != -1) {
        // Note that some tag types are stateless, hence we do not reconnect
        // those. Currently those are the Jewel and Iso15693 technologies.
        if ((libNfcType != phNfc_eJewel_PICC) && (libNfcType != phNfc_eISO15693_PICC)) {
            phLibNfc_Handle handle = nfc_jni_get_connected_handle(e,o);
            return com_android_nfc_NativeNfcTag_doConnect(e, o, handle);
        }
        else {
            return NFCSTATUS_SUCCESS;
        }
    }
    else {
        return NFCSTATUS_REJECTED;
    }
}


static jboolean com_android_nfc_NativeNfcTag_doDisconnect(JNIEnv *e, jobject o)
{
   phLibNfc_Handle handle = 0;
   NFCSTATUS status;
   jboolean result = JNI_FALSE;
   struct nfc_jni_callback_data cb_data;

   CONCURRENCY_LOCK();

   handle = nfc_jni_get_connected_handle(e, o);

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, NULL))
   {
      goto clean_and_return;
   }

   /* Reset the stored handle */
   storedHandle = 0;

   nfc_jni_reset_timeout_values();

   /* Disconnect */
   TRACE("Disconnecting from tag (%x)", handle);

   if (handle == -1) {
       // Was never connected to any tag, exit
       result = JNI_TRUE;
       ALOGE("doDisconnect() - Target already disconnected");
       nfc_jni_restart_discovery_locked(nfc_jni_get_nat_ext(e));
       goto clean_and_return;
   }

    TRACE("phLibNfc_RemoteDev_Disconnect(%x)", handle);
    REENTRANCE_LOCK();
    status = phLibNfc_RemoteDev_Disconnect(handle, NFC_DISCOVERY_CONTINUE,
                                          nfc_jni_disconnect_callback, (void *)&cb_data);
    REENTRANCE_UNLOCK();

    if(status == NFCSTATUS_TARGET_NOT_CONNECTED)
    {
        result = JNI_TRUE;
        TRACE("phLibNfc_RemoteDev_Disconnect() - Target already disconnected");
        goto clean_and_return;
    }
    if(status != NFCSTATUS_PENDING)
    {
        ALOGE("phLibNfc_RemoteDev_Disconnect(%x) returned 0x%04x[%s]", handle, status, nfc_jni_get_status_name(status));
        nfc_jni_restart_discovery_locked(nfc_jni_get_nat_ext(e));
        goto clean_and_return;
    }
    TRACE("phLibNfc_RemoteDev_Disconnect(%x) returned 0x%04x[%s]", handle, status, nfc_jni_get_status_name(status));

    /* Wait for callback response */
    if(sem_wait(&cb_data.sem))
    {
       ALOGE("Failed to wait for semaphore (errno=0x%08x)", errno);
       goto clean_and_return;
    }

    /* Disconnect Status */
    if(cb_data.status != NFCSTATUS_SUCCESS)
    {
        goto clean_and_return;
    }

    result = JNI_TRUE;

clean_and_return:
    /* Reset device connected flag */
    device_connected_flag = 0;
   nfc_cb_data_deinit(&cb_data);
   CONCURRENCY_UNLOCK();
   return result;
}

static uint16_t
crc_16_ccitt1( uint8_t* msg, size_t len, uint16_t init )
{
    uint16_t b, crc = init;

    do {
        b = *msg++ ^ (crc & 0xFF);
        b ^= (b << 4) & 0xFF;
        crc = (crc >> 8) ^ (b << 8) ^ (b << 3) ^ (b >> 4);
    } while( --len );

    return crc;
}

static void
nfc_insert_crc_a( uint8_t* msg, size_t len )
{
    uint16_t crc;

    crc = crc_16_ccitt1( msg, len, 0x6363 );
    msg[len] = crc & 0xFF;
    msg[len + 1] = (crc >> 8) & 0xFF;
}

static void
nfc_get_crc_a( uint8_t* msg, size_t len, uint8_t* byte1, uint8_t* byte2)
{
    uint16_t crc;

    crc = crc_16_ccitt1( msg, len, 0x6363 );
    *byte1 = crc & 0xFF;
    *byte2 = (crc >> 8) & 0xFF;
}

static bool
crc_valid( uint8_t* msg, size_t len)
{
    uint8_t crcByte1, crcByte2;

    nfc_get_crc_a(nfc_jni_transceive_buffer->buffer,
          len - 2, &crcByte1, &crcByte2);

    if (msg[len - 2] == crcByte1 &&
          msg[len - 1] == crcByte2) {
        return true;
    }
    else {
        return false;
    }

}

static jbyteArray com_android_nfc_NativeNfcTag_doTransceive(JNIEnv *e,
   jobject o, jbyteArray data, jboolean raw, jintArray statusTargetLost)
{
    uint8_t offset = 0;
    // buf is the pointer to the JNI array and never overwritten,
    // outbuf is passed into the transceive - it may be pointed to new memory
    // to be extended with CRC.
    uint8_t *buf = NULL;
    uint32_t buflen;

    uint8_t *outbuf = NULL;
    uint32_t outlen;
    phLibNfc_sTransceiveInfo_t transceive_info;
    jbyteArray result = NULL;
    int res;
    phLibNfc_Handle handle = nfc_jni_get_connected_handle(e, o);
    NFCSTATUS status;
    struct nfc_jni_callback_data cb_data;
    int selectedTech = 0;
    int selectedLibNfcType = 0;
    jint* technologies = NULL;
    bool checkResponseCrc = false;

    jint *targetLost;
    if (statusTargetLost != NULL) {
        targetLost = e->GetIntArrayElements(statusTargetLost, 0);
        if (targetLost != NULL) {
            *targetLost = 0;
        }
    } else {
        targetLost = NULL;
    }

    memset(&transceive_info, 0, sizeof(transceive_info));
    CONCURRENCY_LOCK();

    /* Create the local semaphore */
    if (!nfc_cb_data_init(&cb_data, NULL))
    {
       goto clean_and_return;
    }

    selectedTech = nfc_jni_get_connected_technology(e, o);
    selectedLibNfcType = nfc_jni_get_connected_technology_libnfc_type(e, o);

    buf = outbuf = (uint8_t *)e->GetByteArrayElements(data, NULL);
    buflen = outlen = (uint32_t)e->GetArrayLength(data);

    switch (selectedTech) {
        case TARGET_TYPE_FELICA:
          transceive_info.cmd.FelCmd = phNfc_eFelica_Raw;
          transceive_info.addr = 0;
          break;
        case TARGET_TYPE_MIFARE_CLASSIC:
        case TARGET_TYPE_MIFARE_UL:
          if (raw) {
              transceive_info.cmd.MfCmd = phHal_eMifareRaw;
              transceive_info.addr = 0;
              // Need to add in the crc here
              outbuf = (uint8_t*)malloc(buflen + 2);
              outlen += 2;
              memcpy(outbuf, buf, buflen);
              nfc_insert_crc_a(outbuf, buflen);

              checkResponseCrc = true;
          } else {
              offset = 2;
              transceive_info.cmd.MfCmd = (phNfc_eMifareCmdList_t)buf[0];
              transceive_info.addr = (uint8_t)buf[1];
          }
          break;
        case TARGET_TYPE_ISO14443_3A:
          // Check which libnfc type
          if (selectedLibNfcType == phNfc_eJewel_PICC) {
              // For the Jewel pipe, CRC is automatically computed
              transceive_info.cmd.JewelCmd = phNfc_eJewel_Raw;
              transceive_info.addr = 0;
          } else {
              if (raw) {
                  // Use Mifare Raw to implement a standard
                  // ISO14443-3A transceive, with CRC added
                  transceive_info.cmd.MfCmd = phHal_eMifareRaw;
                  transceive_info.addr = 0;
                  // Need to add in the crc here
                  outbuf = (uint8_t*)malloc(buflen + 2);
                  outlen += 2;
                  memcpy(outbuf, buf, buflen);
                  nfc_insert_crc_a(outbuf, buflen);

                  checkResponseCrc = true;
              } else {
                  // Use the mifare pipe
                  offset = 2;
                  transceive_info.cmd.MfCmd = (phNfc_eMifareCmdList_t)buf[0];
                  transceive_info.addr = (uint8_t)buf[1];
              }

          }
          break;
        case TARGET_TYPE_ISO14443_4:
          transceive_info.cmd.Iso144434Cmd = phNfc_eIso14443_4_Raw;
          transceive_info.addr = 0;
          break;
        case TARGET_TYPE_ISO15693:
          transceive_info.cmd.Iso15693Cmd = phNfc_eIso15693_Cmd;
          transceive_info.addr = 0;
          break;
        case TARGET_TYPE_UNKNOWN:
        case TARGET_TYPE_ISO14443_3B:
          // Not supported
          goto clean_and_return;
        default:
          break;
    }

    transceive_info.sSendData.buffer = outbuf + offset;
    transceive_info.sSendData.length = outlen - offset;
    transceive_info.sRecvData.buffer = (uint8_t*)malloc(1024);
    transceive_info.sRecvData.length = 1024;
    if(transceive_info.sRecvData.buffer == NULL)
    {
      goto clean_and_return;
    }

    TRACE("phLibNfc_RemoteDev_Transceive()");
    REENTRANCE_LOCK();
    status = phLibNfc_RemoteDev_Transceive(handle, &transceive_info,
         nfc_jni_transceive_callback, (void *)&cb_data);
    REENTRANCE_UNLOCK();
    if(status != NFCSTATUS_PENDING)
    {
      ALOGE("phLibNfc_RemoteDev_Transceive() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));
      if ((targetLost != NULL) && (status == NFCSTATUS_TARGET_LOST)) {
          *targetLost = 1;
      }
      goto clean_and_return;
    }
    TRACE("phLibNfc_RemoteDev_Transceive() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));

    /* Wait for callback response */
    if(sem_wait(&cb_data.sem))
    {
       ALOGE("Failed to wait for semaphore (errno=0x%08x)", errno);
       goto clean_and_return;
    }

    if(cb_data.status != NFCSTATUS_SUCCESS)
    {
        if ((targetLost != NULL) && (cb_data.status == NFCSTATUS_TARGET_LOST)) {
            *targetLost = 1;
        }
        goto clean_and_return;
    }

    /* Copy results back to Java *
     * In case of NfcA and raw, also check the CRC in the response
     * and cut it off in the returned data.
     */
    if ((nfc_jni_transceive_buffer->length > 2) && checkResponseCrc) {
        if (crc_valid(nfc_jni_transceive_buffer->buffer, nfc_jni_transceive_buffer->length)) {
            result = e->NewByteArray(nfc_jni_transceive_buffer->length - 2);
            if (result != NULL) {
                e->SetByteArrayRegion(result, 0,
                 nfc_jni_transceive_buffer->length - 2,
                 (jbyte *)nfc_jni_transceive_buffer->buffer);
            }
        }
    } else {
        result = e->NewByteArray(nfc_jni_transceive_buffer->length);
        if (result != NULL) {
            e->SetByteArrayRegion(result, 0,
             nfc_jni_transceive_buffer->length,
             (jbyte *)nfc_jni_transceive_buffer->buffer);
        }
    }
clean_and_return:
    if(transceive_info.sRecvData.buffer != NULL)
    {
      free(transceive_info.sRecvData.buffer);
    }

    if ((outbuf != buf) && (outbuf != NULL)) {
        // Buf was extended and re-alloced with crc bytes, free separately
        free(outbuf);
    }

    e->ReleaseByteArrayElements(data,
      (jbyte *)buf, JNI_ABORT);

    if (targetLost != NULL) {
        e->ReleaseIntArrayElements(statusTargetLost, targetLost, 0);
    }

    nfc_cb_data_deinit(&cb_data);

    CONCURRENCY_UNLOCK();

    return result;
}

static jint com_android_nfc_NativeNfcTag_doGetNdefType(JNIEnv *e, jobject o,
        jint libnfcType, jint javaType)
{
    jint ndefType =  NDEF_UNKNOWN_TYPE;

    switch (libnfcType) {
          case phNfc_eJewel_PICC:
              ndefType = NDEF_TYPE1_TAG;
              break;
          case phNfc_eISO14443_3A_PICC:
              ndefType = NDEF_TYPE2_TAG;;
              break;
          case phNfc_eFelica_PICC:
              ndefType = NDEF_TYPE3_TAG;
              break;
          case phNfc_eISO14443_A_PICC:
          case phNfc_eISO14443_4A_PICC:
          case phNfc_eISO14443_B_PICC:
          case phNfc_eISO14443_4B_PICC:
              ndefType = NDEF_TYPE4_TAG;
              break;
          case phNfc_eMifare_PICC:
              if (javaType == TARGET_TYPE_MIFARE_UL) {
                  ndefType = NDEF_TYPE2_TAG;
              } else {
                  ndefType = NDEF_MIFARE_CLASSIC_TAG;
              }
              break;
          case phNfc_eISO15693_PICC:
              ndefType = NDEF_ICODE_SLI_TAG;
              break;
          default:
              ndefType = NDEF_UNKNOWN_TYPE;
              break;
    }
    return ndefType;
}

static jint com_android_nfc_NativeNfcTag_doCheckNdef(JNIEnv *e, jobject o, jintArray ndefinfo)
{
   phLibNfc_Handle handle = 0;
   jint status;
   phLibNfc_ChkNdef_Info_t sNdefInfo;
   struct nfc_jni_callback_data cb_data;
   jint *ndef = e->GetIntArrayElements(ndefinfo, 0);
   int apiCardState = NDEF_MODE_UNKNOWN;

   CONCURRENCY_LOCK();

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, NULL))
   {
      status = NFCSTATUS_NOT_ENOUGH_MEMORY;
      goto clean_and_return;
   }
   cb_data.pContext = &sNdefInfo;

   handle = nfc_jni_get_connected_handle(e, o);

   TRACE("phLibNfc_Ndef_CheckNdef()");
   REENTRANCE_LOCK();
   status = phLibNfc_Ndef_CheckNdef(handle, nfc_jni_checkndef_callback,(void *)&cb_data);
   REENTRANCE_UNLOCK();
   if(status != NFCSTATUS_PENDING)
   {
      ALOGE("phLibNfc_Ndef_CheckNdef() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));
      goto clean_and_return;
   }
   TRACE("phLibNfc_Ndef_CheckNdef() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));

   /* Wait for callback response */
   if(sem_wait(&cb_data.sem))
   {
      ALOGE("Failed to wait for semaphore (errno=0x%08x)", errno);
      status = NFCSTATUS_ABORTED;
      goto clean_and_return;
   }

   status = cb_data.status;
   TRACE("phLibNfc_Ndef_CheckNdef() - Status code = %d", status);

   if (status != NFCSTATUS_SUCCESS)
   {
      goto clean_and_return;
   }

   ndef[0] = sNdefInfo.MaxNdefMsgLength;
   // Translate the card state to know values for the NFC API
   switch (sNdefInfo.NdefCardState) {
       case PHLIBNFC_NDEF_CARD_INITIALISED:
           apiCardState = NDEF_MODE_READ_WRITE;
           break;
       case PHLIBNFC_NDEF_CARD_READ_ONLY:
           apiCardState = NDEF_MODE_READ_ONLY;
           break;
       case PHLIBNFC_NDEF_CARD_READ_WRITE:
           apiCardState = NDEF_MODE_READ_WRITE;
           break;
       case PHLIBNFC_NDEF_CARD_INVALID:
           apiCardState = NDEF_MODE_UNKNOWN;
           break;
   }
   ndef[1] = apiCardState;

clean_and_return:
   e->ReleaseIntArrayElements(ndefinfo, ndef, 0);
   nfc_cb_data_deinit(&cb_data);
   CONCURRENCY_UNLOCK();
   return status;
}

static jboolean com_android_nfc_NativeNfcTag_doPresenceCheck(JNIEnv *e, jobject o)
{
   phLibNfc_Handle handle = 0;
   NFCSTATUS status;
   jboolean result = JNI_FALSE;
   struct nfc_jni_callback_data cb_data;

   CONCURRENCY_LOCK();

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, NULL))
   {
      goto clean_and_return;
   }

   handle = nfc_jni_get_connected_handle(e, o);

   TRACE("phLibNfc_RemoteDev_CheckPresence()");
   REENTRANCE_LOCK();
   status = phLibNfc_RemoteDev_CheckPresence(handle, nfc_jni_presencecheck_callback, (void *)&cb_data);
   REENTRANCE_UNLOCK();

   if(status != NFCSTATUS_PENDING)
   {
      ALOGE("phLibNfc_RemoteDev_CheckPresence() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));
      goto clean_and_return;
   }
   TRACE("phLibNfc_RemoteDev_CheckPresence() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));

   /* Wait for callback response */
   if(sem_wait(&cb_data.sem))
   {
      ALOGE("Failed to wait for semaphore (errno=0x%08x)", errno);
      goto clean_and_return;
   }

   if (cb_data.status == NFCSTATUS_SUCCESS)
   {
       result = JNI_TRUE;
   }

clean_and_return:
   nfc_cb_data_deinit(&cb_data);

   CONCURRENCY_UNLOCK();

   return result;
}

static jboolean com_android_nfc_NativeNfcTag_doIsIsoDepNdefFormatable(JNIEnv *e,
        jobject o, jbyteArray pollBytes, jbyteArray actBytes)
{
    // Determines whether this is a formatable IsoDep tag - currently only NXP DESFire
    // is supported.
    jboolean result = JNI_FALSE;

    // DESfire has one sak byte and 2 ATQA bytes
    if (pollBytes != NULL && (e->GetArrayLength(pollBytes) >= 2) &&
            actBytes != NULL && (e->GetArrayLength(actBytes) >= 1)) {
        jbyte* poll = e->GetByteArrayElements(pollBytes, NULL);
        jbyte* act = e->GetByteArrayElements(actBytes, NULL);
        if (act[0] == 0x20 && poll[1] == 0x03) {
            uint8_t cmd[] = {0x90, 0x60, 0x00, 0x00, 0x00};
            // Identifies as DESfire, use get version cmd to be sure
            jbyteArray versionCmd = e->NewByteArray(5);
            e->SetByteArrayRegion(versionCmd, 0, 5, (jbyte*)cmd);
            jbyteArray respBytes = com_android_nfc_NativeNfcTag_doTransceive(e, o,
                        versionCmd, JNI_TRUE, NULL);
            if (respBytes != NULL) {
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
                jbyte* resp = e->GetByteArrayElements(respBytes, NULL);
                if (respLength == 9 && resp[7] == (jbyte)0x91 &&
                        resp[8] == (jbyte)0xAF) {
                    result = JNI_TRUE;
                }
                e->ReleaseByteArrayElements(respBytes, (jbyte *)resp, JNI_ABORT);
            }
        }
        e->ReleaseByteArrayElements(pollBytes, (jbyte *)poll, JNI_ABORT);
        e->ReleaseByteArrayElements(actBytes, (jbyte *)act, JNI_ABORT);
    }

    return result;
}

static jboolean com_android_nfc_NativeNfcTag_doNdefFormat(JNIEnv *e, jobject o, jbyteArray key)
{
   phLibNfc_Handle handle = 0;
   NFCSTATUS status;
   phNfc_sData_t keyBuffer;
   jboolean result = JNI_FALSE;
   struct nfc_jni_callback_data cb_data;

   CONCURRENCY_LOCK();

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, NULL))
   {
      goto clean_and_return;
   }

   handle = nfc_jni_get_connected_handle(e, o);

   keyBuffer.buffer = (uint8_t *)e->GetByteArrayElements(key, NULL);
   keyBuffer.length = e->GetArrayLength(key);
   TRACE("phLibNfc_RemoteDev_FormatNdef()");
   REENTRANCE_LOCK();
   status = phLibNfc_RemoteDev_FormatNdef(handle, &keyBuffer, nfc_jni_formatndef_callback, (void *)&cb_data);
   REENTRANCE_UNLOCK();

   if(status != NFCSTATUS_PENDING)
   {
      ALOGE("phLibNfc_RemoteDev_FormatNdef() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));
      goto clean_and_return;
   }
   TRACE("phLibNfc_RemoteDev_FormatNdef() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));

   /* Wait for callback response */
   if(sem_wait(&cb_data.sem))
   {
      ALOGE("Failed to wait for semaphore (errno=0x%08x)", errno);
      goto clean_and_return;
   }

   if (cb_data.status == NFCSTATUS_SUCCESS)
   {
       result = JNI_TRUE;
   }

clean_and_return:
   e->ReleaseByteArrayElements(key, (jbyte *)keyBuffer.buffer, JNI_ABORT);
   nfc_cb_data_deinit(&cb_data);
   CONCURRENCY_UNLOCK();
   return result;
}

static jboolean com_android_nfc_NativeNfcTag_doMakeReadonly(JNIEnv *e, jobject o, jbyteArray key)
{
   phLibNfc_Handle handle = 0;
   NFCSTATUS status;
   jboolean result = JNI_FALSE;
   struct nfc_jni_callback_data cb_data;
   phNfc_sData_t keyBuffer;

   CONCURRENCY_LOCK();

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, NULL))
   {
      goto clean_and_return;
   }

   handle = nfc_jni_get_connected_handle(e, o);
   keyBuffer.buffer = (uint8_t *)e->GetByteArrayElements(key, NULL);
   keyBuffer.length = e->GetArrayLength(key);
   TRACE("phLibNfc_ConvertToReadOnlyNdef()");
   REENTRANCE_LOCK();
   status = phLibNfc_ConvertToReadOnlyNdef(handle, &keyBuffer, nfc_jni_readonly_callback,
           (void *)&cb_data);
   REENTRANCE_UNLOCK();

   if(status != NFCSTATUS_PENDING)
   {
      ALOGE("pphLibNfc_ConvertToReadOnlyNdef() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));
      goto clean_and_return;
   }
   TRACE("phLibNfc_ConvertToReadOnlyNdef() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));

   /* Wait for callback response */
   if(sem_wait(&cb_data.sem))
   {
      ALOGE("Failed to wait for semaphore (errno=0x%08x)", errno);
      goto clean_and_return;
   }

   if (cb_data.status == NFCSTATUS_SUCCESS)
   {
       result = JNI_TRUE;
   }

clean_and_return:
   e->ReleaseByteArrayElements(key, (jbyte *)keyBuffer.buffer, JNI_ABORT);
   nfc_cb_data_deinit(&cb_data);
   CONCURRENCY_UNLOCK();
   return result;
}
/*
 * JNI registration.
 */
static JNINativeMethod gMethods[] =
{
   {"doConnect", "(I)I",
      (void *)com_android_nfc_NativeNfcTag_doConnect},
   {"doDisconnect", "()Z",
      (void *)com_android_nfc_NativeNfcTag_doDisconnect},
   {"doReconnect", "()I",
      (void *)com_android_nfc_NativeNfcTag_doReconnect},
   {"doHandleReconnect", "(I)I",
      (void *)com_android_nfc_NativeNfcTag_doHandleReconnect},
   {"doTransceive", "([BZ[I)[B",
      (void *)com_android_nfc_NativeNfcTag_doTransceive},
   {"doGetNdefType", "(II)I",
      (void *)com_android_nfc_NativeNfcTag_doGetNdefType},
   {"doCheckNdef", "([I)I",
      (void *)com_android_nfc_NativeNfcTag_doCheckNdef},
   {"doRead", "()[B",
      (void *)com_android_nfc_NativeNfcTag_doRead},
   {"doWrite", "([B)Z",
      (void *)com_android_nfc_NativeNfcTag_doWrite},
   {"doPresenceCheck", "()Z",
      (void *)com_android_nfc_NativeNfcTag_doPresenceCheck},
   {"doIsIsoDepNdefFormatable", "([B[B)Z",
      (void *)com_android_nfc_NativeNfcTag_doIsIsoDepNdefFormatable},
   {"doNdefFormat", "([B)Z",
      (void *)com_android_nfc_NativeNfcTag_doNdefFormat},
   {"doMakeReadonly", "([B)Z",
      (void *)com_android_nfc_NativeNfcTag_doMakeReadonly},
};

int register_com_android_nfc_NativeNfcTag(JNIEnv *e)
{
   return jniRegisterNativeMethods(e,
      "com/android/nfc/dhimpl/NativeNfcTag",
      gMethods, NELEM(gMethods));
}

} // namespace android
