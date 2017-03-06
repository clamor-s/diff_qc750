/*
 * Copyright (c) 2009-2012, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef FASTBOOT_INCLUDED_H
#define FASTBOOT_INCLUDED_H

#include "nvcommon.h"
#include "nvrm_surface.h"
#include "nvaboot.h"
#include "nvodm_services.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define BOOT_TAG_SIZE (2048/sizeof(NvU32))
#define WB0_SIZE 8192
#ifdef NV_EMBEDDED_BUILD
#define KERNEL_PARTITON "KERNEL_PRIMARY"
#define RECOVERY_KERNEL_PARTITION "KERNEL_RECOVERY"
#else
#define KERNEL_PARTITON "LNX"
#define RECOVERY_KERNEL_PARTITION "SOS"
#endif

#define ANDROID_BOOT_CMDLINE_SIZE 512
#define IGNORE_FASTBOOT_CMD "ignorefastboot"

typedef NvU32 (*GetMoreBitsCallback)(NvU8 *pBuffer, NvU32 MaxSize);

typedef struct FastbootWaveRec *FastbootWaveHandle;

typedef struct IconRec
{
    NvU16 width;
    NvU16 height;
    NvS32 stride;
    NvU8  bpp;
    NvU32 data[1];
} Icon;

typedef struct RadioMenuRec
{
    NvU32 PulseColor;
    NvU16 HorzSpacing;
    NvU16 VertSpacing;
    NvU16 RoundRectRadius;
    NvU8 NumOptions;
    NvU8 CurrOption;
    NvU8 PulseAnim;
    NvS8 PulseSpeed;
    Icon *Images[20];
} RadioMenu;

typedef struct TextMenuRec
{
    NvU8 NumOptions;
    NvU8 CurrOption;
    const char *Texts[1];
} TextMenu;

typedef enum
{
    FileSystem_Basic = 0,
    FileSystem_Yaffs2,
    FileSystem_Ext4
} FastbootFileSystem;

typedef struct PartitionNameMapRec
{
    const char        *LinuxPartName;
    const char        *NvPartName;
    FastbootFileSystem FileSystem;
} PartitionNameMap;

#define FRAMEBUFFER_DOUBLE 0x1
#define FRAMEBUFFER_32BIT  0x2
#define FRAMEBUFFER_CLEAR  0x4

NvBool FastbootInitializeDisplay(
    NvU32        Flags,
    NvRmSurface *pFramebuffer,
    NvRmSurface *pFrontbuffer,
    void       **pPixelData,
    NvBool       UseHdmi);

void FastbootShutdownDisplay(void);

NvU32 FastbootGetDisplayFbFlags( void );

TextMenu *FastbootCreateTextMenu(
    NvU32 NumOptions,
    NvU32 InitOption,
    const char **pTexts
    );

void FastbootTextMenuSelect(
    TextMenu *pMenu,
    NvBool    Next);

void FastbootDrawTextMenu(
    TextMenu   *Menu,
    NvU32 X,
    NvU32 Y);

RadioMenu *FastbootCreateRadioMenu(
    NvU32 NumOptions,
    NvU32 InitOption,
    NvU32 PulseSpeed,
    NvU32 PulseColor,
    Icon **pIcons);

void FastbootRadioMenuSelect(
    RadioMenu *pMenu,
    NvBool    Next);

void FastbootDrawRadioMenu(
    NvRmSurface *pSurface,
    void        *pPixels,
    RadioMenu   *Menu);
//added by jimmy 
void MyFastbootDrawRadioMenu(
    NvRmSurface *pSurface,void *pPixels,RadioMenu   *Menu,
    NvU32 ScreenX,NvU32 ScreenY,NvBool isRef);

void FramebufferUpdate( void );

void FastbootError(const char* fmt, ...);

void FastbootStatus(const char* fmt, ...);

NvBool FastbootAddAtag(
    NvU32 Atag,
    NvU32 DataSize,
    const void *Data);

NvError AddSerialBoardInfoTag(void);

void GetOsAvailableMemSizeParams(
    NvAbootHandle hAboot,
    NvOdmOsOsInfo *OsInfo,
    NvAbootDramPart *DramPart,
    NvU32 *PrimaryMemSize,
    NvU32 *MemSizeBeyondPrimary);

NvError FastbootCommandLine(
    NvAbootHandle hAboot,
    const PartitionNameMap *PartitionList,
    NvU32 NumPartitions,
    NvU32 SecondaryBootDevice,
    const unsigned char *CommandLine,
    NvBool HasInitRamDisk,
    const unsigned char *ProductName,
    NvBool RecoveryKernel);

void FastbootSetUsbMode(NvBool Is3p);

void FastbootCloseWave(FastbootWaveHandle pWave);

NvError FastbootPlayWave(
    GetMoreBitsCallback Callback,
    NvU32 SampleRateHz,
    NvBool Stereo,
    NvBool Signed,
    NvU32 BitsPerSample,
    FastbootWaveHandle *hWav);

extern NvU32 s_BootTags[BOOT_TAG_SIZE];


#ifdef __cplusplus
}
#endif

#endif
