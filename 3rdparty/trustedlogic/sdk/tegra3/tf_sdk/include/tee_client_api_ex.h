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
 * This header file contains extensions to the TEE Client API that are
 * specific to the Trusted Foundations implementations
 */
#ifndef   __TEE_CLIENT_API_EX_H__
#define   __TEE_CLIENT_API_EX_H__

/* Implementation defined Login types  */
#define TEEC_LOGIN_AUTHENTICATION      0x80000000
#define TEEC_LOGIN_PRIVILEGED          0x80000002

/* Type definitions */

typedef struct
{
   uint32_t x;
   uint32_t y;
}
TEEC_TimeLimit;

typedef struct
{
   char apiDescription[65];
   char commsDescription[65];
   char TEEDescription[65];
}
TEEC_ImplementationInfo;

typedef struct
{
   uint32_t pageSize;
   uint32_t tmprefMaxSize;
   uint32_t sharedMemMaxSize;
   uint32_t nReserved3;
   uint32_t nReserved4;
   uint32_t nReserved5;
   uint32_t nReserved6;
   uint32_t nReserved7;
} 
TEEC_ImplementationLimits;

void TEEC_EXPORT TEEC_GetImplementationInfo(
   TEEC_Context*            context,
   TEEC_ImplementationInfo* description);

void TEEC_EXPORT TEEC_GetImplementationLimits(
   TEEC_ImplementationLimits* limits);

void TEEC_EXPORT TEEC_GetTimeLimit(
    TEEC_Context*    context,
    uint32_t         timeout,
    TEEC_TimeLimit*  timeLimit);

TEEC_Result TEEC_EXPORT TEEC_OpenSessionEx (
    TEEC_Context*         context,
    TEEC_Session*         session,
    const TEEC_TimeLimit* timeLimit,
    const TEEC_UUID*      destination,
    uint32_t              connectionMethod,
    void*                 connectionData,
    TEEC_Operation*       operation,
    uint32_t*             errorOrigin);

TEEC_Result TEEC_EXPORT TEEC_InvokeCommandEx(
    TEEC_Session*         session,
    const TEEC_TimeLimit* timeLimit,
    uint32_t              commandID,
    TEEC_Operation*       operation,
    uint32_t*             errorOrigin);

TEEC_Result TEEC_EXPORT TEEC_ReadSignatureFile(
   void**    ppSignatureFile,
   uint32_t* pnSignatureFileLength);

#endif /* __TEE_CLIENT_API_EX_H__ */
