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
 * This header file defines the implementation-dependent types,
 * constants and macros for all the Trusted Foundations implementations
 * of the TEE Client API
 */
#ifndef   __TEE_CLIENT_API_IMP_H__
#define   __TEE_CLIENT_API_IMP_H__

#include "s_type.h"
#include "s_error.h"

typedef struct
{
   uint32_t             _hConnection;
}
TEEC_Context_IMP;

typedef struct
{
   struct TEEC_Context* _pContext;
   S_HANDLE             _hClientSession;
}
TEEC_Session_IMP;

typedef struct
{
   struct TEEC_Context* _pContext;
   S_HANDLE             _hBlock;
   bool                 _bAllocated;
}
TEEC_SharedMemory_IMP;

typedef struct
{
   struct TEEC_Context* _pContext;
    uint32_t            _hSession;
}
TEEC_Operation_IMP;

/* There is no natural, compile-time limit on the shared memory, but a specific
   implementation may introduce a limit (in particular on TrustZone) */
#define TEEC_CONFIG_SHAREDMEM_MAX_SIZE ((size_t)0xFFFFFFFF)

#define TEEC_PARAM_TYPES(entry0Type, entry1Type, entry2Type, entry3Type) \
   ((entry0Type) | ((entry1Type) << 4) | ((entry2Type) << 8) | ((entry3Type) << 12))


#endif /* __TEE_CLIENT_API_IMP_H__ */
