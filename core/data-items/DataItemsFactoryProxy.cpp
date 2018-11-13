/* Copyright (c) 2017, The Linux Foundation. All rights reserved.
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
#define LOG_TAG "DataItemsFactoryProxy"

#include <dlfcn.h>
#include <DataItemId.h>
#include <IDataItemCore.h>
#include <DataItemsFactoryProxy.h>
#include <loc_pla.h>
#include <log_util.h>

namespace loc_core
{
void* DataItemsFactoryProxy::dataItemLibHandle = NULL;
get_concrete_data_item_fn* DataItemsFactoryProxy::getConcreteDIFunc = NULL;

IDataItemCore* DataItemsFactoryProxy::createNewDataItem(DataItemId id)
{
    IDataItemCore *mydi = nullptr;

    if (NULL != getConcreteDIFunc) {
        mydi = (*getConcreteDIFunc)(id);
    }
    else {
        // first call to this function, symbol not yet loaded
        if (NULL == dataItemLibHandle) {
            LOC_LOGD("Loaded library %s",DATA_ITEMS_LIB_NAME);
            dataItemLibHandle = dlopen(DATA_ITEMS_LIB_NAME, RTLD_NOW);
            if (NULL == dataItemLibHandle) {
                // dlopen failed.
                const char * err = dlerror();
                if (NULL == err)
                {
                    err = "Unknown";
                }
                LOC_LOGE("%s:%d]: failed to load library %s; error=%s",
                     __func__, __LINE__, DATA_ITEMS_LIB_NAME, err);
            }
        }

        // load sym - if dlopen handle is obtained and symbol is not yet obtained
        if (NULL != dataItemLibHandle) {
            getConcreteDIFunc = (get_concrete_data_item_fn * )
                                    dlsym(dataItemLibHandle, DATA_ITEMS_GET_CONCRETE_DI);
            if (NULL != getConcreteDIFunc) {
                LOC_LOGD("Loaded function %s : %p",DATA_ITEMS_GET_CONCRETE_DI,getConcreteDIFunc);
                mydi = (*getConcreteDIFunc)(id);
            }
            else {
                // dlysm failed.
                const char * err = dlerror();
                if (NULL == err)
                {
                    err = "Unknown";
                }
                LOC_LOGE("%s:%d]: failed to find symbol %s; error=%s",
                         __func__, __LINE__, DATA_ITEMS_GET_CONCRETE_DI, err);
            }
        }
    }
    return mydi;
}

void DataItemsFactoryProxy::closeDataItemLibraryHandle()
{
    if (NULL != dataItemLibHandle) {
        dlclose(dataItemLibHandle);
        dataItemLibHandle = NULL;
    }
}

} // namespace loc_core


