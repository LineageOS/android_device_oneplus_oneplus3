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
#include "DwpChannel.h"
#include "SecureElement.h"
#include "RoutingManager.h"
#include <cutils/log.h>

static const int EE_ERROR_OPEN_FAIL =  -1;

bool IsWiredMode_Enable();
bool eSE_connected = false;
bool dwpChannelForceClose = false;

/*******************************************************************************
**
** Function:        IsWiredMode_Enable
**
** Description:     Provides the connection status of EE
**
** Returns:         True if ok.
**
*******************************************************************************/
bool IsWiredMode_Enable()
{
    static const char fn [] = "DwpChannel::IsWiredMode_Enable";
    ALOGD ("%s: enter", fn);
    SecureElement &se = SecureElement::getInstance();
    tNFA_STATUS stat = NFA_STATUS_FAILED;

    static const int MAX_NUM_EE = 5;
    UINT16 meSE =0x4C0;
    UINT8 mActualNumEe;
    tNFA_EE_INFO EeInfo[MAX_NUM_EE];
    mActualNumEe = MAX_NUM_EE;

#if 0
    if(mIsInit == false)
    {
        ALOGD ("%s: JcopOs Dwnld is not initialized", fn);
        goto TheEnd;
    }
#endif
    stat = NFA_EeGetInfo(&mActualNumEe, EeInfo);
    if(stat == NFA_STATUS_OK)
    {
        for(int xx = 0; xx <  mActualNumEe; xx++)
        {
            ALOGE("xx=%d, ee_handle=0x0%x, status=0x0%x", xx, EeInfo[xx].ee_handle,EeInfo[xx].ee_status);
            if (EeInfo[xx].ee_handle == meSE)
            {
                if(EeInfo[xx].ee_status == 0x00)
                {
                    stat = NFA_STATUS_OK;
                    ALOGD ("%s: status = 0x%x", fn, stat);
                    break;
                }
                else if(EeInfo[xx].ee_status == 0x01)
                {
                    ALOGE("%s: Enable eSE-mode set ON", fn);
                    se.SecEle_Modeset(0x01);
                    usleep(2000 * 1000);
                    stat = NFA_STATUS_OK;
                    break;
                }
                else
                {
                    stat = NFA_STATUS_FAILED;
                    break;
                }
            }
            else
            {
                stat = NFA_STATUS_FAILED;
            }

        }
    }
//TheEnd: /*commented to eliminate the label defined but not used warning*/
    ALOGD("%s: exit; status = 0x%X", fn, stat);
    if(stat == NFA_STATUS_OK)
        return true;
    else
        return false;
}

/*******************************************************************************
**
** Function:        open
**
** Description:     Opens the DWP channel to eSE
**
** Returns:         True if ok.
**
*******************************************************************************/

INT16 open()
{
    static const char fn [] = "DwpChannel::open";
    bool stat = false;
    INT16 dwpHandle = EE_ERROR_OPEN_FAIL;
    SecureElement &se = SecureElement::getInstance();

    ALOGE("DwpChannel: Sec Element open Enter");

    if (se.isBusy())
    {
        ALOGE("DwpChannel: SE is busy");
        return EE_ERROR_OPEN_FAIL;
    }

    eSE_connected = IsWiredMode_Enable();
    if(eSE_connected != true)
    {
        ALOGE("DwpChannel: Wired mode is not connected");
        return EE_ERROR_OPEN_FAIL;
    }

    /*turn on the sec elem*/
    stat = se.activate(SecureElement::ESE_ID);

    if (stat)
    {
        //establish a pipe to sec elem
        stat = se.connectEE();
        if (!stat)
        {
          se.deactivate (0);
        }else
        {
          dwpChannelForceClose = false;
          dwpHandle = se.mActiveEeHandle;
        }
    }

    ALOGD("%s: Exit. dwpHandle = 0x%02x", fn,dwpHandle);
    return dwpHandle;
}
/*******************************************************************************
**
** Function:        close
**
** Description:     closes the DWP connection with eSE
**
** Returns:         True if ok.
**
*******************************************************************************/

bool close(INT16 mHandle)
{
    static const char fn [] = "DwpChannel::close";
    ALOGD("%s: enter", fn);
    bool stat = false;
    SecureElement &se = SecureElement::getInstance();
    if(mHandle == EE_ERROR_OPEN_FAIL)
    {
        ALOGD("%s: Channel access denied. Returning", fn);
        return stat;
    }
    if(eSE_connected != true)
        return true;

    stat = se.sendEvent(SecureElement::EVT_END_OF_APDU_TRANSFER);

    stat = se.disconnectEE (SecureElement::ESE_ID);

    //if controller is not routing AND there is no pipe connected,
    //then turn off the sec elem
    #if((NFC_NXP_CHIP_TYPE == PN547C2)&&(NFC_NXP_ESE == TRUE))
    if (! se.isBusy())
        se.deactivate (SecureElement::ESE_ID);
    #endif
     return stat;
}

bool transceive (UINT8* xmitBuffer, INT32 xmitBufferSize, UINT8* recvBuffer,
                 INT32 recvBufferMaxSize, INT32& recvBufferActualSize, INT32 timeoutMillisec)
{
    static const char fn [] = "DwpChannel::transceive";
    bool stat = false;
    SecureElement &se = SecureElement::getInstance();
    ALOGD("%s: enter", fn);

    /*When Nfc deinitialization triggered*/
    if(dwpChannelForceClose == true)
        return stat;

    stat = se.transceive (xmitBuffer,
                          xmitBufferSize,
                          recvBuffer,
                          recvBufferMaxSize,
                          recvBufferActualSize,
                          timeoutMillisec);
    ALOGD("%s: exit", fn);
    return stat;
}

void doeSE_Reset(void)
{
    static const char fn [] = "DwpChannel::doeSE_Reset";
    SecureElement &se = SecureElement::getInstance();
    RoutingManager &rm = RoutingManager::getInstance();
    ALOGD("%s: enter:", fn);

    rm.mResetHandlerMutex.lock();
    ALOGD("1st mode set calling");
    se.SecEle_Modeset(0x00);
    usleep(100 * 1000);
    ALOGD("1st mode set called");
    ALOGD("2nd mode set calling");

    se.SecEle_Modeset(0x01);
    ALOGD("2nd mode set called");

    usleep(2000 * 1000);
    rm.mResetHandlerMutex.unlock();
    if((RoutingManager::getInstance().is_ee_recovery_ongoing()))
    {
        ALOGE ("%s: is_ee_recovery_ongoing ", fn);
        SyncEventGuard guard (se.mEEdatapacketEvent);
        se.mEEdatapacketEvent.wait();
    }
    else
    {
       ALOGE ("%s: Not in Recovery State", fn);
    }
}
namespace android
{
    void doDwpChannel_ForceExit()
    {
        static const char fn [] = "DwpChannel::doDwpChannel_ForceExit";
        ALOGD("%s: enter:", fn);
        dwpChannelForceClose = true;
        ALOGD("%s: exit", fn);
    }
}
