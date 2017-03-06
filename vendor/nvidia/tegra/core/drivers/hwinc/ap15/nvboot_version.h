/**
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * Defines version information for the boot rom.
 */

#ifndef INCLUDED_NVBOOT_VERSION_H
#define INCLUDED_NVBOOT_VERSION_H

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * Defines a macro for assembling a 32-bit version number out of
 * a 16-bit major revision number a and a 16-bit minor revision number b.
 */
#define NVBOOT_VERSION(a,b) ((((a)&0xffff) << 16) | ((b)&0xffff))

/**
 * Defines the version of the bootrom code.
 *
 * Revision history (update with significant/special releases):
 *
 * Major Revision 1:
 *   Rev 0: First tapeout.
 *   Rev 1: A02 tapeout.
 *   Rev 2: AP15b/AP16 tapeout.
 *   Rev 3: AP16 A03 tapeout
 */
#define NVBOOT_BOOTROM_VERSION (NVBOOT_VERSION(1, 3))

/**
 * Defines the version of the RCM protocol.
 *
 * Revision history (update with each revision change):
 *
 * Major Revision 1:
 *   Rev 0: First tapeout.
 */
#define NVBOOT_RCM_VERSION (NVBOOT_VERSION(1, 0))

/**
 * Defines the version of the boot data structures (BCT, BIT).
 *
 * Revision history (update with each revision change):
 *
 * Major Revision 1:
 *   Rev 0: First tapeout.
 */
#define NVBOOT_BOOTDATA_VERSION (NVBOOT_VERSION(1, 0))

/**
 * Constants for the version numbers of each chip revision.
 */

/**
 * BootROM versions for each chip revision
 */
#define NVBOOT_BOOTROM_VERSION_AP15_A01_MAJOR 1
#define NVBOOT_BOOTROM_VERSION_AP15_A01_MINOR 0
#define NVBOOT_BOOTROM_VERSION_AP15_A01                 \
  (NVBOOT_VERSION(NVBOOT_BOOTROM_VERSION_AP15_A01_MAJOR,\
                  NVBOOT_BOOTROM_VERSION_AP15_A01_MINOR))

#define NVBOOT_BOOTROM_VERSION_AP15_A02_MAJOR 1
#define NVBOOT_BOOTROM_VERSION_AP15_A02_MINOR 1
#define NVBOOT_BOOTROM_VERSION_AP15_A02                 \
  (NVBOOT_VERSION(NVBOOT_BOOTROM_VERSION_AP15_A02_MAJOR,\
                  NVBOOT_BOOTROM_VERSION_AP15_A02_MINOR))

#define NVBOOT_BOOTROM_VERSION_AP16_A01_MAJOR 1
#define NVBOOT_BOOTROM_VERSION_AP16_A01_MINOR 2
#define NVBOOT_BOOTROM_VERSION_AP16_A01                 \
  (NVBOOT_VERSION(NVBOOT_BOOTROM_VERSION_AP16_A01_MAJOR,\
                  NVBOOT_BOOTROM_VERSION_AP16_A01_MINOR))

#define NVBOOT_BOOTROM_VERSION_AP16_A02_MAJOR 1
#define NVBOOT_BOOTROM_VERSION_AP16_A02_MINOR 2
#define NVBOOT_BOOTROM_VERSION_AP16_A02                 \
  (NVBOOT_VERSION(NVBOOT_BOOTROM_VERSION_AP16_A02_MAJOR,\
                  NVBOOT_BOOTROM_VERSION_AP16_A02_MINOR))

#define NVBOOT_BOOTROM_VERSION_AP16_A03_MAJOR 1
#define NVBOOT_BOOTROM_VERSION_AP16_A03_MINOR 3
#define NVBOOT_BOOTROM_VERSION_AP16_A03                 \
  (NVBOOT_VERSION(NVBOOT_BOOTROM_VERSION_AP16_A03_MAJOR,\
                  NVBOOT_BOOTROM_VERSION_AP16_A03_MINOR))

/**
 * RCM versions for each chip revision
 */
#define NVBOOT_RCM_VERSION_AP15_A01_MAJOR 1
#define NVBOOT_RCM_VERSION_AP15_A01_MINOR 0
#define NVBOOT_RCM_VERSION_AP15_A01                 \
  (NVBOOT_VERSION(NVBOOT_RCM_VERSION_AP15_A01_MAJOR,\
                  NVBOOT_RCM_VERSION_AP15_A01_MINOR))

#define NVBOOT_RCM_VERSION_AP15_A02_MAJOR 1
#define NVBOOT_RCM_VERSION_AP15_A02_MINOR 0
#define NVBOOT_RCM_VERSION_AP15_A02                 \
  (NVBOOT_VERSION(NVBOOT_RCM_VERSION_AP15_A02_MAJOR,\
                  NVBOOT_RCM_VERSION_AP15_A02_MINOR))

#define NVBOOT_RCM_VERSION_AP16_A01_MAJOR 1
#define NVBOOT_RCM_VERSION_AP16_A01_MINOR 0
#define NVBOOT_RCM_VERSION_AP16_A01                 \
  (NVBOOT_VERSION(NVBOOT_RCM_VERSION_AP16_A01_MAJOR,\
                  NVBOOT_RCM_VERSION_AP16_A01_MINOR))

#define NVBOOT_RCM_VERSION_AP16_A02_MAJOR 1
#define NVBOOT_RCM_VERSION_AP16_A02_MINOR 0
#define NVBOOT_RCM_VERSION_AP16_A02                 \
  (NVBOOT_VERSION(NVBOOT_RCM_VERSION_AP16_A02_MAJOR,\
                  NVBOOT_RCM_VERSION_AP16_A02_MINOR))

#define NVBOOT_RCM_VERSION_AP16_A03_MAJOR 1
#define NVBOOT_RCM_VERSION_AP16_A03_MINOR 0
#define NVBOOT_RCM_VERSION_AP16_A03                 \
  (NVBOOT_VERSION(NVBOOT_RCM_VERSION_AP16_A03_MAJOR,\
                  NVBOOT_RCM_VERSION_AP16_A03_MINOR))

/**
 * BootData versions for each chip revision
 */
#define NVBOOT_BOOTDATA_VERSION_AP15_A01_MAJOR 1
#define NVBOOT_BOOTDATA_VERSION_AP15_A01_MINOR 0
#define NVBOOT_BOOTDATA_VERSION_AP15_A01                 \
  (NVBOOT_VERSION(NVBOOT_BOOTDATA_VERSION_AP15_A01_MAJOR,\
                  NVBOOT_BOOTDATA_VERSION_AP15_A01_MINOR))

#define NVBOOT_BOOTDATA_VERSION_AP15_A02_MAJOR 1
#define NVBOOT_BOOTDATA_VERSION_AP15_A02_MINOR 0
#define NVBOOT_BOOTDATA_VERSION_AP15_A02                 \
  (NVBOOT_VERSION(NVBOOT_BOOTDATA_VERSION_AP15_A02_MAJOR,\
                  NVBOOT_BOOTDATA_VERSION_AP15_A02_MINOR))

#define NVBOOT_BOOTDATA_VERSION_AP16_A01_MAJOR 1
#define NVBOOT_BOOTDATA_VERSION_AP16_A01_MINOR 0
#define NVBOOT_BOOTDATA_VERSION_AP16_A01                 \
  (NVBOOT_VERSION(NVBOOT_BOOTDATA_VERSION_AP16_A01_MAJOR,\
                  NVBOOT_BOOTDATA_VERSION_AP16_A01_MINOR))

#define NVBOOT_BOOTDATA_VERSION_AP16_A02_MAJOR 1
#define NVBOOT_BOOTDATA_VERSION_AP16_A02_MINOR 0
#define NVBOOT_BOOTDATA_VERSION_AP16_A02                 \
  (NVBOOT_VERSION(NVBOOT_BOOTDATA_VERSION_AP16_A02_MAJOR,\
                  NVBOOT_BOOTDATA_VERSION_AP16_A02_MINOR))

#define NVBOOT_BOOTDATA_VERSION_AP16_A03_MAJOR 1
#define NVBOOT_BOOTDATA_VERSION_AP16_A03_MINOR 0
#define NVBOOT_BOOTDATA_VERSION_AP16_A03                 \
  (NVBOOT_VERSION(NVBOOT_BOOTDATA_VERSION_AP16_A03_MAJOR,\
                  NVBOOT_BOOTDATA_VERSION_AP16_A03_MINOR))


#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVBOOT_VERSION_H */
