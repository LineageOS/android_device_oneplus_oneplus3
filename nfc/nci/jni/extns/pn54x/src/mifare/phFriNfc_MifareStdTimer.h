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
#include <time.h>

typedef void (*TIMER_FUNC) (union sigval);

typedef struct phFriNfc_MifareStdTimer
{
    timer_t mTimerId; // timer id which will be assigned by create timer
    TIMER_FUNC mCb;   //callback function for timeout
    uint32_t mtimeout; // timeout value in ms.
}phFriNfc_MifareStdTimer_t;

NFCSTATUS  phFriNfc_MifareStd_StartTimer( phFriNfc_MifareStdTimer_t *timer );

NFCSTATUS  phFriNfc_MifareStd_StopTimer( phFriNfc_MifareStdTimer_t *timer );

STATIC NFCSTATUS  phFriNfc_MifareStd_CreateTimer( phFriNfc_MifareStdTimer_t *TimerInfo );
