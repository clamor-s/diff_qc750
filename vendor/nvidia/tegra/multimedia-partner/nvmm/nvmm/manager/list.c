/* Copyright (c) 2006 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an
 * express license agreement from NVIDIA Corporation is strictly prohibited.
 */

// list.c
//

#include "nvmm_manager_internal.h"
#include "string.h"
#include "list.h"
#include "nvassert.h"

typedef  struct ListNode {
    void  * data;
    struct ListNode  *next;
}  ListNode;

/******************************************************************************
 * Linked List Utility Functions
 *****************************************************************************/

NvError
ListCreate(List  *pList)
{
    NvError status = NvSuccess;
    NV_ASSERT(  pList != NULL );
    NvOsMemset( pList, 0, sizeof( List ) );
    return status;
}

NvError 
ListDestroy(List  *pList)
{
    pListNode  pNode, pNext;
    NvError status = NvSuccess;

    pNode = pList->head;
    while ( pNode != NULL ) {
        pNext = pNode->next;
        NvOsFree( pNode );
        pNode = pNext;
    }

    NvOsMemset( pList, 0, sizeof( List ) );
    return status;
}

NvError
ListAddNode(List  *pList, void  * data )
{
    pListNode  * pCurrPtr, pTmp;
    NvError status = NvSuccess;

    NV_ASSERT( pList != NULL );

    pTmp = (pListNode)NvOsAlloc( sizeof( ListNode ) );
    if (!pTmp)
    {
        status = NvError_InsufficientMemory;
        return status;
    }
    NvOsMemset( pTmp, 0, sizeof( ListNode ) );
    pTmp->data = data;

    pCurrPtr = &(pList->head);
    while  (*pCurrPtr != NULL)
           pCurrPtr = &((*pCurrPtr)->next);

    *pCurrPtr = pTmp; 
    pList->size++;

    if (pList->current == NULL)
    {
        pList->current = pList->head;
    }
    return status;
}

void *
ListFindNode(List  *pList, void  *data )
{
    pListNode  * pCurrPtr;

   NV_ASSERT( pList != NULL );
   
    pCurrPtr = &(pList->head);
    while  ( *pCurrPtr != NULL ) 
    {
        if  ( (*pCurrPtr)->data == data  ) 
        {
                return data;
        }
        pCurrPtr = &((*pCurrPtr)->next);
    }
   return NULL;
}

NvError
ListDeleteNode(List  *pList, void  * data )
{
    pListNode  * pCurrPtr, pTmp;
    NvError status = NvSuccess;

   NV_ASSERT( pList != NULL );
   
    pCurrPtr = &(pList->head);
    while  ( *pCurrPtr != NULL ) 
    {
        if  ( (*pCurrPtr)->data == data  ) 
        {
            pTmp = *pCurrPtr;
            *pCurrPtr = ((*pCurrPtr)->next);
            NvOsFree( pTmp );
                return status;
        }
        pCurrPtr = &((*pCurrPtr)->next);
    }
    return status;
}

NvError
ListGetNextNode(List *pList, void  **data)
{
    pListNode pCurrNode;

    pCurrNode = pList->current;
    if (pCurrNode != NULL)
    {
        *data = pCurrNode->data;
        pList->current = pCurrNode->next;
    }
    else
    {
        *data = NULL;
    }
    return NvSuccess;
}

NvError 
ListGetCount(
    List *pList,
    NvU32 *Count)
{
    *Count = pList->size;
    return NvSuccess;
}

NvError
ListReset(
    List *pList)
{
    pList->current = pList->head;
    return NvSuccess;
}


