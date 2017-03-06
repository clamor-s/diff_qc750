/*
 * Copyright (c) 2011-2012, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * <b> NVIDIA Driver Development Kit: SDIO Interface</b>
 *
 * @b Description: SD memory (standard capacity as well
 * as high capacity), SDIO, MMC interface.
 */


#include "nvodm_sdio.h"
#include "nvddk_sdio.h"
#include "nvrm_pinmux.h"
#include "nvrm_memmgr.h"
#include "nvrm_hardware_access.h"
#include "nvrm_drf.h"


/**
 * @brief Contains the sdio instance details . This information is shared
 * between the thread and the isr
 */
typedef struct NvDdkSdioInfoRec
{
    // Nvrm device handle
    NvRmDeviceHandle hRm;
    // Instance of the SDMMC module
    NvU32 Instance;
    // SDMMC configuration pin-map.
    NvOdmSdioPinMap PinMap;
    // Physical base address of the specific sdio instance
    NvRmPhysAddr SdioPhysicalAddress;
    // Base address of PMC
    NvU32 PmcBaseAddress;
    // Virtual base address of the specific sdio instance
    NvU32* pSdioVirtualAddress;
    // Size of PMC register map
    NvU32 PmcBankSize;
    // size of the sdio register map
    NvU32 SdioBankSize;
    // Bus width
    NvU32 BusWidth;
    // Bus voltage
    NvU32 BusVoltage;
    // High speed mode
    NvU32 IsHighSpeedEnabled;
    // Clock frequency
    NvRmFreqKHz ConfiguredFrequency;
    // Indicates whether it is a read or a write transaction
    NvBool IsRead;
    // Request Type
    SdioRequestType RequestType;
    // Semaphore handle used for internal handshaking between the calling thread
    // and the isr
    NvOsSemaphoreHandle PrivSdioSema;
    // Rmmemory Handle of the buffer allocated
    NvRmMemHandle hRmMemHandle;
    // Physical Buffer Address of the memory
    NvU32 pPhysBuffer;
    // Virtual Buffer Address
    void* pVirtBuffer;
    // Controller and Error status
    NvDdkSdioStatus* ControllerStatus;
    NvOsSemaphoreHandle SdioPowerMgtSema;
    NvU32 SdioRmPowerClientId;
    // Maximum block size supported by the controller
    NvU32 MaxBlockLength;
    // Odm Sdio handle used to enable/disable power rails
    NvOdmSdioHandle SdioOdmHandle;
    // Sdio Interrupt handle
    NvOsInterruptHandle InterruptHandle;
    NvBool ISControllerSuspended;
    NvOsIntrMutexHandle SdioThreadSafetyMutex;
    // Uhs mode
    NvU32 Uhsmode;
    /**
     * Sets the given tap delay value for sdmmc.
     *
     * @param hSdio             SDIO device handle
     * @param Tapdelay        The tap delay value that has to be set
     *
     */
    void (*NvDdkSdioSetTapDelay)(NvDdkSdioDeviceHandle hSdio, NvU32 Tapdelay);
    /**
     * Configures the uhs mode. Is valid for SDHOST v3.0 and above controllers.
     *
     * @param hSdio             SDIO device handle
     * @param Uhsmode        Uhs mode to be set(DDR50, SDR104, SDR50)
     *
     */
    void (*NvDdkSdioConfigureUhsMode)(NvDdkSdioDeviceHandle hSdio, NvU32 Uhsmode);
    /**
     * Gets the new higher capabilities like DDR mode support, SDR104 mode support etc.
     *
     * @param hSdio             SDIO device handle
     * @param pHostCap       SDIO host capabilities
     *
     */
    void (*NvDdkSdioGetHigherCapabilities)(NvDdkSdioDeviceHandle hSdio, NvDdkSdioHostCapabilities *pHostCap);
    /**
     * Puts SDMMC pins in Deep Power Down Mode (lowest power mode possible)
     *
     * @param hSdio             SDIO device handle
     */
    void (*NvDdkSdioDpdEnable)(NvDdkSdioDeviceHandle hSdio);
    /**
     * Gets SDMMC pins out of Deep Power Down
     *
     * @param hSdio             SDIO device handle
     */
    void (*NvDdkSdioDpdDisable)(NvDdkSdioDeviceHandle hSdio);
}NvDdkSdioInfo;

enum
{
    SDMMC_SW_RESET_FOR_ALL = 0x1000000,
    SDMMC_SW_RESET_FOR_CMD_LINE = 0x2000000,
    SDMMC_SW_RESET_FOR_DATA_LINE = 0x4000000,
};


// Defines various MMC card specific command types

#define SDMMC_REGR(pSdioHwRegsVirtBaseAdd, reg) \
        NV_READ32((pSdioHwRegsVirtBaseAdd) + ((SDMMC_##reg##_0)/4))

#define SDMMC_REGW(pSdioHwRegsVirtBaseAdd, reg, val) \
    do\
    {\
        NV_WRITE32((((pSdioHwRegsVirtBaseAdd) + ((SDMMC_##reg##_0)/4))), (val));\
    }while (0)
    #define DIFF_FREQ(x, y) \
    (x>y)?(x-y):(y-x)

#define PMC_REGR(PmcHwRegsVirtBaseAdd, reg) \
            NV_READ32((PmcHwRegsVirtBaseAdd) + ((APBDEV_PMC_##reg##_0)))

#define PMC_REGW(PmcHwRegsVirtBaseAdd, reg, val) \
        do\
        {\
            NV_WRITE32(((PmcHwRegsVirtBaseAdd) + (APBDEV_PMC_##reg##_0)), (val));\
        }while (0)

/**
 * Used to issue softreset to Sdio Controller
 *
 * @param hSdio            The SDIO device handle.
 * @mask                   Reset ALL/DATA/CMD lines
 */

void PrivSdioReset(NvDdkSdioDeviceHandle hSdio, NvU32 mask);

