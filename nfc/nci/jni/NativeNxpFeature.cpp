/*
 * Copyright (C) 2012 The Android Open Source Project
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
#include "OverrideLog.h"
#include "NfcJniUtil.h"
#include "SyncEvent.h"
#include "JavaClassConstants.h"
#include "config.h"
#include "NfcAdaptation.h"

extern "C"
{
#include "nfa_api.h"
#include "nfa_rw_api.h"
}
/* Structure to store screen state */
typedef enum screen_state
{
    NFA_SCREEN_STATE_DEFAULT = 0x00,
    NFA_SCREEN_STATE_OFF,
    NFA_SCREEN_STATE_LOCKED,
    NFA_SCREEN_STATE_UNLOCKED
}eScreenState_t;
typedef struct nxp_feature_data
{
    SyncEvent    NxpFeatureConfigEvt;
    tNFA_STATUS  wstatus;
    UINT8        rsp_data[255];
    UINT8        rsp_len;
}Nxp_Feature_Data_t;

extern INT32 gActualSeCount;
namespace android
{
static Nxp_Feature_Data_t gnxpfeature_conf;
void SetCbStatus(tNFA_STATUS status);
tNFA_STATUS GetCbStatus(void);
static void NxpResponse_Cb(UINT8 event, UINT16 param_len, UINT8 *p_param);
static void NxpResponse_SetDhlf_Cb(UINT8 event, UINT16 param_len, UINT8 *p_param);
static void NxpResponse_SetVenConfig_Cb(UINT8 event, UINT16 param_len, UINT8 *p_param);
}

namespace android
{
void SetCbStatus(tNFA_STATUS status)
{
    gnxpfeature_conf.wstatus = status;
}

tNFA_STATUS GetCbStatus(void)
{
    return gnxpfeature_conf.wstatus;
}

static void NxpResponse_Cb(UINT8 event, UINT16 param_len, UINT8 *p_param)
{
    (void)event;
    ALOGD("NxpResponse_Cb Received length data = 0x%x status = 0x%x", param_len, p_param[3]);

    if(p_param[3] == 0x00)
    {
        SetCbStatus(NFA_STATUS_OK);
    }
    else
    {
        SetCbStatus(NFA_STATUS_FAILED);
    }

    SyncEventGuard guard(gnxpfeature_conf.NxpFeatureConfigEvt);
    gnxpfeature_conf.NxpFeatureConfigEvt.notifyOne ();

}
static void NxpResponse_SetDhlf_Cb(UINT8 event, UINT16 param_len, UINT8 *p_param)
{
    (void)event;
    ALOGD("NxpResponse_SetDhlf_Cb Received length data = 0x%x status = 0x%x", param_len, p_param[3]);

    if(p_param[3] == 0x00)
    {
        SetCbStatus(NFA_STATUS_OK);
    }
    else
    {
        SetCbStatus(NFA_STATUS_FAILED);
    }

    SyncEventGuard guard(gnxpfeature_conf.NxpFeatureConfigEvt);
    gnxpfeature_conf.NxpFeatureConfigEvt.notifyOne ();

}

static void NxpResponse_SetVenConfig_Cb(UINT8 event, UINT16 param_len, UINT8 *p_param)
{
    ALOGD("NxpResponse_SetVenConfig_Cb Received length data = 0x%x status = 0x%x", param_len, p_param[3]);
    if(p_param[3] == 0x00)
    {
        SetCbStatus(NFA_STATUS_OK);
    }
    else
    {
        SetCbStatus(NFA_STATUS_FAILED);
    }
    SyncEventGuard guard(gnxpfeature_conf.NxpFeatureConfigEvt);
    gnxpfeature_conf.NxpFeatureConfigEvt.notifyOne ();
}

static void NxpResponse_SetSWPBitRate_Cb(UINT8 event, UINT16 param_len, UINT8 *p_param)
{
    ALOGD("NxpResponse_SetSWPBitRate_CbReceived length data = 0x%x status = 0x%x", param_len, p_param[3]);
    if(p_param[3] == 0x00)
    {
        SetCbStatus(NFA_STATUS_OK);
    }
    else
    {
        SetCbStatus(NFA_STATUS_FAILED);
    }
    SyncEventGuard guard(gnxpfeature_conf.NxpFeatureConfigEvt);
    gnxpfeature_conf.NxpFeatureConfigEvt.notifyOne ();
}

#if(NXP_EXTNS == TRUE)
#if (NFC_NXP_CHIP_TYPE == PN548C2)
/*******************************************************************************
 **
 ** Function:        NxpResponse_EnableAGCDebug_Cb()
 **
 ** Description:     Cb to handle the response of AGC command
 **
 ** Returns:         success/failure
 **
 *******************************************************************************/
static void NxpResponse_EnableAGCDebug_Cb(UINT8 event, UINT16 param_len, UINT8 *p_param)
{
    ALOGD("NxpResponse_EnableAGCDebug_Cb Received length data = 0x%x", param_len);
    SetCbStatus(NFA_STATUS_FAILED);
    if(param_len > 0)
    {
        gnxpfeature_conf.rsp_len = param_len;
        memcpy(gnxpfeature_conf.rsp_data, p_param, gnxpfeature_conf.rsp_len);
        SetCbStatus(NFA_STATUS_OK);
    }
    SyncEventGuard guard(gnxpfeature_conf.NxpFeatureConfigEvt);
    gnxpfeature_conf.NxpFeatureConfigEvt.notifyOne ();
}
/*******************************************************************************
 **
 ** Function:        printDataByte()
 **
 ** Description:     Prints the AGC values
 **
 ** Returns:         success/failure
 **
 *******************************************************************************/
static void printDataByte(UINT16 param_len, UINT8 *p_param)
{
    char print_buffer[param_len * 3 + 1];
    memset (print_buffer, 0, sizeof(print_buffer));
    for (int i = 0; i < param_len; i++)
    {
        snprintf(&print_buffer[i * 2], 3 ,"%02X", p_param[i]);
    }
    ALOGD("AGC Dynamic RSSI values  = %s", print_buffer);
}
/*******************************************************************************
 **
 ** Function:        SendAGCDebugCommand()
 **
 ** Description:     Sends the AGC Debug command.This enables dynamic RSSI
 **                  look up table filling for different "TX RF settings" and enables
 **                  MWdebug prints.
 **
 ** Returns:         success/failure
 **
 *******************************************************************************/
tNFA_STATUS SendAGCDebugCommand()
{
    tNFA_STATUS status = NFA_STATUS_FAILED;
    uint8_t cmd_buf[] = {0x2F, 0x33, 0x04, 0x40, 0x00, 0x40, 0xD8};
    uint8_t dynamic_rssi_buf[50];
    ALOGD("%s: enter", __FUNCTION__);
    SetCbStatus(NFA_STATUS_FAILED);
    gnxpfeature_conf.rsp_len = 0;
    memset(gnxpfeature_conf.rsp_data, 0, 50);
    SyncEventGuard guard (gnxpfeature_conf.NxpFeatureConfigEvt);
    status = NFA_SendNxpNciCommand(sizeof(cmd_buf), cmd_buf, NxpResponse_EnableAGCDebug_Cb);
    if (status == NFA_STATUS_OK)
    {
        ALOGD ("%s: Success NFA_SendNxpNciCommand", __FUNCTION__);
        gnxpfeature_conf.NxpFeatureConfigEvt.wait(1000); /* wait for callback */
    }
    else
    {    tNFA_STATUS status = NFA_STATUS_FAILED;
        ALOGE ("%s: Failed NFA_SendNxpNciCommand", __FUNCTION__);
    }
    status = GetCbStatus();
    if(status == NFA_STATUS_OK && gnxpfeature_conf.rsp_len > 0)
    {
        printDataByte(gnxpfeature_conf.rsp_len, gnxpfeature_conf.rsp_data);
    }
    return status;
}
#endif
/*******************************************************************************
 **
 ** Function:        EmvCo_dosetPoll
 **
 ** Description:     Enable/disable Emv Co polling
 **
 ** Returns:         success/failure
 **
 *******************************************************************************/
tNFA_STATUS EmvCo_dosetPoll(jboolean enable)
{
    tNFA_STATUS status = NFA_STATUS_FAILED;
    uint8_t cmd_buf[] ={0x20, 0x02, 0x05, 0x01, 0xA0, 0x44, 0x01, 0x00};

    ALOGD("%s: enter", __FUNCTION__);

    SetCbStatus(NFA_STATUS_FAILED);
    SyncEventGuard guard (gnxpfeature_conf.NxpFeatureConfigEvt);
    if(enable)
    {
        NFA_SetEmvCoState(TRUE);
        ALOGD("EMV-CO polling profile");
        cmd_buf[7] = 0x01; /*EMV-CO Poll*/
    }
    else
    {
        NFA_SetEmvCoState(FALSE);
        ALOGD("NFC forum polling profile");
    }
    status = NFA_SendNxpNciCommand(sizeof(cmd_buf), cmd_buf, NxpResponse_Cb);
    if (status == NFA_STATUS_OK) {
        ALOGD ("%s: Success NFA_SendNxpNciCommand", __FUNCTION__);
        gnxpfeature_conf.NxpFeatureConfigEvt.wait(); /* wait for callback */
    } else {
        ALOGE ("%s: Failed NFA_SendNxpNciCommand", __FUNCTION__);
    }

    status = GetCbStatus();
    return status;
}

/*******************************************************************************
 **
 ** Function:        SetScreenState
 **
 ** Description:     set/clear SetScreenState
 **
 ** Returns:         success/failure
 **
 *******************************************************************************/
tNFA_STATUS SetScreenState(jint state)
{
    tNFA_STATUS status = NFA_STATUS_FAILED;
    uint8_t screen_off_state_cmd_buff[] = {0x2F, 0x15, 0x01, 0x01};

    ALOGD("%s: enter", __FUNCTION__);

    SetCbStatus(NFA_STATUS_FAILED);
    SyncEventGuard guard (gnxpfeature_conf.NxpFeatureConfigEvt);
    if(state == NFA_SCREEN_STATE_OFF)
    {
        ALOGD("Set Screen OFF");
        screen_off_state_cmd_buff[3] = 0x01;
    }
    else if(state == NFA_SCREEN_STATE_LOCKED)
    {
        ALOGD("Screen ON-locked");
        screen_off_state_cmd_buff[3] = 0x02;
    }
    else if(state == NFA_SCREEN_STATE_UNLOCKED)
    {
        ALOGD("Screen ON-Unlocked");
        screen_off_state_cmd_buff[3] = 0x00;
    }
    else
    {
        ALOGD("Invalid screen state");
    }
    status = NFA_SendNxpNciCommand(sizeof(screen_off_state_cmd_buff), screen_off_state_cmd_buff, NxpResponse_SetDhlf_Cb);
    if (status == NFA_STATUS_OK) {
        ALOGD ("%s: Success NFA_SendNxpNciCommand", __FUNCTION__);
        gnxpfeature_conf.NxpFeatureConfigEvt.wait(); /* wait for callback */
    } else {
        ALOGE ("%s: Failed NFA_SendNxpNciCommand", __FUNCTION__);
    }

    status = GetCbStatus();
    return status;
}

#if(NFC_NXP_CHIP_TYPE == PN547C2)
/*******************************************************************************
 **
 ** Function:        SendAutonomousMode
 **
 ** Description:     set/clear SetDHListenFilter
 **
 ** Returns:         success/failure
 **
 *******************************************************************************/
tNFA_STATUS SendAutonomousMode(jint state ,uint8_t num)
{
    tNFA_STATUS status = NFA_STATUS_FAILED;
    uint8_t autonomos_cmd_buff[] = {0x2F, 0x00, 0x01, 0x00};
    uint8_t core_standby = 0x0;


    ALOGD("%s: enter", __FUNCTION__);

    SetCbStatus(NFA_STATUS_FAILED);
    SyncEventGuard guard (gnxpfeature_conf.NxpFeatureConfigEvt);
    if(state == NFA_SCREEN_STATE_OFF )
    {
        ALOGD("Set Screen OFF");
        /*Standby mode is automatically set with Autonomous mode
         * Value of core_standby will not be considering when state is in SCREEN_OFF Mode*/
        autonomos_cmd_buff[3] = 0x02;
    }
    else if(state == NFA_SCREEN_STATE_UNLOCKED )
    {
        ALOGD("Screen ON-Unlocked");
        core_standby = num;
        autonomos_cmd_buff[3] = 0x00 | core_standby;
    }
    else if(state == NFA_SCREEN_STATE_LOCKED)
    {
        core_standby = num;
        autonomos_cmd_buff[3] = 0x00 | core_standby;
    }
    else
    {
        ALOGD("Invalid screen state");
        return  NFA_STATUS_FAILED;
    }
    status = NFA_SendNxpNciCommand(sizeof(autonomos_cmd_buff), autonomos_cmd_buff, NxpResponse_SetDhlf_Cb);
    if (status == NFA_STATUS_OK) {
        ALOGD ("%s: Success NFA_SendNxpNciCommand", __FUNCTION__);
        gnxpfeature_conf.NxpFeatureConfigEvt.wait(); /* wait for callback */
    } else {
        ALOGE ("%s: Failed NFA_SendNxpNciCommand", __FUNCTION__);
    }

    status = GetCbStatus();
    return status;
}
#endif
//Factory Test Code --start
/*******************************************************************************
 **
 ** Function:        Nxp_SelfTest
 **
 ** Description:     SelfTest SWP, PRBS
 **
 ** Returns:         success/failure
 **
 *******************************************************************************/
tNFA_STATUS Nxp_SelfTest(uint8_t testcase, uint8_t* param)
{
    tNFA_STATUS status = NFA_STATUS_FAILED;
    uint8_t swp_test[] ={0x2F, 0x3E, 0x01, 0x00};   //SWP SelfTest
#if(NFC_NXP_CHIP_TYPE != PN547C2)
    uint8_t prbs_test[] ={0x2F, 0x30, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF};    //PRBS SelfTest
    uint8_t cmd_buf[9] = {0,};
#else
    uint8_t prbs_test[] ={0x2F, 0x30, 0x04, 0x00, 0x00, 0x01, 0xFF};    //PRBS SelfTest
    uint8_t cmd_buf[7] = {0,};
#endif
    //Factory Test Code for PRBS STOP --/
//    uint8_t prbs_stop[] ={0x2F, 0x30, 0x04, 0x53, 0x54, 0x4F, 0x50};  //STOP!!    /*commented to eliminate unused variable warning*/
    uint8_t rst_cmd[] ={0x20, 0x00, 0x01, 0x00};    //CORE_RESET_CMD
    uint8_t init_cmd[] ={0x20, 0x01, 0x00};         //CORE_INIT_CMD
    uint8_t prop_ext_act_cmd[] ={0x2F, 0x02, 0x00};         //CORE_INIT_CMD

    //Factory Test Code for PRBS STOP --/
    uint8_t cmd_len = 0;

    ALOGD("%s: enter", __FUNCTION__);

    NfcAdaptation& theInstance = NfcAdaptation::GetInstance();
    tHAL_NFC_ENTRY* halFuncEntries = theInstance.GetHalEntryFuncs ();

    SetCbStatus(NFA_STATUS_FAILED);
    SyncEventGuard guard (gnxpfeature_conf.NxpFeatureConfigEvt);

    memset(cmd_buf, 0x00, sizeof(cmd_buf));

    switch(testcase){
    case 0 ://SWP Self-Test
        cmd_len = sizeof(swp_test);
        swp_test[3] = param[0];  //select channel 0x00:UICC(SWP1) 0x01:eSE(SWP2)
        memcpy(cmd_buf, swp_test, 4);
        break;

    case 1 ://PRBS Test start
        cmd_len = sizeof(prbs_test);
        //Technology to stream 0x00:TypeA 0x01:TypeB 0x02:TypeF
        //Bitrate                       0x00:106kbps 0x01:212kbps 0x02:424kbps 0x03:848kbps
        memcpy(&prbs_test[3], param, (cmd_len-5));
        memcpy(cmd_buf, prbs_test, cmd_len);
        break;

        //Factory Test Code
    case 2 ://step1. PRBS Test stop : VEN RESET
        halFuncEntries->power_cycle();
        return NFCSTATUS_SUCCESS;
        break;

    case 3 ://step2. PRBS Test stop : CORE RESET
        cmd_len = sizeof(rst_cmd);
        memcpy(cmd_buf, rst_cmd, 4);
        break;

    case 4 ://step3. PRBS Test stop : CORE_INIT
        cmd_len = sizeof(init_cmd);
        memcpy(cmd_buf, init_cmd, cmd_len);
        break;
        //Factory Test Code

    case 5 ://step5. : NXP_ACT_PROP_EXTN
        cmd_len = sizeof(prop_ext_act_cmd);
        memcpy(cmd_buf, prop_ext_act_cmd, 3);
        break;

    default :
        ALOGD("NXP_SelfTest Invalid Parameter!!");
        return status;
    }

    status = NFA_SendNxpNciCommand(cmd_len, cmd_buf, NxpResponse_SetDhlf_Cb);
    if (status == NFA_STATUS_OK) {
        ALOGD ("%s: Success NFA_SendNxpNciCommand", __FUNCTION__);
        gnxpfeature_conf.NxpFeatureConfigEvt.wait(); /* wait for callback */
    } else {
        ALOGE ("%s: Failed NFA_SendNxpNciCommand", __FUNCTION__);
    }

    status = GetCbStatus();
    return status;
}
//Factory Test Code --end

/*******************************************************************************
 **
 ** Function:        SetVenConfigValue
 **
 ** Description:     setting the Ven Config Value
 **
 ** Returns:         success/failure
 **
 *******************************************************************************/
tNFA_STATUS SetVenConfigValue(jint nfcMode)
{
    tNFA_STATUS status = NFA_STATUS_FAILED;
    uint8_t cmd_buf[] = {0x20, 0x02, 0x05, 0x01, 0xA0, 0x07, 0x01, 0x03};
    ALOGD("%s: enter", __FUNCTION__);
    if(nfcMode == NFC_MODE_OFF)
    {
        ALOGD("Setting the VEN_CFG to 2, Disable ESE events");
        cmd_buf[7] = 0x02;
    }
    else if(nfcMode == NFC_MODE_ON)
    {
        ALOGD("Setting the VEN_CFG to 3, Make ");
        cmd_buf[7] = 0x03;
    }
    else
    {
        ALOGE("Wrong VEN_CFG Value");
        return status;
    }
    SetCbStatus(NFA_STATUS_FAILED);
    SyncEventGuard guard (gnxpfeature_conf.NxpFeatureConfigEvt);
    status = NFA_SendNxpNciCommand(sizeof(cmd_buf), cmd_buf, NxpResponse_SetVenConfig_Cb);
    if (status == NFA_STATUS_OK)
    {
        ALOGD ("%s: Success NFA_SendNxpNciCommand", __FUNCTION__);
        gnxpfeature_conf.NxpFeatureConfigEvt.wait(); /* wait for callback */
    }
    else
    {
        ALOGE ("%s: Failed NFA_SendNxpNciCommand", __FUNCTION__);
    }
    status = GetCbStatus();
    return status;
}

static void NxpResponse_GetSwpStausValueCb(UINT8 event, UINT16 param_len, UINT8 *p_param)
{
    ALOGD("NxpResponse_GetSwpStausValueCb length data = 0x%x status = 0x%x", param_len, p_param[3]);
    if(p_param[3] == 0x00)
    {
        if (p_param[8] != 0x00)
        {
            ALOGD("SWP1 Interface is enabled");
            gActualSeCount++;
        }
        if (p_param[12] != 0x00)
        {
            ALOGD("SWP2 Interface is enabled");
            gActualSeCount++;
        }
    }
    else
    {
        /* for fail case assign max no of smx */
        gActualSeCount = 3;
    }
    SetCbStatus(NFA_STATUS_OK);
    SyncEventGuard guard(gnxpfeature_conf.NxpFeatureConfigEvt);
    gnxpfeature_conf.NxpFeatureConfigEvt.notifyOne ();
}
/*******************************************************************************
 **
 ** Function:        GetSwpStausValue
 **
 ** Description:     Get the current SWP1 and SWP2 status
 **
 ** Returns:         success/failure
 **
 *******************************************************************************/
tNFA_STATUS GetSwpStausValue(void)
{
    tNFA_STATUS status = NFA_STATUS_FAILED;
    gActualSeCount = 1; /* default ese present */
    uint8_t cmd_buf[] = {0x20, 0x03, 0x05, 0x02, 0xA0, 0xEC, 0xA0, 0xED};
    ALOGD("%s: enter", __FUNCTION__);

    SetCbStatus(NFA_STATUS_FAILED);
    SyncEventGuard guard (gnxpfeature_conf.NxpFeatureConfigEvt);
    status = NFA_SendNxpNciCommand(sizeof(cmd_buf), cmd_buf, NxpResponse_GetSwpStausValueCb);
    if (status == NFA_STATUS_OK)
    {
        ALOGD ("%s: Success NFA_SendNxpNciCommand", __FUNCTION__);
        gnxpfeature_conf.NxpFeatureConfigEvt.wait(); /* wait for callback */
    }
    else
    {
        ALOGE ("%s: Failed NFA_SendNxpNciCommand", __FUNCTION__);
    }
    status = GetCbStatus();
    ALOGD("%s : gActualSeCount = %d",__FUNCTION__, gActualSeCount);
    return status;
}

#if(NFC_NXP_HFO_SETTINGS == TRUE)
/*******************************************************************************
 **
 ** Function:        SetHfoConfigValue
 **
 ** Description:    Configuring the HFO clock in case of phone power off
 **                       to make CE works in phone off.
 **
 ** Returns:         success/failure
 **
 *******************************************************************************/
tNFA_STATUS SetHfoConfigValue(void)
{
/* set 4 RF registers for phone off
 *
# A0, 0D, 06, 06, 83, 10, 10, 40, 00 RF_CLIF_CFG_TARGET CLIF_DPLL_GEAR_REG
# A0, 0D, 06, 06, 82, 13, 14, 17, 00 RF_CLIF_CFG_TARGET CLIF_DPLL_INIT_REG
# A0, 0D, 06, 06, 84, AA, 85, 00, 00 RF_CLIF_CFG_TARGET CLIF_DPLL_INIT_FREQ_REG
# A0, 0D, 06, 06, 81, 63, 02, 00, 00 RF_CLIF_CFG_TARGET CLIF_DPLL_CONTROL_REG
*/
/* default value of four registers in nxp-ALMSL.conf need to set in full power on
# A0, 0D, 06, 06, 83, 55, 2A, 04, 00 RF_CLIF_CFG_TARGET CLIF_DPLL_GEAR_REG
# A0, 0D, 06, 06, 82, 33, 14, 17, 00 RF_CLIF_CFG_TARGET CLIF_DPLL_INIT_REG
# A0, 0D, 06, 06, 84, AA, 85, 00, 80 RF_CLIF_CFG_TARGET CLIF_DPLL_INIT_FREQ_REG
# A0, 0D, 06, 06, 81, 63, 00, 00, 00 RF_CLIF_CFG_TARGET CLIF_DPLL_CONTROL_REG

*/
    tNFA_STATUS status = NFA_STATUS_FAILED;
    uint8_t cmd_buf[] = {0x20, 0x02, 0x29, 0x05, 0xA0, 0x03, 0x01, 0x06,
                                                 0xA0, 0x0D, 0x06, 0x06, 0x83, 0x10, 0x10, 0x40, 0x00,
                                                 0xA0, 0x0D, 0x06, 0x06, 0x82, 0x13, 0x14, 0x17, 0x00,
                                                 0xA0, 0x0D, 0x06, 0x06, 0x84, 0xAA, 0x85, 0x00, 0x00,
                                                 0xA0, 0x0D, 0x06, 0x06, 0x81, 0x63, 0x02, 0x00, 0x00 };
    ALOGD("%s: enter", __FUNCTION__);

    SetCbStatus(NFA_STATUS_FAILED);
    SyncEventGuard guard (gnxpfeature_conf.NxpFeatureConfigEvt);
    status = NFA_SendNxpNciCommand(sizeof(cmd_buf), cmd_buf, NxpResponse_SetVenConfig_Cb);
    if (status == NFA_STATUS_OK)
    {
        ALOGD ("%s: Success NFA_SendNxpNciCommand", __FUNCTION__);
        gnxpfeature_conf.NxpFeatureConfigEvt.wait(); /* wait for callback */
    }
    else
    {
        ALOGE ("%s: Failed NFA_SendNxpNciCommand", __FUNCTION__);
    }
    status = GetCbStatus();
    if (NFA_STATUS_OK == status)
    {
        ALOGD ("%s: HFO Settinng Success", __FUNCTION__);
         // TBD write value in temp file in /data/nfc
        // At next boot hal will read this file and re-apply the
        // Default Clock setting
    }
    ALOGD("%s: exit", __FUNCTION__);
    return status;
}
#endif

/*******************************************************************************
 **
 ** Function:        ResetEseSession
 **
 ** Description:     Resets the Ese session identity to FF
 **
 ** Returns:         success/failure
 **
 *******************************************************************************/
tNFA_STATUS ResetEseSession()
{
    tNFA_STATUS status = NFA_STATUS_FAILED;
    static uint8_t cmd_buf[] = { 0x20, 0x02, 0x0C, 0x01,
                                 0xA0, 0xEB, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    ALOGD("%s: enter", __FUNCTION__);

    SetCbStatus(NFA_STATUS_FAILED);
    SyncEventGuard guard (gnxpfeature_conf.NxpFeatureConfigEvt);
    status = NFA_SendNxpNciCommand(sizeof(cmd_buf), cmd_buf, NxpResponse_Cb);
    if (status == NFA_STATUS_OK)
    {
        ALOGD ("%s: Success NFA_SendNxpNciCommand", __FUNCTION__);
        gnxpfeature_conf.NxpFeatureConfigEvt.wait(); /* wait for callback */
    }
    else
    {
        ALOGE ("%s: Failed NFA_SendNxpNciCommand", __FUNCTION__);
    }
    status = GetCbStatus();
    if (NFA_STATUS_OK == status)
    {
        ALOGD ("%s: ResetEseSession identity is Success", __FUNCTION__);
    }
    ALOGD("%s: exit", __FUNCTION__);
    return status;
}
/*******************************************************************************
 **
 ** Function:        SetUICC_SWPBitRate()
 **
 ** Description:     Get All UICC Parameters and set SWP bit rate
 **
 ** Returns:         success/failure
 **
 *******************************************************************************/
tNFA_STATUS SetUICC_SWPBitRate(bool isMifareSupported)
{
    tNFA_STATUS status = NFA_STATUS_FAILED;
    uint8_t cmd_buf[] ={0x20, 0x02, 0x05, 0x01, 0xA0, 0xC0, 0x01, 0x03};

    if(isMifareSupported)
    {
        ALOGD("Setting the SWP_BITRATE_INT1 to 0x06 (1250 kb/s)");
        cmd_buf[7] = 0x06;
    }
    else
    {
        ALOGD("Setting the SWP_BITRATE_INT1 to 0x04 (910 kb/s)");
        cmd_buf[7] = 0x04;
    }

    SetCbStatus(NFA_STATUS_FAILED);
    SyncEventGuard guard (gnxpfeature_conf.NxpFeatureConfigEvt);
    status = NFA_SendNxpNciCommand(sizeof(cmd_buf), cmd_buf, NxpResponse_SetSWPBitRate_Cb);
    if (status == NFA_STATUS_OK)
    {
        ALOGD ("%s: Success NFA_SendNxpNciCommand", __FUNCTION__);
        gnxpfeature_conf.NxpFeatureConfigEvt.wait(); /* wait for callback */
    }
    else
    {
         ALOGE ("%s: Failed NFA_SendNxpNciCommand", __FUNCTION__);
    }
    status = GetCbStatus();
    return status;
}
#endif
} /*namespace android*/
