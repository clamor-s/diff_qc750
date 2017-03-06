/*
 * Copyright (c) 2008-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvos.h"
#include "nvcommon.h"
#include "nv3p.h"
#include "nvtest.h"
#include "ap20/nvboot_bit.h"
#include "ap20/arapb_misc.h"
#include "nvrm_drf.h"
#include "nvrm_hardware_access.h"
#include "nvboot_sdram.h"
#include "nvboot_fuse_int.h"
#include "ap20/arfuse.h"
#include "nvboot_aes_int.h"
#include "ap20/nvboot_version.h"
#include "ap20/nvboot_clocks.h"
#include "nvboot_clocks_int.h"


#define BOOTROM_BCT_ALIGNMENT (0x7F)

