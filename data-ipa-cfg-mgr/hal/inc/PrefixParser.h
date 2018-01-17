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
#ifndef _PREFIX_PARSER_H_
#define _PREFIX_PARSER_H_

/* External Includes */
#include <string.h>
#include <sys/types.h>
#include <vector>

/* Internal Includes */
#include "IOffloadManager.h"

/* Avoiding namespace pollution */
using IP_FAM = ::IOffloadManager::IP_FAM;
using Prefix = ::IOffloadManager::Prefix;

using ::std::string;
using ::std::vector;


class PrefixParser {
public:
    PrefixParser();
    bool add(vector<string> /* in */);
    bool add(string /* in */);
    bool addV4(vector<string> /* in */);
    bool addV4(string /* in */);
    bool addV6(vector<string> /* in */);
    bool addV6(string /* in */);
    int size();
    bool allAreFullyQualified();
    Prefix getFirstPrefix();
    Prefix getFirstPrefix(IP_FAM);
    string getLastErrAsStr();
private:
    bool add(string /* in */, IP_FAM /* famHint */);
    bool add(vector<string> /* in */, IP_FAM /* famHint */);
    static IP_FAM guessIPFamily(string /* in */);
    static bool splitIntoAddrAndMask(string /* in */, string& /* addr */,
            string& /* mask */);
    static int parseSubnetMask(string /* in */, IP_FAM /* famHint */);
    static bool parseV4Addr(string /* in */, Prefix& /* out */);
    static bool parseV6Addr(string /* in */, Prefix& /* out */);
    static bool populateV4Mask(int /* mask */, Prefix& /* out */);
    static bool populateV6Mask(int /* mask */, Prefix& /* out */);
    static uint32_t createMask(int /* mask */);
    static Prefix makeBlankPrefix(IP_FAM /* famHint */);
    bool isMaskValid(int /* mask */, IP_FAM /* fam */);
    static const uint32_t FULLY_QUALIFIED_MASK = ~0;
    vector<Prefix> mPrefixes;
    string mLastErr;
}; /* PrefixParser */
#endif /* _PREFIX_PARSER_H_ */