/* Copyright (c) 2012-2019, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_Agps"

#include <Agps.h>
#include <loc_pla.h>
#include <ContextBase.h>
#include <loc_timer.h>
#include <inttypes.h>

/* --------------------------------------------------------------------
 *   AGPS State Machine Methods
 * -------------------------------------------------------------------*/
void AgpsStateMachine::processAgpsEvent(AgpsEvent event){

    LOC_LOGD("processAgpsEvent(): SM %p, Event %d, State %d",
               this, event, mState);

    switch (event) {

        case AGPS_EVENT_SUBSCRIBE:
            processAgpsEventSubscribe();
            break;

        case AGPS_EVENT_UNSUBSCRIBE:
            processAgpsEventUnsubscribe();
            break;

        case AGPS_EVENT_GRANTED:
            processAgpsEventGranted();
            break;

        case AGPS_EVENT_RELEASED:
            processAgpsEventReleased();
            break;

        case AGPS_EVENT_DENIED:
            processAgpsEventDenied();
            break;

        default:
            LOC_LOGE("Invalid Loc Agps Event");
    }
}

void AgpsStateMachine::processAgpsEventSubscribe(){

    switch (mState) {

        case AGPS_STATE_RELEASED:
            /* Add subscriber to list
             * No notifications until we get RSRC_GRANTED */
            addSubscriber(mCurrentSubscriber);
            requestOrReleaseDataConn(true);
            transitionState(AGPS_STATE_PENDING);
            break;

        case AGPS_STATE_PENDING:
            /* Already requested for data connection,
             * do nothing until we get RSRC_GRANTED event;
             * Just add this subscriber to the list, for notifications */
            addSubscriber(mCurrentSubscriber);
            break;

        case AGPS_STATE_ACQUIRED:
            /* We already have the data connection setup,
             * Notify current subscriber with GRANTED event,
             * And add it to the subscriber list for further notifications. */
            notifyEventToSubscriber(AGPS_EVENT_GRANTED, mCurrentSubscriber, false);
            addSubscriber(mCurrentSubscriber);
            break;

        case AGPS_STATE_RELEASING:
            addSubscriber(mCurrentSubscriber);
            break;

        default:
            LOC_LOGE("Invalid state: %d", mState);
    }
}

void AgpsStateMachine::processAgpsEventUnsubscribe(){

    switch (mState) {

        case AGPS_STATE_RELEASED:
            notifyEventToSubscriber(
                    AGPS_EVENT_UNSUBSCRIBE, mCurrentSubscriber, false);
            break;

        case AGPS_STATE_PENDING:
        case AGPS_STATE_ACQUIRED:
            /* If the subscriber wishes to wait for connection close,
             * before being removed from list, move to inactive state
             * and notify */
            if (mCurrentSubscriber->mWaitForCloseComplete) {
                mCurrentSubscriber->mIsInactive = true;
            }
            else {
                /* Notify only current subscriber and then delete it from
                 * subscriberList */
                notifyEventToSubscriber(
                        AGPS_EVENT_UNSUBSCRIBE, mCurrentSubscriber, true);
            }

            /* If no subscribers in list, release data connection */
            if (mSubscriberList.empty()) {
                transitionState(AGPS_STATE_RELEASED);
                requestOrReleaseDataConn(false);
            }
            /* Some subscribers in list, but all inactive;
             * Release data connection */
            else if(!anyActiveSubscribers()) {
                transitionState(AGPS_STATE_RELEASING);
                requestOrReleaseDataConn(false);
            }
            break;

        case AGPS_STATE_RELEASING:
            /* If the subscriber wishes to wait for connection close,
             * before being removed from list, move to inactive state
             * and notify */
            if (mCurrentSubscriber->mWaitForCloseComplete) {
                mCurrentSubscriber->mIsInactive = true;
            }
            else {
                /* Notify only current subscriber and then delete it from
                 * subscriberList */
                notifyEventToSubscriber(
                        AGPS_EVENT_UNSUBSCRIBE, mCurrentSubscriber, true);
            }

            /* If no subscribers in list, just move the state.
             * Request for releasing data connection should already have been
             * sent */
            if (mSubscriberList.empty()) {
                transitionState(AGPS_STATE_RELEASED);
            }
            break;

        default:
            LOC_LOGE("Invalid state: %d", mState);
    }
}

void AgpsStateMachine::processAgpsEventGranted(){

    switch (mState) {

        case AGPS_STATE_RELEASED:
        case AGPS_STATE_ACQUIRED:
        case AGPS_STATE_RELEASING:
            LOC_LOGE("Unexpected event GRANTED in state %d", mState);
            break;

        case AGPS_STATE_PENDING:
            // Move to acquired state
            transitionState(AGPS_STATE_ACQUIRED);
            notifyAllSubscribers(
                    AGPS_EVENT_GRANTED, false,
                    AGPS_NOTIFICATION_TYPE_FOR_ACTIVE_SUBSCRIBERS);
            break;

        default:
            LOC_LOGE("Invalid state: %d", mState);
    }
}

void AgpsStateMachine::processAgpsEventReleased(){

    switch (mState) {

        case AGPS_STATE_RELEASED:
            /* Subscriber list should be empty if we are in released state */
            if (!mSubscriberList.empty()) {
                LOC_LOGE("Unexpected event RELEASED in RELEASED state");
            }
            break;

        case AGPS_STATE_ACQUIRED:
            /* Force release received */
            LOC_LOGW("Force RELEASED event in ACQUIRED state");
            transitionState(AGPS_STATE_RELEASED);
            notifyAllSubscribers(
                    AGPS_EVENT_RELEASED, true,
                    AGPS_NOTIFICATION_TYPE_FOR_ALL_SUBSCRIBERS);
            break;

        case AGPS_STATE_RELEASING:
            /* Notify all inactive subscribers about the event */
            notifyAllSubscribers(
                    AGPS_EVENT_RELEASED, true,
                    AGPS_NOTIFICATION_TYPE_FOR_INACTIVE_SUBSCRIBERS);

            /* If we have active subscribers now, they must be waiting for
             * data conn setup */
            if (anyActiveSubscribers()) {
                transitionState(AGPS_STATE_PENDING);
                requestOrReleaseDataConn(true);
            }
            /* No active subscribers, move to released state */
            else {
                transitionState(AGPS_STATE_RELEASED);
            }
            break;

        case AGPS_STATE_PENDING:
            /* NOOP */
            break;

        default:
            LOC_LOGE("Invalid state: %d", mState);
    }
}

void AgpsStateMachine::processAgpsEventDenied(){

    switch (mState) {

        case AGPS_STATE_RELEASED:
            LOC_LOGE("Unexpected event DENIED in state %d", mState);
            break;

        case AGPS_STATE_ACQUIRED:
            /* NOOP */
            break;

        case AGPS_STATE_RELEASING:
            /* Notify all inactive subscribers about the event */
            notifyAllSubscribers(
                    AGPS_EVENT_RELEASED, true,
                    AGPS_NOTIFICATION_TYPE_FOR_INACTIVE_SUBSCRIBERS);

            /* If we have active subscribers now, they must be waiting for
             * data conn setup */
            if (anyActiveSubscribers()) {
                transitionState(AGPS_STATE_PENDING);
                requestOrReleaseDataConn(true);
            }
            /* No active subscribers, move to released state */
            else {
                transitionState(AGPS_STATE_RELEASED);
            }
            break;

        case AGPS_STATE_PENDING:
            transitionState(AGPS_STATE_RELEASED);
            notifyAllSubscribers(
                    AGPS_EVENT_DENIED, true,
                    AGPS_NOTIFICATION_TYPE_FOR_ALL_SUBSCRIBERS);
            break;

        default:
            LOC_LOGE("Invalid state: %d", mState);
    }
}

/* Request or Release data connection
 * bool request :
 *      true  = Request data connection
 *      false = Release data connection */
void AgpsStateMachine::requestOrReleaseDataConn(bool request){

    AGnssExtStatusIpV4 nifRequest;
    memset(&nifRequest, 0, sizeof(nifRequest));

    nifRequest.type = mAgpsType;
    nifRequest.apnTypeMask = mApnTypeMask;
    if (request) {
        LOC_LOGD("AGPS Data Conn Request mAgpsType=%d mApnTypeMask=0x%X",
                 mAgpsType, mApnTypeMask);
        nifRequest.status = LOC_GPS_REQUEST_AGPS_DATA_CONN;
    }
    else{
        LOC_LOGD("AGPS Data Conn Release mAgpsType=%d mApnTypeMask=0x%X",
                 mAgpsType, mApnTypeMask);
        nifRequest.status = LOC_GPS_RELEASE_AGPS_DATA_CONN;
    }

    mFrameworkStatusV4Cb(nifRequest);
}

void AgpsStateMachine::notifyAllSubscribers(
        AgpsEvent event, bool deleteSubscriberPostNotify,
        AgpsNotificationType notificationType){

    LOC_LOGD("notifyAllSubscribers(): "
            "SM %p, Event %d Delete %d Notification Type %d",
            this, event, deleteSubscriberPostNotify, notificationType);

    std::list<AgpsSubscriber*>::const_iterator it = mSubscriberList.begin();
    while ( it != mSubscriberList.end() ) {

        AgpsSubscriber* subscriber = *it;

        if (notificationType == AGPS_NOTIFICATION_TYPE_FOR_ALL_SUBSCRIBERS ||
                (notificationType == AGPS_NOTIFICATION_TYPE_FOR_INACTIVE_SUBSCRIBERS &&
                        subscriber->mIsInactive) ||
                (notificationType == AGPS_NOTIFICATION_TYPE_FOR_ACTIVE_SUBSCRIBERS &&
                        !subscriber->mIsInactive)) {

            /* Deleting via this call would require another traversal
             * through subscriber list, inefficient; hence pass in false*/
            notifyEventToSubscriber(event, subscriber, false);

            if (deleteSubscriberPostNotify) {
                it = mSubscriberList.erase(it);
                delete subscriber;
            } else {
                it++;
            }
        } else {
            it++;
        }
    }
}

void AgpsStateMachine::notifyEventToSubscriber(
        AgpsEvent event, AgpsSubscriber* subscriberToNotify,
        bool deleteSubscriberPostNotify) {

    LOC_LOGD("notifyEventToSubscriber(): "
            "SM %p, Event %d Subscriber %p Delete %d",
            this, event, subscriberToNotify, deleteSubscriberPostNotify);

    switch (event) {

        case AGPS_EVENT_GRANTED:
            mAgpsManager->mAtlOpenStatusCb(
                    subscriberToNotify->mConnHandle, 1, getAPN(), getAPNLen(),
                    getBearer(), mAgpsType, mApnTypeMask);
            break;

        case AGPS_EVENT_DENIED:
            mAgpsManager->mAtlOpenStatusCb(
                    subscriberToNotify->mConnHandle, 0, getAPN(), getAPNLen(),
                    getBearer(), mAgpsType, mApnTypeMask);
            break;

        case AGPS_EVENT_UNSUBSCRIBE:
        case AGPS_EVENT_RELEASED:
            mAgpsManager->mAtlCloseStatusCb(subscriberToNotify->mConnHandle, 1);
            break;

        default:
            LOC_LOGE("Invalid event %d", event);
    }

    /* Search this subscriber in list and delete */
    if (deleteSubscriberPostNotify) {
        deleteSubscriber(subscriberToNotify);
    }
}

void AgpsStateMachine::transitionState(AgpsState newState){

    LOC_LOGD("transitionState(): SM %p, old %d, new %d",
               this, mState, newState);

    mState = newState;

    // notify state transitions to all subscribers ?
}

void AgpsStateMachine::addSubscriber(AgpsSubscriber* subscriberToAdd){

    LOC_LOGD("addSubscriber(): SM %p, Subscriber %p",
               this, subscriberToAdd);

    // Check if subscriber is already present in the current list
    // If not, then add
    std::list<AgpsSubscriber*>::const_iterator it = mSubscriberList.begin();
    for (; it != mSubscriberList.end(); it++) {
        AgpsSubscriber* subscriber = *it;
        if (subscriber->equals(subscriberToAdd)) {
            LOC_LOGE("Subscriber already in list");
            return;
        }
    }

    AgpsSubscriber* cloned = subscriberToAdd->clone();
    LOC_LOGD("addSubscriber(): cloned subscriber: %p", cloned);
    mSubscriberList.push_back(cloned);
}

void AgpsStateMachine::deleteSubscriber(AgpsSubscriber* subscriberToDelete){

    LOC_LOGD("deleteSubscriber(): SM %p, Subscriber %p",
               this, subscriberToDelete);

    std::list<AgpsSubscriber*>::const_iterator it = mSubscriberList.begin();
    while ( it != mSubscriberList.end() ) {

        AgpsSubscriber* subscriber = *it;
        if (subscriber && subscriber->equals(subscriberToDelete)) {

            it = mSubscriberList.erase(it);
            delete subscriber;
        } else {
            it++;
        }
    }
}

bool AgpsStateMachine::anyActiveSubscribers(){

    std::list<AgpsSubscriber*>::const_iterator it = mSubscriberList.begin();
    for (; it != mSubscriberList.end(); it++) {
        AgpsSubscriber* subscriber = *it;
        if (!subscriber->mIsInactive) {
            return true;
        }
    }
    return false;
}

void AgpsStateMachine::setAPN(char* apn, unsigned int len){

    if (NULL != mAPN) {
        delete mAPN;
        mAPN  = NULL;
    }

    if (NULL == apn || len <= 0 || len > MAX_APN_LEN || strlen(apn) != len) {
        LOC_LOGD("Invalid apn len (%d) or null apn", len);
        mAPN = NULL;
        mAPNLen = 0;
    } else {
        mAPN = new char[len+1];
        if (NULL != mAPN) {
            memcpy(mAPN, apn, len);
            mAPN[len] = '\0';
            mAPNLen = len;
        }
    }
}

AgpsSubscriber* AgpsStateMachine::getSubscriber(int connHandle){

    /* Go over the subscriber list */
    std::list<AgpsSubscriber*>::const_iterator it = mSubscriberList.begin();
    for (; it != mSubscriberList.end(); it++) {
        AgpsSubscriber* subscriber = *it;
        if (subscriber->mConnHandle == connHandle) {
            return subscriber;
        }
    }

    /* Not found, return NULL */
    return NULL;
}

AgpsSubscriber* AgpsStateMachine::getFirstSubscriber(bool isInactive){

    /* Go over the subscriber list */
    std::list<AgpsSubscriber*>::const_iterator it = mSubscriberList.begin();
    for (; it != mSubscriberList.end(); it++) {
        AgpsSubscriber* subscriber = *it;
        if(subscriber->mIsInactive == isInactive) {
            return subscriber;
        }
    }

    /* Not found, return NULL */
    return NULL;
}

void AgpsStateMachine::dropAllSubscribers(){

    LOC_LOGD("dropAllSubscribers(): SM %p", this);

    /* Go over the subscriber list */
    std::list<AgpsSubscriber*>::const_iterator it = mSubscriberList.begin();
    while ( it != mSubscriberList.end() ) {
        AgpsSubscriber* subscriber = *it;
        it = mSubscriberList.erase(it);
        delete subscriber;
    }
}

/* --------------------------------------------------------------------
 *   Loc AGPS Manager Methods
 * -------------------------------------------------------------------*/

/* CREATE AGPS STATE MACHINES
 * Must be invoked in Msg Handler context */
void AgpsManager::createAgpsStateMachines(const AgpsCbInfo& cbInfo) {

    LOC_LOGD("AgpsManager::createAgpsStateMachines");

    bool agpsCapable =
            ((loc_core::ContextBase::mGps_conf.CAPABILITIES & LOC_GPS_CAPABILITY_MSA) ||
                    (loc_core::ContextBase::mGps_conf.CAPABILITIES & LOC_GPS_CAPABILITY_MSB));

    if (NULL == mInternetNif && (cbInfo.atlType & AGPS_ATL_TYPE_WWAN)) {
        mInternetNif = new AgpsStateMachine(this, LOC_AGPS_TYPE_WWAN_ANY);
        mInternetNif->registerFrameworkStatusCallback((AgnssStatusIpV4Cb)cbInfo.statusV4Cb);
        LOC_LOGD("Internet NIF: %p", mInternetNif);
    }
    if (agpsCapable) {
        if (NULL == mAgnssNif && (cbInfo.atlType & AGPS_ATL_TYPE_SUPL) &&
                (cbInfo.atlType & AGPS_ATL_TYPE_SUPL_ES)) {
            mAgnssNif = new AgpsStateMachine(this, LOC_AGPS_TYPE_SUPL);
            mAgnssNif->registerFrameworkStatusCallback((AgnssStatusIpV4Cb)cbInfo.statusV4Cb);
            LOC_LOGD("AGNSS NIF: %p", mAgnssNif);
        }
    }
}

AgpsStateMachine* AgpsManager::getAgpsStateMachine(AGpsExtType agpsType) {

    LOC_LOGD("AgpsManager::getAgpsStateMachine(): agpsType %d", agpsType);

    switch (agpsType) {

        case LOC_AGPS_TYPE_INVALID:
        case LOC_AGPS_TYPE_SUPL:
        case LOC_AGPS_TYPE_SUPL_ES:
            if (mAgnssNif == NULL) {
                LOC_LOGE("NULL AGNSS NIF !");
            }
            return mAgnssNif;
        case LOC_AGPS_TYPE_WWAN_ANY:
            if (mInternetNif == NULL) {
                LOC_LOGE("NULL Internet NIF !");
            }
            return mInternetNif;
        default:
            return mInternetNif;
    }

    LOC_LOGE("No SM found !");
    return NULL;
}

void AgpsManager::requestATL(int connHandle, AGpsExtType agpsType,
                             LocApnTypeMask apnTypeMask){

    LOC_LOGD("AgpsManager::requestATL(): connHandle %d, agpsType 0x%X apnTypeMask: 0x%X",
               connHandle, agpsType, apnTypeMask);

    if (0 == loc_core::ContextBase::mGps_conf.USE_EMERGENCY_PDN_FOR_EMERGENCY_SUPL &&
        LOC_AGPS_TYPE_SUPL_ES == agpsType) {
        agpsType = LOC_AGPS_TYPE_SUPL;
        apnTypeMask &= ~LOC_APN_TYPE_MASK_EMERGENCY;
        apnTypeMask |= LOC_APN_TYPE_MASK_SUPL;
        LOC_LOGD("Changed agpsType to non-emergency when USE_EMERGENCY... is 0"
                 "and removed LOC_APN_TYPE_MASK_EMERGENCY from apnTypeMask"
                 "agpsType 0x%X apnTypeMask : 0x%X",
                 agpsType, apnTypeMask);
    }
    AgpsStateMachine* sm = getAgpsStateMachine(agpsType);

    if (sm == NULL) {

        LOC_LOGE("No AGPS State Machine for agpsType: %d apnTypeMask: 0x%X",
                 agpsType, apnTypeMask);
        mAtlOpenStatusCb(
                connHandle, 0, NULL, 0, AGPS_APN_BEARER_INVALID, agpsType, apnTypeMask);
        return;
    }
    sm->setType(agpsType);
    sm->setApnTypeMask(apnTypeMask);

    /* Invoke AGPS SM processing */
    AgpsSubscriber subscriber(connHandle, false, false, apnTypeMask);
    sm->setCurrentSubscriber(&subscriber);
    /* Send subscriber event */
    sm->processAgpsEvent(AGPS_EVENT_SUBSCRIBE);
}

void AgpsManager::releaseATL(int connHandle){

    LOC_LOGD("AgpsManager::releaseATL(): connHandle %d", connHandle);

    /* First find the subscriber with specified handle.
     * We need to search in all state machines. */
    AgpsStateMachine* sm = NULL;
    AgpsSubscriber* subscriber = NULL;

    if (mAgnssNif &&
            (subscriber = mAgnssNif->getSubscriber(connHandle)) != NULL) {
        sm = mAgnssNif;
    }
    else if (mInternetNif &&
            (subscriber = mInternetNif->getSubscriber(connHandle)) != NULL) {
        sm = mInternetNif;
    }
    if (sm == NULL) {
        LOC_LOGE("Subscriber with connHandle %d not found in any SM",
                    connHandle);
        return;
    }

    /* Now send unsubscribe event */
    sm->setCurrentSubscriber(subscriber);
    sm->processAgpsEvent(AGPS_EVENT_UNSUBSCRIBE);
}

void AgpsManager::reportAtlOpenSuccess(
        AGpsExtType agpsType, char* apnName, int apnLen,
        AGpsBearerType bearerType){

    LOC_LOGD("AgpsManager::reportAtlOpenSuccess(): "
             "AgpsType %d, APN [%s], Len %d, BearerType %d",
             agpsType, apnName, apnLen, bearerType);

    /* Find the state machine instance */
    AgpsStateMachine* sm = getAgpsStateMachine(agpsType);

    /* Set bearer and apn info in state machine instance */
    sm->setBearer(bearerType);
    sm->setAPN(apnName, apnLen);

    /* Send GRANTED event to state machine */
    sm->processAgpsEvent(AGPS_EVENT_GRANTED);
}

void AgpsManager::reportAtlOpenFailed(AGpsExtType agpsType){

    LOC_LOGD("AgpsManager::reportAtlOpenFailed(): AgpsType %d", agpsType);

    /* Fetch SM and send DENIED event */
    AgpsStateMachine* sm = getAgpsStateMachine(agpsType);
    sm->processAgpsEvent(AGPS_EVENT_DENIED);
}

void AgpsManager::reportAtlClosed(AGpsExtType agpsType){

    LOC_LOGD("AgpsManager::reportAtlClosed(): AgpsType %d", agpsType);

    /* Fetch SM and send RELEASED event */
    AgpsStateMachine* sm = getAgpsStateMachine(agpsType);
    sm->processAgpsEvent(AGPS_EVENT_RELEASED);
}

void AgpsManager::handleModemSSR(){

    LOC_LOGD("AgpsManager::handleModemSSR");

    /* Drop subscribers from all state machines */
    if (mAgnssNif) {
        mAgnssNif->dropAllSubscribers();
    }
    if (mInternetNif) {
        mInternetNif->dropAllSubscribers();
    }
}
