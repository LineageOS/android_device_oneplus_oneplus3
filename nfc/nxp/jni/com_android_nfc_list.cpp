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

#include <com_android_nfc_list.h>
#include <com_android_nfc.h>
#include <pthread.h>
#include <errno.h>
#include <cutils/log.h>

#undef LOG_TAG
#define LOG_TAG "NFC_LIST"

bool listInit(listHead* pList)
{
   pList->pFirst = NULL;
   if(pthread_mutex_init(&pList->mutex, NULL) == -1)
   {
      ALOGE("Mutex creation failed (errno=0x%08x)", errno);
      return false;
   }

   return true;
}

bool listDestroy(listHead* pList)
{
   bool bListNotEmpty = true;
   while (bListNotEmpty) {
      bListNotEmpty = listGetAndRemoveNext(pList, NULL);
   }

   if(pthread_mutex_destroy(&pList->mutex) == -1)
   {
      ALOGE("Mutex destruction failed (errno=0x%08x)", errno);
      return false;
   }

   return true;
}

bool listAdd(listHead* pList, void* pData)
{
   struct listNode* pNode;
   struct listNode* pLastNode;
   bool result;

   /* Create node */
   pNode = (struct listNode*)malloc(sizeof(listNode));
   if (pNode == NULL)
   {
      result = false;
      ALOGE("Failed to malloc");
      goto clean_and_return;
   }
   TRACE("Allocated node: %8p (%8p)", pNode, pData);
   pNode->pData = pData;
   pNode->pNext = NULL;

   pthread_mutex_lock(&pList->mutex);

   /* Add the node to the list */
   if (pList->pFirst == NULL)
   {
       /* Set the node as the head */
      pList->pFirst = pNode;
   }
   else
   {
      /* Seek to the end of the list */
      pLastNode = pList->pFirst;
      while(pLastNode->pNext != NULL)
      {
          pLastNode = pLastNode->pNext;
      }

      /* Add the node to the current list */
      pLastNode->pNext = pNode;
   }

   result = true;

clean_and_return:
   pthread_mutex_unlock(&pList->mutex);
   return result;
}

bool listRemove(listHead* pList, void* pData)
{
   struct listNode* pNode;
   struct listNode* pRemovedNode;
   bool result;

   pthread_mutex_lock(&pList->mutex);

   if (pList->pFirst == NULL)
   {
      /* Empty list */
      ALOGE("Failed to deallocate (list empty)");
      result = false;
      goto clean_and_return;
   }

   pNode = pList->pFirst;
   if (pList->pFirst->pData == pData)
   {
      /* Get the removed node */
      pRemovedNode = pNode;

      /* Remove the first node */
      pList->pFirst = pList->pFirst->pNext;
   }
   else
   {
      while (pNode->pNext != NULL)
      {
         if (pNode->pNext->pData == pData)
         {
            /* Node found ! */
            break;
         }
         pNode = pNode->pNext;
      }

      if (pNode->pNext == NULL)
      {
         /* Node not found */
          result = false;
          ALOGE("Failed to deallocate (not found %8p)", pData);
          goto clean_and_return;
      }

      /* Get the removed node */
      pRemovedNode = pNode->pNext;

      /* Remove the node from the list */
      pNode->pNext = pNode->pNext->pNext;
   }

   /* Deallocate the node */
   TRACE("Deallocating node: %8p (%8p)", pRemovedNode, pRemovedNode->pData);
   free(pRemovedNode);

   result = true;

clean_and_return:
   pthread_mutex_unlock(&pList->mutex);
   return result;
}

bool listGetAndRemoveNext(listHead* pList, void** ppData)
{
   struct listNode* pNode;
   bool result;

   pthread_mutex_lock(&pList->mutex);

   if (pList->pFirst)
   {
      /* Empty list */
      ALOGE("Failed to deallocate (list empty)");
      result = false;
      goto clean_and_return;
   }

   /* Work on the first node */
   pNode = pList->pFirst;

   /* Return the data */
   if (ppData != NULL)
   {
      *ppData = pNode->pData;
   }

   /* Remove and deallocate the node */
   pList->pFirst = pNode->pNext;
   TRACE("Deallocating node: %8p (%8p)", pNode, pNode->pData);
   free(pNode);

   result = true;

clean_and_return:
   listDump(pList);
   pthread_mutex_unlock(&pList->mutex);
   return result;
}

void listDump(listHead* pList)
{
   struct listNode* pNode = pList->pFirst;

   TRACE("Node dump:");
   while (pNode != NULL)
   {
      TRACE("- %8p (%8p)", pNode, pNode->pData);
      pNode = pNode->pNext;
   }
}
