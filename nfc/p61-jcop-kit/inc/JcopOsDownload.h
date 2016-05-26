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

#include "data_types.h"
#include "IChannel.h"
#include <stdio.h>

typedef struct JcopOs_TranscieveInfo
{
    INT32 timeout;
    UINT8 sRecvData[1024];
    UINT8 sSendData[1024];
    INT32 sSendlength;
    int sRecvlength;
}JcopOs_TranscieveInfo_t;

typedef struct JcopOs_Version_Info
{
    UINT8 osid;
    UINT8 ver1;
    UINT8 ver0;
    UINT8 OtherValid;
    UINT8 ver_status;
}JcopOs_Version_Info_t;
typedef struct JcopOs_ImageInfo
{
    FILE *fp;
    int   fls_size;
    char  fls_path[256];
    int   index;
    UINT8 cur_state;
    JcopOs_Version_Info_t    version_info;
}JcopOs_ImageInfo_t;
typedef struct JcopOs_Dwnld_Context
{
    JcopOs_Version_Info_t    version_info;
    JcopOs_ImageInfo_t       Image_info;
    JcopOs_TranscieveInfo_t  pJcopOs_TransInfo;
    IChannel_t               *channel;
}JcopOs_Dwnld_Context_t,*pJcopOs_Dwnld_Context_t;


static UINT8 Trigger_APDU[] = {0x4F, 0x70, 0x80, 0x13, 0x04, 0xDE, 0xAD, 0xBE, 0xEF, 0x00};
static UINT8 GetInfo_APDU[] = {0x00, //CLA
                               0xA4, 0x04, 0x00, 0x0C, //INS, P1, P2, Lc
                               0xD2, 0x76, 0x00, 0x00, 0x85, 0x41, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00,   //Data
                               0x00 //Le
                              };
static UINT8 GetInfo_Data[] = {0x55, 0x70, 0x64, 0x61, 0x74, 0x65, 0x72, 0x4F, 0x53};

#define OSID_OFFSET  9
#define VER1_OFFSET  10
#define VER0_OFFSET  11
#define JCOPOS_HEADER_LEN 5

#define JCOP_UPDATE_STATE0 0
#define JCOP_UPDATE_STATE1 1
#define JCOP_UPDATE_STATE2 2
#define JCOP_UPDATE_STATE3 3
#define JCOP_MAX_RETRY_CNT 3
#define JCOP_INFO_PATH  "/data/nfc/jcop_info.txt"

class JcopOsDwnld
{
public:

/*******************************************************************************
**
** Function:        getInstance
**
** Description:     Get the SecureElement singleton object.
**
** Returns:         SecureElement object.
**
*******************************************************************************/
static JcopOsDwnld* getInstance ();

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
bool initialize (IChannel_t *channel);

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

tJBL_STATUS JcopOs_Download();

tJBL_STATUS TriggerApdu(JcopOs_ImageInfo_t* pVersionInfo, tJBL_STATUS status, JcopOs_TranscieveInfo_t* pTranscv_Info);

tJBL_STATUS GetInfo(JcopOs_ImageInfo_t* pVersionInfo, tJBL_STATUS status, JcopOs_TranscieveInfo_t* pTranscv_Info);

tJBL_STATUS load_JcopOS_image(JcopOs_ImageInfo_t *Os_info, tJBL_STATUS status, JcopOs_TranscieveInfo_t *pTranscv_Info);

tJBL_STATUS JcopOs_update_seq_handler();

IChannel_t *mchannel;

private:
static JcopOsDwnld sJcopDwnld;
bool mIsInit;
tJBL_STATUS GetJcopOsState(JcopOs_ImageInfo_t *Os_info, UINT8 *counter);
tJBL_STATUS SetJcopOsState(JcopOs_ImageInfo_t *Os_info, UINT8 state);
};
