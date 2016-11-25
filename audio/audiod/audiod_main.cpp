/* Copyright (C) 2007 The Android Open Source Project

Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.

Not a Contribution, Apache license notifications and license are retained
for attribution purposes only.

* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#define LOG_TAG "AudioDaemonMain"
#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0

#include <cutils/properties.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include <utils/Log.h>
#include <utils/threads.h>

#if defined(HAVE_PTHREADS)
# include <pthread.h>
# include <sys/resource.h>
#endif

#include "AudioDaemon.h"

using namespace android;

// ---------------------------------------------------------------------------

int main(int argc, char** argv)
{
#if defined(HAVE_PTHREADS)
    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_AUDIO);
#endif


    ALOGV("Audio daemon starting sequence..");
    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();

    sp<AudioDaemon> audioService = new AudioDaemon();
    IPCThreadState::self()->joinThreadPool();

    return 0;
}
