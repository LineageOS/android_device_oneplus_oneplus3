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

static void nfc_jni_disconnect_callback(void*        pContext,
                                               NFCSTATUS    status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_disconnect_callback", status);

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}


static void nfc_jni_connect_callback(void* pContext, uint8_t nErrCode, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_llcp_connect_callback", status);

   if(status == NFCSTATUS_SUCCESS)
   {
      TRACE("Socket connected\n");
   }
   else
   {
      ALOGD("Socket not connected:");
      switch(nErrCode)
      {
         case PHFRINFC_LLCP_DM_OPCODE_SAP_NOT_ACTIVE:
            {
               ALOGD("> SAP NOT ACTIVE\n");
            }break;

         case PHFRINFC_LLCP_DM_OPCODE_SAP_NOT_FOUND:
            {
               ALOGD("> SAP NOT FOUND\n");
            }break;

         case PHFRINFC_LLCP_DM_OPCODE_CONNECT_REJECTED:
            {
               ALOGD("> CONNECT REJECTED\n");
            }break;

         case PHFRINFC_LLCP_DM_OPCODE_CONNECT_NOT_ACCEPTED:
            {
               ALOGD("> CONNECT NOT ACCEPTED\n");
            }break;

         case PHFRINFC_LLCP_DM_OPCODE_SOCKET_NOT_AVAILABLE:
            {
               ALOGD("> SOCKET NOT AVAILABLE\n");
            }break;
      }
   }

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}




static void nfc_jni_receive_callback(void* pContext, NFCSTATUS    status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_llcp_receive_callback", status);

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

static void nfc_jni_send_callback(void *pContext, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_llcp_send_callback", status);

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}

/*
 * Methods
 */
static jboolean com_android_nfc_NativeLlcpSocket_doConnect(JNIEnv *e, jobject o, jint nSap)
{
   NFCSTATUS ret;
   struct timespec ts;
   phLibNfc_Handle hRemoteDevice;
   phLibNfc_Handle hLlcpSocket;
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

   TRACE("phLibNfc_Llcp_Connect(%d)",nSap);
   REENTRANCE_LOCK();
   ret = phLibNfc_Llcp_Connect(hRemoteDevice,
                               hLlcpSocket,
                               nSap,
                               nfc_jni_connect_callback,
                               (void*)&cb_data);
   REENTRANCE_UNLOCK();
   if(ret != NFCSTATUS_PENDING)
   {
      ALOGE("phLibNfc_Llcp_Connect(%d) returned 0x%04x[%s]", nSap, ret, nfc_jni_get_status_name(ret));
      goto clean_and_return;
   }
   TRACE("phLibNfc_Llcp_Connect(%d) returned 0x%04x[%s]", nSap, ret, nfc_jni_get_status_name(ret));

   /* Wait for callback response */
   if(sem_wait(&cb_data.sem))
   {
      ALOGE("Failed to wait for semaphore (errno=0x%08x)", errno);
      goto clean_and_return;
   }

   if(cb_data.status != NFCSTATUS_SUCCESS)
   {
      ALOGW("LLCP Connect request failed");
      goto clean_and_return;
   }

   result = JNI_TRUE;

clean_and_return:
   nfc_cb_data_deinit(&cb_data);
   return result;
}

static jboolean com_android_nfc_NativeLlcpSocket_doConnectBy(JNIEnv *e, jobject o, jstring sn)
{
   NFCSTATUS ret;
   struct timespec ts;
   phNfc_sData_t serviceName = {0};
   phLibNfc_Handle hRemoteDevice;
   phLibNfc_Handle hLlcpSocket;
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

   /* Service socket */
   serviceName.buffer = (uint8_t*)e->GetStringUTFChars(sn, NULL);
   serviceName.length = (uint32_t)e->GetStringUTFLength(sn);

   TRACE("phLibNfc_Llcp_ConnectByUri()");
   REENTRANCE_LOCK();
   ret = phLibNfc_Llcp_ConnectByUri(hRemoteDevice,
                                    hLlcpSocket,
                                    &serviceName,
                                    nfc_jni_connect_callback,
                                    (void*)&cb_data);
   REENTRANCE_UNLOCK();
   if(ret != NFCSTATUS_PENDING)
   {
      ALOGE("phLibNfc_Llcp_ConnectByUri() returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));
      goto clean_and_return;
   }
   TRACE("phLibNfc_Llcp_ConnectByUri() returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));

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
   if (serviceName.buffer != NULL) {
      e->ReleaseStringUTFChars(sn, (const char *)serviceName.buffer);
   }
   nfc_cb_data_deinit(&cb_data);
   return result;
}

static jboolean com_android_nfc_NativeLlcpSocket_doClose(JNIEnv *e, jobject o)
{
   NFCSTATUS ret;
   phLibNfc_Handle hLlcpSocket;

   /* Retrieve socket handle */
   hLlcpSocket = nfc_jni_get_nfc_socket_handle(e,o);

   TRACE("phLibNfc_Llcp_Close()");
   REENTRANCE_LOCK();
   ret = phLibNfc_Llcp_Close(hLlcpSocket);
   REENTRANCE_UNLOCK();
   if(ret != NFCSTATUS_SUCCESS)
   {
      ALOGE("phLibNfc_Llcp_Close() returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));
      return FALSE;
   }
   TRACE("phLibNfc_Llcp_Close() returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));
   return TRUE;
}

static jboolean com_android_nfc_NativeLlcpSocket_doSend(JNIEnv *e, jobject o, jbyteArray  data)
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

   TRACE("phLibNfc_Llcp_Send()");
   REENTRANCE_LOCK();
   ret = phLibNfc_Llcp_Send(hRemoteDevice,
                            hLlcpSocket,
                            &sSendBuffer,
                            nfc_jni_send_callback,
                            (void*)&cb_data);
   REENTRANCE_UNLOCK();
   if(ret != NFCSTATUS_PENDING)
   {
      ALOGE("phLibNfc_Llcp_Send() returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));
      goto clean_and_return;
   }
   TRACE("phLibNfc_Llcp_Send() returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));

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

static jint com_android_nfc_NativeLlcpSocket_doReceive(JNIEnv *e, jobject o, jbyteArray  buffer)
{
   NFCSTATUS ret;
   struct timespec ts;
   phLibNfc_Handle hRemoteDevice;
   phLibNfc_Handle hLlcpSocket;
   phNfc_sData_t sReceiveBuffer = {NULL, 0};
   struct nfc_jni_callback_data cb_data;
   jint result = -1;

   /* Retrieve handles */
   hRemoteDevice = nfc_jni_get_p2p_device_handle(e,o);
   hLlcpSocket = nfc_jni_get_nfc_socket_handle(e,o);

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, NULL))
   {
      goto clean_and_return;
   }

   sReceiveBuffer.buffer = (uint8_t*)e->GetByteArrayElements(buffer, NULL);
   sReceiveBuffer.length = (uint32_t)e->GetArrayLength(buffer);

   TRACE("phLibNfc_Llcp_Recv()");
   REENTRANCE_LOCK();
   ret = phLibNfc_Llcp_Recv(hRemoteDevice,
                            hLlcpSocket,
                            &sReceiveBuffer,
                            nfc_jni_receive_callback,
                            (void*)&cb_data);
   REENTRANCE_UNLOCK();
   if(ret == NFCSTATUS_PENDING)
   {
      /* Wait for callback response */
      if(sem_wait(&cb_data.sem))
      {
         ALOGE("Failed to wait for semaphore (errno=0x%08x)", errno);
         goto clean_and_return;
      }

      if(cb_data.status == NFCSTATUS_SUCCESS)
      {
         result = sReceiveBuffer.length;
      }
   }
   else if (ret == NFCSTATUS_SUCCESS)
   {
      result = sReceiveBuffer.length;
   }
   else
   {
      /* Return status should be either SUCCESS or PENDING */
      ALOGE("phLibNfc_Llcp_Recv() returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));
      goto clean_and_return;
   }
   TRACE("phLibNfc_Llcp_Recv() returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));

clean_and_return:
   if (sReceiveBuffer.buffer != NULL)
   {
      e->ReleaseByteArrayElements(buffer, (jbyte*)sReceiveBuffer.buffer, 0);
   }
   nfc_cb_data_deinit(&cb_data);
   return result;
}

static jint com_android_nfc_NativeLlcpSocket_doGetRemoteSocketMIU(JNIEnv *e, jobject o)
{
   NFCSTATUS ret;
   phLibNfc_Handle hRemoteDevice;
   phLibNfc_Handle hLlcpSocket;
   phLibNfc_Llcp_sSocketOptions_t   remoteSocketOption;

   /* Retrieve handles */
   hRemoteDevice = nfc_jni_get_p2p_device_handle(e,o);
   hLlcpSocket = nfc_jni_get_nfc_socket_handle(e,o);

   TRACE("phLibNfc_Llcp_SocketGetRemoteOptions(MIU)");
   REENTRANCE_LOCK();
   ret  = phLibNfc_Llcp_SocketGetRemoteOptions(hRemoteDevice,
                                               hLlcpSocket,
                                               &remoteSocketOption);
   REENTRANCE_UNLOCK();
   if(ret == NFCSTATUS_SUCCESS)
   {
      TRACE("phLibNfc_Llcp_SocketGetRemoteOptions(MIU) returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));
      return remoteSocketOption.miu;
   }
   else
   {
      ALOGW("phLibNfc_Llcp_SocketGetRemoteOptions(MIU) returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));
      return 0;
   }
}

static jint com_android_nfc_NativeLlcpSocket_doGetRemoteSocketRW(JNIEnv *e, jobject o)
{
   NFCSTATUS ret;
   phLibNfc_Handle hRemoteDevice;
   phLibNfc_Handle hLlcpSocket;
   phLibNfc_Llcp_sSocketOptions_t   remoteSocketOption;

   /* Retrieve handles */
   hRemoteDevice = nfc_jni_get_p2p_device_handle(e,o);
   hLlcpSocket = nfc_jni_get_nfc_socket_handle(e,o);

   TRACE("phLibNfc_Llcp_SocketGetRemoteOptions(RW)");
   REENTRANCE_LOCK();
   ret  = phLibNfc_Llcp_SocketGetRemoteOptions(hRemoteDevice,
                                               hLlcpSocket,
                                               &remoteSocketOption);
   REENTRANCE_UNLOCK();
   if(ret == NFCSTATUS_SUCCESS)
   {
      TRACE("phLibNfc_Llcp_SocketGetRemoteOptions(RW) returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));
      return remoteSocketOption.rw;
   }
   else
   {
      ALOGW("phLibNfc_Llcp_SocketGetRemoteOptions(RW) returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));
      return 0;
   }
}


/*
 * JNI registration.
 */
static JNINativeMethod gMethods[] =
{
   {"doConnect", "(I)Z",
      (void *)com_android_nfc_NativeLlcpSocket_doConnect},

   {"doConnectBy", "(Ljava/lang/String;)Z",
      (void *)com_android_nfc_NativeLlcpSocket_doConnectBy},

   {"doClose", "()Z",
      (void *)com_android_nfc_NativeLlcpSocket_doClose},

   {"doSend", "([B)Z",
      (void *)com_android_nfc_NativeLlcpSocket_doSend},

   {"doReceive", "([B)I",
      (void *)com_android_nfc_NativeLlcpSocket_doReceive},

   {"doGetRemoteSocketMiu", "()I",
      (void *)com_android_nfc_NativeLlcpSocket_doGetRemoteSocketMIU},

   {"doGetRemoteSocketRw", "()I",
      (void *)com_android_nfc_NativeLlcpSocket_doGetRemoteSocketRW},
};


int register_com_android_nfc_NativeLlcpSocket(JNIEnv *e)
{
   return jniRegisterNativeMethods(e,
      "com/android/nfc/dhimpl/NativeLlcpSocket",gMethods, NELEM(gMethods));
}

} // namespace android
