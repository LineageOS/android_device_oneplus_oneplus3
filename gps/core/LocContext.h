/* Copyright (c) 2011-2014, 2017-2019 The Linux Foundation. All rights reserved.
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
#ifndef __LOC_CONTEXT__
#define __LOC_CONTEXT__

#include <stdbool.h>
#include <ctype.h>
#include <dlfcn.h>
#include <ContextBase.h>

namespace loc_core {

class LocContext : public ContextBase {
    static const MsgTask* mMsgTask;
    static ContextBase* mContext;
    static const MsgTask* getMsgTask(LocThread::tCreate tCreator,
                                     const char* name, bool joinable = true);
    static const MsgTask* getMsgTask(const char* name, bool joinable = true);
    static pthread_mutex_t mGetLocContextMutex;

protected:
    LocContext(const MsgTask* msgTask);
    inline virtual ~LocContext() {}

public:
    static const char* mLBSLibName;
    static const char* mLocationHalName;

    static ContextBase* getLocContext(LocThread::tCreate tCreator, LocMsg* firstMsg,
                                        const char* name, bool joinable = true);
    inline static ContextBase* getLocContext(const char* name, bool joinable = true) {
        return getLocContext(NULL, NULL, name, joinable);
    }

    static void injectFeatureConfig(ContextBase *context);
};

}

#endif //__LOC_CONTEXT__
