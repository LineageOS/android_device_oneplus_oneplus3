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
#include "JcDnld.h"
#include "JcopOsDownload.h"
#include <data_types.h>
#include <cutils/log.h>

JcopOsDwnld *jd;
IChannel_t *channel;
static bool inUse = false;
static INT16 jcHandle;
extern pJcopOs_Dwnld_Context_t gpJcopOs_Dwnld_Context;
/*******************************************************************************
**
** Function:        JCDNLD_Init
**
** Description:     Initializes the JCOP library and opens the DWP communication channel
**
** Returns:         TRUE if ok.
**
*******************************************************************************/
tJBL_STATUS JCDNLD_Init(IChannel_t *channel)
{
    static const char fn[] = "JCDNLD_Init";
    BOOLEAN stat = FALSE;
    jcHandle = EE_ERROR_OPEN_FAIL;
    ALOGD("%s: enter", fn);

    if (inUse == true)
    {
        return STATUS_INUSE;
    }
    else if(channel == NULL)
    {
        return STATUS_FAILED;
    }
    /*TODO: inUse assignment should be with protection like using semaphore*/
    inUse = true;
    jd = JcopOsDwnld::getInstance();
    stat = jd->initialize (channel);
    if(stat != TRUE)
    {
        ALOGE("%s: failed", fn);
    }
    else
    {
        if((channel != NULL) &&
           (channel->open) != NULL)
        {
            jcHandle = channel->open();
            if(jcHandle == EE_ERROR_OPEN_FAIL)
            {
                ALOGE("%s:Open DWP communication is failed", fn);
                stat = FALSE;
            }
            else
            {
                ALOGE("%s:Open DWP communication is success", fn);
                stat = TRUE;
            }
        }
        else
        {
            ALOGE("%s: NULL DWP channel", fn);
            stat = FALSE;
        }
    }
    return (stat == true)?STATUS_OK:STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        JCDNLD_StartDownload
**
** Description:     Starts the JCOP update
**
** Returns:         SUCCESS if ok.
**
*******************************************************************************/
tJBL_STATUS JCDNLD_StartDownload()
{
    static const char fn[] = "JCDNLD_StartDownload";
    tJBL_STATUS status = STATUS_FAILED;
    BOOLEAN stat = FALSE;

    status = jd->JcopOs_Download();
    ALOGE("%s: Exit; status=0x0%X", fn, status);
    return status;
}

/*******************************************************************************
**
** Function:        JCDNLD_DeInit
**
** Description:     Deinitializes the JCOP Lib
**
** Returns:         TRUE if ok.
**
*******************************************************************************/
bool JCDNLD_DeInit()
{
    static const char fn[] = "JCDNLD_DeInit";
    BOOLEAN stat = FALSE;
    channel = gpJcopOs_Dwnld_Context->channel;
    ALOGD("%s: enter", fn);
    if(channel != NULL)
    {
        if(channel->doeSE_Reset != NULL)
        {
            channel->doeSE_Reset();
            if(channel->close != NULL)
            {
                stat = channel->close(jcHandle);
                if(stat != TRUE)
                {
                    ALOGE("%s:closing DWP channel is failed", fn);
                }
            }
            else
            {
                ALOGE("%s: NULL fp DWP_close", fn);
                stat = FALSE;
            }
        }
    }
    else
    {
        ALOGE("%s: NULL dwp channel", fn);
    }
    jd->finalize();
    /*TODO: inUse assignment should be with protection like using semaphore*/
    inUse = false;
    return stat;
}

/*******************************************************************************
**
** Function:        JCDNLD_CheckVersion
**
** Description:     Check the existing JCOP OS version
**
** Returns:         TRUE if ok.
**
*******************************************************************************/
bool JCDNLD_CheckVersion()
{

    /**
     * Need to implement in case required
     * */
    return TRUE;
}
