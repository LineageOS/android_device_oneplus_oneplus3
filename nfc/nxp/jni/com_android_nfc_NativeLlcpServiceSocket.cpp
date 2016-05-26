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

#include "com_android_nfc.h"

namespace android {

extern void nfc_jni_llcp_transport_socket_err_callback(void*      pContext,
                                                       uint8_t    nErrCode);
/*
 * Callbacks
 */
static void nfc_jni_llcp_accept_socket_callback(void*        pContext,
                                                NFCSTATUS    status)
{
   struct nfc_jni_callback_data * pCallbackData = (struct nfc_jni_callback_data *) pContext;
   LOG_CALLBACK("nfc_jni_llcp_accept_socket_callback", status);

   /* Report the callback status and wake up the caller */
   pCallbackData->status = status;
   sem_post(&pCallbackData->sem);
}


/*
 * Utils
 */

static phLibNfc_Handle getIncomingSocket(nfc_jni_native_monitor_t * pMonitor,
                                                 phLibNfc_Handle hServerSocket)
{
   nfc_jni_listen_data_t * pListenData;
   phLibNfc_Handle pIncomingSocket = NULL;

   /* Look for a pending incoming connection on the current server */
   LIST_FOREACH(pListenData, &pMonitor->incoming_socket_head, entries)
   {
      if (pListenData->pServerSocket == hServerSocket)
      {
         pIncomingSocket = pListenData->pIncomingSocket;
         LIST_REMOVE(pListenData, entries);
         free(pListenData);
         break;
      }
   }

   return pIncomingSocket;
}

/*
 * Methods
 */
static jobject com_NativeLlcpServiceSocket_doAccept(JNIEnv *e, jobject o, jint miu, jint rw, jint linearBufferLength)
{
   NFCSTATUS ret = NFCSTATUS_SUCCESS;
   struct timespec ts;
   phLibNfc_Llcp_sSocketOptions_t sOptions;
   phNfc_sData_t sWorkingBuffer;
   jfieldID f;
   ScopedLocalRef<jclass> clsNativeLlcpSocket(e, NULL);
   jobject clientSocket = NULL;
   struct nfc_jni_callback_data cb_data;
   phLibNfc_Handle hIncomingSocket, hServerSocket;
   nfc_jni_native_monitor_t * pMonitor = nfc_jni_get_monitor();

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, NULL))
   {
      goto clean_and_return;
   }

   /* Get server socket */
   hServerSocket = nfc_jni_get_nfc_socket_handle(e,o);

   /* Set socket options with the socket options of the service */
   sOptions.miu = miu;
   sOptions.rw = rw;

   /* Allocate Working buffer length */
   sWorkingBuffer.buffer = (uint8_t*)malloc((miu*rw)+miu+linearBufferLength);
   sWorkingBuffer.length = (miu*rw)+ miu + linearBufferLength;

   while(cb_data.status != NFCSTATUS_SUCCESS)
   {
      /* Wait for tag Notification */
      pthread_mutex_lock(&pMonitor->incoming_socket_mutex);
      while ((hIncomingSocket = getIncomingSocket(pMonitor, hServerSocket)) == NULL) {
         pthread_cond_wait(&pMonitor->incoming_socket_cond, &pMonitor->incoming_socket_mutex);
      }
      pthread_mutex_unlock(&pMonitor->incoming_socket_mutex);

      /* Accept the incomming socket */
      TRACE("phLibNfc_Llcp_Accept()");
      REENTRANCE_LOCK();
      ret = phLibNfc_Llcp_Accept( hIncomingSocket,
                                  &sOptions,
                                  &sWorkingBuffer,
                                  nfc_jni_llcp_transport_socket_err_callback,
                                  nfc_jni_llcp_accept_socket_callback,
                                  (void*)&cb_data);
      REENTRANCE_UNLOCK();
      if(ret != NFCSTATUS_PENDING)
      {
         // NOTE: This may happen if link went down since incoming socket detected, then
         //       just drop it and start a new accept loop.
         ALOGD("phLibNfc_Llcp_Accept() returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));
         continue;
      }
      TRACE("phLibNfc_Llcp_Accept() returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));

      /* Wait for callback response */
      if(sem_wait(&cb_data.sem))
      {
         ALOGE("Failed to wait for semaphore (errno=0x%08x)", errno);
         goto clean_and_return;
      }

      if(cb_data.status != NFCSTATUS_SUCCESS)
      {
         /* NOTE: Do not generate an error if the accept failed to avoid error in server application */
         ALOGD("Failed to accept incoming socket  0x%04x[%s]", cb_data.status, nfc_jni_get_status_name(cb_data.status));
      }
   }

   /* Create new LlcpSocket object */
   if(nfc_jni_cache_object(e,"com/android/nfc/dhimpl/NativeLlcpSocket",&(clientSocket)) == -1)
   {
      ALOGD("LLCP Socket creation error");
      goto clean_and_return;
   }

   /* Get NativeConnectionOriented class object */
   clsNativeLlcpSocket.reset(e->GetObjectClass(clientSocket));
   if(e->ExceptionCheck())
   {
      ALOGD("LLCP Socket get class object error");
      goto clean_and_return;
   }

   /* Set socket handle */
   f = e->GetFieldID(clsNativeLlcpSocket.get(), "mHandle", "I");
   e->SetIntField(clientSocket, f,(jint)hIncomingSocket);

   /* Set socket MIU */
   f = e->GetFieldID(clsNativeLlcpSocket.get(), "mLocalMiu", "I");
   e->SetIntField(clientSocket, f,(jint)miu);

   /* Set socket RW */
   f = e->GetFieldID(clsNativeLlcpSocket.get(), "mLocalRw", "I");
   e->SetIntField(clientSocket, f,(jint)rw);

   TRACE("socket handle 0x%02x: MIU = %d, RW = %d\n",hIncomingSocket, miu, rw);

clean_and_return:
   nfc_cb_data_deinit(&cb_data);
   return clientSocket;
}

static jboolean com_NativeLlcpServiceSocket_doClose(JNIEnv *e, jobject o)
{
   NFCSTATUS ret;
   phLibNfc_Handle hLlcpSocket;
   nfc_jni_native_monitor_t * pMonitor = nfc_jni_get_monitor();

   TRACE("Close Service socket");

   /* Retrieve socket handle */
   hLlcpSocket = nfc_jni_get_nfc_socket_handle(e,o);

   pthread_mutex_lock(&pMonitor->incoming_socket_mutex);
   /* TODO: implement accept abort */
   pthread_cond_broadcast(&pMonitor->incoming_socket_cond);
   pthread_mutex_unlock(&pMonitor->incoming_socket_mutex);

   REENTRANCE_LOCK();
   ret = phLibNfc_Llcp_Close(hLlcpSocket);
   REENTRANCE_UNLOCK();
   if(ret == NFCSTATUS_SUCCESS)
   {
      TRACE("Close Service socket OK");
      return TRUE;
   }
   else
   {
      ALOGD("Close Service socket KO");
      return FALSE;
   }
}


/*
 * JNI registration.
 */
static JNINativeMethod gMethods[] =
{
   {"doAccept", "(III)Lcom/android/nfc/dhimpl/NativeLlcpSocket;",
      (void *)com_NativeLlcpServiceSocket_doAccept},

   {"doClose", "()Z",
      (void *)com_NativeLlcpServiceSocket_doClose},
};


int register_com_android_nfc_NativeLlcpServiceSocket(JNIEnv *e)
{
   return jniRegisterNativeMethods(e,
      "com/android/nfc/dhimpl/NativeLlcpServiceSocket",
      gMethods, NELEM(gMethods));
}

} // namespace android
