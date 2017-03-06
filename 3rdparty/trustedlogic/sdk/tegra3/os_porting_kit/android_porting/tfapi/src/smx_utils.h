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
#ifndef __SMX_UTILS_H__
#define __SMX_UTILS_H__

#include "smapi.h"

#include "smx_defs.h"
#include "smx_encoding.h"

#include "s_type.h"

/*----------------------------------------------------------------------------
 * Operation Action Type
 *----------------------------------------------------------------------------*/
#define MD_VAR_NOT_USED(variable)  do{(void)(variable);}while(0);

#define SM_ACTION_PERFORM     0x10000000
#define SM_ACTION_CANCEL      0x10000001
#define SM_ACTION_RELEASE     0x10000002

typedef uint32_t SM_ACTION_TYPE;

void SMXCharItoA(
      uint32_t nValue,
      uint32_t nRadix,
      OUT uint8_t* pBuffer,
      uint32_t nBufferCapacity,
      OUT uint32_t* pnBufferSize,
      char nFill);

void SMXWcharItoA(
      uint32_t nValue,
      uint32_t nRadix,
      wchar_t *pBuffer,
      uint32_t nBufferCapacity,
      uint32_t *pnBufferSize);

bool SMXCompareWcharZString(
      const wchar_t *pString1,
      const wchar_t *pString2);

uint32_t SMXGetWcharZStringLength(
      IN const wchar_t *pWcharString);

void SMXStr2Wchar(wchar_t* pWchar, char* pStr, uint32_t nlength);
void SMXWchar2Str(char* pStr, wchar_t* pWchar, uint32_t nlength);

void _TRACE_ERROR(const char* format, ...);
void _TRACE_WARNING(const char* format, ...);
void _TRACE_INFO(const char* format, ...);

#ifdef NDEBUG
/* Compile-out the traces */
#define TRACE_ERROR(...)
#define TRACE_WARNING(...)
#define TRACE_INFO(...)
#else
#define TRACE_ERROR _TRACE_ERROR
#define TRACE_WARNING _TRACE_WARNING
#define TRACE_INFO _TRACE_INFO
#endif


uint64_t SMXGetCurrentTime(void);

SM_HANDLE SMXGetHandleFromPointer(void* pObject);

void* SMXGetPointerFromHandle(SM_HANDLE hObject);

SM_ERROR SMXLockObjectFromHandle(
      SM_HANDLE hService,
      SMX_NODE** ppObject,
      uint32_t nType);

SM_ERROR SMXLockOperationFromHandle(
      SM_HANDLE hOperation,
      SM_OPERATION** ppOperation,
      SM_ACTION_TYPE nActionType);

SM_ERROR SMXUnLockObject(SMX_NODE *pObject);

void SMXListAdd(SMX_LIST *pList,SMX_NODE* pNode);
void SMXListRemove(SMX_LIST *pList,SMX_NODE* pNode);

void* SMXAlloc(
               SM_HANDLE hObject,
               uint32_t nSize,
               bool bUseDeviceLock);

void* SMXReAlloc(
                 SM_HANDLE hObject,
                 void* pBuffer,
                 uint32_t nSize,
                 bool bUseDeviceLock);

SMX_NODE *SMXDeviceContextAlloc(
                            SM_DEVICE_CONTEXT* pDevice,
                            uint32_t nSize);
SMX_NODE *SMXDeviceContextReAlloc(
                            SM_DEVICE_CONTEXT* pDevice,
                            SMX_NODE* pNode,
                            uint32_t nSize);
void SMXDeviceContextFree(
                          SM_DEVICE_CONTEXT* pDevice,
                          SMX_NODE* pNode);
void SMXDeviceContextRemoveAll(
                               SM_DEVICE_CONTEXT *pDevice);

bool SMXEncoderBufferReAllocated(SMX_ENCODER* pEncoder);

void SMXEncoderAutoCloseSequences(SMX_ENCODER* pEncoder);

SM_ERROR TeecErrorToSmapiError(  TEEC_Result nTeecResult,
                                 int32_t nErrorOrigin,
                                 SM_ERROR* pnServiceErrorCode);

#endif  /* !defined(__SMX_UTILS_H__) */
