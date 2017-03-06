/*
 * Copyright (c) 2009-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/*
 * This file implements the pin-mux configuration tables for each I/O module.
 */

// THESE SETTINGS ARE PLATFORM-SPECIFIC (not SOC-specific).
// PLATFORM = AP20 SO-DIMM

#include "nvodm_query_pinmux.h"
#include "nvassert.h"
#include "nvodm_query.h"
#include "nvodm_disp.h"
#include "nvodm_services.h"
#include "nvfuse.h"
#include "ap20/nvboot_bct.h"
#include "nvmachtypes.h"
#include "nvos.h"
#include "nvsku.h"

#define NVODM_PINMUX_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static const NvU32 s_NvOdmPinMuxConfig_Uart[] = {
    NvOdmUartPinMap_Config2, // UART 1
    NvOdmUartPinMap_Config1, // UART 2
    NvOdmUartPinMap_Config1, // UART 3
    NvOdmUartPinMap_Config2, // UART 4
    0,
    0
};

static const NvU32 s_NvOdmPinMuxConfig_Spi[] = {
    NvOdmSpiPinMap_Config2, // SPI 1
    NvOdmSpiPinMap_Config5, // SPI 2
    NvOdmSpiPinMap_Config3, // SPI 3
    NvOdmSpiPinMap_Config2, // SPI 4
    0
};

static const NvU32 s_NvOdmPinMuxConfig_Twc[] = {
    0
};

static const NvU32 s_NvOdmPinMuxConfig_I2c[] = {
    NvOdmI2cPinMap_Config1,
    NvOdmI2cPinMap_Config1,
    NvOdmI2cPinMap_Config1
};

static const NvU32 s_NvOdmPinMuxConfig_I2cPmu[] = {
    NvOdmI2cPmuPinMap_Config1
};

static const NvU32 s_NvOdmPinMuxConfig_Ulpi[] = {
    NvOdmUlpiPinMap_Config1
};

static const NvU32 s_NvOdmPinMuxConfig_Sdio[] = {
    NvOdmSdioPinMap_Config1, // NVTODO : What about the GPIO
    NvOdmSdioPinMap_Config2,
    0,
    NvOdmSdioPinMap_Config2  // SDIO 4 : HSMMC
};

static const NvU32 s_NvOdmPinMuxConfig_Spdif[] = {
    NvOdmSpdifPinMap_Config1
};

static const NvU32 s_NvOdmPinMuxConfig_Hsi[] = {
    0
};

static const NvU32 s_NvOdmPinMuxConfig_Hdmi[] = {
    0
};

static const NvU32 s_NvOdmPinMuxConfig_Pwm[] = {
    0
};

static const NvU32 s_NvOdmPinMuxConfig_Ata[] = {
    0
};

static const NvU32 s_NvOdmPinMuxConfig_Nand[] = {
    NvOdmNandPinMap_Config4
};

static const NvU32 s_NvOdmPinMuxConfig_Dap[] = {
    NvOdmDapPinMap_Config1, // DAP-1
    NvOdmDapPinMap_Config1, // DAP-2
    0,
    0,
    0
};

static const NvU32 s_NvOdmPinMuxConfig_Kbd[] = {
    0
};

static const NvU32 s_NvOdmPinMuxConfig_Hdcp[] = {
    0
};

static const NvU32 s_NvOdmPinMuxConfig_Mio[] = {
    0
};

static const NvU32 s_NvOdmPinMuxConfig_ExternalClock[] = {
    // NVTODO : Looks like AD1937 gets clock from on-board MAX9485, in which
    // entries 1 and 2 in this table are not needed.
    // This config will be required for SMEG board.
    NvOdmExternalClockPinMap_Config1,
    0,
    NvOdmExternalClockPinMap_Config1, // CSUS -> VI_Sensor_CLK
};

static const NvU32 s_NvOdmPinMuxConfig_VideoInput[] = {
    NvOdmVideoInputPinMap_Config3,
};

static const NvU32 s_NvOdmPinMuxConfig_Display[] = {
    NvOdmDisplayPinMap_Config1,
    0  // Only 1 display is connected to the LCD pins
};

static const NvU32 s_NvOdmPinMuxConfig_BacklightPwm[] = {
    0, // NVTODO: Is this correct?
    NvOdmBacklightPwmPinMap_Config2
};

static const NvU32 s_NvOdmPinMuxConfig_Crt[] = {
    0,
};

static const NvU32 s_NvOdmPinMuxConfig_Tvo[] = {
    0,
};

static const NvU32 s_NvOdmPinMuxConfig_OneWire[] = {
    0
};

static const NvU32 s_NvOdmPinMuxConfig_PciExpress[] = {
    0
};

static const NvU32 s_NvOdmPinMuxConfig_SyncNor[] = {
    NvOdmNandPinMap_Config4
};

static const NvU32 s_NvOdmClockLimit_Sdio[] = {
    45000, // SD Card Slot 1
    50000, // WLAN
    50000, // Unused
    50000, // HSMMC (on-board EMMC flash)
};

static const NvU32 s_NvOdmClockLimit_Nand[] = {
    100000,
};

static const NvU32 s_NvOdmClockLimit_SyncNor[] = {
    100000
};

void
NvOdmQueryPinMux(
    NvOdmIoModule IoModule,
    const NvU32 **pPinMuxConfigTable,
    NvU32 *pCount)
{
    NvBool IsBootDevNand = NV_FALSE, IsBootDevNor = NV_FALSE;

#ifdef NV_SKU_SUPPORT
    NvU32 BootDevice, Instance;
    if ((IoModule == NvOdmIoModule_Nand) || (IoModule == NvOdmIoModule_SyncNor))
    {
        NvFuseGetSecondaryBootDevice(&BootDevice, &Instance);
        switch (BootDevice)
        {
            case NvBootDevType_Snor:
                IsBootDevNor = NV_TRUE;
                break;
            case NvBootDevType_Nand:
            default:
                IsBootDevNand = NV_TRUE;
                break;
        }
    }
#endif

    switch (IoModule)
    {
    case NvOdmIoModule_Display:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_Display;
        *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_Display);
        break;

    case NvOdmIoModule_Dap:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_Dap;
        *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_Dap);
        break;

    case NvOdmIoModule_Hdmi:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_Hdmi;
        *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_Hdmi);
        break;

    case NvOdmIoModule_I2c:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_I2c;
        *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_I2c);
        break;

    case NvOdmIoModule_I2c_Pmu:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_I2cPmu;
        *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_I2cPmu);
        break;

    case NvOdmIoModule_Nand:
        if (IsBootDevNand == NV_TRUE)
        {
            *pPinMuxConfigTable = s_NvOdmPinMuxConfig_Nand;
            *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_Nand);
        }
        else
        {
            *pPinMuxConfigTable = NULL;
            *pCount = 0;
        }
        break;

    case NvOdmIoModule_SyncNor:
        if (IsBootDevNor == NV_TRUE)
        {
            *pPinMuxConfigTable = s_NvOdmPinMuxConfig_SyncNor;
            *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_SyncNor);
        }
        else
        {
            *pPinMuxConfigTable = NULL;
            *pCount = 0;
        }
        break;

    case NvOdmIoModule_Sdio:
       *pPinMuxConfigTable = s_NvOdmPinMuxConfig_Sdio;
       *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_Sdio);
        break;

    case NvOdmIoModule_Spi:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_Spi;
        *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_Spi);
        break;

    case NvOdmIoModule_Uart:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_Uart;
        *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_Uart);
        break;

    case NvOdmIoModule_ExternalClock:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_ExternalClock;
        *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_ExternalClock);
        break;

    case NvOdmIoModule_Crt:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_Crt;
        *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_Crt);
        break;

    case NvOdmIoModule_PciExpress:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_PciExpress;
        *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_PciExpress);
        break;

    case NvOdmIoModule_Tvo:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_Tvo;
        *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_Tvo);
        break;
    case NvOdmIoModule_BacklightPwm:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_BacklightPwm;
        *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_BacklightPwm);
        break;

    case NvOdmIoModule_Hdcp:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_Hdcp;
        *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_Hdcp);
        break;

    case NvOdmIoModule_Ulpi:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_Ulpi;
        *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmPinMuxConfig_Ulpi);
        break;

    case NvOdmIoModule_VideoInput:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_VideoInput;
        *pCount = NV_ARRAY_SIZE(s_NvOdmPinMuxConfig_VideoInput);
        break;

    case NvOdmIoModule_Spdif:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_Spdif;
        *pCount = NV_ARRAY_SIZE(s_NvOdmPinMuxConfig_Spdif);
        break;

    case NvOdmIoModule_Ata:
    case NvOdmIoModule_Kbd:
    case NvOdmIoModule_Hsi:
    case NvOdmIoModule_Mio:
    case NvOdmIoModule_OneWire:
        *pPinMuxConfigTable = NULL;
        *pCount = 0;
        break;

    default:
        *pCount = 0;
        break;
    }
}

void
NvOdmQueryClockLimits(
    NvOdmIoModule IoModule,
    const NvU32 **pClockSpeedLimits,
    NvU32 *pCount)
{
    switch (IoModule)
    {
        case NvOdmIoModule_Sdio:
            *pClockSpeedLimits = s_NvOdmClockLimit_Sdio;
            *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmClockLimit_Sdio);
            break;
        case NvOdmIoModule_Nand:
            *pClockSpeedLimits = s_NvOdmClockLimit_Nand;
            *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmClockLimit_Nand);
            break;
        case NvOdmIoModule_SyncNor:
            *pClockSpeedLimits = s_NvOdmClockLimit_SyncNor;
            *pCount = NVODM_PINMUX_ARRAY_SIZE(s_NvOdmClockLimit_SyncNor);
            break;

        default:
            *pClockSpeedLimits = NULL;
            *pCount = 0;
            break;
    }
}
