/*
 * Copyright (c) 2008-2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvassert.h"
#include "nvos.h"
#include "nvutil.h"
#include "nvrm_hardware_access.h"
#include "nvrm_message.h"
#include "nvrm_rpc.h"
#include "nvrm_moduleloader.h"
#include "nvrm_moduleloader_private.h"
#include "ap20/ap20rm_structure.h"
#include "ap20/ap20rm_private.h"

/* Nothing sacrosanct about these values.
 * The only requirement is that SAMPLING_BUCKETS*BYTES_PER_BUCKET be >= 32mb-128k.
 * This is needed to ensure that all of internal memory is profiled (no
 * external memory yet.
 */
#define MAX_STR_LEN 256
#define MAX_MODULES_PROFILED 8
#define MAX_SYMBOLS_PROFILED 256
#define MAX_SAMPLING_BUCKETS 65536
#define MAX_BYTES_PER_BUCKET 512

/* NOTES:
 * Space required by profiling array = 65536 * 2 = 128k bytes.
 * Total Space Profiled = Carveoutsize - 128k.
 */

//Symbol table structures
typedef struct
{
    char *strtab;
    char *symtab;
    NvU32 numSymbols;
} NvRmModuleSymData;

typedef struct
{
    char symName[MAX_STR_LEN];
    NvU32 numExecuted;
} NvRmSymProfile;

/**
  * This function begins profiling a module beginning at loadOffset.
  *
  * @param elfSourceHandle - ELF file data
  * @param loadOffset - Offset at which a the module was loaded
  *
  * @return Returns NvSuccess on success
  */
NvError NvRmProfilerStart(NvOsFileHandle elfSourceHandle, NvU32 loadOffset);

/**
  * This function stops all profiling and creates a human readable
  * profile table for all modules profiled.
  *
  */
void NvRmProfilerStop( void );



