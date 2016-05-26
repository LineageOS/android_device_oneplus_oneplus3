/*
 * Copyright (C) 2010-2014 NXP Semiconductors
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

 /*include files*/
#include <phNfcTypes.h>
#include <phNfcStatus.h>
#include <phNciNfcTypes.h>
#include <phFriNfc_MifareStdTimer.h>
#include <phNxpLog.h>
/*******************************************************************************
**
** Function        phFriNfc_MifareStd_StartTimer
**
** Description     Start timer for Mifare related events
**                 TimerInfo structure contains timer id ,
**                 timeout value and callback function.
**                 Timerid is initially zero for first time.
**                 It will be set to proper value inside this function.
** Returns:        NFCSTATUS_SUCCESS  -  timer started successfully
**                 NFCSTATUS_FAILED   -  otherwise
*******************************************************************************/
NFCSTATUS  phFriNfc_MifareStd_StartTimer( phFriNfc_MifareStdTimer_t *TimerInfo )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    int stat = 0;
    struct itimerspec ts;
    if ( TimerInfo->mTimerId == 0)
    {
        if (TimerInfo->mCb == 0)
        {
            return NFCSTATUS_FAILED;
        }

        if (phFriNfc_MifareStd_CreateTimer(TimerInfo) != NFCSTATUS_SUCCESS)
            return NFCSTATUS_FAILED;
    }

    ts.it_value.tv_sec = (TimerInfo->mtimeout) / 1000;
    ts.it_value.tv_nsec = (TimerInfo->mtimeout % 1000) * 1000000;

    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;

    stat = timer_settime(TimerInfo->mTimerId, 0, &ts, 0);
    if (stat == 0)
    {

        status = NFCSTATUS_SUCCESS;
        return status;
    }else
    {
        status= NFCSTATUS_FAILED;
        return status;
    }
}


/*******************************************************************************
**
** Function         phFriNfc_MifareStd_StopTimer
**
** Description      Stop Timer if Mifare related events received
**
**
** Returns:         NFCSTATUS_SUCCESS  -  timer stopped successfully
**                  NFCSTATUS_FAILED   -  otherwise
*******************************************************************************/
NFCSTATUS  phFriNfc_MifareStd_StopTimer( phFriNfc_MifareStdTimer_t *TimerInfo )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    if (TimerInfo->mTimerId == 0)
    {
        NXPLOG_EXTNS_E(" phFriNfc_MifareStd_CreateTimer() failed to stop timer  ");
        status = NFCSTATUS_FAILED;
        return status;
    }

    timer_delete(TimerInfo->mTimerId);
    TimerInfo->mTimerId = 0;
    TimerInfo->mCb = NULL;
    return status;
}


/*******************************************************************************
**
** Function         phFriNfc_MifareStd_CreateTimer
**
** Description      Create  a mifare timer
**
**
** Returns:         NFCSTATUS_SUCCESS  -  timer created successfully
**                  NFCSTATUS_FAILED   -  otherwise
*******************************************************************************/
STATIC NFCSTATUS  phFriNfc_MifareStd_CreateTimer( phFriNfc_MifareStdTimer_t *TimerInfo )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    struct sigevent se;
    memset(&se,0,sizeof(struct sigevent));
    int stat = 0;
    /*
     * Set the sigevent structure to cause the signal to be
     * delivered by creating a new thread.
     */
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_ptr = &(TimerInfo)->mTimerId;
    se.sigev_notify_function = TimerInfo->mCb;
    se.sigev_notify_attributes = NULL;
    stat = timer_create(CLOCK_MONOTONIC, &se, &(TimerInfo)->mTimerId);
    if (stat == 0)
    {
        NXPLOG_EXTNS_E(" phFriNfc_MifareStd_CreateTimer() Timer created successfully ");
        return status;
    }else
    {
        status= NFCSTATUS_FAILED;
        return status;
    }
}
