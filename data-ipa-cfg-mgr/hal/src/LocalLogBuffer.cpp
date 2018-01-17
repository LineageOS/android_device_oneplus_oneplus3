/*
 * Copyright (c) 2017, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
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
 */
#define LOG_TAG "IPAHALService/dump"

/* External Includes */
#include <cutils/log.h>
#include <deque>
#include <string>
#include <sys/types.h>
#include <vector>

/* Internal Includes */
#include "LocalLogBuffer.h"

/* Namespace pollution avoidance */
using ::std::deque;
using ::std::string;
using ::std::vector;


LocalLogBuffer::FunctionLog::FunctionLog(string funcName) : mName(funcName) {
    mArgsProvided = false;
} /* FunctionLog */

LocalLogBuffer::FunctionLog::FunctionLog(const FunctionLog& other) :
        mName(other.mName) {
    mArgsProvided = other.mArgsProvided;
    /* Is this right? How do you copy stringstreams without wizardry? */
    mSSArgs.str(other.mSSArgs.str());
    mSSReturn.str(other.mSSReturn.str());
} /* FunctionLog */

void LocalLogBuffer::FunctionLog::addArg(string kw, string arg) {
    maybeAddArgsComma();
    mSSArgs << kw << "=" << arg;
} /* addArg */

void LocalLogBuffer::FunctionLog::addArg(string kw, vector<string> args) {
    maybeAddArgsComma();
    mSSArgs << kw << "=[";
    for (size_t i = 0; i < args.size(); i++) {
        mSSArgs << args[i];
        if (i < (args.size() - 1))
            mSSArgs << ", ";
    }
    mSSArgs << "]";
} /* addArg */

void LocalLogBuffer::FunctionLog::addArg(string kw, uint64_t arg) {
    maybeAddArgsComma();
    mSSArgs << kw << "=" << arg;
} /* addArg */

void LocalLogBuffer::FunctionLog::maybeAddArgsComma() {
    if (!mArgsProvided) {
        mArgsProvided = true;
    } else {
        mSSArgs << ", ";
    }
} /* maybeAddArgsComma */

void LocalLogBuffer::FunctionLog::setResult(bool success, string msg) {
    mSSReturn << "[" << ((success) ? "success" : "failure") << ", " << msg
              << "]";
} /* setResult */

void LocalLogBuffer::FunctionLog::setResult(vector<unsigned int> ret) {
    mSSReturn << "[";
    for (size_t i = 0; i < ret.size(); i++) {
        mSSReturn << ret[i];
        if (i < (ret.size() - 1))
            mSSReturn << ", ";
    }
    mSSReturn << "]";
} /* setResult */

void LocalLogBuffer::FunctionLog::setResult(uint64_t rx, uint64_t tx) {
    mSSReturn << "[rx=" << rx << ", tx=" << tx << "]";
} /* setResult */

string LocalLogBuffer::FunctionLog::toString() {
    stringstream ret;
    ret << mName << "(" << mSSArgs.str() << ") returned " << mSSReturn.str();
    return ret.str();
} /* toString */

LocalLogBuffer::LocalLogBuffer(string name, int maxLogs) : mName(name),
        mMaxLogs(maxLogs) {
} /* LocalLogBuffer */

void LocalLogBuffer::addLog(FunctionLog log) {
    while (mLogs.size() > mMaxLogs)
        mLogs.pop_front();
    mLogs.push_back(log);
} /* addLog */

void LocalLogBuffer::toLogcat() {
    for (size_t i = 0; i < mLogs.size(); i++)
        ALOGD("%s: %s", mName.c_str(), mLogs[i].toString().c_str());
} /* toLogcat */
