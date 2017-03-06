/*
 * Copyright (c) 2006 - 2007 NVIDIA Corporation.  All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file nvboot_sku.h
 *
 * SKU support interface for NvBoot
 *
 */

#ifndef INCLUDED_NVBOOT_SKU_INT_H
#define INCLUDED_NVBOOT_SKU_INT_H

#include "nvboot_error.h"

#if defined(__cplusplus)
extern "C"
{
#endif



/*
 * NvBootSku_SkuId -- Identifies the supported SKU
 * Definitiion must follow an encoding in which more bits burned imply less capability
 */
typedef enum
{
    NvBootSku_Sku1 = 0, // naming defined by marketing, sorry
    NvBootSku_Sku2 = 1,
    NvBootSku_Sku3 = 2,
    NvBootSku_SkuReserved = 3,
    NvBootSku_SkuRange = 4,  // must be smallest power of two larger than last valid encoding
    NvBootSku_SkuId_Force32 = 0x7fffffff
} NvBootSku_SkuId;

#define NVBOOT_SKU_MASK ( ((int) NvBootSku_SkuRange) - 1 )


/**
 * Enforcing the SKU restrictions as defined in bug 330532
 * Relevant extracts from the bug comments
 *
 * fuse value description
 * ---------- -----------
 * 2'b00 full features (originally called "SKU 1")
 * 2'b01 feature set originally called "SKU 2"
 * 2'b10 feature set originally called "SKU 3"
 * 2'b11 undefined, but Boot ROM will implement the same as 2'b10 to ensure
 * that functionality always degrades as fuses are burned 
 *
 *
 * SKU 1: Full Features
 * SKU 2:
 * - Features removed in HW (3D, HW encode, ISP, L2, HDMI, 2nd display)
 * - Software controlled (ARM11 @ 450MHz, H.264/MPEG-4/VC-1 decode @ D1 30, H.264/MPEG-4 encode @ CIF 30, 5MP sensor)
 * SKU 3:
 * - Features removed in HW (Video encode and decode, HDMI)
 * - Software controlled (H.264/MPEG-4/VC-2 SW decode @ D1 30, H.264/MPEG-4 encode @ CIF 30) 
 *
 * This was further expanded and clarified in a POR document
 *
 * HW removal is by using sticky clock disable bits
 *
 * @param Sku identification
 *
 * @return void
 */
void
NvBootSkuEnforceSku(NvBootSku_SkuId Sku) ;
  
#if defined(__cplusplus)
}
#endif

#endif // INCLUDED_NVBOOT_SKU_INT_H
