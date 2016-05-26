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
#include <cutils/log.h>
#include <Ala.h>
#include <AlaLib.h>
#include <IChannel.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

pAla_Dwnld_Context_t gpAla_Dwnld_Context=NULL;
extern INT32 gTransceiveTimeout;
#ifdef JCOP3_WR
UINT8 Cmd_Buffer[64*1024];
static INT32 cmd_count = 0;
bool islastcmdLoad;
bool SendBack_cmds = false;
UINT8 *pBuffer;
#endif
BOOLEAN mIsInit;
UINT8 Select_Rsp[1024];
UINT8 Jsbl_RefKey[256];
UINT8 Jsbl_keylen;
#if(NXP_LDR_SVC_VER_2 == TRUE)
UINT8 StoreData[22];
#else
UINT8 StoreData[34];
#endif
int Select_Rsp_Len;
#if(NXP_LDR_SVC_VER_2 == TRUE)
UINT8 lsVersionArr[2];
UINT8 tag42Arr[17];
UINT8 tag45Arr[9];
UINT8 lsExecuteResp[4];
UINT8 AID_ARRAY[22];
INT32 resp_len = 0;
FILE *fAID_MEM = NULL;
FILE *fLS_STATUS = NULL;
UINT8 lsGetStatusArr[2];
tJBL_STATUS (*ls_GetStatus_seqhandler[])(Ala_ImageInfo_t *pContext, tJBL_STATUS status, Ala_TranscieveInfo_t *pInfo)=
{
    ALA_OpenChannel,
    ALA_SelectAla,
    ALA_getAppletLsStatus,
    ALA_CloseChannel,
    NULL
};
#endif
tJBL_STATUS (*Applet_load_seqhandler[])(Ala_ImageInfo_t *pContext, tJBL_STATUS status, Ala_TranscieveInfo_t *pInfo)=
{
    ALA_OpenChannel,
    ALA_SelectAla,
    ALA_StoreData,
    ALA_loadapplet,
    NULL
};

tJBL_STATUS (*Jsblcer_id_seqhandler[])(Ala_ImageInfo_t *pContext, tJBL_STATUS status, Ala_TranscieveInfo_t *pInfo)=
{
    ALA_OpenChannel,
    ALA_SelectAla,
    ALA_CloseChannel,
    NULL
};


/*******************************************************************************
**
** Function:        initialize
**
** Description:     Initialize all member variables.
**                  native: Native data.
**
** Returns:         True if ok.
**
*******************************************************************************/
BOOLEAN initialize (IChannel_t* channel)
{
    static const char fn [] = "Ala_initialize";

    ALOGD ("%s: enter", fn);

    gpAla_Dwnld_Context = (pAla_Dwnld_Context_t)malloc(sizeof(Ala_Dwnld_Context_t));
    if(gpAla_Dwnld_Context != NULL)
    {
        memset((void *)gpAla_Dwnld_Context, 0, (UINT32)sizeof(Ala_Dwnld_Context_t));
    }
    else
    {
        ALOGD("%s: Memory allocation failed", fn);
        return (FALSE);
    }
    gpAla_Dwnld_Context->mchannel = channel;
#if(NXP_LDR_SVC_VER_2 == TRUE)
#ifdef JCOP3_WR
    cmd_count = 0;
    SendBack_cmds = false;
    islastcmdLoad = false;
#endif
    fAID_MEM = fopen(AID_MEM_PATH,"r");

    if(fAID_MEM == NULL)
    {
        ALOGD("%s: AID data file does not exists", fn);
        memcpy(&ArrayOfAIDs[2][1],&SelectAla[0],sizeof(SelectAla));
        ArrayOfAIDs[2][0] = sizeof(SelectAla);
    }
    else
    {
        /*Change is required aidLen = 0x00*/
        UINT8 aidLen = 0x00;
        INT32 wStatus = 0;

    while(!(feof(fAID_MEM)))
    {
        /*the length is not incremented*/
        wStatus = FSCANF_BYTE(fAID_MEM,"%2x",&ArrayOfAIDs[2][aidLen++]);
        if(wStatus == 0)
        {
            ALOGE ("%s: exit: Error during read AID data", fn);
            fclose(fAID_MEM);
            return FALSE;
        }
    }
    ArrayOfAIDs[2][0] = aidLen - 1;
    fclose(fAID_MEM);
    }
    lsExecuteResp[0] = TAG_LSES_RESP;
    lsExecuteResp[1] = TAG_LSES_RSPLEN;
    lsExecuteResp[2] = LS_ABORT_SW1;
    lsExecuteResp[3] = LS_ABORT_SW2;
#endif
#ifdef JCOP3_WR
    pBuffer = Cmd_Buffer;
#endif
    mIsInit = TRUE;
    ALOGD ("%s: exit", fn);
    return (TRUE);
}


/*******************************************************************************
**
** Function:        finalize
**
** Description:     Release all resources.
**
** Returns:         None
**
*******************************************************************************/
void finalize ()
{
    static const char fn [] = "Ala_finalize";
    ALOGD ("%s: enter", fn);
    mIsInit       = FALSE;
    if(gpAla_Dwnld_Context != NULL)
    {
        gpAla_Dwnld_Context->mchannel = NULL;
        free(gpAla_Dwnld_Context);
        gpAla_Dwnld_Context = NULL;
    }
    ALOGD ("%s: exit", fn);
}

/*******************************************************************************
**
** Function:        Perform_ALA
**
** Description:     Performs the ALA download sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
#if(NXP_LDR_SVC_VER_2 == TRUE)
tJBL_STATUS Perform_ALA(const char *name,const char *dest, const UINT8 *pdata,
UINT16 len, UINT8 *respSW)
#else
tJBL_STATUS Perform_ALA(const char *name, const UINT8 *pdata, UINT16 len)
#endif
{
    static const char fn [] = "Perform_ALA";
    static const char Ala_path[] = APPLET_PATH;
    tJBL_STATUS status = STATUS_FAILED;
    ALOGD ("%s: enter; sha-len=%d", fn, len);

    if(mIsInit == false)
    {
        ALOGD ("%s: ALA lib is not initialized", fn);
        status = STATUS_FAILED;
    }
    else if((pdata == NULL) ||
            (len == 0x00))
    {
        ALOGD ("%s: Invalid SHA-data", fn);
    }
    else
    {
        StoreData[0] = STORE_DATA_TAG;
        StoreData[1] = len;
        memcpy(&StoreData[2], pdata, len);
#if(NXP_LDR_SVC_VER_2 == TRUE)
        status = ALA_update_seq_handler(Applet_load_seqhandler, name, dest);
        if((status != STATUS_OK)&&(lsExecuteResp[2] == 0x90)&&
        (lsExecuteResp[3] == 0x00))
        {
            lsExecuteResp[2] = LS_ABORT_SW1;
            lsExecuteResp[3] = LS_ABORT_SW2;
        }
        memcpy(&respSW[0],&lsExecuteResp[0],4);
        ALOGD ("%s: lsExecuteScript Response SW=%2x%2x",fn, lsExecuteResp[2],
        lsExecuteResp[3]);
#else
        status = ALA_update_seq_handler(Applet_load_seqhandler, name);
#endif
    }

    ALOGD("%s: exit; status=0x0%x", fn, status);
    return status;
}
#if(NXP_LDR_SVC_VER_2 == FALSE)
/*******************************************************************************
**
** Function:        GetJsbl_Certificate_ID
**
** Description:     Performs the GetJsbl_Certificate_ID sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS GetJsbl_Certificate_Refkey(UINT8 *pKey, INT32 *pKeylen)
{
    static const char fn [] = "GetJsbl_Certificate_ID";
    tJBL_STATUS status = STATUS_FAILED;
    ALOGD ("%s: enter", fn);

    if(mIsInit == false)
    {
        ALOGD ("%s: ALA lib is not initialized", fn);
        status = STATUS_FAILED;
    }
    else
    {
        status = JsblCerId_seq_handler(Jsblcer_id_seqhandler);
        if(status == STATUS_SUCCESS)
        {
            if(Jsbl_keylen != 0x00)
            {
                *pKeylen = (INT32)Jsbl_keylen;
                memcpy(pKey, Jsbl_RefKey, Jsbl_keylen);
                Jsbl_keylen = 0;
            }
        }
    }

    ALOGD("%s: exit; status=0x0%x", fn, status);
    return status;
}

/*******************************************************************************
**
** Function:        JsblCerId_seq_handler
**
** Description:     Performs get JSBL Certificate Identifier sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS JsblCerId_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo))
{
    static const char fn[] = "JsblCerId_seq_handler";
    UINT16 seq_counter = 0;
    Ala_ImageInfo_t update_info = (Ala_ImageInfo_t )gpAla_Dwnld_Context->Image_info;
    Ala_TranscieveInfo_t trans_info = (Ala_TranscieveInfo_t )gpAla_Dwnld_Context->Transcv_Info;
    tJBL_STATUS status = STATUS_FAILED;
    ALOGD("%s: enter", fn);

    while((seq_handler[seq_counter]) != NULL )
    {
        status = STATUS_FAILED;
        status = (*(seq_handler[seq_counter]))(&update_info, status, &trans_info );
        if(STATUS_SUCCESS != status)
        {
            ALOGE("%s: exiting; status=0x0%X", fn, status);
            break;
        }
        seq_counter++;
    }

    ALOGE("%s: exit; status=0x%x", fn, status);
    return status;
}
#else

/*******************************************************************************
**
** Function:        GetLs_Version
**
** Description:     Performs the GetLs_Version sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS GetLs_Version(UINT8 *pVersion)
{
    static const char fn [] = "GetLs_Version";
    tJBL_STATUS status = STATUS_FAILED;
    ALOGD ("%s: enter", fn);

    if(mIsInit == false)
    {
        ALOGD ("%s: ALA lib is not initialized", fn);
        status = STATUS_FAILED;
    }
    else
    {
        status = GetVer_seq_handler(Jsblcer_id_seqhandler);
        if(status == STATUS_SUCCESS)
        {
            pVersion[0] = 2;
            pVersion[1] = 0;
            memcpy(&pVersion[2], lsVersionArr, sizeof(lsVersionArr));
            ALOGD("%s: GetLsVersion is =0x0%x%x", fn, lsVersionArr[0],lsVersionArr[1]);
        }
    }
    ALOGD("%s: exit; status=0x0%x", fn, status);
    return status;
}
/*******************************************************************************
**
** Function:        Get_LsAppletStatus
**
** Description:     Performs the Get_LsAppletStatus sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS Get_LsAppletStatus(UINT8 *pVersion)
{
    static const char fn [] = "GetLs_Version";
    tJBL_STATUS status = STATUS_FAILED;
    ALOGD ("%s: enter", fn);

    if(mIsInit == false)
    {
        ALOGD ("%s: ALA lib is not initialized", fn);
        status = STATUS_FAILED;
    }
    else
    {
        status = GetLsStatus_seq_handler(ls_GetStatus_seqhandler);
        if(status == STATUS_SUCCESS)
        {
            pVersion[0] = lsGetStatusArr[0];
            pVersion[1] = lsGetStatusArr[1];
            ALOGD("%s: GetLsAppletStatus is =0x0%x%x", fn, lsGetStatusArr[0],lsGetStatusArr[1]);
        }
    }
    ALOGD("%s: exit; status=0x0%x", fn, status);
    return status;
}

/*******************************************************************************
**
** Function:        GetVer_seq_handler
**
** Description:     Performs GetVer_seq_handler sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS GetVer_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t*
        pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo))
{
    static const char fn[] = "GetVer_seq_handler";
    UINT16 seq_counter = 0;
    Ala_ImageInfo_t update_info = (Ala_ImageInfo_t )gpAla_Dwnld_Context->Image_info;
    Ala_TranscieveInfo_t trans_info = (Ala_TranscieveInfo_t )gpAla_Dwnld_Context->Transcv_Info;
    tJBL_STATUS status = STATUS_FAILED;
    ALOGD("%s: enter", fn);

    while((seq_handler[seq_counter]) != NULL )
    {
        status = STATUS_FAILED;
        status = (*(seq_handler[seq_counter]))(&update_info, status, &trans_info );
        if(STATUS_SUCCESS != status)
        {
            ALOGE("%s: exiting; status=0x0%X", fn, status);
            break;
        }
        seq_counter++;
    }

    ALOGE("%s: exit; status=0x%x", fn, status);
    return status;
}

/*******************************************************************************
**
** Function:        GetLsStatus_seq_handler
**
** Description:     Performs GetVer_seq_handler sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS GetLsStatus_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t*
        pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo))
{
    static const char fn[] = "ls_GetStatus_seqhandler";
    UINT16 seq_counter = 0;
    Ala_ImageInfo_t update_info = (Ala_ImageInfo_t )gpAla_Dwnld_Context->Image_info;
    Ala_TranscieveInfo_t trans_info = (Ala_TranscieveInfo_t )gpAla_Dwnld_Context->Transcv_Info;
    tJBL_STATUS status = STATUS_FAILED;
    ALOGD("%s: enter", fn);

    while((seq_handler[seq_counter]) != NULL )
    {
        status = STATUS_FAILED;
        status = (*(seq_handler[seq_counter]))(&update_info, status, &trans_info );
        if(STATUS_SUCCESS != status)
        {
            ALOGE("%s: exiting; status=0x0%X", fn, status);
            break;
        }
        seq_counter++;
    }

    ALOGE("%s: exit; status=0x%x", fn, status);
    return status;
}
#endif
/*******************************************************************************
**
** Function:        ALA_update_seq_handler
**
** Description:     Performs the ALA update sequence handler sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
#if(NXP_LDR_SVC_VER_2 == TRUE)
tJBL_STATUS ALA_update_seq_handler(tJBL_STATUS (*seq_handler[])
        (Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t*
                pInfo), const char *name, const char *dest)
#else
tJBL_STATUS ALA_update_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo), const char *name)
#endif
{
    static const char fn[] = "ALA_update_seq_handler";
    static const char Ala_path[] = APPLET_PATH;
    UINT16 seq_counter = 0;
    Ala_ImageInfo_t update_info = (Ala_ImageInfo_t )gpAla_Dwnld_Context->
        Image_info;
    Ala_TranscieveInfo_t trans_info = (Ala_TranscieveInfo_t )gpAla_Dwnld_Context
    ->Transcv_Info;
    tJBL_STATUS status = STATUS_FAILED;
    ALOGD("%s: enter", fn);

#if(NXP_LDR_SVC_VER_2 == TRUE)
    if(dest != NULL)
    {
        strcat(update_info.fls_RespPath, dest);
        ALOGD("Loader Service response data path/destination: %s", dest);
        update_info.bytes_wrote = 0xAA;
    }
    else
    {
        update_info.bytes_wrote = 0x55;
    }
    if((ALA_UpdateExeStatus(LS_DEFAULT_STATUS))!= TRUE)
    {
        return FALSE;
    }
#endif
    //memcpy(update_info.fls_path, (char*)Ala_path, sizeof(Ala_path));
    strcat(update_info.fls_path, name);
    ALOGD("Selected applet to install is: %s", update_info.fls_path);

    while((seq_handler[seq_counter]) != NULL )
    {
        status = STATUS_FAILED;
        status = (*(seq_handler[seq_counter]))(&update_info, status,
                &trans_info);
        if(STATUS_SUCCESS != status)
        {
            ALOGE("%s: exiting; status=0x0%X", fn, status);
            break;
        }
        seq_counter++;
    }

    ALA_CloseChannel(&update_info, STATUS_FAILED, &trans_info);
    ALOGE("%s: exit; status=0x%x", fn, status);
    return status;

}
/*******************************************************************************
**
** Function:        ALA_OpenChannel
**
** Description:     Creates the logical channel with ala
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_OpenChannel(Ala_ImageInfo_t *Os_info, tJBL_STATUS status,
        Ala_TranscieveInfo_t *pTranscv_Info)
{
    static const char fn[] = "ALA_OpenChannel";
    bool stat = false;
    INT32 recvBufferActualSize = 0;
    IChannel_t *mchannel = gpAla_Dwnld_Context->mchannel;
    Os_info->channel_cnt = 0x00;
    ALOGD("%s: enter", fn);
    if(Os_info == NULL ||
       pTranscv_Info == NULL)
    {
        ALOGD("%s: Invalid parameter", fn);
    }
    else
    {
        pTranscv_Info->timeout = gTransceiveTimeout;
        pTranscv_Info->sSendlength = (INT32)sizeof(OpenChannel);
        pTranscv_Info->sRecvlength = 1024;//(INT32)sizeof(INT32);
        memcpy(pTranscv_Info->sSendData, OpenChannel, pTranscv_Info->sSendlength);

        ALOGD("%s: Calling Secure Element Transceive", fn);
        stat = mchannel->transceive (pTranscv_Info->sSendData,
                                pTranscv_Info->sSendlength,
                                pTranscv_Info->sRecvData,
                                pTranscv_Info->sRecvlength,
                                recvBufferActualSize,
                                pTranscv_Info->timeout);
        if(stat != TRUE &&
           (recvBufferActualSize < 0x03))
        {
#if(NXP_LDR_SVC_VER_2 == TRUE)
            if(recvBufferActualSize == 0x02)
            memcpy(&lsExecuteResp[2],
                    &pTranscv_Info->sRecvData[recvBufferActualSize-2],2);
#endif
            status = STATUS_FAILED;
            ALOGE("%s: SE transceive failed status = 0x%X", fn, status);
        }
        else if(((pTranscv_Info->sRecvData[recvBufferActualSize-2] != 0x90) &&
               (pTranscv_Info->sRecvData[recvBufferActualSize-1] != 0x00)))
        {
#if(NXP_LDR_SVC_VER_2 == TRUE)
            memcpy(&lsExecuteResp[2],
                    &pTranscv_Info->sRecvData[recvBufferActualSize-2],2);
#endif
            status = STATUS_FAILED;
            ALOGE("%s: invalid response = 0x%X", fn, status);
        }
        else
        {
            UINT8 cnt = Os_info->channel_cnt;
            Os_info->Channel_Info[cnt].channel_id = pTranscv_Info->sRecvData[recvBufferActualSize-3];
            Os_info->Channel_Info[cnt].isOpend = true;
            Os_info->channel_cnt++;
            status = STATUS_OK;
        }
    }
    ALOGE("%s: exit; status=0x%x", fn, status);
    return status;
}
/*******************************************************************************
**
** Function:        ALA_SelectAla
**
** Description:     Creates the logical channel with ala
**                  Channel_id will be used for any communication with Ala
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_SelectAla(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
{
    static const char fn[] = "ALA_SelectAla";
    bool stat = false;
    INT32 recvBufferActualSize = 0;
    IChannel_t *mchannel = gpAla_Dwnld_Context->mchannel;
#if(NXP_LDR_SVC_VER_2 == TRUE)
    UINT8 selectCnt = 3;
#endif
    ALOGD("%s: enter", fn);

    if(Os_info == NULL ||
       pTranscv_Info == NULL)
    {
        ALOGD("%s: Invalid parameter", fn);
    }
    else
    {
        pTranscv_Info->sSendData[0] = Os_info->Channel_Info[0].channel_id;
        pTranscv_Info->timeout = gTransceiveTimeout;
        pTranscv_Info->sSendlength = (INT32)sizeof(SelectAla);
        pTranscv_Info->sRecvlength = 1024;//(INT32)sizeof(INT32);

#if(NXP_LDR_SVC_VER_2 == TRUE)
    while((selectCnt--) > 0)
    {
        memcpy(&(pTranscv_Info->sSendData[1]), &ArrayOfAIDs[selectCnt][2],
                ((ArrayOfAIDs[selectCnt][0])-1));
        pTranscv_Info->sSendlength = (INT32)ArrayOfAIDs[selectCnt][0];
        /*If NFC/SPI Deinitialize requested*/
#else
        memcpy(&(pTranscv_Info->sSendData[1]), &SelectAla[1], sizeof(SelectAla)-1);
#endif
        ALOGD("%s: Calling Secure Element Transceive with Loader service AID", fn);

        stat = mchannel->transceive (pTranscv_Info->sSendData,
                                pTranscv_Info->sSendlength,
                                pTranscv_Info->sRecvData,
                                pTranscv_Info->sRecvlength,
                                recvBufferActualSize,
                                pTranscv_Info->timeout);
        if(stat != TRUE &&
           (recvBufferActualSize == 0x00))
        {
            status = STATUS_FAILED;
            ALOGE("%s: SE transceive failed status = 0x%X", fn, status);
#if(NXP_LDR_SVC_VER_2 == TRUE)
            break;
#endif
        }
        else if(((pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x90) &&
               (pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x00)))
        {
            status = Process_SelectRsp(pTranscv_Info->sRecvData, (recvBufferActualSize-2));
            if(status != STATUS_OK)
            {
                ALOGE("%s: Select Ala Rsp doesnt have a valid key; status = 0x%X", fn, status);
            }
#if(NXP_LDR_SVC_VER_2 == TRUE)
           /*If AID is found which is successfully selected break while loop*/
           if(status == STATUS_OK)
           {
               UINT8 totalLen = ArrayOfAIDs[selectCnt][0];
               UINT8 cnt  = 0;
               INT32 wStatus= 0;
               status = STATUS_FAILED;

               fAID_MEM = fopen(AID_MEM_PATH,"w+");

               if(fAID_MEM == NULL)
               {
                   ALOGE("Error opening AID data file for writing: %s",
                           strerror(errno));
                   return status;
               }
               while(cnt <= totalLen)
               {
                   wStatus = fprintf(fAID_MEM, "%02x",
                           ArrayOfAIDs[selectCnt][cnt++]);
                   if(wStatus != 2)
                   {
                      ALOGE("%s: Error writing AID data to AID_MEM file: %s",
                              fn, strerror(errno));
                      break;
                   }
               }
               if(wStatus == 2)
                   status = STATUS_OK;
               fclose(fAID_MEM);
               break;
           }
#endif
        }
#if(NXP_LDR_SVC_VER_2 == TRUE)
        else if(((pTranscv_Info->sRecvData[recvBufferActualSize-2] != 0x90)))
        {
            /*Copy the response SW in failure case*/
            memcpy(&lsExecuteResp[2], &(pTranscv_Info->
                    sRecvData[recvBufferActualSize-2]),2);
        }
#endif
        else
        {
            status = STATUS_FAILED;
        }
#if(NXP_LDR_SVC_VER_2 == TRUE)
    }
#endif
    }
    ALOGE("%s: exit; status=0x%x", fn, status);
    return status;
}

/*******************************************************************************
**
** Function:        ALA_StoreData
**
** Description:     It is used to provide the ALA with an Unique
**                  Identifier of the Application that has triggered the ALA script.
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_StoreData(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
{
    static const char fn[] = "ALA_StoreData";
    bool stat = false;
    INT32 recvBufferActualSize = 0;
    INT32 xx=0, len = 0;
    IChannel_t *mchannel = gpAla_Dwnld_Context->mchannel;
    ALOGD("%s: enter", fn);
    if(Os_info == NULL ||
       pTranscv_Info == NULL)
    {
        ALOGD("%s: Invalid parameter", fn);
    }
    else
    {
        len = StoreData[1] + 2;  //+2 offset is for tag value and length byte
        pTranscv_Info->sSendData[xx++] = STORE_DATA_CLA | (Os_info->Channel_Info[0].channel_id);
        pTranscv_Info->sSendData[xx++] = STORE_DATA_INS;
        pTranscv_Info->sSendData[xx++] = 0x00; //P1
        pTranscv_Info->sSendData[xx++] = 0x00; //P2
        pTranscv_Info->sSendData[xx++] = len;
        memcpy(&(pTranscv_Info->sSendData[xx]), StoreData, len);
        pTranscv_Info->timeout = gTransceiveTimeout;
        pTranscv_Info->sSendlength = (INT32)(xx + sizeof(StoreData));
        pTranscv_Info->sRecvlength = 1024;

        ALOGD("%s: Calling Secure Element Transceive", fn);
        stat = mchannel->transceive (pTranscv_Info->sSendData,
                                pTranscv_Info->sSendlength,
                                pTranscv_Info->sRecvData,
                                pTranscv_Info->sRecvlength,
                                recvBufferActualSize,
                                pTranscv_Info->timeout);
        if((stat != TRUE) &&
           (recvBufferActualSize == 0x00))
        {
            status = STATUS_FAILED;
            ALOGE("%s: SE transceive failed status = 0x%X", fn, status);
        }
        else if((pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x90) &&
                (pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x00))
        {
            ALOGE("STORE CMD is successful");
            status = STATUS_SUCCESS;
        }
        else
        {
#if(NXP_LDR_SVC_VER_2 == TRUE)
            /*Copy the response SW in failure case*/
            memcpy(&lsExecuteResp[2], &(pTranscv_Info->sRecvData
                    [recvBufferActualSize-2]),2);
#endif
            status = STATUS_FAILED;
        }
    }
    ALOGE("%s: exit; status=0x%x", fn, status);
    return status;
}

/*******************************************************************************
**
** Function:        ALA_loadapplet
**
** Description:     Reads the script from the file and sent to Ala
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_loadapplet(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
{
    static const char fn [] = "ALA_loadapplet";
    BOOLEAN stat = FALSE;
    int wResult, size =0;
    INT32 wIndex,wCount=0;
    INT32 wLen = 0;
    INT32 recvBufferActualSize = 0;
    UINT8 temp_buf[1024];
    UINT8 len_byte=0, offset =0;
    IChannel_t *mchannel = gpAla_Dwnld_Context->mchannel;
    Os_info->bytes_read = 0;
#if(NXP_LDR_SVC_VER_2 == TRUE)
    BOOLEAN reachEOFCheck = FALSE;
    tJBL_STATUS tag40_found = STATUS_FAILED;
    if(Os_info->bytes_wrote == 0xAA)
    {
        Os_info->fResp = fopen(Os_info->fls_RespPath, "a+");
        if(Os_info->fResp == NULL)
        {
            ALOGE("Error opening response recording file <%s> for reading: %s",
            Os_info->fls_path, strerror(errno));
            return status;
        }
        ALOGD("%s: Response OUT FILE path is successfully created", fn);
    }
    else
    {
        ALOGD("%s: Response Out file is optional as per input", fn);
    }
#endif
    ALOGD("%s: enter", fn);
    if(Os_info == NULL ||
       pTranscv_Info == NULL)
    {
        ALOGE("%s: invalid parameter", fn);
        return status;
    }
    Os_info->fp = fopen(Os_info->fls_path, "r");

    if (Os_info->fp == NULL) {
        ALOGE("Error opening OS image file <%s> for reading: %s",
                    Os_info->fls_path, strerror(errno));
        return status;
    }
    wResult = fseek(Os_info->fp, 0L, SEEK_END);
    if (wResult) {
        ALOGE("Error seeking end OS image file %s", strerror(errno));
        goto exit;
    }
    Os_info->fls_size = ftell(Os_info->fp);
    ALOGE("fls_size=%d", Os_info->fls_size);
    if (Os_info->fls_size < 0) {
        ALOGE("Error ftelling file %s", strerror(errno));
        goto exit;
    }
    wResult = fseek(Os_info->fp, 0L, SEEK_SET);
    if (wResult) {
        ALOGE("Error seeking start image file %s", strerror(errno));
        goto exit;
    }
#if(NXP_LDR_SVC_VER_2 == TRUE)
    status = ALA_Check_KeyIdentifier(Os_info, status, pTranscv_Info,
        NULL, STATUS_FAILED, 0);
#else
    status = ALA_Check_KeyIdentifier(Os_info, status, pTranscv_Info);
#endif
    if(status != STATUS_OK)
    {
        goto exit;
    }
    while(!feof(Os_info->fp) &&
            (Os_info->bytes_read < Os_info->fls_size))
    {
        len_byte = 0x00;
        offset = 0;
#if(NXP_LDR_SVC_VER_2 == TRUE)
        /*Check if the certificate/ is verified or not*/
        if(status != STATUS_OK)
        {
            goto exit;
        }
#endif
        memset(temp_buf, 0, sizeof(temp_buf));
        ALOGE("%s; Start of line processing", fn);
        status = ALA_ReadScript(Os_info, temp_buf);
        if(status != STATUS_OK)
        {
            goto exit;
        }
#if(NXP_LDR_SVC_VER_2 == TRUE)
        else if(status == STATUS_OK)
        {
            /*Reset the flag in case further commands exists*/
            reachEOFCheck = FALSE;
        }
#endif
        if(temp_buf[offset] == TAG_ALA_CMD_ID)
        {
            /*
             * start sending the packet to Ala
             * */
            offset = offset+1;
            len_byte = Numof_lengthbytes(&temp_buf[offset], &wLen);
#if(NXP_LDR_SVC_VER_2 == TRUE)
            /*If the len data not present or
             * len is less than or equal to 32*/
            if((len_byte == 0)||(wLen <= 32))
#else
            if((len_byte == 0))
#endif
            {
                ALOGE("Invalid length zero");
#if(NXP_LDR_SVC_VER_2 == TRUE)
                goto exit;
#else
                return status;
#endif
            }
            else
            {
#if(NXP_LDR_SVC_VER_2 == TRUE)
                tag40_found = STATUS_OK;
#endif
                offset = offset+len_byte;
                pTranscv_Info->sSendlength = wLen;
                memcpy(pTranscv_Info->sSendData, &temp_buf[offset], wLen);
            }
#if(NXP_LDR_SVC_VER_2 == TRUE)
            status = ALA_SendtoAla(Os_info, status, pTranscv_Info, LS_Comm);
#else
            status = ALA_SendtoAla(Os_info, status, pTranscv_Info);
#endif
            if(status != STATUS_OK)
            {

#if(NXP_LDR_SVC_VER_2 == TRUE)
                /*When the switching of LS 6320 case*/
                if(status == STATUS_FILE_NOT_FOUND)
                {
                    /*When 6320 occurs close the existing channels*/
                    ALA_CloseChannel(Os_info,status,pTranscv_Info);

                    status = STATUS_FAILED;
                    status = ALA_OpenChannel(Os_info,status,pTranscv_Info);
                    if(status == STATUS_OK)
                    {
                        ALOGD("SUCCESS:Post Switching LS open channel");
                        status = STATUS_FAILED;
                        status = ALA_SelectAla(Os_info,status,pTranscv_Info);
                        if(status == STATUS_OK)
                        {
                            ALOGD("SUCCESS:Post Switching LS select");
                            status = STATUS_FAILED;
                            status = ALA_StoreData(Os_info,status,pTranscv_Info);
                            if(status == STATUS_OK)
                            {
                                /*Enable certificate and signature verification*/
                                tag40_found = STATUS_OK;
                                lsExecuteResp[2] = 0x90;
                                lsExecuteResp[3] = 0x00;
                                reachEOFCheck = TRUE;
                                continue;
                            }
                            ALOGE("Post Switching LS store data failure");
                        }
                        ALOGE("Post Switching LS select failure");
                    }
                    ALOGE("Post Switching LS failure");
                }
                ALOGE("Sending packet to ala failed");
                goto exit;
#else
                return status;
#endif
            }
        }
#if(NXP_LDR_SVC_VER_2 == TRUE)
        else if((temp_buf[offset] == (0x7F))&&(temp_buf[offset+1] == (0x21)))
        {
            ALOGD("TAGID: Encountered again certificate tag 7F21");
            if(tag40_found == STATUS_OK)
            {
            ALOGD("2nd Script processing starts with reselect");
            status = STATUS_FAILED;
            status = ALA_SelectAla(Os_info,status,pTranscv_Info);
            if(status == STATUS_OK)
            {
                ALOGD("2nd Script select success next store data command");
                status = STATUS_FAILED;
                status = ALA_StoreData(Os_info,status,pTranscv_Info);
                if(status == STATUS_OK)
                {
                    ALOGD("2nd Script store data success next certificate verification");
                    offset = offset+2;
                    len_byte = Numof_lengthbytes(&temp_buf[offset], &wLen);
                    status = ALA_Check_KeyIdentifier(Os_info, status, pTranscv_Info,
                    temp_buf, STATUS_OK, wLen+len_byte+2);
                    }
                }
                /*If the certificate and signature is verified*/
                if(status == STATUS_OK)
                {
                    /*If the certificate is verified for 6320 then new
                     * script starts*/
                    tag40_found = STATUS_FAILED;
                }
                /*If the certificate or signature verification failed*/
                else{
                  goto exit;
                }
            }
            /*Already certificate&Sginature verified previously skip 7f21& tag 60*/
            else
            {
                memset(temp_buf, 0, sizeof(temp_buf));
                status = ALA_ReadScript(Os_info, temp_buf);
                if(status != STATUS_OK)
                {
                    ALOGE("%s; Next Tag has to TAG 60 not found", fn);
                    goto exit;
                }
                if(temp_buf[offset] == TAG_JSBL_HDR_ID)
                continue;
                else
                    goto exit;
            }
        }
#endif
        else
        {
            /*
             * Invalid packet received in between stop processing packet
             * return failed status
             * */
            status = STATUS_FAILED;
            break;
        }
    }
#if(NXP_LDR_SVC_VER_2 == TRUE)
    if(Os_info->bytes_wrote == 0xAA)
    {
        fclose(Os_info->fResp);
    }
    ALA_UpdateExeStatus(LS_SUCCESS_STATUS);
#endif
    wResult = fclose(Os_info->fp);
    ALOGE("%s exit;End of Load Applet; status=0x%x",fn, status);
    return status;
exit:
    wResult = fclose(Os_info->fp);
#if(NXP_LDR_SVC_VER_2 == TRUE)
    if(Os_info->bytes_wrote == 0xAA)
    {
        fclose(Os_info->fResp);
    }
    /*Script ends with SW 6320 and reached END OF FILE*/
    if(reachEOFCheck == TRUE)
    {
       status = STATUS_OK;
       ALA_UpdateExeStatus(LS_SUCCESS_STATUS);
    }
#endif
    ALOGE("%s close fp and exit; status= 0x%X", fn,status);
    return status;

}
/*******************************************************************************
**
** Function:        ALA_ProcessResp
**
** Description:     Process the response packet received from Ala
**
** Returns:         Success if ok.
**
*******************************************************************************/
#if(NXP_LDR_SVC_VER_2 == TRUE)
tJBL_STATUS ALA_Check_KeyIdentifier(Ala_ImageInfo_t *Os_info, tJBL_STATUS status,
   Ala_TranscieveInfo_t *pTranscv_Info, UINT8* temp_buf, tJBL_STATUS flag,
   INT32 wNewLen)
#else
tJBL_STATUS ALA_Check_KeyIdentifier(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
#endif
{
    static const char fn[] = "ALA_Check_KeyIdentifier";
#if(NXP_LDR_SVC_VER_2 == TRUE)
    UINT16 offset = 0x00, len_byte=0;
#else
    UINT8 offset = 0x00, len_byte=0;
#endif
    tJBL_STATUS key_found = STATUS_FAILED;
    status = STATUS_FAILED;
    UINT8 read_buf[1024];
    bool stat = false;
    INT32 wLen, recvBufferActualSize=0;
    IChannel_t *mchannel = gpAla_Dwnld_Context->mchannel;
#if(NXP_LDR_SVC_VER_2 == TRUE)
    UINT8 certf_found = STATUS_FAILED;
    UINT8 sign_found = STATUS_FAILED;
#endif
    ALOGD("%s: enter", fn);

#if(NXP_LDR_SVC_VER_2 == TRUE)
    while(!feof(Os_info->fp) &&
            (Os_info->bytes_read < Os_info->fls_size))
    {
        offset = 0x00;
        wLen = 0;
        if(flag == STATUS_OK)
        {
            /*If the 7F21 TAG is already read: After TAG 40*/
            memcpy(read_buf, temp_buf, wNewLen);
            status = STATUS_OK;
            flag   = STATUS_FAILED;
        }
        else
        {
            /*If the 7F21 TAG is not read: Before TAG 40*/
            status = ALA_ReadScript(Os_info, read_buf);
        }
        if(status != STATUS_OK)
            return status;
        if(STATUS_OK == Check_Complete_7F21_Tag(Os_info,pTranscv_Info,
                read_buf, &offset))
        {
            ALOGD("%s: Certificate is verified", fn);
            certf_found = STATUS_OK;
            break;
        }
        /*The Loader Service Client ignores all subsequent commands starting by tag
         * �7F21� or tag �60� until the first command starting by tag �40� is found*/
        else if(((read_buf[offset] == TAG_ALA_CMD_ID)&&(certf_found != STATUS_OK)))
        {
            ALOGE("%s: NOT FOUND Root entity identifier's certificate", fn);
            status = STATUS_FAILED;
            return status;
        }
    }
#endif
#if(NXP_LDR_SVC_VER_2 == TRUE)
    memset(read_buf, 0, sizeof(read_buf));
    if(certf_found == STATUS_OK)
    {
#else
        while(!feof(Os_info->fp))
        {
#endif
        offset  = 0x00;
        wLen    = 0;
        status  = ALA_ReadScript(Os_info, read_buf);
        if(status != STATUS_OK)
            return status;
#if(NXP_LDR_SVC_VER_2 == TRUE)
        else
            status = STATUS_FAILED;

        if((read_buf[offset] == TAG_JSBL_HDR_ID)&&
        (certf_found != STATUS_FAILED)&&(sign_found != STATUS_OK))
#else
        if(read_buf[offset] == TAG_JSBL_HDR_ID &&
           key_found != STATUS_OK)
#endif
        {
            //TODO check the SElect cmd response and return status accordingly
            ALOGD("TAGID: TAG_JSBL_HDR_ID");
            offset = offset+1;
            len_byte = Numof_lengthbytes(&read_buf[offset], &wLen);
            offset = offset + len_byte;
#if(NXP_LDR_SVC_VER_2 == FALSE)
            if(read_buf[offset] == TAG_JSBL_KEY_ID)
            {
                ALOGE("TAGID: TAG_JSBL_KEY_ID");
                offset = offset+1;
                wLen = read_buf[offset];
                offset = offset+1;
                key_found = memcmp(&read_buf[offset], Select_Rsp,
                Select_Rsp_Len);

                if(key_found == STATUS_OK)
                {
                    ALOGE("Key is matched");
                    offset = offset + wLen;
#endif
                    if(read_buf[offset] == TAG_SIGNATURE_ID)
                    {
#if(NXP_LDR_SVC_VER_2 == TRUE)
                        offset = offset+1;
                        len_byte = Numof_lengthbytes(&read_buf[offset], &wLen);
                        offset = offset + len_byte;
#endif
                        ALOGE("TAGID: TAG_SIGNATURE_ID");

#if(NXP_LDR_SVC_VER_2 == TRUE)
                        pTranscv_Info->sSendlength = wLen+5;

                        pTranscv_Info->sSendData[0] = 0x00;
                        pTranscv_Info->sSendData[1] = 0xA0;
                        pTranscv_Info->sSendData[2] = 0x00;
                        pTranscv_Info->sSendData[3] = 0x00;
                        pTranscv_Info->sSendData[4] = wLen;

                        memcpy(&(pTranscv_Info->sSendData[5]),
                        &read_buf[offset], wLen);
#else
                        offset = offset+1;
                        wLen = 0;
                        len_byte = Numof_lengthbytes(&read_buf[offset], &wLen);
                        if(len_byte == 0)
                        {
                            ALOGE("Invalid length zero");
                            return STATUS_FAILED;
                        }
                        else
                        {
                            offset = offset+len_byte;
                            pTranscv_Info->sSendlength = wLen;
                            memcpy(pTranscv_Info->sSendData, &read_buf[offset],
                            wLen);
                        }
#endif
                        ALOGE("%s: start transceive for length %ld", fn,
                        pTranscv_Info->sSendlength);
#if(NXP_LDR_SVC_VER_2 == TRUE)
                        status = ALA_SendtoAla(Os_info, status, pTranscv_Info, LS_Sign);
#else
                        status = ALA_SendtoAla(Os_info, status, pTranscv_Info);
#endif
                        if(status != STATUS_OK)
                        {
                            return status;
                        }
#if(NXP_LDR_SVC_VER_2 == TRUE)
                        else
                        {
                            sign_found = STATUS_OK;
                        }
#endif
                    }
#if(NXP_LDR_SVC_VER_2 == FALSE)
                }
                else
                {
                    /*
                     * Discard the packet and goto next line
                     * */
                }
            }
            else
            {
                ALOGE("Invalid Tag ID");
                status = STATUS_FAILED;
                break;
            }
#endif
        }
#if(NXP_LDR_SVC_VER_2 == TRUE)
        else if(read_buf[offset] != TAG_JSBL_HDR_ID )
        {
            status = STATUS_FAILED;
        }
#else
        else if(read_buf[offset] == TAG_ALA_CMD_ID &&
                key_found == STATUS_OK)
        {
            /*Key match is success and start sending the packet to Ala
             * return status ok
             * */
            ALOGE("TAGID: TAG_ALA_CMD_ID");
            offset = offset+1;
            wLen = 0;
            len_byte = Numof_lengthbytes(&read_buf[offset], &wLen);
            if(len_byte == 0)
            {
                ALOGE("Invalid length zero");
                return STATUS_FAILED;
            }
            else
            {
                offset = offset+len_byte;
                pTranscv_Info->sSendlength = wLen;
                memcpy(pTranscv_Info->sSendData, &read_buf[offset], wLen);
            }

            status = ALA_SendtoAla(Os_info, status, pTranscv_Info);
            break;
        }
        else if(read_buf[offset] == TAG_JSBL_HDR_ID &&
                key_found == STATUS_OK)
        {
            /*Key match is success
             * Discard the packets untill we found header T=0x40
             * */
        }
        else
        {
            /*Invalid header*/
            status = STATUS_FAILED;
            break;
        }
#endif

#if(NXP_LDR_SVC_VER_2 == FALSE)
        }
#else
    }
    else
    {
        ALOGE("%s : Exit certificate verification failed", fn);
    }
#endif

    ALOGD("%s: exit: status=0x%x", fn, status);
    return status;
}
/*******************************************************************************
**
** Function:        ALA_ReadScript
**
** Description:     Reads the current line if the script
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_ReadScript(Ala_ImageInfo_t *Os_info, UINT8 *read_buf)
{
    static const char fn[]="ALA_ReadScript";
    INT32 wCount, wLen, wIndex = 0;
    UINT8 len_byte = 0;
    int wResult = 0;
    tJBL_STATUS status = STATUS_FAILED;
    INT32 lenOff = 1;

    ALOGD("%s: enter", fn);

    for(wCount =0; (wCount < 2 && !feof(Os_info->fp)); wCount++, wIndex++)
    {
        wResult = FSCANF_BYTE(Os_info->fp,"%2X",(unsigned int*)&read_buf[wIndex]);
    }
    if(wResult == 0)
        return STATUS_FAILED;

    Os_info->bytes_read = Os_info->bytes_read + (wCount*2);

#if(NXP_LDR_SVC_VER_2 == TRUE)
    if((read_buf[0]==0x7f) && (read_buf[1]==0x21))
    {
        for(wCount =0; (wCount < 1 && !feof(Os_info->fp)); wCount++, wIndex++)
        {
            wResult = FSCANF_BYTE(Os_info->fp,"%2X",(unsigned int*)&read_buf[wIndex]);
        }
        if(wResult == 0)
        {
            ALOGE("%s: Exit Read Script failed in 7F21 ", fn);
            return STATUS_FAILED;
        }
        /*Read_Script from wCount*2 to wCount*1 */
        Os_info->bytes_read = Os_info->bytes_read + (wCount*2);
        lenOff = 2;
    }
    else if((read_buf[0] == 0x40)||(read_buf[0] == 0x60))
    {
        lenOff = 1;
    }
    /*If TAG is neither 7F21 nor 60 nor 40 then ABORT execution*/
    else
    {
        ALOGE("Invalid TAG 0x%X found in the script", read_buf[0]);
        return STATUS_FAILED;
    }
#endif

    if(read_buf[lenOff] == 0x00)
    {
        ALOGE("Invalid length zero");
        len_byte = 0x00;
        return STATUS_FAILED;
    }
    else if((read_buf[lenOff] & 0x80) == 0x80)
    {
        len_byte = read_buf[lenOff] & 0x0F;
        len_byte = len_byte +1; //1 byte added for byte 0x81

        ALOGD("%s: Length byte Read from 0x80 is 0x%x ", fn, len_byte);

        if(len_byte == 0x02)
        {
            for(wCount =0; (wCount < 1 && !feof(Os_info->fp)); wCount++, wIndex++)
            {
                wResult = FSCANF_BYTE(Os_info->fp,"%2X",(unsigned int*)&read_buf[wIndex]);
            }
            if(wResult == 0)
            {
                ALOGE("%s: Exit Read Script failed in length 0x02 ", fn);
                return STATUS_FAILED;
            }

            wLen = read_buf[lenOff+1];
            Os_info->bytes_read = Os_info->bytes_read + (wCount*2);
            ALOGD("%s: Length of Read Script in len_byte= 0x02 is 0x%lx ", fn, wLen);
        }
        else if(len_byte == 0x03)
        {
            for(wCount =0; (wCount < 2 && !feof(Os_info->fp)); wCount++, wIndex++)
            {
                wResult = FSCANF_BYTE(Os_info->fp,"%2X",(unsigned int*)&read_buf[wIndex]);
            }
            if(wResult == 0)
            {
                ALOGE("%s: Exit Read Script failed in length 0x03 ", fn);
                return STATUS_FAILED;
            }

            Os_info->bytes_read = Os_info->bytes_read + (wCount*2);
            wLen = read_buf[lenOff+1]; //Length of the packet send to ALA
            wLen = ((wLen << 8) | (read_buf[lenOff+2]));
            ALOGD("%s: Length of Read Script in len_byte= 0x03 is 0x%lx ", fn, wLen);
        }
        else
        {
            /*Need to provide the support if length is more than 2 bytes*/
            ALOGE("Length recived is greater than 3");
            return STATUS_FAILED;
        }
    }
    else
    {
        len_byte = 0x01;
        wLen = read_buf[lenOff];
        ALOGE("%s: Length of Read Script in len_byte= 0x01 is 0x%lx ", fn, wLen);
    }


    for(wCount =0; (wCount < wLen && !feof(Os_info->fp)); wCount++, wIndex++)
    {
        wResult = FSCANF_BYTE(Os_info->fp,"%2X",(unsigned int*)&read_buf[wIndex]);
    }

    if(wResult == 0)
    {
        ALOGE("%s: Exit Read Script failed in fscanf function ", fn);
        return status;
    }
    else
    {
#if(NXP_LDR_SVC_VER_2 == TRUE)
        Os_info->bytes_read = Os_info->bytes_read + (wCount*2)+1; //not sure why 2 added
#else
        Os_info->bytes_read = Os_info->bytes_read + (wCount*2)+2; //not sure why 2 added
#endif
        status = STATUS_OK;
    }

    ALOGD("%s: exit: status=0x%x; Num of bytes read=%d and index=%ld",
    fn, status, Os_info->bytes_read,wIndex);

    return status;
}

/*******************************************************************************
**
** Function:        ALA_SendtoEse
**
** Description:     It is used to send the packet to p61
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_SendtoEse(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
{
    static const char fn [] = "ALA_SendtoEse";
    bool stat =false, chanl_open_cmd = false;
    UINT8 xx=0;
    status = STATUS_FAILED;
    INT32 recvBufferActualSize=0, recv_len = 0;
    IChannel_t *mchannel = gpAla_Dwnld_Context->mchannel;
    ALOGD("%s: enter", fn);
#ifdef JCOP3_WR
    /*
     * Bufferize_load_cmds function is implemented in JCOP
     * */
    status = Bufferize_load_cmds(Os_info, status, pTranscv_Info);
    if(status != STATUS_FAILED)
    {
#endif
        if(pTranscv_Info->sSendData[1] == 0x70)
        {
            if(pTranscv_Info->sSendData[2] == 0x00)
            {
                ALOGE("Channel open");
                chanl_open_cmd = true;
            }
            else
            {
                ALOGE("Channel close");
                for(UINT8 cnt=0; cnt < Os_info->channel_cnt; cnt++)
                {
                    if(Os_info->Channel_Info[cnt].channel_id == pTranscv_Info->sSendData[3])
                    {
                        ALOGE("Closed channel id = 0x0%x", Os_info->Channel_Info[cnt].channel_id);
                        Os_info->Channel_Info[cnt].isOpend = false;
                    }
                }
            }
        }
        pTranscv_Info->timeout = gTransceiveTimeout;
        pTranscv_Info->sRecvlength = 1024;
        stat = mchannel->transceive(pTranscv_Info->sSendData,
                                    pTranscv_Info->sSendlength,
                                    pTranscv_Info->sRecvData,
                                    pTranscv_Info->sRecvlength,
                                    recvBufferActualSize,
                                    pTranscv_Info->timeout);
        if(stat != TRUE)
        {
            ALOGE("%s: Transceive failed; status=0x%X", fn, stat);
        }
        else
        {
            if(chanl_open_cmd == true)
            {
                if((recvBufferActualSize == 0x03) &&
                   ((pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x90) &&
                    (pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x00)))
                {
                    ALOGE("open channel success");
                    UINT8 cnt = Os_info->channel_cnt;
                    Os_info->Channel_Info[cnt].channel_id = pTranscv_Info->sRecvData[recvBufferActualSize-3];
                    Os_info->Channel_Info[cnt].isOpend = true;
                    Os_info->channel_cnt++;
                }
                else
                {
                    ALOGE("channel open faield");
                }
            }
            status = Process_EseResponse(pTranscv_Info, recvBufferActualSize, Os_info);
        }
#ifdef JCOP3_WR
    }
    else if(SendBack_cmds == false)
    {
        /*
         * Workaround for issue in JCOP
         * Send the fake response back
         * */
        recvBufferActualSize = 0x03;
        pTranscv_Info->sRecvData[0] = 0x00;
        pTranscv_Info->sRecvData[1] = 0x90;
        pTranscv_Info->sRecvData[2] = 0x00;
        status = Process_EseResponse(pTranscv_Info, recvBufferActualSize, Os_info);
    }
    else
    {
#if(NXP_LDR_SVC_VER_2 == TRUE)
        if(islastcmdLoad == true)
        {
            status = Send_Backall_Loadcmds(Os_info, status, pTranscv_Info);
            SendBack_cmds = false;
        }else
        {
            memset(Cmd_Buffer, 0, sizeof(Cmd_Buffer));
            SendBack_cmds = false;
            status = STATUS_FAILED;
        }
#else
        status = Send_Backall_Loadcmds(Os_info, status, pTranscv_Info);
        SendBack_cmds = false;
#endif
    }
#endif
    ALOGD("%s: exit: status=0x%x", fn, status);
    return status;
}

/*******************************************************************************
**
** Function:        ALA_SendtoAla
**
** Description:     It is used to forward the packet to Ala
**
** Returns:         Success if ok.
**
*******************************************************************************/
#if(NXP_LDR_SVC_VER_2 == TRUE)
tJBL_STATUS ALA_SendtoAla(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info, Ls_TagType tType)
#else
tJBL_STATUS ALA_SendtoAla(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
#endif
{
    static const char fn [] = "ALA_SendtoAla";
    bool stat =false;
    status = STATUS_FAILED;
    INT32 recvBufferActualSize = 0;
    IChannel_t *mchannel = gpAla_Dwnld_Context->mchannel;
    ALOGD("%s: enter", fn);
#if(NXP_LDR_SVC_VER_2 == TRUE)
    pTranscv_Info->sSendData[0] = (0x80 | Os_info->Channel_Info[0].channel_id);
#else
    pTranscv_Info->sSendData[0] = (pTranscv_Info->sSendData[0] | Os_info->Channel_Info[0].channel_id);
#endif
    pTranscv_Info->timeout = gTransceiveTimeout;
    pTranscv_Info->sRecvlength = 1024;

    stat = mchannel->transceive(pTranscv_Info->sSendData,
                                pTranscv_Info->sSendlength,
                                pTranscv_Info->sRecvData,
                                pTranscv_Info->sRecvlength,
                                recvBufferActualSize,
                                pTranscv_Info->timeout);
    if(stat != TRUE)
    {
        ALOGE("%s: Transceive failed; status=0x%X", fn, stat);
    }
    else
    {
#if(NXP_LDR_SVC_VER_2 == TRUE)
        status = ALA_ProcessResp(Os_info, recvBufferActualSize, pTranscv_Info, tType);
#else
        status = ALA_ProcessResp(Os_info, recvBufferActualSize, pTranscv_Info);
#endif
    }
    ALOGD("%s: exit: status=0x%x", fn, status);
    return status;
}
/*******************************************************************************
**
** Function:        ALA_CloseChannel
**
** Description:     Closes the previously opened logical channel
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_CloseChannel(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
{
    static const char fn [] = "ALA_CloseChannel";
    status = STATUS_FAILED;
    bool stat = false;
    UINT8 xx =0;
    INT32 recvBufferActualSize = 0;
    IChannel_t *mchannel = gpAla_Dwnld_Context->mchannel;
    UINT8 cnt = 0;
    ALOGD("%s: enter",fn);

    if(Os_info == NULL ||
       pTranscv_Info == NULL)
    {
        ALOGE("Invalid parameter");
    }
    else
    {
        for(cnt =0; (cnt < Os_info->channel_cnt); cnt++)
        {
            if(Os_info->Channel_Info[cnt].isOpend == false)
                continue;
            xx = 0;
            pTranscv_Info->sSendData[xx++] = Os_info->Channel_Info[cnt].channel_id;
            pTranscv_Info->sSendData[xx++] = 0x70;
            pTranscv_Info->sSendData[xx++] = 0x80;
            pTranscv_Info->sSendData[xx++] = Os_info->Channel_Info[cnt].channel_id;
            pTranscv_Info->sSendData[xx++] = 0x00;
            pTranscv_Info->sSendlength = xx;
            pTranscv_Info->timeout = gTransceiveTimeout;
            pTranscv_Info->sRecvlength = 1024;
            stat = mchannel->transceive(pTranscv_Info->sSendData,
                                        pTranscv_Info->sSendlength,
                                        pTranscv_Info->sRecvData,
                                        pTranscv_Info->sRecvlength,
                                        recvBufferActualSize,
                                        pTranscv_Info->timeout);
            if(stat != TRUE &&
               recvBufferActualSize < 2)
            {
                ALOGE("%s: Transceive failed; status=0x%X", fn, stat);
            }
            else if((pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x90) &&
                    (pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x00))
            {
                ALOGE("Close channel id = 0x0%x is success", Os_info->Channel_Info[cnt].channel_id);
                status = STATUS_OK;
            }
            else
            {
                ALOGE("Close channel id = 0x0%x is failed", Os_info->Channel_Info[cnt].channel_id);
            }
        }

    }
    ALOGD("%s: exit; status=0x0%x", fn, status);
    return status;
}
/*******************************************************************************
**
** Function:        ALA_ProcessResp
**
** Description:     Process the response packet received from Ala
**
** Returns:         Success if ok.
**
*******************************************************************************/
#if(NXP_LDR_SVC_VER_2 == TRUE)
tJBL_STATUS ALA_ProcessResp(Ala_ImageInfo_t *image_info, INT32 recvlen, Ala_TranscieveInfo_t *trans_info, Ls_TagType tType)
#else
tJBL_STATUS ALA_ProcessResp(Ala_ImageInfo_t *image_info, INT32 recvlen, Ala_TranscieveInfo_t *trans_info)
#endif
{
    static const char fn [] = "ALA_ProcessResp";
    tJBL_STATUS status = STATUS_FAILED;
    static INT32 temp_len = 0;
    UINT8* RecvData = trans_info->sRecvData;
    UINT8 xx =0;
    char sw[2];

    ALOGD("%s: enter", fn);

    if(RecvData == NULL &&
            recvlen == 0x00)
    {
        ALOGE("%s: Invalid parameter: status=0x%x", fn, status);
        return status;
    }
    else if(recvlen >= 2)
    {
        sw[0] = RecvData[recvlen-2];
        sw[1] = RecvData[recvlen-1];
    }
    else
    {
        ALOGE("%s: Invalid response; status=0x%x", fn, status);
        return status;
    }
#if(NXP_LDR_SVC_VER_2 == TRUE)
    /*Update the Global variable for storing response length*/
    resp_len = recvlen;
    if((sw[0] != 0x63))
    {
        lsExecuteResp[2] = sw[0];
        lsExecuteResp[3] = sw[1];
        ALOGD("%s: Process Response SW; status = 0x%x", fn, sw[0]);
        ALOGD("%s: Process Response SW; status = 0x%x", fn, sw[1]);
    }
#endif
    if((recvlen == 0x02) &&
       (sw[0] == 0x90) &&
       (sw[1] == 0x00))
    {
#if(NXP_LDR_SVC_VER_2 == TRUE)
        tJBL_STATUS wStatus = STATUS_FAILED;
        ALOGE("%s: Before Write Response", fn);
        wStatus = Write_Response_To_OutFile(image_info, RecvData, recvlen, tType);
        if(wStatus != STATUS_FAILED)
#endif
            status = STATUS_OK;
    }
    else if((recvlen > 0x02) &&
            (sw[0] == 0x90) &&
            (sw[1] == 0x00))
    {
#if(NXP_LDR_SVC_VER_2 == TRUE)
        tJBL_STATUS wStatus = STATUS_FAILED;
        ALOGE("%s: Before Write Response", fn);
        wStatus = Write_Response_To_OutFile(image_info, RecvData, recvlen, tType);
        if(wStatus != STATUS_FAILED)
            status = STATUS_OK;
#else
        if(temp_len != 0)
        {
            memcpy((trans_info->sTemp_recvbuf+temp_len), RecvData, (recvlen-2));
            trans_info->sSendlength = temp_len + (recvlen-2);
            memcpy(trans_info->sSendData, trans_info->sTemp_recvbuf, trans_info->sSendlength);
            temp_len = 0;
        }
        else
        {
            memcpy(trans_info->sSendData, RecvData, (recvlen-2));
            trans_info->sSendlength = recvlen-2;
        }
        status = ALA_SendtoEse(image_info, status, trans_info);
#endif
    }
#if(NXP_LDR_SVC_VER_2 == FALSE)
    else if ((recvlen > 0x02) &&
             (sw[0] == 0x61))
    {
        memcpy((trans_info->sTemp_recvbuf+temp_len), RecvData, (recvlen-2));
        temp_len = temp_len + recvlen-2;
        trans_info->sSendData[xx++] = image_info->Channel_Info[0].channel_id;
        trans_info->sSendData[xx++] = 0xC0;
        trans_info->sSendData[xx++] = 0x00;
        trans_info->sSendData[xx++] = 0x00;
        trans_info->sSendData[xx++] = sw[1];
        trans_info->sSendlength = xx;
        status = ALA_SendtoAla(image_info, status, trans_info);
    }
#endif
#if(NXP_LDR_SVC_VER_2 == TRUE)
    else if ((recvlen > 0x02) &&
            (sw[0] == 0x63) &&
            (sw[1] == 0x10))
    {
        if(temp_len != 0)
        {
            memcpy((trans_info->sTemp_recvbuf+temp_len), RecvData, (recvlen-2));
            trans_info->sSendlength = temp_len + (recvlen-2);
            memcpy(trans_info->sSendData, trans_info->sTemp_recvbuf,
                    trans_info->sSendlength);
            temp_len = 0;
        }
        else
        {
            memcpy(trans_info->sSendData, RecvData, (recvlen-2));
            trans_info->sSendlength = recvlen-2;
        }
        status = ALA_SendtoEse(image_info, status, trans_info);
    }
    else if ((recvlen > 0x02) &&
            (sw[0] == 0x63) &&
            (sw[1] == 0x20))
    {
        UINT8 respLen = 0;
        INT32 wStatus = 0;

        AID_ARRAY[0] = recvlen+3;
        AID_ARRAY[1] = 00;
        AID_ARRAY[2] = 0xA4;
        AID_ARRAY[3] = 0x04;
        AID_ARRAY[4] = 0x00;
        AID_ARRAY[5] = recvlen-2;
        memcpy(&AID_ARRAY[6], &RecvData[0],recvlen-2);
        memcpy(&ArrayOfAIDs[2][0], &AID_ARRAY[0], recvlen+4);

        fAID_MEM = fopen(AID_MEM_PATH,"w");

        if (fAID_MEM == NULL) {
            ALOGE("Error opening AID data for writing: %s",strerror(errno));
            return status;
        }

        /*Updating the AID_MEM with new value into AID file*/
        while(respLen <= (recvlen+4))
        {
            wStatus = fprintf(fAID_MEM, "%2x", AID_ARRAY[respLen++]);
            if(wStatus != 2)
            {
                ALOGE("%s: Invalid Response during fprintf; status=0x%x",
                        fn, status);
                fclose(fAID_MEM);
                break;
            }
        }
        if(wStatus == 2)
        {
            status = STATUS_FILE_NOT_FOUND;
        }
        else
        {
           status = STATUS_FAILED;
        }
    }
    else if((recvlen >= 0x02) &&(
            (sw[0] != 0x90) &&
            (sw[0] != 0x63)&&(sw[0] != 0x61)))
    {
        tJBL_STATUS wStatus = STATUS_FAILED;
        wStatus = Write_Response_To_OutFile(image_info, RecvData, recvlen, tType);
        //if(wStatus != STATUS_FAILED)
            //status = STATUS_OK;
    }
#endif
    ALOGD("%s: exit: status=0x%x", fn, status);
    return status;
}
/*******************************************************************************
**
** Function:        ALA_SendtoEse
**
** Description:     It is used to process the received response packet from p61
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS Process_EseResponse(Ala_TranscieveInfo_t *pTranscv_Info, INT32 recv_len, Ala_ImageInfo_t *Os_info)
{
    static const char fn[] = "Process_EseResponse";
    tJBL_STATUS status = STATUS_OK;
    UINT8 xx = 0;
    ALOGD("%s: enter", fn);

    pTranscv_Info->sSendData[xx++] = (CLA_BYTE | Os_info->Channel_Info[0].channel_id);
#if(NXP_LDR_SVC_VER_2 == TRUE)
    pTranscv_Info->sSendData[xx++] = 0xA2;
#else
    pTranscv_Info->sSendData[xx++] = 0xA0;
#endif
    if(recv_len <= 0xFF)
    {
#if(NXP_LDR_SVC_VER_2 == TRUE)
        pTranscv_Info->sSendData[xx++] = 0x80;
#else
        pTranscv_Info->sSendData[xx++] = ONLY_BLOCK;
#endif
        pTranscv_Info->sSendData[xx++] = 0x00;
        pTranscv_Info->sSendData[xx++] = (UINT8)recv_len;
        memcpy(&(pTranscv_Info->sSendData[xx]),pTranscv_Info->sRecvData,recv_len);
        pTranscv_Info->sSendlength = xx+ recv_len;
#if(NXP_LDR_SVC_VER_2)
        status = ALA_SendtoAla(Os_info, status, pTranscv_Info, LS_Comm);
#else
        status = ALA_SendtoAla(Os_info, status, pTranscv_Info);
#endif
    }
    else
    {
        while(recv_len > MAX_SIZE)
        {
            xx = PARAM_P1_OFFSET;
#if(NXP_LDR_SVC_VER_2 == TRUE)
            pTranscv_Info->sSendData[xx++] = 0x00;
#else
            pTranscv_Info->sSendData[xx++] = FIRST_BLOCK;
#endif
            pTranscv_Info->sSendData[xx++] = 0x00;
            pTranscv_Info->sSendData[xx++] = MAX_SIZE;
            recv_len = recv_len - MAX_SIZE;
            memcpy(&(pTranscv_Info->sSendData[xx]),pTranscv_Info->sRecvData,MAX_SIZE);
            pTranscv_Info->sSendlength = xx+ MAX_SIZE;
#if(NXP_LDR_SVC_VER_2 == TRUE)
            /*Need not store Process eSE response's response in the out file so
             * LS_Comm = 0*/
            status = ALA_SendtoAla(Os_info, status, pTranscv_Info, LS_Comm);
#else
            status = ALA_SendtoAla(Os_info, status, pTranscv_Info);
#endif
            if(status != STATUS_OK)
            {
                ALOGE("Sending packet to Ala failed: status=0x%x", status);
                return status;
            }
        }
        xx = PARAM_P1_OFFSET;
        pTranscv_Info->sSendData[xx++] = LAST_BLOCK;
        pTranscv_Info->sSendData[xx++] = 0x01;
        pTranscv_Info->sSendData[xx++] = recv_len;
        memcpy(&(pTranscv_Info->sSendData[xx]),pTranscv_Info->sRecvData,recv_len);
        pTranscv_Info->sSendlength = xx+ recv_len;
#if(NXP_LDR_SVC_VER_2 == TRUE)
            status = ALA_SendtoAla(Os_info, status, pTranscv_Info, LS_Comm);
#else
            status = ALA_SendtoAla(Os_info, status, pTranscv_Info);
#endif
    }
    ALOGD("%s: exit: status=0x%x", fn, status);
    return status;
}
/*******************************************************************************
**
** Function:        Process_SelectRsp
**
** Description:     It is used to process the received response for SELECT ALA cmd
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS Process_SelectRsp(UINT8* Recv_data, INT32 Recv_len)
{
    static const char fn[]="Process_SelectRsp";
    tJBL_STATUS status = STATUS_FAILED;
    int i = 0, len=0;
    ALOGE("%s: enter", fn);

    if(Recv_data[i] == TAG_SELECT_ID)
    {
        ALOGD("TAG: TAG_SELECT_ID");
        i = i +1;
        len = Recv_data[i];
        i = i+1;
        if(Recv_data[i] == TAG_ALA_ID)
        {
            ALOGD("TAG: TAG_ALA_ID");
            i = i+1;
            len = Recv_data[i];
            i = i + 1 + len; //points to next tag name A5
#if(NXP_LDR_SVC_VER_2 == TRUE)
            //points to TAG 9F08 for LS application version
            if((Recv_data[i] == TAG_LS_VER1)&&(Recv_data[i+1] == TAG_LS_VER2))
            {
                UINT8 lsaVersionLen = 0;
                ALOGD("TAG: TAG_LS_APPLICATION_VER");

                i = i+2;
                lsaVersionLen = Recv_data[i];
                //points to TAG 9F08 LS application version
                i = i+1;
                memcpy(lsVersionArr, &Recv_data[i],lsaVersionLen);

                //points to Identifier of the Root Entity key set identifier
                i = i+lsaVersionLen;

                if(Recv_data[i] == TAG_RE_KEYID)
                {
                    UINT8 rootEntityLen = 0;
                    i = i+1;
                    rootEntityLen = Recv_data[i];

                    i = i+1;
                    if(Recv_data[i] == TAG_LSRE_ID)
                    {
                        UINT8 tag42Len = 0;
                        i = i+1;
                        tag42Len = Recv_data[i];
                        //copy the data including length
                        memcpy(tag42Arr, &Recv_data[i], tag42Len+1);
                        i = i+tag42Len+1;

                        if(Recv_data[i] == TAG_LSRE_SIGNID)
                        {
                            UINT8 tag45Len = Recv_data[i+1];
                            memcpy(tag45Arr, &Recv_data[i+1],tag45Len+1);
                            status = STATUS_OK;
                        }
                        else
                        {
                            ALOGE("Invalid Root entity for TAG 45 = 0x%x; "
                            "status=0x%x", Recv_data[i], status);
                            return status;
                        }
                    }
                    else
                    {
                        ALOGE("Invalid Root entity for TAG 42 = 0x%x; "
                        "status=0x%x", Recv_data[i], status);
                        return status;
                    }
                }
                else
                {
                    ALOGE("Invalid Root entity key set TAG ID = 0x%x; "
                    "status=0x%x", Recv_data[i], status);
                    return status;
                }
            }
        }
        else
        {
            ALOGE("Invalid Loader Service AID TAG ID = 0x%x; status=0x%x",
            Recv_data[i], status);
            return status;
        }
    }
    else
    {
        ALOGE("Invalid FCI TAG = 0x%x; status=0x%x", Recv_data[i], status);
        return status;
    }
#else
            if(Recv_data[i] == TAG_PRO_DATA_ID)
            {
                ALOGE("TAG: TAG_PRO_DATA_ID");
                i = i+1;
                len = Recv_data[i];
                i = i + 1; //points to next tag name 61
            }
        }
    }
    else
    {
        /*
         * Invalid start of TAG Name found
         * */
        ALOGE("Invalid TAG ID = 0x%x; status=0x%x", Recv_data[i], status);
        return status;
    }

    if((i < Recv_len) &&
       (Recv_data[i] == TAG_JSBL_KEY_ID))
    {
        /*
         * Valid Key is found
         * Copy the data into Select_Rsp
         * */
        ALOGE("Valid key id is found");
        i = i +1;
        len = Recv_data[i];
        if(len != 0x00)
        {
            i = i+1;
            memcpy(Select_Rsp, &Recv_data[i], len);
            Select_Rsp_Len = len;
            status = STATUS_OK;
        }
        /*
         * Identifier of the certificate storing
         * JSBL encryption key
         * */
        i = i + len;
        if(Recv_data[i] == TAG_JSBL_CER_ID)
        {
            i = i+1;
            len = Recv_data[i];
            if(len != 0x00)
            {
                i = i+1;
                Jsbl_keylen = len;
                memcpy(Jsbl_RefKey, &Recv_data[i], len);
            }
        }
    }
#endif
    ALOGE("%s: Exiting status = 0x%x", fn, status);
    return status;
}


#ifdef JCOP3_WR
tJBL_STATUS Bufferize_load_cmds(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
{
    static const char fn[] = "Bufferize_load_cmds";
    UINT8 Param_P2;
    status = STATUS_FAILED;

    if(cmd_count == 0x00)
    {
        if((pTranscv_Info->sSendData[1] == INSTAL_LOAD_ID) &&
           (pTranscv_Info->sSendData[2] == PARAM_P1_OFFSET) &&
           (pTranscv_Info->sSendData[3] == 0x00))
        {
            ALOGE("BUffer: install for load");
            pBuffer[0] = pTranscv_Info->sSendlength;
            memcpy(&pBuffer[1], &(pTranscv_Info->sSendData[0]), pTranscv_Info->sSendlength);
            pBuffer = pBuffer + pTranscv_Info->sSendlength + 1;
            cmd_count++;
        }
        else
        {
            /*
             * Do not buffer this cmd
             * Send this command to eSE
             * */
            status = STATUS_OK;
        }

    }
    else
    {
        Param_P2 = cmd_count -1;
        if((pTranscv_Info->sSendData[1] == LOAD_CMD_ID) &&
           (pTranscv_Info->sSendData[2] == LOAD_MORE_BLOCKS) &&
           (pTranscv_Info->sSendData[3] == Param_P2))
        {
            ALOGE("BUffer: load");
            pBuffer[0] = pTranscv_Info->sSendlength;
            memcpy(&pBuffer[1], &(pTranscv_Info->sSendData[0]), pTranscv_Info->sSendlength);
            pBuffer = pBuffer + pTranscv_Info->sSendlength + 1;
            cmd_count++;
        }
        else if((pTranscv_Info->sSendData[1] == LOAD_CMD_ID) &&
                (pTranscv_Info->sSendData[2] == LOAD_LAST_BLOCK) &&
                (pTranscv_Info->sSendData[3] == Param_P2))
        {
            ALOGE("BUffer: last load");
            SendBack_cmds = true;
            pBuffer[0] = pTranscv_Info->sSendlength;
            memcpy(&pBuffer[1], &(pTranscv_Info->sSendData[0]), pTranscv_Info->sSendlength);
            pBuffer = pBuffer + pTranscv_Info->sSendlength + 1;
            cmd_count++;
            islastcmdLoad = true;
        }
        else
        {
            ALOGE("BUffer: Not a load cmd");
            SendBack_cmds = true;
            pBuffer[0] = pTranscv_Info->sSendlength;
            memcpy(&pBuffer[1], &(pTranscv_Info->sSendData[0]), pTranscv_Info->sSendlength);
            pBuffer = pBuffer + pTranscv_Info->sSendlength + 1;
            islastcmdLoad = false;
            cmd_count++;
        }
    }
    ALOGE("%s: exit; status=0x%x", fn, status);
    return status;
}

tJBL_STATUS Send_Backall_Loadcmds(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
{
    static const char fn [] = "Send_Backall_Loadcmds";
    bool stat =false;
    UINT8 xx=0;
    status = STATUS_FAILED;
    INT32 recvBufferActualSize=0, recv_len = 0;
    IChannel_t *mchannel = gpAla_Dwnld_Context->mchannel;
    ALOGD("%s: enter", fn);
    pBuffer = Cmd_Buffer; // Points to start of first cmd to send
    if(cmd_count == 0x00)
    {
        ALOGE("No cmds to stored to send to eSE");
    }
    else
    {
        while(cmd_count > 0)
        {
            pTranscv_Info->sSendlength = pBuffer[0];
            memcpy(pTranscv_Info->sSendData, &pBuffer[1], pTranscv_Info->sSendlength);
            pBuffer = pBuffer + 1 + pTranscv_Info->sSendlength;

            stat = mchannel->transceive(pTranscv_Info->sSendData,
                                        pTranscv_Info->sSendlength,
                                        pTranscv_Info->sRecvData,
                                        pTranscv_Info->sRecvlength,
                                        recvBufferActualSize,
                                        pTranscv_Info->timeout);
#if(NXP_LDR_SVC_VER_2 == TRUE)
            cmd_count--;
#endif
            if(stat != TRUE ||
               (recvBufferActualSize < 2))
            {
                ALOGE("%s: Transceive failed; status=0x%X", fn, stat);
            }
#if(NXP_LDR_SVC_VER_2 == TRUE)
            else if(cmd_count == 0x00) //Last command in the buffer
#else
            else if(cmd_count == 0x01) //Last command in the buffer
#endif
            {

                if (islastcmdLoad == false)
                {
                    status = Process_EseResponse(pTranscv_Info, recvBufferActualSize, Os_info);
                }
                else if((recvBufferActualSize == 0x02) &&
                        (pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x90) &&
                        (pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x00))
                {
                    recvBufferActualSize = 0x03;
                    pTranscv_Info->sRecvData[0] = 0x00;
                    pTranscv_Info->sRecvData[1] = 0x90;
                    pTranscv_Info->sRecvData[2] = 0x00;
                    status = Process_EseResponse(pTranscv_Info, recvBufferActualSize, Os_info);
                }
                else
                {
                    status = Process_EseResponse(pTranscv_Info, recvBufferActualSize, Os_info);
                }
            }
            else if((recvBufferActualSize == 0x02) &&
                    (pTranscv_Info->sRecvData[0] == 0x90) &&
                    (pTranscv_Info->sRecvData[1] == 0x00))
            {
                /*Do not do anything
                 * send next command in the buffer*/
            }
            else if((recvBufferActualSize == 0x03) &&
                    (pTranscv_Info->sRecvData[0] == 0x00) &&
                    (pTranscv_Info->sRecvData[1] == 0x90) &&
                    (pTranscv_Info->sRecvData[2] == 0x00))
            {
                /*Do not do anything
                 * Send next cmd in the buffer*/
            }
            else if((pTranscv_Info->sRecvData[recvBufferActualSize-2] != 0x90) &&
                    (pTranscv_Info->sRecvData[recvBufferActualSize-1] != 0x00))
            {
                /*Error condition hence exiting the loop*/
                status = Process_EseResponse(pTranscv_Info, recvBufferActualSize, Os_info);
#if(NXP_LDR_SVC_VER_2 == TRUE)
                /*If the sending of Load fails reset the count*/
                cmd_count=0;
#endif

                break;
            }
#if(NXP_LDR_SVC_VER_2 == FALSE)
            cmd_count--;
#endif
        }
    }
    memset(Cmd_Buffer, 0, sizeof(Cmd_Buffer));
    pBuffer = Cmd_Buffer; //point back to start of line
    cmd_count = 0x00;
    ALOGD("%s: exit: status=0x%x", fn, status);
    return status;
}
#endif
/*******************************************************************************
**
** Function:        Numof_lengthbytes
**
** Description:     Checks the number of length bytes and assigns
**                  length value to wLen.
**
** Returns:         Number of Length bytes
**
*******************************************************************************/
UINT8 Numof_lengthbytes(UINT8 *read_buf, INT32 *pLen)
{
    static const char fn[]= "Numof_lengthbytes";
    UINT8 len_byte=0, i=0;
    INT32 wLen = 0;
    ALOGE("%s:enter", fn);

    if(read_buf[i] == 0x00)
    {
        ALOGE("Invalid length zero");
        len_byte = 0x00;
    }
    else if((read_buf[i] & 0x80) == 0x80)
    {
        len_byte = read_buf[i] & 0x0F;
        len_byte = len_byte +1; //1 byte added for byte 0x81
    }
    else
    {
        len_byte = 0x01;
    }
    /*
     * To get the length of the value field
     * */
    switch(len_byte)
    {
    case 0:
        wLen = read_buf[0];
        break;
    case 1:
        /*1st byte is the length*/
        wLen = read_buf[0];
        break;
    case 2:
        /*2nd byte is the length*/
        wLen = read_buf[1];
        break;
    case 3:
        /*1st and 2nd bytes are length*/
        wLen = read_buf[1];
        wLen = ((wLen << 8) | (read_buf[2]));
        break;
    case 4:
        /*3bytes are the length*/
        wLen = read_buf[1];
        wLen = ((wLen << 16) | (read_buf[2] << 8));
        wLen = (wLen | (read_buf[3]));
        break;
    default:
        ALOGE("default case");
        break;
    }

    *pLen = wLen;
    ALOGE("%s:exit; len_bytes=0x0%x, Length=%ld", fn, len_byte, *pLen);
    return len_byte;
}
#if(NXP_LDR_SVC_VER_2 == TRUE)
/*******************************************************************************
**
** Function:        Write_Response_To_OutFile
**
** Description:     Write the response to Out file
**                  with length recvlen from buffer RecvData.
**
** Returns:         Success if OK
**
*******************************************************************************/
tJBL_STATUS Write_Response_To_OutFile(Ala_ImageInfo_t *image_info, UINT8* RecvData,
    INT32 recvlen, Ls_TagType tType)
{
    INT32 respLen       = 0;
    tJBL_STATUS wStatus = STATUS_FAILED;
    static const char fn [] = "Write_Response_to_OutFile";
    INT32 status = 0;
    UINT8 tagBuffer[12] = {0x61,0,0,0,0,0,0,0,0,0,0,0};
    INT32 tag44Len = 0;
    INT32 tag61Len = 0;
    UINT8 tag43Len = 1;
    UINT8 tag43off = 0;
    UINT8 tag44off = 0;
    UINT8 ucTag44[3] = {0x00,0x00,0x00};
    UINT8 tagLen = 0;
    UINT8 tempLen = 0;
    /*If the Response out file is NULL or Other than LS commands*/
    if((image_info->bytes_wrote == 0x55)||(tType == LS_Default))
    {
        return STATUS_OK;
    }
    /*Certificate TAG occupies 2 bytes*/
    if(tType == LS_Cert)
    {
        tag43Len = 2;
    }
    ALOGE("%s: Enter", fn);

    if(recvlen < 0x80)
    {
        tag44Len = 1;
        ucTag44[0] = recvlen;
        tag61Len = recvlen + 4 + tag43Len;
        tagBuffer[1] = tag61Len;
        tag43off = 2;
        tag44off = 4+tag43Len;
        tagLen = tag44off+2;
    }
    else if((recvlen >= 0x80)&&(recvlen <= 0xFF))
    {
        ucTag44[0] = 0x81;
        ucTag44[1] = recvlen;
        tag61Len = recvlen + 5 + tag43Len;
        tag44Len = 2;

        if((tag61Len&0xFF00) != 0)
        {
            tagBuffer[1] = 0x82;
            tagBuffer[2] = (tag61Len & 0xFF00)>>8;
            tagBuffer[3] = (tag61Len & 0xFF);
            tag43off = 4;
            tag44off = 6+tag43Len;
            tagLen = tag44off+3;
        }
        else
        {
            tagBuffer[1] = 0x81;
            tagBuffer[2] = (tag61Len & 0xFF);
            tag43off = 3;
            tag44off = 5+tag43Len;
            tagLen = tag44off+3;
        }
    }
    else if((recvlen > 0xFF) &&(recvlen <= 0xFFFF))
    {
        ucTag44[0] = 0x82;
        ucTag44[1] = (recvlen&0xFF00)>>8;
        ucTag44[2] = (recvlen&0xFF);
        tag44Len = 3;

        tag61Len = recvlen + 6 + tag43Len;

        if((tag61Len&0xFF00) != 0)
        {
            tagBuffer[1] = 0x82;
            tagBuffer[2] = (tag61Len & 0xFF00)>>8;
            tagBuffer[3] = (tag61Len & 0xFF);
            tag43off = 4;
            tag44off = 6+tag43Len;
            tagLen = tag44off+4;
        }
    }
    tagBuffer[tag43off] = 0x43;
    tagBuffer[tag43off+1] = tag43Len;
    tagBuffer[tag44off] = 0x44;
    memcpy(&tagBuffer[tag44off+1], &ucTag44[0],tag44Len);


    if(tType == LS_Cert)
    {
        tagBuffer[tag43off+2] = 0x7F;
        tagBuffer[tag43off+3] = 0x21;
    }
    else if(tType == LS_Sign)
    {
        tagBuffer[tag43off+2] = 0x60;
    }
    else if(tType == LS_Comm)
    {
        tagBuffer[tag43off+2] = 0x40;
    }
    else
    {
       /*Do nothing*/
    }
    while(tempLen < tagLen)
    {
        status = fprintf(image_info->fResp, "%02X", tagBuffer[tempLen++]);
        if(status != 2)
        {
            ALOGE("%s: Invalid Response during fprintf; status=0x%lx", fn, (status));
            wStatus = STATUS_FAILED;
            break;
        }
    }
    /*Updating the response data into out script*/
    while(respLen < recvlen)
    {
        status = fprintf(image_info->fResp, "%02X", RecvData[respLen++]);
        if(status != 2)
        {
            ALOGE("%s: Invalid Response during fprintf; status=0x%lx", fn, (status));
            wStatus = STATUS_FAILED;
            break;
        }
    }
    if((status == 2))
    {
        fprintf(image_info->fResp, "%s\n", "");
        ALOGE("%s: SUCCESS Response written to script out file; status=0x%lx", fn, (status));
        wStatus = STATUS_OK;
    }
    return wStatus;
}

/*******************************************************************************
**
** Function:        Check_Certificate_Tag
**
** Description:     Check certificate Tag presence in script
**                  by 7F21 .
**
** Returns:         Success if Tag found
**
*******************************************************************************/
tJBL_STATUS Check_Certificate_Tag(UINT8 *read_buf, UINT16 *offset1)
{
    tJBL_STATUS status = STATUS_FAILED;
    UINT16 len_byte = 0;
    INT32 wLen, recvBufferActualSize=0;
    UINT16 offset = *offset1;

    if(((read_buf[offset]<<8|read_buf[offset+1]) == TAG_CERTIFICATE))
    {
        ALOGD("TAGID: TAG_CERTIFICATE");
        offset = offset+2;
        len_byte = Numof_lengthbytes(&read_buf[offset], &wLen);
        offset = offset + len_byte;
        *offset1 = offset;
        if(wLen <= MAX_CERT_LEN)
        status = STATUS_OK;
    }
    return status;
}

/*******************************************************************************
**
** Function:        Check_SerialNo_Tag
**
** Description:     Check Serial number Tag presence in script
**                  by 0x93 .
**
** Returns:         Success if Tag found
**
*******************************************************************************/
tJBL_STATUS Check_SerialNo_Tag(UINT8 *read_buf, UINT16 *offset1)
{
    tJBL_STATUS status = STATUS_FAILED;
    UINT16 offset = *offset1;
    static const char fn[] = "Check_SerialNo_Tag";

    if((read_buf[offset] == TAG_SERIAL_NO))
    {
        ALOGD("TAGID: TAG_SERIAL_NO");
        UINT8 serNoLen = read_buf[offset+1];
        offset = offset + serNoLen + 2;
        *offset1 = offset;
        ALOGD("%s: TAG_LSROOT_ENTITY is %x", fn, read_buf[offset]);
        status = STATUS_OK;
    }
    return status;
}

/*******************************************************************************
**
** Function:        Check_LSRootID_Tag
**
** Description:     Check LS root ID tag presence in script and compare with
**                  select response root ID value.
**
** Returns:         Success if Tag found
**
*******************************************************************************/
tJBL_STATUS Check_LSRootID_Tag(UINT8 *read_buf, UINT16 *offset1)
{
    tJBL_STATUS status = STATUS_FAILED;
    UINT16 offset      = *offset1;

    if(read_buf[offset] == TAG_LSRE_ID)
    {
        ALOGD("TAGID: TAG_LSROOT_ENTITY");
        if(tag42Arr[0] == read_buf[offset+1])
        {
            UINT8 tag42Len = read_buf[offset+1];
            offset = offset+2;
            status = memcmp(&read_buf[offset],&tag42Arr[1],tag42Arr[0]);
            ALOGD("ALA_Check_KeyIdentifier : TAG 42 verified");

            if(status == STATUS_OK)
            {
                ALOGD("ALA_Check_KeyIdentifier : Loader service root entity "
                "ID is matched");
                offset = offset+tag42Len;
                *offset1 = offset;
                }
        }
    }
    return status;
}

/*******************************************************************************
**
** Function:        Check_CertHoldID_Tag
**
** Description:     Check certificate holder ID tag presence in script.
**
** Returns:         Success if Tag found
**
*******************************************************************************/
tJBL_STATUS Check_CertHoldID_Tag(UINT8 *read_buf, UINT16 *offset1)
{
    tJBL_STATUS status = STATUS_FAILED;
    UINT16 offset      = *offset1;

    if((read_buf[offset]<<8|read_buf[offset+1]) == TAG_CERTFHOLD_ID)
    {
        UINT8 certfHoldIDLen = 0;
        ALOGD("TAGID: TAG_CERTFHOLD_ID");
        certfHoldIDLen = read_buf[offset+2];
        offset = offset+certfHoldIDLen+3;
        if(read_buf[offset] == TAG_KEY_USAGE)
        {
            UINT8 keyusgLen = 0;
            ALOGD("TAGID: TAG_KEY_USAGE");
            keyusgLen = read_buf[offset+1];
            offset = offset+keyusgLen+2;
            *offset1 = offset;
            status = STATUS_OK;
        }
    }
    return status;
}

/*******************************************************************************
**
** Function:        Check_Date_Tag
**
** Description:     Check date tags presence in script.
**
** Returns:         Success if Tag found
**
*******************************************************************************/
tJBL_STATUS Check_Date_Tag(UINT8 *read_buf, UINT16 *offset1)
{
    tJBL_STATUS status = STATUS_OK;
    UINT16 offset      = *offset1;

    if((read_buf[offset]<<8|read_buf[offset+1]) == TAG_EFF_DATE)
    {
        UINT8 effDateLen = read_buf[offset+2];
        offset = offset+3+effDateLen;
        ALOGD("TAGID: TAG_EFF_DATE");
        if((read_buf[offset]<<8|read_buf[offset+1]) == TAG_EXP_DATE)
        {
            UINT8 effExpLen = read_buf[offset+2];
            offset = offset+3+effExpLen;
            ALOGD("TAGID: TAG_EXP_DATE");
            status = STATUS_OK;
        }else if(read_buf[offset] == TAG_LSRE_SIGNID)
        {
            status = STATUS_OK;
        }
    }
    else if((read_buf[offset]<<8|read_buf[offset+1]) == TAG_EXP_DATE)
    {
        UINT8 effExpLen = read_buf[offset+2];
        offset = offset+3+effExpLen;
        ALOGD("TAGID: TAG_EXP_DATE");
        status = STATUS_OK;
    }else if(read_buf[offset] == TAG_LSRE_SIGNID)
    {
        status = STATUS_OK;
    }
    else
    {
    /*STATUS_FAILED*/
    }
    *offset1 = offset;
    return status;
}


/*******************************************************************************
**
** Function:        Check_45_Tag
**
** Description:     Check 45 tags presence in script and compare the value
**                  with select response tag 45 value
**
** Returns:         Success if Tag found
**
*******************************************************************************/
tJBL_STATUS Check_45_Tag(UINT8 *read_buf, UINT16 *offset1, UINT8 *tag45Len)
{
    tJBL_STATUS status = STATUS_FAILED;
    UINT16 offset      = *offset1;
    if(read_buf[offset] == TAG_LSRE_SIGNID)
    {
        *tag45Len = read_buf[offset+1];
        offset = offset+2;
        if(tag45Arr[0] == *tag45Len)
        {
            status = memcmp(&read_buf[offset],&tag45Arr[1],tag45Arr[0]);
            if(status == STATUS_OK)
            {
                ALOGD("ALA_Check_KeyIdentifier : TAG 45 verified");
                *offset1 = offset;
            }
        }
    }
    return status;
}

/*******************************************************************************
**
** Function:        Certificate_Verification
**
** Description:     Perform the certificate verification by forwarding it to
**                  LS applet.
**
** Returns:         Success if certificate is verified
**
*******************************************************************************/
tJBL_STATUS Certificate_Verification(Ala_ImageInfo_t *Os_info,
Ala_TranscieveInfo_t *pTranscv_Info, UINT8 *read_buf, UINT16 *offset1,
UINT8 *tag45Len)
{
    tJBL_STATUS status = STATUS_FAILED;
    UINT16 offset      = *offset1;
    INT32 wCertfLen = (read_buf[2]<<8|read_buf[3]);
    tJBL_STATUS certf_found = STATUS_FAILED;
    static const char fn[] = "Certificate_Verification";
    UINT8 tag_len_byte = Numof_lengthbytes(&read_buf[2], &wCertfLen);

    pTranscv_Info->sSendData[0] = 0x80;
    pTranscv_Info->sSendData[1] = 0xA0;
    pTranscv_Info->sSendData[2] = 0x01;
    pTranscv_Info->sSendData[3] = 0x00;
    /*If the certificate is less than 255 bytes*/
    if(wCertfLen <= 251)
    {
        UINT8 tag7f49Off = 0;
        UINT8 u7f49Len = 0;
        UINT8 tag5f37Len = 0;
        ALOGD("Certificate is greater than 255");
        offset = offset+*tag45Len;
        ALOGD("%s: Before TAG_CCM_PERMISSION = %x",fn, read_buf[offset]);
        if(read_buf[offset] == TAG_CCM_PERMISSION)
        {
            INT32 tag53Len = 0;
            UINT8 len_byte = 0;
            offset =offset+1;
            len_byte = Numof_lengthbytes(&read_buf[offset], &tag53Len);
            offset = offset+tag53Len+len_byte;
            ALOGD("%s: Verified TAG TAG_CCM_PERMISSION = 0x53",fn);
            if((UINT16)(read_buf[offset]<<8|read_buf[offset+1]) == TAG_SIG_RNS_COMP)
            {
                tag7f49Off = offset;
                u7f49Len   = read_buf[offset+2];
                offset     = offset+3+u7f49Len;
                if(u7f49Len != 64)
                {
                    return STATUS_FAILED;
                }
                if((UINT16)(read_buf[offset]<<8|read_buf[offset+1]) == 0x7f49)
                {
                    tag5f37Len = read_buf[offset+2];
                    if(read_buf[offset+3] != 0x86 || (read_buf[offset+4] != 65))
                    {
                        return STATUS_FAILED;
                    }
                }
                else
                {
                    return STATUS_FAILED;
                }
             }
             else
             {
                 return STATUS_FAILED;
             }
        }
        else
        {
            return STATUS_FAILED;
        }
        pTranscv_Info->sSendData[4] = wCertfLen+2+tag_len_byte;
        pTranscv_Info->sSendlength  = wCertfLen+7+tag_len_byte;
        memcpy(&(pTranscv_Info->sSendData[5]), &read_buf[0], wCertfLen+2+tag_len_byte);

        ALOGD("%s: start transceive for length %ld", fn, pTranscv_Info->
            sSendlength);
        status = ALA_SendtoAla(Os_info, status, pTranscv_Info,LS_Cert);
        if(status != STATUS_OK)
        {
            return status;
        }
        else
        {
            certf_found = STATUS_OK;
            ALOGD("Certificate is verified");
            return status;
        }
    }
    /*If the certificate is more than 255 bytes*/
    else
    {
        UINT8 tag7f49Off = 0;
        UINT8 u7f49Len = 0;
        UINT8 tag5f37Len = 0;
        ALOGD("Certificate is greater than 255");
        offset = offset+*tag45Len;
        ALOGD("%s: Before TAG_CCM_PERMISSION = %x",fn, read_buf[offset]);
        if(read_buf[offset] == TAG_CCM_PERMISSION)
        {
            INT32 tag53Len = 0;
            UINT8 len_byte = 0;
            offset =offset+1;
            len_byte = Numof_lengthbytes(&read_buf[offset], &tag53Len);
            offset = offset+tag53Len+len_byte;
            ALOGD("%s: Verified TAG TAG_CCM_PERMISSION = 0x53",fn);
            if((UINT16)(read_buf[offset]<<8|read_buf[offset+1]) == TAG_SIG_RNS_COMP)
            {
                tag7f49Off = offset;
                u7f49Len   = read_buf[offset+2];
                offset     = offset+3+u7f49Len;
                if(u7f49Len != 64)
                {
                    return STATUS_FAILED;
                }
                if((UINT16)(read_buf[offset]<<8|read_buf[offset+1]) == 0x7f49)
                {
                    tag5f37Len = read_buf[offset+2];
                    if(read_buf[offset+3] != 0x86 || (read_buf[offset+4] != 65))
                    {
                        return STATUS_FAILED;
                    }
                }
                else
                {
                    return STATUS_FAILED;
                }
                pTranscv_Info->sSendData[4] = tag7f49Off;
                memcpy(&(pTranscv_Info->sSendData[5]), &read_buf[0], tag7f49Off);
                pTranscv_Info->sSendlength = tag7f49Off+5;
                ALOGD("%s: start transceive for length %ld", fn,
                pTranscv_Info->sSendlength);

                status = ALA_SendtoAla(Os_info, status, pTranscv_Info,LS_Default);
                if(status != STATUS_OK)
                {

                    UINT8* RecvData = pTranscv_Info->sRecvData;
                    Write_Response_To_OutFile(Os_info, RecvData,
                    resp_len, LS_Cert);
                    return status;
                }

                pTranscv_Info->sSendData[2] = 0x00;
                pTranscv_Info->sSendData[4] = u7f49Len+tag5f37Len+6;
                memcpy(&(pTranscv_Info->sSendData[5]), &read_buf[tag7f49Off],
                    u7f49Len+tag5f37Len+6);
                pTranscv_Info->sSendlength = u7f49Len+tag5f37Len+11;
                ALOGD("%s: start transceive for length %ld", fn,
                    pTranscv_Info->sSendlength);

                status = ALA_SendtoAla(Os_info, status, pTranscv_Info,LS_Cert);
                if(status != STATUS_OK)
                {
                    return status;
                }
                else
                {
                    ALOGD("Certificate is verified");
                    certf_found = STATUS_OK;
                    return status;

                }
            }
            else
            {
                return STATUS_FAILED;
            }
        }
        else
        {
            return STATUS_FAILED;
        }
    }
return status;
}

/*******************************************************************************
**
** Function:        Check_Complete_7F21_Tag
**
** Description:     Traverses the 7F21 tag for verification of each sub tag with
**                  in the 7F21 tag.
**
** Returns:         Success if all tags are verified
**
*******************************************************************************/
tJBL_STATUS Check_Complete_7F21_Tag(Ala_ImageInfo_t *Os_info,
       Ala_TranscieveInfo_t *pTranscv_Info, UINT8 *read_buf, UINT16 *offset)
{
    static const char fn[] = "Check_Complete_7F21_Tag";
    UINT8 tag45Len = 0;

    if(STATUS_OK == Check_Certificate_Tag(read_buf, offset))
    {
        if(STATUS_OK == Check_SerialNo_Tag(read_buf, offset))
        {
           if(STATUS_OK == Check_LSRootID_Tag(read_buf, offset))
           {
               if(STATUS_OK == Check_CertHoldID_Tag(read_buf, offset))
               {
                   if(STATUS_OK == Check_Date_Tag(read_buf, offset))
                   {
                       UINT8 tag45Len = 0;
                       if(STATUS_OK == Check_45_Tag(read_buf, offset,
                       &tag45Len))
                       {
                           if(STATUS_OK == Certificate_Verification(
                           Os_info, pTranscv_Info, read_buf, offset,
                           &tag45Len))
                           {
                               return STATUS_OK;
                           }
                       }else{
                       ALOGE("%s: FAILED in Check_45_Tag", fn);}
                   }else{
                   ALOGE("%s: FAILED in Check_Date_Tag", fn);}
               }else{
               ALOGE("%s: FAILED in Check_CertHoldID_Tag", fn);}
           }else{
           ALOGE("%s: FAILED in Check_LSRootID_Tag", fn);}
        }else{
        ALOGE("%s: FAILED in Check_SerialNo_Tag", fn);}
    }
    else
    {
        ALOGE("%s: FAILED in Check_Certificate_Tag", fn);
    }
return STATUS_FAILED;
}

BOOLEAN ALA_UpdateExeStatus(UINT16 status)
{
    fLS_STATUS = fopen(LS_STATUS_PATH, "w+");
    ALOGD("enter: ALA_UpdateExeStatus");
    if(fLS_STATUS == NULL)
    {
        ALOGE("Error opening LS Status file for backup: %s",strerror(errno));
        return FALSE;
    }
    if((fprintf(fLS_STATUS, "%04x",status)) != 4)
    {
        ALOGE("Error updating LS Status backup: %s",strerror(errno));
        fclose(fLS_STATUS);
        return FALSE;
    }
    ALOGD("exit: ALA_UpdateExeStatus");
    fclose(fLS_STATUS);
    return TRUE;
}

/*******************************************************************************
**
** Function:        ALA_getAppletLsStatus
**
** Description:     Interface to fetch Loader service Applet status to JNI, Services
**
** Returns:         SUCCESS/FAILURE
**
*******************************************************************************/
tJBL_STATUS ALA_getAppletLsStatus(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
{
    static const char fn[] = "ALA_getAppletLsStatus";
    bool stat = false;
    INT32 recvBufferActualSize = 0;
    IChannel_t *mchannel = gpAla_Dwnld_Context->mchannel;

    ALOGD("%s: enter", fn);

    if(Os_info == NULL ||
       pTranscv_Info == NULL)
    {
        ALOGD("%s: Invalid parameter", fn);
    }
    else
    {
        pTranscv_Info->sSendData[0] = STORE_DATA_CLA | Os_info->Channel_Info[0].channel_id;
        pTranscv_Info->timeout = gTransceiveTimeout;
        pTranscv_Info->sSendlength = (INT32)sizeof(GetData);
        pTranscv_Info->sRecvlength = 1024;//(INT32)sizeof(INT32);


        memcpy(&(pTranscv_Info->sSendData[1]), &GetData[1],
                ((sizeof(GetData))-1));
        ALOGD("%s: Calling Secure Element Transceive with GET DATA apdu", fn);

        stat = mchannel->transceive (pTranscv_Info->sSendData,
                                pTranscv_Info->sSendlength,
                                pTranscv_Info->sRecvData,
                                pTranscv_Info->sRecvlength,
                                recvBufferActualSize,
                                pTranscv_Info->timeout);
        if((stat != TRUE) &&
           (recvBufferActualSize == 0x00))
        {
            status = STATUS_FAILED;
            ALOGE("%s: SE transceive failed status = 0x%X", fn, status);
        }
        else if((pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x90) &&
                (pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x00))
        {
            ALOGE("STORE CMD is successful");
            if((pTranscv_Info->sRecvData[0] == 0x46 )&& (pTranscv_Info->sRecvData[1] == 0x01 ))
            {
               if((pTranscv_Info->sRecvData[2] == 0x01))
               {
                   lsGetStatusArr[0]=0x63;lsGetStatusArr[1]=0x40;
                   ALOGE("%s: Script execution status FAILED", fn);
               }
               else if((pTranscv_Info->sRecvData[2] == 0x00))
               {
                   lsGetStatusArr[0]=0x90;lsGetStatusArr[1]=0x00;
                   ALOGE("%s: Script execution status SUCCESS", fn);
               }
               else
               {
                   lsGetStatusArr[0]=0x90;lsGetStatusArr[1]=0x00;
                   ALOGE("%s: Script execution status UNKNOWN", fn);
               }
            }
            else
            {
                lsGetStatusArr[0]=0x90;lsGetStatusArr[1]=0x00;
                ALOGE("%s: Script execution status UNKNOWN", fn);
            }
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_FAILED;
        }

    ALOGE("%s: exit; status=0x%x", fn, status);
    }
    return status;
}

/*******************************************************************************
**
** Function:        Get_LsStatus
**
** Description:     Interface to fetch Loader service client status to JNI, Services
**
** Returns:         SUCCESS/FAILURE
**
*******************************************************************************/
tJBL_STATUS Get_LsStatus(UINT8 *pStatus)
{
    tJBL_STATUS status = STATUS_FAILED;
    UINT8 lsStatus[2]    = {0x63,0x40};
    UINT8 loopcnt = 0;
    fLS_STATUS = fopen(LS_STATUS_PATH, "r");
    if(fLS_STATUS == NULL)
    {
        ALOGE("Error opening LS Status file for backup: %s",strerror(errno));
        return status;
    }
    for(loopcnt=0;loopcnt<2;loopcnt++)
    {
        if((FSCANF_BYTE(fLS_STATUS, "%2x", &lsStatus[loopcnt])) == 0)
        {
            ALOGE("Error updating LS Status backup: %s",strerror(errno));
            fclose(fLS_STATUS);
            return status;
        }
    }
    ALOGD("enter: ALA_getLsStatus 0x%X 0x%X",lsStatus[0],lsStatus[1] );
    memcpy(pStatus, lsStatus, 2);
    fclose(fLS_STATUS);
    return STATUS_OK;
}


#endif
