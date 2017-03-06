/*
 * Copyright (c) 2008 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/** @file
 * @brief <b>NVIDIA Driver Development Kit: Fuse API</b>
 *
 * @b Description: Contains the NvRM chip unique id implementation.
 */
#include "nvassert.h"
#include "nvrm_drf.h"
#include "nvos.h"
#include "nvrm_module.h"
#include "nvrm_hardware_access.h"
#include "nvrm_hwintf.h"
#include "ap20/arclk_rst.h"
#include "ap20/arfuse_public.h"
#include "common/common_misc_private.h"
#include "ap20rm_clocks.h"

NvError NvRmPrivAp20ChipUniqueId(NvRmDeviceHandle hDevHandle,void* pId)
{
    NvU64*  pOut = (NvU64*)pId; // Pointer to output buffer
    NvU32   OldRegData;                // Old register contents
    NvU32   NewRegData;                // New register contents

    NV_ASSERT(hDevHandle);
    NV_ASSERT(pId);
#if NV_USE_FUSE_CLOCK_ENABLE
    // Enable fuse clock
    Ap20EnableModuleClock(hDevHandle, NvRmModuleID_Fuse, NV_TRUE);
#endif

    // Access to unique id is protected, so make sure all registers visible
    // first.
    OldRegData = NV_REGR(hDevHandle,
        NVRM_MODULE_ID( NvRmPrivModuleID_ClockAndReset, 0 ),
        CLK_RST_CONTROLLER_MISC_CLK_ENB_0);
    NewRegData = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, MISC_CLK_ENB,
        CFG_ALL_VISIBLE, 1, OldRegData);
    NV_REGW(hDevHandle, NVRM_MODULE_ID( NvRmPrivModuleID_ClockAndReset, 0 ),
        CLK_RST_CONTROLLER_MISC_CLK_ENB_0, NewRegData);

    // Read the secure id from the fuse registers and copy to the output buffer
    *pOut = ((NvU64)NV_REGR(hDevHandle,
                NVRM_MODULE_ID( NvRmModuleID_Fuse, 0 ),
                FUSE_JTAG_SECUREID_0_0))
          | (((NvU64)NV_REGR(hDevHandle,
                NVRM_MODULE_ID( NvRmModuleID_Fuse, 0 ),
                FUSE_JTAG_SECUREID_1_0)) << 32);

    // Restore the protected registers enable to the way we found it.
    NV_REGW(hDevHandle, NVRM_MODULE_ID( NvRmPrivModuleID_ClockAndReset, 0 ),
        CLK_RST_CONTROLLER_MISC_CLK_ENB_0, OldRegData);
#if NV_USE_FUSE_CLOCK_ENABLE
    // Disable fuse clock
    Ap20EnableModuleClock(hDevHandle, NvRmModuleID_Fuse, NV_FALSE);
#endif
    return NvError_Success;
}
