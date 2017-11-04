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

#include <string>
#include <IndexFactory.h>
#include <IClientIndex.h>
#include <ClientIndex.h>
#include <IDataItemIndex.h>
#include <DataItemIndex.h>
#include <IDataItemObserver.h>
#include <DataItemId.h>

using namespace std;
using loc_core::IClientIndex;
using loc_core::IDataItemIndex;
using loc_core::IDataItemObserver;
using namespace loc_core;

template <typename CT, typename DIT>
inline IClientIndex <CT, DIT> * IndexFactory <CT, DIT> :: createClientIndex
()
{
    return new (nothrow) ClientIndex <CT, DIT> ();
}

template <typename CT, typename DIT>
inline IDataItemIndex <CT, DIT> * IndexFactory <CT, DIT> :: createDataItemIndex
()
{
    return new (nothrow) DataItemIndex <CT, DIT> ();
}

// Explicit instantiation must occur in same namespace where class is defined
namespace loc_core
{
  template class IndexFactory <IDataItemObserver *, DataItemId>;
  template class IndexFactory <string, DataItemId>;
}
