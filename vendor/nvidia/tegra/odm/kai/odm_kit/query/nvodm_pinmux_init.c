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
#include "pinmux_t30.h"
#include "nvcommon.h"

#if AVP_PINMUX == 0

/* !!!FIXME!!!! POPULATE THIS TABLE */
static NvPinDrivePingroupConfig kai_drive_pinmux[] = {
    /* DEFAULT_DRIVE(<pin_group>), */
    /* SET_DRIVE(ATA, DISABLE, DISABLE, DIV_1, 31, 31, FAST, FAST) */
    SET_DRIVE(DAP2,     DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),

    /* All I2C pins are driven to maximum drive strength */
    /* GEN1 I2C */
    SET_DRIVE(DBG,      DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),

    /* GEN2 I2C */
    SET_DRIVE(AT5,      DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),

    /* CAM I2C */
    SET_DRIVE(GME,      DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),

    /* DDC I2C */
    SET_DRIVE(DDC,      DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),

    /* PWR_I2C */
    SET_DRIVE(AO1,      DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),

    /* UART3 */
    SET_DRIVE(UART3,    DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),

    /* SDMMC1 */
    SET_DRIVE(SDIO1,    DISABLE, DISABLE, DIV_1, 46, 42, FAST, FAST),

    /* SDMMC3 */
    SET_DRIVE(SDIO3,    DISABLE, DISABLE, DIV_1, 46, 42, FAST, FAST),

    /* SDMMC4 */
    SET_DRIVE(GMA,      DISABLE, DISABLE, DIV_1, 9, 9, SLOWEST, SLOWEST),
    SET_DRIVE(GMB,      DISABLE, DISABLE, DIV_1, 9, 9, SLOWEST, SLOWEST),
    SET_DRIVE(GMC,      DISABLE, DISABLE, DIV_1, 9, 9, SLOWEST, SLOWEST),
    SET_DRIVE(GMD,      DISABLE, DISABLE, DIV_1, 9, 9, SLOWEST, SLOWEST),

};
#endif

static NvPingroupConfig kai_pinmux_avp[] = {
    /* UART D : DEBUG */
    DEFAULT_PINMUX(GMI_A16,         UARTD,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_A17,         UARTD,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_A18,         UARTD,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_A19,         UARTD,           NORMAL,    NORMAL,     DISABLE),
    /* Power I2C pinmux */
    I2C_PINMUX(PWR_I2C_SCL,     I2CPWR,     NORMAL, NORMAL, ENABLE,  DISABLE,    ENABLE),
    I2C_PINMUX(PWR_I2C_SDA,     I2CPWR,     NORMAL, NORMAL, ENABLE,  DISABLE,    ENABLE),
};

#if AVP_PINMUX == 0
static NvPingroupConfig kai_pinmux_common[] = {
    /* SDMMC1 pinmux */
    DEFAULT_PINMUX(SDMMC1_CLK,      SDMMC1,          NORMAL,     NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC1_CMD,      SDMMC1,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC1_DAT3,     SDMMC1,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC1_DAT2,     SDMMC1,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC1_DAT1,     SDMMC1,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC1_DAT0,     SDMMC1,          PULL_UP,    NORMAL,     ENABLE),

    /* SDMMC3 pinmux */
    DEFAULT_PINMUX(SDMMC3_CLK,      SDMMC3,          NORMAL,     NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC3_CMD,      SDMMC3,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC3_DAT0,     SDMMC3,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC3_DAT1,     SDMMC3,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC3_DAT2,     SDMMC3,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC3_DAT3,     SDMMC3,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC3_DAT6,     SDMMC3,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC3_DAT7,     SDMMC3,          PULL_UP,    NORMAL,     ENABLE),

    /* SDMMC4 pinmux */
    DEFAULT_PINMUX(SDMMC4_CLK,      SDMMC4,          NORMAL,     NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC4_CMD,      SDMMC4,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC4_DAT0,     SDMMC4,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC4_DAT1,     SDMMC4,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC4_DAT2,     SDMMC4,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC4_DAT3,     SDMMC4,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC4_DAT4,     SDMMC4,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC4_DAT5,     SDMMC4,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC4_DAT6,     SDMMC4,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC4_DAT7,     SDMMC4,          PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC4_RST_N,    RSVD1,           PULL_DOWN,  NORMAL,     ENABLE),

    /* I2C1 pinmux */
    I2C_PINMUX(GEN1_I2C_SCL,    I2C1,       NORMAL, NORMAL, ENABLE,  DISABLE,    ENABLE),
    I2C_PINMUX(GEN1_I2C_SDA,    I2C1,       NORMAL, NORMAL, ENABLE,  DISABLE,    ENABLE),

    /* I2C2 pinmux */
    I2C_PINMUX(GEN2_I2C_SCL,    I2C2,       NORMAL, NORMAL, ENABLE,  DISABLE,    ENABLE),
    I2C_PINMUX(GEN2_I2C_SDA,    I2C2,       NORMAL, NORMAL, ENABLE,  DISABLE,    ENABLE),

    /* I2C3 pinmux */
    I2C_PINMUX(CAM_I2C_SCL,     I2C3,       NORMAL, NORMAL, ENABLE,  DISABLE,    ENABLE),
    I2C_PINMUX(CAM_I2C_SDA,     I2C3,       NORMAL, NORMAL, ENABLE,  DISABLE,    ENABLE),

    /* I2C4 pinmux */
    DEFAULT_PINMUX(DDC_SCL,         I2C4,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DDC_SDA,         I2C4,            NORMAL,    NORMAL,     ENABLE),

    /* LCD */
    DEFAULT_PINMUX(LCD_PCLK,        DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_DE,          DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_HSYNC,       DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_VSYNC,       DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D0,          DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D1,          DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D2,          DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D3,          DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D4,          DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D5,          DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D6,          DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D7,          DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D8,          DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D9,          DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D10,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D11,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D12,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D13,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D14,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D15,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D16,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D17,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D18,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D19,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D20,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D21,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D22,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_D23,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),

    /* UART B : GPS */
    DEFAULT_PINMUX(UART2_RXD,       IRDA,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(UART2_TXD,       IRDA,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(UART2_RTS_N,     UARTB,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(UART2_CTS_N,     UARTB,           NORMAL,    NORMAL,     ENABLE),

    /*UART C : BT */
    DEFAULT_PINMUX(UART3_TXD,       UARTC,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(UART3_RXD,       UARTC,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(UART3_CTS_N,     UARTC,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(UART3_RTS_N,     UARTC,           NORMAL,    NORMAL,     DISABLE),

    /*  KBC keys */
    DEFAULT_PINMUX(KB_COL0,         KBC,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_COL1,         KBC,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_COL2,         KBC,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_COL3,         KBC,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_ROW0,         KBC,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_ROW1,         KBC,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_ROW2,         KBC,             PULL_UP,   NORMAL,     ENABLE),

    /* I2S0 : for MODEM */
    DEFAULT_PINMUX(DAP1_FS,         I2S0,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP1_DIN,        I2S0,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP1_DOUT,       I2S0,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP1_SCLK,       I2S0,            NORMAL,    NORMAL,     ENABLE),

    /* I2S1 : for CODEC */
    DEFAULT_PINMUX(DAP2_FS,         I2S1,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP2_DIN,        I2S1,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP2_DOUT,       I2S1,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP2_SCLK,       I2S1,            NORMAL,    NORMAL,     ENABLE),

    /* I2S3 : for BT */
    DEFAULT_PINMUX(DAP4_FS,         I2S3,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP4_DIN,        I2S3,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP4_DOUT,       I2S3,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP4_SCLK,       I2S3,            NORMAL,    NORMAL,     ENABLE),

    /* SPI1 : touch */
    DEFAULT_PINMUX(SPI1_MOSI,       SPI1,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SPI1_SCK,        SPI1,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SPI1_CS0_N,      SPI1,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SPI1_MISO,       SPI1,            NORMAL,    NORMAL,     ENABLE),

    /* SPIDIF */
    DEFAULT_PINMUX(SPDIF_IN,        SPDIF,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SPDIF_OUT,       SPDIF,           NORMAL,    NORMAL,     DISABLE),

    /* FIXED FUNCTION AND CONFIGURATION */
    DEFAULT_PINMUX(CLK_32K_OUT,     BLINK,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(SYS_CLK_REQ,     SYSCLK,          NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(OWR,             OWR,             NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD4,         RSVD1,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(CLK1_OUT,        EXTPERIPH1,      NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(CLK2_OUT,        EXTPERIPH2,      NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(CLK3_OUT,        EXTPERIPH3,      NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(CLK2_REQ,        DAP,             NORMAL,    NORMAL,     ENABLE),
    //DEFAULT_PINMUX(HDMI_INT,        RSVD0,           NORMAL,    TRISTATE,   ENABLE),

    /* GPIO */
    /* POWER RAIL GPIO */
    DEFAULT_PINMUX(DAP3_FS,         I2S2,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_AD14,        RSVD1,           PULL_DOWN, NORMAL,     DISABLE),
    DEFAULT_PINMUX(SDMMC3_DAT5,     SDMMC3,          NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(KB_ROW6,         KBC,             NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(KB_ROW7,         KBC,             NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(LCD_M1,          DISPLAYA,        NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(LCD_PWR0,        DISPLAYA,        NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(LCD_PWR1,        DISPLAYA,        NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(LCD_PWR2,        DISPLAYA,        NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(KB_ROW8,         KBC,             NORMAL,    NORMAL,     DISABLE),

    /* CAMERA */
    //DEFAULT_PINMUX(CAM_MCLK,        VI_ALT2,         PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PCC1,       RSVD1,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PBB0,       RSVD1,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PBB3,       VGP3,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PBB5,       VGP5,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PBB6,       VGP6,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PBB7,       I2S4,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PCC2,       I2S4,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_ROW4,         KBC,             NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(KB_ROW5,         KBC,             NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(KB_ROW9,         KBC,             NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(KB_ROW10,        KBC,             NORMAL,    NORMAL,     DISABLE),

    /* MODEM */
    //DEFAULT_PINMUX(GPIO_PV0,        RSVD,            NORMAL,    NORMAL,     ENABLE),
    //DEFAULT_PINMUX(GPIO_PV1,        RSVD,            NORMAL,    NORMAL,     ENABLE),

    /* GPS and BT */
    DEFAULT_PINMUX(GPIO_PU0,        RSVD1,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PU1,        RSVD1,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GPIO_PU2,        RSVD1,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PU3,        RSVD1,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PU4,        PWM1,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GPIO_PU5,        PWM2,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PU6,        RSVD1,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_ROW14,        KBC,             NORMAL,    TRISTATE,   DISABLE),

    /* LCD GPIO */
    DEFAULT_PINMUX(GMI_AD0,         RSVD1,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_AD1,         RSVD1,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_AD2,         RSVD1,           PULL_DOWN, NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_AD3,         RSVD1,           PULL_DOWN, NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_AD5,         RSVD1,           PULL_DOWN, NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_AD6,         RSVD1,           PULL_DOWN, NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_AD7,         RSVD1,           PULL_DOWN, NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_AD8,         PWM0,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_AD9,         RSVD2,           PULL_DOWN, NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_AD11,        PWM3,            NORMAL,    NORMAL,     DISABLE),

    /* TOUCH */
    DEFAULT_PINMUX(GMI_WAIT,        RSVD1,           PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_WP_N,        RSVD1,           PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_SDOUT,       DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_DC1,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_WR_N,        DISPLAYA,        PULL_UP,   NORMAL,     ENABLE),

    /* SDMMC */
    DEFAULT_PINMUX(GMI_IORDY,       RSVD1,           PULL_UP,   NORMAL,     ENABLE),

    /* CODEC */
    DEFAULT_PINMUX(SPI2_SCK,        SPI2,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(SPI2_CS1_N,      SPI2,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_CS2_N,       RSVD1,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_CS3_N,       RSVD1,           NORMAL,    NORMAL,     ENABLE),

    /* OTHERS */
    DEFAULT_PINMUX(KB_ROW3,         KBC,             NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(GMI_DQS,         RSVD1,           NORMAL,    NORMAL,     ENABLE),

    DEFAULT_PINMUX(GMI_AD15,        RSVD1,           PULL_UP,   NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_CLK,         RSVD1,           PULL_UP,   NORMAL,     ENABLE),

    DEFAULT_PINMUX(GMI_RST_N,       NAND,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(LCD_DC0,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_CS0_N,       DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_CS1_N,       DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_SCK,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_SDIN,        DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(CRT_HSYNC,       CRT,             NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(CRT_VSYNC,       CRT,             NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(PEX_WAKE_N,      PCIE,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(PEX_L2_PRSNT_N,  PCIE,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(PEX_L2_RST_N,    PCIE,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(PEX_L2_CLKREQ_N, PCIE,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(HDMI_CEC,        CEC,             NORMAL,    NORMAL,     ENABLE),

    DEFAULT_PINMUX(KB_ROW15,        KBC,             NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(SPI2_CS2_N,      SPI2,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SPI2_MISO,       SPI2,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SPI2_MOSI,       SPI2,            NORMAL,    NORMAL,     ENABLE),

    DEFAULT_PINMUX(KB_ROW11,        KBC,             NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(KB_ROW12,        KBC,             NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(KB_ROW13,        KBC,             NORMAL,    TRISTATE,   DISABLE),
};

/*Do not use for now*/
static NvPingroupConfig unused_pins_lowpower[] = {
    DEFAULT_PINMUX(ULPI_CLK,        ULPI,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(ULPI_DATA0,      ULPI,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(ULPI_DATA1,      ULPI,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(ULPI_DATA2,      ULPI,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(ULPI_DATA3,      ULPI,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(ULPI_DATA4,      ULPI,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(ULPI_DATA5,      ULPI,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(ULPI_DATA6,      ULPI,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(ULPI_DATA7,      ULPI,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(ULPI_DIR,        ULPI,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(ULPI_NXT,        ULPI,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(ULPI_STP,        ULPI,          NORMAL,    TRISTATE,   DISABLE),

    DEFAULT_PINMUX(GMI_AD10,        PWM2,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(GMI_AD12,        RSVD1,         NORMAL,    TRISTATE,   ENABLE),
    DEFAULT_PINMUX(GMI_AD13,        RSVD1,         NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(CLK1_REQ,        DAP,           NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(GMI_ADV_N,       RSVD1,         NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(GMI_CS0_N,       RSVD1,         NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(GMI_CS1_N,       RSVD1,         NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(GMI_CS4_N,       RSVD1,         NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(GMI_CS6_N,       NAND,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(GMI_CS7_N,       NAND,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(GMI_OE_N,        RSVD1,         NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(GMI_WR_N,        RSVD1,         NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(PEX_L0_CLKREQ_N, PCIE,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(PEX_L0_PRSNT_N,  PCIE,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(PEX_L0_RST_N,    PCIE,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(PEX_L1_CLKREQ_N, PCIE,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(PEX_L1_PRSNT_N,  PCIE,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(PEX_L1_RST_N,    PCIE,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(GPIO_PV2,        OWR,           NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(GPIO_PV3,        RSVD1,         NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(HDMI_CEC,        CEC,           NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(KB_COL4,         KBC,           NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(KB_COL5,         KBC,           NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(KB_COL6,         KBC,           NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(KB_COL7,         KBC,           NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(CLK3_REQ,        DEV3,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(VI_D0,           VI,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(VI_D1,           VI,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(VI_D10,          VI,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(VI_D11,          VI,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(VI_D2,           VI,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(VI_D3,           VI,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(VI_D4,           VI,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(VI_D5,           VI,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(VI_D6,           VI,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(VI_D7,           VI,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(VI_D8,           VI,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(VI_D9,           VI,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(VI_HSYNC,        VI,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(VI_MCLK,         VI,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(VI_PCLK,         VI,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(VI_VSYNC,        VI,            NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(DAP3_DIN,        I2S2,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(DAP3_DOUT,       I2S2,          NORMAL,    TRISTATE,   DISABLE),
    DEFAULT_PINMUX(DAP3_SCLK,       I2S2,          NORMAL,    TRISTATE,   DISABLE),
};
#endif

int NvOdmPinmuxInit(void)
{
    int err = 0;

    err = NvPinmuxConfigTable(kai_pinmux_avp, NV_ARRAY_SIZE(kai_pinmux_avp));
    NV_CHECK_PINMUX_ERROR(err);
#if AVP_PINMUX == 0
    err = NvPinmuxConfigTable(kai_pinmux_common, NV_ARRAY_SIZE(kai_pinmux_common));
    NV_CHECK_PINMUX_ERROR(err);

    err = NvPinmuxDriveConfigTable(kai_drive_pinmux,
                   NV_ARRAY_SIZE(kai_drive_pinmux));
    NV_CHECK_PINMUX_ERROR(err);

    err = NvPinmuxConfigTable(unused_pins_lowpower,
        NV_ARRAY_SIZE(unused_pins_lowpower));
    NV_CHECK_PINMUX_ERROR(err);
#endif

fail:
    return err;
}
