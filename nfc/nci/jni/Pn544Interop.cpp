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

/*****************************************************************************
**
**  Description:    Implement operations that provide compatibility with NXP
**                  PN544 controller.  Specifically facilitate peer-to-peer
**                  operations with PN544 controller.
**
*****************************************************************************/
#include "OverrideLog.h"
#include "Pn544Interop.h"
#include "IntervalTimer.h"
#include "Mutex.h"
#include "NfcTag.h"
namespace android
{
    extern void startStopPolling (bool isStartPolling);
    extern void releaseRfInterfaceMutexLock();
    extern void acquireRfInterfaceMutexLock();
}


/*****************************************************************************
**
** private variables and functions
**
*****************************************************************************/


static const int gIntervalTime = 1000; //millisecond between the check to restore polling
static IntervalTimer gTimer;
static Mutex gMutex;
static void pn544InteropStartPolling (union sigval); //callback function for interval timer
static bool gIsBusy = false; //is timer busy?
static bool gAbortNow = false; //stop timer during next callback


/*******************************************************************************
**
** Function:        pn544InteropStopPolling
**
** Description:     Stop polling to let NXP PN544 controller poll.
**                  PN544 should activate in P2P mode.
**
** Returns:         None
**
*******************************************************************************/
void pn544InteropStopPolling ()
{
    ALOGD ("%s: enter", __FUNCTION__);
    gMutex.lock ();
    gTimer.kill ();
    android::startStopPolling (false);
    gIsBusy = true;
    gAbortNow = false;
    gTimer.set (gIntervalTime, pn544InteropStartPolling); //after some time, start polling again
    gMutex.unlock ();
    ALOGD ("%s: exit", __FUNCTION__);
}


/*******************************************************************************
**
** Function:        pn544InteropStartPolling
**
** Description:     Start polling when activation state is idle.
**                  sigval: Unused.
**
** Returns:         None
**
*******************************************************************************/
void pn544InteropStartPolling (union sigval)
{
    ALOGD ("%s: enter", __FUNCTION__);
    android::acquireRfInterfaceMutexLock();
    gMutex.lock ();
    NfcTag::ActivationState state = NfcTag::getInstance ().getActivationState ();

    if (gAbortNow)
    {
        ALOGD ("%s: abort now", __FUNCTION__);
        gIsBusy = false;
        goto TheEnd;
    }

    if (state == NfcTag::Idle)
    {
        ALOGD ("%s: start polling", __FUNCTION__);
        android::startStopPolling (true);
        gIsBusy = false;
    }
    else
    {
        ALOGD ("%s: try again later", __FUNCTION__);
        gTimer.set (gIntervalTime, pn544InteropStartPolling); //after some time, start polling again
    }

TheEnd:
    gMutex.unlock ();
    android::releaseRfInterfaceMutexLock();
    ALOGD ("%s: exit", __FUNCTION__);
}


/*******************************************************************************
**
** Function:        pn544InteropIsBusy
**
** Description:     Is the code performing operations?
**
** Returns:         True if the code is busy.
**
*******************************************************************************/
bool pn544InteropIsBusy ()
{
    bool isBusy = false;
    gMutex.lock ();
    isBusy = gIsBusy;
    gMutex.unlock ();
    ALOGD ("%s: %u", __FUNCTION__, isBusy);
    return isBusy;
}


/*******************************************************************************
**
** Function:        pn544InteropAbortNow
**
** Description:     Request to abort all operations.
**
** Returns:         None.
**
*******************************************************************************/
void pn544InteropAbortNow ()
{
    ALOGD ("%s", __FUNCTION__);
    gMutex.lock ();
    gAbortNow = true;
    gMutex.unlock ();
}
