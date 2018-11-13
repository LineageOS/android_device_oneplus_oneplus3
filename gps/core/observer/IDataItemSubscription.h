/* Copyright (c) 2015, 2017 The Linux Foundation. All rights reserved.
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

#ifndef __IDATAITEMSUBSCRIPTION_H__
#define __IDATAITEMSUBSCRIPTION_H__

#include  <list>
#include  <DataItemId.h>

namespace loc_core
{
class IDataItemObserver;

/**
 * @brief IDataItemSubscription interface
 * @details IDataItemSubscription interface;
 *          Defines an interface for operations such as subscribe,
 *          unsubscribe data items by their IDs.
 *          Must be implemented by OS dependent code.
 */
class IDataItemSubscription {

public:
    /**
     * @brief Subscribe for data items by their IDs
     * @details Subscribe for data items by their IDs;
     *          An IDataItemObserver implementer invokes this method to subscribe
     *          for a list of DataItems by passing in their Ids.
     *          A symbolic invocation of this method in the following order
     *          subscribe ( {1,2,3}, &obj), subscribe ( {2,3,4,5}, &obj)
     *          where the numbers enclosed in braces indicate a list of data item Ids
     *          will cause this class implementer to update its subscription list for
     *          &obj to only contain the following Data Item Ids 1,2,3,4,5.
     *
     * @param l List of DataItemId
     * @param o Pointer to an instance of IDataItemObserver
     */
    virtual void subscribe (const std :: list <DataItemId> & l, IDataItemObserver * o = NULL) = 0;

    /**
     * @brief Update subscription for Data items
     * @details Update subscription for Data items;
     *          An IDataItemObserver implementer invokes this method to update their
     *          subscription for a list of DataItems by passing in their Ids
     *          A symbolic invocation of this method in the following order
     *          updateSubscription ( {1,2,3}, &obj),updateSubscription ( {2,3,4,5}, &obj)
     *          where the numbers enclosed in braces indicate a list of data item Ids
     *          will cause this class implementer to update its subscription list for
     *          &obj to only contain the following Data Item Ids 2,3,4,5.
     *          Note that this method may or may not be called.
     *
     * @param l List of DataItemId
     * @param o Pointer to an instance of IDataItemObserver
     */
    virtual void updateSubscription (const std :: list <DataItemId> & l, IDataItemObserver * o = NULL) = 0;

    /**
     * @brief Request Data
     * @details Request Data
     *
     * @param l List of DataItemId
     * @param o Pointer to an instance of IDataItemObserver
     */
    virtual void requestData (const std :: list <DataItemId> & l, IDataItemObserver * o = NULL) = 0;

    /**
     * @brief Unsubscribe Data items
     * @details Unsubscrbe Data items;
     *          An IDataItemObserver implementer invokes this method to unsubscribe their
     *          subscription for a list of DataItems by passing in their Ids
     *          Suppose this class implementor has a currently active subscription list
     *          containing 1,2,3,4,5,6,7 for &obj then a symbolic invocation of this
     *          method in the following order
     *          unsubscribe ( {1,2,3}, &obj), unsubscribe (  {1,2,3,4}, &obj),
     *          unsubscribe ( {7}, &obj)
     *          where the numbers enclosed in braces indicate a list of data item Ids
     *          will cause this class implementer to update its subscription list for
     *          &obj to only contain the following data item id 5,6.
     *
     * @param l List of DataItemId
     * @param o Pointer to an instance of IDataItemObserver
     */
    virtual void unsubscribe (const std :: list <DataItemId> & l, IDataItemObserver * o = NULL) = 0;

    /**
     * @brief Unsubscribe all data items
     * @details Unsubscribe all data items
     *
     * @param o Pointer to an instance of IDataItemObserver
     */
    virtual void unsubscribeAll (IDataItemObserver * o = NULL) = 0;

    /**
     * @brief Destructor
     * @details Destructor
     */
    virtual ~IDataItemSubscription () {}
};

} // namespace loc_core

#endif // #ifndef __IDATAITEMSUBSCRIPTION_H__

