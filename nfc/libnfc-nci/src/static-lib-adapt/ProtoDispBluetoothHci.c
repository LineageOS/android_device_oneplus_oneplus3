/******************************************************************************
 *
 *  Copyright (C) 2011-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
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
#include "OverrideLog.h"
#include "ProtoDispBluetoothHci.h"
#include "nfc_target.h"
#include <cutils/log.h>


extern UINT8 *HCIDisp1 (char *p_descr, UINT8 *p_data);
extern UINT32 ScrProtocolTraceFlag;
#define HCI_GEN_TRACE   (TRACE_CTRL_GENERAL | TRACE_LAYER_HCI | \
                         TRACE_ORG_PROTO_DISP | hci_trace_type)
static UINT8 hci_trace_type = 0;
static char* modes_str [] =
{
    "No sleep mode",
    "UART",
    "UART with messaging",
    "USB",
    "H4IBSS",
    "USB with host wake",
    "SDIO",
    "UART CS-N",
    "SPI",
    "H5",
    "H4DS",
    "",
    "UART with BREAK"
};
static UINT8* p_end_hci = NULL;
static UINT8* HCIDisp1Ext (char *p_descr, UINT8 *p_data, char * p_ext);
static void disp_sleepmode (UINT8* p);
static void disp_sleepmode_evt (UINT8* p);


///////////////////////////////////////////
///////////////////////////////////////////


UINT8 *HCIDisp1Ext (char *p_descr, UINT8 *p_data, char * p_ext)
{
    if (p_data == p_end_hci)
        return p_data;

    char    buff[200];

    sprintf (buff, "%40s : %u (0x%02x): %s", p_descr, *p_data, *p_data, p_ext);

    ScrLog (HCI_GEN_TRACE, "%s", buff);
    return (p_data + 1);
}


/*******************************************************************************
**
** Function         disp_sleepmode
**
** Description      Displays VSC sleep mode
**
** Returns          none.
**
*******************************************************************************/
void disp_sleepmode(UINT8 * p)
{
    hci_trace_type = TRACE_TYPE_CMD_TX;
    ScrLog (HCI_GEN_TRACE, "--");
    int len     = p[2];
    ScrLog (HCI_GEN_TRACE, "SEND Command to HCI.  Name: Set_Sleepmode_Param   (Hex Code: 0xfc27  Param Len: %d)", len);
    p       += 3;
    p_end_hci   = p + len;
    p = HCIDisp1Ext("Sleep_Mode", p, (*p <= 12) ? modes_str[*p] : "");
    p = HCIDisp1("Idle_Threshold_Host", p);
    p = HCIDisp1("Idle_Threshold_HC", p);
    p = HCIDisp1Ext("BT_WAKE_Active_Mode", p, (*p == 0) ? "Active Low" : ((*p == 1) ? "Active High" : ""));
    p = HCIDisp1Ext("HOST_WAKE_Active_Mode", p, (*p == 0) ? "Active Low" : ((*p == 1) ? "Active High" : ""));
    p = HCIDisp1("Allow_Host_Sleep_During_SCO", p);
    p = HCIDisp1("Combine_Sleep_Mode_And_LPM", p);
    p = HCIDisp1("Enable_Tristate_Control_Of_UART_Tx_Line", p);
    p = HCIDisp1Ext("Active_Connection_Handling_On_Suspend", p, (*p == 0) ? "Maintain connections; sleep when timed activity allows" : ((*p == 1) ? "Sleep until resume is detected" : ""));
    p = HCIDisp1("Resume_Timeout", p);
    p = HCIDisp1("Enable_BREAK_To_Host", p);
    p = HCIDisp1("Pulsed_HOST_WAKE", p);

    ScrLog (HCI_GEN_TRACE, "--");
}


/*******************************************************************************
**
** Function         disp_sleepmode_evt
**
** Description      Displays HCI comand complete event for VSC sleep mode.
**
** Returns          none.
**
*******************************************************************************/
void disp_sleepmode_evt(UINT8* p)
{
    UINT8   len=p[1], status=p[5];

    hci_trace_type = TRACE_TYPE_EVT_RX;
    ScrLog (HCI_GEN_TRACE, "--");
    ScrLog (HCI_GEN_TRACE, "RCVD Event from HCI. Name: HCI_Command_Complete  (Hex Code: 0x0e  Param Len: %d)", len);

    p = HCIDisp1 ("Num HCI Cmd Packets", p+2);
    ScrLog (HCI_GEN_TRACE,"%40s : 0xfc27  (Set_Sleepmode_Param)", "Cmd Code");
    ScrLog (HCI_GEN_TRACE, "%40s : %d (0x%02x) %s", "Status", status, status, (status == 0) ? "Success" : "");
    ScrLog (HCI_GEN_TRACE, "--");
}

/*******************************************************************************
**
** Function         ProtoDispBluetoothHciCmd
**
** Description      Display a HCI command string
**
** Returns:
**                  Nothing
**
*******************************************************************************/
void ProtoDispBluetoothHciCmd (BT_HDR *p_buf)
{
    if (!(ScrProtocolTraceFlag & SCR_PROTO_TRACE_HCI_SUMMARY))
        return;
    UINT8 * p = (UINT8 *)(p_buf + 1) + p_buf->offset;
    if (*(p) == 0x27 && *(p+1) == 0xfc) // opcode sleep mode
    {
        disp_sleepmode(p);
    }
}


/*******************************************************************************
**
** Function         ProtoDispBluetoothHciEvt
**
** Description      display a NCI event
**
** Returns:
**                  Nothing
**
*******************************************************************************/
void ProtoDispBluetoothHciEvt (BT_HDR *pBuffer)
{
    if (!(ScrProtocolTraceFlag & SCR_PROTO_TRACE_HCI_SUMMARY))
        return;

    UINT8   *p = (UINT8 *)(pBuffer + 1) + pBuffer->offset;
    if (*p == 0x0e) // command complete
    {
        if (*(p+1) == 4)    // length
        {
            if (*(p+3) == 0x27 && *(p+4) == 0xfc)   // opcode 0x27fc (sleep mode)
            {
                disp_sleepmode_evt(p);
            }
        }
    }
}
