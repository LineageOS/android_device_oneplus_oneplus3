/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __COM_ANDROID_NFC_LIST_H__
#define __COM_ANDROID_NFC_LIST_H__

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

struct listNode
{
   void* pData;
   struct listNode* pNext;
};

struct listHead
{
    listNode* pFirst;
    pthread_mutex_t mutex;
};

bool listInit(listHead* pList);
bool listDestroy(listHead* pList);
bool listAdd(listHead* pList, void* pData);
bool listRemove(listHead* pList, void* pData);
bool listGetAndRemoveNext(listHead* pList, void** ppData);
void listDump(listHead* pList);

#ifdef __cplusplus
}
#endif

#endif /* __COM_ANDROID_NFC_LIST_H__ */
