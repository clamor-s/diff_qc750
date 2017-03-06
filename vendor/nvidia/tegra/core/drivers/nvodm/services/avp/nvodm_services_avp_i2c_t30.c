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
#include "nvrm_hardware_access.h"
#include "t30/artimerus.h"
#include "t30/arslink.h"
#include "t30/arapb_misc.h"
#include "t30/arclk_rst.h"
#include "t30/arapbpm.h"
#include "t30/ari2c.h"

#define PMC_PA_BASE      1879106560
#define NV_ADDRESS_MAP_TMRUS_BASE    1610633232
#define NV_ADDRESS_MAP_PWR_I2C_BASE   1879101440

#define PINMUX_BASE 1879048192
#define CLK_RST_BASE 1610637312

#define GPIO7_BASE  1610667520
#define MASK_CNF_1_6 0x84

#define NV_PWR_I2C_WRITE(reg, value)    \
    NV_WRITE32((NV_ADDRESS_MAP_PWR_I2C_BASE + I2C_##reg##_0), value)

#define NV_PWR_I2C_READ(reg)   \
    NV_READ32((NV_ADDRESS_MAP_PWR_I2C_BASE + I2C_##reg##_0))

#define NV_CLK_RST_READ(reg, value)    \
        value = NV_READ32(CLK_RST_BASE + CLK_RST_CONTROLLER_##reg##_0)

#define NV_CLK_RST_WRITE(reg, value)    \
    NV_WRITE32((CLK_RST_BASE + CLK_RST_CONTROLLER_##reg##_0), value)

static NvU32
GetTimeMS(void)
{
    NvU32 timeUs = NV_READ32(NV_ADDRESS_MAP_TMRUS_BASE + TIMERUS_CNTR_1US_0);
    return timeUs >> 10; // avoid using division
}

static NvU32
GetTimeUS(void)
{
    NvU32 timeUs = NV_READ32(NV_ADDRESS_MAP_TMRUS_BASE + TIMERUS_CNTR_1US_0);
    return timeUs;
}

static void PinmuxInit()
{
    //Enable SCL and SDA
    NV_WRITE32(PINMUX_BASE + PINMUX_AUX_PWR_I2C_SCL_0, 0x60);
    NV_WRITE32(PINMUX_BASE + PINMUX_AUX_PWR_I2C_SDA_0, 0x60);

    NV_WRITE32(GPIO7_BASE + MASK_CNF_1_6, 0x4000);
    NV_WRITE32(GPIO7_BASE + MASK_CNF_1_6, 0x8000);
}

static int initialized = 0;

static void PrivI2cInit(void)
{
    NvU32 Reg;
    NvU32 TimeStart;

    // Avoid running this function more than once.
    if (initialized)
        return;
    initialized = 1;
    PinmuxInit();
   //Enable clock
   NV_CLK_RST_READ(CLK_OUT_ENB_H, Reg);
   Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_OUT_ENB_H,
                             CLK_ENB_DVC_I2C, 1, Reg);
   NV_CLK_RST_WRITE(CLK_OUT_ENB_H, Reg);

   //Reset the controller
   NV_CLK_RST_READ(RST_DEVICES_H, Reg);
   Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_H,
                             SWR_DVC_I2C_RST, 1, Reg);
   NV_CLK_RST_WRITE(RST_DEVICES_H, Reg);
   TimeStart = GetTimeUS();
   while(GetTimeUS() < (TimeStart + 2));
   NV_CLK_RST_READ(RST_DEVICES_H, Reg);
   Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_H,
                             SWR_DVC_I2C_RST, 0, Reg);
   NV_CLK_RST_WRITE(RST_DEVICES_H, Reg);
}

typedef struct NvOdmServicesI2cRec
{
    NvRmDeviceHandle hRmDev;
    NvRmI2cHandle hI2c;
    NvOdmI2cPinMap I2cPinMap;
} NvOdmServicesI2c;


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

/* FIXME: gcc uses glibc call __aeabi_uidiv for division. If glibc build is
 * optimized for armv6, things will not go well. Using simple integer
 * division algo to avoid glibc function replacement
 */
static NvU32 NvIntDiv(NvU32 Dividend, NvU32 Divisor)
{
    NvU32 i = 0, OldVal = 0, NewVal;

    NV_ASSERT(Divisor > 0);

    NewVal = Divisor;
    while((NewVal <= Dividend) && (OldVal < NewVal))
    {
        OldVal = NewVal;
        NewVal += Divisor;
        i++;
    }
    return i;
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

    /* FIXME: Compiler optimize initialized array to use memcpy. If glibc build is optimized
     * for armv6, things will not go well. Using "static const" prevent compiler
     * to use memcpy */
    static const NvU32 OscFreq2KHz[] = {13000, 16800, 0, 0, 19200, 38400, 0, 0, 12000, 48000, 0, 0, 26000, 0, 0, 0};

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
    Divisor = NvIntDiv(OscFreqKHz >> 3, ClockSpeedKHz);

    NV_CLK_RST_READ(CLK_SOURCE_DVC_I2C, Reg);
    Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_DVC_I2C,
                             DVC_I2C_CLK_DIVISOR, Divisor - 1, Reg);
    NV_CLK_RST_WRITE(CLK_SOURCE_DVC_I2C, Reg);

    NV_CLK_RST_READ(CLK_SOURCE_DVC_I2C, Reg);

    // Set parameters for the 1st transaction.
    Config = 0;
    Config = NV_FLD_SET_DRF_NUM(I2C, I2C_CNFG, LENGTH,
                                TransactionInfo[0].NumBytes - 1, Config);

    Addr0 = TransactionInfo[0].Address;
    if (!(TransactionInfo[0].Flags & NVODM_I2C_IS_WRITE))
    {
        Addr0 |= 1; // read address
        Config = NV_FLD_SET_DRF_DEF(I2C, I2C_CNFG, CMD1, ENABLE, Config);
    }
    NV_PWR_I2C_WRITE(I2C_CMD_ADDR0, Addr0);

    Reg = NV_PWR_I2C_READ(I2C_CMD_ADDR0);

    DataTx = (NvU32)TransactionInfo[0].Buf[0]
             | ((NvU32)TransactionInfo[0].Buf[1] << 8)
             | ((NvU32)TransactionInfo[0].Buf[2] << 16)
             | ((NvU32)TransactionInfo[0].Buf[3] << 24);
    NV_PWR_I2C_WRITE(I2C_CMD_DATA1, DataTx);

    // Set parameters for the 2nd transaction.
    if (NumberOfTransactions == 2)
    {
        Addr1 = TransactionInfo[1].Address;
        Config = NV_FLD_SET_DRF_DEF(I2C, I2C_CNFG, SLV2, ENABLE, Config);
        if (!(TransactionInfo[1].Flags & NVODM_I2C_IS_WRITE))
        {
            Addr1 |= 1; // read address
            Config = NV_FLD_SET_DRF_DEF(I2C, I2C_CNFG, CMD2, ENABLE, Config);
        }
        NV_PWR_I2C_WRITE(I2C_CMD_ADDR1, Addr1);
    }
    // Start the transaction.
    NV_PWR_I2C_WRITE(I2C_CNFG, Config);
    Config = NV_FLD_SET_DRF_DEF(I2C, I2C_CNFG, SEND, GO, Config);
    NV_PWR_I2C_WRITE(I2C_CNFG, Config);

    // Wait for the end of transaction.
    TimeStart = GetTimeMS();
    do
    {
        Status = NV_PWR_I2C_READ(I2C_STATUS);

        if (WaitTimeoutInMilliSeconds == NV_WAIT_INFINITE)
            continue;

        if (GetTimeMS() > TimeStart + WaitTimeoutInMilliSeconds)
        {
            initialized = 0;
            PrivI2cInit();
            return NvOdmI2cStatus_Timeout;
        }
    } while (Status & I2C_I2C_STATUS_0_BUSY_FIELD);

    // Check error in the transaction.
    Status = NV_PWR_I2C_READ(I2C_STATUS);
    if (Status
        & (I2C_I2C_STATUS_0_CMD1_STAT_FIELD
           | I2C_I2C_STATUS_0_CMD2_STAT_FIELD))
    {
        initialized = 0;
        PrivI2cInit();
        return NvOdmI2cStatus_SlaveNotFound;
    }

    // Get RX data from transaction 1.
    if (! (TransactionInfo[0].Flags & NVODM_I2C_IS_WRITE))
    {
        DataRx = NV_PWR_I2C_READ(I2C_CMD_DATA1);

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
            DataRx = NV_PWR_I2C_READ(I2C_CMD_DATA2);

            for (i = 0; i < TransactionInfo[1].NumBytes; i++)
            {
                TransactionInfo[1].Buf[i] = DataRx >> (i * 8);
            }
        }
    }
    return NvOdmI2cStatus_Success;
}
