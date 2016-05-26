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

#include "com_android_nfc.h"

static phNfc_sData_t *com_android_nfc_jni_transceive_buffer;
static phNfc_sData_t *com_android_nfc_jni_ioctl_buffer;
static phNfc_sRemoteDevInformation_t* SecureElementInfo;
static int secureElementHandle;
extern void                 *gHWRef;
static int SecureElementTech;
extern uint8_t device_connected_flag;

namespace android {

static void com_android_nfc_jni_ioctl_callback ( void*            pContext,
                                            phNfc_sData_t*   Outparam_Cb,
                                            NFCSTATUS        status)
{
    struct nfc_jni_callback_data * pContextData =  (struct nfc_jni_callback_data*)pContext;

    if (status == NFCSTATUS_SUCCESS )
    {
        LOG_CALLBACK("> IOCTL successful",status);
    }
    else
    {
        LOG_CALLBACK("> IOCTL error",status);
    }

   com_android_nfc_jni_ioctl_buffer = Outparam_Cb;
   pContextData->status = status;
   sem_post(&pContextData->sem);
}

static void com_android_nfc_jni_transceive_callback(void *pContext,
   phLibNfc_Handle handle, phNfc_sData_t *pResBuffer, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pContextData =  (struct nfc_jni_callback_data*)pContext;

   LOG_CALLBACK("com_android_nfc_jni_transceive_callback", status);

   com_android_nfc_jni_transceive_buffer = pResBuffer;
   pContextData->status = status;
   sem_post(&pContextData->sem);
}


static void com_android_nfc_jni_connect_callback(void *pContext,
                                            phLibNfc_Handle hRemoteDev,
                                            phLibNfc_sRemoteDevInformation_t *psRemoteDevInfo, NFCSTATUS status)
{
   struct nfc_jni_callback_data * pContextData =  (struct nfc_jni_callback_data*)pContext;

   LOG_CALLBACK("com_android_nfc_jni_connect_callback", status);

   pContextData->status = status;
   sem_post(&pContextData->sem);
}

static void com_android_nfc_jni_disconnect_callback(void *pContext,
                                               phLibNfc_Handle hRemoteDev,
                                               NFCSTATUS status)
{
   struct nfc_jni_callback_data * pContextData =  (struct nfc_jni_callback_data*)pContext;

   LOG_CALLBACK("com_android_nfc_jni_disconnect_callback", status);

   pContextData->status = status;
   sem_post(&pContextData->sem);
}

/* Set Secure Element mode callback*/
static void com_android_nfc_jni_smartMX_setModeCb (void*            pContext,
                                                            phLibNfc_Handle  hSecureElement,
                                              NFCSTATUS        status)
{
    struct nfc_jni_callback_data * pContextData =  (struct nfc_jni_callback_data*)pContext;

    if(status==NFCSTATUS_SUCCESS)
    {
        LOG_CALLBACK("SE Set Mode is Successful",status);
        TRACE("SE Handle: %lu", hSecureElement);
    }
    else
    {
        LOG_CALLBACK("SE Set Mode is failed\n ",status);
  }

   pContextData->status = status;
   sem_post(&pContextData->sem);
}

static void com_android_nfc_jni_open_secure_element_notification_callback(void *pContext,
                                                                     phLibNfc_RemoteDevList_t *psRemoteDevList,
                                                                     uint8_t uNofRemoteDev,
                                                                     NFCSTATUS status)
{
   struct nfc_jni_callback_data * pContextData =  (struct nfc_jni_callback_data*)pContext;
   NFCSTATUS ret;
   int i;
   JNIEnv *e = nfc_get_env();

   if(status == NFCSTATUS_DESELECTED)
   {
      LOG_CALLBACK("com_android_nfc_jni_open_secure_element_notification_callback: Target deselected", status);
   }
   else
   {
      LOG_CALLBACK("com_android_nfc_jni_open_secure_element_notification_callback", status);
      TRACE("Discovered %d secure elements", uNofRemoteDev);

      if(status == NFCSTATUS_MULTIPLE_PROTOCOLS)
      {
         bool foundHandle = false;
         TRACE("Multiple Protocol supported\n");
         for (i=0; i<uNofRemoteDev; i++) {
             // Always open the phNfc_eISO14443_A_PICC protocol
             TRACE("Protocol %d handle=%x type=%d", i, psRemoteDevList[i].hTargetDev,
                     psRemoteDevList[i].psRemoteDevInfo->RemDevType);
             if (psRemoteDevList[i].psRemoteDevInfo->RemDevType == phNfc_eISO14443_A_PICC) {
                 secureElementHandle = psRemoteDevList[i].hTargetDev;
                 foundHandle = true;
             }
         }
         if (!foundHandle) {
             ALOGE("Could not find ISO-DEP secure element");
             status = NFCSTATUS_FAILED;
             goto clean_and_return;
         }
      }
      else
      {
         secureElementHandle = psRemoteDevList->hTargetDev;
      }

      TRACE("Secure Element Handle: 0x%08x", secureElementHandle);

      /* Set type name */
      ScopedLocalRef<jintArray> techList(e, NULL);
      nfc_jni_get_technology_tree(e, psRemoteDevList,uNofRemoteDev, &techList, NULL, NULL);

      // TODO: Should use the "connected" technology, for now use the first
      if ((techList.get() != NULL) && e->GetArrayLength(techList.get()) > 0) {
         e->GetIntArrayRegion(techList.get(), 0, 1, &SecureElementTech);
         TRACE("Store Secure Element Info\n");
         SecureElementInfo = psRemoteDevList->psRemoteDevInfo;

         TRACE("Discovered secure element: tech=%d", SecureElementTech);
      }
      else {
         ALOGE("Discovered secure element, but could not resolve tech");
         status = NFCSTATUS_FAILED;
      }
   }

clean_and_return:
   pContextData->status = status;
   sem_post(&pContextData->sem);
}


static jint com_android_nfc_NativeNfcSecureElement_doOpenSecureElementConnection(JNIEnv *e, jobject o)
{
   NFCSTATUS ret;
   int semResult;
   jint errorCode = EE_ERROR_INIT;

   phLibNfc_SE_List_t SE_List[PHLIBNFC_MAXNO_OF_SE];
   uint8_t i, No_SE = PHLIBNFC_MAXNO_OF_SE, SmartMX_index=0, SmartMX_detected = 0;
   phLibNfc_sADD_Cfg_t discovery_cfg;
   phLibNfc_Registry_Info_t registry_info;
   phNfc_sData_t        InParam;
   phNfc_sData_t        OutParam;
   uint8_t              ExternalRFDetected[3] = {0x00, 0xFC, 0x01};
   uint8_t              GpioGetValue[3] = {0x00, 0xF8, 0x2B};
   uint8_t              GpioSetValue[4];
   uint8_t              gpioValue;
   uint8_t              Output_Buff[10];
   uint8_t              reg_value;
   uint8_t              mask_value;
   struct nfc_jni_callback_data cb_data;
   struct nfc_jni_callback_data cb_data_SE_Notification;

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, NULL))
   {
      goto clean_and_return;
   }

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data_SE_Notification, NULL))
   {
      goto clean_and_return;
   }

   /* Registery */
   registry_info.MifareUL = TRUE;
   registry_info.MifareStd = TRUE;
   registry_info.ISO14443_4A = TRUE;
   registry_info.ISO14443_4B = TRUE;
   registry_info.Jewel = TRUE;
   registry_info.Felica = TRUE;
   registry_info.NFC = FALSE;

   CONCURRENCY_LOCK();

   TRACE("Open Secure Element");

   /* Check if NFC device is already connected to a tag or P2P peer */
   if (device_connected_flag == 1)
   {
       ALOGE("Unable to open SE connection, device already connected to a P2P peer or a Tag");
       errorCode = EE_ERROR_LISTEN_MODE;
       goto clean_and_return;
   }

   /* Test if External RF field is detected */
   InParam.buffer = ExternalRFDetected;
   InParam.length = 3;
   OutParam.buffer = Output_Buff;
   TRACE("phLibNfc_Mgt_IoCtl()");
   REENTRANCE_LOCK();
   ret = phLibNfc_Mgt_IoCtl(gHWRef,NFC_MEM_READ,&InParam, &OutParam,com_android_nfc_jni_ioctl_callback, (void *)&cb_data);
   REENTRANCE_UNLOCK();
   if(ret!=NFCSTATUS_PENDING)
   {
      ALOGE("IOCTL status error");
      goto clean_and_return;
   }

   /* Wait for callback response */
   if(sem_wait(&cb_data.sem))
   {
      ALOGE("IOCTL semaphore error");
      goto clean_and_return;
   }

   if(cb_data.status != NFCSTATUS_SUCCESS)
   {
      ALOGE("READ MEM ERROR");
      goto clean_and_return;
   }

   /* Check the value */
   reg_value = com_android_nfc_jni_ioctl_buffer->buffer[0];
   mask_value = reg_value & 0x40;

   if(mask_value == 0x40)
   {
      // There is an external RF field present, fail the open request
      ALOGD("Unable to open SE connection, external RF Field detected");
      errorCode = EE_ERROR_EXT_FIELD;
      goto clean_and_return;
   }

   /* Get Secure Element List */
   TRACE("phLibNfc_SE_GetSecureElementList()");
   ret = phLibNfc_SE_GetSecureElementList( SE_List, &No_SE);
   if (ret == NFCSTATUS_SUCCESS)
   {
      TRACE("\n> Number of Secure Element(s) : %d\n", No_SE);
      /* Display Secure Element information */
      for (i = 0; i<No_SE; i++)
      {
         if (SE_List[i].eSE_Type == phLibNfc_SE_Type_SmartMX)
         {
           TRACE("> SMX detected");
           TRACE("> Secure Element Handle : %d\n", SE_List[i].hSecureElement);
           /* save SMARTMX index */
           SmartMX_detected = 1;
           SmartMX_index = i;
         }
      }

      if(SmartMX_detected)
      {
         REENTRANCE_LOCK();
         TRACE("phLibNfc_RemoteDev_NtfRegister()");
         ret = phLibNfc_RemoteDev_NtfRegister(&registry_info,
                 com_android_nfc_jni_open_secure_element_notification_callback,
                 (void *)&cb_data_SE_Notification);
         REENTRANCE_UNLOCK();
         if(ret != NFCSTATUS_SUCCESS)
         {
            ALOGE("Register Notification error");
            goto clean_and_return;
         }

         /* Set wired mode */
         REENTRANCE_LOCK();
         TRACE("phLibNfc_SE_SetMode: Wired mode");
         ret = phLibNfc_SE_SetMode( SE_List[SmartMX_index].hSecureElement,
                                     phLibNfc_SE_ActModeWired,
                                     com_android_nfc_jni_smartMX_setModeCb,
                                     (void *)&cb_data);
         REENTRANCE_UNLOCK();
         if (ret != NFCSTATUS_PENDING )
         {
            ALOGE("\n> SE Set SmartMX mode ERROR \n" );
            goto clean_and_return;
         }

         /* Wait for callback response */
         if(sem_wait(&cb_data.sem))
         {
            ALOGE("Secure Element opening error");
            goto clean_and_return;
         }

         if(cb_data.status != NFCSTATUS_SUCCESS)
         {
            ALOGE("SE set mode failed");
            goto clean_and_return;
         }

         TRACE("Waiting for notification");
         /* Wait for callback response */
         if(sem_wait(&cb_data_SE_Notification.sem))
         {
            ALOGE("Secure Element opening error");
            goto clean_and_return;
         }

         if(cb_data_SE_Notification.status != NFCSTATUS_SUCCESS &&
                 cb_data_SE_Notification.status != NFCSTATUS_MULTIPLE_PROTOCOLS)
         {
            ALOGE("SE detection failed");
            goto clean_and_return;
         }
         CONCURRENCY_UNLOCK();

         /* Connect Tag */
         CONCURRENCY_LOCK();
         TRACE("phLibNfc_RemoteDev_Connect(SMX)");
         REENTRANCE_LOCK();
         ret = phLibNfc_RemoteDev_Connect(secureElementHandle, com_android_nfc_jni_connect_callback,(void *)&cb_data);
         REENTRANCE_UNLOCK();
         if(ret != NFCSTATUS_PENDING)
         {
            ALOGE("phLibNfc_RemoteDev_Connect(SMX) returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));
            goto clean_and_return;
         }
         TRACE("phLibNfc_RemoteDev_Connect(SMX) returned 0x%04x[%s]", ret, nfc_jni_get_status_name(ret));

         /* Wait for callback response */
         if(sem_wait(&cb_data.sem))
         {
             ALOGE("CONNECT semaphore error");
             goto clean_and_return;
         }

         /* Connect Status */
         if(cb_data.status != NFCSTATUS_SUCCESS)
         {
            ALOGE("Secure Element connect error");
            goto clean_and_return;
         }

         CONCURRENCY_UNLOCK();

         /* Get GPIO information */
         CONCURRENCY_LOCK();
         InParam.buffer = GpioGetValue;
         InParam.length = 3;
         OutParam.buffer = Output_Buff;
         TRACE("phLibNfc_Mgt_IoCtl()- GPIO Get Value");
         REENTRANCE_LOCK();
         ret = phLibNfc_Mgt_IoCtl(gHWRef,NFC_MEM_READ,&InParam, &OutParam,com_android_nfc_jni_ioctl_callback, (void *)&cb_data);
         REENTRANCE_UNLOCK();
         if(ret!=NFCSTATUS_PENDING)
         {
             ALOGE("IOCTL status error");
         }

         /* Wait for callback response */
         if(sem_wait(&cb_data.sem))
         {
            ALOGE("IOCTL semaphore error");
            goto clean_and_return;
         }

         if(cb_data.status != NFCSTATUS_SUCCESS)
         {
            ALOGE("READ MEM ERROR");
            goto clean_and_return;
         }

         gpioValue = com_android_nfc_jni_ioctl_buffer->buffer[0];
         TRACE("GpioValue = Ox%02x",gpioValue);

         /* Set GPIO information */
         GpioSetValue[0] = 0x00;
         GpioSetValue[1] = 0xF8;
         GpioSetValue[2] = 0x2B;
         GpioSetValue[3] = (gpioValue | 0x40);

         TRACE("GpioValue to be set = Ox%02x",GpioSetValue[3]);

         for(i=0;i<4;i++)
         {
             TRACE("0x%02x",GpioSetValue[i]);
         }

         InParam.buffer = GpioSetValue;
         InParam.length = 4;
         OutParam.buffer = Output_Buff;
         TRACE("phLibNfc_Mgt_IoCtl()- GPIO Set Value");
         REENTRANCE_LOCK();
         ret = phLibNfc_Mgt_IoCtl(gHWRef,NFC_MEM_WRITE,&InParam, &OutParam,com_android_nfc_jni_ioctl_callback, (void *)&cb_data);
         REENTRANCE_UNLOCK();
         if(ret!=NFCSTATUS_PENDING)
         {
             ALOGE("IOCTL status error");
             goto clean_and_return;
         }

         /* Wait for callback response */
         if(sem_wait(&cb_data.sem))
         {
            ALOGE("IOCTL semaphore error");
            goto clean_and_return;
         }

         if(cb_data.status != NFCSTATUS_SUCCESS)
         {
            ALOGE("READ MEM ERROR");
            goto clean_and_return;
         }
         CONCURRENCY_UNLOCK();

         nfc_cb_data_deinit(&cb_data);
         nfc_cb_data_deinit(&cb_data_SE_Notification);

         /* Return the Handle of the SecureElement */
         return secureElementHandle;
      }
      else
      {
         ALOGE("phLibNfc_SE_GetSecureElementList(): No SMX detected");
         goto clean_and_return;
      }
  }
  else
  {
      ALOGE("phLibNfc_SE_GetSecureElementList(): Error");
      goto clean_and_return;
  }

clean_and_return:
   nfc_cb_data_deinit(&cb_data);
   nfc_cb_data_deinit(&cb_data_SE_Notification);

   CONCURRENCY_UNLOCK();
   return errorCode;
}


static jboolean com_android_nfc_NativeNfcSecureElement_doDisconnect(JNIEnv *e, jobject o, jint handle)
{
   jclass cls;
   jfieldID f;
   NFCSTATUS status;
   jboolean result = JNI_FALSE;
   phLibNfc_SE_List_t SE_List[PHLIBNFC_MAXNO_OF_SE];
   uint8_t i, No_SE = PHLIBNFC_MAXNO_OF_SE, SmartMX_index=0, SmartMX_detected = 0;
   uint32_t SmartMX_Handle;
   struct nfc_jni_callback_data cb_data;
   phNfc_sData_t    InParam;
   phNfc_sData_t    OutParam;
   uint8_t          Output_Buff[10];
   uint8_t          GpioGetValue[3] = {0x00, 0xF8, 0x2B};
   uint8_t          GpioSetValue[4];
   uint8_t          gpioValue;

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, NULL))
   {
      goto clean_and_return;
   }

   TRACE("Close Secure element function ");

   CONCURRENCY_LOCK();
   /* Disconnect */
   TRACE("Disconnecting from SMX (handle = 0x%x)", handle);
   REENTRANCE_LOCK();
   status = phLibNfc_RemoteDev_Disconnect(handle,
                                          NFC_SMARTMX_RELEASE,
                                          com_android_nfc_jni_disconnect_callback,
                                          (void *)&cb_data);
   REENTRANCE_UNLOCK();
   if(status != NFCSTATUS_PENDING)
   {
      ALOGE("phLibNfc_RemoteDev_Disconnect(SMX) returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));
      goto clean_and_return;
   }
   TRACE("phLibNfc_RemoteDev_Disconnect(SMX) returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));

   /* Wait for callback response */
   if(sem_wait(&cb_data.sem))
   {
       goto clean_and_return;
   }

   /* Disconnect Status */
   if(cb_data.status != NFCSTATUS_SUCCESS)
   {
     ALOGE("\n> Disconnect SE ERROR \n" );
      goto clean_and_return;
   }
   CONCURRENCY_UNLOCK();

   /* Get GPIO information */
   CONCURRENCY_LOCK();
   InParam.buffer = GpioGetValue;
   InParam.length = 3;
   OutParam.buffer = Output_Buff;
   TRACE("phLibNfc_Mgt_IoCtl()- GPIO Get Value");
   REENTRANCE_LOCK();
   status = phLibNfc_Mgt_IoCtl(gHWRef,NFC_MEM_READ,&InParam, &OutParam,com_android_nfc_jni_ioctl_callback, (void *)&cb_data);
   REENTRANCE_UNLOCK();
   if(status!=NFCSTATUS_PENDING)
   {
       ALOGE("IOCTL status error");
       goto clean_and_return;
   }

   /* Wait for callback response */
   if(sem_wait(&cb_data.sem))
   {
      ALOGE("IOCTL semaphore error");
      goto clean_and_return;
   }

   if(cb_data.status != NFCSTATUS_SUCCESS)
   {
      ALOGE("READ MEM ERROR");
      goto clean_and_return;
   }

   gpioValue = com_android_nfc_jni_ioctl_buffer->buffer[0];
   TRACE("GpioValue = Ox%02x",gpioValue);

   /* Set GPIO information */
   GpioSetValue[0] = 0x00;
   GpioSetValue[1] = 0xF8;
   GpioSetValue[2] = 0x2B;
   GpioSetValue[3] = (gpioValue & 0xBF);

   TRACE("GpioValue to be set = Ox%02x",GpioSetValue[3]);

   for(i=0;i<4;i++)
   {
       TRACE("0x%02x",GpioSetValue[i]);
   }

   InParam.buffer = GpioSetValue;
   InParam.length = 4;
   OutParam.buffer = Output_Buff;
   TRACE("phLibNfc_Mgt_IoCtl()- GPIO Set Value");
   REENTRANCE_LOCK();
   status = phLibNfc_Mgt_IoCtl(gHWRef,NFC_MEM_WRITE,&InParam, &OutParam,com_android_nfc_jni_ioctl_callback, (void *)&cb_data);
   REENTRANCE_UNLOCK();
   if(status!=NFCSTATUS_PENDING)
   {
       ALOGE("IOCTL status error");
       goto clean_and_return;
   }

   /* Wait for callback response */
   if(sem_wait(&cb_data.sem))
   {
      ALOGE("IOCTL semaphore error");
      goto clean_and_return;
   }

   if(cb_data.status != NFCSTATUS_SUCCESS)
   {
      ALOGE("READ MEM ERROR");
      goto clean_and_return;
   }

   result = JNI_TRUE;

clean_and_return:
   nfc_cb_data_deinit(&cb_data);

   CONCURRENCY_UNLOCK();
   return result;
}

static jbyteArray com_android_nfc_NativeNfcSecureElement_doTransceive(JNIEnv *e,
   jobject o,jint handle, jbyteArray data)
{
   uint8_t offset = 0;
   uint8_t *buf;
   uint32_t buflen;
   phLibNfc_sTransceiveInfo_t transceive_info;
   jbyteArray result = NULL;
   int res;

   int tech = SecureElementTech;
   NFCSTATUS status;
   struct nfc_jni_callback_data cb_data;

   /* Create the local semaphore */
   if (!nfc_cb_data_init(&cb_data, NULL))
   {
      goto clean_and_return;
   }

   TRACE("Exchange APDU function ");

   CONCURRENCY_LOCK();

   TRACE("Secure Element tech: %d\n", tech);

   buf = (uint8_t *)e->GetByteArrayElements(data, NULL);
   buflen = (uint32_t)e->GetArrayLength(data);

   /* Prepare transceive info structure */
   if(tech == TARGET_TYPE_MIFARE_CLASSIC || tech == TARGET_TYPE_MIFARE_UL)
   {
      offset = 2;
      transceive_info.cmd.MfCmd = (phNfc_eMifareCmdList_t)buf[0];
      transceive_info.addr = (uint8_t)buf[1];
   }
   else if(tech == TARGET_TYPE_ISO14443_4)
   {
      transceive_info.cmd.Iso144434Cmd = phNfc_eIso14443_4_Raw;
      transceive_info.addr = 0;
   }

   transceive_info.sSendData.buffer = buf + offset;
   transceive_info.sSendData.length = buflen - offset;
   transceive_info.sRecvData.buffer = (uint8_t*)malloc(1024);
   transceive_info.sRecvData.length = 1024;

   if(transceive_info.sRecvData.buffer == NULL)
   {
      goto clean_and_return;
   }

   TRACE("phLibNfc_RemoteDev_Transceive(SMX)");
   REENTRANCE_LOCK();
   status = phLibNfc_RemoteDev_Transceive(handle, &transceive_info,
           com_android_nfc_jni_transceive_callback, (void *)&cb_data);
   REENTRANCE_UNLOCK();
   if(status != NFCSTATUS_PENDING)
   {
      ALOGE("phLibNfc_RemoteDev_Transceive(SMX) returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));
      goto clean_and_return;
   }
   TRACE("phLibNfc_RemoteDev_Transceive(SMX) returned 0x%04x[%s]", status, nfc_jni_get_status_name(status));

   /* Wait for callback response */
   if(sem_wait(&cb_data.sem))
   {
       ALOGE("TRANSCEIVE semaphore error");
       goto clean_and_return;
   }

   if(cb_data.status != NFCSTATUS_SUCCESS)
   {
      ALOGE("TRANSCEIVE error");
      goto clean_and_return;
   }

   /* Copy results back to Java */
   result = e->NewByteArray(com_android_nfc_jni_transceive_buffer->length);
   if(result != NULL)
   {
      e->SetByteArrayRegion(result, 0,
              com_android_nfc_jni_transceive_buffer->length,
         (jbyte *)com_android_nfc_jni_transceive_buffer->buffer);
   }

clean_and_return:
   nfc_cb_data_deinit(&cb_data);

   if(transceive_info.sRecvData.buffer != NULL)
   {
      free(transceive_info.sRecvData.buffer);
   }

   e->ReleaseByteArrayElements(data,
      (jbyte *)transceive_info.sSendData.buffer, JNI_ABORT);

   CONCURRENCY_UNLOCK();

   return result;
}

static jbyteArray com_android_nfc_NativeNfcSecureElement_doGetUid(JNIEnv *e, jobject o, jint handle)
{
   TRACE("Get Secure element UID function ");
   jbyteArray SecureElementUid;

   if(handle == secureElementHandle)
   {
      SecureElementUid = e->NewByteArray(SecureElementInfo->RemoteDevInfo.Iso14443A_Info.UidLength);
      e->SetByteArrayRegion(SecureElementUid, 0, SecureElementInfo->RemoteDevInfo.Iso14443A_Info.UidLength,(jbyte *)SecureElementInfo->RemoteDevInfo.Iso14443A_Info.Uid);
      return SecureElementUid;
   }
   else
   {
      return NULL;
   }
}

static jintArray com_android_nfc_NativeNfcSecureElement_doGetTechList(JNIEnv *e, jobject o, jint handle)
{
   jintArray techList;
   TRACE("Get Secure element Type function ");

   if (handle != secureElementHandle) {
      return NULL;
   }
   jintArray result = e->NewIntArray(1);
   e->SetIntArrayRegion(result, 0, 1, &SecureElementTech);
   return result;
}


/*
 * JNI registration.
 */
static JNINativeMethod gMethods[] =
{
   {"doNativeOpenSecureElementConnection", "()I",
      (void *)com_android_nfc_NativeNfcSecureElement_doOpenSecureElementConnection},
   {"doNativeDisconnectSecureElementConnection", "(I)Z",
      (void *)com_android_nfc_NativeNfcSecureElement_doDisconnect},
   {"doTransceive", "(I[B)[B",
      (void *)com_android_nfc_NativeNfcSecureElement_doTransceive},
   {"doGetUid", "(I)[B",
      (void *)com_android_nfc_NativeNfcSecureElement_doGetUid},
   {"doGetTechList", "(I)[I",
      (void *)com_android_nfc_NativeNfcSecureElement_doGetTechList},
};

int register_com_android_nfc_NativeNfcSecureElement(JNIEnv *e)
{
   return jniRegisterNativeMethods(e,
      "com/android/nfc/dhimpl/NativeNfcSecureElement",
      gMethods, NELEM(gMethods));
}

} // namespace android
