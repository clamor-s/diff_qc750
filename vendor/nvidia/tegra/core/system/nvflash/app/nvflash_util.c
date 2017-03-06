/*
 * Copyright (c) 2008-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvos.h"
#include "nvassert.h"
#include "nvflash_util.h"
#include "nvflash_util_private.h"
#include "nvapputil.h"
#include "nvutil.h"
#include "nvbuildbct.h"

static NvU32 g_sDevId = 0;

/**
 * Windows CE ROMHDR. -- copy/pased from msdn.microsoft.com, then converted
 * to use nvidia types.
 */
typedef struct ROMHDR {
  NvU32 dllfirst;
  NvU32 dlllast;
  NvU32 physfirst;
  NvU32 physlast;
  NvU32 nummods;
  NvU32 ulRAMStart;
  NvU32 ulRAMFree;
  NvU32 ulRAMEnd;
  NvU32 ulCopyEntries;
  NvU32 ulCopyOffset;
  NvU32 ulProfileLen;
  NvU32 ulProfileOffset;
  NvU32 numfiles;
  NvU32 ulKernelFlags;
  NvU32 ulFSRamPercent;
  NvU32 ulDrivglobStart;
  NvU32 ulDrivglobLen;
  NvU16 usCPUType;
  NvU16 usMiscFlags;
  void *pExtensions;
  NvU32 ulTrackingStart;
  NvU32 ulTrackingLen;
} ROMHDR;

#define ROM_SIGNATURE_OFFSET    (0x40)
#define ROM_SIGNATURE           (0x43454345)
#define CEIL_A_DIV_B(a, b) ((a)/(b) + (1 && ((a)%(b))))

#define VERIFY( exp, code ) \
    if( !(exp) ) { \
        code; \
    }

/* this value should be updated as per change in Bootrom*/
#define BOOTROM_PAGESIZE 512

NvU8 s_buf[1024 * 4];

NvBool
nvflash_util_parseimage( const char *filename, NvU32 *load, NvU32 *entry )
{
    NvBool bFound;
    NvError e;
    NvOsFileHandle hFile = 0;
    NvU32 bytes;
    NvU32 tmp;

    bFound = NV_FALSE;

    NV_CHECK_ERROR_CLEANUP(
        NvOsFopen( filename, NVOS_OPEN_READ, &hFile )
    );

    /* is this a windows rom? */
    NV_CHECK_ERROR_CLEANUP(
        NvOsFread( hFile, s_buf, sizeof(s_buf), &bytes )
    );

    /* check the signature */
    tmp = *(NvU32 *)(&s_buf[ROM_SIGNATURE_OFFSET]);
    if( tmp == ROM_SIGNATURE )
    {
        NvU32 offset;
        ROMHDR *hdr;
        offset = *(NvU32 *)&s_buf[ROM_SIGNATURE_OFFSET + 8];

        NV_CHECK_ERROR_CLEANUP(
            NvOsFseek( hFile, offset, NvOsSeek_Set )
        );

        NV_CHECK_ERROR_CLEANUP(
            NvOsFread( hFile, s_buf, sizeof(ROMHDR), &bytes )
        );

        NV_ASSERT( bytes == sizeof(ROMHDR) );

        hdr = (ROMHDR *)(&s_buf[0]);
        *load = hdr->physfirst;
        *entry = hdr->physfirst;
        bFound = NV_TRUE;
    }

    // FIXME: should check for another format, eventually

    goto clean;

fail:
    bFound = NV_FALSE;

clean:
    NvOsFclose( hFile );
    return bFound;
}

NvBool
nvflash_util_bct_munge( const char *bct_file, const char *tmp_file )
{
    NvFlashUtilHal Hal;
    Hal.devId = g_sDevId;
    if (NvFlashUtilGetAp20Hal(&Hal) || NvFlashUtilGetT30Hal(&Hal))
    {
        return Hal.MungeBct(bct_file,tmp_file);
    }
    return NV_FALSE;
}

NvU32
nvflash_get_bct_size( void )
{
    NvFlashUtilHal Hal;
    Hal.devId = g_sDevId;
    if (NvFlashUtilGetAp20Hal(&Hal) || NvFlashUtilGetT30Hal(&Hal))
    {
        return Hal.GetBctSize();
    }
    return 0;
}

void
nvflash_set_devid( NvU32 devid )
{
    g_sDevId = devid;
}

NvU32
nvflash_get_devid( void )
{
    return g_sDevId;
}

void
nvflash_get_bl_loadaddr( NvU32* entry, NvU32* addr)
{
    NvFlashUtilHal Hal;
    Hal.devId = g_sDevId;
    if (NvFlashUtilGetAp20Hal(&Hal) || NvFlashUtilGetT30Hal(&Hal))
    {
        *entry = Hal.GetBlEntry();
        *addr = Hal.GetBlAddress();
    }
}


/* don't include the header -- source orgianization with binary release
 * issue.
 */
// FIXME: should use const char * for inputs
NvError
PostProcDioOSImage(NvU8 *InputFile, NvU8 *OutputFile);
NvError
NvConvertStoreBin(NvU8 *InputFile, NvU32 BlockSize, NvU8 *OutputFile);

NvBool
nvflash_util_preprocess( const char *filename, NvFlashPreprocType type,
    NvU32 blocksize, char **new_filename )
{
    NvError e;
    NvBool b;
    char *f = 0;

    if( type == NvFlashPreprocType_DIO )
    {
        f = "post_process.dio2";

        NV_CHECK_ERROR_CLEANUP(
            PostProcDioOSImage( (NvU8 *)filename, (NvU8 *)f )
        );
    }
    else if( type == NvFlashPreprocType_StoreBin )
    {
        // FIXME: should this be a .dio2?
        f = "post_munge.dio2";

        NV_CHECK_ERROR_CLEANUP(
            NvConvertStoreBin( (NvU8 *)filename, blocksize, (NvU8 *)f )
        );
    }
    else
    {
        goto fail;
    }

    *new_filename = f;

    b = NV_TRUE;
    goto clean;

fail:
    b = NV_FALSE;

clean:
    return b;
}

NvError
nvflash_util_dumpbit(NvU8 * BitBuffer, const char *dumpOption)
{
    NvFlashUtilHal Hal;
    Hal.devId = g_sDevId;
    if (NvFlashUtilGetAp20Hal(&Hal) || NvFlashUtilGetT30Hal(&Hal))
    {
        Hal.GetDumpBit(BitBuffer, dumpOption);
        return NvSuccess;
    }
    return NvError_BadParameter;
}

int
nvflash_util_atoi(char *str, int len)
{
    int n = 0, i = 0;

    if (!str)
        return -1;
    while (i < len)
    {
        n = n * 10 + (str[i] - '0');
        i++;
    }
    return n;
}

NvU64 nvflash_util_roundsize(NvU64 Num, NvU32 SizeMultiple)
{
    NvU64 Rem;
    Rem = Num % SizeMultiple;
    if (Rem != 0)
        Num = (CEIL_A_DIV_B(Num, SizeMultiple)) * SizeMultiple;

    return Num;
}

NvBool nvflash_align_bootloader_length(const char *filename)
{
    NvBool b =NV_TRUE;
    NvError e;
    NvOsFileHandle hFile = 0;
    NvOsStatType stat;
    NvU8 *buf = 0;
    NvU32 total;
    NvU32 bytestoadd =0;
    NvU32 padsize = 0;
    char *err_str = 0;
    NvU32 ChipId =0;

    e = NvOsFopen( filename, NVOS_OPEN_APPEND, &hFile );
    VERIFY(e == NvSuccess, err_str = "file open failed"; goto fail);

    e = NvOsFstat( hFile, &stat );
    VERIFY(e == NvSuccess, err_str = "file stat failed"; goto fail);

    total = (NvU32) stat.size;
    //CHECK IF LENGTH IS MULTIPLE OF NV3P_AES_HASH_BLOCK_LEN
    if ((total % NV3P_AES_HASH_BLOCK_LEN) != 0)
        bytestoadd = NV3P_AES_HASH_BLOCK_LEN - (total % NV3P_AES_HASH_BLOCK_LEN);

    //WORKAROUND FOR HW BUG 378464
    ChipId = nvflash_get_devid();
    if (ChipId == 0x20 || ChipId == 0x30)
    {
        if((total + bytestoadd) % BOOTROM_PAGESIZE == 0)
        {
            bytestoadd += NV3P_AES_HASH_BLOCK_LEN;
            padsize += NV3P_AES_HASH_BLOCK_LEN;
        }
    }

    if (bytestoadd)
    {
        buf = NvOsAlloc(bytestoadd);
        VERIFY(buf, err_str = "buffer allocation failed"; goto fail);
        NvOsMemset(buf, 0x0, bytestoadd);
        buf[padsize] = 0x80;
        e= NvOsFseek(hFile, 0, NvOsSeek_End);
        VERIFY(e == NvSuccess, err_str = "file seek failed"; goto fail);
        e = NvOsFwrite(hFile, buf, bytestoadd);
        VERIFY(e == NvSuccess, err_str = "file write failed"; goto fail);
        NvAuPrintf("padded %d bytes to bootloader\n",bytestoadd);
    }
    goto clean;

fail:
    b = NV_FALSE;
    if (err_str)
    {
        NvAuPrintf("%s NvError 0x%x\n", err_str, e);
    }

clean:
    NvOsFclose(hFile);
    if (buf)
        NvOsFree(buf);
    return b;
}

NvError
nvflash_util_buildbct(const char *CfgFile, const char *BctFile)
{
    NvError e;
    e = NvBuildBct(CfgFile, BctFile, g_sDevId);
    if (e == NvSuccess)
        return NvSuccess;
    else
        return NvError_BadParameter;
}
