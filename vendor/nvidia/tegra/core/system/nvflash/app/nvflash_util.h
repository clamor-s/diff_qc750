/*
 * Copyright (c) 2008-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVFLASH_UTIL_H
#define INCLUDED_NVFLASH_UTIL_H

#include "nvcommon.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#define NV3P_AES_HASH_BLOCK_LEN 16

/**
 * Attempt to figure out an image entry point and load address.
 *
 * @param filename The file to parse
 * @param load The load address
 * @param entry The entry address
 *
 * Returns NV_TRUE is the load address and entry point were found,
 * NV_FALSE otherwise.
 */
NvBool
nvflash_util_parseimage( const char *filename, NvU32 *load, NvU32 *entry );

/**
 * Attempts to fix broken BCT files.
 */
NvBool
nvflash_util_bct_munge( const char *bct_file, const char *tmp_file );
/**
 * Gets the bct size
 */
NvU32
nvflash_get_bct_size( void );
/**
 * sets the device ID
 */
void
nvflash_set_devid( NvU32 devid );

/**
 * gets the device ID
 */
NvU32
nvflash_get_devid( void );

/**
 * gets default entry and load address for bootloader
 */
void
nvflash_get_bl_loadaddr( NvU32* entry, NvU32* addr);

typedef struct NvFlashBlobHeaderRec
{
    enum
    {
        NvFlash_Version = 0x1,
        NvFlash_RCM1,
        NvFlash_RCM2,
        NvFlash_BlHash,
        NvFlash_Force32 = 0x7FFFFFFF,
    } BlobHeaderType;
    NvU32 length;
    NvU32 reserved1;
    NvU32 reserved2;
    NvU8 resbuff[NV3P_AES_HASH_BLOCK_LEN];
} NvFlashBlobHeader;

typedef enum
{
    NvFlashPreprocType_DIO = 1,
    NvFlashPreprocType_StoreBin,

    NvFlashPreprocType_Force32 = 0x7FFFFFFF,
} NvFlashPreprocType;

/**
 * Converts a .dio or a .store.bin file to the new format (dio2) by inserting
 * the required compaction blocks.
 *
 * @param filename The dio or the .store.bin file to convert
 * @param type File type: dio or store.bin.
 * @param blocksize The blocksize of the media
 * @param new_filename The converted file name
 */
NvBool
nvflash_util_preprocess( const char *filename, NvFlashPreprocType type,
    NvU32 blocksize, char **new_filename );

NvError
nvflash_util_dumpbit(NvU8 * BitBuffer, const char *dumpOption);

int
nvflash_util_atoi(char *str, int len);

/**
 * This function converts input size into the nearest multiple
 * size.
 *
 * @param num input size to be rounded off to the nearest multiple
 * @param SizeMultiple the size multiple used to round off num
 */
NvU64
nvflash_util_roundsize(NvU64 Num, NvU32 SizeMultiple);

/**
 * This function aligns the size of bootloader to multiple of
 * 16 bytes.
 *
 * @param filename  to be aligned to multiple of 16
 */
NvBool nvflash_align_bootloader_length(const char *filename);

/**
 * Uses the libnvbuildbct library to create the bctfile from the cfgfile.
 */
NvError
nvflash_util_buildbct(const char *CfgFile, const char *BctFile);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVFLASH_UTIL_H */
