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

#include <semaphore.h>
#include <errno.h>
#include <ScopedLocalRef.h>

#include "com_android_nfc.h"

extern uint8_t device_connected_flag;

namespace android {

extern void nfc_jni_restart_discovery_locked(struct nfc_jni_native_data *nat);

/*
 * Callbacks
 */
static void nfc_jni_presence_check_callback(void* pContext, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_presence_check_callback", status);

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

static void nfc_jni_connect_callback(void *pContext,
                                     phLibNfc_Handle hRemoteDev,
                                     phLibNfc_sRemoteDevInformation_t *psRemoteDevInfo, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   phNfc_sData_t * psGeneralBytes = (phNfc_sData_t *)pCallbackData->pContext;
   LOG_CALLBACK("nfc_jni_connect_callback", status);

   if(status == NFCSTATUS_SUCCESS)
   {
      psGeneralBytes->length = psRemoteDevInfo->RemoteDevInfo.NfcIP_Info.ATRInfo_Length;
      psGeneralBytes->buffer = (uint8_t*)malloc(psRemoteDevInfo->RemoteDevInfo.NfcIP_Info.ATRInfo_Length);
      psGeneralBytes->buffer = psRemoteDevInfo->RemoteDevInfo.NfcIP_Info.ATRInfo;
   }

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

static void nfc_jni_disconnect_callback(void *pContext, phLibNfc_Handle hRemoteDev, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_disconnect_callback", status);

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

static void nfc_jni_receive_callback(void *pContext, phNfc_sData_t *data, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   phNfc_sData_t **ptr = (phNfc_sData_t **)pCallbackData->pContext;
   LOG_CALLBACK("nfc_jni_receive_callback", status);

   if(status == NFCSTATUS_SUCCESS)
   {
      *ptr = data;
   }
   else
   {
      *ptr = NULL;
   }

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

static void nfc_jni_send_callback(void *pContext, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_send_callback", status);

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

/*
 * Functions
 */

static void nfc_jni_transceive_callback(void *pContext,
  phLibNfc_Handle handle, phNfc_sData_t *pResBuffer, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_transceive_callback", status);

   /* Report the callback data and wake up the caller */
   pCallbackData->pContext = pResBuffer;
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

static jboolean com_android_nfc_NativeP2pDevice_doConnect(JNIEnv *e, jobject o)
{
    phLibNfc_Handle handle = 0;
    NFCSTATUS status;
    jboolean result = JNI_FALSE;
    struct nfc_jni_callback_data cb_data;

    ScopedLocalRef<jclass> target_cls(e, NULL);
    jobject tag;
    jmethodID ctor;
    jfieldID f;
    jbyteArray generalBytes = NULL;
    phNfc_sData_t sGeneralBytes;
    unsigned int i;

    CONCURRENCY_LOCK();

    handle = nfc_jni_get_p2p_device_handle(e, o);

    /* Create the local semaphore */
    if (!nfc_cb_data_init(&cb_data, (void*)&sGeneralBytes))
    {
       goto clean_and_return;
    }

    TRACE("phLibNfc_RemoteDev_Connect(P2P)");
    REENTRANCE_LOCK();
    status = phLibNfc_RemoteDev_Connect(handle, nfc_jni_connect_callback, (void*)&cb_data);
    REENTRANCE_UNLOCK();
    if(status != NFCSTATUS_PENDING)
    {
      ALOGE("phLibNfc_RemoteDev_Connect(P2P) returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));
      goto clean_and_return;
    }
    TRACE("phLibNfc_RemoteDev_Connect(P2P) returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));

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

    /* Set General Bytes */
    target_cls.reset(e->GetObjectClass(o));

    f = e->GetFieldID(target_cls.get(), "mGeneralBytes", "[B");

    TRACE("General Bytes Length = %d", sGeneralBytes.length);
    TRACE("General Bytes =");
    for(i=0;i<sGeneralBytes.length;i++)
    {
      TRACE("0x%02x ", sGeneralBytes.buffer[i]);
    }

    generalBytes = e->NewByteArray(sGeneralBytes.length);

    e->SetByteArrayRegion(generalBytes, 0,
                         sGeneralBytes.length,
                         (jbyte *)sGeneralBytes.buffer);

    e->SetObjectField(o, f, generalBytes);

    result = JNI_TRUE;

clean_and_return:
    if (result != JNI_TRUE)
    {
       /* Restart the polling loop if the connection failed */
       nfc_jni_restart_discovery_locked(nfc_jni_get_nat_ext(e));
    }
    nfc_cb_data_deinit(&cb_data);
    CONCURRENCY_UNLOCK();
    return result;
}

static jboolean com_android_nfc_NativeP2pDevice_doDisconnect(JNIEnv *e, jobject o)
{
    phLibNfc_Handle     handle = 0;
    jboolean            result = JNI_FALSE;
    NFCSTATUS           status;
    struct nfc_jni_callback_data cb_data;

    CONCURRENCY_LOCK();

    handle = nfc_jni_get_p2p_device_handle(e, o);

    /* Create the local semaphore */
    if (!nfc_cb_data_init(&cb_data, NULL))
    {
       goto clean_and_return;
    }

    /* Disconnect */
    TRACE("Disconnecting from target (handle = 0x%x)", handle);

    /* NativeNfcTag waits for tag to leave the field here with presence check.
     * We do not in P2P path because presence check is not safe while transceive may be
     * in progress.
     */

    TRACE("phLibNfc_RemoteDev_Disconnect()");
    REENTRANCE_LOCK();
    status = phLibNfc_RemoteDev_Disconnect(handle, NFC_DISCOVERY_CONTINUE,nfc_jni_disconnect_callback, (void *)&cb_data);
    REENTRANCE_UNLOCK();
    if(status != NFCSTATUS_PENDING)
    {
        ALOGE("phLibNfc_RemoteDev_Disconnect() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));
        if(status == NFCSTATUS_TARGET_NOT_CONNECTED)
        {
            ALOGE("phLibNfc_RemoteDev_Disconnect() failed: Target not connected");
        }
        else
        {
            ALOGE("phLibNfc_RemoteDev_Disconnect() failed");
            nfc_jni_restart_discovery_locked(nfc_jni_get_nat_ext(e));
        }

        goto clean_and_return;
    }
    TRACE("phLibNfc_RemoteDev_Disconnect() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));

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

static jbyteArray com_android_nfc_NativeP2pDevice_doTransceive(JNIEnv *e,
   jobject o, jbyteArray data)
{
   NFCSTATUS status;
   uint8_t offset = 2;
   uint8_t *buf;
   uint32_t buflen;
   phLibNfc_sTransceiveInfo_t transceive_info;
   jbyteArray result = NULL;
   phLibNfc_Handle handle = nfc_jni_get_p2p_device_handle(e, o);
   phNfc_sData_t * receive_buffer = NULL;
   struct nfc_jni_callback_data cb_data;

   CONCURRENCY_LOCK();

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, (void*)receive_buffer))
   {
      goto clean_and_return;
   }

   /* Transceive*/
   TRACE("Transceive data to target (handle = 0x%x)", handle);

   buf = (uint8_t *)e->GetByteArrayElements(data, NULL);
   buflen = (uint32_t)e->GetArrayLength(data);

   TRACE("Buffer Length = %d\n", buflen);

   transceive_info.sSendData.buffer = buf; //+ offset;
   transceive_info.sSendData.length = buflen; //- offset;
   transceive_info.sRecvData.buffer = (uint8_t*)malloc(1024);
   transceive_info.sRecvData.length = 1024;

   if(transceive_info.sRecvData.buffer == NULL)
   {
      goto clean_and_return;
   }

   TRACE("phLibNfc_RemoteDev_Transceive(P2P)");
   REENTRANCE_LOCK();
   status = phLibNfc_RemoteDev_Transceive(handle, &transceive_info, nfc_jni_transceive_callback, (void *)&cb_data);
   REENTRANCE_UNLOCK();
   if(status != NFCSTATUS_PENDING)
   {
      ALOGE("phLibNfc_RemoteDev_Transceive(P2P) returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));
      goto clean_and_return;
   }
   TRACE("phLibNfc_RemoteDev_Transceive(P2P) returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));

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

   /* Copy results back to Java */
   result = e->NewByteArray(receive_buffer->length);
   if(result != NULL)
      e->SetByteArrayRegion(result, 0,
         receive_buffer->length,
         (jbyte *)receive_buffer->buffer);

clean_and_return:
   if(transceive_info.sRecvData.buffer != NULL)
   {
      free(transceive_info.sRecvData.buffer);
   }

   e->ReleaseByteArrayElements(data,
      (jbyte *)transceive_info.sSendData.buffer, JNI_ABORT);

   nfc_cb_data_deinit(&cb_data);

   CONCURRENCY_UNLOCK();

   return result;
}


static jbyteArray com_android_nfc_NativeP2pDevice_doReceive(
   JNIEnv *e, jobject o)
{
   NFCSTATUS status;
   struct timespec ts;
   phLibNfc_Handle handle;
   jbyteArray buf = NULL;
   static phNfc_sData_t *data;
   struct nfc_jni_callback_data cb_data;

   CONCURRENCY_LOCK();

   handle = nfc_jni_get_p2p_device_handle(e, o);

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, (void*)data))
   {
      goto clean_and_return;
   }

   /* Receive */
   TRACE("phLibNfc_RemoteDev_Receive()");
   REENTRANCE_LOCK();
   status = phLibNfc_RemoteDev_Receive(handle, nfc_jni_receive_callback,(void *)&cb_data);
   REENTRANCE_UNLOCK();
   if(status != NFCSTATUS_PENDING)
   {
      ALOGE("phLibNfc_RemoteDev_Receive() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));
      goto clean_and_return;
   }
   TRACE("phLibNfc_RemoteDev_Receive() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));

   /* Wait for callback response */
   if(sem_wait(&cb_data.sem))
   {
      ALOGE("Failed to wait for semaphore (errno=0x%08x)", errno);
      goto clean_and_return;
   }

   if(data == NULL)
   {
      goto clean_and_return;
   }

   buf = e->NewByteArray(data->length);
   e->SetByteArrayRegion(buf, 0, data->length, (jbyte *)data->buffer);

clean_and_return:
   nfc_cb_data_deinit(&cb_data);
   CONCURRENCY_UNLOCK();
   return buf;
}

static jboolean com_android_nfc_NativeP2pDevice_doSend(
   JNIEnv *e, jobject o, jbyteArray buf)
{
   NFCSTATUS status;
   phNfc_sData_t data;
   jboolean result = JNI_FALSE;
   struct nfc_jni_callback_data cb_data;

   phLibNfc_Handle handle = nfc_jni_get_p2p_device_handle(e, o);

   CONCURRENCY_LOCK();

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, NULL))
   {
      goto clean_and_return;
   }

   /* Send */
   TRACE("Send data to the Initiator (handle = 0x%x)", handle);

   data.length = (uint32_t)e->GetArrayLength(buf);
   data.buffer = (uint8_t *)e->GetByteArrayElements(buf, NULL);

   TRACE("phLibNfc_RemoteDev_Send()");
   REENTRANCE_LOCK();
   status = phLibNfc_RemoteDev_Send(handle, &data, nfc_jni_send_callback,(void *)&cb_data);
   REENTRANCE_UNLOCK();
   if(status != NFCSTATUS_PENDING)
   {
      ALOGE("phLibNfc_RemoteDev_Send() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));
      goto clean_and_return;
   }
   TRACE("phLibNfc_RemoteDev_Send() returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));

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
   if (result != JNI_TRUE)
   {
      e->ReleaseByteArrayElements(buf, (jbyte *)data.buffer, JNI_ABORT);
   }
   nfc_cb_data_deinit(&cb_data);
   CONCURRENCY_UNLOCK();
   return result;
}

/*
 * JNI registration.
 */
static JNINativeMethod gMethods[] =
{
   {"doConnect", "()Z",
      (void *)com_android_nfc_NativeP2pDevice_doConnect},
   {"doDisconnect", "()Z",
      (void *)com_android_nfc_NativeP2pDevice_doDisconnect},
   {"doTransceive", "([B)[B",
      (void *)com_android_nfc_NativeP2pDevice_doTransceive},
   {"doReceive", "()[B",
      (void *)com_android_nfc_NativeP2pDevice_doReceive},
   {"doSend", "([B)Z",
      (void *)com_android_nfc_NativeP2pDevice_doSend},
};

int register_com_android_nfc_NativeP2pDevice(JNIEnv *e)
{
   return jniRegisterNativeMethods(e,
      "com/android/nfc/dhimpl/NativeP2pDevice",
      gMethods, NELEM(gMethods));
}

} // namepspace android
