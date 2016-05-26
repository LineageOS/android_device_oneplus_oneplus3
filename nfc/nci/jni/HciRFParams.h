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

#include "OverrideLog.h"
#include "SyncEvent.h"
#include <cstdlib>
#include <cstring>

extern "C"
{
    #include "nfa_api.h"
}/*namespace android*/

namespace android {

extern SyncEvent sNfaGetConfigEvent;

}  // namespace android

class HciRFParams
{
public:
    /*******************************************************************************
    **
    ** Function:        getInstance
    **
    ** Description:     Get the HciRFParams singleton object.
    **
    ** Returns:         HciRFParams object.
    **
    *******************************************************************************/
    static HciRFParams& getInstance ();

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
    bool initialize ();


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

    bool isTypeBSupported();

    void connectionEventHandler (UINT8 dmEvent, tNFA_DM_CBACK_DATA* eventData);

    void getESeUid(uint8_t* uidbuff, uint8_t* uidlen);
    uint8_t getESeSak();

private:

    uint8_t bPipeStatus_CeA;
    uint8_t bMode_CeA;
    uint8_t bUidRegSize_CeA;
    uint8_t aUidReg_CeA[10];
    uint8_t bSak_CeA;
    uint8_t aATQA_CeA[2];
    uint8_t bApplicationDataSize_CeA;
    uint8_t aApplicationData_CeA[15];
    uint8_t bFWI_SFGI_CeA;
    uint8_t bCidSupport_CeA;
    uint8_t bCltSupport_CeA;
    uint8_t aDataRateMax_CeA[3];
    uint8_t bPipeStatus_CeB;
    uint8_t bMode_CeB;
    uint8_t aPupiRegDataSize_CeB;
    uint8_t aPupiReg_CeB[4];
    uint8_t bAfi_CeB;
    uint8_t aATQB_CeB[4];
    uint8_t bHighLayerRspSize_CeB;
    uint8_t aHighLayerRsp_CeB_CeB[15];
    uint8_t aDataRateMax_CeB[3];

    tNFA_GET_CONFIG *get_config;

    static HciRFParams sHciRFParams;

    bool mIsInit;                // whether HciRFParams is initialized
    SyncEvent mGetConfigEvent;


    SyncEvent mUiccListenEvent;
    SyncEvent mAidRegisterEvent;
    SyncEvent mAidDegisterEvent;
    SyncEvent mDataRecvEvent;


    /*******************************************************************************
    **
    ** Function:        HciRFParams
    **
    ** Description:     Initialize member variables.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    HciRFParams ();


    /*******************************************************************************
    **
    ** Function:        ~HciRFParams
    **
    ** Description:     Release all resources.
    **
    ** Returns:         None
    **
    *******************************************************************************/
    ~HciRFParams ();



};
