/* AudioDaemon.cpp
Copyright (c) 2012-2015, The Linux Foundation. All rights reserved.

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

#define LOG_TAG "AudioDaemon"
#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0

#include <dirent.h>
#include <media/AudioSystem.h>
#include <sys/poll.h>

#include "AudioDaemon.h"

#define CPE_MAGIC_NUM 0x2000
#define MAX_CPE_SLEEP_RETRY 2
#define CPE_SLEEP_WAIT 100

#define MAX_SLEEP_RETRY 100
#define AUDIO_INIT_SLEEP_WAIT 100 /* 100 ms */

int bootup_complete = 0;
bool cpe_bootup_complete = false;

namespace android {

    AudioDaemon::AudioDaemon() : Thread(false) {
    }

    AudioDaemon::~AudioDaemon() {
        putStateFDs(mSndCardFd);
    }

    void AudioDaemon::onFirstRef() {
        ALOGV("Start audiod daemon");
        run("AudioDaemon", PRIORITY_URGENT_AUDIO);
    }

    void AudioDaemon::binderDied(const wp<IBinder>& who)
    {
        requestExit();
    }

    bool AudioDaemon::getStateFDs(std::vector<std::pair<int,int> > &sndcardFdPair)
    {
        FILE *fp;
        int fd;
        char *ptr, *saveptr;
        char buffer[128];
        int line = 0;
        String8 path;
        int sndcard;
        const char* cards = "/proc/asound/cards";

        if ((fp = fopen(cards, "r")) == NULL) {
            ALOGE("Cannot open %s file to get list of sound cars", cards);
            return false;
        }

        sndcardFdPair.clear();
        memset(buffer, 0x0, sizeof(buffer));
        while ((fgets(buffer, sizeof(buffer), fp) != NULL)) {
            if (line % 2)
                continue;
            ptr = strtok_r(buffer, " [", &saveptr);
            if (ptr) {
                path = "/proc/asound/card";
                path += ptr;
                path += "/state";
                ALOGD("Opening sound card state : %s", path.string());
                fd = open(path.string(), O_RDONLY);
                if (fd == -1) {
                    ALOGE("Open %s failed : %s", path.string(), strerror(errno));
                } else {
                    /* returns vector of pair<sndcard, fd> */
                    sndcard = atoi(ptr);
                    sndcardFdPair.push_back(std::make_pair(sndcard, fd));
                }
            }
            line++;
        }

        ALOGV("%s: %d sound cards detected", __func__, sndcardFdPair.size());
        fclose(fp);

        return sndcardFdPair.size() > 0 ? true : false;
    }

    void AudioDaemon::putStateFDs(std::vector<std::pair<int,int> > &sndcardFdPair)
    {
        unsigned int i;
        for (i = 0; i < sndcardFdPair.size(); i++)
            close(sndcardFdPair[i].second);
        sndcardFdPair.clear();
    }

    bool AudioDaemon::getDeviceEventFDs()
    {
        const char* events_dir = "/sys/class/switch/";
        DIR *dp;
        struct dirent* in_file;
        int fd;
        String8 path;
        String8 d_name;

        if ((dp = opendir(events_dir)) == NULL) {
            ALOGE("Cannot open switch directory to get list of audio events %s", events_dir);
            return false;
        }

        mAudioEvents.clear();
        mAudioEventsStatus.clear();

        while ((in_file = readdir(dp)) != NULL) {

            if (!strstr(in_file->d_name, "qc_"))
                continue;
            ALOGD(" Found event file = %s", in_file->d_name);
            path = "/sys/class/switch/";
            path += in_file->d_name;
            path += "/state";

            ALOGE("Opening audio event state : %s ", path.string());
            fd = open(path.string(), O_RDONLY);
            if (fd == -1) {
                ALOGE("Open %s failed : %s", path.string(), strerror(errno));
            } else {
                d_name = in_file->d_name;
                mAudioEvents.push_back(std::make_pair(d_name, fd));
                mAudioEventsStatus.push_back(std::make_pair(d_name, 0));
                ALOGD("event status mAudioEventsStatus= %s",
                          mAudioEventsStatus[0].first.string());
            }
        }

        ALOGV("%s: %d audio device event detected",
                  __func__,
                  mAudioEvents.size());

        closedir(dp);
        return mAudioEvents.size() > 0 ? true : false;

    }

    void  AudioDaemon::putDeviceEventFDs()
    {
        unsigned int i;
        for (i = 0; i < mAudioEvents.size(); i++) {
            close(mAudioEvents[i].second);
            delete(mAudioEvents[i].first);
        }
        mAudioEvents.clear();
        mAudioEventsStatus.clear();
    }

    void AudioDaemon::checkEventState(int fd, int index)
    {
        char state_buf[2];
        audio_event_status event_cur_state = audio_event_off;

        if (!read(fd, (void *)state_buf, 1)) {
            ALOGE("Error receiving device state event (%s)", strerror(errno));
        } else {
             state_buf[1] = '\0';
            if (atoi(state_buf) != mAudioEventsStatus[index].second) {
                ALOGD("notify audio HAL %s",
                        mAudioEvents[index].first.string());
                mAudioEventsStatus[index].second = atoi(state_buf);

                if (mAudioEventsStatus[index].second == 1)
                    event_cur_state = audio_event_on;
                else
                    event_cur_state = audio_event_off;
                notifyAudioSystemEventStatus(
                               mAudioEventsStatus[index].first.string(),
                               event_cur_state);
            }
        }
        lseek(fd, 0, SEEK_SET);
    }

    status_t AudioDaemon::readyToRun() {

        ALOGV("readyToRun: open snd card state node files");
        return NO_ERROR;
    }

    bool AudioDaemon::threadLoop()
    {
        int max = -1;
        unsigned int i;
        bool ret = true;
        notify_status cur_state = snd_card_offline;
        struct pollfd *pfd = NULL;
        char rd_buf[9];
        unsigned int sleepRetry = 0;
        bool audioInitDone = false;
        int fd = 0;
        char path[50];
        notify_status cur_cpe_state = cpe_offline;
        notify_status prev_cpe_state = cpe_offline;
        unsigned int cpe_cnt = CPE_MAGIC_NUM;
        unsigned int num_snd_cards = 0;

        ALOGV("Start threadLoop()");
        while (audioInitDone == false && sleepRetry < MAX_SLEEP_RETRY) {
            if (mSndCardFd.empty() && !getStateFDs(mSndCardFd)) {
                ALOGE("Sleeping for 100 ms");
                usleep(AUDIO_INIT_SLEEP_WAIT*1000);
                sleepRetry++;
            } else {
                audioInitDone = true;
            }
        }

        if (!getDeviceEventFDs()) {
            ALOGE("No audio device events detected");
        }

        if (audioInitDone == false) {
            ALOGE("Sound Card is empty!!!");
            goto thread_exit;
        }

        /* soundcards are opened, now get the cpe state nodes */
        num_snd_cards = mSndCardFd.size();
        for (i = 0; i < num_snd_cards; i++) {
            snprintf(path, sizeof(path), "/proc/asound/card%d/cpe0_state", mSndCardFd[i].first);
            ALOGD("Opening cpe0_state : %s", path);
            sleepRetry = 0;
            do {
                fd = open(path, O_RDONLY);
                if (fd == -1)  {
                    sleepRetry++;
                    ALOGE("CPE state open %s failed %s, Retrying %d",
                          path, strerror(errno), sleepRetry);
                    usleep(CPE_SLEEP_WAIT*1000);
                } else {
                    ALOGD("cpe state opened: %s", path);
                    mSndCardFd.push_back(std::make_pair(cpe_cnt++, fd));
                }
            }while ((fd == -1) &&  sleepRetry < MAX_CPE_SLEEP_RETRY);
        }
        ALOGD("number of sndcards %d CPEs %d", i, cpe_cnt - CPE_MAGIC_NUM);

        pfd = new pollfd[mSndCardFd.size() + mAudioEvents.size()];
        bzero(pfd, (sizeof(*pfd) * mSndCardFd.size() +
                    sizeof(*pfd) * mAudioEvents.size()));
        for (i = 0; i < mSndCardFd.size(); i++) {
            pfd[i].fd = mSndCardFd[i].second;
            pfd[i].events = POLLPRI;
        }

        /*insert all audio events*/
        for(i = 0; i < mAudioEvents.size(); i++) {
            pfd[i+mSndCardFd.size()].fd = mAudioEvents[i].second;
            pfd[i+mSndCardFd.size()].events = POLLPRI;
        }

        ALOGD("read for sound card state change before while");
        for (i = 0; i < mSndCardFd.size(); i++) {
            if (!read(pfd[i].fd, (void *)rd_buf, 8)) {
               ALOGE("Error receiving sound card state event (%s)", strerror(errno));
               ret = false;
            } else {
               rd_buf[8] = '\0';
               lseek(pfd[i].fd, 0, SEEK_SET);

               if(mSndCardFd[i].first >= CPE_MAGIC_NUM) {
                   ALOGD("CPE %d state file content: %s before while",
                         mSndCardFd[i].first - CPE_MAGIC_NUM, rd_buf);
                   if (strstr(rd_buf, "OFFLINE")) {
                       ALOGD("CPE state offline");
                       cur_cpe_state = cpe_offline;
                   } else if (strstr(rd_buf, "ONLINE")){
                       ALOGD("CPE state online");
                       cur_cpe_state = cpe_online;
                   } else {
                       ALOGE("ERROR CPE rd_buf %s", rd_buf);
                   }
                   if (cur_cpe_state == cpe_online && !cpe_bootup_complete) {
                       cpe_bootup_complete = true;
                       ALOGD("CPE boot up completed before polling");
                   }
                   prev_cpe_state = cur_cpe_state;
               }
               else {
                   ALOGD("sound card state file content: %s before while",rd_buf);
                   if (strstr(rd_buf, "OFFLINE")) {
                       ALOGE("put cur_state to offline");
                       cur_state = snd_card_offline;
                   } else if (strstr(rd_buf, "ONLINE")){
                       ALOGE("put cur_state to online");
                       cur_state = snd_card_online;
                   } else {
                       ALOGE("ERROR rd_buf %s", rd_buf);
                   }

                   ALOGD("cur_state=%d, bootup_complete=%d", cur_state, cur_state );
                   if (cur_state == snd_card_online && !bootup_complete) {
                       bootup_complete = 1;
                       ALOGE("sound card up is deteced before while");
                       ALOGE("bootup_complete set to 1");
                   }
               }
            }
        }

       ALOGE("read for event state change before while");
       for (i = 0; i < mAudioEvents.size(); i++){
           checkEventState(pfd[i+mSndCardFd.size()].fd, i);
       }

        while (1) {
           ALOGD("poll() for sound card state change ");
           if (poll(pfd, (mSndCardFd.size() + mAudioEvents.size()), -1) < 0) {
              ALOGE("poll() failed (%s)", strerror(errno));
              ret = false;
              break;
           }

           ALOGD("out of poll() for sound card state change, SNDCARD size=%d", mSndCardFd.size());
           for (i = 0; i < mSndCardFd.size(); i++) {
               if (pfd[i].revents & POLLPRI) {
                   if (!read(pfd[i].fd, (void *)rd_buf, 8)) {
                       ALOGE("Error receiving sound card %d state event (%s)",
                             mSndCardFd[i].first, strerror(errno));
                       ret = false;
                   } else {
                       rd_buf[8] = '\0';
                       lseek(pfd[i].fd, 0, SEEK_SET);

                       if(mSndCardFd[i].first >= CPE_MAGIC_NUM) {
                           if (strstr(rd_buf, "OFFLINE"))
                               cur_cpe_state = cpe_offline;
                           else if (strstr(rd_buf, "ONLINE"))
                               cur_cpe_state = cpe_online;
                           else
                               ALOGE("ERROR CPE rd_buf %s", rd_buf);

                           if (cpe_bootup_complete && (prev_cpe_state != cur_cpe_state)) {
                               ALOGD("CPE state is %d, nofity AudioSystem", cur_cpe_state);
                               notifyAudioSystem(mSndCardFd[i].first, cur_cpe_state, CPE_STATE);
                           }
                           if (!cpe_bootup_complete && (cur_cpe_state == cpe_online)) {
                               cpe_bootup_complete = true;
                               ALOGD("CPE boot up completed");
                           }
                           prev_cpe_state = cur_cpe_state;
                       }
                       else {
                           ALOGV("sound card state file content: %s, bootup_complete=%d",rd_buf, bootup_complete);
                           if (strstr(rd_buf, "OFFLINE")) {
                               cur_state = snd_card_offline;
                           } else if (strstr(rd_buf, "ONLINE")){
                               cur_state = snd_card_online;
                           }

                           if (bootup_complete) {
                               ALOGV("bootup_complete, so NofityAudioSystem");
                               notifyAudioSystem(mSndCardFd[i].first, cur_state, SND_CARD_STATE);
                           }

                           if (cur_state == snd_card_online && !bootup_complete) {
                               bootup_complete = 1;
                           }
                       }
                   }
               }
           }
           for (i = 0; i < mAudioEvents.size(); i++) {
               if (pfd[i + mSndCardFd.size()].revents & POLLPRI) {
                   ALOGE("EVENT recieved pfd[i].revents= 0x%x %d",
                       pfd[i + mSndCardFd.size()].revents,
                       mAudioEvents[i].second);

                   checkEventState(pfd[i + mSndCardFd.size()].fd, i);
               }
           }
       }

       putStateFDs(mSndCardFd);
       putDeviceEventFDs();
       delete [] pfd;

    thread_exit:
       ALOGV("Exiting Poll ThreadLoop");
       return ret;
    }

    void AudioDaemon::notifyAudioSystem(int snd_card,
                                        notify_status status,
                                        notify_status_type type)
    {

        String8 str;
        char buf[4] = {0,};

        if (type == CPE_STATE) {
            str = "CPE_STATUS=";
            snprintf(buf, sizeof(buf), "%d", snd_card - CPE_MAGIC_NUM);
            str += buf;
            if (status == cpe_online)
                str += ",ONLINE";
            else
                str += ",OFFLINE";
        }
        else {
            str = "SND_CARD_STATUS=";
            snprintf(buf, sizeof(buf), "%d", snd_card);
            str += buf;
            if (status == snd_card_online)
                str += ",ONLINE";
            else
                str += ",OFFLINE";
        }
        ALOGV("%s: notifyAudioSystem : %s", __func__, str.string());
        AudioSystem::setParameters(0, str);
    }

    void AudioDaemon::notifyAudioSystemEventStatus(const char* event,
                                            audio_event_status status) {

        String8 str;
        str += AUDIO_PARAMETER_KEY_EXT_AUDIO_DEVICE;
        str += "=";
        str += event;

        if (status == audio_event_on)
            str += ",ON";
        else
            str += ",OFF";
        ALOGD("%s: notifyAudioSystemEventStatus : %s", __func__, str.string());
        AudioSystem::setParameters(0, str);
    }
}
