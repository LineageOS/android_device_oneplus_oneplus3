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


#include "HciRFParams.h"

#define VAL_START_IDX 4
#define MAX_AID_SIZE 10
#define MAX_APP_DATA_SIZE 15
#define MAX_HIGHER_LAYER_RSP_SIZE 15

HciRFParams HciRFParams::sHciRFParams;

/*******************************************************************************
 **
 ** Function:        HciRFParams
 **
 ** Description:     Initialize member variables.
 **
 ** Returns:         None
 **
 *******************************************************************************/
HciRFParams::HciRFParams ()
{
  memset (aATQA_CeA, 0, sizeof(aATQA_CeA));
  memset (aATQB_CeB, 0, sizeof(aATQB_CeB));
  memset (aApplicationData_CeA, 0, sizeof(aApplicationData_CeA));
  memset (aDataRateMax_CeA, 0, sizeof(aDataRateMax_CeA));
  memset (aDataRateMax_CeB, 0, sizeof(aDataRateMax_CeB));
  memset (aHighLayerRsp_CeB_CeB, 0, sizeof(aHighLayerRsp_CeB_CeB));
  memset (aPupiReg_CeB, 0, sizeof(aPupiReg_CeB));
  memset (aUidReg_CeA, 0, sizeof(aUidReg_CeA));
  bMode_CeA=0;
  bUidRegSize_CeA=0;
  bSak_CeA =0;
  bApplicationDataSize_CeA=0;
  bFWI_SFGI_CeA=0;
  bCidSupport_CeA=0;
  bCltSupport_CeA=0;
  bPipeStatus_CeA=0;
  bPipeStatus_CeB=0;
  bMode_CeB=0;
  bAfi_CeB=0;
  bHighLayerRspSize_CeB=0;
  mIsInit=false;
}

/*******************************************************************************
 **
 ** Function:        ~HciRFParams
 **
 ** Description:     Release all resources.
 **
 ** Returns:         None
 **
 *******************************************************************************/
HciRFParams::~HciRFParams ()
{
}

/*******************************************************************************
 **
 ** Function:        getInstance
 **
 ** Description:     Get the HciRFParams singleton object.
 **
 ** Returns:         HciRFParams object.
 **
 *******************************************************************************/
HciRFParams& HciRFParams::getInstance()
{
    return sHciRFParams;
}

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
bool HciRFParams::initialize ()
{
    static const char fn [] = "HciRFParams::initialize";
    ALOGD ("%s: enter", fn);

    tNFA_PMID param_ids[] = {0xA0, 0xF0};
    {
        SyncEventGuard guard (android::sNfaGetConfigEvent);
        tNFA_STATUS stat = NFA_GetConfig(0x01,param_ids);
//        NFA_GetConfig(0x01,param_ids);
        if(stat == NFA_STATUS_OK)
        {
            android::sNfaGetConfigEvent.wait();
        }
        else
        {
            return false;
        }
    }
    ALOGD ("%s: status %x", __FUNCTION__,get_config->status);
    ALOGD ("%s: tlv_size %d", __FUNCTION__,get_config->tlv_size);
    ALOGD ("%s: param_tlvs %x", __FUNCTION__,get_config->param_tlvs[0]);

    uint8_t *params = get_config->param_tlvs;
    params+=VAL_START_IDX;

    bPipeStatus_CeA = *params++;
    bMode_CeA = *params++;
    bUidRegSize_CeA = *params++;

    memcpy(aUidReg_CeA,params,bUidRegSize_CeA);
    params += MAX_AID_SIZE;

    bSak_CeA = *params++;

    aATQA_CeA[0] = *params++;
    aATQA_CeA[1] = *params++;
    bApplicationDataSize_CeA = *params++;

    memcpy(aApplicationData_CeA,params,bApplicationDataSize_CeA);
    params += MAX_APP_DATA_SIZE;

    bFWI_SFGI_CeA = *params++;
    bCidSupport_CeA =  *params++;
    bCltSupport_CeA =  *params++;

    memcpy(aDataRateMax_CeA,params,0x03);
    params += 3;

    bPipeStatus_CeB = *params++;
    bMode_CeB = *params++;

#if(NFC_NXP_CHIP_TYPE != PN547C2)
        aPupiRegDataSize_CeB = *params++;
#endif
        aPupiRegDataSize_CeB = 4;


    memcpy(aPupiReg_CeB,params, aPupiRegDataSize_CeB);
    params += aPupiRegDataSize_CeB;

    bAfi_CeB = *params++;

    memcpy(aATQB_CeB,params,0x04);
    params += 4;

    bHighLayerRspSize_CeB = *params++;

    memcpy(aHighLayerRsp_CeB_CeB,params,bHighLayerRspSize_CeB);
    params += MAX_HIGHER_LAYER_RSP_SIZE;

    memcpy(aDataRateMax_CeB,params,0x03);
//    aDataRateMax_CeB[3];


    mIsInit = true;

    ALOGD ("%s: exit", fn);
    return (true);
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
void HciRFParams::finalize ()
{
    static const char fn [] = "HciRFParams::finalize";
    ALOGD ("%s: enter", fn);

    mIsInit       = false;

    ALOGD ("%s: exit", fn);
}


void HciRFParams::connectionEventHandler (UINT8 event, tNFA_DM_CBACK_DATA* eventData)
{
//    static const char fn [] = "HciRFParams::connectionEventHandler";  /*commented to eliminate unused variable warning*/

    switch (event)
    {
    case NFA_DM_GET_CONFIG_EVT:
    {
        get_config = (tNFA_GET_CONFIG*) eventData;
//        SyncEventGuard guard (android::sNfaGetConfigEvent);
//        android::sNfaGetConfigEvent.notifyOne ();
    }
    break;
    }
}

void HciRFParams::getESeUid(uint8_t* uidbuff, uint8_t* uidlen)
{
    if(false == mIsInit || *uidlen < bUidRegSize_CeA || (uint8_t)NULL == *uidbuff)
    {
        *uidlen = 0x00;
        *uidbuff = (uint8_t)NULL;
    }

    memcpy(uidbuff,aUidReg_CeA,bUidRegSize_CeA);
    *uidlen = bUidRegSize_CeA;

}

uint8_t HciRFParams::getESeSak()
{
    if(false == mIsInit)
    {
        return 0x00;
    }

    return bSak_CeA;
}

bool HciRFParams::isTypeBSupported()
{
    bool status = false;

    if(false == mIsInit)
    {
        return 0x00;
    }

    if(bPipeStatus_CeB == 0x02 &&
            bMode_CeB == 0x02)
    {
        status = true;
    }
    return status;
}
