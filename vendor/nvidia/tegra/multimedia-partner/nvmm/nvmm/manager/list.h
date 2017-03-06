/*
 * Copyright (c) 2007 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_LIST_H
#define INCLUDED_LIST_H

#include "nvcommon.h"


struct ListNode;
typedef struct ListNode  *pListNode;


typedef  struct List{
    NvU32       size;
    pListNode   head;
    pListNode   current;
 } List;

/******************************************************************************
 * Linked List Utility Functions
 *****************************************************************************/

NvError 
ListCreate(
 List  * pList);

NvError 
ListDestroy(
 List  * pList);

NvError
ListAddNode(
 List  * pList, 
 void  * data );

void *ListFindNode(
 List  * pList, 
 void  * data );

NvError
ListDeleteNode(
 List  * pList, 
 void  * data );

NvError
ListGetNextNode(
    List *pList, 
    void  **data);

NvError 
ListGetCount(
    List *pList,
    NvU32 *Count);

NvError
ListReset(
    List *pList);

#endif 
