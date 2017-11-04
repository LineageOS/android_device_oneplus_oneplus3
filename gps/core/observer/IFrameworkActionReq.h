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

#ifndef __IFRAMEWORKACTIONREQ_H__
#define __IFRAMEWORKACTIONREQ_H__

#include  <DataItemId.h>

namespace loc_core
{

/**
 * @brief IFrameworkActionReq interface
 * @details IFrameworkActionReq interface;
 *          Defines an interface for operations such as turnOn, turnOff a
 *          framework module described by the data item. Framework module
 *          could be bluetooth, wifi etc.
 *          Must be implemented by OS dependent code.
 *
 */
class IFrameworkActionReq {

public:
    /**
     * @brief Turn on the framework module described by the data item.
     * @details  Turn on the framework module described by the data item;
     *          An IFrameworkActionReq implementer invokes this method to
     *          turn on the framework module described by the data item.
     *          Framework module could be bluetooth, wifi etc.
     *
     * @param dit DataItemId
     * @param timeout Timeout after which to turn off the framework module.
     */
    virtual void turnOn (DataItemId dit, int timeOut = 0) = 0;

    /**
     * @brief Turn off the framework module described by the data item.
     * @details  Turn off the framework module described by the data item;
     *          An IFrameworkActionReq implementer invokes this method to
     *          turn off the framework module described by the data item.
     *          Framework module could be bluetooth, wifi etc.
     *
     * @param dit DataItemId
     */
    virtual void turnOff (DataItemId dit) = 0;

    /**
     * @brief Destructor
     * @details Destructor
     */
    virtual ~IFrameworkActionReq () {}
};

} // namespace loc_core

#endif // #ifndef __IFRAMEWORKACTIONREQ_H__

