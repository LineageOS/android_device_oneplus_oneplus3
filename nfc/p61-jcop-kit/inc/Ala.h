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
#ifndef ALA_H_
#define ALA_H_

#define NXP_LS_AID
#include "data_types.h"
#include "IChannel.h"
#include <stdio.h>

typedef struct Ala_ChannelInfo
{
    UINT8 channel_id;
    bool  isOpend;
}Ala_ChannelInfo_t;

typedef struct Ala_TranscieveInfo
{
    INT32 timeout;
    UINT8 sRecvData[1024];
    UINT8 sSendData[1024];
    INT32 sSendlength;
    int   sRecvlength;
    UINT8 sTemp_recvbuf[1024];
}Ala_TranscieveInfo_t;

#if(NXP_LDR_SVC_VER_2 == TRUE)
typedef struct Ala_ImageInfo
{
    FILE                 *fp;
    int                  fls_size;
    char                 fls_path[384];
    int                  bytes_read;
    FILE                 *fResp;
    int                  fls_RespSize;
    char                 fls_RespPath[384];
    int                  bytes_wrote;
    Ala_ChannelInfo_t    Channel_Info[10];
    UINT8                channel_cnt;
}Ala_ImageInfo_t;
typedef enum
{
    LS_Default = 0x00,
    LS_Cert = 0x7F21,
    LS_Sign = 0x60,
    LS_Comm = 0x40
}Ls_TagType;
#else
typedef struct Ala_ImageInfo
{
    FILE                 *fp;
    int                  fls_size;
    char                 fls_path[256];
    int                  bytes_read;
    Ala_ChannelInfo_t    Channel_Info[10];
    UINT8                channel_cnt;
}Ala_ImageInfo_t;
#endif
typedef struct Ala_lib_Context
{
    IChannel_t            *mchannel;
    Ala_ImageInfo_t       Image_info;
    Ala_TranscieveInfo_t  Transcv_Info;
}Ala_Dwnld_Context_t,*pAla_Dwnld_Context_t;

static UINT8 OpenChannel[] = {0x00, 0x70, 0x00, 0x00, 0x01};
#if(NXP_LDR_SVC_VER_2 == TRUE)
static UINT8 GetData[] = {0x80, 0xCA, 0x00, 0x46, 0x00};
#ifndef NXP_LS_AID
static UINT8 SelectAla[] = {0x00, 0xA4, 0x04, 0x00, 0x0D, 0xA0, 0x00, 0x00, 0x03, 0x96, 0x41, 0x4C, 0x41, 0x01, 0x43, 0x4F, 0x52, 0x01};
#else
static UINT8 SelectAla[] = {0x00, 0xA4, 0x04, 0x00, 0x0F, 0xA0, 0x00, 0x00, 0x03, 0x96, 0x54, 0x43, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0B
, 0x00,0x01};
#endif
#else
static UINT8 SelectAla[] = {0x00, 0xA4, 0x04, 0x00, 0x0D, 0xA0, 0x00, 0x00, 0x03, 0x96, 0x41, 0x4C, 0x41, 0x01, 0x43, 0x4F, 0x52, 0x01};
#endif
/*ALA2*/
#if(NXP_LDR_SVC_VER_2 == TRUE)
#define NOOFAIDS     0x03
#define LENOFAIDS    0x16
#ifndef NXP_LS_AID
static UINT8 ArrayOfAIDs[NOOFAIDS][LENOFAIDS] = {
        {0x12, 0x00, 0xA4, 0x04, 0x00, 0x0D, 0xA0, 0x00, 0x00, 0x03, 0x96, 0x41, 0x4C, 0x41, 0x01, 0x4C, 0x44, 0x52, 0x01,0x00,0x00,0x00},
        {0x12, 0x00, 0xA4, 0x04, 0x00, 0x0D, 0xA0, 0x00, 0x00, 0x03, 0x96, 0x41, 0x4C, 0x41, 0x01, 0x43, 0x4F, 0x52, 0x01,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};
#else
static UINT8 ArrayOfAIDs[NOOFAIDS][LENOFAIDS] = {
        {0x14, 0x00, 0xA4, 0x04, 0x00, 0x0F, 0xA0, 0x00, 0x00, 0x03, 0x96, 0x54, 0x43, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0B,0x00,0x02,0x00},
        {0x14, 0x00, 0xA4, 0x04, 0x00, 0x0F, 0xA0, 0x00, 0x00, 0x03, 0x96, 0x54, 0x43, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0B,0x00,0x01,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};
#endif
#define TAG_CERTIFICATE     0x7F21
#define TAG_LSES_RESP       0x4E
#define TAG_LSES_RSPLEN     0x02
#define TAG_SERIAL_NO       0x93
#define TAG_LSRE_ID         0x42
#define TAG_LSRE_SIGNID     0x45
#define TAG_CERTFHOLD_ID    0x5F20
#define TAG_KEY_USAGE       0x95
#define TAG_EFF_DATE        0x5F25
#define TAG_EXP_DATE        0x5F24
#define TAG_CCM_PERMISSION  0x53
#define TAG_SIG_RNS_COMP    0x5F37

#define TAG_LS_VER1         0x9F
#define TAG_LS_VER2         0x08
#define LS_DEFAULT_STATUS   0x6340
#define LS_SUCCESS_STATUS   0x9000
#define TAG_RE_KEYID        0x65

#define LS_ABORT_SW1        0x69
#define LS_ABORT_SW2        0x87
#define AID_MEM_PATH       "/data/nfc/AID_MEM.txt"
#define LS_STATUS_PATH     "/data/nfc/LS_Status.txt"
#define LS_SRC_BACKUP      "/data/nfc/LS_Src_Backup.txt"
#define LS_DST_BACKUP      "/data/nfc/LS_Dst_Backup.txt"
#define MAX_CERT_LEN        (255+137)

#endif
/*ALA2*/

#define JCOP3_WR
#define APPLET_PATH        "/data/ala/"
#define MAX_SIZE            0xFF
#define PARAM_P1_OFFSET     0x02
#define FIRST_BLOCK         0x05
#define LAST_BLOCK          0x84
#define ONLY_BLOCK          0x85
#define CLA_BYTE            0x80
#define JSBL_HEADER_LEN     0x03
#define ALA_CMD_HDR_LEN     0x02

/* Definations for TAG ID's present in the script file*/
#define TAG_SELECT_ID       0x6F
#define TAG_ALA_ID          0x84
#define TAG_PRO_DATA_ID     0xA5
#define TAG_JSBL_HDR_ID     0x60
#define TAG_JSBL_KEY_ID     0x61
#define TAG_SIGNATURE_ID    0x41
#define TAG_ALA_CMD_ID      0x40
#define TAG_JSBL_CER_ID     0x44

/*Definitions for Install for load*/
#define INSTAL_LOAD_ID      0xE6
#define LOAD_CMD_ID         0xE8
#define LOAD_MORE_BLOCKS    0x00
#define LOAD_LAST_BLOCK     0x80

#define STORE_DATA_CLA      0x80
#define STORE_DATA_INS      0xE2
#define STORE_DATA_LEN      32
#define STORE_DATA_TAG      0x4F

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
BOOLEAN initialize (IChannel_t *channel);

/*******************************************************************************
**
** Function:        finalize
**
** Description:     Release all resources.
**
** Returns:         None
**
*******************************************************************************/
void finalize ();

#if(NXP_LDR_SVC_VER_2 == TRUE)
tJBL_STATUS Perform_ALA(const char *path,const char *dest, const UINT8 *pdata, UINT16 len, UINT8 *respSW);
#else
tJBL_STATUS Perform_ALA(const char *path, const UINT8 *pdata, UINT16 len);
#endif
tJBL_STATUS GetJsbl_Certificate_Refkey(UINT8 *pkey, INT32 *pKeylen);

static tJBL_STATUS
ALA_OpenChannel(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo);

static tJBL_STATUS
ALA_SelectAla(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo);

static tJBL_STATUS
ALA_StoreData(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo);

static tJBL_STATUS
ALA_loadapplet(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info);

#if(NXP_LDR_SVC_VER_2 == TRUE)
static tJBL_STATUS
ALA_update_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo), const char *name, const char *dest);

tJBL_STATUS GetLs_Version(UINT8 *pKey);

tJBL_STATUS GetVer_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo));

tJBL_STATUS Write_Response_To_OutFile(Ala_ImageInfo_t *image_info, UINT8* RecvData, INT32 recvlen, Ls_TagType tType);

tJBL_STATUS Check_Certificate_Tag(UINT8 *read_buf, UINT16 *offset1);

tJBL_STATUS Check_SerialNo_Tag(UINT8 *read_buf, UINT16 *offset1);

tJBL_STATUS Check_LSRootID_Tag(UINT8 *read_buf, UINT16 *offset1);

tJBL_STATUS Check_CertHoldID_Tag(UINT8 *read_buf, UINT16 *offset1);

tJBL_STATUS Check_Date_Tag(UINT8 *read_buf, UINT16 *offset1);

tJBL_STATUS Check_45_Tag(UINT8 *read_buf, UINT16 *offset1, UINT8 *tag45Len);

tJBL_STATUS Certificate_Verification(Ala_ImageInfo_t *Os_info, Ala_TranscieveInfo_t
*pTranscv_Info, UINT8 *read_buf, UINT16 *offset1, UINT8 *tag45Len);

tJBL_STATUS Check_Complete_7F21_Tag(Ala_ImageInfo_t *Os_info,
       Ala_TranscieveInfo_t *pTranscv_Info, UINT8 *read_buf, UINT16 *offset);
BOOLEAN ALA_UpdateExeStatus(UINT16 status);
tJBL_STATUS ALA_getAppletLsStatus(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info);
tJBL_STATUS Get_LsStatus(UINT8 *pVersion);
tJBL_STATUS Get_LsAppletStatus(UINT8 *pVersion);
#else
static tJBL_STATUS
ALA_update_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo), const char *name);
#endif
static tJBL_STATUS
JsblCerId_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo));

tJBL_STATUS ALA_SendtoEse(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info);
#if(NXP_LDR_SVC_VER_2 == TRUE)
tJBL_STATUS GetLsStatus_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t*
        pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo));
tJBL_STATUS ALA_SendtoAla(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info, Ls_TagType tType);
#else
tJBL_STATUS ALA_SendtoAla(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info);
#endif
tJBL_STATUS ALA_CloseChannel(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info);
#if(NXP_LDR_SVC_VER_2 == TRUE)
tJBL_STATUS ALA_ProcessResp(Ala_ImageInfo_t *image_info, INT32 recvlen, Ala_TranscieveInfo_t *trans_info, Ls_TagType tType);
#else
tJBL_STATUS ALA_ProcessResp(Ala_ImageInfo_t *image_info, INT32 recvlen, Ala_TranscieveInfo_t *trans_info);
#endif
#if(NXP_LDR_SVC_VER_2 == TRUE)
tJBL_STATUS ALA_Check_KeyIdentifier(Ala_ImageInfo_t *Os_info, tJBL_STATUS status,
   Ala_TranscieveInfo_t *pTranscv_Info, UINT8* temp_buf, tJBL_STATUS flag,
   INT32 wNewLen);
#else
tJBL_STATUS ALA_Check_KeyIdentifier(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info);
#endif
tJBL_STATUS ALA_ReadScript(Ala_ImageInfo_t *Os_info, UINT8 *read_buf);

tJBL_STATUS Process_EseResponse(Ala_TranscieveInfo_t *pTranscv_Info, INT32 recv_len, Ala_ImageInfo_t *Os_info);

tJBL_STATUS Process_SelectRsp(UINT8* Recv_data, INT32 Recv_len);
#ifdef JCOP3_WR
tJBL_STATUS Send_Backall_Loadcmds(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info);

tJBL_STATUS Bufferize_load_cmds(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info);
#endif
UINT8 Numof_lengthbytes(UINT8 *read_buf, INT32 *wLen);
#endif /*ALA_H*/
