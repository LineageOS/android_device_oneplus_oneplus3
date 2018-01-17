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
/* External Includes */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

/* Internal Includes */
#include "IOffloadManager.h"
#include "PrefixParser.h"

/* Avoiding namespace pollution */
using IP_FAM = ::IOffloadManager::IP_FAM;
using Prefix = ::IOffloadManager::Prefix;

using ::std::string;
using ::std::vector;


/* ------------------------------ PUBLIC ------------------------------------ */
PrefixParser::PrefixParser() {
    mLastErr = "No Err";
} /* PrefixParser */

bool PrefixParser::add(vector<string> in) {
    return add(in, IP_FAM::INVALID);
} /* add */

bool PrefixParser::add(string in) {
    return add(in, IP_FAM::INVALID);
} /* add */

bool PrefixParser::addV4(string in) {
    return add(in, IP_FAM::V4);
} /* addV4 */

bool PrefixParser::addV4(vector<string> in) {
    return add(in, IP_FAM::V4);
} /* addV4 */

bool PrefixParser::addV6(string in) {
    return add(in, IP_FAM::V6);
} /* addV6 */

bool PrefixParser::addV6(vector<string> in) {
    for (size_t i = 0; i < in.size(); i++) {
        if (!addV6(in[i]))
            return false;
    }
    return true;
} /* addV6 */

int PrefixParser::size() {
    return mPrefixes.size();
} /* size */

bool PrefixParser::allAreFullyQualified() {
    for (size_t i = 0; i < mPrefixes.size(); i++) {
        if (mPrefixes[i].fam == IP_FAM::V4) {
            uint32_t masked = mPrefixes[i].v4Addr & mPrefixes[i].v4Mask;
            if (masked != mPrefixes[i].v4Addr)
                return false;
        } else {
            uint32_t masked[4];
            masked[0] = mPrefixes[i].v6Addr[0] & mPrefixes[i].v6Mask[0];
            masked[1] = mPrefixes[i].v6Addr[1] & mPrefixes[i].v6Mask[1];
            masked[2] = mPrefixes[i].v6Addr[2] & mPrefixes[i].v6Mask[2];
            masked[3] = mPrefixes[i].v6Addr[3] & mPrefixes[i].v6Mask[3];
            for (int j = 0; j < 4; j++) {
                if (masked[j] != mPrefixes[i].v6Addr[j])
                    return false;
            }
        }
    }
    return true;
} /* allAreFullyQualified */

Prefix PrefixParser::getFirstPrefix() {
    if (size() >= 1)
        return mPrefixes[0];
    return makeBlankPrefix(IP_FAM::INVALID);
} /* getFirstPrefix */

Prefix PrefixParser::getFirstPrefix(IP_FAM famHint) {
    if (size() >= 1)
        return mPrefixes[0];
    return makeBlankPrefix(famHint);
} /* getFirstPrefix */

string PrefixParser::getLastErrAsStr() {
    return mLastErr;
} /* getLastErrAsStr */


/* ------------------------------ PRIVATE ----------------------------------- */
bool PrefixParser::add(vector<string> in, IP_FAM famHint) {
    if (in.size() == 0)
        return false;

    for (size_t i = 0; i < in.size(); i++) {
        if (!add(in[i], famHint))
            return false;
    }
    return true;
} /* add */

bool PrefixParser::add(string in, IP_FAM famHint) {
    if (in.length() == 0) {
        mLastErr = "Failed to parse string, length = 0...";
        return false;
    }

    if (famHint == IP_FAM::INVALID)
        famHint = guessIPFamily(in);

    string subnet;
    string addr;

    if (!splitIntoAddrAndMask(in, addr, subnet)) {
        mLastErr = "Failed to split into Address and Mask(" + in + ")";
        return false;
    }

    int mask = parseSubnetMask(subnet, famHint);
    if (!isMaskValid(mask, famHint)) {
        mLastErr = "Invalid mask";
        return false;
    }

    Prefix pre = makeBlankPrefix(famHint);

    if (famHint == IP_FAM::V4) {
        if (!parseV4Addr(addr, pre)) {
            mLastErr = "Failed to parse V4 Address(" + addr + ")";
            return false;
        }
    } else if (!parseV6Addr(addr, pre)) {
        mLastErr = "Failed to parse V6 Address(" + addr + ")";
        return false;
    }

    if (famHint == IP_FAM::V4 && !populateV4Mask(mask, pre)) {
        mLastErr = "Failed to populate IPv4 Mask(" + std::to_string(mask)
                + ", " + addr + ")";
        return false;
    } else if (!populateV6Mask(mask, pre)) {
        mLastErr = "Failed to populate IPv6 Mask(" + std::to_string(mask)
                + ", " + addr + ")";
        return false;
    }

    mPrefixes.push_back(pre);
    return true;
} /* add */

/* Assumption (based on man inet_pton)
 *
 * X represents a hex character
 * d represents a base 10 digit
 * / represents the start of the subnet mask
 *              (assume that it can be left off of all below combinations)
 *
 * IPv4 Addresses always look like the following:
 *      ddd.ddd.ddd.ddd/dd
 *
 * IPv6 Addresses can look a few different ways:
 *      x:x:x:x:x:x:x:x/ddd
 *      x::x/ddd
 *      x:x:x:x:x:x:d.d.d.d/ddd
 *
 * Therefore, if a presentation of an IP Address contains a colon, then it
 * may not be a valid IPv6, but, it is definitely not valid IPv4.  If a
 * presentation of an IP Address does not contain a colon, then it may not be
 * a valid IPv4, but, it is definitely not IPv6.
 */
IP_FAM PrefixParser::guessIPFamily(string in) {
    size_t found = in.find(":");
    if (found != string::npos)
        return IP_FAM::V6;
    return IP_FAM::V4;
} /* guessIPFamily */

bool PrefixParser::splitIntoAddrAndMask(string in, string &addr, string &mask) {
    size_t pos = in.find("/");

    if (pos != string::npos && pos >= 1) {
        /* addr is now everything up until the first / */
        addr = in.substr(0, pos);
    } else if (pos == string::npos) {
        /* There is no /, so the entire input is an address */
        addr = in;
    } else {
        /* There was nothing before the /, not recoverable */
        return false;
    }

    if (pos != string::npos && pos < in.size()) {
        /* There is a / and it is not the last character.  Everything after /
         * must be the subnet.
         */
        mask = in.substr(pos + 1);
    } else if (pos != string::npos && pos == in.size()) {
        /* There is a /, but it is the last character.  This is garbage, but,
         * we may still be able to interpret the address so we will throw it
         * out.
         */
        mask = "";
    } else if (pos == string::npos) {
        /* There is no /, therefore, there is no subnet */
        mask = "";
    } else {
        /* This really shouldn't be possible because it would imply that find
         * returned a position larger than the size of the input.  Just
         * preserving sanity that mask is always initialized.
         */
        mask = "";
    }

    return true;
} /* splitIntoAddrAndMask */

int PrefixParser::parseSubnetMask(string in, IP_FAM famHint) {
    if (in.empty())
        /* Treat no subnet mask as fully qualified */
        return (famHint == IP_FAM::V6) ? 128 : 32;
    return atoi(in.c_str());
} /* parseSubnetMask */

bool PrefixParser::parseV4Addr(string in, Prefix &out) {
    struct sockaddr_in sa;

    int ret = inet_pton(AF_INET, in.c_str(), &(sa.sin_addr));

    if (ret < 0) {
        /* errno would be valid */
        return false;
    } else if (ret == 0) {
        /* input was not a valid IP address */
        return false;
    }

    /* Address in network byte order */
    out.v4Addr = htonl(sa.sin_addr.s_addr);
    return true;
} /* parseV4Addr */

bool PrefixParser::parseV6Addr(string in, Prefix &out) {
    struct sockaddr_in6 sa;

    int ret = inet_pton(AF_INET6, in.c_str(), &(sa.sin6_addr));

    if (ret < 0) {
        /* errno would be valid */
        return false;
    } else if (ret == 0) {
        /* input was not a valid IP address */
        return false;
    }

    /* Translate unsigned chars to unsigned ints to match IPA
     *
     * TODO there must be a better way to do this beyond bit fiddling
     * Maybe a Union since we've already made the assumption that the data
     * structures match?
     */
    out.v6Addr[0] = (sa.sin6_addr.s6_addr[0] << 24) |
                    (sa.sin6_addr.s6_addr[1] << 16) |
                    (sa.sin6_addr.s6_addr[2] << 8) |
                    (sa.sin6_addr.s6_addr[3]);
    out.v6Addr[1] = (sa.sin6_addr.s6_addr[4] << 24) |
                    (sa.sin6_addr.s6_addr[5] << 16) |
                    (sa.sin6_addr.s6_addr[6] << 8) |
                    (sa.sin6_addr.s6_addr[7]);
    out.v6Addr[2] = (sa.sin6_addr.s6_addr[8] << 24) |
                    (sa.sin6_addr.s6_addr[9] << 16) |
                    (sa.sin6_addr.s6_addr[10] << 8) |
                    (sa.sin6_addr.s6_addr[11]);
    out.v6Addr[3] = (sa.sin6_addr.s6_addr[12] << 24) |
                    (sa.sin6_addr.s6_addr[13] << 16) |
                    (sa.sin6_addr.s6_addr[14] << 8) |
                    (sa.sin6_addr.s6_addr[15]);
    return true;
} /* parseV6Addr */

bool PrefixParser::populateV4Mask(int mask, Prefix &out) {
    if (mask < 0 || mask > 32)
        return false;
    out.v4Mask = createMask(mask);
    return true;
} /* populateV4Mask */

bool PrefixParser::populateV6Mask(int mask, Prefix &out) {
    if (mask < 0 || mask > 128)
        return false;

    for (int i = 0; i < 4; i++) {
        out.v6Mask[i] = createMask(mask);
        mask = (mask > 32) ? mask - 32 : 0;
    }

    return true;
} /* populateV6Mask */

uint32_t PrefixParser::createMask(int mask) {
    uint32_t ret = 0;

    if (mask >= 32) {
        ret = ~ret;
        return ret;
    }

    for (int i = 0; i < 32; i++) {
        if (i < mask)
            ret = (ret << 1) | 1;
        else
            ret = (ret << 1);
    }

    return ret;
} /* createMask */

Prefix PrefixParser::makeBlankPrefix(IP_FAM famHint) {
    Prefix ret;

    ret.fam = famHint;

    ret.v4Addr = 0;
    ret.v4Mask = 0;

    ret.v6Addr[0] = 0;
    ret.v6Addr[1] = 0;
    ret.v6Addr[2] = 0;
    ret.v6Addr[3] = 0;

    ret.v6Mask[0] = 0;
    ret.v6Mask[1] = 0;
    ret.v6Mask[2] = 0;
    ret.v6Mask[3] = 0;

    return ret;
} /* makeBlankPrefix */

bool PrefixParser::isMaskValid(int mask, IP_FAM fam) {
    if (mask < 0) {
        mLastErr = "Failed parse subnet mask(" + std::to_string(mask) + ")";
        return false;
    } else if (mask == 0) {
        mLastErr = "Subnet mask cannot be 0(" + std::to_string(mask) + ")";
        return false;
    } else if (fam == IP_FAM::V4 && mask > 32) {
        mLastErr = "Interpreted address as V4 but mask was too large("
                + std::to_string(mask) + ")";
        return false;
    } else if (fam == IP_FAM::V6 && mask > 128) {
        mLastErr = "Interpreted address as V6 but mask was too large("
                + std::to_string(mask) + ")";
        return false;
    }

    return true;
} /* isMaskValid */
