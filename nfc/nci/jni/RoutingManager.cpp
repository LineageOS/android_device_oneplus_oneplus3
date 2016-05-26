/*
 * Copyright (C) 2013 The Android Open Source Project
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
/*
 *  Manage the listen-mode routing table.
 */

#include <cutils/log.h>
#include <ScopedLocalRef.h>
#include <JNIHelp.h>
#include "config.h"
#include "JavaClassConstants.h"
#include "RoutingManager.h"
#include "SecureElement.h"
#if(NXP_EXTNS == TRUE)
extern "C"{
#include "phNxpConfig.h"
#include "nfc_api.h"
#include "nfa_api.h"
}

extern INT32 gSeDiscoverycount;
extern SyncEvent gNfceeDiscCbEvent;
extern INT32 gActualSeCount;

int gUICCVirtualWiredProtectMask = 0;
int gEseVirtualWiredProtectMask = 0;
int gWiredModeRfFieldEnable = 0;
#endif
extern bool sHCEEnabled;

const JNINativeMethod RoutingManager::sMethods [] =
{
    {"doGetDefaultRouteDestination", "()I", (void*) RoutingManager::com_android_nfc_cardemulation_doGetDefaultRouteDestination},
    {"doGetDefaultOffHostRouteDestination", "()I", (void*) RoutingManager::com_android_nfc_cardemulation_doGetDefaultOffHostRouteDestination},
    {"doGetAidMatchingMode", "()I", (void*) RoutingManager::com_android_nfc_cardemulation_doGetAidMatchingMode},
    {"doGetAidMatchingPlatform", "()I", (void*) RoutingManager::com_android_nfc_cardemulation_doGetAidMatchingPlatform}
};

static UINT16 rdr_req_handling_timeout = 50;

Rdr_req_ntf_info_t swp_rdr_req_ntf_info;
#if(NFC_NXP_ESE == TRUE && NFC_NXP_CHIP_TYPE == PN548C2)
static IntervalTimer swp_rd_req_timer;
#endif
static const int MAX_NUM_EE = 5;
UINT16 lastcehandle = 0;
NfcID2_add_req_info_t NfcID2_add_req;//FelicaOnHost
NfcID2_rmv_req_info_t NfcId2_rmv_req;
namespace android
{
    extern  void  checkforTranscation(UINT8 connEvent, void* eventData );
#if (NXP_EXTNS == TRUE)
    extern UINT16 sRoutingBuffLen;
    extern bool isNfcInitializationDone();
    extern void startRfDiscovery (bool isStart);
    extern bool isDiscoveryStarted();
    extern int getScreenState();
#endif
}

#define NFCID2_ADD_REQ_TIMEOUT 100
#define NFCID2_RMV_REQ_TIMEOUT 100
#define NFCID2_COUNT_MAX 4
#define NFCID2_LEN_MAX 8

RoutingManager::RoutingManager ()
: mNativeData(NULL),
  mDefaultEe (NFA_HANDLE_INVALID),
  mHostListnEnable (true),
  mFwdFuntnEnable (true),
  mAddAid(1)
{
    static const char fn [] = "RoutingManager::RoutingManager()";
    unsigned long num = 0;

    // Get the active SE
    if (GetNumValue("ACTIVE_SE", &num, sizeof(num)))
        mActiveSe = num;
    else
        mActiveSe = 0x00;

    // Get the "default" route
    if (GetNumValue("DEFAULT_ISODEP_ROUTE", &num, sizeof(num)))
        mDefaultEe = num;
    else
        mDefaultEe = 0x00;
    ALOGD("%s: default route is 0x%02X", fn, mDefaultEe);
    // Get the default "off-host" route.  This is hard-coded at the Java layer
    // but we can override it here to avoid forcing Java changes.
    if (GetNumValue("DEFAULT_OFFHOST_ROUTE", &num, sizeof(num)))
        mOffHostEe = num;
    else
        mOffHostEe = 0x02;
    if (GetNumValue("AID_MATCHING_MODE", &num, sizeof(num)))
        mAidMatchingMode = num;
    else
        mAidMatchingMode = AID_MATCHING_EXACT_ONLY;

    if (GetNxpNumValue("AID_MATCHING_PLATFORM", &num, sizeof(num)))
        mAidMatchingPlatform = num;
    else
        mAidMatchingPlatform = AID_MATCHING_L;
    ALOGD("%s: mOffHostEe=0x%02X mAidMatchingMode=0x%2X", fn, mOffHostEe, mAidMatchingMode);
    mSeTechMask = 0x00;
}


void *nfcID2_req_handler_async(void *arg);
void NfcID2_req_timoutHandler (union sigval);
void NfcID2_rmv_timoutHandler (union sigval);

int RoutingManager::mChipId = 0;
#if (NXP_EXTNS == TRUE)
bool recovery;
#endif

#if(NFC_NXP_ESE == TRUE && NFC_NXP_CHIP_TYPE == PN548C2)
void reader_req_event_ntf (union sigval);
#endif
RoutingManager::~RoutingManager ()
{
    NFA_EeDeregister (nfaEeCallback);
}

bool RoutingManager::initialize (nfc_jni_native_data* native)
{
    static const char fn [] = "RoutingManager::initialize()";
    unsigned long num = 0, tech = 0;
    mNativeData = native;
    UINT8 mActualNumEe = SecureElement::MAX_NUM_EE;
    tNFA_EE_INFO mEeInfo [mActualNumEe];

#if (NXP_EXTNS == TRUE)
    if ((GetNumValue(NAME_HOST_LISTEN_ENABLE, &tech, sizeof(tech))))
    {
        mHostListnEnable = tech;
        ALOGE ("%s:HOST_LISTEN_ENABLE=%d;", __FUNCTION__, mHostListnEnable);
    }

    if ((GetNumValue(NAME_NXP_FWD_FUNCTIONALITY_ENABLE, &tech, sizeof(tech))))
    {
        mFwdFuntnEnable = tech;
        ALOGE ("%s:NXP_FWD_FUNCTIONALITY_ENABLE=%d;", __FUNCTION__, mFwdFuntnEnable);
    }

    if (GetNxpNumValue (NAME_NXP_DEFAULT_SE, (void*)&num, sizeof(num)))
    {
        ALOGD ("%d: nfcManager_GetDefaultSE", num);
        mDefaultEe = num;
    }
    if (GetNxpNumValue (NAME_NXP_ENABLE_ADD_AID, (void*)&num, sizeof(num)))
    {
        ALOGD ("%d: mAddAid", num);
        mAddAid = num;
    }
#if (NFC_NXP_ESE == TRUE && NFC_NXP_CHIP_TYPE == PN548C2)
    if (GetNxpNumValue (NAME_NXP_ESE_WIRED_PRT_MASK, (void*)&num, sizeof(num)))
    {
        ALOGD (" NAME_NXP_ESE_WIRED_PRT_MASK :%d", num);
        gUICCVirtualWiredProtectMask = num;
    }
    if (GetNxpNumValue (NAME_NXP_UICC_WIRED_PRT_MASK, (void*)&num, sizeof(num)))
    {
        ALOGD ("%d: NAME_NXP_UICC_WIRED_PRT_MASK", num);
        gEseVirtualWiredProtectMask = num;
    }
    if (GetNxpNumValue (NAME_NXP_WIRED_MODE_RF_FIELD_ENABLE, (void*)&num, sizeof(num)))
    {
        ALOGD ("%d: NAME_NXP_WIRED_MODE_RF_FIELD_ENABLE", num);
        gWiredModeRfFieldEnable = num;
    }
#endif
#endif
    if ((GetNxpNumValue(NAME_NXP_NFC_CHIP, &num, sizeof(num))))
    {
        ALOGD ("%s:NXP_NFC_CHIP=0x0%lu;", __FUNCTION__, num);
        mChipId = num;
    }

    tNFA_STATUS nfaStat;
    {
        SyncEventGuard guard (mEeRegisterEvent);
        ALOGD ("%s: try ee register", fn);
        nfaStat = NFA_EeRegister (nfaEeCallback);
        if (nfaStat != NFA_STATUS_OK)
        {
            ALOGE ("%s: fail ee register; error=0x%X", fn, nfaStat);
            return false;
        }
        mEeRegisterEvent.wait ();
    }

#if(NXP_EXTNS == TRUE)
    if(mHostListnEnable)
    {
        // Tell the host-routing to only listen on Nfc-A/Nfc-B
        nfaStat = NFA_CeRegisterAidOnDH (NULL, 0, stackCallback);
        if (nfaStat != NFA_STATUS_OK)
            ALOGE ("Failed to register wildcard AID for DH");
        // Tell the host-routing to only listen on Nfc-A/Nfc-B
        nfaStat = NFA_CeSetIsoDepListenTech(NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B);
        if (nfaStat != NFA_STATUS_OK)
            ALOGE ("Failed to configure CE IsoDep technologies");
        //setRouting(true);
    }
    mRxDataBuffer.clear ();
#else
//    setDefaultRouting();
#endif

    if ((nfaStat = NFA_AllEeGetInfo (&mActualNumEe, mEeInfo)) != NFA_STATUS_OK)
    {
        ALOGE ("%s: fail get info; error=0x%X", fn, nfaStat);
        mActualNumEe = 0;
    }
    else
    {
        gSeDiscoverycount = mActualNumEe;
#if 0
        if(mChipId == 0x02 || mChipId == 0x04)
        {
            for(int xx = 0; xx <  mActualNumEe; xx++)
            {
                ALOGE("xx=%d, ee_handle=0x0%x, status=0x0%x", xx, mEeInfo[xx].ee_handle,mEeInfo[xx].ee_status);
                if ((mEeInfo[xx].ee_handle == 0x4C0) &&
                        (mEeInfo[xx].ee_status == 0x02))
                {
                    ee_removed_disc_ntf_handler(mEeInfo[xx].ee_handle, mEeInfo[xx].ee_status);
                    break;
                }
            }
        }
#endif
    }

    ALOGD("gSeDiscoverycount = %d", gSeDiscoverycount);
#if(NFC_NXP_ESE == TRUE && NFC_NXP_CHIP_TYPE == PN548C2)
    swp_rdr_req_ntf_info.mMutex.lock();
    memset(&(swp_rdr_req_ntf_info.swp_rd_req_info),0x00,sizeof(rd_swp_req_t));
    memset(&(swp_rdr_req_ntf_info.swp_rd_req_current_info),0x00,sizeof(rd_swp_req_t));
    swp_rdr_req_ntf_info.swp_rd_req_current_info.src = NFA_HANDLE_INVALID;
    swp_rdr_req_ntf_info.swp_rd_req_info.src = NFA_HANDLE_INVALID;
    swp_rdr_req_ntf_info.swp_rd_state = STATE_SE_RDR_MODE_STOPPED;
    swp_rdr_req_ntf_info.mMutex.unlock();
#endif

    memset(&NfcID2_add_req,0x00,sizeof(NfcID2_add_req));
    memset(&NfcId2_rmv_req,0x00,sizeof(NfcId2_rmv_req));
    return true;
}

RoutingManager& RoutingManager::getInstance ()
{
    static RoutingManager manager;
    return manager;
}

void RoutingManager::cleanRouting()
{
    tNFA_STATUS nfaStat;
    //tNFA_HANDLE seHandle = NFA_HANDLE_INVALID;        /*commented to eliminate unused variable warning*/
    tNFA_HANDLE ee_handleList[SecureElement::MAX_NUM_EE];
    UINT8 i, count;
   // static const char fn [] = "SecureElement::cleanRouting";   /*commented to eliminate unused variable warning*/

    SecureElement::getInstance().getEeHandleList(ee_handleList, &count);
    if (count > SecureElement::MAX_NUM_EE) {
        count = SecureElement::MAX_NUM_EE;
        ALOGD("Count is more than SecureElement::MAX_NUM_EE,Forcing to SecureElement::MAX_NUM_EE");
    }
    for ( i = 0; i < count; i++)
    {
#if(NXP_EXTNS == TRUE)
        nfaStat =  NFA_EeSetDefaultTechRouting(ee_handleList[i],0,0,0,0,0);
#else
        nfaStat =  NFA_EeSetDefaultTechRouting(ee_handleList[i],0,0,0);
#endif
        if(nfaStat == NFA_STATUS_OK)
        {
            mRoutingEvent.wait ();
        }
#if(NXP_EXTNS == TRUE)
        nfaStat =  NFA_EeSetDefaultProtoRouting(ee_handleList[i],0,0,0,0,0);
#else
        nfaStat =  NFA_EeSetDefaultProtoRouting(ee_handleList[i],0,0,0);
#endif
        if(nfaStat == NFA_STATUS_OK)
        {
            mRoutingEvent.wait ();
        }
    }
    //clean HOST
#if(NXP_EXTNS == TRUE)
    nfaStat =  NFA_EeSetDefaultTechRouting(NFA_EE_HANDLE_DH,0,0,0,0,0);
#else
    nfaStat =  NFA_EeSetDefaultTechRouting(NFA_EE_HANDLE_DH,0,0,0);
#endif
    if(nfaStat == NFA_STATUS_OK)
    {
        mRoutingEvent.wait ();
    }
#if(NXP_EXTNS == TRUE)
    nfaStat =  NFA_EeSetDefaultProtoRouting(NFA_EE_HANDLE_DH,0,0,0,0,0);
#else
    nfaStat =  NFA_EeSetDefaultProtoRouting(NFA_EE_HANDLE_DH,0,0,0);
#endif
    if(nfaStat == NFA_STATUS_OK)
    {
        mRoutingEvent.wait ();
    }
#if 0
    /*commented to avoid send LMRT command twice*/
    nfaStat = NFA_EeUpdateNow();
    if (nfaStat != NFA_STATUS_OK)
        ALOGE("Failed to commit routing configuration");
#endif
}

#if(NXP_EXTNS == TRUE)
void RoutingManager::setRouting(bool isHCEEnabled)
{
    tNFA_STATUS nfaStat;
    tNFA_HANDLE defaultHandle = NFA_HANDLE_INVALID;
    tNFA_HANDLE ee_handleList[SecureElement::MAX_NUM_EE];
    UINT8 i, count;
    static const char fn [] = "SecureElement::setRouting";
    unsigned long num = 0;

    if ((GetNumValue(NAME_UICC_LISTEN_TECH_MASK, &num, sizeof(num))))
    {
        ALOGE ("%s:UICC_LISTEN_MASK=0x0%lu;", __FUNCTION__, num);
    }

    if (isHCEEnabled)
    {
        defaultHandle = NFA_EE_HANDLE_DH;
    }
    else
    {
        SecureElement::getInstance().getEeHandleList(ee_handleList, &count);
        for ( i = 0; i < count; i++)
        {
            if (defaultHandle == NFA_HANDLE_INVALID)
            {
                defaultHandle = ee_handleList[i];
                break;
            }
        }
    }
    ALOGD ("%s: defaultHandle %u = 0x%X", fn, i, defaultHandle);

    if (defaultHandle != NFA_HANDLE_INVALID)
    {
        {
            SyncEventGuard guard (mRoutingEvent);

            tNFA_STATUS status =  NFA_EeSetDefaultTechRouting(0x402,0,0,0,0,0); //UICC clear

            if(status == NFA_STATUS_OK)
            {
                mRoutingEvent.wait ();
            }
        }

        {
            SyncEventGuard guard (mRoutingEvent);

            tNFA_STATUS status =  NFA_EeSetDefaultProtoRouting(0x402,0,0,0,0,0); //UICC clear

            if(status == NFA_STATUS_OK)
            {
                mRoutingEvent.wait ();
            }
        }

        {
            SyncEventGuard guard (mRoutingEvent);

            tNFA_STATUS status =  NFA_EeSetDefaultTechRouting(0x4C0,0,0,0,0,0); //SMX clear

            if(status == NFA_STATUS_OK)
            {
                mRoutingEvent.wait ();
            }
        }

        {
            SyncEventGuard guard (mRoutingEvent);

            tNFA_STATUS status =  NFA_EeSetDefaultProtoRouting(0x4C0,0,0,0,0,0); //SMX clear

            if(status == NFA_STATUS_OK)
            {
                mRoutingEvent.wait ();
            }
        }

        {
            SyncEventGuard guard (mRoutingEvent);

            tNFA_STATUS status =  NFA_EeSetDefaultTechRouting(0x400,0,0,0,0,0); //HOST clear

            if(status == NFA_STATUS_OK)
            {
                mRoutingEvent.wait ();
            }
        }

        {
            SyncEventGuard guard (mRoutingEvent);

            tNFA_STATUS status =  NFA_EeSetDefaultProtoRouting(0x400,0,0,0,0,0); //HOST clear

            if(status == NFA_STATUS_OK)
            {
                mRoutingEvent.wait ();
            }
        }
        if(defaultHandle == NFA_EE_HANDLE_DH)
        {
            SyncEventGuard guard (mRoutingEvent);
            // Default routing for NFC-A technology
            if(mCeRouteStrictDisable == 0x01)
            {
                nfaStat = NFA_EeSetDefaultTechRouting (defaultHandle, 0x01, 0, 0, 0x01, 0);
            }else
            {
                nfaStat = NFA_EeSetDefaultTechRouting (defaultHandle, 0x01, 0, 0, 0, 0);
            }

            if (nfaStat == NFA_STATUS_OK)
                mRoutingEvent.wait ();
            else
                ALOGE ("Fail to set default tech routing");
        }
        else
        {
            SyncEventGuard guard (mRoutingEvent);
            // Default routing for NFC-A technology
            if(mCeRouteStrictDisable == 0x01)
            {
                nfaStat = NFA_EeSetDefaultTechRouting (defaultHandle, num, num, num, num, num);
            }else{
                nfaStat = NFA_EeSetDefaultTechRouting (defaultHandle, num, num, num, 0, 0);
            }
            if (nfaStat == NFA_STATUS_OK)
                mRoutingEvent.wait ();
            else
                ALOGE ("Fail to set default tech routing");
        }

        if(defaultHandle == NFA_EE_HANDLE_DH)
        {
            SyncEventGuard guard (mRoutingEvent);
            // Default routing for IsoDep protocol
            if(mCeRouteStrictDisable == 0x01)
            {
                nfaStat = NFA_EeSetDefaultProtoRouting(defaultHandle, NFA_PROTOCOL_MASK_ISO_DEP, 0, 0, NFA_PROTOCOL_MASK_ISO_DEP, 0);
            }
            else
            {
                nfaStat = NFA_EeSetDefaultProtoRouting(defaultHandle, NFA_PROTOCOL_MASK_ISO_DEP, 0, 0, 0 ,0);
            }
            if (nfaStat == NFA_STATUS_OK)
                mRoutingEvent.wait ();
            else
                ALOGE ("Fail to set default proto routing");
        }
        else
        {
            SyncEventGuard guard (mRoutingEvent);
            // Default routing for IsoDep protocol
            if(mCeRouteStrictDisable == 0x01)
            {
                nfaStat = NFA_EeSetDefaultProtoRouting(defaultHandle,
                                                       NFA_PROTOCOL_MASK_ISO_DEP,
                                                       NFA_PROTOCOL_MASK_ISO_DEP,
                                                       NFA_PROTOCOL_MASK_ISO_DEP,
                                                       NFA_PROTOCOL_MASK_ISO_DEP,
                                                       NFA_PROTOCOL_MASK_ISO_DEP);
            }
            else
            {
                nfaStat = NFA_EeSetDefaultProtoRouting(defaultHandle,
                                                       NFA_PROTOCOL_MASK_ISO_DEP,
                                                       NFA_PROTOCOL_MASK_ISO_DEP,
                                                       NFA_PROTOCOL_MASK_ISO_DEP,
                                                       0,
                                                       0);
            }
            if (nfaStat == NFA_STATUS_OK)
                mRoutingEvent.wait ();
            else
                ALOGE ("Fail to set default proto routing");
        }

        if(defaultHandle != NFA_EE_HANDLE_DH)
        {
            {
                SyncEventGuard guard (SecureElement::getInstance().mUiccListenEvent);
                nfaStat = NFA_CeConfigureUiccListenTech (defaultHandle, 0x00);
                if (nfaStat == NFA_STATUS_OK)
                {
                    SecureElement::getInstance().mUiccListenEvent.wait ();
                }
                else
                    ALOGE ("fail to start UICC listen");
            }

            {
                SyncEventGuard guard (SecureElement::getInstance().mUiccListenEvent);
                nfaStat = NFA_CeConfigureUiccListenTech (defaultHandle, (num & 0x07));
                if(nfaStat == NFA_STATUS_OK)
                {
                    SecureElement::getInstance().mUiccListenEvent.wait ();
                }
                else
                    ALOGE ("fail to start UICC listen");
            }
        }
    }

    // Commit the routing configuration
    nfaStat = NFA_EeUpdateNow();
    if (nfaStat != NFA_STATUS_OK)
        ALOGE("Failed to commit routing configuration");
}

bool RoutingManager::setDefaultRoute(const UINT8 defaultRoute, const UINT8 protoRoute, const UINT8 techRoute)
{

    tNFA_STATUS nfaStat;
    static const char fn [] = "RoutingManager::setDefaultRoute";   /*commented to eliminate unused variable warning*/
    unsigned long uiccListenTech = 0,check_default_proto_se_id_req = 0;
    tNFA_HANDLE defaultHandle = NFA_HANDLE_INVALID;
    tNFA_HANDLE ActDevHandle = NFA_HANDLE_INVALID;
    tNFA_HANDLE preferred_defaultHandle = 0x402;
    UINT8 isDefaultProtoSeIDPresent = 0;
    SyncEventGuard guard (mRoutingEvent);
    if (mDefaultEe == SecureElement::ESE_ID) //eSE
    {
        preferred_defaultHandle = 0x4C0;
    }
    else if (mDefaultEe == SecureElement::UICC_ID) //UICC
    {
        preferred_defaultHandle = 0x402;
    }
    // Host
       uiccListenTech = NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B;

       ALOGD ("%s: enter, defaultRoute:%x protoRoute:0x%x TechRoute:0x%x ", fn, defaultRoute, protoRoute, techRoute);
       defaultSeID = (((defaultRoute & 0x60) >> 5) == 0x00)  ? 0x400 :  ((((defaultRoute & 0x60)>>5 )== 0x01 ) ? 0x4C0 : 0x402);
       defaultPowerstate=defaultRoute & 0x1F;

       ALOGD ("%s: enter, defaultSeID:%x defaultPowerstate:0x%x", fn, defaultSeID,defaultPowerstate);
       defaultProtoSeID = (((protoRoute & 0x60) >> 5) == 0x00)  ? 0x400 :  ((((protoRoute & 0x60)>>5 )== 0x01 ) ? 0x4C0 : 0x402);
       defaultProtoPowerstate = protoRoute & 0x1F;

       ALOGD ("%s: enter, defaultProtoSeID:%x defaultProtoPowerstate:0x%x", fn, defaultProtoSeID,defaultProtoPowerstate);

       defaultTechSeID = (((techRoute & 0x60) >> 5) == 0x00)  ? 0x400 :  ((((techRoute & 0x60)>>5 )== 0x01 ) ? 0x4C0 : 0x402);
       defaultTechAPowerstate = techRoute & 0x1F;
       DefaultTechType = (techRoute & 0x80) >> 7;

       ALOGD ("%s: enter, defaultTechSeID:%x defaultTechAPowerstate:0x%x,defaultTechType:0x%x", fn, defaultTechSeID,defaultTechAPowerstate,DefaultTechType);

       cleanRouting();
       if(mHostListnEnable)
       {
           nfaStat = NFA_CeRegisterAidOnDH (NULL, 0, stackCallback);
           if (nfaStat != NFA_STATUS_OK)
               ALOGE("Failed to register wildcard AID for DH");
       }

       if (GetNxpNumValue(NAME_CHECK_DEFAULT_PROTO_SE_ID, &check_default_proto_se_id_req, sizeof(check_default_proto_se_id_req)))
       {
           ALOGE("%s : CHECK_DEFAULT_PROTO_SE_ID - 0x%02x ",fn,check_default_proto_se_id_req);
       }
       else
       {
           ALOGE("%s : CHECK_DEFAULT_PROTO_SE_ID not defined. Taking default value - 0x%02x ",fn,check_default_proto_se_id_req);
       }
       if(check_default_proto_se_id_req == 0x01)
       {
           UINT8 count,seId=0;
           tNFA_HANDLE ee_handleList[SecureElement::MAX_NUM_EE];
           SecureElement::getInstance().getEeHandleList(ee_handleList, &count);

           for (int  i = 0; ((count != 0 ) && (i < count)); i++)
           {
               seId = SecureElement::getInstance().getGenericEseId(ee_handleList[i]);
               defaultHandle = SecureElement::getInstance().getEseHandleFromGenericId(seId);
               ALOGD ("%s: enter, ee_handleList[%d]:%x", fn, i,ee_handleList[i]);
               //defaultHandle = ee_handleList[i];
               if (preferred_defaultHandle == defaultHandle)
               {
                   //ActSEhandle = defaultHandle;
                   break;
               }
           }
           for (int  i = 0; ((count != 0 ) && (i < count)); i++)
           {
               seId = SecureElement::getInstance().getGenericEseId(ee_handleList[i]);
               ActDevHandle = SecureElement::getInstance().getEseHandleFromGenericId(seId);
               ALOGD ("%s: enter, ee_handleList[%d]:%x", fn, i,ee_handleList[i]);
               if (defaultProtoSeID == ActDevHandle)
               {
                   isDefaultProtoSeIDPresent =1;
                   break;
               }
           }


           if(!isDefaultProtoSeIDPresent)
           {
               defaultProtoSeID = 0x400;
               defaultProtoPowerstate = 0x01;
           }
           ALOGD ("%s: enter, isDefaultProtoSeIDPresent:%x", fn, isDefaultProtoSeIDPresent);
       }

       if( defaultProtoSeID == defaultSeID)
       {
             unsigned int default_proto_power_mask[5] = {0,};
             for(int pCount=0 ; pCount< 5 ;pCount++)
             {
                  if((defaultPowerstate >> pCount)&0x01)
                  {
                       default_proto_power_mask[pCount] |= NFC_PROTOCOL_MASK_ISO7816;
                  }
                  if((defaultProtoPowerstate >> pCount)&0x01)
                  {
                       default_proto_power_mask[pCount] |= NFA_PROTOCOL_MASK_ISO_DEP;
                  }
             }
             if(defaultProtoSeID == 0x400 && mHostListnEnable == FALSE)
             {
                 ALOGE("%s, HOST is disabled hence skipping configure proto route to host", fn);
             }
             else
             {
                 if(mCeRouteStrictDisable == 0x01)
                 {

#if(NFC_NXP_CHIP_TYPE != PN547C2)
                     if(defaultProtoSeID == 0x400)
                     {
                         default_proto_power_mask[0] |= NFA_PROTOCOL_MASK_T3T;
                     }
#endif
                     nfaStat = NFA_EeSetDefaultProtoRouting(defaultProtoSeID ,
                                                            default_proto_power_mask[0],
                                                            default_proto_power_mask[1],
                                                            default_proto_power_mask[2],
                                                            default_proto_power_mask[3],
                                                            default_proto_power_mask[4]);
                 }
                 else
                 {
                     nfaStat = NFA_EeSetDefaultProtoRouting(defaultProtoSeID ,
                                                           default_proto_power_mask[0],
                                                           default_proto_power_mask[1],
                                                           default_proto_power_mask[2],
                                                           0, 0);
                 }
                 if (nfaStat == NFA_STATUS_OK)
                      mRoutingEvent.wait ();
                 else
                 {
                      ALOGE ("Fail to set  iso7816 routing");
                 }
             }
       }
       else
       {
              int t3t_protocol_mask = 0;
              ALOGD ("%s: enter, defaultPowerstate:%x", fn, defaultPowerstate);
              if(mHostListnEnable == FALSE && defaultSeID == 0x400)
              {
                  ALOGE("%s, HOST is disabled hence skipping configure 7816 route to host", fn);
              }
              else
              {
                  if(mCeRouteStrictDisable == 0x01)
                  {
#if(NFC_NXP_CHIP_TYPE != PN547C2)
                      if(defaultSeID == 0x400)
                      {
                          t3t_protocol_mask = NFA_PROTOCOL_MASK_T3T;
                      }
#endif
                      nfaStat = NFA_EeSetDefaultProtoRouting(defaultSeID ,
                                                            (defaultPowerstate & 01) ? (NFC_PROTOCOL_MASK_ISO7816 | t3t_protocol_mask) :0,
                                                            (defaultPowerstate & 02) ? (NFC_PROTOCOL_MASK_ISO7816) :0,
                                                            (defaultPowerstate & 04) ? (NFC_PROTOCOL_MASK_ISO7816) :0,
                                                            (defaultPowerstate & 0x08) ? NFC_PROTOCOL_MASK_ISO7816 :0,
                                                            (defaultPowerstate & 0x10) ? NFC_PROTOCOL_MASK_ISO7816 :0);
                  }else
                  {
                      nfaStat = NFA_EeSetDefaultProtoRouting(defaultSeID ,
                                                            (defaultPowerstate & 01) ? NFC_PROTOCOL_MASK_ISO7816 :0,
                                                            (defaultPowerstate & 02) ? NFC_PROTOCOL_MASK_ISO7816 :0,
                                                            (defaultPowerstate & 04) ? NFC_PROTOCOL_MASK_ISO7816 :0,
                                                            0, 0);
                  }
                  if (nfaStat == NFA_STATUS_OK)
                  mRoutingEvent.wait ();
                  else
                  {
                       ALOGE ("Fail to set  iso7816 routing");
                  }


                  t3t_protocol_mask =0;
              }
              if(mHostListnEnable == FALSE && defaultProtoSeID == 0x400)
              {
                  ALOGE("%s, HOST is disabled hence skipping configure ISO-DEP route to host", fn);
              }
              else
              {
                  if(mCeRouteStrictDisable == 0x01)
                  {
#if(NFC_NXP_CHIP_TYPE != PN547C2)
                      if(defaultProtoSeID == 0x400)
                      {
                          t3t_protocol_mask = NFA_PROTOCOL_MASK_T3T;
                      }
#endif

                      nfaStat = NFA_EeSetDefaultProtoRouting(defaultProtoSeID,
                                                            (defaultProtoPowerstate& 01) ? (NFA_PROTOCOL_MASK_ISO_DEP | t3t_protocol_mask): 0,
                                                            (defaultProtoPowerstate & 02) ? (NFA_PROTOCOL_MASK_ISO_DEP) :0,
                                                            (defaultProtoPowerstate & 04) ? (NFA_PROTOCOL_MASK_ISO_DEP) :0,
                                                            (defaultProtoPowerstate & 0x08) ? NFA_PROTOCOL_MASK_ISO_DEP :0,
                                                            (defaultProtoPowerstate & 0x10) ? NFA_PROTOCOL_MASK_ISO_DEP :0);
                  }else{
                      nfaStat = NFA_EeSetDefaultProtoRouting(defaultProtoSeID,
                                                             (defaultProtoPowerstate& 01) ? NFA_PROTOCOL_MASK_ISO_DEP: 0,
                                                             (defaultProtoPowerstate & 02) ? NFA_PROTOCOL_MASK_ISO_DEP :0,
                                                             (defaultProtoPowerstate & 04) ? NFA_PROTOCOL_MASK_ISO_DEP :0,
                                                             0, 0 );
                  }
                  if (nfaStat == NFA_STATUS_OK)
                       mRoutingEvent.wait ();
                  else
                  {
                       ALOGE ("Fail to set  iso7816 routing");
                  }
                  t3t_protocol_mask =0;
              }
       }

#if(NFC_NXP_CHIP_TYPE != PN547C2)
       if(0x400 != defaultProtoSeID  && 0x400 != defaultSeID && mHostListnEnable == TRUE)
       {

           if(mCeRouteStrictDisable == 0x01)
           {
               nfaStat = NFA_EeSetDefaultProtoRouting(0x400 ,
                                                      NFA_PROTOCOL_MASK_T3T,
                                                      0,
                                                      0,
                                                      NFA_PROTOCOL_MASK_T3T,
                                                      0);

           }else{
                 nfaStat = NFA_EeSetDefaultProtoRouting(0x400 ,
                                                        NFA_PROTOCOL_MASK_T3T,
                                                        0,
                                                        0,
                                                        0,
                                                        0);
           }
           if (nfaStat == NFA_STATUS_OK)//FelicaOnHost
               mRoutingEvent.wait ();
           else
           {
               ALOGE ("Fail to set T3T protocol routing to HOST");
           }
       }
#endif
//ee_handle = SecureElement::getInstance().getEseHandleFromGenericId(switch_on);
    ALOGD ("%s: enter, defaultHandle:%x", fn, defaultHandle);
    ALOGD ("%s: enter, preferred_defaultHandle:%x", fn, preferred_defaultHandle);


    {
        unsigned long max_tech_mask = 0x03;
        max_tech_mask = SecureElement::getInstance().getSETechnology(defaultTechSeID);
        ALOGD ("%s: enter,max_tech_mask :%x", fn, max_tech_mask);
        unsigned int default_tech_power_mask[5]={0,};
        unsigned int defaultTechFPowerstate=0x1F;

        ALOGD ("%s: enter, defaultTechSeID:%x", fn, defaultTechSeID);
        if(defaultTechSeID == 0x402)
        {
               for(int pCount=0 ; pCount< 5 ;pCount++)
               {
                    if((defaultTechAPowerstate >> pCount)&0x01)
                    {
                         default_tech_power_mask[pCount] |= (NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B);
                    }
                    if((defaultTechFPowerstate >> pCount)&0x01)
                    {
                         default_tech_power_mask[pCount] |= NFA_TECHNOLOGY_MASK_F;
                    }
               }

               if(mHostListnEnable == TRUE && mFwdFuntnEnable == TRUE)
               {
                   if(!(max_tech_mask & NFA_TECHNOLOGY_MASK_A)
                       && ( max_tech_mask & NFA_TECHNOLOGY_MASK_B))
                   {
                       if(mCeRouteStrictDisable == 0x01)
                       {
                           nfaStat =  NFA_EeSetDefaultTechRouting (0x400,
                                                                   NFA_TECHNOLOGY_MASK_A,
                                                                   0,
                                                                   0,
                                                                   NFA_TECHNOLOGY_MASK_A,
                                                                   0 );
                       }else{
                           nfaStat =  NFA_EeSetDefaultTechRouting (0x400,
                                                                   NFA_TECHNOLOGY_MASK_A,
                                                                   0, 0, 0, 0 );
                       }
                       if (nfaStat == NFA_STATUS_OK)
                          mRoutingEvent.wait ();
                       else
                       {
                           ALOGE ("Fail to set tech routing");
                       }
                   }
                   else if(!(max_tech_mask & NFA_TECHNOLOGY_MASK_B)
                            && (max_tech_mask & NFA_TECHNOLOGY_MASK_A))
                   {
                       if(mCeRouteStrictDisable == 0x01)
                       {
                           nfaStat =  NFA_EeSetDefaultTechRouting (0x400,
                                                                  NFA_TECHNOLOGY_MASK_B,
                                                                  0,
                                                                  0,
                                                                  NFA_TECHNOLOGY_MASK_B,
                                                                  0 );
                       }else{
                           nfaStat =  NFA_EeSetDefaultTechRouting (0x400,
                                                                  NFA_TECHNOLOGY_MASK_B,
                                                                  0, 0, 0, 0 );
                       }
                       if (nfaStat == NFA_STATUS_OK)
                          mRoutingEvent.wait ();
                       else
                       {
                           ALOGE ("Fail to set tech routing");
                       }
                   }
               }
               if(mCeRouteStrictDisable == 0x01)
               {
                   nfaStat =  NFA_EeSetDefaultTechRouting (defaultTechSeID,
                                                      (max_tech_mask & default_tech_power_mask[0]),
                                                      (max_tech_mask & default_tech_power_mask[1]),
                                                      (max_tech_mask & default_tech_power_mask[2]),
                                                      (max_tech_mask & default_tech_power_mask[3]),
                                                      (max_tech_mask & default_tech_power_mask[4]));
               }else{
                   nfaStat =  NFA_EeSetDefaultTechRouting (defaultTechSeID,
                                                      (max_tech_mask & default_tech_power_mask[0]),
                                                      (max_tech_mask & default_tech_power_mask[1]),
                                                      (max_tech_mask & default_tech_power_mask[2]),
                                                      0,
                                                      0);
               }
               if (nfaStat == NFA_STATUS_OK)
                     mRoutingEvent.wait ();
               else
               {
                     ALOGE ("Fail to set tech routing");
               }
        }
        else
        {
              DefaultTechType &= ~NFA_TECHNOLOGY_MASK_F;
              DefaultTechType |= (NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B);
              if(mCeRouteStrictDisable == 0x01)
              {
                  nfaStat =  NFA_EeSetDefaultTechRouting (defaultTechSeID,
                                                         (defaultTechAPowerstate& 01) ?  (max_tech_mask & DefaultTechType): 0,
                                                         (defaultTechAPowerstate & 02) ? (max_tech_mask & DefaultTechType) :0,
                                                         (defaultTechAPowerstate & 04) ? (max_tech_mask & DefaultTechType) :0,
                                                         (defaultTechAPowerstate & 0x08) ? (max_tech_mask & DefaultTechType) :0,
                                                         (defaultTechAPowerstate & 0x10) ? (max_tech_mask & DefaultTechType) :0);
              }else{
                   nfaStat =  NFA_EeSetDefaultTechRouting (defaultTechSeID,
                                                          (defaultTechAPowerstate& 01) ?  (max_tech_mask & DefaultTechType): 0,
                                                          (defaultTechAPowerstate & 02) ? (max_tech_mask & DefaultTechType) :0,
                                                          (defaultTechAPowerstate & 04) ? (max_tech_mask & DefaultTechType) :0,
                                                          0, 0 );
              }
              if (nfaStat == NFA_STATUS_OK)
                  mRoutingEvent.wait ();
              else
              {
                  ALOGE ("Fail to set  tech routing");
              }
              max_tech_mask = SecureElement::getInstance().getSETechnology(0x402);
              if(mCeRouteStrictDisable == 0x01)
              {
                  nfaStat =  NFA_EeSetDefaultTechRouting (0x402,
                                                         (defaultTechFPowerstate& 01) ?  (max_tech_mask & NFA_TECHNOLOGY_MASK_F): 0,
                                                         (defaultTechFPowerstate & 02) ? (max_tech_mask & NFA_TECHNOLOGY_MASK_F) :0,
                                                         (defaultTechFPowerstate & 04) ? (max_tech_mask & NFA_TECHNOLOGY_MASK_F) :0,
                                                         (defaultTechFPowerstate & 0x08) ? (max_tech_mask & NFA_TECHNOLOGY_MASK_F) :0,
                                                         (defaultTechFPowerstate & 0x10) ? (max_tech_mask & NFA_TECHNOLOGY_MASK_F) :0);
              }else{
                  nfaStat =  NFA_EeSetDefaultTechRouting (0x402,
                                                          (defaultTechFPowerstate& 01) ?  (max_tech_mask & NFA_TECHNOLOGY_MASK_F): 0,
                                                          (defaultTechFPowerstate & 02) ? (max_tech_mask & NFA_TECHNOLOGY_MASK_F) :0,
                                                          (defaultTechFPowerstate & 04) ? (max_tech_mask & NFA_TECHNOLOGY_MASK_F) :0,
                                                          0, 0 );
              }
              if (nfaStat == NFA_STATUS_OK)
                     mRoutingEvent.wait ();
              else
              {
                     ALOGE ("Fail to set  tech routing");
              }

              if(mHostListnEnable == TRUE && mFwdFuntnEnable == TRUE)
              {
                  if(!(max_tech_mask & NFA_TECHNOLOGY_MASK_A)
                      && ( max_tech_mask & NFA_TECHNOLOGY_MASK_B))
                  {
                      if(mCeRouteStrictDisable == 0x01)
                      {
                      nfaStat =  NFA_EeSetDefaultTechRouting (0x400,
                                                              NFA_TECHNOLOGY_MASK_A,
                                                              0,
                                                              0,
                                                              NFA_TECHNOLOGY_MASK_A ,
                                                              0 );
                      }else
                      {
                          nfaStat =  NFA_EeSetDefaultTechRouting (0x400,
                                                                  NFA_TECHNOLOGY_MASK_A,
                                                                  0, 0, 0, 0 );
                      }
                      if (nfaStat == NFA_STATUS_OK)
                          mRoutingEvent.wait ();
                      else
                      {
                          ALOGE ("Fail to set  tech routing");
                      }
                  }
                  else if(!(max_tech_mask & NFA_TECHNOLOGY_MASK_B)
                          && (max_tech_mask & NFA_TECHNOLOGY_MASK_A))
                  {
                      if(mCeRouteStrictDisable == 0x01)
                      {
                      nfaStat =  NFA_EeSetDefaultTechRouting (0x400,
                                                              NFA_TECHNOLOGY_MASK_B,
                                                              0,
                                                              0,
                                                              NFA_TECHNOLOGY_MASK_B,
                                                              0 );
                      }else{
                          nfaStat =  NFA_EeSetDefaultTechRouting (0x400,
                                                                  NFA_TECHNOLOGY_MASK_B,
                                                                  0, 0, 0, 0 );
                      }
                      if (nfaStat == NFA_STATUS_OK)
                          mRoutingEvent.wait ();
                      else
                      {
                          ALOGE ("Fail to set  tech routing");
                      }
                  }
              }
        }
    }

    if ((GetNumValue(NAME_UICC_LISTEN_TECH_MASK, &uiccListenTech, sizeof(uiccListenTech))))
    {
         ALOGD ("%s:UICC_TECH_MASK=0x0%lu;", __FUNCTION__, uiccListenTech);
    }
    if((defaultHandle != NFA_HANDLE_INVALID)  &&  (0 != uiccListenTech))
    {
         {
               SyncEventGuard guard (SecureElement::getInstance().mUiccListenEvent);
               nfaStat = NFA_CeConfigureUiccListenTech (defaultHandle, 0x00);
               if (nfaStat == NFA_STATUS_OK)
               {
                     SecureElement::getInstance().mUiccListenEvent.wait ();
               }
               else
                     ALOGE ("fail to start UICC listen");
         }
         {
               SyncEventGuard guard (SecureElement::getInstance().mUiccListenEvent);
               nfaStat = NFA_CeConfigureUiccListenTech (defaultHandle, (uiccListenTech & 0x07));
               if(nfaStat == NFA_STATUS_OK)
               {
                     SecureElement::getInstance().mUiccListenEvent.wait ();
               }
               else
                     ALOGE ("fail to start UICC listen");
         }
    }
    return true;
}
void RoutingManager::setCeRouteStrictDisable(UINT32 state)
{
    ALOGD ("%s: mCeRouteScreenLock = 0x%X", __FUNCTION__, state);
    mCeRouteStrictDisable = state;
}

#endif

void RoutingManager::enableRoutingToHost()
{
    tNFA_STATUS nfaStat;

    {
        SyncEventGuard guard (mRoutingEvent);

        // Route Nfc-A to host if we don't have a SE
        if (mSeTechMask == 0)
        {
#if(NXP_EXTNS == TRUE)
            if(mCeRouteStrictDisable == 0x01)
            {
                nfaStat = NFA_EeSetDefaultTechRouting (mDefaultEe, NFA_TECHNOLOGY_MASK_A, 0, 0, NFA_TECHNOLOGY_MASK_A, 0);
            }else{
                nfaStat = NFA_EeSetDefaultTechRouting (mDefaultEe, NFA_TECHNOLOGY_MASK_A, 0, 0, 0, 0);
            }
            if (nfaStat == NFA_STATUS_OK)
#else
            nfaStat = NFA_EeSetDefaultTechRouting (mDefaultEe, NFA_TECHNOLOGY_MASK_A, 0, 0);
#endif
            if (nfaStat == NFA_STATUS_OK)
                mRoutingEvent.wait ();
            else
                ALOGE ("Fail to set default tech routing");
        }

        // Default routing for IsoDep protocol
#if(NXP_EXTNS == TRUE)
        if(mCeRouteStrictDisable == 0x01)
        {
            nfaStat = NFA_EeSetDefaultProtoRouting(mDefaultEe,
                                                   NFA_PROTOCOL_MASK_ISO_DEP,
                                                   0,
                                                   0,
                                                   NFA_PROTOCOL_MASK_ISO_DEP,
                                                   0);
        }else{
            nfaStat = NFA_EeSetDefaultProtoRouting(mDefaultEe,
                                                   NFA_PROTOCOL_MASK_ISO_DEP,
                                                   0,
                                                   0,
                                                   0,
                                                   0);
        }
#else
        nfaStat = NFA_EeSetDefaultProtoRouting(mDefaultEe,
                                               NFA_PROTOCOL_MASK_ISO_DEP,
                                               0,
                                               0);
#endif
        if (nfaStat == NFA_STATUS_OK)
            mRoutingEvent.wait ();
        else
            ALOGE ("Fail to set default proto routing");
    }
}

//TODO:
void RoutingManager::disableRoutingToHost()
{
    tNFA_STATUS nfaStat;

    {
        SyncEventGuard guard (mRoutingEvent);
        // Default routing for NFC-A technology if we don't have a SE
        if (mSeTechMask == 0)
        {
#if(NXP_EXTNS == TRUE)
            nfaStat = NFA_EeSetDefaultTechRouting (mDefaultEe, 0, 0, 0, 0, 0);
#else
            nfaStat = NFA_EeSetDefaultTechRouting (mDefaultEe, 0, 0, 0);
#endif
            if (nfaStat == NFA_STATUS_OK)
                mRoutingEvent.wait ();
            else
                ALOGE ("Fail to set default tech routing");
        }

        // Default routing for IsoDep protocol
#if(NXP_EXTNS == TRUE)
        nfaStat = NFA_EeSetDefaultProtoRouting(mDefaultEe, 0, 0, 0, 0, 0);
#else
        nfaStat = NFA_EeSetDefaultProtoRouting(mDefaultEe, 0, 0, 0);
#endif
        if (nfaStat == NFA_STATUS_OK)
            mRoutingEvent.wait ();
        else
            ALOGE ("Fail to set default proto routing");
    }
}
#if(NXP_EXTNS == TRUE)
bool RoutingManager::setRoutingEntry(int type, int value, int route, int power)
{
    static const char fn [] = "RoutingManager::setRoutingEntry";
    ALOGD ("%s: enter, type:0x%x value =0x%x route:%x power:0x%x", fn, type, value ,route, power);
    unsigned long max_tech_mask = 0x03;
    unsigned long uiccListenTech = 0;
    max_tech_mask = SecureElement::getInstance().getSETechnology(0x402);
    ALOGD ("%s: enter,max_tech_mask :%x", fn, max_tech_mask);

    tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
    tNFA_HANDLE ee_handle = NFA_HANDLE_INVALID;
    SyncEventGuard guard (mRoutingEvent);
    UINT8 switch_on_mask = 0x00;
    UINT8 switch_off_mask   = 0x00;
    UINT8 battery_off_mask = 0x00;
    UINT8 screen_lock_mask = 0x00;
    UINT8 screen_off_mask = 0x00;
    UINT8 protocol_mask = 0x00;
    tNFA_HANDLE uicc_handle = NFA_HANDLE_INVALID;
    tNFA_HANDLE ese_handle = NFA_HANDLE_INVALID;
    ee_handle = (( route == 0x01)? 0x4C0 : (( route == 0x02)? 0x402 : NFA_HANDLE_INVALID));
    if(0x00 == route)
    {
        ee_handle = 0x400;
    }
    if(ee_handle == NFA_HANDLE_INVALID )
    {
        ALOGD ("%s: enter, handle:%x invalid", fn, ee_handle);
        return nfaStat;
    }

    tNFA_HANDLE ActDevHandle = NFA_HANDLE_INVALID;
    UINT8 count,seId=0;
    UINT8 isSeIDPresent = 0;
    tNFA_HANDLE ee_handleList[SecureElement::MAX_NUM_EE];
    SecureElement::getInstance().getEeHandleList(ee_handleList, &count);


    for (int  i = 0; ((count != 0 ) && (i < count)); i++)
    {
        seId = SecureElement::getInstance().getGenericEseId(ee_handleList[i]);
        ActDevHandle = SecureElement::getInstance().getEseHandleFromGenericId(seId);
        ALOGD ("%s: enter, ee_handleList[%d]:%x", fn, i,ee_handleList[i]);
        if ((ee_handle != 0x400) &&
            (ee_handle == ActDevHandle))
        {
            isSeIDPresent =1;
            break;
        }
    }

    if(!isSeIDPresent)
    {
        ee_handle = 0x400;
    }

    if(NFA_SET_TECHNOLOGY_ROUTING == type)
    {
        switch_on_mask    = (power & 0x01) ? value : 0;
        switch_off_mask   = (power & 0x02) ? value : 0;
        battery_off_mask  = (power & 0x04) ? value : 0;
        screen_off_mask   = (power & 0x08) ? value : 0;
        screen_lock_mask  = (power & 0x10) ? value : 0;


        if(mHostListnEnable == 0x01 && mFwdFuntnEnable == TRUE)
        {
            if((max_tech_mask != 0x01) && (max_tech_mask == 0x02))
            {
                switch_on_mask &= ~NFA_TECHNOLOGY_MASK_A;
                switch_off_mask &= ~NFA_TECHNOLOGY_MASK_A;
                battery_off_mask &= ~NFA_TECHNOLOGY_MASK_A;
                screen_off_mask &= ~NFA_TECHNOLOGY_MASK_A;
                screen_lock_mask &= ~NFA_TECHNOLOGY_MASK_A;

                if(mCeRouteStrictDisable == 0x01)
                {
                    nfaStat =  NFA_EeSetDefaultTechRouting (0x400,
                                                            NFA_TECHNOLOGY_MASK_A,
                                                            0,
                                                            0,
                                                            NFA_TECHNOLOGY_MASK_A,
                                                            0 );
                }else{
                    nfaStat =  NFA_EeSetDefaultTechRouting (0x400,
                                                            NFA_TECHNOLOGY_MASK_A,
                                                            0, 0, 0, 0 );
                }
                if (nfaStat == NFA_STATUS_OK)
                   mRoutingEvent.wait ();
                else
                {
                    ALOGE ("Fail to set tech routing");
                }
            }
            else if((max_tech_mask == 0x01) && (max_tech_mask != 0x02))
            {
                switch_on_mask &= ~NFA_TECHNOLOGY_MASK_B;
                switch_off_mask &= ~NFA_TECHNOLOGY_MASK_B;
                battery_off_mask &= ~NFA_TECHNOLOGY_MASK_B;
                screen_off_mask &= ~NFA_TECHNOLOGY_MASK_B;
                screen_lock_mask &= ~NFA_TECHNOLOGY_MASK_B;

                if(mCeRouteStrictDisable == 0x01)
                {
                    nfaStat =  NFA_EeSetDefaultTechRouting (0x400,
                                                           NFA_TECHNOLOGY_MASK_B,
                                                           0,
                                                           0,
                                                           NFA_TECHNOLOGY_MASK_B,
                                                           0 );
                }else{
                    nfaStat =  NFA_EeSetDefaultTechRouting (0x400,
                                                           NFA_TECHNOLOGY_MASK_B,
                                                           0, 0, 0, 0 );
                }
                if (nfaStat == NFA_STATUS_OK)
                   mRoutingEvent.wait ();
                else
                {
                    ALOGE ("Fail to set tech routing");
                }
            }
        }

        nfaStat = NFA_EeSetDefaultTechRouting (ee_handle, switch_on_mask, switch_off_mask, battery_off_mask, screen_lock_mask, screen_off_mask);
        if(nfaStat == NFA_STATUS_OK){
            mRoutingEvent.wait ();
            ALOGD ("tech routing SUCCESS");
        }
        else{
            ALOGE ("Fail to set default tech routing");
        }
    }else if(NFA_SET_PROTOCOL_ROUTING == type)
    {
        if( value == 0x01)
            protocol_mask = NFA_PROTOCOL_MASK_ISO_DEP;
        if( value == 0x02)
            protocol_mask = NFA_PROTOCOL_MASK_NFC_DEP;
        switch_on_mask     = (power & 0x01) ? protocol_mask : 0;
        switch_off_mask    = (power & 0x02) ? protocol_mask : 0;
        battery_off_mask   = (power & 0x04) ? protocol_mask : 0;
        screen_lock_mask   = (power & 0x10) ? protocol_mask : 0;
        screen_off_mask    = (power & 0x08) ? protocol_mask : 0;
        nfaStat = NFA_EeSetDefaultProtoRouting (ee_handle,switch_on_mask , switch_off_mask, battery_off_mask, screen_lock_mask, screen_off_mask);
        if(nfaStat == NFA_STATUS_OK){
            mRoutingEvent.wait ();
            ALOGD ("tech routing SUCCESS");
        }
        else{
            ALOGE ("Fail to set default tech routing");
        }
    }

    if ((GetNumValue(NAME_UICC_LISTEN_TECH_MASK, &uiccListenTech, sizeof(uiccListenTech))))
    {
         ALOGD ("%s:UICC_TECH_MASK=0x0%lu;", __FUNCTION__, uiccListenTech);
    }
    if((ActDevHandle != NFA_HANDLE_INVALID)  &&  (0 != uiccListenTech))
    {
         {
               SyncEventGuard guard (SecureElement::getInstance().mUiccListenEvent);
               nfaStat = NFA_CeConfigureUiccListenTech (ActDevHandle, 0x00);
               if (nfaStat == NFA_STATUS_OK)
               {
                     SecureElement::getInstance().mUiccListenEvent.wait ();
               }
               else
                     ALOGE ("fail to start UICC listen");
         }
         {
               SyncEventGuard guard (SecureElement::getInstance().mUiccListenEvent);
               nfaStat = NFA_CeConfigureUiccListenTech (ActDevHandle, (uiccListenTech & 0x07));
               if(nfaStat == NFA_STATUS_OK)
               {
                     SecureElement::getInstance().mUiccListenEvent.wait ();
               }
               else
                     ALOGE ("fail to start UICC listen");
         }
    }
    return nfaStat;
}

bool RoutingManager::clearRoutingEntry(int type)
{
    static const char fn [] = "RoutingManager::clearRoutingEntry";
    ALOGD ("%s: enter, type:0x%x", fn, type );
    tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
    //tNFA_HANDLE ee_handle = NFA_HANDLE_INVLAID;
    SyncEventGuard guard (mRoutingEvent);
    if(NFA_SET_TECHNOLOGY_ROUTING & type)
    {
        nfaStat = NFA_EeSetDefaultTechRouting (0x400, 0x00, 0x00, 0x00, 0x00, 0x00);
        if(nfaStat == NFA_STATUS_OK){
            mRoutingEvent.wait ();
            ALOGD ("tech routing SUCCESS");
        }
        else{
            ALOGE ("Fail to set default tech routing");
        }
        nfaStat = NFA_EeSetDefaultTechRouting (0x402, 0x00, 0x00, 0x00, 0x00, 0x00);
        if(nfaStat == NFA_STATUS_OK){
            mRoutingEvent.wait ();
            ALOGD ("tech routing SUCCESS");
        }
        else{
            ALOGE ("Fail to set default tech routing");
        }
        nfaStat = NFA_EeSetDefaultTechRouting (0x4C0, 0x00, 0x00, 0x00, 0x00, 0x00);
        if(nfaStat == NFA_STATUS_OK){
            mRoutingEvent.wait ();
            ALOGD ("tech routing SUCCESS");
        }
        else{
            ALOGE ("Fail to set default tech routing");
        }
    }

    if(NFA_SET_PROTOCOL_ROUTING & type)
    {
        nfaStat = NFA_EeSetDefaultProtoRouting (0x400, 0x00, 0x00, 0x00, 0x00 ,0x00);
        if(nfaStat == NFA_STATUS_OK){
            mRoutingEvent.wait ();
            ALOGD ("protocol routing SUCCESS");
        }
        else{
            ALOGE ("Fail to set default protocol routing");
        }
        nfaStat = NFA_EeSetDefaultProtoRouting (0x402, 0x00, 0x00, 0x00, 0x00, 0x00);
        if(nfaStat == NFA_STATUS_OK){
            mRoutingEvent.wait ();
            ALOGD ("protocol routing SUCCESS");
        }
        else{
            ALOGE ("Fail to set default protocol routing");
        }
        nfaStat = NFA_EeSetDefaultProtoRouting (0x4C0, 0x00, 0x00, 0x00, 0x00, 0x00);
        if(nfaStat == NFA_STATUS_OK){
            mRoutingEvent.wait ();
            ALOGD ("protocol routing SUCCESS");
        }
        else{
            ALOGE ("Fail to set default protocol routing");
        }
    }

    if (NFA_SET_AID_ROUTING & type)
    {
        clearAidTable();
    }
    return nfaStat;
}
#endif
#if(NXP_EXTNS == TRUE)
bool RoutingManager::addAidRouting(const UINT8* aid, UINT8 aidLen, int route, int power, bool isprefix)
#else
bool RoutingManager::addAidRouting(const UINT8* aid, UINT8 aidLen, int route)
#endif
{
    static const char fn [] = "RoutingManager::addAidRouting";
    ALOGD ("%s: enter", fn);
#if(NXP_EXTNS == TRUE)
    tNFA_HANDLE handle;
    tNFA_HANDLE current_handle;
    unsigned long num = 0;
    ALOGD ("%s: enter, route:%x power:0x%x isprefix:%x", fn, route, power, isprefix);
    handle = SecureElement::getInstance().getEseHandleFromGenericId(route);
    ALOGD ("%s: enter, route:%x", fn, handle);
    if (handle  == NFA_HANDLE_INVALID)
    {
        return false;
    }
    if(mAddAid == 0x00)
    {
        ALOGD ("%s: enter, mAddAid set to 0 from config file, ignoring all aids", fn);
        return false;
    }
    current_handle = ((handle == 0x4C0)?SecureElement::ESE_ID:SecureElement::UICC_ID);
    if(handle == 0x400)
        current_handle = 0x00;

    ALOGD ("%s: enter, mDefaultEe:%x", fn, current_handle);
    //SecureElement::getInstance().activate(current_handle);
    // Set power config


    SyncEventGuard guard(SecureElement::getInstance().mAidAddRemoveEvent);
    UINT8 vs_info = 0x00;
    if(isprefix) {
        vs_info = NFA_EE_AE_NXP_PREFIX_MATCH;
    }
    tNFA_STATUS nfaStat = NFA_EeAddAidRouting(handle, aidLen, (UINT8*) aid, power, vs_info);
#else
    tNFA_STATUS nfaStat = NFA_EeAddAidRouting(route, aidLen, (UINT8*) aid, 0x01);
#endif
    if (nfaStat == NFA_STATUS_OK)
    {
//        ALOGD ("%s: routed AID", fn);
#if(NXP_EXTNS == TRUE)
        SecureElement::getInstance().mAidAddRemoveEvent.wait();
#endif
        return true;
    } else
    {
        ALOGE ("%s: failed to route AID",fn);
        return false;
    }
}

bool RoutingManager::removeAidRouting(const UINT8* aid, UINT8 aidLen)
{
    static const char fn [] = "RoutingManager::removeAidRouting";
    ALOGD ("%s: enter", fn);
    SyncEventGuard guard(SecureElement::getInstance().mAidAddRemoveEvent);
    tNFA_STATUS nfaStat = NFA_EeRemoveAidRouting(aidLen, (UINT8*) aid);
    if (nfaStat == NFA_STATUS_OK)
    {
        SecureElement::getInstance().mAidAddRemoveEvent.wait();
        ALOGD ("%s: removed AID", fn);
        return true;
    } else
    {
        ALOGE ("%s: failed to remove AID",fn);
        return false;
    }
}

#if(NXP_EXTNS == TRUE)
//FelicaOnHost


int RoutingManager::addNfcid2Routing(UINT8* nfcid2, UINT8 nfcid2Len,const UINT8* syscode,
        int syscodelen,const UINT8* optparam, int optparamlen)
{
    static const char fn [] = "RoutingManager::addNfcid2Routing";
    ALOGD ("%s: enter", fn);

    UINT8 NfcID2_loc_add =0;;
    NfcID2_add_req.mMutex.lock();

    if((nfcid2 == NULL) || (nfcid2Len !=8) || (syscode == NULL) ||(syscodelen !=2))
    {
     ALOGE ("%s: return ---1", fn);
        return 1;

    }


    for(NfcID2_loc_add = 0 ; NfcID2_loc_add < NFCID2_COUNT_MAX ; NfcID2_loc_add++)
    {
        if( 0==memcmp(NfcID2_add_req.NfcID2_info[NfcID2_loc_add].nfcid2,nfcid2,NFCID2_LEN_MAX))

        {
            ALOGE ("%s: return ---2", fn);
            return 1;
        }
    }

    for(NfcID2_loc_add = 0 ; NfcID2_loc_add < NFCID2_COUNT_MAX ; NfcID2_loc_add++)
    {
        if(NfcID2_add_req.NfcID2_info[NfcID2_loc_add].InUse == 0 )
        {
            ALOGE ("%s:  NfcID2_loc_add=0x%X", fn, NfcID2_loc_add);
            break;
        }
    }

    if(NFCID2_COUNT_MAX == NfcID2_loc_add)
    {
        ALOGE ("%s:  NfcID2_loc_add   1111  =0x%X", fn, NfcID2_loc_add);
        return 1;

    }

    memcpy(NfcID2_add_req.NfcID2_info[NfcID2_loc_add].nfcid2,nfcid2,nfcid2Len);
    NfcID2_add_req.NfcID2_info[NfcID2_loc_add].InUse = 1;
    memcpy(NfcID2_add_req.NfcID2_info[NfcID2_loc_add].sysCode,syscode,syscodelen);

    if(optparam !=NULL)
    {

        memcpy(NfcID2_add_req.NfcID2_info[NfcID2_loc_add].optParam,optparam,optparamlen);
    }

    NfcID2_add_req.NfcID2_info[NfcID2_loc_add].nfcid2Handle = 0xFF;
    NfcID2_add_req.nfcID2_req_timer.kill();
    NfcID2_add_req.nfcID2_req_timer.set(NFCID2_ADD_REQ_TIMEOUT,NfcID2_req_timoutHandler);
    NfcID2_add_req.mMutex.unlock();


    return 1;
}

bool RoutingManager::removeNfcid2Routing(UINT8* nfcid2) {

    static const char fn [] = "RoutingManager::removeNfcid2Routing";
    ALOGD ("%s: enter", fn);
    int i=0;
    UINT16 nfcid2Handle=0;
    UINT8 nfcid2HandleLoc=0;
    UINT8 NfcID2_loc_rmv=0 ;
    //NfcID2_add_req.mMutex.lock();

    if(nfcid2 == NULL)
        return false;

    for(nfcid2HandleLoc=0;nfcid2HandleLoc< NFCID2_COUNT_MAX ; nfcid2HandleLoc++)
    {
        if (0== memcmp(NfcID2_add_req.NfcID2_info[nfcid2HandleLoc].nfcid2,nfcid2,NFCID2_LEN_MAX))
        {
            nfcid2Handle = NfcID2_add_req.NfcID2_info[nfcid2HandleLoc].nfcid2Handle;
            if(0xFF != nfcid2Handle)
            {

                for(NfcID2_loc_rmv = 0 ; NfcID2_loc_rmv < NFCID2_COUNT_MAX ; NfcID2_loc_rmv++)
                {
                    if( nfcid2Handle == NfcId2_rmv_req.NfcID2_info[NfcID2_loc_rmv].nfcid2Handle)
                        return 0xFF;
                }
                for(NfcID2_loc_rmv = 0 ; NfcID2_loc_rmv < NFCID2_COUNT_MAX ; NfcID2_loc_rmv++)
                {
                    if(NfcId2_rmv_req.NfcID2_info[NfcID2_loc_rmv].InUse == 0 )
                        break;
                }

                if(NfcID2_loc_rmv < NFCID2_COUNT_MAX)
                {
                    NfcId2_rmv_req.NfcID2_info[NfcID2_loc_rmv].nfcid2Handle = nfcid2Handle;
                    NfcId2_rmv_req.NfcID2_info[NfcID2_loc_rmv].InUse = 1;
                }
                NfcId2_rmv_req.nfcID2_rmv_req_timer.kill();
                NfcId2_rmv_req.nfcID2_rmv_req_timer.set(NFCID2_RMV_REQ_TIMEOUT,NfcID2_rmv_timoutHandler);
            }
        }
    }
    //NfcID2_add_req.mMutex.unlock();

    return true;;
}



//FelicaOnHost
void *nfcID2_req_handler_async(void *arg)
{

     tNFC_STATUS status;
     static const char fn [] = "RoutingManager::nfcID2_req_handler_async";
     ALOGD ("%s:  ", fn);
     RoutingManager& routingManager = RoutingManager::getInstance();
     int scrState = android ::getScreenState();
     if(android::isDiscoveryStarted() == true)
     {
         android::startRfDiscovery(false);
     }

     {
     SyncEventGuard guard (android::sNfaEnableDisablePollingEvent);
     ALOGD ("%s: disable polling", __FUNCTION__);
     status = NFA_DisablePolling ();
     if (status == NFA_STATUS_OK)
     {
         ALOGE ("%s: sNfaEnableDisablePollingEvent.wait=0x%X", __FUNCTION__, status);
         android::sNfaEnableDisablePollingEvent.wait (); //wait for NFA_POLL_DISABLED_EVT
     }
     else
         ALOGE ("%s: fail disable polling; error=0x%X", __FUNCTION__, status);
     }

     {
        routingManager.HandleAddNfcID2_Req();
        scrState = android::getScreenState();
        ALOGE ("%s:startStopPolling scrState =0x%X", __FUNCTION__, scrState);
        if(scrState == 3) //NFA_SCREEN_STATE_UNLOCKED
        {
            android::startStopPolling(true);
        }
        else
        {
            android::startStopPolling(false);
        }
        //android::startRfDiscovery(true);
     }

    return NULL;
}


//FelicaOnHost
void *nfcID2_rmv_handler_async(void *arg)
{

     tNFC_STATUS status;
     static const char fn [] = "RoutingManager::nfcID2_req_handler_async";
     ALOGD ("%s:  ", fn);
     RoutingManager& routingManager = RoutingManager::getInstance();
     int scrState = android ::getScreenState();
     if(android::isDiscoveryStarted() == true)
     {
         android::startRfDiscovery(false);
     }

     {
     SyncEventGuard guard (android::sNfaEnableDisablePollingEvent);
     ALOGD ("%s: disable polling", __FUNCTION__);
     status = NFA_DisablePolling ();
     if (status == NFA_STATUS_OK)
     {
         ALOGE ("%s: sNfaEnableDisablePollingEvent.wait=0x%X", __FUNCTION__, status);
         android::sNfaEnableDisablePollingEvent.wait (); //wait for NFA_POLL_DISABLED_EVT
     }
     else
         ALOGE ("%s: fail disable polling; error=0x%X", __FUNCTION__, status);
     }

     {
        routingManager.HandleRmvNfcID2_Req();
        scrState = android::getScreenState();
        ALOGE ("%s:startStopPolling scrState =0x%X", __FUNCTION__, scrState);
        if(scrState == 3) //NFA_SCREEN_STATE_UNLOCKED
        {
            android::startStopPolling(true);
        }
        else
        {
            android::startStopPolling(false);
        }
        //android::startRfDiscovery(true);
     }

     return NULL;
}



void NfcID2_req_timoutHandler (union sigval)
{

    static const char fn [] = "RoutingManager::NfcID2_req_timoutHandler";
    ALOGD ("%s:  ", fn);
    int ret = -1;
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&thread, &attr, &nfcID2_req_handler_async, NULL);
    pthread_attr_destroy(&attr);
    if (ret != 0)
    {
        ALOGE("Unable to create the thread");
    }
}


void NfcID2_rmv_timoutHandler (union sigval)
{

    static const char fn [] = "RoutingManager::NfcID2_req_timoutHandler";
    ALOGD ("%s:  ", fn);
    int ret = -1;
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&thread, &attr, &nfcID2_rmv_handler_async, NULL);
    pthread_attr_destroy(&attr);
    if (ret != 0)
    {
        ALOGE("Unable to create the thread");
    }
}

void RoutingManager::HandleAddNfcID2_Req()
{
    int i=0;
    UINT16 sysCode=0;
    UINT8 defaultNfcId2[NFCID2_LEN_MAX]={0,0,0,0,0,0,0,0};
    static const char fn [] = "RoutingManager::HandleAddNfcID2_Req";
    RoutingManager& routingManager = RoutingManager::getInstance();
    //NfcID2_add_req.mMutex.lock();
    for(i=0; i< NFCID2_COUNT_MAX ; i++)
    {

        if(NfcID2_add_req.NfcID2_info[i].nfcid2Handle == 0xFF)
        {
            sysCode = ((NfcID2_add_req.NfcID2_info[i].sysCode[1])
                          | (NfcID2_add_req.NfcID2_info[i].sysCode[0])<<8);
            ALOGE ("%s: NfcID2_add_req.system_code=0x%X", __FUNCTION__, sysCode);
            SyncEventGuard guard (routingManager.mCeRegisterEvent);
            tNFA_STATUS status = NFA_CeRegisterFelicaSystemCodeOnDH (sysCode,
                                 NfcID2_add_req.NfcID2_info[i].nfcid2,
                                 stackCallback);

            if(status == NFA_STATUS_OK) {
                ALOGE ("%s:mCeRegisterEvent.wait status =0x%X", __FUNCTION__, status);
                routingManager.mCeRegisterEvent.wait();
                if( lastcehandle != 0xFF) {
                    NfcID2_add_req.NfcID2_info[i].nfcid2Handle = lastcehandle;
                     ALOGD ("%s: success", fn);
                } else {
                 ALOGD ("%s: failed", fn);
                }
                ALOGE ("%s:mCeRegisterEvent.wait over status =0x%X", __FUNCTION__, status);
            }
       }
    }
    //NfcID2_add_req.mMutex.unlock();
}


void RoutingManager::HandleRmvNfcID2_Req()
{

    UINT8 nfcid2RmvHandleLoc =0;
    UINT8 nfcid2AddHandleLoc =0;
    UINT16 nfcid2RMVHandle;
    static const char fn [] = "RoutingManager::HandleRmvNfcID2_Req";
    RoutingManager& routingManager = RoutingManager::getInstance();
    //NfcID2_add_req.mMutex.lock();
    for(nfcid2RmvHandleLoc =0; nfcid2RmvHandleLoc < NFCID2_COUNT_MAX ; nfcid2RmvHandleLoc++)
    {
        nfcid2RMVHandle = NfcId2_rmv_req.NfcID2_info[nfcid2RmvHandleLoc].nfcid2Handle ;
       // NfcId2_rmv_req.NfcID2_info[NfcID2_loc_rmv].nfcid2Handle = nfcid2Handle;

        if(0xFF != nfcid2RMVHandle)
        {
             bool result = false;
             SyncEventGuard guard (mCeDeRegisterEvent);
             tNFA_STATUS status =  NFA_CeDeregisterFelicaSystemCodeOnDH (nfcid2RMVHandle);
             if(status == NFA_STATUS_OK) {
                 mCeDeRegisterEvent.wait();
                 result = true;
             }

            if(true == result)
            {
                NfcId2_rmv_req.NfcID2_info[nfcid2RmvHandleLoc].InUse =0;
                NfcId2_rmv_req.NfcID2_info[nfcid2RmvHandleLoc].nfcid2Handle = 0xFF;

                for(nfcid2AddHandleLoc =0; nfcid2AddHandleLoc < NFCID2_COUNT_MAX ; nfcid2AddHandleLoc++)
                {
                   if(NfcID2_add_req.NfcID2_info[nfcid2AddHandleLoc].nfcid2Handle == nfcid2RMVHandle)
                   {
                     NfcID2_add_req.NfcID2_info[nfcid2AddHandleLoc].InUse =0;
                     NfcID2_add_req.NfcID2_info[nfcid2AddHandleLoc].nfcid2Handle = 0xFF;
                   }
                }
            }
        }
    }
    //NfcID2_add_req.mMutex.unlock();
}

void RoutingManager::setDefaultTechRouting (int seId, int tech_switchon,int tech_switchoff)
{
    ALOGD ("ENTER setDefaultTechRouting");
    tNFA_STATUS nfaStat;
    /*// !!! CLEAR ALL REGISTERED TECHNOLOGIES !!!*/
    {
        SyncEventGuard guard (mRoutingEvent);
        tNFA_STATUS status =  NFA_EeSetDefaultTechRouting(0x400,0,0,0,0,0); //HOST clear
        if(status == NFA_STATUS_OK)
        {
            mRoutingEvent.wait ();
        }
    }

    {
        SyncEventGuard guard (mRoutingEvent);
        tNFA_STATUS status =  NFA_EeSetDefaultTechRouting(0x402,0,0,0,0,0); //UICC clear
        if(status == NFA_STATUS_OK)
        {
           mRoutingEvent.wait ();
        }
    }

    {
        SyncEventGuard guard (mRoutingEvent);
        tNFA_STATUS status =  NFA_EeSetDefaultTechRouting(0x4C0,0,0,0,0,0); //SMX clear
        if(status == NFA_STATUS_OK)
        {
           mRoutingEvent.wait ();
        }
    }

    {
        SyncEventGuard guard (mRoutingEvent);
        if(mCeRouteStrictDisable == 0x01)
        {
            nfaStat = NFA_EeSetDefaultTechRouting (seId, tech_switchon, tech_switchoff, 0, NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B, NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B);
        }else{
            nfaStat = NFA_EeSetDefaultTechRouting (seId, tech_switchon, tech_switchoff, 0, 0, 0);
        }
        if(nfaStat == NFA_STATUS_OK)
        {
           mRoutingEvent.wait ();
           ALOGD ("tech routing SUCCESS");
        }
        else
        {
            ALOGE ("Fail to set default tech routing");
        }
    }

    nfaStat = NFA_EeUpdateNow();
    if (nfaStat != NFA_STATUS_OK){
        ALOGE("Failed to commit routing configuration");
    }
}

void RoutingManager::setDefaultProtoRouting (int seId, int proto_switchon,int proto_switchoff)
{
    tNFA_STATUS nfaStat;
    ALOGD ("ENTER setDefaultProtoRouting");
    SyncEventGuard guard (mRoutingEvent);
    if(mCeRouteStrictDisable == 0x01)
    {
        nfaStat = NFA_EeSetDefaultProtoRouting (seId, proto_switchon, proto_switchoff, 0, NFA_PROTOCOL_MASK_ISO_DEP, NFA_PROTOCOL_MASK_ISO_DEP);
    }else{
        nfaStat = NFA_EeSetDefaultProtoRouting (seId, proto_switchon, proto_switchoff, 0, 0, 0);
    }
    if(nfaStat == NFA_STATUS_OK){
        mRoutingEvent.wait ();
        ALOGD ("proto routing SUCCESS");
    }
    else{
        ALOGE ("Fail to set default proto routing");
    }
//    nfaStat = NFA_EeUpdateNow();
//    if (nfaStat != NFA_STATUS_OK){
//        ALOGE("Failed to commit routing configuration");
//    }
}

bool RoutingManager::clearAidTable ()
{
    static const char fn [] = "RoutingManager::clearAidTable";
    ALOGD ("%s: enter", fn);
    SyncEventGuard guard(SecureElement::getInstance().mAidAddRemoveEvent);
    tNFA_STATUS nfaStat = NFA_EeRemoveAidRouting(NFA_REMOVE_ALL_AID_LEN, (UINT8*) NFA_REMOVE_ALL_AID);
    if (nfaStat == NFA_STATUS_OK)
    {
        SecureElement::getInstance().mAidAddRemoveEvent.wait();
        ALOGD ("%s: removed AID", fn);
        return true;
    } else
    {
        ALOGE ("%s: failed to remove AID", fn);
        return false;
    }
}


#endif

bool RoutingManager::commitRouting()
{
    static const char fn [] = "RoutingManager::commitRouting";
    tNFA_STATUS nfaStat = 0;
    ALOGD ("%s", fn);
    {
        SyncEventGuard guard (mEeUpdateEvent);
        nfaStat = NFA_EeUpdateNow();
        if (nfaStat == NFA_STATUS_OK)
        {
            mEeUpdateEvent.wait (); //wait for NFA_EE_UPDATED_EVT
        }
    }
    return (nfaStat == NFA_STATUS_OK);
}

void RoutingManager::onNfccShutdown ()
{
    static const char fn [] = "RoutingManager:onNfccShutdown";
    if (mActiveSe == 0x00) return;

    tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
    UINT8 actualNumEe = MAX_NUM_EE;
    tNFA_EE_INFO eeInfo[MAX_NUM_EE];

    memset (&eeInfo, 0, sizeof(eeInfo));
    if ((nfaStat = NFA_EeGetInfo (&actualNumEe, eeInfo)) != NFA_STATUS_OK)
    {
        ALOGE ("%s: fail get info; error=0x%X", fn, nfaStat);
        return;
    }
    if (actualNumEe != 0)
    {
        for (UINT8 xx = 0; xx < actualNumEe; xx++)
        {
            if ((eeInfo[xx].num_interface != 0)
                && (eeInfo[xx].ee_interface[0] != NCI_NFCEE_INTERFACE_HCI_ACCESS)
                && (eeInfo[xx].ee_status == NFA_EE_STATUS_ACTIVE))
            {
                ALOGD ("%s: Handle: 0x%04x Change Status Active to Inactive", fn, eeInfo[xx].ee_handle);
                if ((nfaStat = SecureElement::getInstance().SecElem_EeModeSet (eeInfo[xx].ee_handle, NFA_EE_MD_DEACTIVATE)) != NFA_STATUS_OK)
                {
                    ALOGE ("Failed to set EE inactive");
                }
            }
        }
    }
    else
    {
        ALOGD ("%s: No active EEs found", fn);
    }
}

void RoutingManager::notifyActivated ()
{
    JNIEnv* e = NULL;
    ScopedAttach attach(mNativeData->vm, &e);
    if (e == NULL)
    {
        ALOGE ("jni env is null");
        return;
    }

    e->CallVoidMethod (mNativeData->manager, android::gCachedNfcManagerNotifyHostEmuActivated);
    if (e->ExceptionCheck())
    {
        e->ExceptionClear();
        ALOGE ("fail notify");
    }
}

void RoutingManager::notifyDeactivated ()
{
    SecureElement::getInstance().notifyListenModeState (false);
    mRxDataBuffer.clear();
    JNIEnv* e = NULL;
    ScopedAttach attach(mNativeData->vm, &e);
    if (e == NULL)
    {
        ALOGE ("jni env is null");
        return;
    }

    e->CallVoidMethod (mNativeData->manager, android::gCachedNfcManagerNotifyHostEmuDeactivated);
    if (e->ExceptionCheck())
    {
        e->ExceptionClear();
        ALOGE ("fail notify");
    }
}

void RoutingManager::notifyLmrtFull ()
{
    JNIEnv* e = NULL;
    ScopedAttach attach(mNativeData->vm, &e);
    if (e == NULL)
    {
        ALOGE ("jni env is null");
        return;
    }

    e->CallVoidMethod (mNativeData->manager, android::gCachedNfcManagerNotifyAidRoutingTableFull);
    if (e->ExceptionCheck())
    {
        e->ExceptionClear();
        ALOGE ("fail notify");
    }
}

void RoutingManager::handleData (const UINT8* data, UINT32 dataLen, tNFA_STATUS status)
{
    if (dataLen <= 0)
    {
        ALOGE("no data");
        goto TheEnd;
    }

    if (status == NFA_STATUS_CONTINUE)
    {
        ALOGE ("jni env is null");
        mRxDataBuffer.insert (mRxDataBuffer.end(), &data[0], &data[dataLen]); //append data; more to come
        return; //expect another NFA_CE_DATA_EVT to come
    }
    else if (status == NFA_STATUS_OK)
    {
        mRxDataBuffer.insert (mRxDataBuffer.end(), &data[0], &data[dataLen]); //append data
        //entire data packet has been received; no more NFA_CE_DATA_EVT
    }
    else if (status == NFA_STATUS_FAILED)
    {
        ALOGE("RoutingManager::handleData: read data fail");
        goto TheEnd;
    }
    {
        JNIEnv* e = NULL;
        ScopedAttach attach(mNativeData->vm, &e);
        if (e == NULL)
        {
            ALOGE ("jni env is null");
            goto TheEnd;
        }

        ScopedLocalRef<jobject> dataJavaArray(e, e->NewByteArray(mRxDataBuffer.size()));
        if (dataJavaArray.get() == NULL)
        {
            ALOGE ("fail allocate array");
            goto TheEnd;
        }

        e->SetByteArrayRegion ((jbyteArray)dataJavaArray.get(), 0, mRxDataBuffer.size(),
                (jbyte *)(&mRxDataBuffer[0]));
        if (e->ExceptionCheck())
        {
            e->ExceptionClear();
            ALOGE ("fail fill array");
            goto TheEnd;
        }

        e->CallVoidMethod (mNativeData->manager, android::gCachedNfcManagerNotifyHostEmuData, dataJavaArray.get());
        if (e->ExceptionCheck())
        {
            e->ExceptionClear();
            ALOGE ("fail notify");
        }

    }
TheEnd:
    mRxDataBuffer.clear();
}

void RoutingManager::stackCallback (UINT8 event, tNFA_CONN_EVT_DATA* eventData)
{
    static const char fn [] = "RoutingManager::stackCallback";
    ALOGD("%s: event=0x%X", fn, event);
    RoutingManager& routingManager = RoutingManager::getInstance();
    SecureElement& se = SecureElement::getInstance();

    switch (event)
    {
    case NFA_CE_REGISTERED_EVT:
        {
            tNFA_CE_REGISTERED& ce_registered = eventData->ce_registered;
            ALOGD("%s: NFA_CE_REGISTERED_EVT; status=0x%X; h=0x%X", fn, ce_registered.status, ce_registered.handle);
            SyncEventGuard guard (routingManager.mCeRegisterEvent);
            if(ce_registered.status == NFA_STATUS_OK)
            {
                lastcehandle = ce_registered.handle;
            }
            else
            {
                lastcehandle = 0xFF;
            }
            routingManager.mCeRegisterEvent.notifyOne();
        }
        break;

    case NFA_CE_DEREGISTERED_EVT:
        {
            tNFA_CE_DEREGISTERED& ce_deregistered = eventData->ce_deregistered;
            ALOGD("%s: NFA_CE_DEREGISTERED_EVT; h=0x%X", fn, ce_deregistered.handle);
            SyncEventGuard guard (routingManager.mCeDeRegisterEvent);
            routingManager.mCeDeRegisterEvent.notifyOne();
        }
        break;

    case NFA_CE_ACTIVATED_EVT:
        {
            android::checkforTranscation(NFA_CE_ACTIVATED_EVT, (void *)eventData);
            routingManager.notifyActivated();
        }
        break;
    case NFA_DEACTIVATED_EVT:
    case NFA_CE_DEACTIVATED_EVT:
        {
            android::checkforTranscation(NFA_CE_DEACTIVATED_EVT, (void *)eventData);
            routingManager.notifyDeactivated();
        }
        break;
    case NFA_CE_DATA_EVT:
        {
#if (NFC_NXP_ESE == TRUE && NFC_NXP_CHIP_TYPE != PN547C2)
            se.mRecvdTransEvt = true;
            se.mAllowWiredMode = true;
            SyncEventGuard guard (se.mAllowWiredModeEvent);
            se.mAllowWiredModeEvent.notifyOne();
#endif
            tNFA_CE_DATA& ce_data = eventData->ce_data;
            ALOGD("%s: NFA_CE_DATA_EVT; stat=0x%X; h=0x%X; data len=%u", fn, ce_data.status, ce_data.handle, ce_data.len);
            getInstance().handleData(ce_data.p_data, ce_data.len, ce_data.status);
        }
        break;
    }
}
/*******************************************************************************
**
** Function:        nfaEeCallback
**
** Description:     Receive execution environment-related events from stack.
**                  event: Event code.
**                  eventData: Event data.
**
** Returns:         None
**
*******************************************************************************/
void RoutingManager::nfaEeCallback (tNFA_EE_EVT event, tNFA_EE_CBACK_DATA* eventData)
{
    static const char fn [] = "RoutingManager::nfaEeCallback";

    SecureElement& se = SecureElement::getInstance();
    RoutingManager& routingManager = RoutingManager::getInstance();
    tNFA_EE_DISCOVER_REQ info = eventData->discover_req;

    switch (event)
    {
    case NFA_EE_REGISTER_EVT:
        {
            SyncEventGuard guard (routingManager.mEeRegisterEvent);
            ALOGD ("%s: NFA_EE_REGISTER_EVT; status=%u", fn, eventData->ee_register);
            routingManager.mEeRegisterEvent.notifyOne();
        }
        break;

    case NFA_EE_MODE_SET_EVT:
        {
            SyncEventGuard guard (routingManager.mEeSetModeEvent);
            ALOGD ("%s: NFA_EE_MODE_SET_EVT; status: 0x%04X  handle: 0x%04X  mActiveEeHandle: 0x%04X", fn,
                    eventData->mode_set.status, eventData->mode_set.ee_handle, se.mActiveEeHandle);
            routingManager.mEeSetModeEvent.notifyOne();
            se.notifyModeSet(eventData->mode_set.ee_handle, !(eventData->mode_set.status),eventData->mode_set.ee_status );
        }
        break;

    case NFA_EE_SET_TECH_CFG_EVT:
        {
            ALOGD ("%s: NFA_EE_SET_TECH_CFG_EVT; status=0x%X", fn, eventData->status);
            SyncEventGuard guard(routingManager.mRoutingEvent);
            routingManager.mRoutingEvent.notifyOne();
        }
        break;

    case NFA_EE_SET_PROTO_CFG_EVT:
        {
            ALOGD ("%s: NFA_EE_SET_PROTO_CFG_EVT; status=0x%X", fn, eventData->status);
            SyncEventGuard guard(routingManager.mRoutingEvent);
            routingManager.mRoutingEvent.notifyOne();
        }
        break;

    case NFA_EE_ACTION_EVT:
        {
            tNFA_EE_ACTION& action = eventData->action;
            tNFC_APP_INIT& app_init = action.param.app_init;
            android::checkforTranscation(NFA_EE_ACTION_EVT, (void *)eventData);
#if (NFC_NXP_ESE == TRUE && NFC_NXP_CHIP_TYPE != PN547C2)
            se.mRecvdTransEvt = true;
#endif
            if (action.trigger == NFC_EE_TRIG_SELECT)
            {
                ALOGD ("%s: NFA_EE_ACTION_EVT; h=0x%X; trigger=select (0x%X); aid len=%u", fn, action.ee_handle, action.trigger, app_init.len_aid);
                se.notifyTransactionListenersOfAid (app_init.aid, app_init.len_aid, NULL, 0, SecureElement::getInstance().getGenericEseId(action.ee_handle & ~NFA_HANDLE_GROUP_EE));
            }
            else if (action.trigger == NFC_EE_TRIG_APP_INIT)
            {
                ALOGD ("%s: NFA_EE_ACTION_EVT; h=0x%X; trigger=app-init (0x%X); aid len=%u; data len=%u", fn,
                        action.ee_handle, action.trigger, app_init.len_aid, app_init.len_data);
                //if app-init operation is successful;
                //app_init.data[] contains two bytes, which are the status codes of the event;
                //app_init.data[] does not contain an APDU response;
                //see EMV Contactless Specification for Payment Systems; Book B; Entry Point Specification;
                //version 2.1; March 2011; section 3.3.3.5;
                if ( (app_init.len_data > 1) &&
                     (app_init.data[0] == 0x90) &&
                     (app_init.data[1] == 0x00) )
                {
                    se.notifyTransactionListenersOfAid (app_init.aid, app_init.len_aid, app_init.data, app_init.len_data, SecureElement::getInstance().getGenericEseId(action.ee_handle & ~NFA_HANDLE_GROUP_EE));
                }
            }
            else if (action.trigger == NFC_EE_TRIG_RF_PROTOCOL)
                ALOGD ("%s: NFA_EE_ACTION_EVT; h=0x%X; trigger=rf protocol (0x%X)", fn, action.ee_handle, action.trigger);
            else if (action.trigger == NFC_EE_TRIG_RF_TECHNOLOGY)
                ALOGD ("%s: NFA_EE_ACTION_EVT; h=0x%X; trigger=rf tech (0x%X)", fn, action.ee_handle, action.trigger);
            else
                ALOGE ("%s: NFA_EE_ACTION_EVT; h=0x%X; unknown trigger (0x%X)", fn, action.ee_handle, action.trigger);

#if((NXP_EXTNS == TRUE)&&(NFC_NXP_ESE == TRUE) && (NFC_NXP_CHIP_TYPE == PN548C2))
            if((action.ee_handle == 0x4C0))
            {
                ALOGE ("%s: NFA_EE_ACTION_EVT; h=0x%X;DWP CL activated (0x%X)", fn, action.ee_handle, action.trigger);
                se.setCLState(true);
            }
#endif

#if (NFC_NXP_ESE == TRUE && NFC_NXP_CHIP_TYPE != PN547C2)
            /*if(action.ee_handle == 0x4C0 && (action.trigger != NFC_EE_TRIG_RF_TECHNOLOGY) &&
               !(action.trigger == NFC_EE_TRIG_RF_PROTOCOL && action.param.protocol == NFA_PROTOCOL_ISO_DEP))
            {
                ALOGE("%s,Allow wired mode connection", fn);
                se.mAllowWiredMode = true;
                SyncEventGuard guard (se.mAllowWiredModeEvent);
                se.mAllowWiredModeEvent.notifyOne();
            }
            else
               se.mAllowWiredMode = false;*/
            if(action.ee_handle == 0x4C0)
            {
                if((action.trigger == NFC_EE_TRIG_RF_TECHNOLOGY)&& (gEseVirtualWiredProtectMask & 0x04))
                {
                    se.mAllowWiredMode = false;
                }
                else if((action.trigger == NFC_EE_TRIG_RF_PROTOCOL && action.param.protocol == NFA_PROTOCOL_ISO_DEP)&&(gEseVirtualWiredProtectMask & 0x02))
                {
                    se.mAllowWiredMode = false;
                }
                else
                {
                    se.mAllowWiredMode = true;
                }
            }
            if(action.ee_handle == 0x402)
            {
                if((action.trigger == NFC_EE_TRIG_RF_TECHNOLOGY)&& (gUICCVirtualWiredProtectMask & 0x04))
                {
                    se.mAllowWiredMode = false;
                }
                else if((action.trigger == NFC_EE_TRIG_RF_PROTOCOL)&&(gUICCVirtualWiredProtectMask & 0x02))
                {
                    se.mAllowWiredMode = false;
                }
                else if(((action.trigger == NFC_EE_TRIG_SELECT)||(action.trigger == NFC_EE_TRIG_APP_INIT))&&(gUICCVirtualWiredProtectMask & 0x01))
                {
                    se.mAllowWiredMode = false;
                }
                else
                {
                    se.mAllowWiredMode = true;
                }
            }
    if(se.mAllowWiredMode == true)
    {
        SyncEventGuard guard (se.mAllowWiredModeEvent);
        se.mAllowWiredModeEvent.notifyOne();
    }
#endif
        }
        break;

    case NFA_EE_DISCOVER_EVT:
        {
            UINT8 num_ee = eventData->ee_discover.num_ee;
            tNFA_EE_DISCOVER ee_disc_info = eventData->ee_discover;
            ALOGD ("%s: NFA_EE_DISCOVER_EVT; status=0x%X; num ee=%u", __FUNCTION__,eventData->status, eventData->ee_discover.num_ee);
            if(android::isNfcInitializationDone() == true)
            {
                if(mChipId == 0x02 || mChipId == 0x04)
                {
                    for(int xx = 0; xx <  num_ee; xx++)
                    {
                        ALOGE("xx=%d, ee_handle=0x0%x, status=0x0%x", xx, ee_disc_info.ee_info[xx].ee_handle,ee_disc_info.ee_info[xx].ee_status);
                        if ((ee_disc_info.ee_info[xx].ee_handle == 0x4C0) &&
                            (ee_disc_info.ee_info[xx].ee_status == 0x02))
                        {
#if(NXP_EXTNS == TRUE)
                            recovery=TRUE;
#endif
                            routingManager.ee_removed_disc_ntf_handler(ee_disc_info.ee_info[xx].ee_handle, ee_disc_info.ee_info[xx].ee_status);
                            break;
                        }
                    }
                }
            }
            gSeDiscoverycount++;
            if(gSeDiscoverycount == gActualSeCount)
            {
                SyncEventGuard g (gNfceeDiscCbEvent);
                ALOGD("%s: Sem Post for gNfceeDiscCbEvent", __FUNCTION__);
                gNfceeDiscCbEvent.notifyOne ();
            }
        }
        break;

    case NFA_EE_DISCOVER_REQ_EVT:
        ALOGD ("%s: NFA_EE_DISCOVER_REQ_EVT; status=0x%X; num ee=%u", __FUNCTION__,
                eventData->discover_req.status, eventData->discover_req.num_ee);
#if(NXP_EXTNS == TRUE)
#if(NFC_NXP_ESE == TRUE && NFC_NXP_CHIP_TYPE != PN547C2)
        /* Handle Reader over SWP.
         * 1. Check if the event is for Reader over SWP.
         * 2. IF yes than send this info(READER_REQUESTED_EVENT) till FWK level.
         * 3. Stop the discovery.
         * 4. MAP the proprietary interface for Reader over SWP.NFC_DiscoveryMap, nfc_api.h
         * 5. start the discovery with reader req, type and DH configuration.
         *
         * 6. IF yes than send this info(STOP_READER_EVENT) till FWK level.
         * 7. MAP the DH interface for Reader over SWP. NFC_DiscoveryMap, nfc_api.h
         * 8. start the discovery with DH configuration.
         */
        swp_rdr_req_ntf_info.mMutex.lock ();
        for (UINT8 xx = 0; xx < info.num_ee; xx++)
        {
            //for each technology (A, B, F, B'), print the bit field that shows
            //what protocol(s) is support by that technology
            ALOGD ("%s   EE[%u] Handle: 0x%04x  PA: 0x%02x  PB: 0x%02x",
                    fn, xx, info.ee_disc_info[xx].ee_handle,
                    info.ee_disc_info[xx].pa_protocol,
                    info.ee_disc_info[xx].pb_protocol);

            ALOGD("%s, swp_rd_state=%x", fn, swp_rdr_req_ntf_info.swp_rd_state);
            if( (info.ee_disc_info[xx].ee_req_op == NFC_EE_DISC_OP_ADD) &&
                    (swp_rdr_req_ntf_info.swp_rd_state == STATE_SE_RDR_MODE_STOPPED ||
                    swp_rdr_req_ntf_info.swp_rd_state == STATE_SE_RDR_MODE_START_CONFIG)&&
                    (info.ee_disc_info[xx].pa_protocol ==  0x04 || info.ee_disc_info[xx].pb_protocol == 0x04 ))
            {
                ALOGD ("%s NFA_RD_SWP_READER_REQUESTED  EE[%u] Handle: 0x%04x  PA: 0x%02x  PB: 0x%02x",
                        fn, xx, info.ee_disc_info[xx].ee_handle,
                        info.ee_disc_info[xx].pa_protocol,
                        info.ee_disc_info[xx].pb_protocol);

                swp_rdr_req_ntf_info.swp_rd_req_info.src = info.ee_disc_info[xx].ee_handle;
                swp_rdr_req_ntf_info.swp_rd_req_info.tech_mask = 0;
                swp_rdr_req_ntf_info.swp_rd_req_info.reCfg = false;

                if( !(swp_rdr_req_ntf_info.swp_rd_req_info.tech_mask & NFA_TECHNOLOGY_MASK_A) )
                {
                    if(info.ee_disc_info[xx].pa_protocol ==  0x04)
                    {
                        swp_rdr_req_ntf_info.swp_rd_req_info.tech_mask |= NFA_TECHNOLOGY_MASK_A;
                        swp_rdr_req_ntf_info.swp_rd_req_info.reCfg = true;
                    }
                }

                if( !(swp_rdr_req_ntf_info.swp_rd_req_info.tech_mask & NFA_TECHNOLOGY_MASK_B) )
                {
                    if(info.ee_disc_info[xx].pb_protocol ==  0x04)
                    {
                        swp_rdr_req_ntf_info.swp_rd_req_info.tech_mask |= NFA_TECHNOLOGY_MASK_B;
                        swp_rdr_req_ntf_info.swp_rd_req_info.reCfg = true;
                    }
                }

                if(swp_rdr_req_ntf_info.swp_rd_req_info.reCfg)
                {
                    ALOGD("%s, swp_rd_state=%x  evt : NFA_RD_SWP_READER_REQUESTED swp_rd_req_timer start", fn, swp_rdr_req_ntf_info.swp_rd_state);
                    swp_rdr_req_ntf_info.swp_rd_state = STATE_SE_RDR_MODE_START_CONFIG;
                    swp_rd_req_timer.kill();
                    swp_rd_req_timer.set (rdr_req_handling_timeout, reader_req_event_ntf);
                    swp_rdr_req_ntf_info.swp_rd_req_info.reCfg = false;
                }
                //Reader over SWP - Reader Requested.
                //se.handleEEReaderEvent(NFA_RD_SWP_READER_REQUESTED, tech, info.ee_disc_info[xx].ee_handle);
                break;
            }
            else if((info.ee_disc_info[xx].ee_req_op == NFC_EE_DISC_OP_REMOVE) &&
                    ((swp_rdr_req_ntf_info.swp_rd_state == STATE_SE_RDR_MODE_STARTED) ||
                    (swp_rdr_req_ntf_info.swp_rd_state == STATE_SE_RDR_MODE_START_CONFIG) ||
                    (swp_rdr_req_ntf_info.swp_rd_state == STATE_SE_RDR_MODE_STOP_CONFIG) ||
                    (swp_rdr_req_ntf_info.swp_rd_state == STATE_SE_RDR_MODE_ACTIVATED)) &&
                    (info.ee_disc_info[xx].pa_protocol ==  0xFF || info.ee_disc_info[xx].pb_protocol == 0xFF))
            {
                ALOGD ("%s NFA_RD_SWP_READER_STOP  EE[%u] Handle: 0x%04x  PA: 0x%02x  PB: 0x%02x",
                        fn, xx, info.ee_disc_info[xx].ee_handle,
                        info.ee_disc_info[xx].pa_protocol,
                        info.ee_disc_info[xx].pb_protocol);

                if(swp_rdr_req_ntf_info.swp_rd_req_info.src == info.ee_disc_info[xx].ee_handle)
                {
                     if(info.ee_disc_info[xx].pa_protocol ==  0xFF)
                     {
                         if(swp_rdr_req_ntf_info.swp_rd_req_info.tech_mask & NFA_TECHNOLOGY_MASK_A)
                         {
                             swp_rdr_req_ntf_info.swp_rd_req_info.tech_mask &= ~NFA_TECHNOLOGY_MASK_A;
                             swp_rdr_req_ntf_info.swp_rd_req_info.reCfg = true;
                             //swp_rdr_req_ntf_info.swp_rd_state = STATE_SE_RDR_MODE_STOP_CONFIG;
                         }
                     }

                     if(info.ee_disc_info[xx].pb_protocol ==  0xFF)
                     {
                         if(swp_rdr_req_ntf_info.swp_rd_req_info.tech_mask & NFA_TECHNOLOGY_MASK_B)
                         {
                             swp_rdr_req_ntf_info.swp_rd_req_info.tech_mask &= ~NFA_TECHNOLOGY_MASK_B;
                             swp_rdr_req_ntf_info.swp_rd_req_info.reCfg = true;
                         }

                     }

                     if(swp_rdr_req_ntf_info.swp_rd_req_info.reCfg)
                     {
                         ALOGD("%s, swp_rd_state=%x  evt : NFA_RD_SWP_READER_STOP swp_rd_req_timer start", fn, swp_rdr_req_ntf_info.swp_rd_state);
                         swp_rdr_req_ntf_info.swp_rd_state = STATE_SE_RDR_MODE_STOP_CONFIG;
                         swp_rd_req_timer.kill();
                         swp_rd_req_timer.set (rdr_req_handling_timeout, reader_req_event_ntf);
                         swp_rdr_req_ntf_info.swp_rd_req_info.reCfg = false;
                     }
                }
                break;
            }
        }
        swp_rdr_req_ntf_info.mMutex.unlock();
        /*Set the configuration for UICC/ESE */
        se.storeUiccInfo (eventData->discover_req);
#endif
#endif
        break;

    case NFA_EE_NO_CB_ERR_EVT:
        ALOGD ("%s: NFA_EE_NO_CB_ERR_EVT  status=%u", fn, eventData->status);
        break;

    case NFA_EE_ADD_AID_EVT:
        {
            ALOGD ("%s: NFA_EE_ADD_AID_EVT  status=%u", fn, eventData->status);
            if(eventData->status == NFA_STATUS_BUFFER_FULL)
            {
                ALOGD ("%s: AID routing table is FULL!!!", fn);
                RoutingManager::getInstance().notifyLmrtFull();
            }
            SyncEventGuard guard(se.mAidAddRemoveEvent);
            se.mAidAddRemoveEvent.notifyOne();
        }
        break;

    case NFA_EE_REMOVE_AID_EVT:
        {
            ALOGD ("%s: NFA_EE_REMOVE_AID_EVT  status=%u", fn, eventData->status);
            SyncEventGuard guard(se.mAidAddRemoveEvent);
            se.mAidAddRemoveEvent.notifyOne();
        }
        break;

    case NFA_EE_NEW_EE_EVT:
        {
            ALOGD ("%s: NFA_EE_NEW_EE_EVT  h=0x%X; status=%u", fn,
                eventData->new_ee.ee_handle, eventData->new_ee.ee_status);
        }
        break;
    case NFA_EE_ROUT_ERR_EVT:
        {
            ALOGD ("%s: NFA_EE_ROUT_ERR_EVT  status=%u", fn,eventData->status);
        }
        break;
    case NFA_EE_UPDATED_EVT:
        {
            ALOGD("%s: NFA_EE_UPDATED_EVT", fn);
            SyncEventGuard guard(routingManager.mEeUpdateEvent);
            routingManager.mEeUpdateEvent.notifyOne();
        }
        break;
    default:
        ALOGE ("%s: unknown event=%u ????", fn, event);
        break;
    }
}

int RoutingManager::registerJniFunctions (JNIEnv* e)
{
    static const char fn [] = "RoutingManager::registerJniFunctions";
    ALOGD ("%s", fn);
    return jniRegisterNativeMethods (e, "com/android/nfc/cardemulation/AidRoutingManager", sMethods, NELEM(sMethods));
}

int RoutingManager::com_android_nfc_cardemulation_doGetDefaultRouteDestination (JNIEnv*)
{
    return getInstance().mDefaultEe;
}

int RoutingManager::com_android_nfc_cardemulation_doGetDefaultOffHostRouteDestination (JNIEnv*)
{
    return getInstance().mOffHostEe;
}

int RoutingManager::com_android_nfc_cardemulation_doGetAidMatchingMode (JNIEnv*)
{
    return getInstance().mAidMatchingMode;
}


int RoutingManager::com_android_nfc_cardemulation_doGetAidMatchingPlatform(JNIEnv*)
{
    return getInstance().mAidMatchingPlatform;
}

/*
*This fn gets called when timer gets expired.
*When reader requested events (add for polling tech - tech A/tech B)comes it is expected to come back to back with in timer expiry value(50ms)
*case 1:If all the add request comes before the timer expiry , poll request for all isn handled
*case 2:If the second add request comes after timer expiry, it is not handled

*When reader requested events (remove polling tech - tech A/tech B)comes it is expected to come back to back for the add requestes before
 timer expiry happens(50ms)
*case 1:If all the removal request comes before the timer expiry , poll removal  request for all is handled
*case 2:If the only one of the removal request is reached before timer expiry, it is not handled
           :When ever the second removal request is also reached , it is handled.

*/
#if(NFC_NXP_ESE == TRUE && NFC_NXP_CHIP_TYPE == PN548C2)
void reader_req_event_ntf (union sigval)
{
    static const char fn [] = "RoutingManager::reader_req_event_ntf";
    ALOGD ("%s:  ", fn);
    JNIEnv* e = NULL;
    int disc_ntf_timeout = 10;
    ScopedAttach attach(RoutingManager::getInstance().mNativeData->vm, &e);
    if (e == NULL)
    {
        ALOGE ("%s: jni env is null", fn);
        return;
    }

    GetNumValue ( NAME_NFA_DM_DISC_NTF_TIMEOUT, &disc_ntf_timeout, sizeof ( disc_ntf_timeout ) );

    Rdr_req_ntf_info_t mSwp_info = RoutingManager::getInstance().getSwpRrdReqInfo();

    ALOGD ("%s: swp_rdr_req_ntf_info.swp_rd_req_info.src = 0x%4x ", fn,mSwp_info.swp_rd_req_info.src);

    if(RoutingManager::getInstance().getEtsiReaederState() == STATE_SE_RDR_MODE_START_CONFIG)
    {
        e->CallVoidMethod (RoutingManager::getInstance().mNativeData->manager, android::gCachedNfcManagerNotifyETSIReaderModeStartConfig, (UINT16)mSwp_info.swp_rd_req_info.src);
    }
    else if(RoutingManager::getInstance().getEtsiReaederState() == STATE_SE_RDR_MODE_STOP_CONFIG)
    {
        ALOGD ("%s: sSwpReaderTimer.kill() ", fn);
        SecureElement::getInstance().sSwpReaderTimer.kill();
        e->CallVoidMethod (RoutingManager::getInstance().mNativeData->manager, android::gCachedNfcManagerNotifyETSIReaderModeStopConfig,disc_ntf_timeout);
    }
}
#endif
void *ee_removed_ntf_handler_thread(void *data)
{
    static const char fn [] = "ee_removed_ntf_handler_thread";
    tNFA_STATUS stat = NFA_STATUS_FAILED;
    SecureElement &se = SecureElement::getInstance();
    RoutingManager &rm = RoutingManager::getInstance();
    ALOGD ("%s: Enter: ", fn);
    rm.mResetHandlerMutex.lock();
    ALOGD ("%s: enter sEseRemovedHandlerMutex lock", fn);
    stat = NFA_EeModeSet(0x4c0, NFA_EE_MD_DEACTIVATE);

    if(stat == NFA_STATUS_OK)
    {
        SyncEventGuard guard (se.mEeSetModeEvent);
        se.mEeSetModeEvent.wait ();
    }
#if((NFC_NXP_ESE == TRUE)&&(NXP_EXTNS == TRUE))
    se.NfccStandByOperation(STANDBY_GPIO_LOW);
    usleep(10*1000);
    se.NfccStandByOperation(STANDBY_GPIO_HIGH);
#endif
    stat = NFA_EeModeSet(0x4c0, NFA_EE_MD_ACTIVATE);

    if(stat == NFA_STATUS_OK)
    {
        SyncEventGuard guard(se.mEeSetModeEvent);
        se.mEeSetModeEvent.wait ();
    }
    NFA_HciW4eSETransaction_Complete(Release);
    SyncEventGuard guard(se.mEEdatapacketEvent);
    recovery=FALSE;
    se.mEEdatapacketEvent.notifyOne();
    rm.mResetHandlerMutex.unlock();
    ALOGD ("%s: exit sEseRemovedHandlerMutex lock ", fn);
    ALOGD ("%s: exit ", fn);
    return NULL;
}

void RoutingManager::ee_removed_disc_ntf_handler(tNFA_HANDLE handle, tNFA_EE_STATUS status)
{
    static const char fn [] = "RoutingManager::ee_disc_ntf_handler";
    ALOGD("%s; ee_handle=0x0%x, status=0x0%x", fn, handle, status);
    int ret = -1;
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&thread, &attr, &ee_removed_ntf_handler_thread, (void*)NULL);
    pthread_attr_destroy(&attr);
    if (ret != 0)
    {
        ALOGE("Unable to create the thread");
    }
}
#if(NFC_NXP_ESE == TRUE && NFC_NXP_CHIP_TYPE == PN548C2)
/*******************************************************************************
**
** Function:        getEtsiReaederState
**
** Description:     Get the current ETSI Reader state
**
** Returns:         Current ETSI state
**
*******************************************************************************/
se_rd_req_state_t RoutingManager::getEtsiReaederState()
{
    return swp_rdr_req_ntf_info.swp_rd_state;
}

/*******************************************************************************
**
** Function:        setEtsiReaederState
**
** Description:     Set the current ETSI Reader state
**
** Returns:         None
**
*******************************************************************************/
void RoutingManager::setEtsiReaederState(se_rd_req_state_t newState)
{
    swp_rdr_req_ntf_info.mMutex.lock();
    if(newState == STATE_SE_RDR_MODE_STOPPED)
    {
        swp_rdr_req_ntf_info.swp_rd_req_current_info.tech_mask &= ~NFA_TECHNOLOGY_MASK_A;
        swp_rdr_req_ntf_info.swp_rd_req_current_info.tech_mask &= ~NFA_TECHNOLOGY_MASK_B;

        //If all the requested tech are removed, set the hande to invalid , so that next time poll add request can be handled

        swp_rdr_req_ntf_info.swp_rd_req_current_info.src = NFA_HANDLE_INVALID;
        swp_rdr_req_ntf_info.swp_rd_req_info = swp_rdr_req_ntf_info.swp_rd_req_current_info;
    }
    swp_rdr_req_ntf_info.swp_rd_state = newState;
    swp_rdr_req_ntf_info.mMutex.unlock();
}

/*******************************************************************************
**
** Function:        getSwpRrdReqInfo
**
** Description:     get swp_rdr_req_ntf_info
**
** Returns:         swp_rdr_req_ntf_info
**
*******************************************************************************/
Rdr_req_ntf_info_t RoutingManager::getSwpRrdReqInfo()
{
    ALOGE("%s Enter",__FUNCTION__);
    return swp_rdr_req_ntf_info;
}
#endif

#if(NXP_EXTNS == TRUE)
bool RoutingManager::is_ee_recovery_ongoing()
{
    ALOGD("is_ee_recovery_ongoing : recovery");
    if(recovery)
        return true;
    else
        return false;
}

/*******************************************************************************
**
** Function:        getRouting
**
** Description:     Send GET_LISTEN_MODE_ROUTING command
**
** Returns:         None
**
*******************************************************************************/
void RoutingManager::getRouting()
{
    tNFA_STATUS nfcStat;
    nfcStat = NFC_GetRouting();
    if(nfcStat == NFA_STATUS_OK)
    {
        ALOGE ("getRouting failed. status=0x0%x", nfcStat);
    }
}

/*******************************************************************************
**
** Function:        processGetRouting
**
** Description:     Process the eventData(current routing info) received during
**                  getRouting
**                  eventData : eventData
**                  sRoutingBuff : Array containing processed data
**
** Returns:         None
**
*******************************************************************************/
void RoutingManager::processGetRoutingRsp(tNFA_DM_CBACK_DATA* eventData, UINT8* sRoutingBuff)
{
    ALOGD ("%s : Enter", __FUNCTION__);
    UINT8 xx=0,numTLVs = 0,currPos = 0,curTLVLen = 0;
    UINT8 sRoutingCurrent[256];
    numTLVs = *(eventData->get_routing.param_tlvs+1);
    /*Copying only routing Entries.
    Skipping fields,
    More                  : 1Byte
    No of Routing Entries : 1Byte*/
    memcpy(sRoutingCurrent,eventData->get_routing.param_tlvs+2,eventData->get_routing.tlv_size-2);

    while(xx < numTLVs)
    {
        curTLVLen = *(sRoutingCurrent+currPos+1);
        /*Filtering out Routing Entry corresponding to PROTOCOL_NFC_DEP*/
        if((*(sRoutingCurrent+currPos) == PROTOCOL_BASED_ROUTING)&&(*(sRoutingCurrent+currPos+(curTLVLen+1))==NFA_PROTOCOL_NFC_DEP))
        {
            currPos = currPos + curTLVLen+TYPE_LENGTH_SIZE;
        }
        else
        {
            memcpy(sRoutingBuff+android::sRoutingBuffLen,sRoutingCurrent+currPos,curTLVLen+TYPE_LENGTH_SIZE);
            currPos = currPos + curTLVLen+TYPE_LENGTH_SIZE;
            android::sRoutingBuffLen = android::sRoutingBuffLen + curTLVLen+TYPE_LENGTH_SIZE;
        }
        xx++;
    }
}
/*******************************************************************************
**
** Function:        handleSERemovedNtf()
**
** Description:     The Function checks whether eSE is Removed Ntf
**
** Returns:         None
**
*******************************************************************************/
void RoutingManager::handleSERemovedNtf()
{
    static const char fn [] = "RoutingManager::handleSERemovedNtf()";
    UINT8 mActualNumEe = SecureElement::MAX_NUM_EE;
    tNFA_EE_INFO mEeInfo [mActualNumEe];
    tNFA_STATUS nfaStat;
    ALOGE ("%s:Enter", __FUNCTION__);
    if ((nfaStat = NFA_AllEeGetInfo (&mActualNumEe, mEeInfo)) != NFA_STATUS_OK)
    {
        ALOGE ("%s: fail get info; error=0x%X", fn, nfaStat);
        mActualNumEe = 0;
    }
    else
    {
        if(mChipId == 0x02 || mChipId == 0x04)
        {
            for(int xx = 0; xx <  mActualNumEe; xx++)
            {
               ALOGE("xx=%d, ee_handle=0x0%x, status=0x0%x", xx, mEeInfo[xx].ee_handle,mEeInfo[xx].ee_status);
                if ((mEeInfo[xx].ee_handle == 0x4C0) &&
                    (mEeInfo[xx].ee_status == 0x02))
                {
                    recovery = TRUE;
                    ee_removed_disc_ntf_handler(mEeInfo[xx].ee_handle, mEeInfo[xx].ee_status);
                    break;
                }
            }
        }
    }
}
#endif
