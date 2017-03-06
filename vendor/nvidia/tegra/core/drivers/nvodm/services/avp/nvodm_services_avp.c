/*
 * Copyright (c) 2010 - 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_services.h"
#include "nvcommon.h"
#include "nvassert.h"
#include "nvodm_query.h"
#include "nvodm_query_gpio.h"
#include "nvodm_query_discovery.h"
#include "nvrm_gpio.h"
#include "nvrm_i2c.h"
#include "nvrm_pmu.h"
#include "nvrm_keylist.h"
#include "nvrm_power.h"
#include "nvrm_analog.h"
#ifndef SET_KERNEL_PINMUX
#include "nvrm_pinmux.h"
#endif
#include "nvrm_hardware_access.h"
#include "nvboot_clocks_int.h"
#include "ap20/ardvc.h"
#include "ap20/artimerus.h"
#include "ap20/arslink.h"
#include "ap20/arapb_misc.h"
#include "ap20/arclk_rst.h"
#include "ap20/arapbpm.h"

#define PMC_PA_BASE      0x7000E400

// Maximum SPI instance number
#define MAX_SPI_INSTANCES    4

// Maximum chipselect available for per spi/slink channel.
#define MAX_CHIPSELECT_PER_INSTANCE 4

// Maximum buffer size when transferring the data using the cpu.
enum {MAX_CPU_TRANSACTION_SIZE_WORD  = 0x80};  // 256 bytes

// Maximum Slink Fifo Width
enum {MAX_SLINK_FIFO_DEPTH =  32};

// Maximum  PLL_P clock frequncy in KHZ
#define MAX_PLLP_CLOCK_FREQUENY_INKHZ 216000

// Maximum number of cycles for chip select should stay inactive
#define MAX_INACTIVE_CHIPSELECT_SETUP_CYCLES 3

// Hardware wait time after write
#define HW_WAIT_TIME_IN_US 10

// Number of bits per byte
#define BITS_PER_BYTE  8

// Maximum number which  is return by the NvOsGetTimeMS()
#define MAX_TIME_IN_MS 0xFFFFFFFF

//base adresses from project.h
#define NV_ADDRESS_MAP_APB_MISC_BASE        0x70000000
#define NV_ADDRESS_MAP_APB_DVC_BASE         0x7000D000
#define NV_ADDRESS_MAP_PPSB_CLK_RST_BASE    0x60006000
#define NV_ADDRESS_MAP_TMRUS_BASE           0x60005010
#define NV_ADDRESS_MAP_APB_SBC1_BASE        0x7000D400
#define NV_ADDRESS_MAP_APB_SBC2_BASE        0x7000D600
#define NV_ADDRESS_MAP_APB_SBC3_BASE        0x7000D800
#define NV_ADDRESS_MAP_APB_SBC4_BASE        0x7000DA00

#define ALL_SLINK_STATUS_CLEAR \
        (NV_DRF_NUM(SLINK, STATUS,  RDY, 1) | \
        NV_DRF_NUM(SLINK, STATUS,  RX_UNF, 1) | \
        NV_DRF_NUM(SLINK, STATUS,  TX_UNF, 1) | \
        NV_DRF_NUM(SLINK, STATUS,  TX_OVF, 1) | \
        NV_DRF_NUM(SLINK, STATUS,  RX_OVF, 1))

#define RX_ERROR_STATUS (NV_DRF_NUM(SLINK, STATUS, RX_UNF, 1) | \
                            NV_DRF_NUM(SLINK, STATUS, RX_OVF, 1))
#define TX_ERROR_STATUS (NV_DRF_NUM(SLINK, STATUS, TX_OVF, 1) | \
                            NV_DRF_NUM(SLINK, STATUS, TX_UNF, 1))

#ifndef SET_KERNEL_PINMUX
#define TRISTATE_SLINK(reg,group) \
    TristateReg = NV_READ32(NV_ADDRESS_MAP_APB_MISC_BASE + APB_MISC_PP_TRISTATE_REG_##reg##_0); \
    TristateReg = NV_FLD_SET_DRF_DEF(APB_MISC_PP, TRISTATE_REG_##reg, \
    Z_##group, NORMAL, TristateReg); \
    NV_WRITE32(NV_ADDRESS_MAP_APB_MISC_BASE + APB_MISC_PP_TRISTATE_REG_##reg##_0, TristateReg);

#define PINMUX_SLINK(reg,group,slink) \
    PinMuxReg = NV_READ32(NV_ADDRESS_MAP_APB_MISC_BASE + APB_MISC_PP_PIN_MUX_CTL_##reg##_0); \
    PinMuxReg = NV_FLD_SET_DRF_DEF(APB_MISC_PP, PIN_MUX_CTL_##reg, group##_SEL, \
    SPI##slink, PinMuxReg); \
    NV_WRITE32(NV_ADDRESS_MAP_APB_MISC_BASE + APB_MISC_PP_PIN_MUX_CTL_##reg##_0, PinMuxReg);
#endif

// Wrapper macros for reading/writing from/to SPI registers
#define SLINK_REG_READ32(pSlinkHwRegsVirtBaseAdd, reg) \
    NV_READ32((pSlinkHwRegsVirtBaseAdd) + ((SLINK_##reg##_0)/4))

#define SLINK_REG_WRITE32(pSlinkHwRegsVirtBaseAdd, reg, val) \
    do { \
        NV_WRITE32((((pSlinkHwRegsVirtBaseAdd) + ((SLINK_##reg##_0)/4))), (val)); \
    } while(0)

#define NV_DVC_READ(reg, value)                                         \
    do                                                                  \
    {                                                                   \
        value = NV_READ32(NV_ADDRESS_MAP_APB_DVC_BASE + DVC_##reg##_0); \
    } while (0)

#define NV_DVC_WRITE(reg, value)                                        \
    do                                                                  \
    {                                                                   \
        NV_WRITE32((NV_ADDRESS_MAP_APB_DVC_BASE                         \
                    + DVC_##reg##_0), value);                           \
    } while (0)

#define NV_CLK_RST_READ(reg, value)                                     \
    do                                                                  \
    {                                                                   \
        value = NV_READ32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE              \
                          + CLK_RST_CONTROLLER_##reg##_0);              \
    } while (0)

#define NV_CLK_RST_WRITE(reg, value)                                    \
    do                                                                  \
    {                                                                   \
        NV_WRITE32((NV_ADDRESS_MAP_PPSB_CLK_RST_BASE                    \
                    + CLK_RST_CONTROLLER_##reg##_0), value);            \
    } while (0)


typedef struct
{
    NvU32 *pTxBuff;
    NvU32 *pRxBuff;
    NvU32 BytesPerPacket;
    NvU32 PacketBitLength;
    NvU32 PacketsPerWord;
    NvU32 PacketRequested;
    NvU32 PacketTransferred;
    NvU32 TotalPacketsRemaining;
    NvU32 RxPacketsRemaining;
    NvU32 TxPacketsRemaining;
    NvU32 CurrPacketCount;
} TransferBufferInfo;

typedef enum
{
    // No data transfer.
    SerialDataFlow_None    = 0x0,

    // Receive data flow.
    SerialDataFlow_Rx      = 0x1,

    // Transmit data flow.
    SerialDataFlow_Tx      = 0x2,

    SerialDataFlow_Force32 = 0x7FFFFFFF
} SerialDataFlow;

typedef struct NvOdmServicesSpiRec
{
    // SPI instance Id
    NvU32 InstanceId;
    // Instance Open Count
    NvU32 OpenCount;
    // Chip select Id
    NvU32 ChipSelectId;
    // SPI Clock frequency in KHZ
    NvU32 ClockFreqInKHz;
    // Base address of the controller
    volatile NvU32 *pRegBaseAdd;
    // Mux Pinmap for the selected instance
    NvOdmSpiPinMap SpiPinMap;
    // Idle states for the controller instance
    NvBool IsIdleSignalTristate;
    NvOdmQuerySpiSignalMode IdleSignalMode;
    NvBool IsIdleDataOutHigh;

    // Current signal mode
    NvOdmQuerySpiSignalMode CurSignalMode;
    // SPI Device info
    NvOdmQuerySpiDeviceInfo DeviceInfo[MAX_CHIPSELECT_PER_INSTANCE];
    // Chip select level
    NvBool IsCurrentChipSelStateHigh[MAX_CHIPSELECT_PER_INSTANCE];
    // Is chip select support
    NvBool IsChipSelSupported[MAX_CHIPSELECT_PER_INSTANCE];
    // Is chip select configured
    NvBool IsChipSelConfigured;
    // Is Hw based chip select supported
    NvBool IsHwChipSelectSupported;
    // Is Packed Mode
    NvBool IsPackedMode;
    // Current transfer info
    TransferBufferInfo CurrTransInfo;
    // Current data flow direction
    SerialDataFlow CurrentDirection;
    // pointer to receive CPU buffer
    NvU32 *pRxCpuBuffer;
    // pointer to transmit CPU buffer
    NvU32 *pTxCpuBuffer;
    // Recieve status error
    NvError RxTransferStatus;
    // Transfer status error
    NvError TxTransferStatus;
} NvOdmServicesSpi;

static NvOdmServicesSpi * s_hOdmSpiHandle[MAX_SPI_INSTANCES] = {NULL};

static NvU32
GetTimeMS(void)
{
    NvU32 timeUs = NV_READ32(NV_ADDRESS_MAP_TMRUS_BASE + TIMERUS_CNTR_1US_0);
    return timeUs >> 10; // avoid using division
}

static void PrivI2cInit(void)
{
    NvU32 Reg;

    // Avoid running this function more than once.
    static int initialized = 0;
    if (initialized) return;
    initialized = 1;

    // Configure I2C clock.
    // Use the oscillator input.
    // Divisor value is a placeholder;
    // The value is overwritten by NvOdmI2cTransaction().
    NV_CLK_RST_READ(CLK_SOURCE_DVC_I2C, Reg);
    Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_DVC_I2C,
                             DVC_I2C_CLK_SRC, CLK_M, Reg);
    Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_DVC_I2C,
                             DVC_I2C_CLK_DIVISOR, 100 - 1, Reg);
    NV_CLK_RST_WRITE(CLK_SOURCE_DVC_I2C, Reg);

    // Enable I2C clock.
    NV_CLK_RST_READ(CLK_OUT_ENB_H, Reg);
    Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H,
                             CLK_ENB_DVC_I2C, ENABLE, Reg);
    NV_CLK_RST_WRITE(CLK_OUT_ENB_H, Reg);

    // Reset I2C controller.
    NV_CLK_RST_READ(RST_DEVICES_H, Reg);
    Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H,
                             SWR_DVC_I2C_RST, ENABLE, Reg);
    NV_CLK_RST_WRITE(RST_DEVICES_H, Reg);
    Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H,
                             SWR_DVC_I2C_RST, DISABLE, Reg);
    NV_CLK_RST_WRITE(RST_DEVICES_H, Reg);

    // Configure I2C controller.
    NV_DVC_READ(CTRL_REG3, Reg);
    Reg = NV_FLD_SET_DRF_DEF(DVC, CTRL_REG3,
                             I2C_HW_SW_PROG, SW, Reg);
    NV_DVC_WRITE(CTRL_REG3, Reg);
}


/* SPI static helper functions */

/* Get Device information from odm query */
static NvBool
SpiGetDeviceInfo(
    NvU32 InstanceId,
    NvU32 ChipSelect,
    NvOdmQuerySpiDeviceInfo *pDeviceInfo)
{
    const NvOdmQuerySpiDeviceInfo *pSpiDevInfo = NULL;
    pSpiDevInfo = NvOdmQuerySpiGetDeviceInfo(NvOdmIoModule_Spi, InstanceId, ChipSelect);
    if (!pSpiDevInfo)
    {
        // No device info in odm, so set it on default info
        pDeviceInfo->SignalMode = NvOdmQuerySpiSignalMode_0;
        pDeviceInfo->ChipSelectActiveLow = NV_TRUE;
        pDeviceInfo->CanUseHwBasedCs = NV_FALSE;
        pDeviceInfo->CsHoldTimeInClock = 0;
        pDeviceInfo->CsSetupTimeInClock = 0;
        return NV_FALSE;
    }
    pDeviceInfo->SignalMode = pSpiDevInfo->SignalMode;
    pDeviceInfo->ChipSelectActiveLow = pSpiDevInfo->ChipSelectActiveLow;
    pDeviceInfo->CanUseHwBasedCs = pSpiDevInfo->CanUseHwBasedCs;
    pDeviceInfo->CsHoldTimeInClock = pSpiDevInfo->CsHoldTimeInClock;
    pDeviceInfo->CsSetupTimeInClock = pSpiDevInfo->CsSetupTimeInClock;
    return NV_TRUE;
}

#ifndef SET_KERNEL_PINMUX
/* MUX related helpers */
static void SPI1MuxConfigure(NvU32 SpiPinMap)
{
    NvU32 TristateReg;
    NvU32 PinMuxReg;
    switch (SpiPinMap)
    {
        case NvOdmSpiPinMap_Config1:
            TRISTATE_SLINK(D,UDA);
            PINMUX_SLINK(A,UDA,1);
            break;
        case NvOdmSpiPinMap_Config2:
            TRISTATE_SLINK(A,DTE);
            TRISTATE_SLINK(A,DTB);
            PINMUX_SLINK(B,DTE,1);
            PINMUX_SLINK(B,DTB,1);
            break;
        case NvOdmSpiPinMap_Config3:
            TRISTATE_SLINK(B,SPIC);
            TRISTATE_SLINK(B,SPIB);
            TRISTATE_SLINK(B,SPIA);
            PINMUX_SLINK(D,SPIC,1);
            PINMUX_SLINK(D,SPIB,1);
            PINMUX_SLINK(D,SPIA,1);
            break;
        case NvOdmSpiPinMap_Config4:
            TRISTATE_SLINK(B,SPIE);
            TRISTATE_SLINK(B,SPIF);
            TRISTATE_SLINK(B,SPID);
            PINMUX_SLINK(D,SPIE,1);
            PINMUX_SLINK(D,SPIF,1);
            PINMUX_SLINK(D,SPID,1);
            break;
        default:
            NV_ASSERT(!"Invalid SpiPinMap");
            break;
    }
}

static void SPI2MuxConfigure(NvU32 SpiPinMap)
{
    NvU32 TristateReg;
    NvU32 PinMuxReg;
    switch (SpiPinMap)
    {
        case NvOdmSpiPinMap_Config1:
            TRISTATE_SLINK(B,UAB);
            PINMUX_SLINK(A,UAB,2);
            break;
        case NvOdmSpiPinMap_Config2:
            TRISTATE_SLINK(B,SPIC);
            TRISTATE_SLINK(B,SPIB);
            TRISTATE_SLINK(B,SPIA);
            TRISTATE_SLINK(B,SPIG);
            TRISTATE_SLINK(B,SPIH);
            PINMUX_SLINK(D,SPIC,2);
            PINMUX_SLINK(D,SPIB,2);
            PINMUX_SLINK(D,SPIA,2);
            PINMUX_SLINK(D,SPIG,2);
            PINMUX_SLINK(D,SPIH,2);
            break;
        case NvOdmSpiPinMap_Config3:
            TRISTATE_SLINK(B,SPIE);
            TRISTATE_SLINK(B,SPIF);
            TRISTATE_SLINK(B,SPID);
            TRISTATE_SLINK(B,SPIG);
            TRISTATE_SLINK(B,SPIH);
            PINMUX_SLINK(D,SPIE,2_ALT);
            PINMUX_SLINK(D,SPIF,2);
            PINMUX_SLINK(D,SPID,2_ALT);
            PINMUX_SLINK(D,SPIG,2_ALT);
            PINMUX_SLINK(D,SPIH,2_ALT);
            break;
        case NvOdmSpiPinMap_Config4:
            TRISTATE_SLINK(B,SPIC);
            TRISTATE_SLINK(B,SPIB);
            TRISTATE_SLINK(B,SPIA);
            PINMUX_SLINK(D,SPID,2);
            PINMUX_SLINK(D,SPIB,2);
            PINMUX_SLINK(D,SPIA,2);
            break;
        case NvOdmSpiPinMap_Config5:
            TRISTATE_SLINK(B,SPIE);
            TRISTATE_SLINK(B,SPIF);
            TRISTATE_SLINK(B,SPID);
            PINMUX_SLINK(D,SPIE,2_ALT);
            PINMUX_SLINK(D,SPIF,2);
            PINMUX_SLINK(D,SPID,2_ALT);
            break;
        default:
            NV_ASSERT(!"Invalid SpiPinMap");
            break;
    }
}

static void SPI3MuxConfigure(NvU32 SpiPinMap)
{
    NvU32 TristateReg;
    NvU32 PinMuxReg;
    switch (SpiPinMap)
    {
        case NvOdmSpiPinMap_Config1:
            TRISTATE_SLINK(B,UAA);
            PINMUX_SLINK(A,UAA,3);
            break;
        case NvOdmSpiPinMap_Config2:
            TRISTATE_SLINK(C,LSC1);
            TRISTATE_SLINK(D,LPW2);
            TRISTATE_SLINK(D,LPW0);
            TRISTATE_SLINK(C,LM0);
            PINMUX_SLINK(E,LSC1,3);
            PINMUX_SLINK(E,LPW2,3);
            PINMUX_SLINK(E,LPW0,3);
            PINMUX_SLINK(E,LM0,3);
            break;
        case NvOdmSpiPinMap_Config3:
            TRISTATE_SLINK(C,LSCK);
            TRISTATE_SLINK(D,LSDI);
            TRISTATE_SLINK(D,LSDA);
            TRISTATE_SLINK(C,LCSN);
            PINMUX_SLINK(E,LSCK,3);
            PINMUX_SLINK(E,LSDI,3);
            PINMUX_SLINK(E,LSDA,3);
            PINMUX_SLINK(E,LCSN,3);
            break;
        case NvOdmSpiPinMap_Config4:
            TRISTATE_SLINK(A,GMA);
            PINMUX_SLINK(B,GMA,3);
            break;
        case NvOdmSpiPinMap_Config5:
            TRISTATE_SLINK(B,SPIC);
            TRISTATE_SLINK(B,SPIB);
            TRISTATE_SLINK(B,SPIA);
            PINMUX_SLINK(D,SPIC,3);
            PINMUX_SLINK(D,SPIB,3);
            PINMUX_SLINK(D,SPIA,3);
            break;
        case NvOdmSpiPinMap_Config6:
            TRISTATE_SLINK(B,SDC);
            TRISTATE_SLINK(B,SDD);
            PINMUX_SLINK(D,SDC,3);
            PINMUX_SLINK(D,SDD,3);
            break;
        case NvOdmSpiPinMap_Multiplexed:
            TRISTATE_SLINK(B,SPIA);
            TRISTATE_SLINK(B,SPIF);
            TRISTATE_SLINK(B,SPIG);
            TRISTATE_SLINK(B,SPIH);
            PINMUX_SLINK(D,SPIA,3);
            PINMUX_SLINK(D,SPIF,3);
            PINMUX_SLINK(D,SPIG,3);
            PINMUX_SLINK(D,SPIH,3);
            break;
        default:
            NV_ASSERT(!"Invalid SpiPinMap");
            break;
    }
}

static void SPI4MuxConfigure(NvU32 SpiPinMap)
{
    NvU32 TristateReg;
    NvU32 PinMuxReg;
    switch (SpiPinMap)
    {
        case NvOdmSpiPinMap_Config1:
            TRISTATE_SLINK(B,UAD);
            TRISTATE_SLINK(A,IRRX);
            TRISTATE_SLINK(A,IRTX);
            PINMUX_SLINK(A,UAD,4);
            PINMUX_SLINK(C,IRRX,4);
            PINMUX_SLINK(C,IRTX,4);
            break;
        case NvOdmSpiPinMap_Config2:
            TRISTATE_SLINK(A,GMC);
            PINMUX_SLINK(B,GMC,4);
            break;
        case NvOdmSpiPinMap_Config3:
            TRISTATE_SLINK(B,SLXC);
            TRISTATE_SLINK(B,SLXK);
            TRISTATE_SLINK(B,SLXA);
            TRISTATE_SLINK(B,SLXD);
            PINMUX_SLINK(B,SLXC,4);
            PINMUX_SLINK(B,SLXK,4);
            PINMUX_SLINK(B,SLXA,4);
            PINMUX_SLINK(B,SLXD,4);
            break;
        default:
            NV_ASSERT(!"Invalid SpiPinMap");
            break;
    }
}

static void
SpiMuxConfigSelect(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU32 SpiPinMap)
{
    NvU32 Reg;

    switch (hOdmSpi->InstanceId)
    {
        case 0:
            SPI1MuxConfigure(SpiPinMap);
            break;
        case 1:
            SPI2MuxConfigure(SpiPinMap);
            break;
        case 2:
            SPI3MuxConfigure(SpiPinMap);
            break;
        case 3:
            SPI4MuxConfigure(SpiPinMap);
            break;
        default:
            NV_ASSERT(!"Invalid SpiInstanceId");
            break;
   }
}
#endif

/* Clock related helper functions */
static void
SpiClockEnable(
    NvU32 InstanceId,
    NvBool Enable)
{
    NvU32 Reg;
    switch (InstanceId)
    {
        case 0:
            NV_CLK_RST_READ(CLK_OUT_ENB_H, Reg);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_OUT_ENB_H,
                             CLK_ENB_SBC1, Enable, Reg);
            NV_CLK_RST_WRITE(CLK_OUT_ENB_H, Reg);
            break;
        case 1:
            NV_CLK_RST_READ(CLK_OUT_ENB_H, Reg);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_OUT_ENB_H,
                             CLK_ENB_SBC2, Enable, Reg);
            NV_CLK_RST_WRITE(CLK_OUT_ENB_H, Reg);
            break;
        case 2:
            NV_CLK_RST_READ(CLK_OUT_ENB_H, Reg);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_OUT_ENB_H,
                             CLK_ENB_SBC3, Enable, Reg);
            NV_CLK_RST_WRITE(CLK_OUT_ENB_H, Reg);
            break;
        case 3:
            NV_CLK_RST_READ(CLK_OUT_ENB_H, Reg);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_OUT_ENB_U,
                             CLK_ENB_SBC4, Enable, Reg);
            NV_CLK_RST_WRITE(CLK_OUT_ENB_H, Reg);
            break;
        default:
            NV_ASSERT(!"Invalid SpiInstanceId");
            break;
   }
}

static void
SpiSetClockSpeed(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU32 ClockSpeedInKHz)
{
    NvU32 PrefClockFreqInKHz;
    NvU32 Divisor;
    NvU32 Reg;
    if(hOdmSpi->ClockFreqInKHz != ClockSpeedInKHz)
    {
        // The slink clock source should be 4 times of the interface clock speed
        PrefClockFreqInKHz = (ClockSpeedInKHz << 2);
        // Setup divisor.
        Divisor = MAX_PLLP_CLOCK_FREQUENY_INKHZ / PrefClockFreqInKHz;
        Divisor = Divisor << 1;

        switch(hOdmSpi->InstanceId)
        {
            case 0:
                NV_CLK_RST_READ(CLK_SOURCE_SBC1, Reg);
                Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_SBC1,
                         SBC1_CLK_DIVISOR, Divisor - 1, Reg);
                NV_CLK_RST_WRITE(CLK_SOURCE_SBC1, Reg);
                break;
            case 1:
                NV_CLK_RST_READ(CLK_SOURCE_SBC2, Reg);
                Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_SBC2,
                             SBC2_CLK_DIVISOR, Divisor - 1, Reg);
                NV_CLK_RST_WRITE(CLK_SOURCE_SBC2, Reg);
                break;
            case 2:
                NV_CLK_RST_READ(CLK_SOURCE_SBC3, Reg);
                Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_SBC3,
                          SBC3_CLK_DIVISOR, Divisor - 1, Reg);
                NV_CLK_RST_WRITE(CLK_SOURCE_SBC3, Reg);
                break;
            case 3:
                NV_CLK_RST_READ(CLK_SOURCE_SBC4, Reg);
                Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_SBC4,
                         SBC4_CLK_DIVISOR, Divisor - 1, Reg);
                NV_CLK_RST_WRITE(CLK_SOURCE_SBC4, Reg);
                break;
            default:
                NV_ASSERT(!"Invalid SpiInstanceId");
                break;
           }
           NvOsWaitUS(HW_WAIT_TIME_IN_US);
           hOdmSpi->ClockFreqInKHz = ClockSpeedInKHz;
        }
}

static void
SpiModuleReset(
    NvU32 InstanceId,
    NvBool Enable)
{
    NvU32 Reg;
    switch (InstanceId)
    {
        case 0:
            NV_CLK_RST_READ(RST_DEVICES_H, Reg);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_H,
                             SWR_SBC1_RST, Enable, Reg);
            NV_CLK_RST_WRITE(RST_DEVICES_H, Reg);
            break;
        case 1:
            NV_CLK_RST_READ(RST_DEVICES_H, Reg);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_H,
                             SWR_SBC2_RST, Enable, Reg);
            NV_CLK_RST_WRITE(RST_DEVICES_H, Reg);
            break;
        case 2:
            NV_CLK_RST_READ(RST_DEVICES_H, Reg);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_H,
                             SWR_SBC3_RST, Enable, Reg);
            NV_CLK_RST_WRITE(RST_DEVICES_H, Reg);
            break;
        case 3:
            NV_CLK_RST_READ(RST_DEVICES_H, Reg);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_U,
                            SWR_SBC4_RST, Enable, Reg);
            NV_CLK_RST_WRITE(RST_DEVICES_H, Reg);
            break;
       default:
            break;
   }
}

/**
 * Set the signal mode in the command register.
 */
static void
SpiSetSignalMode(
    NvOdmServicesSpiHandle hOdmSpi,
    NvOdmQuerySpiSignalMode SpiSignalMode)
{
    NvU32 CommandReg = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, COMMAND);
    switch (SpiSignalMode)
    {
        case NvOdmQuerySpiSignalMode_0:
            CommandReg = NV_FLD_SET_DRF_DEF(SLINK, COMMAND, IDLE_SCLK,
                DRIVE_LOW, CommandReg);
            CommandReg = NV_FLD_SET_DRF_DEF(SLINK, COMMAND, CK_SDA, FIRST_CLK_EDGE,
                CommandReg);
            break;
        case NvOdmQuerySpiSignalMode_1:
            CommandReg = NV_FLD_SET_DRF_DEF(SLINK, COMMAND, IDLE_SCLK,
                DRIVE_LOW, CommandReg);
            CommandReg = NV_FLD_SET_DRF_DEF(SLINK, COMMAND, CK_SDA, SECOND_CLK_EDGE,
                CommandReg);
            break;
        case NvOdmQuerySpiSignalMode_2:
            CommandReg = NV_FLD_SET_DRF_DEF(SLINK, COMMAND, IDLE_SCLK,
                DRIVE_HIGH, CommandReg);
            CommandReg = NV_FLD_SET_DRF_DEF(SLINK, COMMAND, CK_SDA, FIRST_CLK_EDGE,
                CommandReg);
            break;
        case NvOdmQuerySpiSignalMode_3:
            CommandReg = NV_FLD_SET_DRF_DEF(SLINK, COMMAND, IDLE_SCLK,
                DRIVE_HIGH, CommandReg);
            CommandReg = NV_FLD_SET_DRF_DEF(SLINK, COMMAND, CK_SDA, SECOND_CLK_EDGE,
                CommandReg);
            break;
        default:
            NV_ASSERT(!"Invalid SignalMode");
            break;
    }
    hOdmSpi->CurSignalMode = SpiSignalMode;
    SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, COMMAND, CommandReg);
}

/**
 * Set the chip select polarity bit in the command register.
 */
static void
SpiSetChipSelectPolarity(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU32 ChipSelectId,
    NvBool IsHigh)
{
    NvU32 CSPolVal = (IsHigh)?0:1;
    NvU32 CommandReg = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, COMMAND);
    switch (ChipSelectId)
    {
        case 0:
            CommandReg = NV_FLD_SET_DRF_NUM(SLINK, COMMAND, CS_POLARITY0,
                                                       CSPolVal, CommandReg);
            break;
        case 1:
            CommandReg = NV_FLD_SET_DRF_NUM(SLINK, COMMAND, CS_POLARITY1,
                                                       CSPolVal, CommandReg);
            break;
        case 2:
            CommandReg = NV_FLD_SET_DRF_NUM(SLINK, COMMAND, CS_POLARITY2,
                                                       CSPolVal, CommandReg);
            break;
        case 3:
            CommandReg = NV_FLD_SET_DRF_NUM(SLINK, COMMAND, CS_POLARITY3,
                                                       CSPolVal, CommandReg);
            break;
        default:
            NV_ASSERT(!"Invalid ChipSelectId");
            break;
    }
    SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, COMMAND, CommandReg);
}

/**
 * Set the chip select numbers in the command register.
 */
static void
SpiSetChipSelectNumber(
    NvOdmServicesSpiHandle hOdmSpi)
{
    NvU32 CommandReg2 = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, COMMAND2);
    switch (hOdmSpi->ChipSelectId)
    {
        case 0:
            CommandReg2 = NV_FLD_SET_DRF_DEF(SLINK, COMMAND2, SS_EN, CS0, CommandReg2);
            break;

        case 1:
            CommandReg2 = NV_FLD_SET_DRF_DEF(SLINK, COMMAND2, SS_EN, CS1, CommandReg2);
            break;

        case 2:
            CommandReg2 = NV_FLD_SET_DRF_DEF(SLINK, COMMAND2, SS_EN, CS2, CommandReg2);
            break;

        case 3:
            CommandReg2 = NV_FLD_SET_DRF_DEF(SLINK, COMMAND2, SS_EN, CS3, CommandReg2);
            break;

        default:
            NV_ASSERT(!"Invalid ChipSelectId");
            break;
    }
    SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, COMMAND2, CommandReg2);
}

static void
SpiWaitForSetUpTime(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU32 ChipSelectId)
{
    NvU32 SetupTime;
    NvU32 CommandReg2;
    if(hOdmSpi->IsHwChipSelectSupported)
    {
        // Rounding the  clock time to the cycles
        SetupTime = (hOdmSpi->DeviceInfo[ChipSelectId].CsSetupTimeInClock +1)/2;
        SetupTime = (SetupTime > MAX_INACTIVE_CHIPSELECT_SETUP_CYCLES)? \
                            MAX_INACTIVE_CHIPSELECT_SETUP_CYCLES: SetupTime;
        CommandReg2 = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, COMMAND2);
        CommandReg2 = NV_FLD_SET_DRF_NUM(SLINK, COMMAND2, SS_SETUP,
                   SetupTime, CommandReg2);
        SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, COMMAND2,CommandReg2);
    }
}

static void
SpiSetChipSelectSw(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU32 ChipSelectId)
{
    NvU32 CommandReg;
    CommandReg = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, COMMAND);
    CommandReg = NV_FLD_SET_DRF_DEF(SLINK, COMMAND, CS_SW, SOFT, CommandReg);
    SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, COMMAND, CommandReg);
}

static void
SpiSetChipSelectHw(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU32 ChipSelectId,
    NvU32 RefillCount)
{
    NvU32 CommandReg;
    NvU32 CommandReg2;
    CommandReg = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, COMMAND);
    CommandReg = NV_FLD_SET_DRF_DEF(SLINK, COMMAND, CS_SW, HARD, CommandReg);
    SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, COMMAND, CommandReg);
    CommandReg2 = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, COMMAND2);
    CommandReg2 = NV_FLD_SET_DRF_NUM(SLINK, COMMAND2, FIFO_REFILLS,
                                        RefillCount, CommandReg2);
    SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, COMMAND, CommandReg);
    SpiSetChipSelectNumber(hOdmSpi);
    SpiWaitForSetUpTime(hOdmSpi, hOdmSpi->ChipSelectId);
}


static NvBool
PrivSpiInit(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU32 InstanceId)
{
    NvU32 Reg;
    NvU32 ChipSelIndex;
#ifndef SET_KERNEL_PINMUX
    const NvU32 *pOdmConfigs;
    NvU32 NumOdmConfigs;
#endif
    NvU32 CommandReg;
    NvU32 CommandReg2;
    const NvOdmQuerySpiIdleSignalState *pSignalState = NULL;

    hOdmSpi->InstanceId = InstanceId;
    hOdmSpi->ClockFreqInKHz = 0;

    for (ChipSelIndex = 0; ChipSelIndex < MAX_CHIPSELECT_PER_INSTANCE; ++ChipSelIndex)
        hOdmSpi->IsChipSelSupported[ChipSelIndex] = SpiGetDeviceInfo(
               hOdmSpi->InstanceId, ChipSelIndex, &hOdmSpi->DeviceInfo[ChipSelIndex]);

#ifndef SET_KERNEL_PINMUX
    NvOdmQueryPinMux(NvOdmIoModule_Spi, &pOdmConfigs, &NumOdmConfigs);
    NV_ASSERT((InstanceId < NumOdmConfigs) && (pOdmConfigs[InstanceId]));
    hOdmSpi->SpiPinMap = pOdmConfigs[InstanceId];

    // Do Mux selection here
    SpiMuxConfigSelect(hOdmSpi, hOdmSpi->SpiPinMap);
#endif

    SpiClockEnable(InstanceId, NV_TRUE);
    SpiModuleReset(InstanceId, NV_TRUE);
    NvOsWaitUS(HW_WAIT_TIME_IN_US);
    SpiModuleReset(InstanceId, NV_FALSE);

    // assign reg base address
    switch (InstanceId)
    {
        case 0:
            hOdmSpi->pRegBaseAdd = (NvU32 *)NV_ADDRESS_MAP_APB_SBC1_BASE;
        break;
        case 1:
            hOdmSpi->pRegBaseAdd = (NvU32 *)NV_ADDRESS_MAP_APB_SBC2_BASE;
        break;
        case 2:
            hOdmSpi->pRegBaseAdd = (NvU32 *)NV_ADDRESS_MAP_APB_SBC3_BASE;
        break;
        case 3:
            hOdmSpi->pRegBaseAdd = (NvU32 *)NV_ADDRESS_MAP_APB_SBC4_BASE;
        break;
        default:
            NV_ASSERT(!"Invalid SpiInstanceId");
        break;
    }

    pSignalState = NvOdmQuerySpiGetIdleSignalState(NvOdmIoModule_Spi, InstanceId);
    if (pSignalState)
    {
        hOdmSpi->IsIdleSignalTristate = pSignalState->IsTristate;
        hOdmSpi->IdleSignalMode = pSignalState->SignalMode;
        hOdmSpi->IsIdleDataOutHigh = pSignalState->IsIdleDataOutHigh;
    }
    else
    {
        hOdmSpi->IsIdleSignalTristate = NV_FALSE;
        hOdmSpi->IdleSignalMode = NvOdmQuerySpiSignalMode_0;
        hOdmSpi->IsIdleDataOutHigh = NV_FALSE;
    }

    // Set the CPU Buffers
    hOdmSpi->pRxCpuBuffer = NvOsAlloc(MAX_CPU_TRANSACTION_SIZE_WORD);
    if (!hOdmSpi->pRxCpuBuffer)
        return NV_FALSE;
    hOdmSpi->pTxCpuBuffer = NvOsAlloc(MAX_CPU_TRANSACTION_SIZE_WORD);
    if (!hOdmSpi->pTxCpuBuffer)
        return NV_FALSE;

    // Do not toggle the CS between each packet.
    CommandReg2 = NV_FLD_SET_DRF_DEF(SLINK, COMMAND2, CS_ACTIVE_BETWEEN, HIGH,
                    CommandReg2);
    SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, COMMAND2, CommandReg2);

    // Set the Master mode
    CommandReg = NV_FLD_SET_DRF_DEF(SLINK, COMMAND, M_S, MASTER, CommandReg);
    if (hOdmSpi->IsIdleDataOutHigh)
        CommandReg = NV_FLD_SET_DRF_DEF(SLINK, COMMAND, IDLE_SDA, DRIVE_HIGH, CommandReg);
    else
        CommandReg = NV_FLD_SET_DRF_DEF(SLINK, COMMAND, IDLE_SDA, DRIVE_LOW, CommandReg);
    SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, COMMAND, CommandReg);

    hOdmSpi->RxTransferStatus = NvSuccess;
    hOdmSpi->TxTransferStatus = NvSuccess;
    hOdmSpi->OpenCount = 1;

    // Set the default signal mode of the spi channel.
    SpiSetSignalMode(hOdmSpi, hOdmSpi->IdleSignalMode);

    // Set chip select to non active state.
    for (ChipSelIndex = 0; ChipSelIndex < MAX_CHIPSELECT_PER_INSTANCE; ++ChipSelIndex)
    {
        hOdmSpi->IsCurrentChipSelStateHigh[ChipSelIndex] = NV_TRUE;
        if (hOdmSpi->IsChipSelSupported[ChipSelIndex])
        {
            hOdmSpi->IsCurrentChipSelStateHigh[ChipSelIndex] =
                    hOdmSpi->DeviceInfo[ChipSelIndex].ChipSelectActiveLow;
            SpiSetChipSelectPolarity(hOdmSpi, ChipSelIndex,
                            hOdmSpi->IsCurrentChipSelStateHigh[ChipSelIndex]);
         }
    }
    // Let chipselect to be stable before doing any transaction.
    NvOsWaitUS(100*HW_WAIT_TIME_IN_US);
    s_hOdmSpiHandle[InstanceId] = hOdmSpi;

    return NV_TRUE;
}

static void
SpiSetChipSelectLevel(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU32 ChipSelectId,
    NvBool IsHigh)
{
    SpiSetChipSelectPolarity(hOdmSpi, ChipSelectId, IsHigh);
    SpiSetChipSelectSw(hOdmSpi, ChipSelectId);
    SpiSetChipSelectNumber(hOdmSpi);
}

static void
SpiSetChipSelectLevelBasedOnPacket(
    NvOdmServicesSpiHandle hOdmSpi,
    NvBool IsHigh,
    NvU32 PacketRequested,
    NvU32 PacketPerWord)
{
    NvU32 MaxWordReq;
    NvU32 RefillCount = 0;
    NvBool UseSWBaseCS;

    MaxWordReq = (PacketRequested + PacketPerWord -1)/PacketPerWord;
    UseSWBaseCS = (MaxWordReq <= MAX_CPU_TRANSACTION_SIZE_WORD) ? NV_FALSE : NV_TRUE;
    if (UseSWBaseCS)
    {
        SpiSetChipSelectLevel(hOdmSpi, hOdmSpi->ChipSelectId, IsHigh);
    }
    else
    {
        RefillCount = (MaxWordReq)/MAX_SLINK_FIFO_DEPTH;
        SpiSetChipSelectHw(hOdmSpi, hOdmSpi->ChipSelectId, RefillCount);
    }
}


static void
SpiSetPacketLength(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU32 PacketLength,
    NvBool IsPackedMode)
{
    NvU32 DmaControlReg;
    NvU32 CommandReg = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, COMMAND);
    // Set the Packet Length bits minus 1
    CommandReg = NV_FLD_SET_DRF_NUM(SLINK, COMMAND,
                              BIT_LENGTH,(PacketLength - 1), CommandReg);

    DmaControlReg = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, DMA_CTL);
    // Unset the packed bit if it is there
    DmaControlReg = NV_FLD_SET_DRF_DEF(SLINK, DMA_CTL, PACKED, DISABLE, DmaControlReg);
    if(IsPackedMode)
    {
        if(PacketLength == 4)
            DmaControlReg = NV_FLD_SET_DRF_DEF(SLINK, DMA_CTL, PACK_SIZE, PACK4,
                            DmaControlReg);
        else if (PacketLength == 8)
            DmaControlReg = NV_FLD_SET_DRF_DEF(SLINK, DMA_CTL, PACK_SIZE, PACK8,
                            DmaControlReg);
        else if (PacketLength == 16)
            DmaControlReg = NV_FLD_SET_DRF_DEF(SLINK, DMA_CTL, PACK_SIZE, PACK16,
                            DmaControlReg);
        else if(PacketLength == 32)
            DmaControlReg = NV_FLD_SET_DRF_DEF(SLINK, DMA_CTL, PACK_SIZE, PACK32,
                            DmaControlReg);
    }
    else
    {
        DmaControlReg = NV_FLD_SET_DRF_DEF(SLINK, DMA_CTL, PACK_SIZE, PACK4,
                            DmaControlReg);
    }
    SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, COMMAND, CommandReg);
    SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, DMA_CTL, DmaControlReg);
    hOdmSpi->IsPackedMode = IsPackedMode;
}

static void
SpiSetDmaTransferSize(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU32 DmaBlockSize)
{
    NvU32 DmaControlReg = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, DMA_CTL);;
    // Set the DMA size for Transmit and Enable DMA in packed mode
    DmaControlReg = NV_DRF_NUM(SLINK, DMA_CTL, DMA_BLOCK_SIZE, (DmaBlockSize-1));
    SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, DMA_CTL, DmaControlReg);
}

static void
SpiSetDataFlowDirection(
    NvOdmServicesSpiHandle hOdmSpi,
    NvBool IsEnable)
{
    NvU32 CommandReg2 = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, COMMAND2);
    if (hOdmSpi->CurrentDirection & SerialDataFlow_Rx)
    {
        if (IsEnable)
            CommandReg2 = NV_FLD_SET_DRF_DEF(SLINK, COMMAND2,  RXEN,
                            ENABLE, CommandReg2);
        else
            CommandReg2 = NV_FLD_SET_DRF_DEF(SLINK, COMMAND2,  RXEN,
                            DISABLE, CommandReg2);
    }

    if (hOdmSpi->CurrentDirection & SerialDataFlow_Tx)
    {
        if (IsEnable)
            CommandReg2 = NV_FLD_SET_DRF_DEF(SLINK, COMMAND2,  TXEN,
                            ENABLE, CommandReg2);
        else
            CommandReg2 = NV_FLD_SET_DRF_DEF(SLINK, COMMAND2,  TXEN,
                            DISABLE, CommandReg2);
    }
    SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, COMMAND2, CommandReg2);
}

static NvBool
IsSpiTransferComplete( NvOdmServicesSpiHandle hOdmSpi)
{
    // Read the Status register and findout whether the SPI is Busy
    NvU32 StatusReg = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, STATUS);
    if (StatusReg & NV_DRF_DEF(SLINK, STATUS, BSY, BUSY))
        return NV_FALSE;
    return NV_TRUE;
}

static void
SpiStartTransfer(
    NvOdmServicesSpiHandle hOdmSpi,
    NvBool IsEnable)
{
    NvU32 DmaControlReg =  SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, DMA_CTL);
    if(hOdmSpi->IsPackedMode)
    {
        DmaControlReg = NV_FLD_SET_DRF_DEF(SLINK, DMA_CTL,  PACKED, ENABLE,
            DmaControlReg);
        SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, DMA_CTL, DmaControlReg);
        // HW Bug : Need to give delay after setting packed mode
        NvOsWaitUS(HW_WAIT_TIME_IN_US);
    }
    if(IsEnable)
        DmaControlReg = NV_FLD_SET_DRF_DEF(SLINK, DMA_CTL, DMA_EN, ENABLE, DmaControlReg);
    else
        DmaControlReg = NV_FLD_SET_DRF_DEF(SLINK, DMA_CTL, DMA_EN, DISABLE, DmaControlReg);
    SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, DMA_CTL, DmaControlReg);
}

static NvError
SpiGetTransferStatus(
    NvOdmServicesSpiHandle hOdmSpi,
    SerialDataFlow DataFlow)
{
    NvU32 StatusReg = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, STATUS);
    // Check for the receive error
    if (DataFlow & SerialDataFlow_Rx)
    {
        if (StatusReg & RX_ERROR_STATUS)
             return NvError_SpiReceiveError;
    }
    // Check for the transmit error
    if (DataFlow & SerialDataFlow_Tx)
    {
        if (StatusReg & TX_ERROR_STATUS)
            return NvError_SpiTransmitError;
    }
    return NvSuccess;
}

static void
SpiClearTransferStatus(
    NvOdmServicesSpiHandle hOdmSpi,
    SerialDataFlow DataFlow)
{
    NvU32 StatusReg = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, STATUS);
    // Clear all  status bits.
    StatusReg &= (~ALL_SLINK_STATUS_CLEAR);

    // Make ready clear to 1.
    StatusReg = NV_FLD_SET_DRF_NUM(SLINK, STATUS, RDY, 1, StatusReg);

    // Check for the receive error
    if (DataFlow & SerialDataFlow_Rx)
    {
        StatusReg |= RX_ERROR_STATUS;
    }

    // Check for the transmit error
    if (DataFlow & SerialDataFlow_Tx)
    {
        StatusReg |= TX_ERROR_STATUS;
    }
    // Write on slink status register.
    SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, STATUS, StatusReg);
}

static void
SpiFlushFifos(NvOdmServicesSpiHandle hOdmSpi)
{
    NvU32 ResetBits = 0;
    NvU32 StatusReg = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, STATUS);
    ResetBits = NV_DRF_NUM(SLINK, STATUS, RX_FLUSH, 1);
    ResetBits |= NV_DRF_NUM(SLINK, STATUS,TX_FLUSH, 1);
    StatusReg |= ResetBits;
    SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, STATUS, StatusReg);

    // Now wait till the flush bits become 0
    do
    {
        StatusReg = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, STATUS);
    } while (StatusReg & ResetBits);
}

/**
 * Write into the transmit fifo register.
 * returns the number of words written.
 */
static NvU32
SpiWriteInTransmitFifo(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU32 *pTxBuff,
    NvU32 WordRequested)
{
    NvU32 WordWritten = 0;
    NvU32 WordsRemaining;
    NvU32 SlinkFifoEmptyCountReg = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, STATUS2);
    SlinkFifoEmptyCountReg = NV_DRF_VAL(SLINK, STATUS2, TX_FIFO_EMPTY_COUNT, SlinkFifoEmptyCountReg);
    WordsRemaining = NV_MIN(WordRequested, SlinkFifoEmptyCountReg);
    WordWritten = WordsRemaining;
    while (WordsRemaining)
    {
        SLINK_REG_WRITE32(hOdmSpi->pRegBaseAdd, TX_FIFO, *pTxBuff);
        pTxBuff++;
        WordsRemaining--;
    }
    return WordWritten;
}

/**
 * Read the data from the receive fifo.
 * Returns the number of words it read.
 */
static NvU32
SpiReadFromReceiveFifo(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU32 *pRxBuff,
    NvU32 WordRequested)
{
    NvU32 WordsRemaining;
    NvU32 WordsRead;
    NvU32 SlinkFifoFullCountReg =  SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, STATUS2);
    SlinkFifoFullCountReg = NV_DRF_VAL(SLINK, STATUS2, RX_FIFO_FULL_COUNT, SlinkFifoFullCountReg);
    WordsRemaining = NV_MIN(WordRequested, SlinkFifoFullCountReg);
    WordsRead = WordsRemaining;
    while (WordsRemaining)
    {
        *pRxBuff = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, RX_FIFO);
        pRxBuff++;
        WordsRemaining--;
    }
    return WordsRead;
}

static NvU32
SpiGetTransferdCount(NvOdmServicesSpiHandle hOdmSpi)
{
    NvU32 DmaBlockSize;
    NvU32 StatusReg = SLINK_REG_READ32(hOdmSpi->pRegBaseAdd, STATUS);
    DmaBlockSize = NV_DRF_VAL(SLINK, STATUS, BLK_CNT, StatusReg);
    return (DmaBlockSize);
}


static NvBool
SpiHandleTransferComplete( NvOdmServicesSpiHandle hOdmSpi)
{
    NvU32 WordsReq;
    NvU32 WordsRead;
    NvU32 CurrPacketSize;
    NvU32 WordsWritten;

    if (hOdmSpi->CurrentDirection & SerialDataFlow_Tx)
        hOdmSpi->TxTransferStatus = SpiGetTransferStatus(
                                     hOdmSpi, SerialDataFlow_Tx);

    if (hOdmSpi->CurrentDirection & SerialDataFlow_Rx)
        hOdmSpi->RxTransferStatus = SpiGetTransferStatus(
                                     hOdmSpi, SerialDataFlow_Rx);

    SpiClearTransferStatus(hOdmSpi, hOdmSpi->CurrentDirection);

    // Any error then stop the transfer and return.
    if (hOdmSpi->RxTransferStatus || hOdmSpi->TxTransferStatus)
    {
        SpiSetDataFlowDirection(hOdmSpi, NV_FALSE);
        SpiFlushFifos(hOdmSpi);
        hOdmSpi->CurrTransInfo.PacketTransferred +=
                               SpiGetTransferdCount(hOdmSpi);
        hOdmSpi->CurrentDirection = SerialDataFlow_None;
        return NV_TRUE;
    }

    if ((hOdmSpi->CurrentDirection & SerialDataFlow_Rx) &&
                                (hOdmSpi->CurrTransInfo.RxPacketsRemaining))
    {
        WordsReq = ((hOdmSpi->CurrTransInfo.CurrPacketCount) +
                        ((hOdmSpi->CurrTransInfo.PacketsPerWord) -1))/
                        (hOdmSpi->CurrTransInfo.PacketsPerWord);
        WordsRead = SpiReadFromReceiveFifo(hOdmSpi,
                            hOdmSpi->CurrTransInfo.pRxBuff, WordsReq);
        hOdmSpi->CurrTransInfo.RxPacketsRemaining -=
                                hOdmSpi->CurrTransInfo.CurrPacketCount;
        hOdmSpi->CurrTransInfo.PacketTransferred +=
                                hOdmSpi->CurrTransInfo.CurrPacketCount;
        hOdmSpi->CurrTransInfo.pRxBuff += WordsRead;
     }

    if ((hOdmSpi->CurrentDirection & SerialDataFlow_Tx) &&
                            (hOdmSpi->CurrTransInfo.TxPacketsRemaining))
    {
        WordsReq = (hOdmSpi->CurrTransInfo.TxPacketsRemaining +
                   hOdmSpi->CurrTransInfo.PacketsPerWord -1)/
                        hOdmSpi->CurrTransInfo.PacketsPerWord;
        WordsWritten = SpiWriteInTransmitFifo(
                        hOdmSpi, hOdmSpi->CurrTransInfo.pTxBuff, WordsReq);
        CurrPacketSize = NV_MIN(hOdmSpi->CurrTransInfo.PacketsPerWord * WordsWritten,
                            hOdmSpi->CurrTransInfo.TxPacketsRemaining);
        SpiStartTransfer(hOdmSpi,NV_FALSE);
        hOdmSpi->CurrTransInfo.CurrPacketCount = CurrPacketSize;
        hOdmSpi->CurrTransInfo.TxPacketsRemaining -= CurrPacketSize;
        hOdmSpi->CurrTransInfo.PacketTransferred += CurrPacketSize;
        hOdmSpi->CurrTransInfo.pTxBuff += WordsWritten;
        return NV_FALSE;
    }

    // If still need to do the transfer for receiving the data then start now.
    if ((hOdmSpi->CurrentDirection & SerialDataFlow_Rx) &&
                            (hOdmSpi->CurrTransInfo.RxPacketsRemaining))
    {
        CurrPacketSize = NV_MIN(hOdmSpi->CurrTransInfo.RxPacketsRemaining,
                                (MAX_CPU_TRANSACTION_SIZE_WORD*
                                    hOdmSpi->CurrTransInfo.PacketsPerWord));
        hOdmSpi->CurrTransInfo.CurrPacketCount = CurrPacketSize;
        SpiStartTransfer(hOdmSpi, NV_FALSE);
        SpiSetDmaTransferSize(hOdmSpi, CurrPacketSize);
        return NV_FALSE;
    }

    // All requested transfer is completed.
    return NV_TRUE;
}

static NvError
SpiWaitForTransferComplete(
   NvOdmServicesSpiHandle hOdmSpi,
   NvU32 WaitTimeOutMS)
{
    NvU32 StartTime;
    NvU32 CurrentTime;
    NvU32 TimeElapsed;
    NvBool IsWait = NV_TRUE;
    NvError Error = NvSuccess;
    NvBool IsReady;
    NvBool IsTransferComplete= NV_FALSE;
    NvU32 CurrentPacketTransfer;

    StartTime = NvOsGetTimeMS();
    while (IsWait)
    {
        IsReady = IsSpiTransferComplete(hOdmSpi);
        if (IsReady)
        {
            IsTransferComplete = SpiHandleTransferComplete(hOdmSpi);
            if(IsTransferComplete)
                break;
        }
        if (WaitTimeOutMS != NV_WAIT_INFINITE)
        {
            CurrentTime = NvOsGetTimeMS();
            TimeElapsed = (CurrentTime >= StartTime)? (CurrentTime - StartTime):
                             MAX_TIME_IN_MS - StartTime + CurrentTime;
            IsWait = (TimeElapsed > WaitTimeOutMS)? NV_FALSE: NV_TRUE;
        }
    }
    Error = (IsTransferComplete)? NvError_Success: NvError_Timeout;
    // If timeout happen then stop all transfer and exit.
    if (Error == NvError_Timeout)
    {
        // Data flow direction Unset
        SpiSetDataFlowDirection(hOdmSpi, NV_FALSE);
        hOdmSpi->CurrentDirection = SerialDataFlow_None;

        // Check again whether the transfer is completed or not.
        IsReady = IsSpiTransferComplete(hOdmSpi);
        if (IsReady)
        {
            // All requested transfer has been done.
            CurrentPacketTransfer = hOdmSpi->CurrTransInfo.CurrPacketCount;
            Error = NvSuccess;
        }
        else
        {
            // Get the transfer word count from status register.
            if (hOdmSpi->CurrentDirection & SerialDataFlow_Rx)
            {
                if ((hOdmSpi->CurrTransInfo.PacketsPerWord > 1))
                    CurrentPacketTransfer -=
                       SpiGetTransferdCount(hOdmSpi) * hOdmSpi->CurrTransInfo.PacketsPerWord;
            }
        }
        hOdmSpi->CurrTransInfo.PacketTransferred += CurrentPacketTransfer;
   }
   return Error;
}

static void
SpiSetChipSelectSignalLevel(
   NvOdmServicesSpiHandle hOdmSpi,
   NvBool IsActive)
{
    NvBool IsHigh;
    NvOdmQuerySpiDeviceInfo *pDevInfo =
                 &hOdmSpi->DeviceInfo[hOdmSpi->ChipSelectId];

    if(IsActive)
    {
        if (!hOdmSpi->IsHwChipSelectSupported)
        {
            IsHigh = (pDevInfo->ChipSelectActiveLow)? NV_FALSE: NV_TRUE;
            SpiSetChipSelectLevel(hOdmSpi, hOdmSpi->ChipSelectId, IsHigh);
            hOdmSpi->IsCurrentChipSelStateHigh[hOdmSpi->ChipSelectId] = IsHigh;
            hOdmSpi->IsChipSelConfigured = NV_TRUE;
        }
        else
        {
            hOdmSpi->IsChipSelConfigured = NV_FALSE;
        }
    }
    else
    {
       IsHigh = (pDevInfo->ChipSelectActiveLow)? NV_TRUE: NV_FALSE;
       SpiSetChipSelectLevel(hOdmSpi,hOdmSpi->ChipSelectId, IsHigh);
       if(hOdmSpi->IdleSignalMode != hOdmSpi->CurSignalMode)
           SpiSetSignalMode(hOdmSpi, hOdmSpi->IdleSignalMode);
       hOdmSpi->IsCurrentChipSelStateHigh[hOdmSpi->ChipSelectId] = IsHigh;
       hOdmSpi->IsChipSelConfigured = NV_FALSE;
    }
}

static void
MakeSpiBufferFromClientBuffer(
    const NvU8 *pTxBuffer,
    NvU32 *pSpiBuffer,
    NvU32 BytesRequested,
    NvU32 PacketBitLength,
    NvU32 IsPackedMode)
{
    NvU32 Shift0;
    NvU32 MSBMaskData = 0xFF;
    NvU32 BytesPerPacket;
    NvU32 Index;
    NvU32 PacketRequest;

    if (IsPackedMode)
    {
        if (PacketBitLength == BITS_PER_BYTE)
        {
            NvOsMemcpy(pSpiBuffer, pTxBuffer, BytesRequested);
            return;
        }

        BytesPerPacket = (PacketBitLength + (BITS_PER_BYTE - 1))/BITS_PER_BYTE;
        PacketRequest = BytesRequested / BytesPerPacket;
        if (PacketBitLength == 16)
        {
            NvU16 *pOutBuffer = (NvU16 *)pSpiBuffer;
             for (Index =0; Index < PacketRequest; ++Index)
             {
                 *pOutBuffer++ = (NvU16)(((*(pTxBuffer  )) << 8) |
                                         ((*(pTxBuffer+1))& 0xFF));
                 pTxBuffer += 2;
             }
            return;
        }
    }

    BytesPerPacket = (PacketBitLength + (BITS_PER_BYTE - 1))/BITS_PER_BYTE;
    PacketRequest = BytesRequested / BytesPerPacket;
    Shift0 = (PacketBitLength & 7);
    if (Shift0)
        MSBMaskData = (0xFF >> (8-Shift0));

    if (BytesPerPacket == 1)
    {
        for (Index = 0; Index < PacketRequest; ++Index)
        {
            *pSpiBuffer++ = (NvU32)((*(pTxBuffer))& MSBMaskData);
            pTxBuffer++;
        }
        return;
    }

    if (BytesPerPacket == 2)
    {
        for (Index = 0; Index < PacketRequest; ++Index)
        {
            *pSpiBuffer++ = (NvU32)((((*(pTxBuffer))& MSBMaskData) << 8) |
                                    ((*(pTxBuffer+1))));
            pTxBuffer += 2;
        }
        return;
    }

    if (BytesPerPacket == 3)
    {
        for (Index = 0; Index < PacketRequest; ++Index)
        {
            *pSpiBuffer++ = (NvU32)((((*(pTxBuffer)) & MSBMaskData) << 16) |
                                    ((*(pTxBuffer+1)) << 8) |
                                    ((*(pTxBuffer+2))));
            pTxBuffer += 3;
        }
        return;
    }

    if (BytesPerPacket == 4)
    {
        for (Index = 0; Index < PacketRequest; ++Index)
        {
            *pSpiBuffer++ = (NvU32)((((*(pTxBuffer))& MSBMaskData) << 24) |
                                    ((*(pTxBuffer+1)) << 16) |
                                    ((*(pTxBuffer+2)) << 8) |
                                    ((*(pTxBuffer+3))));
            pTxBuffer += 4;
        }
        return;
    }
}

static void
MakeClientBufferFromSpiBuffer(
    NvU8 *pRxBuffer,
    NvU32 *pSpiBuffer,
    NvU32 BytesRequested,
    NvU32 PacketBitLength,
    NvU32 IsPackedMode)
{
    NvU32 Shift0;
    NvU32 MSBMaskData = 0xFF;
    NvU32 BytesPerPacket;
    NvU32 Index;
    NvU32 RxData;
    NvU32 PacketRequest;

    NvU8 *pOutBuffer = NULL;

    if (IsPackedMode)
    {
        if (PacketBitLength == 8)
        {
            NvOsMemcpy(pRxBuffer, pSpiBuffer, BytesRequested);
            return;
        }

        BytesPerPacket = (PacketBitLength + (BITS_PER_BYTE - 1))/BITS_PER_BYTE;
        PacketRequest = BytesRequested / BytesPerPacket;
        if (PacketBitLength == 16)
        {
            pOutBuffer = (NvU8 *)pSpiBuffer;
            for (Index =0; Index < PacketRequest; ++Index)
            {
                *pRxBuffer++  = (NvU8) (*(pOutBuffer+1));
                *pRxBuffer++  = (NvU8) (*(pOutBuffer));
                pOutBuffer += 2;
            }
            return;
        }
    }

    BytesPerPacket = (PacketBitLength + (BITS_PER_BYTE - 1))/BITS_PER_BYTE;
    PacketRequest = BytesRequested / BytesPerPacket;
    Shift0 = (PacketBitLength & 7);
    if (Shift0)
        MSBMaskData = (0xFF >> (8-Shift0));

    if (BytesPerPacket == 1)
    {
      for (Index = 0; Index < PacketRequest; ++Index)
            *pRxBuffer++ = (NvU8)((*pSpiBuffer++) & MSBMaskData);
        return;
    }

    if (BytesPerPacket == 2)
    {
        for (Index = 0; Index < PacketRequest; ++Index)
        {
            RxData = *pSpiBuffer++;
            *pRxBuffer++ = (NvU8)((RxData >> 8) & MSBMaskData);
            *pRxBuffer++ = (NvU8)((RxData) & 0xFF);
        }
        return;
    }

    if (BytesPerPacket == 3)
    {
        for (Index = 0; Index < PacketRequest; ++Index)
        {
            RxData = *pSpiBuffer++;
            *pRxBuffer++ = (NvU8)((RxData >> 16)& MSBMaskData);
            *pRxBuffer++ = (NvU8)((RxData >> 8)& 0xFF);
            *pRxBuffer++ = (NvU8)((RxData) & 0xFF);
        }
        return;
    }

    if (BytesPerPacket == 4)
    {
        for (Index = 0; Index < PacketRequest; ++Index)
        {
            RxData = *pSpiBuffer++;
            *pRxBuffer++ = (NvU8)((RxData >> 24)& MSBMaskData);
            *pRxBuffer++ = (NvU8)((RxData >> 16)& 0xFF);
            *pRxBuffer++ = (NvU8)((RxData >> 8)& 0xFF);
            *pRxBuffer++ = (NvU8)((RxData) & 0xFF);
        }
        return;
    }
}

static NvError
SpiReadWriteCpu(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU8 *pClientRxBuffer,
    const NvU8 *pClientTxBuffer,
    NvU32 PacketsRequested,
    NvU32 *pPacketsTransferred,
    NvU32 IsPackedMode,
    NvU32 PacketBitLength)
{
    NvError Error = NvSuccess;
    NvU32 CurrentTransWord;
    NvU32 BufferOffset = 0;
    NvU32 WordsWritten;
    NvU32 MaxPacketPerTrans;
    NvU32 CurrentTransPacket;
    NvU32 PacketsPerWord;
    NvU32 MaxPacketTrans;
    NvBool IsPolling;

    hOdmSpi->CurrTransInfo.BytesPerPacket = (PacketBitLength + 7)/8;
    PacketsPerWord = (IsPackedMode)? 4/hOdmSpi->CurrTransInfo.BytesPerPacket:1;

    MaxPacketPerTrans = MAX_CPU_TRANSACTION_SIZE_WORD*PacketsPerWord;
    hOdmSpi->CurrTransInfo.TotalPacketsRemaining = PacketsRequested;
    // Set the transfer
    hOdmSpi->CurrTransInfo.PacketsPerWord = PacketsPerWord;
    hOdmSpi->CurrTransInfo.pRxBuff = NULL;
    hOdmSpi->CurrTransInfo.RxPacketsRemaining = 0;
    hOdmSpi->CurrTransInfo.pTxBuff = NULL;
    hOdmSpi->CurrTransInfo.TxPacketsRemaining = 0;

    while (hOdmSpi->CurrTransInfo.TotalPacketsRemaining)
    {
        MaxPacketTrans = NV_MIN(hOdmSpi->CurrTransInfo.TotalPacketsRemaining,
                            MaxPacketPerTrans);
        CurrentTransWord = (MaxPacketTrans)/PacketsPerWord;

        if (!CurrentTransWord)
        {
            PacketsPerWord = 1;
            CurrentTransWord = MaxPacketTrans;
            SpiSetPacketLength(hOdmSpi, PacketBitLength, NV_FALSE);
            hOdmSpi->CurrTransInfo.PacketsPerWord = PacketsPerWord;
            IsPackedMode = NV_FALSE;
        }

        CurrentTransPacket = NV_MIN(MaxPacketTrans, CurrentTransWord*PacketsPerWord);
        hOdmSpi->RxTransferStatus = NvSuccess;
        hOdmSpi->TxTransferStatus = NvSuccess;
        hOdmSpi->CurrTransInfo.PacketTransferred  = 0;

        // Do Read/Write in Polling mode only
        if (pClientRxBuffer)
        {
            hOdmSpi->CurrTransInfo.pRxBuff = hOdmSpi->pRxCpuBuffer;
            hOdmSpi->CurrTransInfo.RxPacketsRemaining = CurrentTransPacket;
        }
        if (pClientTxBuffer)
        {
            MakeSpiBufferFromClientBuffer(pClientTxBuffer + BufferOffset,
                hOdmSpi->pTxCpuBuffer, CurrentTransPacket*hOdmSpi->CurrTransInfo.BytesPerPacket,
                PacketBitLength, IsPackedMode);
            WordsWritten = SpiWriteInTransmitFifo(hOdmSpi, hOdmSpi->pTxCpuBuffer,
                                                                 CurrentTransWord);
            hOdmSpi->CurrTransInfo.CurrPacketCount =
                      NV_MIN(WordsWritten*PacketsPerWord, CurrentTransPacket);
            hOdmSpi->CurrTransInfo.pTxBuff =
                                    hOdmSpi->pTxCpuBuffer + WordsWritten;
            hOdmSpi->CurrTransInfo.TxPacketsRemaining = CurrentTransPacket -
                                    hOdmSpi->CurrTransInfo.CurrPacketCount;
        }
        else
        {
            hOdmSpi->CurrTransInfo.CurrPacketCount =
              NV_MIN(MAX_CPU_TRANSACTION_SIZE_WORD*PacketsPerWord, CurrentTransPacket);
        }
        if(!hOdmSpi->IsChipSelConfigured)
        {
            SpiSetChipSelectLevelBasedOnPacket(hOdmSpi,
                           hOdmSpi->DeviceInfo[hOdmSpi->ChipSelectId].ChipSelectActiveLow,
                           CurrentTransPacket, PacketsPerWord);
            hOdmSpi->IsChipSelConfigured = NV_TRUE;
        }
        SpiSetDmaTransferSize(hOdmSpi, hOdmSpi->CurrTransInfo.CurrPacketCount);
        // start transfer
        SpiStartTransfer(hOdmSpi, NV_TRUE);

        // Wait for transfer
        Error = SpiWaitForTransferComplete(hOdmSpi, NV_WAIT_INFINITE);

        if (Error)
            break;

        if (pClientRxBuffer)
        {
            MakeClientBufferFromSpiBuffer(pClientRxBuffer + BufferOffset,
                hOdmSpi->pRxCpuBuffer, CurrentTransPacket*hOdmSpi->CurrTransInfo.BytesPerPacket,
                PacketBitLength, IsPackedMode);
        }

        BufferOffset += CurrentTransPacket*hOdmSpi->CurrTransInfo.BytesPerPacket;
        hOdmSpi->CurrTransInfo.TotalPacketsRemaining -= CurrentTransPacket;
    }

    *pPacketsTransferred = PacketsRequested - hOdmSpi->CurrTransInfo.TotalPacketsRemaining;
     return Error;
}

typedef struct NvOdmServicesGpioRec
{
    NvRmDeviceHandle hRmDev;
    NvRmGpioHandle hGpio;
} NvOdmServicesGpio;

typedef struct NvOdmServicesI2cRec
{
    NvRmDeviceHandle hRmDev;
    NvRmI2cHandle hI2c;
    NvOdmI2cPinMap I2cPinMap;
} NvOdmServicesI2c;

typedef struct NvOdmServicesPmuRec
{
    NvRmDeviceHandle hRmDev;
} NvOdmServicesPmu;

NvOdmServicesGpioHandle NvOdmGpioOpen(void)
{
    NV_ASSERT(! "not implemented");
    return NULL;
}

void NvOdmGpioClose(NvOdmServicesGpioHandle hOdmGpio)
{
    NV_ASSERT(! "not implemented");
}

NvOdmGpioPinHandle
NvOdmGpioAcquirePinHandle(NvOdmServicesGpioHandle hOdmGpio,
        NvU32 Port, NvU32 Pin)
{
    NV_ASSERT(! "not implemented");
    return NULL;
}

void
NvOdmGpioReleasePinHandle(NvOdmServicesGpioHandle hOdmGpio,
        NvOdmGpioPinHandle hPin)
{
    NV_ASSERT(! "not implemented");
}

void
NvOdmGpioSetState(
    NvOdmServicesGpioHandle hOdmGpio,
    NvOdmGpioPinHandle hGpioPin,
    NvU32 PinValue)
{
    NV_ASSERT(! "not implemented");
}

void
NvOdmGpioGetState(
    NvOdmServicesGpioHandle hOdmGpio,
    NvOdmGpioPinHandle hGpioPin,
    NvU32 *PinValue)
{
    NV_ASSERT(! "not implemented");
}

void
NvOdmGpioConfig(
    NvOdmServicesGpioHandle hOdmGpio,
    NvOdmGpioPinHandle hGpioPin,
    NvOdmGpioPinMode mode)
{
    NV_ASSERT(! "not implemented");
}

NvBool
NvOdmGpioInterruptRegister(NvOdmServicesGpioHandle hOdmGpio,
    NvOdmServicesGpioIntrHandle *hGpioIntr,
    NvOdmGpioPinHandle hGpioPin,
    NvOdmGpioPinMode Mode,
    NvOdmInterruptHandler Callback,
    void *arg,
    NvU32 DebounceTime)
{
    NV_ASSERT(! "not implemented");
    return NV_FALSE;
}

void
NvOdmGpioInterruptMask(NvOdmServicesGpioIntrHandle handle, NvBool mask)
{
    NV_ASSERT(! "not implemented");
}

void
NvOdmGpioInterruptUnregister(NvOdmServicesGpioHandle hOdmGpio,
    NvOdmGpioPinHandle hGpioPin,
    NvOdmServicesGpioIntrHandle handle)
{
    NV_ASSERT(! "not implemented");
}

void NvOdmGpioInterruptDone( NvOdmServicesGpioIntrHandle handle )
{
    NV_ASSERT(! "not implemented");
}

NvOdmServicesI2cHandle
NvOdmI2cOpen(
    NvOdmIoModule OdmIoModuleId,
    NvU32 instance)
{
    static NvOdmServicesI2c OdmServicesI2c;

    // Initialize. Done only once.
    PrivI2cInit();

    return &OdmServicesI2c;
}

void NvOdmI2cClose(NvOdmServicesI2cHandle hOdmI2c)
{
}

NvOdmServicesI2cHandle
NvOdmI2cPinMuxOpen(
    NvOdmIoModule OdmIoModuleId,
    NvU32 instance,
    NvOdmI2cPinMap PinMap)
{
    NV_ASSERT(! "not implemented");
    return NULL;
}

// Only the following transactions are supported.
//     - Single read; up to 4 bytes.
//     - Single write; up to 4 bytes.
//     - Repeated start Write-Read transactions; up to 4 bytes each.
//     - No-Repeated start Write-Read transactions; up to 4 bytes each.
NvOdmI2cStatus
NvOdmI2cTransaction(
    NvOdmServicesI2cHandle hOdmI2c,
    NvOdmI2cTransactionInfo *TransactionInfo,
    NvU32 NumberOfTransactions,
    NvU32 ClockSpeedKHz,
    NvU32 WaitTimeoutInMilliSeconds)
{
    NvU32 TimeStart;
    NvU32 Addr0;
    NvU32 Addr1;
    NvU32 Config;
    NvU32 Status;
    NvU32 DataTx;
    NvU32 DataRx;
    NvU32 i;

    NvU32 Divisor;
    NvU32 Reg;
    NvU32 OscFreq;
    NvU32 OscFreqKHz;
    NvU32 OscFreq2KHz[] = {13000, 19200, 12000, 26000};

    // Initialize. Done only once.
    PrivI2cInit();

    // Sanity check.
    NV_ASSERT(ClockSpeedKHz > 0);
    NV_ASSERT(NumberOfTransactions > 0);
    NV_ASSERT(NumberOfTransactions <= 2);
    if (NumberOfTransactions == 2)
    {
        NV_ASSERT(TransactionInfo[1].NumBytes < 4);
        NV_ASSERT(! (TransactionInfo[1].Flags & NVODM_I2C_IS_10_BIT_ADDRESS));
        NV_ASSERT(! (TransactionInfo[1].Flags & NVODM_I2C_NO_ACK));
        // Only Write-Read combination is supported.
        NV_ASSERT(TransactionInfo[0].Flags & NVODM_I2C_IS_WRITE);
        NV_ASSERT(! (TransactionInfo[1].Flags & NVODM_I2C_IS_WRITE));
    }

    NV_ASSERT(TransactionInfo[0].NumBytes < 4);
    NV_ASSERT(! (TransactionInfo[0].Flags & NVODM_I2C_IS_10_BIT_ADDRESS));
    NV_ASSERT(! (TransactionInfo[0].Flags & NVODM_I2C_NO_ACK));

    // Split the request if repeated start is not specified.
    if (NumberOfTransactions == 2
        && !(TransactionInfo[0].Flags & NVODM_I2C_USE_REPEATED_START))
    {
        NvOdmI2cStatus SubStatus;
        SubStatus = NvOdmI2cTransaction(hOdmI2c, &TransactionInfo[0], 1,
                                        ClockSpeedKHz,
                                        WaitTimeoutInMilliSeconds);
        if (SubStatus != NvOdmI2cStatus_Success)
            return SubStatus;
        SubStatus = NvOdmI2cTransaction(hOdmI2c, &TransactionInfo[1], 1,
                                        ClockSpeedKHz,
                                        WaitTimeoutInMilliSeconds);
        return SubStatus;
    }

    // Setup divisor.
    NV_CLK_RST_READ(OSC_CTRL, Reg);
    OscFreq = NV_DRF_VAL(CLK_RST_CONTROLLER, OSC_CTRL, OSC_FREQ, Reg);
    OscFreqKHz = OscFreq2KHz[OscFreq];
    Divisor = (OscFreqKHz >> 3) / ClockSpeedKHz;

    NV_CLK_RST_READ(CLK_SOURCE_DVC_I2C, Reg);
    Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_DVC_I2C,
                             DVC_I2C_CLK_DIVISOR, Divisor - 1, Reg);
    NV_CLK_RST_WRITE(CLK_SOURCE_DVC_I2C, Reg);

    // Set parameters for the 1st transaction.
    Config = 0;
    Config = NV_FLD_SET_DRF_NUM(DVC, I2C_CNFG, LENGTH,
                                TransactionInfo[0].NumBytes - 1, Config);

    Addr0 = TransactionInfo[0].Address;
    if (!(TransactionInfo[0].Flags & NVODM_I2C_IS_WRITE))
    {
        Addr0 |= 1; // read address
        Config = NV_FLD_SET_DRF_DEF(DVC, I2C_CNFG, CMD1, ENABLE, Config);
    }
    NV_DVC_WRITE(I2C_CMD_ADDR0, Addr0);

    DataTx = (NvU32)TransactionInfo[0].Buf[0]
             | ((NvU32)TransactionInfo[0].Buf[1] << 8)
             | ((NvU32)TransactionInfo[0].Buf[2] << 16)
             | ((NvU32)TransactionInfo[0].Buf[3] << 24);
    NV_DVC_WRITE(I2C_CMD_DATA1, DataTx);

    // Set parameters for the 2nd transaction.
    if (NumberOfTransactions == 2)
    {
        Addr1 = TransactionInfo[1].Address;
        Config = NV_FLD_SET_DRF_DEF(DVC, I2C_CNFG, SLV2, ENABLE, Config);
        if (!(TransactionInfo[1].Flags & NVODM_I2C_IS_WRITE))
        {
            Addr1 |= 1; // read address
            Config = NV_FLD_SET_DRF_DEF(DVC, I2C_CNFG, CMD2, ENABLE, Config);
        }
        NV_DVC_WRITE(I2C_CMD_ADDR1, Addr1);
    }

    // Start the transaction.
    NV_DVC_WRITE(I2C_CNFG, Config);
    Config = NV_FLD_SET_DRF_DEF(DVC, I2C_CNFG, SEND, GO, Config);
    NV_DVC_WRITE(I2C_CNFG, Config);

    // Wait for the end of transaction.
    TimeStart = GetTimeMS();
    do
    {
        NV_DVC_READ(I2C_STATUS, Status);

        if (WaitTimeoutInMilliSeconds == NV_WAIT_INFINITE)
            continue;

        if (GetTimeMS() > TimeStart + WaitTimeoutInMilliSeconds)
            return NvOdmI2cStatus_Timeout;
    } while (Status & DVC_I2C_STATUS_0_BUSY_FIELD);

    // Check error in the transaction.
    NV_DVC_READ(I2C_STATUS, Status);
    if (Status
        & (DVC_I2C_STATUS_0_CMD1_STAT_FIELD
           | DVC_I2C_STATUS_0_CMD2_STAT_FIELD))
    {
        return NvOdmI2cStatus_SlaveNotFound;
    }

    // Get RX data from transaction 1.
    if (! (TransactionInfo[0].Flags & NVODM_I2C_IS_WRITE))
    {
        NV_DVC_READ(I2C_CMD_DATA1, DataRx);

        for (i = 0; i < TransactionInfo[0].NumBytes; i++)
        {
            TransactionInfo[0].Buf[i] = DataRx >> (i * 8);
        }
    }

    // Get RX data from transaction 2.
    if (NumberOfTransactions == 2)
    {
        if (! (TransactionInfo[1].Flags & NVODM_I2C_IS_WRITE))
        {
            NV_DVC_READ(I2C_CMD_DATA2, DataRx);

            for (i = 0; i < TransactionInfo[1].NumBytes; i++)
            {
                TransactionInfo[1].Buf[i] = DataRx >> (i * 8);
            }
        }
    }

    return NvOdmI2cStatus_Success;
}

NvOdmServicesPmuHandle NvOdmServicesPmuOpen(void)
{
    static NvOdmServicesPmu OdmServicesPmu;

    return &OdmServicesPmu;
}

void NvOdmServicesPmuClose(NvOdmServicesPmuHandle handle)
{
}

void NvOdmServicesPmuGetCapabilities(
        NvOdmServicesPmuHandle handle,
        NvU32 vddId,
        NvOdmServicesPmuVddRailCapabilities *pCapabilities )
{
    NV_ASSERT(! "not implemented");
}

void NvOdmServicesPmuGetVoltage(
        NvOdmServicesPmuHandle handle,
        NvU32 vddId,
        NvU32 * pMilliVolts )
{
    NV_ASSERT(! "not implemented");
}

void NvOdmServicesPmuSetVoltage(
        NvOdmServicesPmuHandle handle,
        NvU32 vddId,
        NvU32 MilliVolts,
        NvU32 * pSettleMicroSeconds )
{
    NV_ASSERT(! "not implemented");
}

void NvOdmServicesPmuSetSocRailPowerState(
        NvOdmServicesPmuHandle handle,
        NvU32 vddId,
        NvBool Enable )
{
    NV_ASSERT(! "not implemented");
}

NvBool
NvOdmServicesPmuGetBatteryStatus(
    NvOdmServicesPmuHandle handle,
    NvOdmServicesPmuBatteryInstance batteryInst,
    NvU8 * pStatus)
{
    NV_ASSERT(! "not implemented");
    return NV_FALSE;
}

NvBool
NvOdmServicesPmuGetBatteryData(
    NvOdmServicesPmuHandle handle,
    NvOdmServicesPmuBatteryInstance batteryInst,
    NvOdmServicesPmuBatteryData * pData)
{
    NV_ASSERT(! "not implemented");
    return NV_FALSE;
}

void
NvOdmServicesPmuGetBatteryFullLifeTime(
    NvOdmServicesPmuHandle handle,
    NvOdmServicesPmuBatteryInstance batteryInst,
    NvU32 * pLifeTime)
{
    NV_ASSERT(! "not implemented");
}

void
NvOdmServicesPmuGetBatteryChemistry(
    NvOdmServicesPmuHandle handle,
    NvOdmServicesPmuBatteryInstance batteryInst,
    NvOdmServicesPmuBatteryChemistry * pChemistry)
{
    NV_ASSERT(! "not implemented");
}

void
NvOdmServicesPmuSetChargingCurrentLimit(
    NvOdmServicesPmuHandle handle,
    NvOdmServicesPmuChargingPath ChargingPath,
    NvU32 ChargingCurrentLimitMa,
    NvOdmUsbChargerType ChargerType)
{
    NV_ASSERT(! "not implemented");
}

void NvOdmEnableOtgCircuitry(NvBool Enable)
{
    NV_ASSERT(! "not implemented");
}

// From nvodm_services_ext.c.
NvOdmServicesKeyListHandle
NvOdmServicesKeyListOpen(void)
{
    return (NvOdmServicesKeyListHandle)0x1;
}

// From nvodm_services_ext.c.
void NvOdmServicesKeyListClose(NvOdmServicesKeyListHandle handle)
{

}

// From nvodm_services_common.c.
NvU32 NvOdmServicesGetKeyValue(
    NvOdmServicesKeyListHandle handle,
    NvU32 Key)
{
    return NV_READ32(PMC_PA_BASE + APBDEV_PMC_SCRATCH20_0);
}

/* Spi Releated odm functionality */

// Create SPI handle in master mode
NvOdmServicesSpiHandle
NvOdmSpiOpen(NvOdmIoModule OdmIoModule,
    NvU32 ControllerId)
{
    NvOdmServicesSpi *hOdmServiceSpi;

    if(s_hOdmSpiHandle[ControllerId] == NULL)
    {
        hOdmServiceSpi = (NvOdmServicesSpi *)
                           NvOsAlloc(sizeof(NvOdmServicesSpi));
        if(hOdmServiceSpi)
        {
            if (PrivSpiInit(hOdmServiceSpi, ControllerId))
            {
                return hOdmServiceSpi;
            }
            else
                return NULL;
        }
        else
            return NULL;
    }
    else
    {
      s_hOdmSpiHandle[ControllerId]->OpenCount++;
      return s_hOdmSpiHandle[ControllerId];
    }
}


void
NvOdmSpiClose(NvOdmServicesSpiHandle hOdmSpi)
{
    if(hOdmSpi)
    {
        hOdmSpi->OpenCount--;
        if(!hOdmSpi->OpenCount)
        {
            // Disable CLK
            SpiClockEnable(hOdmSpi->InstanceId, NV_FALSE);
            s_hOdmSpiHandle[hOdmSpi->InstanceId] = NULL;
            if(hOdmSpi->pTxCpuBuffer)
                NvOsFree(hOdmSpi->pTxCpuBuffer);
            if(hOdmSpi->pRxCpuBuffer)
                NvOsFree(hOdmSpi->pRxCpuBuffer);
            NvOsFree(hOdmSpi);
        }
    }
}

void
NvOdmSpiTransaction(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU32 ChipSelect,
    NvU32 ClockSpeedInKHz,
    NvU8 *ReadBuf,
    const NvU8 *WriteBuf,
    NvU32 BytesRequested,
    NvU32 PacketSizeInBits)
{
    NvError Error = NvSuccess;
    NvBool IsPackedMode;
    NvU32 BytesPerPacket;
    NvU32 PacketsTransferred;
    NvU32 PacketsPerWord;
    NvU32 TotalPacketsRequsted;
    NvU32 TotalWordsRequested;
    NvU32 i;
    NvU32 TotalTransByte = 0;
    NvU8 BytesTransfered;
    NvU8 BytesToTransfer;
    NvU32 CommandReg;

    // Sanity checks
    NV_ASSERT(hOdmSpi);
    NV_ASSERT( ReadBuf || WriteBuf);
    NV_ASSERT(ClockSpeedInKHz > 0);
    NV_ASSERT((PacketSizeInBits > 0) && (PacketSizeInBits <= 32));
    // Chip select should be supported by the odm.
    NV_ASSERT(hOdmSpi->IsChipSelSupported[ChipSelect]);

    // Select Packed mode for the 8/16 bit length.
    BytesPerPacket = (PacketSizeInBits + 7)/8;
    TotalPacketsRequsted = BytesRequested/BytesPerPacket;
    //IsPackedMode = ((PacketSizeInBits == 8) || ((PacketSizeInBits == 16)));
    // Use un-packed mode
    IsPackedMode = NV_FALSE;
    PacketsPerWord =  (IsPackedMode)? 4/BytesPerPacket: 1;
    TotalWordsRequested = (TotalPacketsRequsted + PacketsPerWord -1)/PacketsPerWord;
    NV_ASSERT((BytesRequested % BytesPerPacket) == 0);
    NV_ASSERT(TotalPacketsRequsted);

    // Set current chip select Id
    hOdmSpi->ChipSelectId = ChipSelect;
    if (hOdmSpi->DeviceInfo[ChipSelect].CanUseHwBasedCs)
        hOdmSpi->IsHwChipSelectSupported = NV_TRUE;
    else
        hOdmSpi->IsHwChipSelectSupported = NV_FALSE;
    // set the clock speed
    SpiSetClockSpeed(hOdmSpi, ClockSpeedInKHz);
    // set the chip select
    SpiSetChipSelectSignalLevel(hOdmSpi,NV_TRUE);

    // Set the data flow direction
    hOdmSpi->CurrentDirection = SerialDataFlow_None;
    if (ReadBuf)
        hOdmSpi->CurrentDirection |= SerialDataFlow_Rx;
    if (WriteBuf)
        hOdmSpi->CurrentDirection |= SerialDataFlow_Tx;
    SpiSetDataFlowDirection(hOdmSpi, NV_TRUE);
    // Set the packet length
    SpiSetPacketLength(hOdmSpi,PacketSizeInBits, IsPackedMode);
    // Do CPU transfer for data
    Error = SpiReadWriteCpu(hOdmSpi, ReadBuf, WriteBuf,
        TotalPacketsRequsted, &PacketsTransferred,
        IsPackedMode,PacketSizeInBits);

    if(Error)
       goto cleanup;

cleanup:
    // Data flow direction Unset
    SpiSetDataFlowDirection(hOdmSpi, NV_FALSE);
    hOdmSpi->CurrentDirection = SerialDataFlow_None;
    // Release the chip select
    SpiSetChipSelectSignalLevel(hOdmSpi, NV_FALSE);

}

void
NvOdmSpiSetSignalMode(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU32 ChipSelectId,
    NvOdmQuerySpiSignalMode SpiSignalMode)
{

    NV_ASSERT(hOdmSpi);
    if((hOdmSpi->IsChipSelSupported[ChipSelectId]))
    {
      hOdmSpi->DeviceInfo[ChipSelectId].SignalMode = SpiSignalMode;
    }
}

NvOdmServicesSpiHandle
NvOdmSpiPinMuxOpen(NvOdmIoModule OdmIoModule,
                   NvU32 ControllerId,
                   NvOdmSpiPinMap PinMap)
{
    NV_ASSERT(! "not implemented");
    return NULL;
}

NvBool NvOdmSpiSlaveStartTransaction(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU32 ChipSelectId,
    NvU32 ClockSpeedInKHz,
    NvBool IsReadTransfer,
    NvU8 * pWriteBuffer,
    NvU32 BytesRequested,
    NvU32 PacketSizeInBits )
{
    NV_ASSERT(! "not implemented");
}


NvBool NvOdmSpiSlaveGetTransactionData(
    NvOdmServicesSpiHandle hOdmSpi,
    NvU8 * pReadBuffer,
    NvU32 BytesRequested,
    NvU32 * pBytesTransfererd,
    NvU32 WaitTimeout )
{
    NV_ASSERT(! "not implemented");
}



NvOdmServicesSpiHandle
NvOdmSpiSlaveOpen(NvOdmIoModule OdmIoModule,
                  NvU32 ControllerId)
{
    NV_ASSERT(! "not implemented");
    return NULL;
}

NvBool NvOdmServicesFuseGet(NvOdmServiceFuseType FuseType, NvU32 *Value)
{
    NV_ASSERT(! "not implemented");
    return NV_FALSE;
}
