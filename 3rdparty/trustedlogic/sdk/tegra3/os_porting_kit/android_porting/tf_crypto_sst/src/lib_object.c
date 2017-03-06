/*
 * Copyright (c) 2011 Trusted Logic S.A.
 * All Rights Reserved.
 *
 * This software is the confidential and proprietary information of
 * Trusted Logic S.A. ("Confidential Information"). You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered
 * into with Trusted Logic S.A.
 *
 * TRUSTED LOGIC S.A. MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. TRUSTED LOGIC S.A. SHALL
 * NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
 * MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */


/** Simple implementation of lib_object using doubly-linked lists. This
   implementation should outperform more sophisticated data structures
   like blanced binary trees when the tables have fewer than 10 to 20 
   elements.  */

#include <string.h>
#include "s_type.h"

#include "lib_object.h"

/* Generic node */
typedef struct LIB_OBJECT_NODE
{
   struct LIB_OBJECT_NODE* pPrevious;
   struct LIB_OBJECT_NODE* pNext;
   union
   {
      uint16_t nHandle;

      S_STORAGE_NAME sStorageName;

      struct
      {
         /* Public fields */
         uint8_t  sFilename[64];
         uint8_t  nFilenameLength;
      }
      f;
   }
   key;
}
LIB_OBJECT_NODE;

typedef enum
{
   LIB_OBJECT_NODE_TYPE_HANDLE16,
   LIB_OBJECT_NODE_TYPE_STORAGE_NAME,
   LIB_OBJECT_NODE_TYPE_FILENAME,
   LIB_OBJECT_NODE_TYPE_UNINDEXED
}
LIB_OBJECT_NODE_TYPE;

/* -----------------------------------------------------------------------
   Search functions
   -----------------------------------------------------------------------*/
static bool libObjectKeyEqualNode(
   LIB_OBJECT_NODE* pNode,
   uint32_t nKey1,
   void* pKey2,
   LIB_OBJECT_NODE_TYPE eNodeType)
{
   switch (eNodeType)
   {
   default:
   case LIB_OBJECT_NODE_TYPE_HANDLE16:
      return 
         nKey1 == pNode->key.nHandle;
   case LIB_OBJECT_NODE_TYPE_STORAGE_NAME:
      return 
         memcmp(
            (S_STORAGE_NAME*)pKey2,
            &pNode->key.sStorageName,
            sizeof(S_STORAGE_NAME)) == 0;
   case LIB_OBJECT_NODE_TYPE_FILENAME:
   {      
      uint32_t nLength1 = nKey1;
      uint32_t nLength2 = pNode->key.f.nFilenameLength;
      return 
         nLength1 == nLength2 
         &&
         memcmp(
            pKey2,
            &pNode->key.f.sFilename,
            nLength1) == 0;
   }
   }
}

/* Polymorphic search function */
static LIB_OBJECT_NODE* libObjectSearch(
   LIB_OBJECT_NODE* pRoot, 
   uint32_t nKey1, 
   void* pKey2, 
   LIB_OBJECT_NODE_TYPE eNodeType)
{
   if (pRoot != NULL)
   {
      LIB_OBJECT_NODE* pNode = pRoot;
      do
      {
         if (libObjectKeyEqualNode(pNode, nKey1, pKey2, eNodeType))
         {
            /* Match found */
            return pNode;
         }
         pNode = pNode->pNext;
      }
      while (pNode != pRoot);
   }
   return NULL;
}

LIB_OBJECT_NODE_HANDLE16* libObjectHandle16Search(
               LIB_OBJECT_TABLE_HANDLE16* pTable,
               uint32_t nHandle)
{
   return (LIB_OBJECT_NODE_HANDLE16*)libObjectSearch(
      (LIB_OBJECT_NODE*)pTable->pRoot, nHandle, NULL, LIB_OBJECT_NODE_TYPE_HANDLE16);
}


LIB_OBJECT_NODE_STORAGE_NAME* libObjectStorageNameSearch(
               LIB_OBJECT_TABLE_STORAGE_NAME* pTable,
               S_STORAGE_NAME* pStorageName)
{
   return (LIB_OBJECT_NODE_STORAGE_NAME*)libObjectSearch(
      (LIB_OBJECT_NODE*)pTable->pRoot, 0, pStorageName, LIB_OBJECT_NODE_TYPE_STORAGE_NAME);
}

LIB_OBJECT_NODE_FILENAME* libObjectFilenameSearch(
               LIB_OBJECT_TABLE_FILENAME* pTable,
               uint8_t* pFilename,
               uint32_t  nFilenameLength)
{
   return (LIB_OBJECT_NODE_FILENAME*)libObjectSearch(
      (LIB_OBJECT_NODE*)pTable->pRoot, nFilenameLength, pFilename, LIB_OBJECT_NODE_TYPE_FILENAME);
}

/* -----------------------------------------------------------------------
   Add functions
   -----------------------------------------------------------------------*/

/* Polymorphic add function. Add the node at the end of the linked list */
static bool libObjectAdd(
   LIB_OBJECT_NODE** ppRoot, 
   LIB_OBJECT_NODE* pNew,
   LIB_OBJECT_NODE_TYPE eNodeType)
{
   LIB_OBJECT_NODE* pRoot;
   LIB_OBJECT_NODE* pLast;
   if (*ppRoot == NULL)
   {
      *ppRoot = pNew;
      pNew->pNext = pNew;
      pNew->pPrevious = pNew;
      if (eNodeType == LIB_OBJECT_NODE_TYPE_HANDLE16)
      {
         pNew->key.nHandle = 1;
      }
      return true;
   }
   else
   {
      pRoot = *ppRoot;
      pLast = pRoot->pPrevious;
      if (eNodeType == LIB_OBJECT_NODE_TYPE_HANDLE16)
      {
         if (pLast->key.nHandle == LIB_OBJECT_HANDLE16_MAX)
         {
            /* We cannot just assign last handle + 1 because it will
               overflow. So, scan the whole list */
            goto scan_list;
         }
         pNew->key.nHandle = pLast->key.nHandle + 1;
      }
      pLast->pNext = pNew;
      pNew->pPrevious = pLast;
      pNew->pNext = pRoot;
      pRoot->pPrevious = pNew;
      return true;
   }
scan_list:
   {
      LIB_OBJECT_NODE* pNode = pRoot;
      uint16_t nFreeHandle = 1;
      do
      {
         if (pNode->key.nHandle > nFreeHandle)
         {
            /* nMaxHandle+1 is not allocated. Insert pNew just before pNode */
            pNew->key.nHandle = nFreeHandle;
            pNew->pNext = pNode;
            pNew->pPrevious = pNode->pPrevious;
            pNode->pPrevious->pNext = pNew;
            pNode->pPrevious = pNew;
            if (pNode == pRoot)
            {
               /* Special case, pNew is the new root */
               *ppRoot = pNew;
            }
            return true;
         }
         pNode = pNode->pNext;
         nFreeHandle++;
      }
      while (pNode != pRoot);
      /* No free handle */
      return false;
   }
}

bool libObjectHandle16Add(
               LIB_OBJECT_TABLE_HANDLE16* pTable, 
               LIB_OBJECT_NODE_HANDLE16* pObject)
{
   return libObjectAdd(
      (LIB_OBJECT_NODE**)&pTable->pRoot,
      (LIB_OBJECT_NODE*)pObject,
      LIB_OBJECT_NODE_TYPE_HANDLE16);
}


void libObjectStorageNameAdd(
               LIB_OBJECT_TABLE_STORAGE_NAME* pTable, 
               LIB_OBJECT_NODE_STORAGE_NAME* pObject)
{
   libObjectAdd(
      (LIB_OBJECT_NODE**)&pTable->pRoot,
      (LIB_OBJECT_NODE*)pObject,
      LIB_OBJECT_NODE_TYPE_STORAGE_NAME);
}


void libObjectFilenameAdd(
               LIB_OBJECT_TABLE_FILENAME* pTable, 
               LIB_OBJECT_NODE_FILENAME* pObject)
{
   libObjectAdd(
      (LIB_OBJECT_NODE**)&pTable->pRoot,
      (LIB_OBJECT_NODE*)pObject,
      LIB_OBJECT_NODE_TYPE_FILENAME);
}


void libObjectUnindexedAdd(
         LIB_OBJECT_TABLE_UNINDEXED* pTable, 
         LIB_OBJECT_NODE_UNINDEXED* pObject)
{
   libObjectAdd(
      (LIB_OBJECT_NODE**)&pTable->pRoot,
      (LIB_OBJECT_NODE*)pObject,
      LIB_OBJECT_NODE_TYPE_UNINDEXED);
}


/* -----------------------------------------------------------------------
   Remove functions
   -----------------------------------------------------------------------*/
static void libObjectRemove(LIB_OBJECT_NODE** ppRoot, LIB_OBJECT_NODE* pObject)
{
   LIB_OBJECT_NODE* pPrevious = pObject->pPrevious;
   LIB_OBJECT_NODE* pNext = pObject->pNext;

   pPrevious->pNext = pNext;
   pNext->pPrevious = pPrevious;

   if (pPrevious == pObject)
   {
      /* Removed the last object in the table */
      *ppRoot = NULL;
   }
   else if (pObject == *ppRoot)
   {
      /* Removed the first object in the list */
      *ppRoot = pNext;
   }
}

static LIB_OBJECT_NODE* libObjectRemoveOne(LIB_OBJECT_NODE** ppRoot)
{
   if (*ppRoot == NULL)
   {
      return NULL;
   }
   else
   {
      LIB_OBJECT_NODE* pObject = *ppRoot;
      libObjectRemove(ppRoot, pObject);
      return pObject;
   }
}

void libObjectHandle16Remove(
               LIB_OBJECT_TABLE_HANDLE16* pTable, 
               LIB_OBJECT_NODE_HANDLE16* pObject)
{
   libObjectRemove((LIB_OBJECT_NODE**)&pTable->pRoot, (LIB_OBJECT_NODE*)pObject);
   pObject->nHandle = 0;
}

LIB_OBJECT_NODE_HANDLE16* libObjectHandle16RemoveOne(
               LIB_OBJECT_TABLE_HANDLE16* pTable)
{
   LIB_OBJECT_NODE_HANDLE16* pObject = (LIB_OBJECT_NODE_HANDLE16*)libObjectRemoveOne((LIB_OBJECT_NODE**)&pTable->pRoot);
   if (pObject != NULL)
   {
      pObject->nHandle = 0;
   }
   return pObject;
}

void libObjectStorageNameRemove(
               LIB_OBJECT_TABLE_STORAGE_NAME* pTable, 
               LIB_OBJECT_NODE_STORAGE_NAME* pObject)
{
   libObjectRemove((LIB_OBJECT_NODE**)&pTable->pRoot, (LIB_OBJECT_NODE*)pObject);
}

LIB_OBJECT_NODE_STORAGE_NAME* libObjectStorageNameRemoveOne(
               LIB_OBJECT_TABLE_STORAGE_NAME* pTable)
{
   return (LIB_OBJECT_NODE_STORAGE_NAME*)libObjectRemoveOne((LIB_OBJECT_NODE**)&pTable->pRoot);
}

void libObjectFilenameRemove(
               LIB_OBJECT_TABLE_FILENAME* pTable, 
               LIB_OBJECT_NODE_FILENAME* pObject)
{
   libObjectRemove((LIB_OBJECT_NODE**)&pTable->pRoot, (LIB_OBJECT_NODE*)pObject);
}

LIB_OBJECT_NODE_FILENAME* libObjectFilenameRemoveOne(
               LIB_OBJECT_TABLE_FILENAME* pTable)
{
   return (LIB_OBJECT_NODE_FILENAME*)libObjectRemoveOne((LIB_OBJECT_NODE**)&pTable->pRoot);
}

void libObjectUnindexedRemove(
         LIB_OBJECT_TABLE_UNINDEXED* pTable, 
         LIB_OBJECT_NODE_UNINDEXED* pObject)
{
   libObjectRemove((LIB_OBJECT_NODE**)&pTable->pRoot, (LIB_OBJECT_NODE*)pObject);
}

LIB_OBJECT_NODE_UNINDEXED* libObjectUnindexedRemoveOne(LIB_OBJECT_TABLE_UNINDEXED* pTable)
{
   return (LIB_OBJECT_NODE_UNINDEXED*)libObjectRemoveOne((LIB_OBJECT_NODE**)&pTable->pRoot);
}


/* -----------------------------------------------------------------------
   Get-next functions
   -----------------------------------------------------------------------*/

static LIB_OBJECT_NODE* libObjectNext(LIB_OBJECT_NODE* pRoot, LIB_OBJECT_NODE* pObject)
{
   if (pObject == NULL)
   {
      return pRoot;
   }
   else if (pObject == pRoot->pPrevious)
   {
      return NULL;
   }
   else
   {
      return pObject->pNext;
   }
}


LIB_OBJECT_NODE_HANDLE16* libObjectHandle16Next(
               LIB_OBJECT_TABLE_HANDLE16* pTable,
               LIB_OBJECT_NODE_HANDLE16* pObject)
{
   return (LIB_OBJECT_NODE_HANDLE16*)libObjectNext((LIB_OBJECT_NODE*)pTable->pRoot, (LIB_OBJECT_NODE*)pObject);
}

LIB_OBJECT_NODE_STORAGE_NAME* libObjectStorageNameNext(
               LIB_OBJECT_TABLE_STORAGE_NAME* pTable,
               LIB_OBJECT_NODE_STORAGE_NAME* pObject)
{
   return (LIB_OBJECT_NODE_STORAGE_NAME*)libObjectNext((LIB_OBJECT_NODE*)pTable->pRoot, (LIB_OBJECT_NODE*)pObject);
}

LIB_OBJECT_NODE_FILENAME* libObjectFilenameNext(
               LIB_OBJECT_TABLE_FILENAME* pTable,
               LIB_OBJECT_NODE_FILENAME* pObject)
{
   return (LIB_OBJECT_NODE_FILENAME*)libObjectNext((LIB_OBJECT_NODE*)pTable->pRoot, (LIB_OBJECT_NODE*)pObject);
}

LIB_OBJECT_NODE_UNINDEXED* libObjectUnindexedNext(
               LIB_OBJECT_TABLE_UNINDEXED* pTable,
               LIB_OBJECT_NODE_UNINDEXED* pObject)
{
   return (LIB_OBJECT_NODE_UNINDEXED*)libObjectNext((LIB_OBJECT_NODE*)pTable->pRoot, (LIB_OBJECT_NODE*)pObject);
}


