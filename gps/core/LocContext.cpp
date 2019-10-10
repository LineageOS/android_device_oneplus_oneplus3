/* Copyright (c) 2011-2014, 2016-2019 The Linux Foundation. All rights reserved.
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
#define LOG_TAG "LocSvc_Ctx"

#include <cutils/sched_policy.h>
#include <unistd.h>
#include <LocContext.h>
#include <msg_q.h>
#include <log_util.h>
#include <loc_log.h>

namespace loc_core {

const MsgTask* LocContext::mMsgTask = NULL;
ContextBase* LocContext::mContext = NULL;
// the name must be shorter than 15 chars
const char* LocContext::mLocationHalName = "Loc_hal_worker";
#ifndef USE_GLIB
const char* LocContext::mLBSLibName = "liblbs_core.so";
#else
const char* LocContext::mLBSLibName = "liblbs_core.so.1";
#endif

pthread_mutex_t LocContext::mGetLocContextMutex = PTHREAD_MUTEX_INITIALIZER;

const MsgTask* LocContext::getMsgTask(LocThread::tCreate tCreator,
                                          const char* name, bool joinable)
{
    if (NULL == mMsgTask) {
        mMsgTask = new MsgTask(tCreator, name, joinable);
    }
    return mMsgTask;
}

inline
const MsgTask* LocContext::getMsgTask(const char* name, bool joinable) {
    return getMsgTask((LocThread::tCreate)NULL, name, joinable);
}

ContextBase* LocContext::getLocContext(LocThread::tCreate tCreator,
            LocMsg* firstMsg, const char* name, bool joinable)
{
    pthread_mutex_lock(&LocContext::mGetLocContextMutex);
    LOC_LOGD("%s:%d]: querying ContextBase with tCreator", __func__, __LINE__);
    if (NULL == mContext) {
        LOC_LOGD("%s:%d]: creating msgTask with tCreator", __func__, __LINE__);
        const MsgTask* msgTask = getMsgTask(tCreator, name, joinable);
        mContext = new LocContext(msgTask);
    }
    pthread_mutex_unlock(&LocContext::mGetLocContextMutex);

    if (firstMsg) {
        mContext->sendMsg(firstMsg);
    }

    return mContext;
}

void LocContext :: injectFeatureConfig(ContextBase *curContext)
{
    LOC_LOGD("%s:%d]: Calling LBSProxy (%p) to inject feature config",
             __func__, __LINE__, ((LocContext *)curContext)->mLBSProxy);
    ((LocContext *)curContext)->mLBSProxy->injectFeatureConfig(curContext);
}

LocContext::LocContext(const MsgTask* msgTask) :
    ContextBase(msgTask, 0, mLBSLibName)
{
}

}
