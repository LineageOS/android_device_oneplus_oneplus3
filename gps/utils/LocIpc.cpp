/* Copyright (c) 2017 The Linux Foundation. All rights reserved.
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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <array>
#include <platform_lib_log_util.h>
#include <string>
#include <vector>
#include "gps_extended_c.h"
#include "LocIpc.h"

namespace loc_util {

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "LocSvc_LocIpc"

#define LOC_MSG_BUF_LEN 1024
#define LOC_MSG_HEAD "$MSGLEN$"

class LocIpcRunnable : public LocRunnable {
public:
    LocIpcRunnable(LocIpc& locIpc, const std::string& ipcName)
            : mLocIpc(locIpc), mIpcName(ipcName) {}
    bool run() override {
        if (!mLocIpc.startListeningBlocking(mIpcName)) {
            LOC_LOGe("listen to socket failed");
        }

        return false;
    }
private:
     LocIpc& mLocIpc;
     const std::string mIpcName;
};

bool LocIpc::startListeningNonBlocking(const std::string& name) {
    mRunnable.reset(new LocIpcRunnable(*this, name));
    std::string threadName("LocIpc-");
    threadName.append(name);
    return mThread.start(threadName.c_str(), mRunnable.get());
}

bool LocIpc::startListeningBlocking(const std::string& name) {
    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0) {
        LOC_LOGe("create socket error. reason:%s", strerror(errno));
        return false;
    }

    if ((unlink(name.c_str()) < 0) && (errno != ENOENT)) {
       LOC_LOGw("unlink socket error. reason:%s", strerror(errno));
    }

    struct sockaddr_un addr = { .sun_family = AF_UNIX };
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", name.c_str());

    umask(0157);

    if (::bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOC_LOGe("bind socket error. reason:%s", strerror(errno));
        ::close(fd);
        fd = -1;
        return false;
    }

    mIpcFd = fd;

    ssize_t nBytes = 0;
    std::vector<char> buf(LOC_MSG_BUF_LEN);
    while ((nBytes = ::recvfrom(mIpcFd, buf.data(), buf.size(), 0, NULL, NULL)) >= 0) {
        if (nBytes == 0) {
            continue;
        }

        std::string msg;
        if (strncmp(buf.data(), LOC_MSG_HEAD, sizeof(LOC_MSG_HEAD) - 1)) {
            // short message
            msg.append(buf.data(), nBytes);
            onReceive(msg);

        } else {
            // long message
            size_t msgLen = 0;
            sscanf(buf.data(), LOC_MSG_HEAD"%zu", &msgLen);
            while (msg.length() < msgLen &&
                    (nBytes = recvfrom(mIpcFd, buf.data(), buf.size(), 0, NULL, NULL)) >= 0) {
                msg.append(buf.data(), nBytes);
            }

            if (nBytes >= 0) {
                onReceive(msg);
            } else {
                break;
            }
        }
    }

    if (mStopRequested) {
        mStopRequested = false;
        return true;

    } else {
        LOC_LOGe("cannot read socket. reason:%s", strerror(errno));
        (void)::close(mIpcFd);
        mIpcFd = -1;
        return false;
    }
}

void LocIpc::stopListening() {
    mStopRequested = true;

    if (mIpcFd >= 0) {
        if (::close(mIpcFd)) {
            LOC_LOGe("cannot close socket:%s", strerror(errno));
        }
        mIpcFd = -1;
    }
}

bool LocIpc::send(const char name[], const std::string& data) {
    int fd  = ::socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0) {
        LOC_LOGe("create socket error. reason:%s", strerror(errno));
        return false;
    }

    struct sockaddr_un addr = { .sun_family = AF_UNIX };
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", name);

    bool result = true;
    if (data.length() <= LOC_MSG_BUF_LEN) {
        if (::sendto(fd, data.c_str(), data.length(), 0,
                (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            LOC_LOGe("cannot send to socket. reason:%s", strerror(errno));
            result = false;
        }
    } else {
        std::string head = LOC_MSG_HEAD;
        head.append(std::to_string(data.length()));
        if (::sendto(fd, head.c_str(), head.length(), 0,
                (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            LOC_LOGe("cannot send to socket. reason:%s", strerror(errno));
            result = false;
        } else {
            size_t sentBytes = 0;
            while(sentBytes < data.length()) {
                size_t partLen = data.length() - sentBytes;
                if (partLen > LOC_MSG_BUF_LEN) {
                    partLen = LOC_MSG_BUF_LEN;
                }
                ssize_t rv = ::sendto(fd, data.c_str() + sentBytes, partLen, 0,
                        (struct sockaddr*)&addr, sizeof(addr));
                if (rv < 0) {
                    LOC_LOGe("cannot send to socket. reason:%s", strerror(errno));
                    result = false;
                    break;
                }
                sentBytes += rv;
            }
        }
    }

    (void)::close(fd);
    return result;
}

}
