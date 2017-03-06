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

#ifndef __SERVICE_DELEGATION_PROTOCOL_H__
#define __SERVICE_DELEGATION_PROTOCOL_H__

#include "s_type.h"

/* -----------------------------------------------------------------------------
   UUID
----------------------------------------------------------------------------- */
/** Service Identifier: {71139E60-ECAE-11DF-98CF-0800200C9A66} */
#define SERVICE_DELEGATION_UUID { 0x71139e60, 0xecae, 0x11df, { 0x98, 0xcf, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 } }


/* -----------------------------------------------------------------------------
   Command identifiers
----------------------------------------------------------------------------- */
/* See your Product Reference Manual for a specification of the delegation protocol */

/**
 * Command identifier : get instruction
 *
 */
#define SERVICE_DELEGATION_GET_INSTRUCTIONS      0x00000002

/* Instruction codes */
#define DELEGATION_INSTRUCTION_SHUTDOWN             0xF0
#define DELEGATION_INSTRUCTION_NOTIFY               0xE0

/* Partition-specific instruction codes (high-nibble encodes the partition identifier) */
#define DELEGATION_INSTRUCTION_PARTITION_CREATE     0x01
#define DELEGATION_INSTRUCTION_PARTITION_OPEN       0x02
#define DELEGATION_INSTRUCTION_PARTITION_READ       0x03
#define DELEGATION_INSTRUCTION_PARTITION_WRITE      0x04
#define DELEGATION_INSTRUCTION_PARTITION_SET_SIZE   0x05
#define DELEGATION_INSTRUCTION_PARTITION_SYNC       0x06
#define DELEGATION_INSTRUCTION_PARTITION_CLOSE      0x07
#define DELEGATION_INSTRUCTION_PARTITION_DESTROY    0x08

#define DELEGATION_NOTIFY_TYPE_ERROR                0x000000E1
#define DELEGATION_NOTIFY_TYPE_WARNING              0x000000E2
#define DELEGATION_NOTIFY_TYPE_INFO                 0x000000E3
#define DELEGATION_NOTIFY_TYPE_DEBUG                0x000000E4

#ifdef SUPPORT_RPMB_PARTITION
#define RPMB_PARTITION_ID                 3
#endif
#ifdef SUPPORT_SUPER_PARTITION
#define SUPER_PARTITION_ID                15
#define SUPER_PARTITION_MAX_SIZE          2 /* 2 super sector */ * 4 /* maximum number of partition */
#endif

typedef struct
{
   uint32_t nInstructionID;
} DELEGATION_GENERIC_INSTRUCTION;

typedef struct
{
   uint32_t nInstructionID;
   uint32_t nMessageType;
   uint32_t nMessageSize;
   char     nMessage[1];
} DELEGATION_NOTIFY_INSTRUCTION;

typedef struct
{
   uint32_t nInstructionID;
   uint32_t nSectorID;
   uint32_t nWorkspaceOffset;
} DELEGATION_RW_INSTRUCTION;

#ifdef SUPPORT_RPMB_PARTITION
typedef struct
{
   uint32_t nInstructionID;
   uint8_t  nMAC[32];
   uint32_t nWorkspaceOffset[16];
   uint8_t  pNonce[16];
   uint32_t nMC;
   uint16_t nAddr;
   uint16_t nBlockCount;
   uint16_t nResult;
   uint16_t nRequest;
} DELEGATION_RPMB_INSTRUCTION;
#endif

typedef struct
{
   uint32_t nInstructionID;
   uint32_t nNewSize;
} DELEGATION_SET_SIZE_INSTRUCTION;

typedef union
{
   DELEGATION_GENERIC_INSTRUCTION    sGeneric;
   DELEGATION_NOTIFY_INSTRUCTION     sNotify;
   DELEGATION_RW_INSTRUCTION         sReadWrite;
   DELEGATION_SET_SIZE_INSTRUCTION   sSetSize;
#ifdef SUPPORT_RPMB_PARTITION
   DELEGATION_RPMB_INSTRUCTION       sAuthRW;
#endif
} DELEGATION_INSTRUCTION;

typedef struct
{
   uint32_t    nSyncExecuted;
   uint32_t    nPartitionErrorStates[16];
   uint32_t    nPartitionOpenSizes[16];
} DELEGATION_ADMINISTRATIVE_DATA;

#endif /* __SERVICE_DELEGATION_PROTOCOL_H__ */
