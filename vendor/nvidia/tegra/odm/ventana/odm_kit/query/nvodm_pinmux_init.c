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
#include "nvcommon.h"
#include "pinmux_ap20.h"

#if AVP_PINMUX == 0
static NvPinDrivePingroupConfig ventana_drive_pinmux[] = {
    DEFAULT_DRIVE(DDC),
    DEFAULT_DRIVE(VI1),
    DEFAULT_DRIVE(SDIO1),

    SET_DRIVE(DBG,        DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),
    SET_DRIVE(VI2,        DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),
    SET_DRIVE(AT1,        DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),
    SET_DRIVE(AO1,        DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),
};
#endif

static NvPingroupConfig ventana_pinmux_avp[] = {
                                                             /*  mux     pupd    tristate */
    DEFAULT_CONFIG(GMC,   UARTD,         NORMAL,    NORMAL,     CTL_B,   REG_E,   REG_A),
    DEFAULT_CONFIG(SPID,  SPI1,          NORMAL,    TRISTATE,   CTL_D,   REG_C,   REG_B),
    DEFAULT_CONFIG(SPIE,  SPI1,          NORMAL,    TRISTATE,   CTL_D,   REG_C,   REG_B),
    DEFAULT_CONFIG(SPIF,  SPI1,          PULL_DOWN, TRISTATE,   CTL_D,   REG_C,   REG_B),
    DEFAULT_CONFIG(SPIG,  SPI2_ALT,      PULL_UP,   TRISTATE,   CTL_D,   REG_C,   REG_B),
    DEFAULT_CONFIG(SPIH,  SPI2_ALT,      PULL_UP,   TRISTATE,   CTL_D,   REG_C,   REG_B),
};


#if AVP_PINMUX == 0
static NvPingroupConfig ventana_pinmux[] = {
                                                           /*  mux     pupd    tristate */
    DEFAULT_CONFIG(ATA,   IDE,           NORMAL,    NORMAL,   CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(ATB,   SDIO4,         NORMAL,    NORMAL,   CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(ATC,   NAND,          NORMAL,    NORMAL,   CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(ATD,   GMI,           NORMAL,    NORMAL,   CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(ATE,   GMI,           NORMAL,    TRISTATE, CTL_A,   REG_A,   REG_B),
    DEFAULT_CONFIG(CDEV1, PLLA_OUT,      NORMAL,    NORMAL,   CTL_C,   REG_C,   REG_A),
    DEFAULT_CONFIG(CDEV2, PLLP_OUT4,     NORMAL,    NORMAL,   CTL_C,   REG_C,   REG_A),
    DEFAULT_CONFIG(CRTP,  CRT,           PULL_UP,   TRISTATE, CTL_G,   REG_B,   REG_D),
    DEFAULT_CONFIG(CSUS,  VI_SENSOR_CLK, NORMAL,    NORMAL,   CTL_C,   REG_D,   REG_A),
    DEFAULT_CONFIG(DAP1,  DAP1,          NORMAL,    NORMAL,   CTL_C,   REG_A,   REG_A),
    DEFAULT_CONFIG(DAP2,  DAP2,          NORMAL,    TRISTATE, CTL_C,   REG_A,   REG_A),
    DEFAULT_CONFIG(DAP3,  DAP3,          NORMAL,    TRISTATE, CTL_C,   REG_A,   REG_A),
    DEFAULT_CONFIG(DAP4,  DAP4,          NORMAL,    NORMAL,   CTL_C,   REG_A,   REG_A),
    DEFAULT_CONFIG(DDC,   RSVD1,         NORMAL,    NORMAL,   CTL_C,   REG_E,   REG_B),
    DEFAULT_CONFIG(DTA,   VI,            PULL_DOWN, NORMAL,   CTL_B,   REG_A,   REG_A),
    DEFAULT_CONFIG(DTB,   VI,            PULL_DOWN, NORMAL,   CTL_B,   REG_A,   REG_A),
    DEFAULT_CONFIG(DTC,   VI,            PULL_DOWN, NORMAL,   CTL_B,   REG_A,   REG_A),
    DEFAULT_CONFIG(DTD,   VI,            PULL_DOWN, NORMAL,   CTL_B,   REG_A,   REG_A),
    DEFAULT_CONFIG(DTE,   VI,            NORMAL,    NORMAL,   CTL_B,   REG_A,   REG_A),
    DEFAULT_CONFIG(DTF,   I2C3,          NORMAL,    NORMAL,   CTL_G,   REG_A,   REG_D),
    DEFAULT_CONFIG(GMA,   SDIO4,         NORMAL,    NORMAL,   CTL_B,   REG_E,   REG_A),
    DEFAULT_CONFIG(GMB,   GMI,           PULL_UP,   TRISTATE, CTL_C,   REG_E,   REG_B),
    DEFAULT_CONFIG(GMD,   SFLASH,        NORMAL,    TRISTATE, CTL_C,   REG_E,   REG_B),
    DEFAULT_CONFIG(GME,   SDIO4,         NORMAL,    NORMAL,   CTL_D,   REG_C,   REG_B),
    DEFAULT_CONFIG(GPU,   PWM,           NORMAL,    NORMAL,   CTL_D,   REG_B,   REG_A),
    DEFAULT_CONFIG(GPU7,  RTCK,          NORMAL,    NORMAL,   CTL_G,   REG_B,   REG_D),
    DEFAULT_CONFIG(GPV,   PCIE,          NORMAL,    NORMAL,   CTL_D,   REG_A,   REG_A),

    MUX_TRISTATE_CONFIG(HDINT, HDMI,         TRISTATE, CTL_B, REG_C),

    DEFAULT_CONFIG(I2CP,  I2C,           NORMAL,    NORMAL,   CTL_C,   REG_B,   REG_A),
    DEFAULT_CONFIG(IRRX,  UARTB,         NORMAL,    NORMAL,   CTL_C,   REG_C,   REG_A),
    DEFAULT_CONFIG(IRTX,  UARTB,         NORMAL,    NORMAL,   CTL_C,   REG_C,   REG_A),
    DEFAULT_CONFIG(KBCA,  KBC,           PULL_UP,   NORMAL,   CTL_C,   REG_B,   REG_A),

    DEFAULT_CONFIG(KBCB,  KBC,           PULL_DOWN, NORMAL,   CTL_C,   REG_B,   REG_A),
    DEFAULT_CONFIG(KBCC,  KBC,           PULL_UP,   NORMAL,   CTL_C,   REG_B,   REG_B),
    DEFAULT_CONFIG(KBCD,  KBC,           PULL_UP,   NORMAL,   CTL_G,   REG_B,   REG_D),
    DEFAULT_CONFIG(KBCE,  KBC,           PULL_UP,   NORMAL,   CTL_A,   REG_E,   REG_A),
    DEFAULT_CONFIG(KBCF,  KBC,           PULL_UP,   NORMAL,   CTL_A,   REG_E,   REG_A),

    MUX_TRISTATE_CONFIG(LCSN,  RSVD4,         TRISTATE, CTL_E,   REG_C),
    MUX_TRISTATE_CONFIG(LD0,   DISPLAYA,      NORMAL,   CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD1,   DISPLAYA,      NORMAL,   CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD10,  DISPLAYA,      NORMAL,   CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD11,  DISPLAYA,      NORMAL,   CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD12,  DISPLAYA,      NORMAL,   CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD13,  DISPLAYA,      NORMAL,   CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD14,  DISPLAYA,      NORMAL,   CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD15,  DISPLAYA,      NORMAL,   CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD16,  DISPLAYA,      NORMAL,   CTL_G,   REG_C),
    MUX_TRISTATE_CONFIG(LD17,  DISPLAYA,      NORMAL,   CTL_G,   REG_C),
    MUX_TRISTATE_CONFIG(LD2,   DISPLAYA,      NORMAL,   CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD3,   DISPLAYA,      NORMAL,   CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD4,   DISPLAYA,      NORMAL,   CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD5,   DISPLAYA,      NORMAL,   CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD6,   DISPLAYA,      NORMAL,   CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD7,   DISPLAYA,      NORMAL,   CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD8,   DISPLAYA,      NORMAL,   CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LD9,   DISPLAYA,      NORMAL,   CTL_F,   REG_C),
    MUX_TRISTATE_CONFIG(LDC,   RSVD,          TRISTATE, CTL_E,   REG_C),
    MUX_TRISTATE_CONFIG(LDI,   DISPLAYA,      NORMAL,   CTL_G,   REG_D),
    MUX_TRISTATE_CONFIG(LHP0,  DISPLAYA,      NORMAL,   CTL_G,   REG_C),
    MUX_TRISTATE_CONFIG(LHP1,  DISPLAYA,      NORMAL,   CTL_G,   REG_C),
    MUX_TRISTATE_CONFIG(LHP2,  DISPLAYA,      NORMAL,   CTL_G,   REG_C),
    MUX_TRISTATE_CONFIG(LHS,   DISPLAYA,      NORMAL,   CTL_E,   REG_D),
    MUX_TRISTATE_CONFIG(LM0,   RSVD,          NORMAL,   CTL_E,   REG_C),
    MUX_TRISTATE_CONFIG(LM1,   RSVD3,         TRISTATE, CTL_E,   REG_C),
    MUX_TRISTATE_CONFIG(LPP,   DISPLAYA,      NORMAL,   CTL_G,   REG_D),
    MUX_TRISTATE_CONFIG(LPW0,  DISPLAYA,      NORMAL,   CTL_E,   REG_D),
    MUX_TRISTATE_CONFIG(LPW1,  RSVD,          TRISTATE, CTL_E,   REG_D),
    MUX_TRISTATE_CONFIG(LPW2,  DISPLAYA,      NORMAL,   CTL_E,   REG_D),
    MUX_TRISTATE_CONFIG(LSC0,  DISPLAYA,      NORMAL,   CTL_E,   REG_C),
    MUX_TRISTATE_CONFIG(LSC1,  DISPLAYA,      NORMAL,   CTL_E,   REG_C),
    MUX_TRISTATE_CONFIG(LSCK,  DISPLAYA,      TRISTATE, CTL_E,   REG_C),
    MUX_TRISTATE_CONFIG(LSDA,  DISPLAYA,      TRISTATE, CTL_E,   REG_D),
    MUX_TRISTATE_CONFIG(LSDI,  RSVD,          TRISTATE, CTL_E,   REG_D),
    MUX_TRISTATE_CONFIG(LSPI,  DISPLAYA,      NORMAL,   CTL_E,   REG_D),
    MUX_TRISTATE_CONFIG(LVP0,  RSVD,          TRISTATE, CTL_E,   REG_C),
    MUX_TRISTATE_CONFIG(LVP1,  DISPLAYA,      NORMAL,   CTL_G,   REG_C),
    MUX_TRISTATE_CONFIG(LVS,   DISPLAYA,      NORMAL,   CTL_E,   REG_C),

    DEFAULT_CONFIG(OWC,   RSVD1,         NORMAL,    TRISTATE, CTL_B,   REG_E,   REG_A),

    MUX_TRISTATE_CONFIG(PMC,   PWR_ON,        NORMAL,   CTL_G,   REG_A),

    DEFAULT_CONFIG(PTA,   RSVD3,         NORMAL,    NORMAL,   CTL_G,   REG_B,   REG_A),
    DEFAULT_CONFIG(RM,    I2C,           NORMAL,    NORMAL,   CTL_A,   REG_B,   REG_A),

    MUX_TRISTATE_CONFIG(SDB,   SDIO3,       NORMAL,   CTL_D,  REG_D),

    DEFAULT_CONFIG(SDC,   SDIO3,         NORMAL,    NORMAL,   CTL_D,   REG_D,   REG_B),
    DEFAULT_CONFIG(SDD,   SDIO3,         NORMAL,    NORMAL,   CTL_D,   REG_D,   REG_B),
    DEFAULT_CONFIG(SDIO1, SDIO1,         PULL_UP,   NORMAL,   CTL_A,   REG_E,   REG_A),
    DEFAULT_CONFIG(SLXA,  PCIE,          PULL_UP,   TRISTATE, CTL_B,   REG_B,   REG_B),
    DEFAULT_CONFIG(SLXC,  SDIO3,         NORMAL,    NORMAL,   CTL_B,   REG_B,   REG_B),
    DEFAULT_CONFIG(SLXD,  SPDIF,         NORMAL,    NORMAL,   CTL_B,   REG_B,   REG_B),
    DEFAULT_CONFIG(SLXK,  SDIO3,         NORMAL,    NORMAL,   CTL_B,   REG_B,   REG_B),
    DEFAULT_CONFIG(SPDI,  RSVD,          NORMAL,    NORMAL,   CTL_D,   REG_B,   REG_B),
    DEFAULT_CONFIG(SPDO,  RSVD,          NORMAL,    NORMAL,   CTL_D,   REG_B,   REG_B),
    DEFAULT_CONFIG(SPIA,  GMI,           PULL_UP,   TRISTATE, CTL_D,   REG_C,   REG_B),
    DEFAULT_CONFIG(SPIB,  GMI,           NORMAL,    TRISTATE, CTL_D,   REG_C,   REG_B),
    DEFAULT_CONFIG(SPIC,  GMI,           NORMAL,    TRISTATE, CTL_D,   REG_C,   REG_B),
    DEFAULT_CONFIG(UAA,   ULPI,          PULL_UP,   NORMAL,   CTL_A,   REG_D,   REG_B),
    DEFAULT_CONFIG(UAB,   ULPI,          PULL_UP,   NORMAL,   CTL_A,   REG_D,   REG_B),
    DEFAULT_CONFIG(UAC,   RSVD2,         NORMAL,    NORMAL,   CTL_A,   REG_D,   REG_B),
    DEFAULT_CONFIG(UAD,   IRDA,          NORMAL,    NORMAL,   CTL_A,   REG_D,   REG_B),
    DEFAULT_CONFIG(UCA,   UARTC,         PULL_UP,   NORMAL,   CTL_B,   REG_D,   REG_B),
    DEFAULT_CONFIG(UCB,   UARTC,         PULL_UP,   NORMAL,   CTL_B,   REG_D,   REG_B),
    DEFAULT_CONFIG(UDA,   ULPI,          NORMAL,    NORMAL,   CTL_A,   REG_E,   REG_D),

/*
    //apply on need basis
    DEFAULT_CONFIG(CK32,  NONE,          NORMAL,    NORMAL,   CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(DDRC,  NONE,          NORMAL,    NORMAL,   CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(PMCA,  NONE,          NORMAL,    NORMAL,   CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(PMCB,  NONE,          NORMAL,    NORMAL,   CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(PMCC,  NONE,          NORMAL,    NORMAL,   CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(PMCD,  NONE,          NORMAL,    NORMAL,   CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(PMCE,  NONE,          NORMAL,    NORMAL,   CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(XM2C,  NONE,          NORMAL,    NORMAL,   CTL_A,   REG_A,   REG_A),
    DEFAULT_CONFIG(XM2D,  NONE,          NORMAL,    NORMAL,   CTL_A,   REG_A,   REG_A),
*/
};
#endif

NvError NvOdmPinmuxInit(void)
{
    NvError err = 0;

    err = NvPinmuxConfigTable(ventana_pinmux_avp, NV_ARRAY_SIZE(ventana_pinmux_avp));
    NV_CHECK_PINMUX_ERROR(err);

#if AVP_PINMUX == 0
    err = NvPinmuxConfigTable(ventana_pinmux, NV_ARRAY_SIZE(ventana_pinmux));
    NV_CHECK_PINMUX_ERROR(err);

    err = NvPinmuxDriveConfigTable(ventana_drive_pinmux,
                   NV_ARRAY_SIZE(ventana_drive_pinmux));
    NV_CHECK_PINMUX_ERROR(err);
#endif

fail:
    return err;
}
