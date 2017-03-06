/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "pinmux.h"
#include "nvodm_pinmux_init.h"
#include "pinmux_ap20.h"

#if AVP_PINMUX == 0
static NvPinDrivePingroupConfig whistler_drive_pinmux[] = {
    DEFAULT_DRIVE(DBG),
    DEFAULT_DRIVE(DDC),
    DEFAULT_DRIVE(VI1),
    DEFAULT_DRIVE(VI2),
    DEFAULT_DRIVE(SDIO1),
};
#endif

static NvPingroupConfig whistler_pinmux_avp[] = {
    DEFAULT_CONFIG(UAA,   UARTA,         PULL_UP,   NORMAL, CTL_A, REG_D, REG_B),
    DEFAULT_CONFIG(UAB,   UARTA,         PULL_UP,   NORMAL, CTL_A, REG_D, REG_B),
    DEFAULT_CONFIG(IRRX,  UARTB,         PULL_UP,   NORMAL, CTL_C, REG_C, REG_A),

    DEFAULT_CONFIG(IRTX,  UARTB,         PULL_UP,   NORMAL, CTL_C, REG_C, REG_A),
    DEFAULT_CONFIG(UCA,   UARTC,         PULL_UP,   NORMAL, CTL_B, REG_D, REG_B),
    DEFAULT_CONFIG(UCB,   UARTC,         PULL_UP,   NORMAL, CTL_B, REG_D, REG_B),
    // below 4 pins should be in normal state in bootloader unlike kernel
    // since SPI3 is used for display in BL.
    MUX_TRISTATE_CONFIG(LCSN,  SPI3,          NORMAL, CTL_E, REG_C),
    MUX_TRISTATE_CONFIG(LSCK,  SPI3,          NORMAL, CTL_E, REG_C),
    MUX_TRISTATE_CONFIG(LSDA,  SPI3,          NORMAL, CTL_E, REG_D),
    MUX_TRISTATE_CONFIG(LSDI,  SPI3,          NORMAL, CTL_E, REG_D),

    DEFAULT_CONFIG(SPIA,  SPI3,          PULL_DOWN, TRISTATE, CTL_D, REG_C, REG_B),
    DEFAULT_CONFIG(SPIB,  SPI3,          PULL_DOWN, TRISTATE, CTL_D, REG_C, REG_B),
    DEFAULT_CONFIG(SPIC,  SPI3,          PULL_UP,   TRISTATE, CTL_D, REG_C, REG_B),
    DEFAULT_CONFIG(SPID,  SPI2_ALT,      PULL_DOWN, NORMAL,   CTL_D, REG_C, REG_B),
    DEFAULT_CONFIG(SPIE,  SPI2_ALT,      PULL_UP,   NORMAL,   CTL_D, REG_C, REG_B),
    DEFAULT_CONFIG(SPIF,  SPI2,          PULL_DOWN, NORMAL,   CTL_D, REG_C, REG_B),
    DEFAULT_CONFIG(SPIG,  SPI2_ALT,      PULL_UP,   NORMAL,   CTL_D, REG_C, REG_B),
    DEFAULT_CONFIG(SPIH,  SPI2_ALT,      PULL_UP,   NORMAL,   CTL_D, REG_C, REG_B),
    DEFAULT_CONFIG(UDA,   SPI1,          NORMAL,    NORMAL,   CTL_A, REG_E, REG_D),
};

#if AVP_PINMUX == 0
static NvPingroupConfig whistler_pinmux[] = {
    DEFAULT_CONFIG(ATA,   GMI,           PULL_UP,   NORMAL,    CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(ATB,   GMI,           PULL_UP,   NORMAL,    CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(ATC,   SDIO4,         PULL_UP,   NORMAL,    CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(ATD,   SDIO4,         NORMAL,    NORMAL,    CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(ATE,   GMI,           NORMAL,    NORMAL,    CTL_A,   REG_A,   REG_B),
    DEFAULT_CONFIG(CDEV1, PLLA_OUT,      NORMAL,    NORMAL,    CTL_C,   REG_C,   REG_A),
    DEFAULT_CONFIG(CDEV2, OSC,           PULL_DOWN, TRISTATE,  CTL_C,   REG_C,   REG_A),
    DEFAULT_CONFIG(CRTP,  CRT,           NORMAL,    TRISTATE,  CTL_G,   REG_B,   REG_D),
    DEFAULT_CONFIG(CSUS,  VI_SENSOR_CLK, NORMAL,    NORMAL,    CTL_C,   REG_D,   REG_A),
    DEFAULT_CONFIG(DAP1,  DAP1,          NORMAL,    NORMAL,    CTL_C,   REG_A,   REG_A),
    DEFAULT_CONFIG(DAP2,  DAP2,          NORMAL,    NORMAL,    CTL_C,   REG_A,   REG_A),
    DEFAULT_CONFIG(DAP3,  DAP3,          NORMAL,    NORMAL,    CTL_C,   REG_A,   REG_A),
    DEFAULT_CONFIG(DAP4,  DAP4,          NORMAL,    NORMAL,    CTL_C,   REG_A,   REG_A),
    DEFAULT_CONFIG(DDC,   I2C2,          PULL_UP,   NORMAL,    CTL_C,   REG_E,   REG_B),
    DEFAULT_CONFIG(DTA,   VI,            PULL_DOWN, NORMAL,    CTL_B,   REG_A,   REG_A),
    DEFAULT_CONFIG(DTB,   VI,            PULL_DOWN, NORMAL,    CTL_B,   REG_A,   REG_A),
    DEFAULT_CONFIG(DTC,   VI,            PULL_DOWN, NORMAL,    CTL_B,   REG_A,   REG_A),
    DEFAULT_CONFIG(DTD,   VI,            PULL_DOWN, NORMAL,    CTL_B,   REG_A,   REG_A),
    DEFAULT_CONFIG(DTE,   RSVD1,         NORMAL,    NORMAL,    CTL_B,   REG_A,   REG_A),
    DEFAULT_CONFIG(DTF,   I2C3,          NORMAL,    NORMAL,    CTL_G,   REG_A,   REG_D),
    DEFAULT_CONFIG(GMA,   GMI,           PULL_UP,   NORMAL,    CTL_B,   REG_E,   REG_A),
    DEFAULT_CONFIG(GMB,   GMI,           PULL_UP,   NORMAL,    CTL_C,   REG_E,   REG_B),
    DEFAULT_CONFIG(GMC,   GMI,           PULL_UP,   NORMAL,    CTL_B,   REG_E,   REG_A),
    DEFAULT_CONFIG(GMD,   GMI,           PULL_UP,   NORMAL,    CTL_C,   REG_E,   REG_B),
    DEFAULT_CONFIG(GME,   DAP5,          PULL_UP,   TRISTATE,  CTL_D,   REG_C,   REG_B),
    DEFAULT_CONFIG(GPU,   GMI,           NORMAL,    NORMAL,    CTL_D,   REG_B,   REG_A),
    DEFAULT_CONFIG(GPU7,  RTCK,          NORMAL,    NORMAL,    CTL_G,   REG_B,   REG_D),
    DEFAULT_CONFIG(GPV,   PCIE,          NORMAL,    NORMAL,    CTL_D,   REG_A,   REG_A),

    MUX_TRISTATE_CONFIG(HDINT, HDMI,     TRISTATE,  CTL_B,   REG_C),

    DEFAULT_CONFIG(I2CP,  I2C,           NORMAL,    NORMAL,    CTL_C,   REG_B,   REG_A),
    DEFAULT_CONFIG(KBCA,  KBC,           PULL_UP,   NORMAL,    CTL_C,   REG_B,   REG_A),
    DEFAULT_CONFIG(KBCB,  SDIO2,         PULL_UP,   NORMAL,    CTL_C,   REG_B,   REG_A),
    DEFAULT_CONFIG(KBCC,  KBC,           PULL_UP,   NORMAL,    CTL_C,   REG_B,   REG_B),
    DEFAULT_CONFIG(KBCD,  SDIO2,         PULL_UP,   NORMAL,    CTL_G,   REG_B,   REG_D),
    DEFAULT_CONFIG(KBCE,  KBC,           PULL_UP,   NORMAL,    CTL_A,   REG_E,   REG_A),
    DEFAULT_CONFIG(KBCF,  KBC,           PULL_UP,   NORMAL,    CTL_A,   REG_E,   REG_A),

    MUX_TRISTATE_CONFIG(LD0,   DISPLAYA, NORMAL,    CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD1,   DISPLAYA, NORMAL,    CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD10,  DISPLAYA, NORMAL,    CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD11,  DISPLAYA, NORMAL,    CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD12,  DISPLAYA, NORMAL,    CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD13,  DISPLAYA, NORMAL,    CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD14,  DISPLAYA, NORMAL,    CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD15,  DISPLAYA, NORMAL,    CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD16,  DISPLAYA, NORMAL,    CTL_G,   REG_C),
    MUX_TRISTATE_CONFIG(LD17,  DISPLAYA, NORMAL,    CTL_G,   REG_C),
    MUX_TRISTATE_CONFIG(LD2,   DISPLAYA, NORMAL,    CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD3,   DISPLAYA, NORMAL,    CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD4,   DISPLAYA, NORMAL,    CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD5,   DISPLAYA, NORMAL,    CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD6,   DISPLAYA, NORMAL,    CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD7,   DISPLAYA, NORMAL,    CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD8,   DISPLAYA, NORMAL,    CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD9,   DISPLAYA, NORMAL,    CTL_F,   REG_C),

    MUX_TRISTATE_CONFIG(LDC,   DISPLAYA, TRISTATE,  CTL_E,   REG_C),
    MUX_TRISTATE_CONFIG(LDI,   DISPLAYA, NORMAL,    CTL_G,   REG_D),
    MUX_TRISTATE_CONFIG(LHP0,  DISPLAYA, NORMAL,    CTL_G,   REG_C),
    MUX_TRISTATE_CONFIG(LHP1,  DISPLAYA, NORMAL,    CTL_G,   REG_C),
    MUX_TRISTATE_CONFIG(LHP2,  DISPLAYA, NORMAL,    CTL_G,   REG_C),
    MUX_TRISTATE_CONFIG(LHS,   DISPLAYA, NORMAL,    CTL_E,   REG_D),

    MUX_TRISTATE_CONFIG(LM0,   DISPLAYA, TRISTATE,  CTL_E,   REG_C),
    MUX_TRISTATE_CONFIG(LM1,   DISPLAYA, NORMAL,    CTL_E,   REG_C),
    MUX_TRISTATE_CONFIG(LPP,   DISPLAYA, NORMAL,    CTL_G,   REG_D),
    MUX_TRISTATE_CONFIG(LPW0,  DISPLAYA, TRISTATE,  CTL_E,   REG_D),
    MUX_TRISTATE_CONFIG(LPW1,  DISPLAYA, TRISTATE,  CTL_E,   REG_D),
    MUX_TRISTATE_CONFIG(LPW2,  DISPLAYA, NORMAL,    CTL_E,   REG_D),
    MUX_TRISTATE_CONFIG(LSC0,  DISPLAYA, NORMAL,    CTL_E,   REG_C),
    MUX_TRISTATE_CONFIG(LSC1,  DISPLAYA, TRISTATE,  CTL_E,   REG_C),

    MUX_TRISTATE_CONFIG(LSPI,  DISPLAYA, NORMAL,    CTL_E,   REG_D),
    MUX_TRISTATE_CONFIG(LVP0,  DISPLAYA, NORMAL,    CTL_E,   REG_C),
    MUX_TRISTATE_CONFIG(LVP1,  DISPLAYA, NORMAL,    CTL_G,   REG_C),
    MUX_TRISTATE_CONFIG(LVS,   DISPLAYA, NORMAL,    CTL_E,   REG_C),

    DEFAULT_CONFIG(OWC,   OWR,           PULL_UP,   TRISTATE,    CTL_B,   REG_E, REG_A),

    MUX_TRISTATE_CONFIG(PMC,   PWR_ON,   NORMAL,    CTL_G,   REG_A),

    DEFAULT_CONFIG(PTA,   HDMI,          PULL_UP,   TRISTATE,    CTL_G,   REG_B, REG_A),

    DEFAULT_CONFIG(RM,    I2C,           NORMAL,    NORMAL,    CTL_A,   REG_B,   REG_A),

    MUX_TRISTATE_CONFIG(SDB,   SDIO3,    NORMAL,    CTL_D,   REG_D),

    DEFAULT_CONFIG(SDC,   SDIO3,         PULL_UP,   NORMAL,    CTL_D,   REG_D,   REG_B),
    DEFAULT_CONFIG(SDD,   SDIO3,         PULL_UP,   NORMAL,    CTL_D,   REG_D,   REG_B),
    DEFAULT_CONFIG(SDIO1, RSVD1,         NORMAL,    NORMAL,    CTL_A,   REG_E,   REG_A),

    DEFAULT_CONFIG(SLXA,  SDIO3,         NORMAL,    NORMAL,     CTL_B,   REG_B,   REG_B),
    DEFAULT_CONFIG(SLXC,  SDIO3,         NORMAL,    NORMAL,     CTL_B,   REG_B,   REG_B),
    DEFAULT_CONFIG(SLXD,  SDIO3,         NORMAL,    NORMAL,     CTL_B,   REG_B,   REG_B),
    DEFAULT_CONFIG(SLXK,  SDIO3,         NORMAL,    NORMAL,     CTL_B,   REG_B,   REG_B),
    DEFAULT_CONFIG(SPDI,  RSVD,          NORMAL,    NORMAL,     CTL_D,   REG_B,   REG_B),
    DEFAULT_CONFIG(SPDO,  RSVD,          NORMAL,    NORMAL,     CTL_D,   REG_B,   REG_B),
    DEFAULT_CONFIG(UAC,   OWR,           NORMAL,    NORMAL,     CTL_A,  REG_D,    REG_B),
    DEFAULT_CONFIG(UAD,   IRDA,          PULL_UP,   NORMAL,     CTL_A,  REG_D,    REG_B),
#if 0
    DEFAULT_CONFIG(CK32,  NONE,          NORMAL,    NORMAL,     CTL_A,   REG_D,   REG_A),
    DEFAULT_CONFIG(DDRC,  NONE,          NORMAL,    NORMAL,     CTL_A,   REG_D,   REG_A),
    DEFAULT_CONFIG(PMCA,  NONE,          NORMAL,    NORMAL,     CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(PMCB,  NONE,          NORMAL,    NORMAL,     CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(PMCC,  NONE,          NORMAL,    NORMAL,     CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(PMCD,  NONE,          NORMAL,    NORMAL,     CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(PMCE,  NONE,          PULL_UP,   NORMAL,     CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(XM2C,  NONE,          NORMAL,    NORMAL,     CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(XM2D,  NONE,          NORMAL,    NORMAL,     CTL_A,   REG_A,   REG_A),
#endif
};
#endif

NvError NvOdmPinmuxInit(void)
{
    int err = 0;

    err = NvPinmuxConfigTable(whistler_pinmux_avp, NV_ARRAY_SIZE(whistler_pinmux_avp));
    NV_CHECK_PINMUX_ERROR(err);

#if AVP_PINMUX == 0
    err = NvPinmuxConfigTable(whistler_pinmux, NV_ARRAY_SIZE(whistler_pinmux));
    NV_CHECK_PINMUX_ERROR(err);

    err = NvPinmuxDriveConfigTable(whistler_drive_pinmux,
                    NV_ARRAY_SIZE(whistler_drive_pinmux));
    NV_CHECK_PINMUX_ERROR(err);
#endif

fail:
    return err;
}
