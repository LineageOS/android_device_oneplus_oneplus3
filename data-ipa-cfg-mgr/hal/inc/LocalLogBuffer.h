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
#ifndef _LOCAL_LOG_BUFFER_H_
#define _LOCAL_LOG_BUFFER_H_
/* External Includes */
#include <deque>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <vector>

/* Namespace pollution avoidance */
using ::std::deque;
using ::std::string;
using ::std::stringstream;
using ::std::vector;


class LocalLogBuffer {
public:
    class FunctionLog {
    public:
        FunctionLog(string /* funcName */);
        FunctionLog(const FunctionLog& /* other */);
        void addArg(string /* kw */, string /* arg */);
        void addArg(string /* kw */, vector<string> /* args */);
        void addArg(string /* kw */, uint64_t /* arg */);
        void setResult(bool /* success */, string /* msg */);
        void setResult(vector<unsigned int> /* ret */);
        void setResult(uint64_t /* rx */, uint64_t /* tx */);
        string toString();
    private:
        void maybeAddArgsComma();
        const string mName;
        bool mArgsProvided;
        stringstream mSSArgs;
        stringstream mSSReturn;
    }; /* FunctionLog */
    LocalLogBuffer(string /* name */, int /* maxLogs */);
    void addLog(FunctionLog /* log */);
    void toLogcat();
private:
    deque<FunctionLog> mLogs;
    const string mName;
    const size_t mMaxLogs;
}; /* LocalLogBuffer */
#endif /* _LOCAL_LOG_BUFFER_H_ */