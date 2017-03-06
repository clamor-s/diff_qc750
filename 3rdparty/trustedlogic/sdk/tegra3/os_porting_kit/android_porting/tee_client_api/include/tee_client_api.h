/*
 * Copyright (c) 2010 Trusted Logic S.A.
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

/*
 * This header file corresponds to V1.0 of the GlobalPlatform 
 * TEE Client API Specification
 */
#ifndef   __TEE_CLIENT_API_H__
#define   __TEE_CLIENT_API_H__

#include "s_type.h"
#include "s_error.h"

#ifndef TEEC_EXPORT
#define TEEC_EXPORT
#endif

/* The header tee_client_api_imp.h must define implementation-dependent
   types, constants and macros.
   
   The implementation-dependent types are:
     - TEEC_Context_IMP
     - TEEC_Session_IMP
     - TEEC_SharedMemory_IMP
     - TEEC_Operation_IMP

   The implementation-dependent constants are:
     - TEEC_CONFIG_SHAREDMEM_MAX_SIZE  
   The implementation-dependent macros are:
     - TEEC_PARAM_TYPES
*/
#include "tee_client_api_imp.h"

/* Type definitions */
typedef struct TEEC_Context
{
   TEEC_Context_IMP imp;
} TEEC_Context;

typedef struct TEEC_Session
{
   TEEC_Session_IMP imp; 
} TEEC_Session;

typedef struct TEEC_SharedMemory
{
    void*    buffer;
    size_t   size;
    uint32_t flags;
    TEEC_SharedMemory_IMP imp;
} TEEC_SharedMemory;

typedef struct
{
    void*     buffer;
    size_t    size;
} TEEC_TempMemoryReference;

typedef struct
{
    TEEC_SharedMemory * parent;
    size_t    size;
    size_t    offset;
} TEEC_RegisteredMemoryReference;

typedef struct
{
    uint32_t   a;
    uint32_t   b;
} TEEC_Value;

typedef union
{
   TEEC_TempMemoryReference        tmpref;
   TEEC_RegisteredMemoryReference  memref;
   TEEC_Value                      value;
} TEEC_Parameter;

typedef struct TEEC_Operation
{
    volatile uint32_t    started;
    uint32_t             paramTypes;
    TEEC_Parameter       params[4];   
    
    TEEC_Operation_IMP   imp;
} TEEC_Operation;


#define TEEC_ORIGIN_API                      0x00000001
#define TEEC_ORIGIN_COMMS                    0x00000002
#define TEEC_ORIGIN_TEE                      0x00000003
#define TEEC_ORIGIN_TRUSTED_APP              0x00000004

#define TEEC_MEM_INPUT                       0x00000001
#define TEEC_MEM_OUTPUT                      0x00000002

#define TEEC_NONE                     0x0
#define TEEC_VALUE_INPUT              0x1
#define TEEC_VALUE_OUTPUT             0x2
#define TEEC_VALUE_INOUT              0x3
#define TEEC_MEMREF_TEMP_INPUT        0x5
#define TEEC_MEMREF_TEMP_OUTPUT       0x6
#define TEEC_MEMREF_TEMP_INOUT        0x7
#define TEEC_MEMREF_WHOLE             0xC
#define TEEC_MEMREF_PARTIAL_INPUT     0xD
#define TEEC_MEMREF_PARTIAL_OUTPUT    0xE
#define TEEC_MEMREF_PARTIAL_INOUT     0xF

#define TEEC_LOGIN_PUBLIC                    0x00000000
#define TEEC_LOGIN_USER                      0x00000001
#define TEEC_LOGIN_GROUP                     0x00000002
#define TEEC_LOGIN_APPLICATION               0x00000004
#define TEEC_LOGIN_USER_APPLICATION          0x00000005
#define TEEC_LOGIN_GROUP_APPLICATION         0x00000006

TEEC_Result TEEC_EXPORT TEEC_InitializeContext(
    const char*   name,
    TEEC_Context* context);

void TEEC_EXPORT TEEC_FinalizeContext(
    TEEC_Context* context);

TEEC_Result TEEC_EXPORT TEEC_RegisterSharedMemory(
    TEEC_Context*      context,
    TEEC_SharedMemory* sharedMem);

TEEC_Result TEEC_EXPORT TEEC_AllocateSharedMemory(
    TEEC_Context*      context,
    TEEC_SharedMemory* sharedMem);

void TEEC_EXPORT TEEC_ReleaseSharedMemory (
    TEEC_SharedMemory* sharedMem);

TEEC_Result TEEC_EXPORT TEEC_OpenSession (
    TEEC_Context*    context,
    TEEC_Session*    session,
    const TEEC_UUID* destination,
    uint32_t         connectionMethod,
    void*            connectionData,
    TEEC_Operation*  operation,
    uint32_t*        errorOrigin);

void TEEC_EXPORT TEEC_CloseSession (
    TEEC_Session* session);

TEEC_Result TEEC_EXPORT TEEC_InvokeCommand(
    TEEC_Session*     session,
    uint32_t          commandID,
    TEEC_Operation*   operation,
    uint32_t*         errorOrigin);

void TEEC_EXPORT TEEC_RequestCancellation(
    TEEC_Operation* operation);

#include "tee_client_api_ex.h"

#endif /* __TEE_CLIENT_API_H__ */
