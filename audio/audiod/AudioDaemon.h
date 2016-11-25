/* AudioDaemon.h

Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <vector>

#include <utils/threads.h>
#include <utils/String8.h>


namespace android {

enum notify_status {
    snd_card_online,
    snd_card_offline,
    cpe_online,
    cpe_offline
};

enum notify_status_type {
    SND_CARD_STATE,
    CPE_STATE
};

enum audio_event_status {audio_event_on, audio_event_off};

#define AUDIO_PARAMETER_KEY_EXT_AUDIO_DEVICE "ext_audio_device"

class AudioDaemon:public Thread, public IBinder :: DeathRecipient
{
    /*Overrides*/
    virtual bool        threadLoop();
    virtual status_t    readyToRun();
    virtual void        onFirstRef();
    virtual void        binderDied(const wp < IBinder > &who);

    bool processUeventMessage();
    void notifyAudioSystem(int snd_card,
                           notify_status status,
                           notify_status_type type);
    void notifyAudioSystemEventStatus(const char* event, audio_event_status status);
    int mUeventSock;
    bool getStateFDs(std::vector<std::pair<int,int> > &sndcardFdPair);
    void putStateFDs(std::vector<std::pair<int,int> > &sndcardFdPair);
    bool getDeviceEventFDs();
    void putDeviceEventFDs();
    void checkEventState(int fd, int index);

public:
    AudioDaemon();
    virtual ~AudioDaemon();

private:
    std::vector<std::pair<int,int> > mSndCardFd;

    //file descriptors for audio device events and their statuses
    std::vector<std::pair<String8, int> > mAudioEvents;
    std::vector<std::pair<String8, int> > mAudioEventsStatus;

};

}
