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

#include "com_android_nfc.h"

namespace android {

/*
 * Callbacks
 */

static void nfc_jni_receive_callback(void* pContext, uint8_t ssap, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_receiveFrom_callback", status);

   if(status == NFCSTATUS_SUCCESS)
   {
      pCallbackData->pContext = (void*)ssap;
      TRACE("RECEIVE UI_FRAME FROM SAP %d OK \n", ssap);
   }

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

static void nfc_jni_send_callback(void *pContext, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_sendTo_callback", status);

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

/*
* Methods
*/
static jboolean com_android_nfc_NativeLlcpConnectionlessSocket_doSendTo(JNIEnv *e, jobject o, jint nsap, jbyteArray data)
{
   NFCSTATUS ret;
   struct timespec ts;
   phLibNfc_Handle hRemoteDevice;
   phLibNfc_Handle hLlcpSocket;
   phNfc_sData_t sSendBuffer = {NULL, 0};
   struct nfc_jni_callback_data cb_data;
   jboolean result = JNI_FALSE;

   /* Retrieve handles */
   hRemoteDevice = nfc_jni_get_p2p_device_handle(e,o);
   hLlcpSocket = nfc_jni_get_nfc_socket_handle(e,o);

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, NULL))
   {
      goto clean_and_return;
   }

   sSendBuffer.buffer = (uint8_t*)e->GetByteArrayElements(data, NULL);
   sSendBuffer.length = (uint32_t)e->GetArrayLength(data);

   TRACE("phLibNfc_Llcp_SendTo()");
   REENTRANCE_LOCK();
   ret = phLibNfc_Llcp_SendTo(hRemoteDevice,
                              hLlcpSocket,
                              nsap,
                              &sSendBuffer,
                              nfc_jni_send_callback,
                              (void*)&cb_data);
   REENTRANCE_UNLOCK();
   if(ret != NFCSTATUS_PENDING)
   {
      ALOGE("phLibNfc_Llcp_SendTo() returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));
      goto clean_and_return;
   }
   TRACE("phLibNfc_Llcp_SendTo() returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));

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
   if (sSendBuffer.buffer != NULL)
   {
      e->ReleaseByteArrayElements(data, (jbyte*)sSendBuffer.buffer, JNI_ABORT);
   }
   nfc_cb_data_deinit(&cb_data);
   return result;
}

static jobject com_android_nfc_NativeLlcpConnectionlessSocket_doReceiveFrom(JNIEnv *e, jobject o, jint linkMiu)
{
   NFCSTATUS ret;
   struct timespec ts;
   uint8_t ssap;
   jobject llcpPacket = NULL;
   phLibNfc_Handle hRemoteDevice;
   phLibNfc_Handle hLlcpSocket;
   phNfc_sData_t sReceiveBuffer;
   jclass clsLlcpPacket;
   jfieldID f;
   jbyteArray receivedData = NULL;
   struct nfc_jni_callback_data cb_data;

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, NULL))
   {
      goto clean_and_return;
   }

   /* Create new LlcpPacket object */
   if(nfc_jni_cache_object(e,"com/android/nfc/LlcpPacket",&(llcpPacket)) == -1)
   {
      ALOGE("Find LlcpPacket class error");
      goto clean_and_return;
   }

   /* Get NativeConnectionless class object */
   clsLlcpPacket = e->GetObjectClass(llcpPacket);
   if(e->ExceptionCheck())
   {
      ALOGE("Get Object class error");
      goto clean_and_return;
   }

   /* Retrieve handles */
   hRemoteDevice = nfc_jni_get_p2p_device_handle(e,o);
   hLlcpSocket = nfc_jni_get_nfc_socket_handle(e,o);
   TRACE("phLibNfc_Llcp_RecvFrom(), Socket Handle = 0x%02x, Link LIU = %d", hLlcpSocket, linkMiu);

   sReceiveBuffer.buffer = (uint8_t*)malloc(linkMiu);
   sReceiveBuffer.length = linkMiu;

   REENTRANCE_LOCK();
   ret = phLibNfc_Llcp_RecvFrom(hRemoteDevice,
                                hLlcpSocket,
                                &sReceiveBuffer,
                                nfc_jni_receive_callback,
                                &cb_data);
   REENTRANCE_UNLOCK();
   if(ret != NFCSTATUS_PENDING && ret != NFCSTATUS_SUCCESS)
   {
      ALOGE("phLibNfc_Llcp_RecvFrom() returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));
      goto clean_and_return;
   }
   TRACE("phLibNfc_Llcp_RecvFrom() returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));

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

   ssap = (uint32_t)cb_data.pContext;
   TRACE("Data Received From SSAP = %d\n, length = %d", ssap, sReceiveBuffer.length);

   /* Set Llcp Packet remote SAP */
   f = e->GetFieldID(clsLlcpPacket, "mRemoteSap", "I");
   e->SetIntField(llcpPacket, f,(jbyte)ssap);

   /* Set Llcp Packet Buffer */
   ALOGD("Set LlcpPacket Data Buffer\n");
   f = e->GetFieldID(clsLlcpPacket, "mDataBuffer", "[B");
   receivedData = e->NewByteArray(sReceiveBuffer.length);
   e->SetByteArrayRegion(receivedData, 0, sReceiveBuffer.length,(jbyte *)sReceiveBuffer.buffer);
   e->SetObjectField(llcpPacket, f, receivedData);

clean_and_return:
   nfc_cb_data_deinit(&cb_data);
   return llcpPacket;
}

static jboolean com_android_nfc_NativeLlcpConnectionlessSocket_doClose(JNIEnv *e, jobject o)
{
   NFCSTATUS ret;
   phLibNfc_Handle hLlcpSocket;
   TRACE("Close Connectionless socket");

   /* Retrieve socket handle */
   hLlcpSocket = nfc_jni_get_nfc_socket_handle(e,o);

   TRACE("phLibNfc_Llcp_Close()");
   REENTRANCE_LOCK();
   ret = phLibNfc_Llcp_Close(hLlcpSocket);
   REENTRANCE_UNLOCK();
   if(ret == NFCSTATUS_SUCCESS)
   {
      TRACE("phLibNfc_Llcp_Close() returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));
      return TRUE;
   }
   else
   {
      ALOGE("phLibNfc_Llcp_Close() returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));
      return FALSE;
   }
}


/*
 * JNI registration.
 */
static JNINativeMethod gMethods[] =
{
   {"doSendTo", "(I[B)Z", (void *)com_android_nfc_NativeLlcpConnectionlessSocket_doSendTo},

   {"doReceiveFrom", "(I)Lcom/android/nfc/LlcpPacket;", (void *)com_android_nfc_NativeLlcpConnectionlessSocket_doReceiveFrom},

   {"doClose", "()Z", (void *)com_android_nfc_NativeLlcpConnectionlessSocket_doClose},
};


int register_com_android_nfc_NativeLlcpConnectionlessSocket(JNIEnv *e)
{
   return jniRegisterNativeMethods(e,
      "com/android/nfc/dhimpl/NativeLlcpConnectionlessSocket",
      gMethods, NELEM(gMethods));
}

} // android namespace
