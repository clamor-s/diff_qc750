/*
 * Copyright (c) 2011-2012, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef INCLUDED_VERSION_H
#define INCLUDED_VERSION_H


// @Major: Need to be changed when compatibility breaks between nvflash and sbktool
// @Minor: Need to be changed when significant amount of change is done
// @Gerrit id: Changes with every single change for nvflash/sbktool respectively.

// nvflash version

// nvflash includes --blob option for nvflash in secure mode
#define NVFLASH_VERSION_1 1.1.57814

// moved --setblhash impl from NVML to BL to support multi bootloader download
#define NVFLASH_VERSION_2 1.2.66746

// added --create support in resume mode
#define NVFLASH_VERSION_3 1.3.66747

// Remove refrences of 'NvBootConfigTable' structure
// from common files in miniloader.
#define NVFLASH_VERSION_4 1.3.68156

// removed  ap15 support from nvflash
#define NVFLASH_VERSION_5 1.4.66748

// moving miniloader to GCC compilation
#define NVFLASH_VERSION_6 1.5.66719

// Add support in Nvflash to flash a device using
// the usb device path given as an argument to --instance
#define NVFLASH_VERSION_7 1.5.71595

// Add --skip_part <partition_name> option
#define NVFLASH_VERSION_8 1.6.71610

// Add --nvinternal <blob>
#define NVFLASH_VERSION_9 1.7.75664

// Add support for running SD and SE diag tests via Nv3pServer
#define NVFLASH_VERSION_10 1.8.78681

// Correcting bct sync into device(76836)
// Error check for skipped part only(85133)
// Read jtag disable bit from fuse
#define NVFLASH_VERSION_11 1.9.86345

// moving bootloader padding to nvflash host app
#define NVFLASH_VERSION_12 1.10.76762

// Nvflash commands can use parition names as well as partition ids
#define NVFLASH_VERSION_13 1.10.78199

// Add support for running PWM diag test via Nv3pServer
#define NVFLASH_VERSION_14 1.10.96665

// Use erasing whole device instead of format individual paritions during flash
// This is only for non embedded build
#define NVFLASH_VERSION_15 1.11.104376

// Added support to create filesystem and kernel image
#define NVFLASH_VERSION_16 1.12.96349

// Create libnvbuildbct to be used by Nvflash and buildbct for converting
// cfg file into bct file
#define NVFLASH_VERSION_17 1.13.87205

// nvsbktool version

// clubs RCM messages, --bl BL hash and sbktool version
#define NVSBKTOOL_VERSION_1 1.1.57813

// Error check for SBK key input
#define NVSBKTOOL_VERSION_2 1.2.84270


// stringize version number of nvflash and nvsbktool
#define STRING2(s) #s
#define STRING(s) STRING2(s)
#define NVFLASH_VERSION "v"STRING(NVFLASH_VERSION_17)
#define NVSBKTOOL_VERSION "v"STRING(NVSBKTOOL_VERSION_2)

#endif//INCLUDED_VERSION_H

