/*
 * Copyright (c) 2011-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */


#ifndef INCLUDED_IRAM_ADDRESS_ALLOCATION_H
#define INCLUDED_IRAM_ADDRESS_ALLOCATION_H

// It contains a note of all the addresses in IRAM used by bootloader

//Iram usage in recovery mode
//                            Ap20                          t30
//BIT                         0x0~0xf0                      0x0~0xf0
//BCT[FR]                     0xf0~0x10f0                   0xf0~0x18f0
//BCT[RC]                     0x10f0~0x20f0                 0x18f0~0x30f0
//FREE AREA                   0x20f0~0x2100[16]             0x30f0~0x3100[16]
//AES KEY TABLE               0x2100~0x2140                 0x3100~0x3140
//FREE AREA                   0x2140~0x3000[3776]           0x3140~0x4000[3776]
//USB                         0x3000~0x8000                 0x4000~0x9000
//FREE AREA                   NONE                          0x9000~0xA000[4096]
//ML                          0x8000                        0xA000
//[]->free space in bytes

// Size of BCT for AP20 platforms is 4080 bytes (~0x1000)
// Bct uses 0xF0 to 0x10F0 offset in IRAM in FR mode and
// address 0x10F0 to 0x20F0 when device goes into recovery mode
// due to bct validation failure.

// Iram Address offsets aligned to 0x1000 allocated to USB transfer buffer for Ap15/20
#define NV3P_AES_KEYTABLE_OFFSET_AP20            (0x2100)
#define NV3P_TRANSFER_EP0_BUFFER_OFFSET_AP20     (0x3000)
#define NV3P_TRANSFER_EP_BULK_BUFFER_OFFSET_AP20 (0x5000)
#define NV3P_USB_BUFFER_OFFSET_AP20              (0x6000)

// Size of BCT for T30 platforms is 6128 bytes (~0x1800)
// Bct uses 0xF0 to 0x18F0 offset in IRAM in FR mode and
// address 0x18F0 to 0x30F0 when device goes into recovery mode
// due to bct validation failure.

// Iram Address offsets aligned to 0x1000 allocated to USB transfer buffer for t30
#define NV3P_AES_KEYTABLE_OFFSET_T30            (0x3100)
#define NV3P_TRANSFER_EP0_BUFFER_OFFSET_T30     (0x4000)
#define NV3P_TRANSFER_EP_BULK_BUFFER_OFFSET_T30 (0x6000)
#define NV3P_USB_BUFFER_OFFSET_T30              (0x7000)

#endif // INCLUDED_IRAM_ADDRESS_ALLOCATION_H
