/*
 * Copyright (c) 2009-2012 NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#define NV_IDL_IS_STUB

#include "nvcommon.h"
#include "nvos.h"
#include "nvassert.h"
#include "nvidlcmd.h"
#include "nvrm_module.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

static NvOsFileHandle gs_IoctlFile;
static NvU32 gs_IoctlCode;

#define AP20_ID 0x20
#define T30_ID  0x30

typedef struct ChipIdRec
{
    NvU32 Id;
    NvU32 Revision;
} ChipId;

static ChipId gs_ChipId;

static void NvIdlIoctlFile( void )
{
    if( !gs_IoctlFile )
    {
        gs_IoctlFile = NvRm_NvIdlGetIoctlFile(); 
        gs_IoctlCode = NvRm_NvIdlGetIoctlCode(); 
    }
}

#define OFFSET( s, e )       (NvU32)(void *)(&(((s*)0)->e))


// NvRm Package
typedef enum
{
    NvRm_Invalid = 0,
    NvRm_nvrm_xpc,
    NvRm_nvrm_transport,
    NvRm_nvrm_memctrl,
    NvRm_nvrm_pcie,
    NvRm_nvrm_pwm,
    NvRm_nvrm_keylist,
    NvRm_nvrm_pmu,
    NvRm_nvrm_diag,
    NvRm_nvrm_pinmux,
    NvRm_nvrm_analog,
    NvRm_nvrm_owr,
    NvRm_nvrm_i2c,
    NvRm_nvrm_spi,
    NvRm_nvrm_interrupt,
    NvRm_nvrm_dma,
    NvRm_nvrm_power,
    NvRm_nvrm_gpio,
    NvRm_nvrm_module,
    NvRm_nvrm_memmgr,
    NvRm_nvrm_init,
    NvRm_Num,
    NvRm_Force32 = 0x7FFFFFFF,
} NvRm;

typedef struct NvRegw08_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle rm;
    NvRmModuleID aperture;
    NvU32 offset;
    NvU8 data;
} NV_ALIGN(4) NvRegw08_in;

typedef struct NvRegw08_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRegw08_inout;

typedef struct NvRegw08_out_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRegw08_out;

typedef struct NvRegw08_params_t
{
    NvRegw08_in in;
    NvRegw08_inout inout;
    NvRegw08_out out;
} NvRegw08_params;

typedef struct NvRegr08_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hDeviceHandle;
    NvRmModuleID aperture;
    NvU32 offset;
} NV_ALIGN(4) NvRegr08_in;

typedef struct NvRegr08_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRegr08_inout;

typedef struct NvRegr08_out_t
{
    NvU8 ret_;
} NV_ALIGN(4) NvRegr08_out;

typedef struct NvRegr08_params_t
{
    NvRegr08_in in;
    NvRegr08_inout inout;
    NvRegr08_out out;
} NvRegr08_params;

typedef struct NvRegrb_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hRmDeviceHandle;
    NvRmModuleID aperture;
    NvU32 num;
    NvU32 offset;
    NvU32  * values;
} NV_ALIGN(4) NvRegrb_in;

typedef struct NvRegrb_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRegrb_inout;

typedef struct NvRegrb_out_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRegrb_out;

typedef struct NvRegrb_params_t
{
    NvRegrb_in in;
    NvRegrb_inout inout;
    NvRegrb_out out;
} NvRegrb_params;

typedef struct NvRegwb_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hRmDeviceHandle;
    NvRmModuleID aperture;
    NvU32 num;
    NvU32 offset;
    NvU32  * values;
} NV_ALIGN(4) NvRegwb_in;

typedef struct NvRegwb_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRegwb_inout;

typedef struct NvRegwb_out_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRegwb_out;

typedef struct NvRegwb_params_t
{
    NvRegwb_in in;
    NvRegwb_inout inout;
    NvRegwb_out out;
} NvRegwb_params;

typedef struct NvRegwm_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hRmDeviceHandle;
    NvRmModuleID aperture;
    NvU32 num;
    NvU32  * offsets;
    NvU32  * values;
} NV_ALIGN(4) NvRegwm_in;

typedef struct NvRegwm_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRegwm_inout;

typedef struct NvRegwm_out_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRegwm_out;

typedef struct NvRegwm_params_t
{
    NvRegwm_in in;
    NvRegwm_inout inout;
    NvRegwm_out out;
} NvRegwm_params;

typedef struct NvRegrm_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hRmDeviceHandle;
    NvRmModuleID aperture;
    NvU32 num;
    NvU32  * offsets;
    NvU32  * values;
} NV_ALIGN(4) NvRegrm_in;

typedef struct NvRegrm_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRegrm_inout;

typedef struct NvRegrm_out_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRegrm_out;

typedef struct NvRegrm_params_t
{
    NvRegrm_in in;
    NvRegrm_inout inout;
    NvRegrm_out out;
} NvRegrm_params;

typedef struct NvRegw_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hDeviceHandle;
    NvRmModuleID aperture;
    NvU32 offset;
    NvU32 data;
} NV_ALIGN(4) NvRegw_in;

typedef struct NvRegw_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRegw_inout;

typedef struct NvRegw_out_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRegw_out;

typedef struct NvRegw_params_t
{
    NvRegw_in in;
    NvRegw_inout inout;
    NvRegw_out out;
} NvRegw_params;

typedef struct NvRegr_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hDeviceHandle;
    NvRmModuleID aperture;
    NvU32 offset;
} NV_ALIGN(4) NvRegr_in;

typedef struct NvRegr_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRegr_inout;

typedef struct NvRegr_out_t
{
    NvU32 ret_;
} NV_ALIGN(4) NvRegr_out;

typedef struct NvRegr_params_t
{
    NvRegr_in in;
    NvRegr_inout inout;
    NvRegr_out out;
} NvRegr_params;

typedef struct NvRmGetRandomBytes_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hRmDeviceHandle;
    NvU32 NumBytes;
    void* pBytes;
} NV_ALIGN(4) NvRmGetRandomBytes_in;

typedef struct NvRmGetRandomBytes_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRmGetRandomBytes_inout;

typedef struct NvRmGetRandomBytes_out_t
{
    NvError ret_;
} NV_ALIGN(4) NvRmGetRandomBytes_out;

typedef struct NvRmGetRandomBytes_params_t
{
    NvRmGetRandomBytes_in in;
    NvRmGetRandomBytes_inout inout;
    NvRmGetRandomBytes_out out;
} NvRmGetRandomBytes_params;

typedef struct NvRmQueryChipUniqueId_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hDevHandle;
    NvU32 IdSize;
    void* pId;
} NV_ALIGN(4) NvRmQueryChipUniqueId_in;

typedef struct NvRmQueryChipUniqueId_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRmQueryChipUniqueId_inout;

typedef struct NvRmQueryChipUniqueId_out_t
{
    NvError ret_;
} NV_ALIGN(4) NvRmQueryChipUniqueId_out;

typedef struct NvRmQueryChipUniqueId_params_t
{
    NvRmQueryChipUniqueId_in in;
    NvRmQueryChipUniqueId_inout inout;
    NvRmQueryChipUniqueId_out out;
} NvRmQueryChipUniqueId_params;

typedef struct NvRmModuleGetCapabilities_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hDeviceHandle;
    NvRmModuleID Module;
    NvRmModuleCapability  * pCaps;
    NvU32 NumCaps;
} NV_ALIGN(4) NvRmModuleGetCapabilities_in;

typedef struct NvRmModuleGetCapabilities_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRmModuleGetCapabilities_inout;

typedef struct NvRmModuleGetCapabilities_out_t
{
    NvError ret_;
    void* Capability;
} NV_ALIGN(4) NvRmModuleGetCapabilities_out;

typedef struct NvRmModuleGetCapabilities_params_t
{
    NvRmModuleGetCapabilities_in in;
    NvRmModuleGetCapabilities_inout inout;
    NvRmModuleGetCapabilities_out out;
} NvRmModuleGetCapabilities_params;

typedef struct NvRmModuleResetWithHold_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hRmDeviceHandle;
    NvRmModuleID Module;
    NvBool bHold;
} NV_ALIGN(4) NvRmModuleResetWithHold_in;

typedef struct NvRmModuleResetWithHold_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRmModuleResetWithHold_inout;

typedef struct NvRmModuleResetWithHold_out_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRmModuleResetWithHold_out;

typedef struct NvRmModuleResetWithHold_params_t
{
    NvRmModuleResetWithHold_in in;
    NvRmModuleResetWithHold_inout inout;
    NvRmModuleResetWithHold_out out;
} NvRmModuleResetWithHold_params;

typedef struct NvRmModuleReset_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hRmDeviceHandle;
    NvRmModuleID Module;
} NV_ALIGN(4) NvRmModuleReset_in;

typedef struct NvRmModuleReset_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRmModuleReset_inout;

typedef struct NvRmModuleReset_out_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRmModuleReset_out;

typedef struct NvRmModuleReset_params_t
{
    NvRmModuleReset_in in;
    NvRmModuleReset_inout inout;
    NvRmModuleReset_out out;
} NvRmModuleReset_params;

typedef struct NvRmModuleGetNumInstances_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hRmDeviceHandle;
    NvRmModuleID Module;
} NV_ALIGN(4) NvRmModuleGetNumInstances_in;

typedef struct NvRmModuleGetNumInstances_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRmModuleGetNumInstances_inout;

typedef struct NvRmModuleGetNumInstances_out_t
{
    NvU32 ret_;
} NV_ALIGN(4) NvRmModuleGetNumInstances_out;

typedef struct NvRmModuleGetNumInstances_params_t
{
    NvRmModuleGetNumInstances_in in;
    NvRmModuleGetNumInstances_inout inout;
    NvRmModuleGetNumInstances_out out;
} NvRmModuleGetNumInstances_params;

typedef struct NvRmModuleGetBaseAddress_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hRmDeviceHandle;
    NvRmModuleID Module;
} NV_ALIGN(4) NvRmModuleGetBaseAddress_in;

typedef struct NvRmModuleGetBaseAddress_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRmModuleGetBaseAddress_inout;

typedef struct NvRmModuleGetBaseAddress_out_t
{
    NvRmPhysAddr pBaseAddress;
    NvU32 pSize;
} NV_ALIGN(4) NvRmModuleGetBaseAddress_out;

typedef struct NvRmModuleGetBaseAddress_params_t
{
    NvRmModuleGetBaseAddress_in in;
    NvRmModuleGetBaseAddress_inout inout;
    NvRmModuleGetBaseAddress_out out;
} NvRmModuleGetBaseAddress_params;

typedef struct NvRmModuleGetModuleInfo_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hDevice;
    NvRmModuleID module;
    NvRmModuleInfo  * pModuleInfo;
} NV_ALIGN(4) NvRmModuleGetModuleInfo_in;

typedef struct NvRmModuleGetModuleInfo_inout_t
{
    NvU32 pNum;
} NV_ALIGN(4) NvRmModuleGetModuleInfo_inout;

typedef struct NvRmModuleGetModuleInfo_out_t
{
    NvError ret_;
} NV_ALIGN(4) NvRmModuleGetModuleInfo_out;

typedef struct NvRmModuleGetModuleInfo_params_t
{
    NvRmModuleGetModuleInfo_in in;
    NvRmModuleGetModuleInfo_inout inout;
    NvRmModuleGetModuleInfo_out out;
} NvRmModuleGetModuleInfo_params;

void NvRegw08( NvRmDeviceHandle rm, NvRmModuleID aperture, NvU32 offset, NvU8 data )
{
    NvError err_;
    NvU32 i_size_ = OFFSET(NvRegw08_params, inout);
    NvU32 io_size_ = OFFSET(NvRegw08_params, out) - OFFSET(NvRegw08_params, inout);
    NvU32 o_size_ = sizeof(NvRegw08_params) - OFFSET(NvRegw08_params, out);
    NvRegw08_params p_;

    p_.in.package_ = NvRm_nvrm_module;
    p_.in.function_ = 15;
    p_.in.rm = rm;
    p_.in.aperture = aperture;
    p_.in.offset = offset;
    p_.in.data = data;

    NvIdlIoctlFile();

    err_ = NvOsIoctl( gs_IoctlFile, gs_IoctlCode, &p_, i_size_, io_size_, o_size_ );
    NV_ASSERT( err_ == NvSuccess );
}

NvU8 NvRegr08( NvRmDeviceHandle hDeviceHandle, NvRmModuleID aperture, NvU32 offset )
{
    NvError err_;
    NvU32 i_size_ = OFFSET(NvRegr08_params, inout);
    NvU32 io_size_ = OFFSET(NvRegr08_params, out) - OFFSET(NvRegr08_params, inout);
    NvU32 o_size_ = sizeof(NvRegr08_params) - OFFSET(NvRegr08_params, out);
    NvRegr08_params p_;

    p_.in.package_ = NvRm_nvrm_module;
    p_.in.function_ = 14;
    p_.in.hDeviceHandle = hDeviceHandle;
    p_.in.aperture = aperture;
    p_.in.offset = offset;

    NvIdlIoctlFile();

    err_ = NvOsIoctl( gs_IoctlFile, gs_IoctlCode, &p_, i_size_, io_size_, o_size_ );
    NV_ASSERT( err_ == NvSuccess );

    return p_.out.ret_;
}

void NvRegrb( NvRmDeviceHandle hRmDeviceHandle, NvRmModuleID aperture, NvU32 num, NvU32 offset, NvU32 * values )
{
    NvError err_;
    NvU32 i_size_ = OFFSET(NvRegrb_params, inout);
    NvU32 io_size_ = OFFSET(NvRegrb_params, out) - OFFSET(NvRegrb_params, inout);
    NvU32 o_size_ = sizeof(NvRegrb_params) - OFFSET(NvRegrb_params, out);
    NvRegrb_params p_;

    p_.in.package_ = NvRm_nvrm_module;
    p_.in.function_ = 13;
    p_.in.hRmDeviceHandle = hRmDeviceHandle;
    p_.in.aperture = aperture;
    p_.in.num = num;
    p_.in.offset = offset;
    p_.in.values = values;

    NvIdlIoctlFile();

    err_ = NvOsIoctl( gs_IoctlFile, gs_IoctlCode, &p_, i_size_, io_size_, o_size_ );
    NV_ASSERT( err_ == NvSuccess );
}

void NvRegwb( NvRmDeviceHandle hRmDeviceHandle, NvRmModuleID aperture, NvU32 num, NvU32 offset, const NvU32 * values )
{
    NvError err_;
    NvU32 i_size_ = OFFSET(NvRegwb_params, inout);
    NvU32 io_size_ = OFFSET(NvRegwb_params, out) - OFFSET(NvRegwb_params, inout);
    NvU32 o_size_ = sizeof(NvRegwb_params) - OFFSET(NvRegwb_params, out);
    NvRegwb_params p_;

    p_.in.package_ = NvRm_nvrm_module;
    p_.in.function_ = 12;
    p_.in.hRmDeviceHandle = hRmDeviceHandle;
    p_.in.aperture = aperture;
    p_.in.num = num;
    p_.in.offset = offset;
    p_.in.values = ( NvU32 * )values;

    NvIdlIoctlFile();

    err_ = NvOsIoctl( gs_IoctlFile, gs_IoctlCode, &p_, i_size_, io_size_, o_size_ );
    NV_ASSERT( err_ == NvSuccess );
}

void NvRegwm( NvRmDeviceHandle hRmDeviceHandle, NvRmModuleID aperture, NvU32 num, const NvU32 * offsets, const NvU32 * values )
{
    NvError err_;
    NvU32 i_size_ = OFFSET(NvRegwm_params, inout);
    NvU32 io_size_ = OFFSET(NvRegwm_params, out) - OFFSET(NvRegwm_params, inout);
    NvU32 o_size_ = sizeof(NvRegwm_params) - OFFSET(NvRegwm_params, out);
    NvRegwm_params p_;

    p_.in.package_ = NvRm_nvrm_module;
    p_.in.function_ = 11;
    p_.in.hRmDeviceHandle = hRmDeviceHandle;
    p_.in.aperture = aperture;
    p_.in.num = num;
    p_.in.offsets = ( NvU32 * )offsets;
    p_.in.values = ( NvU32 * )values;

    NvIdlIoctlFile();

    err_ = NvOsIoctl( gs_IoctlFile, gs_IoctlCode, &p_, i_size_, io_size_, o_size_ );
    NV_ASSERT( err_ == NvSuccess );
}

void NvRegrm( NvRmDeviceHandle hRmDeviceHandle, NvRmModuleID aperture, NvU32 num, const NvU32 * offsets, NvU32 * values )
{
    NvError err_;
    NvU32 i_size_ = OFFSET(NvRegrm_params, inout);
    NvU32 io_size_ = OFFSET(NvRegrm_params, out) - OFFSET(NvRegrm_params, inout);
    NvU32 o_size_ = sizeof(NvRegrm_params) - OFFSET(NvRegrm_params, out);
    NvRegrm_params p_;

    p_.in.package_ = NvRm_nvrm_module;
    p_.in.function_ = 10;
    p_.in.hRmDeviceHandle = hRmDeviceHandle;
    p_.in.aperture = aperture;
    p_.in.num = num;
    p_.in.offsets = ( NvU32 * )offsets;
    p_.in.values = values;

    NvIdlIoctlFile();

    err_ = NvOsIoctl( gs_IoctlFile, gs_IoctlCode, &p_, i_size_, io_size_, o_size_ );
    NV_ASSERT( err_ == NvSuccess );
}

void NvRegw( NvRmDeviceHandle hDeviceHandle, NvRmModuleID aperture, NvU32 offset, NvU32 data )
{
    NvError err_;
    NvU32 i_size_ = OFFSET(NvRegw_params, inout);
    NvU32 io_size_ = OFFSET(NvRegw_params, out) - OFFSET(NvRegw_params, inout);
    NvU32 o_size_ = sizeof(NvRegw_params) - OFFSET(NvRegw_params, out);
    NvRegw_params p_;

    p_.in.package_ = NvRm_nvrm_module;
    p_.in.function_ = 9;
    p_.in.hDeviceHandle = hDeviceHandle;
    p_.in.aperture = aperture;
    p_.in.offset = offset;
    p_.in.data = data;

    NvIdlIoctlFile();

    err_ = NvOsIoctl( gs_IoctlFile, gs_IoctlCode, &p_, i_size_, io_size_, o_size_ );
    NV_ASSERT( err_ == NvSuccess );
}

NvU32 NvRegr( NvRmDeviceHandle hDeviceHandle, NvRmModuleID aperture, NvU32 offset )
{
    NvError err_;
    NvU32 i_size_ = OFFSET(NvRegr_params, inout);
    NvU32 io_size_ = OFFSET(NvRegr_params, out) - OFFSET(NvRegr_params, inout);
    NvU32 o_size_ = sizeof(NvRegr_params) - OFFSET(NvRegr_params, out);
    NvRegr_params p_;

    p_.in.package_ = NvRm_nvrm_module;
    p_.in.function_ = 8;
    p_.in.hDeviceHandle = hDeviceHandle;
    p_.in.aperture = aperture;
    p_.in.offset = offset;

    NvIdlIoctlFile();

    err_ = NvOsIoctl( gs_IoctlFile, gs_IoctlCode, &p_, i_size_, io_size_, o_size_ );
    NV_ASSERT( err_ == NvSuccess );

    return p_.out.ret_;
}

static NvError ReadIntFromFile( const char *filename, NvU32 *value )
{
    char buffer[257] = { '\0' };
    int fd, readcount, scancount;
    NvU32 rval = 0;

    fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        return NvError_BadValue;
    }
    readcount = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (readcount <= 0)
    {
        return NvError_BadValue;
    }

    scancount = sscanf(buffer, "%d", &rval);
    if (scancount != 1)
    {
        return NvError_BadValue;
    }
    *value = rval;
    return NvError_Success;
}

static ChipId NvRmPrivGetChipIdStub( void )
{
    NvError rval1, rval2;
    ChipId chipId = { 0, 0 };

    if (gs_ChipId.Id)
        return gs_ChipId;

    rval1 = ReadIntFromFile("/sys/module/fuse/parameters/tegra_chip_id",  &chipId.Id);
    rval2 = ReadIntFromFile("/sys/module/fuse/parameters/tegra_chip_rev", &chipId.Revision);

    if (rval1 == NvError_Success && rval2 == NvError_Success)
    {
        gs_ChipId = chipId;
    }
    else
    {
        NvOsDebugPrintf("%s: Could not read Tegra chip id/rev \r\n", __func__);
        NvOsDebugPrintf("Expected on kernels without Tegra3 support, using Tegra2\n");
        gs_ChipId.Id = AP20_ID;
        gs_ChipId.Revision = 3;
    }

    return gs_ChipId;
}

NvError NvRmGetRandomBytes( NvRmDeviceHandle hRmDeviceHandle, NvU32 NumBytes, void* pBytes )
{
    NvError err_;
    NvU32 i_size_ = OFFSET(NvRmGetRandomBytes_params, inout);
    NvU32 io_size_ = OFFSET(NvRmGetRandomBytes_params, out) - OFFSET(NvRmGetRandomBytes_params, inout);
    NvU32 o_size_ = sizeof(NvRmGetRandomBytes_params) - OFFSET(NvRmGetRandomBytes_params, out);
    NvRmGetRandomBytes_params p_;

    p_.in.package_ = NvRm_nvrm_module;
    p_.in.function_ = 7;
    p_.in.hRmDeviceHandle = hRmDeviceHandle;
    p_.in.NumBytes = NumBytes;
    p_.in.pBytes = pBytes;

    NvIdlIoctlFile();

    err_ = NvOsIoctl( gs_IoctlFile, gs_IoctlCode, &p_, i_size_, io_size_, o_size_ );
    NV_ASSERT( err_ == NvSuccess );

    return p_.out.ret_;
}

NvError NvRmQueryChipUniqueId( NvRmDeviceHandle hDevHandle, NvU32 IdSize, void* pId )
{
    NvError err_;
    NvU32 i_size_ = OFFSET(NvRmQueryChipUniqueId_params, inout);
    NvU32 io_size_ = OFFSET(NvRmQueryChipUniqueId_params, out) - OFFSET(NvRmQueryChipUniqueId_params, inout);
    NvU32 o_size_ = sizeof(NvRmQueryChipUniqueId_params) - OFFSET(NvRmQueryChipUniqueId_params, out);
    NvRmQueryChipUniqueId_params p_;

    p_.in.package_ = NvRm_nvrm_module;
    p_.in.function_ = 6;
    p_.in.hDevHandle = hDevHandle;
    p_.in.IdSize = IdSize;
    p_.in.pId = pId;

    NvIdlIoctlFile();

    err_ = NvOsIoctl( gs_IoctlFile, gs_IoctlCode, &p_, i_size_, io_size_, o_size_ );
    NV_ASSERT( err_ == NvSuccess );

    return p_.out.ret_;
}

NvError NvRmModuleGetCapabilities(
    NvRmDeviceHandle hDeviceHandle,
    NvRmModuleID Module,
    NvRmModuleCapability *pCaps,
    NvU32 NumCaps,
    void **Capability)
{
    NvU32 i;
    ChipId chipId;
    NvRmModuleCapability *cap;
    void *ret = 0;
    NvRmModulePlatform curPlatform = NvRmModulePlatform_Silicon;

    NvU32 ver_tab[][2] = {
        [NvRmModuleID_Mpe] = { 1, 2 },
        [NvRmModuleID_BseA] = { 1, 1 },
        [NvRmModuleID_Display] = { 1, 3 },
        [NvRmModuleID_Spdif] = { 1, 0 },
        [NvRmModuleID_I2s] = { 1, 1 },
        [NvRmModuleID_Misc] = { 2, 0 },
        [NvRmModuleID_Vde] = { 1, 2 },
        [NvRmModuleID_Isp] = { 1, 0 },
        [NvRmModuleID_Vi] = { 1, 1 },
        [NvRmModuleID_3D] = { 1, 2 },
        [NvRmModuleID_2D] = { 1, 1 },
        [NvRmPrivModuleID_Gart] = { 1, 1 },
    };

    chipId = NvRmPrivGetChipIdStub();

    Module = NVRM_MODULE_ID_MODULE(Module);

#if PLATFORM_SIMULATION
    curPlatform = NvRmModulePlatform_Simulation;
#endif

#if PLATFORM_EMULATION
    curPlatform = NvRmModulePlatform_Emulation;
#endif

    if (Module >= NV_ARRAY_SIZE(ver_tab) || !ver_tab[Module][0])
    {
        NvOsDebugPrintf("%s: MOD[%u] not implemented!\n", __func__, Module);
        return NvError_NotSupported;
    }

    switch ( Module )
    {
        case NvRmModuleID_3D:
            if (chipId.Id >= T30_ID)
            {
                if (chipId.Id == T30_ID)
                    ver_tab[Module][1] = 3;
                else
                    ver_tab[Module][1] = 4;
            }
            break;
        case NvRmModuleID_Vde:
            if (chipId.Id >= T30_ID)
                ver_tab[Module][1] = 3;
            break;
        case NvRmModuleID_Display:
            if (chipId.Id >= T30_ID)
                ver_tab[Module][1] = 4;
            break;
        case NvRmPrivModuleID_Gart:
            if (chipId.Id >= T30_ID)
                /* T30+ don't have a GART (although they do have an SMMU) */
                ver_tab[Module][0] = 0;
            break;
        default:
            /* nothing */
            break;
    }

    // Loop through the caps and return the last config that matches
    for (i=0; i<NumCaps; i++)
    {
        cap = &pCaps[i];

        if( cap->MajorVersion != ver_tab[Module][0] ||
            cap->MinorVersion != ver_tab[Module][1] ||
            cap->Platform != curPlatform )
        {
            continue;
        }

        if( (cap->EcoLevel == 0) || (cap->EcoLevel < chipId.Revision ))
        {
            ret = cap->Capability;
        }
    }

    if( !ret )
    {
        *Capability = 0;
        return NvError_NotSupported;
    }

    *Capability = ret;
    return NvSuccess;
}

void NvRmModuleResetWithHold(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmModuleID Module,
    NvBool bHold )
{
    NvOsDebugPrintf("%s %s MOD[%u] INST[%u]\n", __func__,
                    (bHold)?"assert" : "deassert",
                    NVRM_MODULE_ID_MODULE(Module),
                    NVRM_MODULE_ID_INSTANCE(Module));
}

void NvRmModuleReset(NvRmDeviceHandle hRmDeviceHandle, NvRmModuleID Module)
{
    NvRmModuleResetWithHold(hRmDeviceHandle, Module, NV_FALSE);
}

NvU32 NvRmModuleGetNumInstances(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmModuleID Module)
{
    ChipId chipId;

    chipId = NvRmPrivGetChipIdStub();

    switch (NVRM_MODULE_ID_MODULE(Module))
    {
    case NvRmModuleID_I2s:
        return 4;
    case NvRmModuleID_Display:
        return 2;
    case NvRmModuleID_3D:
        if (chipId.Id == T30_ID)
            return 2;
        else
            return 1;
    case NvRmModuleID_Avp:
    case NvRmModuleID_GraphicsHost:
    case NvRmModuleID_Vcp:
    case NvRmModuleID_Isp:
    case NvRmModuleID_Vi:
    case NvRmModuleID_Epp:
    case NvRmModuleID_2D:
    case NvRmModuleID_Spdif:
    case NvRmModuleID_Vde:
    case NvRmModuleID_Mpe:
    case NvRmModuleID_Hdcp:
    case NvRmModuleID_Hdmi:
    case NvRmModuleID_Tvo:
    case NvRmModuleID_Dsi:
    case NvRmModuleID_BseA:
        return 1;
    default:
        NvOsDebugPrintf("%s: MOD[%u] not implemented\n", __func__,
                        NVRM_MODULE_ID_MODULE(Module));
        return 1;
    }
}

void NvRmModuleGetBaseAddress(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmModuleID Module,
    NvRmPhysAddr *pBaseAddress,
    NvU32 *pSize)
{
    NvRmPhysAddr base;
    NvU32 size;

    switch (NVRM_MODULE_ID_MODULE(Module))
    {
    case NvRmModuleID_GraphicsHost:
        base = 0x50000000;
        size = 144*1024;
        break;
    case NvRmModuleID_Display:
        base = 0x54200000 + (NVRM_MODULE_ID_INSTANCE(Module))*0x40000;
        size = 256*1024;
        break;
    case NvRmModuleID_Mpe:
        base = 0x54040000;
        size = 256 * 1024;
        break;
    case NvRmModuleID_Dsi:
        base = 0x54300000;
        size = 256*1024;
        break;
    case NvRmModuleID_Vde:
        base = 0x6001a000;
        size = 0x3c00;
        break;
    case NvRmModuleID_ExceptionVector:
        base = 0x6000f000;
        size = 4096;
        break;
    case NvRmModuleID_FlowCtrl:
        base = 0x60007000;
        size = 32;
        break;
    default:
        NvOsDebugPrintf("%s: MOD[%u] INST[%u] not implemented\n",
                        __func__, NVRM_MODULE_ID_MODULE(Module),
                        NVRM_MODULE_ID_INSTANCE(Module));
        base = ~0;
        size = 0;
    }

    if (pBaseAddress)
        *pBaseAddress = base;
    if (pSize)
        *pSize = size;
}

NvError NvRmModuleGetModuleInfo(
    NvRmDeviceHandle hDevice,
    NvRmModuleID module,
    NvU32 *pNum,
    NvRmModuleInfo *pModuleInfo )
{
    return NvError_NotSupported;
}
