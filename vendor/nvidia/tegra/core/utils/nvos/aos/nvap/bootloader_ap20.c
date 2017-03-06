/*
 * Copyright (c) 2009-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "bootloader_ap20.h"
#include "nvassert.h"
#include "nvodm_query.h"
#include "nvuart.h"

/* This test will ensure that the security keys in the system are not exposed.
 * !!!!!!!!!!PLEASE DO NOT REMOVE THIS!!!!!!!!!!!!
 */
#define KEYSCHED_SECURITY_WAR_BUG_598910    1    // Verify if  key schedule is safe

#if KEYSCHED_SECURITY_WAR_BUG_598910
#include "ap20/arclk_rst.h"
#include "ap20/arapbpm.h"

#define VDE_PA_BASE         0x6001a000  // Base address for arvde.h registers
#define AVP_BSEA_PA_BASE    0x60011000  // Base address for aravp_bsea.h registers

#define ARVDE_BSEV_SECURE_SECURITY_0                    (0x1110)
#define AVPBSEA_SECURE_SECURITY_0                       (0x110)

#define AVPBSEA_SECURE_SECURITY_0_KEY_SCHED_READ_RANGE  1:1

#endif

#pragma arm


//----------------------------------------------------------------------------------------------
// Static Functions
//----------------------------------------------------------------------------------------------
#if KEYSCHED_SECURITY_WAR_BUG_598910
typedef enum
{
    /// Specifies AES Engine "A" (in BSEV).
    AesEngine_A = 0,

    /// Specifies AES Engine "B" (in BSEA).
    AesEngine_B,

    AesEngine_Num,
    AesEngine_Force32 = 0x7FFFFFFF

} AesEngine;

typedef struct
{
    NvU32 EngineClockEnabled;
    NvU32 EngineInReset;
}EngineState;

static EngineState StateofEngine[AesEngine_Num];

static void
AesEngineInit(AesEngine engine)
{
    NvU32 ClockRegValue = 0;
    NvU32 ResetRegValue = 0;
    // Save previous state of the engine and enable clock and reset for the engine

    ClockRegValue = NV_READ32(CLK_RST_PA_BASE + CLK_RST_CONTROLLER_CLK_OUT_ENB_H_0);
    ResetRegValue = NV_READ32(CLK_RST_PA_BASE + CLK_RST_CONTROLLER_RST_DEVICES_H_0);
    switch (engine)
    {
        case AesEngine_A:
            StateofEngine[engine].EngineClockEnabled = NV_DRF_VAL(
                                    CLK_RST_CONTROLLER, CLK_OUT_ENB_H,
                                    CLK_ENB_VDE, ClockRegValue);
            ClockRegValue = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER,
                                                     CLK_OUT_ENB_H,
                                                     CLK_ENB_VDE,
                                                     1,
                                                     ClockRegValue);
            StateofEngine[engine].EngineInReset = NV_DRF_VAL(
                                        CLK_RST_CONTROLLER, RST_DEVICES_H,
                                        SWR_VDE_RST, ResetRegValue);
            ResetRegValue = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER,
                                             RST_DEVICES_H,
                                             SWR_VDE_RST,
                                             0,
                                             ResetRegValue);
            break;
        case AesEngine_B:
            StateofEngine[engine].EngineClockEnabled = NV_DRF_VAL(
                                        CLK_RST_CONTROLLER, CLK_OUT_ENB_H,
                                        CLK_ENB_BSEA, ClockRegValue);
            ClockRegValue = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER,
                                                     CLK_OUT_ENB_H,
                                                     CLK_ENB_BSEA,
                                                     1,
                                                     ClockRegValue);
            StateofEngine[engine].EngineInReset = NV_DRF_VAL(
                                        CLK_RST_CONTROLLER, RST_DEVICES_H,
                                        SWR_BSEA_RST, ResetRegValue);
            ResetRegValue = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER,
                                             RST_DEVICES_H,
                                             SWR_BSEA_RST,
                                             0,
                                             ResetRegValue);
            break;
    }

    NV_WRITE32(CLK_RST_PA_BASE + CLK_RST_CONTROLLER_CLK_OUT_ENB_H_0, ClockRegValue);
    NV_WRITE32(CLK_RST_PA_BASE + CLK_RST_CONTROLLER_RST_DEVICES_H_0, ResetRegValue);

}

static NvBool
IsKeyScheduleLocked(AesEngine engine)
{
    NvU32   Reg = 0;
    NvBool Disabled = NV_TRUE;

    switch(engine)
    {
        case AesEngine_A:
            Reg = NV_READ32(VDE_PA_BASE + ARVDE_BSEV_SECURE_SECURITY_0);
            break;

        case AesEngine_B:
            Reg = NV_READ32(AVP_BSEA_PA_BASE + AVPBSEA_SECURE_SECURITY_0);
            break;

        default:
            break;

    }
    // Here we take advantage of the fact that the field KEY_SCHED_READ
    // is a field in register SECURE_SECURITY for both engines and
    // the offset of field KEY_SCHED_READ in register SECURE_SECURITY
    //is the same in both engines in AP20.
    Reg = NV_DRF_VAL(AVPBSEA, SECURE_SECURITY, KEY_SCHED_READ, Reg);
    if (Reg)
    {
        Disabled = NV_FALSE;
    }

    return Disabled;

}
static void
ResetFullChip(void)
{
    NvU32 Reg;
    // Issue SOC reset
    Reg = NV_READ32(PMC_PA_BASE + APBDEV_PMC_CNTRL_0);
    Reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC, CNTRL, MAIN_RST, ENABLE, Reg);
    NV_WRITE32(PMC_PA_BASE + APBDEV_PMC_CNTRL_0, Reg);

    // Otherwise avp continues booting the OS
    while(1)
    {
    }

}

static void
RestoreAesEngineState(AesEngine engine)
{
    NvU32 ClockRegValue = 0;
    NvU32 ResetRegValue = 0;
    // Save previous state of the engine and enable clock and reset for the engine

    ClockRegValue = NV_READ32(CLK_RST_PA_BASE + CLK_RST_CONTROLLER_CLK_OUT_ENB_H_0);
    ResetRegValue = NV_READ32(CLK_RST_PA_BASE + CLK_RST_CONTROLLER_RST_DEVICES_H_0);
    switch (engine)
    {
        case AesEngine_A:
            ClockRegValue = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER,
                                                     CLK_OUT_ENB_H,
                                                     CLK_ENB_VDE,
                                                     StateofEngine[engine].EngineClockEnabled,
                                                     ClockRegValue);
            ResetRegValue = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER,
                                             RST_DEVICES_H,
                                             SWR_VDE_RST,
                                             StateofEngine[engine].EngineInReset,
                                             ResetRegValue);
            break;
        case AesEngine_B:
            ClockRegValue = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER,
                                                     CLK_OUT_ENB_H,
                                                     CLK_ENB_BSEA,
                                                     StateofEngine[engine].EngineClockEnabled,
                                                     ClockRegValue);
            ResetRegValue = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER,
                                             RST_DEVICES_H,
                                             SWR_BSEA_RST,
                                             StateofEngine[engine].EngineInReset,
                                             ResetRegValue);
            break;
    }

    NV_WRITE32(CLK_RST_PA_BASE + CLK_RST_CONTROLLER_CLK_OUT_ENB_H_0, ClockRegValue);
    NV_WRITE32(CLK_RST_PA_BASE + CLK_RST_CONTROLLER_RST_DEVICES_H_0, ResetRegValue);

}

static void
NvBlAvpTestKeyScheduleSecurity(void)
{
    AesEngine engine;

    for (engine = AesEngine_A; engine < AesEngine_Num; engine++)
    {
        AesEngineInit(engine);
        if (!IsKeyScheduleLocked(engine))
        {
            NV_ASSERT(0);
            ResetFullChip();
        }
        RestoreAesEngineState(engine);
    } // End for
}
#endif

static NvBool NvBlIsValidBootRomSignature(const NvBootInfoTable*  pBootInfo)
{
    // Look at the beginning of IRAM to see if the boot ROM placed a Boot
    // Information Table (BIT) there.
    if (((pBootInfo->BootRomVersion == NVBOOT_VERSION(2, 3))
      || (pBootInfo->BootRomVersion == NVBOOT_VERSION(2, 2))
      || (pBootInfo->BootRomVersion == NVBOOT_VERSION(2, 1)))
    &&   (pBootInfo->DataVersion    == NVBOOT_VERSION(2, 1))
    &&   (pBootInfo->RcmVersion     == NVBOOT_VERSION(2, 1))
    &&   (pBootInfo->BootType       == NvBootType_Cold)
    &&   (pBootInfo->PrimaryDevice  == NvBootDevType_Irom))
    {
        // There is a valid Boot Information Table.
        return NV_TRUE;
    }

    return NV_FALSE;
}

static NvBool NvBlIsChipInitialized( void )
{
    // Pointer to the start of IRAM which is where the Boot Information Table
    // will reside (if it exists at all).
    const NvBootInfoTable*  pBootInfo = (const NvBootInfoTable*)(AP20_BASE_PA_BOOT_INFO);

    // Look at the beginning of IRAM to see if the boot ROM placed a Boot
    // Information Table (BIT) there.
    if (NvBlIsValidBootRomSignature(pBootInfo))
    {
        // There is a valid Boot Information Table.
        // Return status the boot ROM gave us.
        return pBootInfo->DevInitialized;
    }

    return NV_FALSE;
}

static NvBool NvBlIsChipInRecovery( void )
{
    // Pointer to the start of IRAM which is where the Boot Information Table
    // will reside (if it exists at all).
    const NvBootInfoTable*  pBootInfo = (const NvBootInfoTable*)(AP20_BASE_PA_BOOT_INFO);

    // Look at the beginning of IRAM to see if the boot ROM placed a Boot
    // Information Table (BIT) there.
    if (((pBootInfo->BootRomVersion == NVBOOT_VERSION(2, 3))
      || (pBootInfo->BootRomVersion == NVBOOT_VERSION(2, 2))
      || (pBootInfo->BootRomVersion == NVBOOT_VERSION(2, 1)))
    &&   (pBootInfo->DataVersion    == NVBOOT_VERSION(2, 1))
    &&   (pBootInfo->RcmVersion     == NVBOOT_VERSION(2, 1))
    &&   (pBootInfo->BootType       == NvBootType_Recovery)
    &&   (pBootInfo->PrimaryDevice  == NvBootDevType_Irom))
    {
        return NV_TRUE;
    }

    return NV_FALSE;
}

static NvU32  NvBlAvpQueryBootCpuFrequency( void )
{
    NvU32   frequency;

    #if     NVBL_PLL_BYPASS
        // In bypass mode we run at the oscillator frequency.
        frequency = NvBlAvpQueryOscillatorFrequency(ChipId);
    #else
        frequency = 600000;
    #endif

    return frequency;
}

#if NVBL_PLL_BYPASS
static NvU32  NvBlAvpQueryOscillatorFrequency( void )
{
    NvU32               Reg;
    NvU32               Oscillator;
    NvBootClocksOscFreq Freq;

    Reg = NV_CAR_REGR(CLK_RST_PA_BASE, OSC_CTRL);
    Freq = (NvBootClocksOscFreq)NV_DRF_VAL(CLK_RST_CONTROLLER, OSC_CTRL, OSC_FREQ, Reg);

    switch (Freq)
    {
        case NvBootClocksOscFreq_13:
            Oscillator = 13000;
            break;

        case NvBootClocksOscFreq_19_2:
            Oscillator = 19200;
            break;

        case NvBootClocksOscFreq_12:
            Oscillator = 12000;
            break;

        case NvBootClocksOscFreq_26:
            Oscillator = 26000;
            break;

        default:
            NV_ASSERT(0);
            Oscillator = 0;
            break;
    }

    return Oscillator;
}
#endif

static void SetAvpClockToClkM()
{
    NvU32 RegData;
    RegData = NV_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, SYS_STATE, RUN) |
              NV_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY,
                  SWAKEUP_RUN_SOURCE, CLKM);
    NV_CAR_REGW(CLK_RST_PA_BASE, SCLK_BURST_POLICY, RegData);
    NvBlAvpStallUs(3);
}

static void SetAvpClockToPllP()
{
    NvU32 RegData;
    RegData = NV_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, SYS_STATE, RUN) |
              NV_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY,
                  SWAKEUP_RUN_SOURCE, PLLP_OUT4);
    NV_CAR_REGW(CLK_RST_PA_BASE, SCLK_BURST_POLICY, RegData);
}

static void InitPllP(NvBootClocksOscFreq OscFreq)
{
    NvU32   Reg;

    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON,
          (OscFreq == NvBootClocksOscFreq_19_2) ? NVBOOT_CLOCKS_PLLP_CPCON_19_2
                                                : NVBOOT_CLOCKS_PLLP_CPCON_DEFAULT);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_MISC, Reg);

#if defined(NVBL_FPGA_SUPPORT)
    if (NvBlSocIsFpga(ChipId))
    {
        Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, DISABLE)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, DEFAULT)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, DEFAULT)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, DEFAULT);
    }
    else
#endif  // defined(NVBL_FPGA_SUPPORT)
    {
        SetAvpClockToClkM();
        // DIVP/DIVM/DIVN values taken from arclk_rst.h table for fixed 432 MHz operation and
        // divivied by 2 for 216 MHz operation.
        switch (OscFreq)
        {
            case NvBootClocksOscFreq_13:
                #if NVBL_PLL_BYPASS
                    Reg = 0x80000D0D;
                #else
                Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, ENABLE)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x1)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, NVBL_PLLP_KHZ/500)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, 0xD);
                #endif
                break;

            case NvBootClocksOscFreq_19_2:
                Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, ENABLE)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x1)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, NVBL_PLLP_KHZ/2400)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, 0x4);
                break;

            case NvBootClocksOscFreq_12:
                #if NVBL_PLL_BYPASS
                    Reg = 0x80000C0C;
                #else
                Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, ENABLE)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x1)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, NVBL_PLLP_KHZ/500)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, 0x0C);
                #endif
                break;

            case NvBootClocksOscFreq_26:
                #if NVBL_PLL_BYPASS
                    Reg = 0x80001A1A;
                #else
                Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, ENABLE)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x1)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, NVBL_PLLP_KHZ/500)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, 0x1A);
                #endif
                break;

            default:
                NV_ASSERT(0);
        }
    }

    NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_BASE, Reg);

    Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, ENABLE, Reg);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_BASE, Reg);

    NvBlAvpStallUs(301);

#if !NVBL_PLL_BYPASS
    Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, DISABLE, Reg);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_BASE, Reg);

    // 0x600060A4 = 0x10031C03; CLK_RST_CONTROLLER_PLLP_OUTA_0
    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_RATIO, CLK_DIVIDER(NVBL_PLLP_KHZ,48000))
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_OVRRIDE, DISABLE)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_CLKEN, ENABLE)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_RSTN, RESET_DISABLE)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_RATIO, CLK_DIVIDER(NVBL_PLLP_KHZ,28800))
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_OVRRIDE, DISABLE)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_CLKEN, ENABLE)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_RSTN, RESET_DISABLE);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_OUTA, Reg);

    // 0x600060A8 = 0x06030A03; CLK_RST_CONTROLLER_PLLP_OUTB_0
    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_RATIO, CLK_DIVIDER(NVBL_PLLP_KHZ,108000))
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_OVRRIDE, DISABLE)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_CLKEN, ENABLE)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_RSTN, RESET_DISABLE)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_RATIO, CLK_DIVIDER(NVBL_PLLP_KHZ,72000))
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_OVRRIDE, DISABLE)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_CLKEN, ENABLE)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_RSTN, RESET_DISABLE);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_OUTB, Reg);
#endif
}


//----------------------------------------------------------------------------------------------
static void InitPllU(NvBootClocksOscFreq OscFreq)
{
    NvU32   Reg;
    NvU32   isEnabled;
    NvU32   isBypassed;

    Reg = NV_CAR_REGR(CLK_RST_PA_BASE, PLLU_BASE);
    isEnabled = NV_DRF_VAL(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_ENABLE, Reg);
    isBypassed = NV_DRF_VAL(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_BYPASS, Reg);

    if (isEnabled && !isBypassed )
    {
        return;
    }

    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_MISC, PLLU_PTS, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_MISC, PLLU_LOCK_ENABLE, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_MISC, PLLU_CPCON, 0x5)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_MISC, PLLU_LFCON, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_MISC, PLLU_VCOCON, 0x0);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLU_MISC, Reg);

#if defined(NVBL_FPGA_SUPPORT)
    if (NvBlSocIsFpga(ChipId))
    {
        Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_BYPASS, ENABLE)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_ENABLE, DISABLE)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_REF_DIS, REF_ENABLE)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_LOCK, 0x0)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_OVERRIDE, DEFAULT)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_ICUSB, DEFAULT)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_HSIC, DEFAULT)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_USB, DEFAULT)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_VCO_FREQ, 1)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVN, DEFAULT)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVM, DEFAULT);
    }
    else
#endif  // defined(NVBL_FPGA_SUPPORT)
    {
        // DIVP/DIVM/DIVN values taken from arclk_rst.h table
        switch (OscFreq)
        {
            case NvBootClocksOscFreq_13:
                #if NVBL_PLL_BYPASS
                    Reg = 0x80000D0D;
                #else
                Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_BYPASS, ENABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_ENABLE, DISABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_REF_DIS, REF_ENABLE)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_LOCK, 0x0)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_OVERRIDE, DEFAULT)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_ICUSB, DEFAULT)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_HSIC, DEFAULT)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_USB, DEFAULT)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_VCO_FREQ, 1)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVN, 0x1E0)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVM, 0xD);
                #endif
                break;

            case NvBootClocksOscFreq_19_2:
                Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_BYPASS, ENABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_ENABLE, DISABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_REF_DIS, REF_ENABLE)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_LOCK, 0x0)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_OVERRIDE, DEFAULT)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_ICUSB, DEFAULT)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_HSIC, DEFAULT)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_USB, DEFAULT)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_VCO_FREQ, 0)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVN, 0x64)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVM, 0x4);
                break;

            case NvBootClocksOscFreq_12:
                #if NVBL_PLL_BYPASS
                    Reg = 0x80000C0C;
                #else
                Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_BYPASS, ENABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_ENABLE, DISABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_REF_DIS, REF_ENABLE)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_LOCK, 0x0)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_OVERRIDE, DEFAULT)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_ICUSB, DEFAULT)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_HSIC, DEFAULT)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_USB, DEFAULT)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_VCO_FREQ, 1)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVN, 0x1E0)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVM, 0xC);
                #endif
                break;

            case NvBootClocksOscFreq_26:
                #if NVBL_PLL_BYPASS
                    Reg = 0x80001A1A;
                #else
                Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_BYPASS, ENABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_ENABLE, DISABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_REF_DIS, REF_ENABLE)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_LOCK, 0x0)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_OVERRIDE, DEFAULT)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_ICUSB, DEFAULT)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_HSIC, DEFAULT)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_USB, DEFAULT)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_VCO_FREQ, 1)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVN, 0x1E0)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVM, 0x1A);
                #endif
                break;

            default:
                NV_ASSERT(0);
        }
    }

    NV_CAR_REGW(CLK_RST_PA_BASE, PLLU_BASE, Reg);

    Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_ENABLE, ENABLE, Reg);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLU_BASE, Reg);

#if !NVBL_PLL_BYPASS
    Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_BYPASS, DISABLE, Reg);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLU_BASE, Reg);
#endif
}

static void InitPllX(void)
{
    NvU32               Reg;        // Scratch
    NvU32               Divm;       // Reference
    NvU32               Divn;       // Multiplier
    NvU32               Divp;       // Divider == Divp ^ 2
    NvBootClocksOscFreq OscFreq;    // Oscillator frequency

    // Is PLL-X already running?
    Reg = NV_CAR_REGR(CLK_RST_PA_BASE, PLLX_BASE);
    Reg = NV_DRF_VAL(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, Reg);
    if (Reg == NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, ENABLE))
    {
        return;
    }

    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_MISC, PLLX_CPCON, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_MISC, PLLX_LFCON, 0);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_MISC, Reg);

    {
        Divm = 1;
        Divp = 0;
        Divn = NvBlAvpQueryBootCpuFrequency() / 1000;

        // Operating below the 50% point of the divider's range?
        if (Divn <= (NV_DRF_VAL(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVN, ~0)/2))
        {
            // Yes, double the post divider and the feedback divider.
            Divp = 1;
            Divn <<= Divp;
        }
        // Operating above the range of the feedback divider?
        else if (Divn > NV_DRF_VAL(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVN, ~0))
        {
            // Yes, double the input divider and halve the feedback divider.
            Divn >>= 1;
            Divm = 2;
        }

        // Get the oscillator frequency.
        OscFreq  = ((NvBootInfoTable*)(AP20_BASE_PA_BOOT_INFO))->OscFrequency;

        // Program PLL-X.
        switch (OscFreq)
        {
            case NvBootClocksOscFreq_13:
                #if NVBL_PLL_BYPASS
                    Reg = 0x80000D0D;
                #else
                Divm = (Divm == 1) ? 13 : (13 / Divm);
                Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_BYPASS, ENABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, DISABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_REF_DIS, REF_ENABLE)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_LOCK, 0x0)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVP, Divp)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVN, Divn)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVM, Divm);
                #endif
                break;

            case NvBootClocksOscFreq_19_2:
                // NOTE: With a 19.2 MHz oscillator, the PLL will run 1.05% faster
                //       than the target frequency.
                #if NVBL_PLL_BYPASS
                    Reg = 0x80001313;
                #else
                Divm = (Divm == 1) ? 19 : (19 / Divm);
                Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_BYPASS, ENABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, DISABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_REF_DIS, REF_ENABLE)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_LOCK, 0x0)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVP, Divp)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVN, Divn)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVM, Divm);
                #endif
                break;

            case NvBootClocksOscFreq_12:
                #if NVBL_PLL_BYPASS
                    Reg = 0x80000C0C;
                #else
                Divm = (Divm == 1) ? 12 : (12 / Divm);
                Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_BYPASS, ENABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, DISABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_REF_DIS, REF_ENABLE)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_LOCK, 0x0)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVP, Divp)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVN, Divn)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVM, Divm);
                #endif
                break;

            case NvBootClocksOscFreq_26:
                #if NVBL_PLL_BYPASS
                    Reg = 0x80001A1A;
                #else
                Divm = (Divm == 1) ? 26 : (26 / Divm);
                Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_BYPASS, ENABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, DISABLE)
                    | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_REF_DIS, REF_ENABLE)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_LOCK, 0x0)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVP, Divp)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVN, Divn)
                    | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVM, Divm);
                #endif
                break;

            default:
                NV_ASSERT(0);
        }
    }

    NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_BASE, Reg);

    Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, ENABLE, Reg);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_BASE, Reg);

#if !NVBL_PLL_BYPASS
    Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_BYPASS, DISABLE, Reg);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_BASE, Reg);
#endif
}

static void NvBlClockInit(NvBool IsChipInitialized)
{
    const NvBootInfoTable*  pBootInfo;      // Boot Information Table pointer
    NvBootClocksOscFreq     OscFreq;        // Oscillator frequency
    NvU32                   Reg;            // Temporary register
    NvU32                   OscCtrl;        // Oscillator control register
    NvU32                   OscStrength;    // Oscillator Drive Strength
    NvU32                   UsecCfg;        // Usec timer configuration register

    // Get a pointer to the Boot Information Table.
    pBootInfo = (const NvBootInfoTable*)(AP20_BASE_PA_BOOT_INFO);

    // Get the oscillator frequency.
    OscFreq = pBootInfo->OscFrequency;

    // Set up the oscillator dependent registers.
    // NOTE: Don't try to replace this switch statement with an array lookup.
    //       Can't use global arrays here because the data segment isn't valid yet.
    switch (OscFreq)
    {
        default:
            // Fall Through -- this is what the boot ROM does.
        case NvBootClocksOscFreq_13:
            UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (1-1)) | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (13-1));
            break;

        case NvBootClocksOscFreq_19_2:
            UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (5-1)) | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (96-1));
            break;

        case NvBootClocksOscFreq_12:
            UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (1-1)) | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (12-1));
            break;

        case NvBootClocksOscFreq_26:
            UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (1-1)) | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (26-1));
            break;

#if defined(NVBL_FPGA_SUPPORT)
        case NvBootClocksOscFreq_8_6:
            UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (3-1)) | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (26-1));
            break;

        case NvBootClocksOscFreq_17_3:
            UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (3-1)) | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (52-1));
            break;
#endif
    }

    //-------------------------------------------------------------------------
    // Reconfigure PLLP.
    //-------------------------------------------------------------------------

    if (!IsChipInitialized)
    {
        // Program the microsecond scaler.
        NV_TIMERUS_REGW(TIMERUS_PA_BASE, USEC_CFG, UsecCfg);

        // Program the oscillator control register.
        OscCtrl = NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, OSC_FREQ, (int)OscFreq)
                | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, PLL_REF_DIV, 0)
                | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, OSCFI_SPARE, 0)
                | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XODS, 0)
                | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XOBP, 0)
                | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XOE, 1);
        NV_CAR_REGW(CLK_RST_PA_BASE, OSC_CTRL, OscCtrl);
    }

    // Change the drive strength of oscillator here.
    // Get Oscillator Drive Strength Setting
    OscStrength = NvOdmQueryGetOscillatorDriveStrength();
    OscCtrl = NV_CAR_REGR(CLK_RST_PA_BASE, OSC_CTRL);
    OscCtrl = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XOFS, OscStrength,
                                                     OscCtrl);
    NV_CAR_REGW(CLK_RST_PA_BASE, OSC_CTRL, OscCtrl);


    // Initialize PLL-P.
    InitPllP(OscFreq);

    // Wait until stable
    NvBlAvpStallUs(NVBOOT_CLOCKS_PLL_STABILIZATION_DELAY);

    SetAvpClockToPllP();

    if (!IsChipInitialized)
    {
        Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLM_OUT, PLLM_OUT1_RATIO, 0x2)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLM_OUT, PLLM_OUT1_CLKEN, ENABLE)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLM_OUT, PLLM_OUT1_RSTN, RESET_DISABLE);
        NV_CAR_REGW(CLK_RST_PA_BASE, PLLM_OUT, Reg);
    }

    //-------------------------------------------------------------------------
    // Switch system clock to PLLP_out 4 (108 MHz) MHz, AVP will now run at 108 MHz.
    // This is glitch free as only the source is changed, no special precaution needed.
    //-------------------------------------------------------------------------

    //*((volatile long *)0x60006028) = 0x20002222 ;// SCLK_BURST_POLICY
    Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, SYS_STATE, RUN)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, COP_AUTO_SWAKEUP_FROM_FIQ, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, CPU_AUTO_SWAKEUP_FROM_FIQ, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, COP_AUTO_SWAKEUP_FROM_IRQ, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, CPU_AUTO_SWAKEUP_FROM_IRQ, 0x0)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, SWAKEUP_FIQ_SOURCE, PLLP_OUT4)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, SWAKEUP_IRQ_SOURCE, PLLP_OUT4)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, SWAKEUP_RUN_SOURCE, PLLP_OUT4)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, SWAKEUP_IDLE_SOURCE, PLLP_OUT4);
    NV_CAR_REGW(CLK_RST_PA_BASE, SCLK_BURST_POLICY, Reg);

    //*((volatile long *)0x6000602C) = 0x80000000 ;// SUPER_SCLK_DIVIDER
    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_ENB, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIS_FROM_COP_FIQ, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIS_FROM_CPU_FIQ, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIS_FROM_COP_IRQ, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIS_FROM_CPU_IRQ, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIVIDEND, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIVISOR, 0x0);
    NV_CAR_REGW(CLK_RST_PA_BASE, SUPER_SCLK_DIVIDER, Reg);

    // Initialize PLL-X.
    InitPllX();

    //-------------------------------------------------------------------------
    // Set up the rest of the PLLs.
    //-------------------------------------------------------------------------

    // Initialize PLL-U.
    InitPllU(OscFreq);

    // Give PLLs time to stabilize.
    NvBlAvpStallUs(NVBOOT_CLOCKS_PLL_STABILIZATION_DELAY);

    //*((volatile long *)0x60006020) = 0x20001111 ;// CCLK_BURST_POLICY
    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CPU_STATE, 0x2)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, COP_AUTO_CWAKEUP_FROM_FIQ, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CPU_AUTO_CWAKEUP_FROM_FIQ, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, COP_AUTO_CWAKEUP_FROM_IRQ, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CPU_AUTO_CWAKEUP_FROM_IRQ, 0x0)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_FIQ_SOURCE, PLLX_OUT0)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_IRQ_SOURCE, PLLX_OUT0)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_RUN_SOURCE, PLLX_OUT0)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_IDLE_SOURCE, PLLX_OUT0);
    NV_CAR_REGW(CLK_RST_PA_BASE, CCLK_BURST_POLICY, Reg);

    //*((volatile long *)0x60006024) = 0x80000000 ;// SUPER_CCLK_DIVIDER
    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_ENB, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_COP_FIQ, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_CPU_FIQ, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_COP_IRQ, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_CPU_IRQ, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIVIDEND, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIVISOR, 0x0);
    NV_CAR_REGW(CLK_RST_PA_BASE, SUPER_CCLK_DIVIDER, Reg);

    //*((volatile long *)0x60006030) = 0x00000010 ;// CLK_SYSTEM_RATE
    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, HCLK_DIS, 0x0)
#if NVBL_PLL_BYPASS
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, AHB_RATE, 0x0)
#else
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, AHB_RATE, 0x1)
#endif
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, PCLK_DIS, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, APB_RATE, 0x0);
    NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SYSTEM_RATE, Reg);

    //-------------------------------------------------------------------------
    // If the boot ROM hasn't already enabled the clocks to the memory
    // controller we have to do it here.
    //-------------------------------------------------------------------------

    if (!IsChipInitialized)
    {
        //*((volatile long *)0x6000619C) = 0x03000000 ;//  CLK_SOURCE_EMC
        Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_EMC, EMC_2X_CLK_SRC, 0x0)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_EMC, EMC_2X_CLK_ENB, 0x1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_EMC, EMC_1X_CLK_ENB, 0x1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_EMC, EMC_2X_CLK_DIVISOR, 0x0);
        NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_EMC, Reg);
    }

    //-------------------------------------------------------------------------
    // Enable clocks to required peripherals. Also disable other module clocks
    // which are not required for booting and which got enabled by Power on Reset.
    //-------------------------------------------------------------------------
    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_L_SET, SET_CLK_ENB_CACHE2, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_L_SET, SET_CLK_ENB_USBD, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_L_SET, SET_CLK_ENB_I2C1, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_L_SET, SET_CLK_ENB_GPIO, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_L_SET, SET_CLK_ENB_TMR, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_L_SET, SET_CLK_ENB_RTC, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_L_SET, SET_CLK_ENB_CPU, 0x1);
    NV_CAR_REGW(CLK_RST_PA_BASE, CLK_ENB_L_SET, Reg);

    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_H_SET, SET_CLK_ENB_EMC, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_H_SET, SET_CLK_ENB_FUSE, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_H_SET, SET_CLK_ENB_PMC, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_H_SET, SET_CLK_ENB_STAT_MON, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_H_SET, SET_CLK_ENB_APBDMA, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_H_SET, SET_CLK_ENB_MEM, 0x1);
    NV_CAR_REGW(CLK_RST_PA_BASE, CLK_ENB_H_SET, Reg);

    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_H_CLR, CLR_CLK_ENB_NOR, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_H_CLR, CLR_CLK_ENB_SNOR, 0x0);
    NV_CAR_REGW(CLK_RST_PA_BASE, CLK_ENB_H_CLR, Reg);

    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_U_SET, SET_CLK_M_DOUBLER_ENB, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_U_SET, SET_CLK_ENB_CRAM2, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_U_SET, SET_CLK_ENB_IRAMD, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_U_SET, SET_CLK_ENB_IRAMC, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_U_SET, SET_CLK_ENB_IRAMB, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_U_SET, SET_CLK_ENB_IRAMA, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_U_SET, SET_CLK_ENB_AVPUCQ, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_U_SET, SET_CLK_ENB_CSITE, 0x1);
    NV_CAR_REGW(CLK_RST_PA_BASE, CLK_ENB_U_SET, Reg);

    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_U_CLR, CLR_SYNC_CLK_DOUBLER_ENB, 0x0);
    NV_CAR_REGW(CLK_RST_PA_BASE, CLK_ENB_U_CLR, Reg);

    // Give clocks time to stabilize.
    NvBlAvpStallMs(1);

    // Take requried peripherals out of reset . Also reset  other modules
    // which are not required for booting and which got enabled by Power on Reset.
    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_L_CLR, CLR_CACHE2_RST, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_L_CLR, CLR_USBD_RST, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_L_CLR, CLR_I2C1_RST, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_L_CLR, CLR_GPIO_RST, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_L_CLR, CLR_TMR_RST, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_L_CLR, CLR_RTC_RST, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_L_CLR, CLR_CPU_RST, 0x0);
    NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEV_L_CLR, Reg);

    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_H_CLR, CLR_EMC_RST, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_H_CLR, CLR_FUSE_RST, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_H_CLR, CLR_PMC_RST, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_H_CLR, CLR_PMC_RST, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_H_CLR, CLR_MEM_RST, 0x0);
    NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEV_H_CLR, Reg);

    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_H_SET, SET_NOR_RST, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_H_SET, SET_SNOR_RST, 0x1);
    NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEV_H_SET, Reg);

    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_U_CLR, CLR_AVPUCQ_RST, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_U_CLR, CLR_CSITE_RST, 0x0);
    NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEV_U_SET, Reg);
}

static void NvBlMemoryControllerInit(NvBool IsChipInitialized)
{
    NvU32   Reg;                // Temporary register
    NvBool  IsSdramInitialized; // Nonzero if SDRAM already initialized

    // Get a pointer to the Boot Information Table.
    const NvBootInfoTable*  pBootInfo = (const NvBootInfoTable*)(AP20_BASE_PA_BOOT_INFO);

    // Get SDRAM initialization state.
    IsSdramInitialized = pBootInfo->SdramInitialized;

    //-------------------------------------------------------------------------
    // EMC/MC (Memory Controller) Initialization
    //-------------------------------------------------------------------------

    // Has the boot from initialized the memory controllers?
    if (!IsSdramInitialized)
    {
#if defined(NVBL_FPGA_SUPPORT)
        // Is this an FPGA?
        if (NvBlSocIsFpga(ChipId))
        {
            // *((volatile long *)0x7000F40C) = 0x0300FF00; // EMC_CFG_0
            Reg = NV_DRF_DEF(EMC, CFG, PRE_IDLE_EN, DISABLED)
                | NV_DRF_NUM(EMC, CFG, PRE_IDLE_CYCLES, 0xFF)
                | NV_DRF_DEF(EMC, CFG, CLEAR_AP_PREV_SPREQ, DISABLED)
                | NV_DRF_DEF(EMC, CFG, AUTO_PRE_RD, ENABLED)
                | NV_DRF_DEF(EMC, CFG, AUTO_PRE_WR, ENABLED)
                | NV_DRF_DEF(EMC, CFG, DRAM_ACPD, NO_POWERDOWN)
                | NV_DRF_DEF(EMC, CFG, DRAM_CLKSTOP_PDSR_ONLY, DISABLED)
                | NV_DRF_DEF(EMC, CFG, DRAM_CLKSTOP, DISABLED);
            NV_REG_CHECK(Reg, 0x0300FF00);
            NV_EMC_REGW(EMC_PA_BASE, CFG, Reg);

            // *((volatile long *)0x7000F6B8) = 0x00000000; // EMC_CFG_2_0
            Reg = NV_DRF_DEF(EMC, CFG_2, CLKCHANGE_REQ_ENABLE, DISABLED)
                | NV_DRF_DEF(EMC, CFG_2, CLKCHANGE_PD_ENABLE, DISABLED)
                | NV_DRF_DEF(EMC, CFG_2, PIN_CONFIG, LPDDR2);
            NV_REG_CHECK(Reg, 0x00000000);
            NV_EMC_REGW(EMC_PA_BASE, CFG_2, Reg);

            // *((volatile long *)0x7000F408) = 0x01000410; // EMC_DBG_0
            Reg = NV_DRF_DEF(EMC, DBG, READ_MUX, ACTIVE)
                | NV_DRF_DEF(EMC, DBG, WRITE_MUX, ASSEMBLY )
                | NV_DRF_DEF(EMC, DBG, FORCE_UPDATE, DISABLED)
                | NV_DRF_DEF(EMC, DBG, MRS_WAIT, MRS_256)
                | NV_DRF_DEF(EMC, DBG, PERIODIC_QRST, DISABLED)
                | NV_DRF_DEF(EMC, DBG, READ_DQM_CTRL, MANAGED)
                | NV_DRF_DEF(EMC, DBG, AP_REQ_BUSY_CTRL, ENABLED)
                | NV_DRF_DEF(EMC, DBG, CFG_PRIORITY, ENABLED);
            NV_REG_CHECK(Reg, 0x01000410);
            NV_EMC_REGW(EMC_PA_BASE, DBG, Reg);

            // *((volatile long *)0x7000F504) = 0x00000001; // EMC_FBIO_CFG5_0
            Reg = NV_DRF_DEF(EMC, FBIO_CFG5, DRAM_TYPE, DDR1)
                | NV_DRF_DEF(EMC, FBIO_CFG5, DRAM_WIDTH, X32)
                | NV_DRF_DEF(EMC, FBIO_CFG5, DIFFERENTIAL_DQS, DISABLED)
                | NV_DRF_DEF(EMC, FBIO_CFG5, CTT_TERMINATION, DISABLED)
                | NV_DRF_DEF(EMC, FBIO_CFG5, DQS_PULLD, DISABLED);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, FBIO_CFG5, Reg);

            // *((volatile long *)0x7000F42C) = 0x00000001; // EMC_RC_0
            Reg = NV_DRF_NUM(EMC, RC, RC, 0x1);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, RC, Reg);

            // *((volatile long *)0x7000F430) = 0x00000002; // EMC_RFC_0
            Reg = NV_DRF_NUM(EMC, RFC, RFC, 0x2);
            NV_REG_CHECK(Reg, 0x00000002);
            NV_EMC_REGW(EMC_PA_BASE, RFC, Reg);

            // *((volatile long *)0x7000F434) = 0x00000001; // EMC_RAS_0
            Reg = NV_DRF_NUM(EMC, RAS, RAS, 0x1);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, RAS, Reg);

            // *((volatile long *)0x7000F438) = 0x00000001; // EMC_RP_0
            Reg = NV_DRF_NUM(EMC, RP, RP, 0x1);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, RP, Reg);

            // *((volatile long *)0x7000F43C) = 0x00000004; // EMC_R2W_0
            Reg = NV_DRF_NUM(EMC, R2W, R2W, 0x4);
            NV_REG_CHECK(Reg, 0x00000004);
            NV_EMC_REGW(EMC_PA_BASE, R2W, Reg);

            // *((volatile long *)0x7000F440) = 0x00000003; // EMC_W2R_0
            Reg = NV_DRF_NUM(EMC, W2R, W2R, 0x3);
            NV_REG_CHECK(Reg, 0x00000003);
            NV_EMC_REGW(EMC_PA_BASE, W2R, Reg);

            // *((volatile long *)0x7000F444) = 0x00000001; // EMC_R2P_0
            Reg = NV_DRF_NUM(EMC, R2P, R2P, 0x1);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, R2P, Reg);

            // *((volatile long *)0x7000F448) = 0x00000003; // EMC_W2P_0
            Reg = NV_DRF_NUM(EMC, W2P, W2P, 0x3);
            NV_REG_CHECK(Reg, 0x00000003);
            NV_EMC_REGW(EMC_PA_BASE, W2P, Reg);

            // *((volatile long *)0x7000F44C) = 0x00000001; // EMC_RD_RCD_0
            Reg = NV_DRF_NUM(EMC, RD_RCD, RD_RCD, 0x1);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, RD_RCD, Reg);

            // *((volatile long *)0x7000F450) = 0x00000001; // EMC_WR_RCD_0
            Reg = NV_DRF_NUM(EMC, WR_RCD, WR_RCD, 0x1);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, WR_RCD, Reg);

            // *((volatile long *)0x7000F454) = 0x00000001; // EMC_RRD_0
            Reg = NV_DRF_NUM(EMC, RRD, RRD, 0x1);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, RRD, Reg);

            // *((volatile long *)0x7000F458) = 0x00000001; // EMC_REXT_0
            Reg = NV_DRF_NUM(EMC, REXT, REXT, 0x1);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, REXT, Reg);

            // *((volatile long *)0x7000F45C) = 0x00000000; // EMC_WDV_0
            Reg = NV_DRF_NUM(EMC, WDV, WDV, 0x0);
            NV_REG_CHECK(Reg, 0x00000000);
            NV_EMC_REGW(EMC_PA_BASE, WDV, Reg);

            // *((volatile long *)0x7000F460) = 0x00000001; // EMC_QUSE_0
            Reg = NV_DRF_NUM(EMC, QUSE, QUSE, 0x1);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, QUSE, Reg);

            // *((volatile long *)0x7000F464) = 0x00000000; // EMC_QRST_0
            Reg = NV_DRF_NUM(EMC, QRST, QRST, 0x0);
            NV_REG_CHECK(Reg, 0x00000000);
            NV_EMC_REGW(EMC_PA_BASE, QRST, Reg);

            // *((volatile long *)0x7000F468) = 0x00000007; // EMC_QSAFE_0
            Reg = NV_DRF_NUM(EMC, QSAFE, QSAFE, 0x7);
            NV_REG_CHECK(Reg, 0x00000007);
            NV_EMC_REGW(EMC_PA_BASE, QSAFE, Reg);

            // *((volatile long *)0x7000F46C) = 0x00000007; // EMC_RDV_0
            Reg = NV_DRF_NUM(EMC, RDV, RDV, 0x7);
            NV_REG_CHECK(Reg, 0x00000007);
            NV_EMC_REGW(EMC_PA_BASE, RDV, Reg);

            // *((volatile long *)0x7000F470) = 0x0000003F; // EMC_REFRESH_0
            Reg = NV_DRF_NUM(EMC, REFRESH, REFRESH_LO, 0x1F)
                | NV_DRF_NUM(EMC, REFRESH, REFRESH, 0x1);
            NV_REG_CHECK(Reg, 0x0000003F);
            NV_EMC_REGW(EMC_PA_BASE, REFRESH, Reg);

            //*((volatile long *)0x7000F474) = 0x00000000 ; //   EMC_BURST_REFRESH_NUM_0
            Reg = NV_DRF_DEF(EMC, BURST_REFRESH_NUM, BURST_REFRESH_NUM, BR1);
            NV_REG_CHECK(Reg, 0x00000000);
            NV_EMC_REGW(EMC_PA_BASE, BURST_REFRESH_NUM, Reg);

            // *((volatile long *)0x7000F478) = 0x00000002; // EMC_PDEX2WR_0
            Reg = NV_DRF_NUM(EMC, PDEX2WR, PDEX2WR, 0x2);
            NV_REG_CHECK(Reg, 0x00000002);
            NV_EMC_REGW(EMC_PA_BASE, PDEX2WR, Reg);

            // *((volatile long *)0x7000F47C) = 0x00000002; // EMC_PDEX2RD_0
            Reg = NV_DRF_NUM(EMC, PDEX2RD, PDEX2RD, 0x2);
            NV_REG_CHECK(Reg, 0x00000002);
            NV_EMC_REGW(EMC_PA_BASE, PDEX2RD, Reg);

            // *((volatile long *)0x7000F480) = 0x00000001; // EMC_PCHG2PDEN_0
            Reg = NV_DRF_NUM(EMC, PCHG2PDEN, PCHG2PDEN, 0x1);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, PCHG2PDEN, Reg);

            // *((volatile long *)0x7000F484) = 0x00000001; // EMC_ACT2PDEN_0
            Reg = NV_DRF_NUM(EMC, ACT2PDEN, ACT2PDEN, 0x1);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, ACT2PDEN, Reg);

            // *((volatile long *)0x7000F488) = 0x00000002; // EMC_AR2PDEN_0
            Reg = NV_DRF_NUM(EMC, AR2PDEN, AR2PDEN, 0x2);
            NV_REG_CHECK(Reg, 0x00000002);
            NV_EMC_REGW(EMC_PA_BASE, AR2PDEN, Reg);

            // *((volatile long *)0x7000F490) = 0x00000003; // EMC_TXSR_0
            Reg = NV_DRF_NUM(EMC, TXSR, TXSR, 0x3);
            NV_REG_CHECK(Reg, 0x00000003);
            NV_EMC_REGW(EMC_PA_BASE, TXSR, Reg);

            //*((volatile long *)0x7000F494) = 0x00000002;  // EMC_TCKE_0
            Reg = NV_DRF_NUM(EMC, TCKE, TCKE, 0x2);
            NV_REG_CHECK(Reg, 0x00000002);
            NV_EMC_REGW(EMC_PA_BASE, TCKE, Reg);

            //*((volatile long *)0x7000F498) = 0x00000000;  // EMC_TFAW_0
            Reg = NV_DRF_NUM(EMC, TFAW, TFAW, 0x0);
            NV_REG_CHECK(Reg, 0x00000000);
            NV_EMC_REGW(EMC_PA_BASE, TFAW, Reg);

            //*((volatile long *)0x7000F49C) = 0x00000000;  // EMC_TRPAB_0
            Reg = NV_DRF_NUM(EMC, TRPAB, TRPAB, 0x0);
            NV_REG_CHECK(Reg, 0x00000000);
            NV_EMC_REGW(EMC_PA_BASE, TRPAB, Reg);

            //*((volatile long *)0x7000F4A0) = 0x00000000;  // EMC_TCLKSTABLE_0
            Reg = NV_DRF_NUM(EMC, TCLKSTABLE, TCLKSTABLE, 0x0);
            NV_REG_CHECK(Reg, 0x00000000);
            NV_EMC_REGW(EMC_PA_BASE, TCLKSTABLE, Reg);

            // *((volatile long *)0x7000F4A4) = 0x00000002; // EMC_TCLKSTOP_0
            Reg = NV_DRF_NUM(EMC, TCLKSTOP, TCLKSTOP, 0x2);
            NV_REG_CHECK(Reg, 0x00000002);
            NV_EMC_REGW(EMC_PA_BASE, TCLKSTOP, Reg);

            // *((volatile long *)0x7000F4A8) = 0x00000000; // EMC_TREFBW_0
            Reg = NV_DRF_NUM(EMC, TREFBW, TREFBW, 0x0);
            NV_REG_CHECK(Reg, 0x00000000);
            NV_EMC_REGW(EMC_PA_BASE, TREFBW, Reg);

            // *((volatile long *)0x7000F48C) = 0x00000004; // EMC_RW2PDEN_0
            Reg = NV_DRF_NUM(EMC, RW2PDEN, RW2PDEN, 0x4);
            NV_REG_CHECK(Reg, 0x00000004);
            NV_EMC_REGW(EMC_PA_BASE, RW2PDEN, Reg);

            // *((volatile long *)0x7000F4F8) = 0x2F2F2F2F; // EMC_FBIO_DQSIB_DLY_0
            Reg = NV_DRF_NUM(EMC, FBIO_DQSIB_DLY, CFG_DQSIB_DLY_BYTE_0, 0x2F)
                | NV_DRF_NUM(EMC, FBIO_DQSIB_DLY, CFG_DQSIB_DLY_BYTE_1, 0x2F)
                | NV_DRF_NUM(EMC, FBIO_DQSIB_DLY, CFG_DQSIB_DLY_BYTE_2, 0x2F)
                | NV_DRF_NUM(EMC, FBIO_DQSIB_DLY, CFG_DQSIB_DLY_BYTE_3, 0x2F);
            NV_REG_CHECK(Reg, 0x2F2F2F2F);
            NV_EMC_REGW(EMC_PA_BASE, FBIO_DQSIB_DLY, Reg);

            // *((volatile long *)0x7000F4FC) = 0x00000000; // EMC_FBIO_DQSIB_DLY_MSB_0
            Reg = NV_DRF_NUM(EMC, FBIO_DQSIB_DLY_MSB, CFG_DQSIB_DLY_MSB_BYTE_0, 0)
                | NV_DRF_NUM(EMC, FBIO_DQSIB_DLY_MSB, CFG_DQSIB_DLY_MSB_BYTE_1, 0)
                | NV_DRF_NUM(EMC, FBIO_DQSIB_DLY_MSB, CFG_DQSIB_DLY_MSB_BYTE_2, 0)
                | NV_DRF_NUM(EMC, FBIO_DQSIB_DLY_MSB, CFG_DQSIB_DLY_MSB_BYTE_3, 0);
            NV_REG_CHECK(Reg, 0x00000000);
            NV_EMC_REGW(EMC_PA_BASE, FBIO_DQSIB_DLY_MSB, Reg);

            // *((volatile long *)0x7000F50C) = 0x2F2F2F2F; // EMC_FBIO_QUSE_DLY_0
            Reg = NV_DRF_NUM(EMC, FBIO_QUSE_DLY, CFG_QUSE_DLY_BYTE_0, 0x2F)
                | NV_DRF_NUM(EMC, FBIO_QUSE_DLY, CFG_QUSE_DLY_BYTE_1, 0x2F)
                | NV_DRF_NUM(EMC, FBIO_QUSE_DLY, CFG_QUSE_DLY_BYTE_2, 0x2F)
                | NV_DRF_NUM(EMC, FBIO_QUSE_DLY, CFG_QUSE_DLY_BYTE_3, 0x2F);
            NV_REG_CHECK(Reg, 0x2F2F2F2F);
            NV_EMC_REGW(EMC_PA_BASE, FBIO_QUSE_DLY, Reg);

            // *((volatile long *)0x7000F50C) = 0x00000000; // EMC_FBIO_QUSE_DLY_MSB_0
            Reg = NV_DRF_NUM(EMC, FBIO_QUSE_DLY_MSB, CFG_QUSE_DLY_MSB_BYTE_0, 0)
                | NV_DRF_NUM(EMC, FBIO_QUSE_DLY_MSB, CFG_QUSE_DLY_MSB_BYTE_1, 0)
                | NV_DRF_NUM(EMC, FBIO_QUSE_DLY_MSB, CFG_QUSE_DLY_MSB_BYTE_2, 0)
                | NV_DRF_NUM(EMC, FBIO_QUSE_DLY_MSB, CFG_QUSE_DLY_MSB_BYTE_3, 0);
            NV_REG_CHECK(Reg, 0x00000000);
            NV_EMC_REGW(EMC_PA_BASE, FBIO_QUSE_DLY_MSB, Reg);

            // *((volatile long *)0x7000F514) = 0x00000002; // EMC_FBIO_CFG6_0
            Reg = NV_DRF_NUM(EMC, FBIO_CFG6, CFG_QUSE_LATE, 2);
            NV_REG_CHECK(Reg, 0x00000002);
            NV_EMC_REGW(EMC_PA_BASE, FBIO_CFG6, Reg);

            // *((volatile long *)0x7000F428) = 0x00000001; // EMC_TIMING_CONTROL_0
            Reg = NV_DRF_NUM(EMC, TIMING_CONTROL, TIMING_UPDATE, 1);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, TIMING_CONTROL, Reg);

            // *((volatile long *)0x7000F424) = 0x00000001; // EMC_PIN_0
            Reg = NV_DRF_DEF(EMC, PIN, PIN_CKE, NORMAL)
                | NV_DRF_DEF(EMC, PIN, PIN_DQM, NORMAL);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, PIN, Reg);

            // *((volatile long *)0x7000F4DC) = 0x00000001; // EMC_NOP_0
            Reg = NV_DRF_NUM(EMC, NOP, NOP_CMD, 1);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, NOP, Reg);

            // *((volatile long *)0x7000F4D8) = 0x00000001; // EMC_PRE_0
            Reg = NV_DRF_NUM(EMC, PRE, PRE_CMD, 0x1);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, PRE, Reg);

            // *((volatile long *)0x7000F4D0) = 0x00100001; // EMC_EMRS_0
            Reg = NV_DRF_NUM(EMC, EMRS, EMRS_ADR, 0x1)
                | NV_DRF_NUM(EMC, EMRS, EMRS_BA, 0x1)
                | NV_DRF_NUM(EMC, EMRS, EMRS_DEV_SELECTN, 0x0);
            NV_REG_CHECK(Reg, 0x00100001);
            NV_EMC_REGW(EMC_PA_BASE, EMRS, Reg);

            // *((volatile long *)0x7000F4CC) = 0x0000002A; // EMC_MRS_0
            Reg = NV_DRF_NUM(EMC, MRS, MRS_ADR, 0x2A)
                | NV_DRF_NUM(EMC, MRS, MRS_BA, 0x0)
                | NV_DRF_NUM(EMC, MRS, MRS_DEV_SELECTN, 0x0);
            NV_REG_CHECK(Reg, 0x0000002A);
            NV_EMC_REGW(EMC_PA_BASE, MRS, Reg);

            //*((volatile long *)0x7000F4D4) = 0x00000001 ;//   EMC_REF_0
            Reg = NV_DRF_NUM(EMC, REF, REF_CMD, 0x1)
                | NV_DRF_NUM(EMC, REF, REF_NUM, 0x0);
            NV_REG_CHECK(Reg, 0x00000001);
            NV_EMC_REGW(EMC_PA_BASE, REF, Reg);
            // We need to write this register twice because that's what the vendor spec requires!
            NV_EMC_REGW(EMC_PA_BASE, REF, Reg);

            //*((volatile long *)0x7000F414) = 0x80000000 ;//   EMC_REFCTRL_0
            Reg = NV_DRF_NUM(EMC, REFCTRL, DEVICE_REFRESH_DISABLE, 0x0)
                | NV_DRF_NUM(EMC, REFCTRL, REF_VALID, 0x1);
            NV_REG_CHECK(Reg, 0x80000000);
            NV_EMC_REGW(EMC_PA_BASE, REFCTRL, Reg);

            // *((volatile long *)0x7000F00C) = 0x00080000; // MC_EMEM_CFG_0
            Reg = NV_DRF_NUM(MC, EMEM_CFG, EMEM_SIZE_KB, 0x00080000);   // !!!FIXME!!! USE ODM QUERY
            NV_REG_CHECK(Reg, 0x00080000);                              // !!!FIXME!!! USE ODM QUERY
            NV_MC_REGW(MC_PA_BASE, EMEM_CFG, Reg);

            // MC_EMEM_ADR_CFG should have same value as that of EMC_ADR_CFG
            // *((volatile long *)0x7000F010) = 0x01060204; // MC_EMEM_ADR_CFG_0
            Reg = NV_DRF_DEF(MC, EMEM_ADR_CFG, EMEM_COLWIDTH, W11)
                | NV_DRF_DEF(MC, EMEM_ADR_CFG, EMEM_BANKWIDTH, W2)
                | NV_DRF_DEF(MC, EMEM_ADR_CFG, EMEM_DEVSIZE, D256MB)
                | NV_DRF_DEF(MC, EMEM_ADR_CFG, EMEM_NUMDEV, N2);
            NV_REG_CHECK(Reg, 0x01060204);
            NV_MC_REGW(MC_PA_BASE, EMEM_ADR_CFG, Reg);

            // Set the EMC_ADR_CFG_0 for SDRAM number of banks, Dev Size, col width, and number of devices
            // EMC_ADR_CFG should have same value as that of MC_EMEM_ADR_CFG
            // *((volatile long *)0x7000F410) = 0x01060204; // EMC_ADR_CFG_0
            Reg = NV_DRF_DEF(EMC, ADR_CFG, EMEM_COLWIDTH, W11)
                | NV_DRF_DEF(EMC, ADR_CFG, EMEM_BANKWIDTH, W2)
                | NV_DRF_DEF(EMC, ADR_CFG, EMEM_DEVSIZE, D256MB)
                | NV_DRF_DEF(EMC, ADR_CFG, EMEM_NUMDEV, N2);
            NV_REG_CHECK(Reg, 0x01060204);
            NV_EMC_REGW(EMC_PA_BASE, ADR_CFG, Reg);

            // Set the EMC_ADR_CFG_1_0 for SDRAM number of banks, Dev Size, col width for the second device.
            // *((volatile long *)0x7000F410) = 0x00060204; // EMC_ADR_CFG_1_0
            Reg = NV_DRF_DEF(EMC, ADR_CFG_1, EMEM1_COLWIDTH, W11)
                | NV_DRF_DEF(EMC, ADR_CFG_1, EMEM1_BANKWIDTH, W2)
                | NV_DRF_DEF(EMC, ADR_CFG_1, EMEM1_DEVSIZE, D256MB);
            NV_REG_CHECK(Reg, 0x00060204);
            NV_EMC_REGW(EMC_PA_BASE, ADR_CFG_1, Reg);

            // *((volatile long *)0x7000F014) = 0x20002030; // MC_EMEM_ARB_CFG0_0
            Reg = NV_DRF_NUM(MC, EMEM_ARB_CFG0, EMEM_RWCNT_TH, 0x30)
                | NV_DRF_NUM(MC, EMEM_ARB_CFG0, EMEM_WRCNT_TH, 0x20)
                | NV_DRF_DEF(MC, EMEM_ARB_CFG0, EMEM_CLEAR_SP_ON_AUTOPC, DISABLED)
                | NV_DRF_DEF(MC, EMEM_ARB_CFG0, EMEM_CLEAR_AP_PREV_SPREQ, ENABLED);
            NV_REG_CHECK(Reg, 0x20002030);
            NV_MC_REGW(MC_PA_BASE, EMEM_ARB_CFG0, Reg);

            // *((volatile long *)0x7000F018) = 0x000007DF; // MC_EMEM_ARB_CFG1_0
            Reg = NV_DRF_DEF(MC, EMEM_ARB_CFG1, EMEM_RCL_MASK, ALL)
                | NV_DRF_DEF(MC, EMEM_ARB_CFG1, EMEM_WCL_MASK, ALL)
                | NV_DRF_DEF(MC, EMEM_ARB_CFG1, EMEM_NORWSWITCH_BKBLOCK, DISABLED)
                | NV_DRF_DEF(MC, EMEM_ARB_CFG1, EMEM_NOWRSWITCH_BKBLOCK, DISABLED)
                | NV_DRF_DEF(MC, EMEM_ARB_CFG1, EMEM_RWSWITCH_RWINEXPIRED, DISABLED)
                | NV_DRF_DEF(MC, EMEM_ARB_CFG1, EMEM_WRSWITCH_WWINEXPIRED, DISABLED)
                | NV_DRF_NUM(MC, EMEM_ARB_CFG1, EMEM_SP_MAX_GRANT, 0);
            NV_REG_CHECK(Reg, 0x000007DF);
            NV_MC_REGW(MC_PA_BASE, EMEM_ARB_CFG1, Reg);

            // *((volatile long *)0x7000F01C) = 0x120A120A; // MC_EMEM_ARB_CFG2_0
            Reg = NV_DRF_NUM(MC, EMEM_ARB_CFG2, EMEM_BANKCNT_RD_TH, 0xA)
                | NV_DRF_NUM(MC, EMEM_ARB_CFG2, EMEM_BANKCNT_NSP_RD_TH, 0x12)
                | NV_DRF_NUM(MC, EMEM_ARB_CFG2, EMEM_BANKCNT_WR_TH, 0xA)
                | NV_DRF_NUM(MC, EMEM_ARB_CFG2, EMEM_BANKCNT_NSP_WR_TH, 0x12);
            NV_REG_CHECK(Reg, 0x120A120A);
            NV_MC_REGW(MC_PA_BASE, EMEM_ARB_CFG2, Reg);

            // *((volatile long *)0x6000C0E0) = 0x00010000; // AHB_ARBITRATION_XBAR_CTRL_0
            Reg = NV_DRF_DEF(AHB_ARBITRATION, XBAR_CTRL, MEM_INIT_DONE, DONE)
                | NV_DRF_DEF(AHB_ARBITRATION, XBAR_CTRL, HOLD_DIS, ENABLE)
                | NV_DRF_DEF(AHB_ARBITRATION, XBAR_CTRL, POST_DIS, ENABLE);
            NV_REG_CHECK(Reg, 0x00010000);
            NV_AHB_ARBC_REGW(AHB_PA_BASE, XBAR_CTRL, Reg);
        }
        else    // !FPGA
#endif  // defined(NVBL_FPGA_SUPPORT)
        {
            NV_ASSERT(0);   // NOT SUPPORTED
        }
    }

    //-------------------------------------------------------------------------
    // Memory Controller Tuning
    //-------------------------------------------------------------------------

#if defined(NVBL_FPGA_SUPPORT)
    // Is this an FPGA?
    if (NvBlSocIsFpga(ChipId))
    {
        // Do nothing for now.
    }
    else
#endif  // defined(NVBL_FPGA_SUPPORT)
    {
        // USE DEFAULTS FOR NOW NV_ASSERT(0);   // NOT SUPPORTED
    }

    // Set up the AHB Mem Gizmo
    Reg = NV_AHB_GIZMO_REGR(AHB_PA_BASE, AHB_MEM);
    Reg = NV_FLD_SET_DRF_NUM(AHB_GIZMO, AHB_MEM, ENABLE_SPLIT, 1, Reg);
    Reg = NV_FLD_SET_DRF_NUM(AHB_GIZMO, AHB_MEM, DONT_SPLIT_AHB_WR, 0, Reg);
    /// Added for USB Controller
    Reg = NV_FLD_SET_DRF_NUM(AHB_GIZMO, AHB_MEM, ENB_FAST_REARBITRATE, 1, Reg);
    NV_AHB_GIZMO_REGW(AHB_PA_BASE, AHB_MEM, Reg);

    // Enable GIZMO settings for USB controller.
    Reg = NV_AHB_GIZMO_REGR(AHB_PA_BASE, USB);
    Reg = NV_FLD_SET_DRF_NUM(AHB_GIZMO, USB, IMMEDIATE, 1, Reg);
    NV_AHB_GIZMO_REGW(AHB_PA_BASE, USB, Reg);

    NV_MC_REGW(MC_PA_BASE, AP_CTRL_0, 0x00000000);
    NV_MC_REGW(MC_PA_BASE, AP_CTRL_1, 0x00000000);

    Reg = NV_EMC_REGR(EMC_PA_BASE, CFG);
    Reg = NV_FLD_SET_DRF_DEF(EMC, CFG, AUTO_PRE_WR, DISABLED, Reg);
    NV_EMC_REGW(EMC_PA_BASE, CFG, Reg);

    // Make sure the debug register is clear.
    Reg = NV_DRF_DEF(APB_MISC, GP_ASDBGREG, CFG2TMC_IDDQ_EN, DISABLE)
        | NV_DRF_DEF(APB_MISC, GP_ASDBGREG, CFG2TMC_IDDQ_EN, DISABLE)
        | NV_DRF_DEF(APB_MISC, GP_ASDBGREG, CFG2TMC_PULLUP_EN, DISABLE)
        | NV_DRF_DEF(APB_MISC, GP_ASDBGREG, CFG2TMC_PULLDOWN_EN, DISABLE)
        | NV_DRF_DEF(APB_MISC, GP_ASDBGREG, CFG2TMC_DEBUG_ENABLE, DISABLE)
        | NV_DRF_DEF(APB_MISC, GP_ASDBGREG, CFG2TMC_PM_ENABLE, DISABLE)
        | NV_DRF_DEF(APB_MISC, GP_ASDBGREG, CFG2TMC_CLKBYP_FUNC, DISABLE)
        | NV_DRF_DEF(APB_MISC, GP_ASDBGREG, CFG2TMC_SW_BP_T1CLK, DISABLE)
        | NV_DRF_DEF(APB_MISC, GP_ASDBGREG, CFG2TMC_SW_BP_T2CLK, DISABLE)
        | NV_DRF_DEF(APB_MISC, GP_ASDBGREG, CFG2TMC_SW_BP_T3CLK, DISABLE)
        | NV_DRF_DEF(APB_MISC, GP_ASDBGREG, CFG2TMC_SW_BP_T4CLK, DISABLE)
        | NV_DRF_DEF(APB_MISC, GP_ASDBGREG, CFG2TMC_SW_BP_T5CLK, DISABLE)
        | NV_DRF_DEF(APB_MISC, GP_ASDBGREG, CFG2TMC_SW_BP_T6CLK, DISABLE)
        | NV_DRF_DEF(APB_MISC, GP_ASDBGREG, CFG2TMC_SW_BP_T7CLK, DISABLE)
        | NV_DRF_DEF(APB_MISC, GP_ASDBGREG, CFG2TMC_SW_BP_CLK_DIV, DISABLE)
        | NV_DRF_DEF(APB_MISC, GP_ASDBGREG, CFG2TMC_RAM_EMAA, DISABLE)
        | NV_DRF_NUM(APB_MISC, GP_ASDBGREG, CFG2TMC_RAM_SVOP_DP, 0)
        | NV_DRF_NUM(APB_MISC, GP_ASDBGREG, CFG2TMC_RAM_SVOP_PDP, 0)
        | NV_DRF_NUM(APB_MISC, GP_ASDBGREG, CFG2TMC_RAM_SVOP_REG, 0)
        | NV_DRF_NUM(APB_MISC, GP_ASDBGREG, CFG2TMC_RAM_SVOP_SP, 0);
    NV_MISC_REGW(MISC_PA_BASE, GP_ASDBGREG, Reg);

    Reg = NV_DRF_NUM( APB_MISC, GP_ASDBGREG3, CFG2TMC_RAM_SVOP_L1DATA, 0)
        | NV_DRF_NUM( APB_MISC, GP_ASDBGREG3, CFG2TMC_RAM_SVOP_L1TAG, 0)
        | NV_DRF_NUM( APB_MISC, GP_ASDBGREG3, CFG2TMC_RAM_SVOP_L2DATA, 0)
        | NV_DRF_NUM( APB_MISC, GP_ASDBGREG3, CFG2TMC_RAM_SVOP_L2TAG, 0)
        | NV_DRF_NUM( APB_MISC, GP_ASDBGREG3, CFG2TMC_RAM_SVOP_IRAM, 0);
    NV_MISC_REGW(MISC_PA_BASE, GP_ASDBGREG3, Reg);

    Reg = NV_MC_REGR(MC_PA_BASE, LOWLATENCY_CONFIG);
    #if DISABLE_LOW_LATENCY_PATH
    // Disable low-latency path.
    Reg = NV_FLD_SET_DRF_DEF(MC, LOWLATENCY_CONFIG, LL_DRAM_INTERLEAVE, ENABLED, Reg)
        | NV_FLD_SET_DRF_DEF(MC, LOWLATENCY_CONFIG, MPCORER_LL_SEND_BOTH, DISABLED, Reg)
        | NV_FLD_SET_DRF_DEF(MC, LOWLATENCY_CONFIG, MPCORER_LL_CTRL, DISABLED, Reg);
    #else
    // Enable low-latency path.
    Reg = NV_FLD_SET_DRF_DEF(MC, LOWLATENCY_CONFIG, LL_DRAM_INTERLEAVE, ENABLED, Reg)
        | NV_FLD_SET_DRF_DEF(MC, LOWLATENCY_CONFIG, MPCORER_LL_SEND_BOTH, ENABLED, Reg)
        | NV_FLD_SET_DRF_DEF(MC, LOWLATENCY_CONFIG, MPCORER_LL_CTRL, ENABLED, Reg);
    #endif
    NV_MC_REGW(MC_PA_BASE, LOWLATENCY_CONFIG, Reg);
}

static void NvBlAvpEnableCpuPowerRailAp20(void)
{
    NvU32   Reg;        // Scratch reg

    Reg = NV_PMC_REGR(PMC_PA_BASE, CNTRL);
    Reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC, CNTRL, CPUPWRREQ_OE, ENABLE, Reg);
    NV_PMC_REGW(PMC_PA_BASE, CNTRL, Reg);
}

NvBool NvBlAvpInit_AP20(NvBool IsRunningFromSdram)
{
    NvBool              IsChipInitialized;
    NvBool              IsLoadedByScript = NV_FALSE;
    NvBootInfoTable*    pBootInfo = (NvBootInfoTable*)(AP20_BASE_PA_BOOT_INFO);

    // See if the boot ROM initialized us.
    IsChipInitialized = NvBlIsChipInitialized();

    // Has the boot ROM done it's part of chip initialization?
    if (!IsChipInitialized)
    {
        // Is this the special case of the boot loader being directly deposited
        // in SDRAM by a debugger script?
        if ((pBootInfo->BootRomVersion == ~0)
        &&  (pBootInfo->DataVersion    == ~0)
        &&  (pBootInfo->RcmVersion     == ~0)
        &&  (pBootInfo->BootType       == ~0)
        &&  (pBootInfo->PrimaryDevice  == ~0))
        {
            // The script must have initialized everything necessary to get
            // the chip up so don't try to reinitialize.
            IsChipInitialized = NV_TRUE;
            IsLoadedByScript  = NV_TRUE;
        }

        // Was the bootloader placed in SDRAM by a USB Recovery Mode applet?
        if (IsRunningFromSdram && NvBlIsChipInRecovery())
        {
            // Keep the Boot Information Table setup by that applet
            // but mark the SDRAM and device as initialized.
            pBootInfo->SdramInitialized = NV_TRUE;
            pBootInfo->DevInitialized   = NV_TRUE;
            IsChipInitialized           = NV_TRUE;
        }
        else
        {
            //Better not come here...ever
        }
    }

    // Enable VDD_CPU
    NvBlAvpEnableCpuPowerRailAp20();

    // Bootloader loaded by a debugger script?
    if (!IsLoadedByScript)
    {
        // Enable the boot clocks.
        NvBlClockInit(IsChipInitialized);
    }
    // Enable Uart after PLLP init
    NvAvpUartInit(PLLP_FIXED_FREQ_KHZ_216000);
    NvOsAvpDebugPrintf("Bootloader AVP Init\n");

    // Initialize memory controller.
    NvBlMemoryControllerInit(IsChipInitialized);

#if KEYSCHED_SECURITY_WAR_BUG_598910
    NvBlAvpTestKeyScheduleSecurity();
#endif

    return IsRunningFromSdram;
}

static void NvBlAvpEnableCpuClock(NvBool enable)
{
    // !!!WARNING!!! THIS FUNCTION MUST NOT USE ANY GLOBAL VARIABLES
    // !!!WARNING!!! THIS FUNCTION MUST NOT USE THE RELOCATION TABLE
    // !!!WARNING!!! THIS FUNCTION MUST NOT BE CALLED FROM THE CPU
    // !!!WARNING!!! THIS FUNCTION MUST BE CALLED WITH A FLAT VIRTUAL ADDRESSING MAP

    NvU32   Reg;        // Scratch reg
    NvU32   Clk;        // Scratch reg

    //-------------------------------------------------------------------------
    // NOTE:  Regardless of whether the request is to enable or disable the CPU
    //        clock, every processor in the CPU complex except the master (CPU
    //        0) will have it's clock stopped because the AVP only talks to the
    //        master. The AVP, it does not know, nor does it need to know that
    //        there are multiple processors in the CPU complex.
    //-------------------------------------------------------------------------

    // Always halt CPU 1 at the flow controller so that in uni-processor
    // configurations the low-power trigger conditions will work properly.
    Reg = NV_DRF_DEF(FLOW_CTLR, HALT_CPU1_EVENTS, MODE, FLOW_MODE_STOP);
    //NV_FLOW_REGW(FLOW_PA_BASE, HALT_CPU1_EVENTS, Reg);    // BUG 557466

    // Need to initialize PLLX?
    if (enable)
    {
        // Initialize PLLX.
        InitPllX();

        // Wait until stable
        NvBlAvpStallUs(NVBOOT_CLOCKS_PLL_STABILIZATION_DELAY);

        //*((volatile long *)0x60006020) = 0x20008888 ;// CCLK_BURST_POLICY
        Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CPU_STATE, 0x2)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, COP_AUTO_CWAKEUP_FROM_FIQ, 0x0)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CPU_AUTO_CWAKEUP_FROM_FIQ, 0x0)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, COP_AUTO_CWAKEUP_FROM_IRQ, 0x0)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CPU_AUTO_CWAKEUP_FROM_IRQ, 0x0)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_FIQ_SOURCE, PLLX_OUT0)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_IRQ_SOURCE, PLLX_OUT0)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_RUN_SOURCE, PLLX_OUT0)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_IDLE_SOURCE, PLLX_OUT0);
        NV_CAR_REGW(CLK_RST_PA_BASE, CCLK_BURST_POLICY, Reg);

        //*((volatile long *)0x60006024) = 0x80000000 ;// SUPER_CCLK_DIVIDER
        Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_ENB, 0x1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_COP_FIQ, 0x0)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_CPU_FIQ, 0x0)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_COP_IRQ, 0x0)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_CPU_IRQ, 0x0)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIVIDEND, 0x0)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIVISOR, 0x0);
        NV_CAR_REGW(CLK_RST_PA_BASE, SUPER_CCLK_DIVIDER, Reg);
    }

    // Read the register containing the main CPU complex clock enable.
    Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_L);

    // Read the register containing the individual CPU clock enables and
    // always stop the clock to CPU 1.
    Clk = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_CPU_CMPLX);
    Clk = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX, CPU1_CLK_STP, 1, Clk);

    if (enable)
    {
        // Enable the CPU clock.
        Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_CPU, ENABLE, Reg);
        Clk  = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX, CPU0_CLK_STP, 0, Clk);
    }
    else
    {
        // Disable the CPU clock.
        Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_CPU, ENABLE, Reg);
        Clk = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX, CPU0_CLK_STP, 1, Clk);
    }
    NV_CAR_REGW(CLK_RST_PA_BASE, CLK_CPU_CMPLX, Clk);
    NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_L, Reg);
}

//----------------------------------------------------------------------------------------------
static NvBool NvBlAvpIsCpuPowered(void)
{
    NvU32   Reg;        // Scratch reg

    Reg = NV_PMC_REGR(PMC_PA_BASE, PWRGATE_STATUS);

    if (!NV_DRF_VAL(APBDEV_PMC, PWRGATE_STATUS, CPU, Reg))
    {
        return NV_FALSE;
    }

    return NV_TRUE;
}

static void NvBlAvpRemoveCpuIoClamps(void)
{
    NvU32   Reg;        // Scratch reg

    // Remove the clamps on the CPU I/O signals.
    Reg = NV_DRF_DEF(APBDEV_PMC, REMOVE_CLAMPING_CMD, CPU, ENABLE);
    NV_PMC_REGW(PMC_PA_BASE, REMOVE_CLAMPING_CMD, Reg);

    // Give I/O signals time to stabilize.
    NvBlAvpStallMs(1);
}

static void  NvBlAvpPowerUpCpu(void)
{
    NvU32   Reg;        // Scratch reg

    if (!NvBlAvpIsCpuPowered())
    {
        // Toggle the CPU power state (OFF -> ON).
        Reg = NV_DRF_DEF(APBDEV_PMC, PWRGATE_TOGGLE, PARTID, CP)
            | NV_DRF_DEF(APBDEV_PMC, PWRGATE_TOGGLE, START, ENABLE);
        NV_PMC_REGW(PMC_PA_BASE, PWRGATE_TOGGLE, Reg);

        // Wait for the power to come up.
        while (!NvBlAvpIsCpuPowered())
        {
            // Do nothing
        }

        // Remove the I/O clamps from CPU power partition.
        // Recommended only a Warm boot, if the CPU partition gets power gated.
        // Shouldn't cause any harm, when called after a cold boot, according to h/w.
        // probably just redundant.
        NvBlAvpRemoveCpuIoClamps();
    }
}

static void NvBlAvpEnableCpuPowerRail(void)
{
    NvU32   Reg;        // Scratch reg

    Reg = NV_PMC_REGR(PMC_PA_BASE, CNTRL);
    Reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC, CNTRL, CPUPWRREQ_OE, ENABLE, Reg);
    NV_PMC_REGW(PMC_PA_BASE, CNTRL, Reg);
}

static void NvBlAvpResetCpu(NvBool reset)
{
    NvU32   Reg;    // Scratch reg
    NvU32   Cpu;    // Scratch reg

    //-------------------------------------------------------------------------
    // NOTE:  Regardless of whether the request is to hold the CPU in reset or
    //        take it out of reset, every processor in the CPU complex except
    //        the master (CPU 0) will be held in reset because the AVP only
    //        talks to the master. The AVP does not know, nor does it need to
    //        know, that there are multiple processors in the CPU complex.
    //-------------------------------------------------------------------------

    // Hold CPU 1 in reset.
    Cpu = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CPURESET1, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DBGRESET1, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DERESET1,  1);
    NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPU_CMPLX_SET, Cpu);

    Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_L);
    if (reset)
    {
        // Place CPU0 into reset.
        Cpu = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CPURESET0, 1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DBGRESET0, 1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DERESET0,  1);
        NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPU_CMPLX_SET, Cpu);

        // Enable master CPU reset.
        Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_CPU_RST, ENABLE, Reg);
    }
    else
    {
        // Take CPU0 out of reset.
        Cpu = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_CPURESET0, 1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_DBGRESET0, 1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_DERESET0,  1);
        NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPU_CMPLX_CLR, Cpu);
        // Disable master CPU reset.
        Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_CPU_RST, DISABLE, Reg);
    }
    NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_L, Reg);
}

static void NvBlAvpClockEnableCorsight(NvBool enable)
{
    NvU32   Rst;        // Scratch register
    NvU32   Clk;        // Scratch register
    NvU32   Src;        // Scratch register

    Rst = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_U);
    Clk = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_U);

    if (enable)
    {
        Rst = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_CSITE_RST, DISABLE, Rst);
        Clk = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_CSITE, ENABLE, Clk);
    }
    else
    {
        Rst = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_CSITE_RST, ENABLE, Rst);
        Clk = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_CSITE, DISABLE, Clk);
    }

    NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_U, Clk);
    NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_U, Rst);

    if (enable)
    {
        // Put CoreSight on PLLP_OUT0 (216 MHz) and divide it down by 1.5
        // giving an effective frequency of 144MHz.
        #if defined(NVBL_FPGA_SUPPORT)
        if (NvBlSocIsFpga(ChipId))
        {
            // Leave CoreSight on the default clock source (oscillator).
        }
        else
        #endif  // defined(NVBL_FPGA_SUPPORT)
        {
            // Note that CoreSight has a fractional divider (LSB == .5).
            Src = NV_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_CSITE, CSITE_CLK_SRC, PLLP_OUT0)
                | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_CSITE, CSITE_CLK_DIVISOR, CLK_DIVIDER(NVBL_PLLP_KHZ, 144000));
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_CSITE, Src);
        }
    }
}

void NvBlStartCpu_AP20(NvU32 ResetVector)
{
    // Enable VDD_CPU
    NvBlAvpEnableCpuPowerRail();

    // Hold the CPUs in reset.
    NvBlAvpResetCpu(NV_TRUE);

    // Disable the CPU clock.
    NvBlAvpEnableCpuClock(NV_FALSE);

    // Enable CoreSight.
    NvBlAvpClockEnableCorsight(NV_TRUE);

    // Set the entry point for CPU execution from reset, if it's a non-zero value.
    if (ResetVector)
    {
        NvBlAvpSetCpuResetVector(ResetVector);
    }

    // Enable the CPU clock.
    NvBlAvpEnableCpuClock(NV_TRUE);

    // Does the CPU already have power?
    if (!NvBlAvpIsCpuPowered())
    {
        // Power up the CPU.
        NvBlAvpPowerUpCpu();
    }

    // Remove the I/O clamps from CPU power partition.
    //NvBlAvpRemoveCpuIoClamps();

    // Take the CPU out of reset.
    NvBlAvpResetCpu(NV_FALSE);
}

NvBool NvBlQueryGetNv3pServerFlag_AP20()
{
    // Pointer to the start of IRAM which is where the Boot Information Table
    // will reside (if it exists at all).
    NvBootInfoTable*  pBootInfo = (NvBootInfoTable*)(NV_BIT_ADDRESS);
    NvU32 * SafeStartAddress = 0;
    NvU32 Nv3pSignature = NV3P_SIGNATURE;
    static NvS8 isNv3pSignatureSet = -1;

    if (isNv3pSignatureSet == 0 || isNv3pSignatureSet == 1)
        return isNv3pSignatureSet;

    // Yes, is there a valid BCT?
    if (pBootInfo->BctValid)
    {   // Is the address valid?
        if (pBootInfo->SafeStartAddr)
        {
            // Get the address where the signature is stored. (Safe start - 4)
            SafeStartAddress = (NvU32 *)(pBootInfo->SafeStartAddr - sizeof(NvU32));
            // compare signature...if this is coming from miniloader

            isNv3pSignatureSet = 0;
            if (*(SafeStartAddress) == Nv3pSignature)
            {
                isNv3pSignatureSet = 1;
                //if yes then modify the safe start address with correct start address (Safe address -4)
                (pBootInfo->SafeStartAddr) -= sizeof(NvU32);
                return NV_TRUE;
            }

        }

    }
    // return false on any failure.
    return NV_FALSE;
}

#if !__GNUC__
__asm void NvBlStartUpAvp_AP20( void )
{
    CODE32
    PRESERVE8

    IMPORT NvBlAvpInit_AP20
    IMPORT NvBlStartCpu_AP20
    IMPORT NvBlAvpHalt

    //;------------------------------------------------------------------
    //; Initialize the AVP, clocks, and memory controller.
    //;------------------------------------------------------------------

    //The SDRAM is guaranteed to be on at this point
    //in the nvml environment. Set r0 = 1.
    MOV     r0, #1
    BL      NvBlAvpInit_AP20

    //;------------------------------------------------------------------
    //; Start the CPU.
    //;------------------------------------------------------------------

    LDR     r0, =ColdBoot_AP20               //; R0 = reset vector for CPU
    BL      NvBlStartCpu_AP20

    //;------------------------------------------------------------------
    //; Transfer control to the AVP code.
    //;------------------------------------------------------------------

    BL      NvBlAvpHalt

    //;------------------------------------------------------------------
    //; Should never get here.
    //;------------------------------------------------------------------
    B       .
}

// we're hard coding the entry point for all AOS images

// this function inits the MPCORE and then jumps to the to the MPCORE
// executable at NV_AOS_ENTRY_POINT
__asm void ColdBoot_AP20( void )
{
    CODE32
    PRESERVE8

    MSR     CPSR_c, #PSR_MODE_SVC _OR_ PSR_I_BIT _OR_ PSR_F_BIT

    //;------------------------------------------------------------------
    //; Check current processor: CPU or AVP?
    //; If AVP, go to AVP boot code, else continue on.
    //;------------------------------------------------------------------

    LDR     r0, =PG_UP_PA_BASE
    LDRB    r2, [r0, #PG_UP_TAG_0]
    CMP     r2, #PG_UP_TAG_0_PID_CPU _AND_ 0xFF //; are we the CPU?
    LDR     sp, =NVAP_LIMIT_PA_IRAM_CPU_EARLY_BOOT_STACK
    // leave in some symbols for release debugging
    LDR     r3, =0xDEADBEEF
    STR     r3, [sp, #-4]!
    STR     r3, [sp, #-4]!
    BNE     is_ap20_avp
    // Workaround for ARM ERRATA 742230 (NVIDIA BUG #716466)
    MRC     p15, 0, r0, c15, c0, 1
    ORR     r0, r0, #(1<<4)
    MCR     p15, 0, r0, c15, c0, 1
    // Workaround for ARM ERRATA 716044 (NVIDIA BUG #750224)
    MRC     p15, 0, r0, c1, c0, 0
    ORR     r0, r0, #(1 << 14)
    MCR     p15, 0, r0, c1, c0, 0
    B       NV_AOS_ENTRY_POINT                   //; yep, we are the CPU

    //;==================================================================
    //; AVP Initialization follows this path
    //;==================================================================

is_ap20_avp
    LDR     sp, =NVAP_LIMIT_PA_IRAM_AVP_EARLY_BOOT_STACK
    // leave in some symbols for release debugging
    LDR     r3, =0xDEADBEEF
    STR     r3, [sp, #-4]!
    STR     r3, [sp, #-4]!
    B       NvBlStartUpAvp_AP20
}
#endif

