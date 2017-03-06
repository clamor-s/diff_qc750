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
#include "nvodm_query_discovery.h"

#define I2C_CNFG 0x0
#define I2C_CMD_ADDR0 0x4
#define I2C_CMD_ADDR1 0x8
#define I2C_CMD_DATA1 0xc
#define I2C_CMD_DATA2 0x10
#define I2C_STATUS 0x1c
#define PWRI2C_PA_BASE 0x7000D000  // Base address for ari2c.h registers

#if AVP_PINMUX == 0

#define NV_BOARD_E1187_ID   0x0B57
#define NV_BOARD_E1186_ID   0x0B56
#define NV_BOARD_E1198_ID   0x0B62
#define NV_BOARD_E1256_ID   0x0C38
#define NV_BOARD_E1257_ID   0x0C39
#define NV_BOARD_E1291_ID   0x0C5B
#define NV_BOARD_PM267_ID   0x0243
#define NV_BOARD_PM269_ID   0x0245
#define NV_BOARD_PM305_ID   0x0305
#define NV_BOARD_PM311_ID   0x030B

/* Board Fab version */
#define BOARD_FAB_A00                   0x0
#define BOARD_FAB_A01                   0x1
#define BOARD_FAB_A02                   0x2
#define BOARD_FAB_A03                   0x3
#define BOARD_FAB_A04                   0x4

static NvPinDrivePingroupConfig cardhu_drive_pinmux[] = {
    /* DEFAULT_DRIVE(<pin_group>), */
    /* SET_DRIVE(ATA, DISABLE, DISABLE, DIV_1, 31, 31, FAST, FAST) */
    SET_DRIVE(DAP2,     DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),

    /* All I2C pins are driven to maximum drive strength */
    /* GEN1 I2C */
    SET_DRIVE(DBG,        DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),

    /* GEN2 I2C */
    SET_DRIVE(AT5,        DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),

    /* CAM I2C */
    SET_DRIVE(GME,        DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),

    /* DDC I2C */
    SET_DRIVE(DDC,        DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),

    /* PWR_I2C */
    SET_DRIVE(AO1,        DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),

    /* UART3 */
    SET_DRIVE(UART3,      DISABLE, ENABLE, DIV_1, 31, 31, FASTEST, FASTEST),

    /* SDMMC1 */
    SET_DRIVE(SDIO1,      DISABLE, DISABLE, DIV_1, 46, 42, FAST, FAST),

    /* SDMMC3 */
    SET_DRIVE(SDIO3,      DISABLE, DISABLE, DIV_1, 46, 42, FAST, FAST),

    /* SDMMC4 */
    SET_DRIVE(GMA,        DISABLE, DISABLE, DIV_1, 9, 9, SLOWEST, SLOWEST),
    SET_DRIVE(GMB,        DISABLE, DISABLE, DIV_1, 9, 9, SLOWEST, SLOWEST),
    SET_DRIVE(GMC,        DISABLE, DISABLE, DIV_1, 9, 9, SLOWEST, SLOWEST),
    SET_DRIVE(GMD,        DISABLE, DISABLE, DIV_1, 9, 9, SLOWEST, SLOWEST),
};
#endif

static NvPingroupConfig cardhu_pinmux_avp[] = {
    DEFAULT_PINMUX(ULPI_DATA0,      UARTA,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(ULPI_DATA1,      UARTA,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_DATA2,      UARTA,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_DATA3,      UARTA,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_DATA4,      UARTA,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_DATA5,      UARTA,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_DATA6,      UARTA,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_DATA7,      UARTA,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(ULPI_CLK,        UARTD,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(ULPI_DIR,        UARTD,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_NXT,        UARTD,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_STP,        UARTD,           NORMAL,    NORMAL,     DISABLE),
    /* Power I2C pinmux */
    I2C_PINMUX(PWR_I2C_SCL,    I2CPWR,    NORMAL,    NORMAL,    ENABLE,    DISABLE,    ENABLE),
    I2C_PINMUX(PWR_I2C_SDA,    I2CPWR,    NORMAL,    NORMAL,    ENABLE,    DISABLE,    ENABLE),
    DEFAULT_PINMUX(SPI1_MOSI,       SPI1,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SPI1_SCK,        SPI1,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SPI1_CS0_N,      SPI1,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SPI1_MISO,       SPI1,            NORMAL,    NORMAL,     ENABLE),
};

#if AVP_PINMUX == 0
static NvPingroupConfig cardhu_pinmux_common[] = {
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
    I2C_PINMUX(GEN1_I2C_SCL,    I2C1,        NORMAL,    NORMAL,    ENABLE,    DISABLE,    ENABLE),
    I2C_PINMUX(GEN1_I2C_SDA,    I2C1,        NORMAL,    NORMAL,    ENABLE,    DISABLE,    ENABLE),

    /* I2C2 pinmux */
    I2C_PINMUX(GEN2_I2C_SCL,    I2C2,        NORMAL,    NORMAL,    ENABLE,    DISABLE,    ENABLE),
    I2C_PINMUX(GEN2_I2C_SDA,    I2C2,        NORMAL,    NORMAL,    ENABLE,    DISABLE,    ENABLE),

    /* I2C3 pinmux */
    I2C_PINMUX(CAM_I2C_SCL,     I2C3,        NORMAL,    NORMAL,    ENABLE,    DISABLE,    ENABLE),
    I2C_PINMUX(CAM_I2C_SDA,     I2C3,        NORMAL,    NORMAL,    ENABLE,    DISABLE,    ENABLE),

    /* I2C4 pinmux */
    DEFAULT_PINMUX(DDC_SCL,     I2C4,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DDC_SDA,     I2C4,        NORMAL,    NORMAL,     ENABLE),

    DEFAULT_PINMUX(ULPI_DATA0,      UARTA,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(ULPI_DATA1,      UARTA,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_DATA2,      UARTA,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_DATA3,      UARTA,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_DATA4,      UARTA,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_DATA5,      UARTA,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_DATA6,      UARTA,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_DATA7,      UARTA,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(ULPI_CLK,        UARTD,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(ULPI_DIR,        UARTD,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_NXT,        UARTD,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_STP,        UARTD,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(DAP3_FS,         I2S2,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP3_DIN,        I2S2,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP3_DOUT,       I2S2,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP3_SCLK,       I2S2,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PV2,        OWR,             NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GPIO_PV3,        RSVD1,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(CLK2_OUT,        EXTPERIPH2,      NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(CLK2_REQ,        DAP,             NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_PWR1,        DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_PWR2,        DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_SDIN,        DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_SDOUT,       DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_WR_N,        DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_DC0,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_PWR0,        DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
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
    DEFAULT_PINMUX(LCD_DC1,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(CRT_HSYNC,       CRT,             NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(CRT_VSYNC,       CRT,             NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(VI_D0,           RSVD1,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(VI_D1,           SDMMC2,          NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(VI_D2,           SDMMC2,          NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(VI_D3,           SDMMC2,          NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(VI_D4,           VI,              NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(VI_D5,           SDMMC2,          NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(VI_D7,           SDMMC2,          NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(VI_D10,          RSVD1,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(VI_MCLK,         VI,              PULL_UP,   NORMAL,     ENABLE),

    DEFAULT_PINMUX(UART2_RXD,       IRDA,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(UART2_TXD,       IRDA,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(UART2_RTS_N,     UARTB,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(UART2_CTS_N,     UARTB,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(UART3_TXD,       UARTC,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(UART3_RXD,       UARTC,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(UART3_CTS_N,     UARTC,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(UART3_RTS_N,     UARTC,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GPIO_PU0,        RSVD1,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PU1,        RSVD1,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GPIO_PU2,        RSVD1,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PU3,        RSVD1,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PU4,        PWM1,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GPIO_PU5,        PWM2,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PU6,        RSVD1,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP4_FS,         I2S3,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP4_DIN,        I2S3,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP4_DOUT,       I2S3,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP4_SCLK,       I2S3,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(CLK3_OUT,        EXTPERIPH3,      NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(CLK3_REQ,        DEV3,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_WP_N,        GMI,             NORMAL,    NORMAL,     ENABLE),

    DEFAULT_PINMUX(KB_ROW5,         OWR,             NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(KB_ROW12,        KBC,             NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(KB_ROW14,        KBC,             NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(KB_ROW15,        KBC,             NORMAL,    NORMAL,     DISABLE),

#if 0 /* for testing on Verbier */
    DEFAULT_PINMUX(GMI_WAIT,        NAND,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_ADV_N,       NAND,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_CLK,         NAND,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_CS0_N,       NAND,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_CS1_N,       NAND,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_CS3_N,       NAND,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_CS4_N,       NAND,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_CS6_N,       NAND_ALT,        NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_CS7_N,       NAND_ALT,        NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_AD0,         NAND,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD1,         NAND,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD2,         NAND,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD3,         NAND,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD4,         NAND,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD5,         NAND,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD6,         NAND,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD7,         NAND,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD8,         NAND,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_AD9,         NAND,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD10,        NAND,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_AD11,        NAND,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD12,        NAND,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD13,        NAND,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD14,        NAND,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD15,        NAND,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_WR_N,        NAND,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_OE_N,        NAND,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_DQS,         NAND,            NORMAL,    NORMAL,     ENABLE),
#else
    DEFAULT_PINMUX(GMI_AD8,         PWM0,            NORMAL,    NORMAL,     DISABLE), /* LCD1_BL_PWM */
#endif
    DEFAULT_PINMUX(GMI_A16,         SPI4,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_A17,         SPI4,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_A18,         SPI4,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_A19,         SPI4,            NORMAL,    NORMAL,     ENABLE),
//FIXME: DEFAULT_PINMUX(CAM_MCLK,        VI_ALT2,         PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PCC1,       RSVD1,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PBB0,       RSVD1,           NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PBB3,       VGP3,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PBB5,       VGP5,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PBB6,       VGP6,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PBB7,       I2S4,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PCC2,       I2S4,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(JTAG_RTCK,       RTCK,            NORMAL,    NORMAL,     DISABLE),

    /*  KBC keys */
    DEFAULT_PINMUX(KB_ROW0,         KBC,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_ROW1,         KBC,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_ROW2,         KBC,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_ROW3,         KBC,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_COL0,         KBC,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_COL1,         KBC,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_COL2,         KBC,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_COL3,         KBC,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_COL4,         KBC,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_COL5,         KBC,             PULL_UP,   NORMAL,     ENABLE),
//FIXME:DEFAULT_PINMUX(GPIO_PV0,        RSVD,            PULL_UP,   NORMAL,     ENABLE),

    DEFAULT_PINMUX(CLK_32K_OUT,     BLINK,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(SYS_CLK_REQ,     SYSCLK,          NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(OWR,             OWR,             NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP1_FS,         I2S0,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP1_DIN,        I2S0,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP1_DOUT,       I2S0,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP1_SCLK,       I2S0,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(CLK1_REQ,        DAP,             NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(CLK1_OUT,        EXTPERIPH1,      NORMAL,    NORMAL,     ENABLE),
#if 0 /* For HDA realtek Codec */
    DEFAULT_PINMUX(SPDIF_IN,        DAP2,            PULL_DOWN, NORMAL,     ENABLE),
#else
    DEFAULT_PINMUX(SPDIF_IN,        SPDIF,           NORMAL,    NORMAL,     ENABLE),
#endif
    DEFAULT_PINMUX(SPDIF_OUT,       SPDIF,           NORMAL,    NORMAL,     DISABLE),
#if 0 /* For HDA realtek Codec */
    DEFAULT_PINMUX(DAP2_FS,         HDA,             PULL_DOWN, NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP2_DIN,        HDA,             PULL_DOWN, NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP2_DOUT,       HDA,             PULL_DOWN, NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP2_SCLK,       HDA,             PULL_DOWN, NORMAL,     ENABLE),
#else
    DEFAULT_PINMUX(DAP2_FS,         I2S1,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP2_DIN,        I2S1,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP2_DOUT,       I2S1,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP2_SCLK,       I2S1,            NORMAL,    NORMAL,     ENABLE),
#endif
    DEFAULT_PINMUX(SPI2_CS1_N,      SPI2,            PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(PEX_L0_PRSNT_N,  PCIE,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(PEX_L0_RST_N,    PCIE,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(PEX_L0_CLKREQ_N, PCIE,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(PEX_WAKE_N,      PCIE,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(PEX_L1_PRSNT_N,  PCIE,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(PEX_L1_RST_N,    PCIE,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(PEX_L1_CLKREQ_N, PCIE,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(PEX_L2_PRSNT_N,  PCIE,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(PEX_L2_RST_N,    PCIE,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(PEX_L2_CLKREQ_N, PCIE,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(HDMI_CEC,        CEC,             NORMAL,    NORMAL,     ENABLE),
//FIXME:DEFAULT_PINMUX(HDMI_INT,        RSVD0,           NORMAL,    TRISTATE,   ENABLE),

    /* Gpios */
    /* SDMMC1 CD gpio */
    DEFAULT_PINMUX(GMI_IORDY,       RSVD1,           PULL_UP,   NORMAL,     ENABLE),
    /* SDMMC1 WP gpio */
    DEFAULT_PINMUX(VI_D11,          RSVD1,           PULL_UP,   NORMAL,     ENABLE),
    /* Touch panel GPIO */
    /* Touch IRQ */
    DEFAULT_PINMUX(GMI_AD12,        NAND,            PULL_UP,   NORMAL,     ENABLE),

    /* Touch RESET */
    DEFAULT_PINMUX(GMI_AD14,        NAND,            NORMAL,    NORMAL,     DISABLE),


    /* Power rails GPIO */
    DEFAULT_PINMUX(SPI2_SCK,        GMI,             NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GPIO_PBB4,       VGP4,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(KB_ROW8,         KBC,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(SDMMC3_DAT5,     SDMMC3,          PULL_UP,   NORMAL,     ENABLE),
    // putting below pin in tristate unlike kernel since otherwise
    // flash fails timeout error from ddk block driver.
    DEFAULT_PINMUX(SDMMC3_DAT4,     SDMMC3,          PULL_UP,   TRISTATE,     ENABLE),

    VI_PINMUX(VI_D6,           VI,              NORMAL,    NORMAL,     DISABLE,  DISABLE, NORMAL),
    VI_PINMUX(VI_D8,           SDMMC2,          NORMAL,    NORMAL,     ENABLE,  DISABLE, NORMAL),
    VI_PINMUX(VI_D9,           SDMMC2,          NORMAL,    NORMAL,     ENABLE,  DISABLE, NORMAL),
    VI_PINMUX(VI_PCLK,         RSVD1,           PULL_UP,   TRISTATE,   ENABLE,  DISABLE, NORMAL),
    VI_PINMUX(VI_HSYNC,        RSVD1,           NORMAL,    NORMAL,     ENABLE,  DISABLE, NORMAL),
    VI_PINMUX(VI_VSYNC,        RSVD1,           NORMAL,    NORMAL,     ENABLE,  DISABLE, NORMAL),
};

static NvPingroupConfig cardhu_pinmux_e118x[] = {
    /* Power rails GPIO */
    DEFAULT_PINMUX(SPI2_SCK,        SPI2,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_RST_N,       RSVD3,           PULL_UP,   TRISTATE,   ENABLE),
    DEFAULT_PINMUX(GMI_AD15,        NAND,            PULL_UP,   TRISTATE,   ENABLE),
};

static NvPingroupConfig cardhu_pinmux_cardhu[] = {
    DEFAULT_PINMUX(LCD_CS0_N,       DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_SCK,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_CS1_N,       DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_M1,          DISPLAYA,        NORMAL,    NORMAL,     ENABLE),

    DEFAULT_PINMUX(GMI_CS2_N,       RSVD1,           PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD8,         PWM0,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_AD10,        NAND,            NORMAL,    NORMAL,     DISABLE),

    /* Power rails GPIO */
    DEFAULT_PINMUX(GMI_CS2_N,       NAND,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_RST_N,       RSVD3,           PULL_UP,   TRISTATE,   ENABLE),
    DEFAULT_PINMUX(GMI_AD15,        NAND,            PULL_UP,   TRISTATE,   ENABLE),

    DEFAULT_PINMUX(GMI_CS0_N,       GMI,             PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_CS1_N,       GMI,             PULL_UP,   TRISTATE,   ENABLE),
    /*TP_IRQ*/
    DEFAULT_PINMUX(GMI_CS4_N,       GMI,             PULL_UP,   NORMAL,     ENABLE),
    /*PCIE dock detect*/
    DEFAULT_PINMUX(GPIO_PU4,        PWM1,            PULL_UP,   NORMAL,     ENABLE),
};

static NvPingroupConfig cardhu_pinmux_cardhu_a03[] = {
    DEFAULT_PINMUX(LCD_CS0_N,       DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_SCK,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_CS1_N,       DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_M1,          DISPLAYA,        NORMAL,    NORMAL,     ENABLE),

    DEFAULT_PINMUX(GMI_CS2_N,       RSVD1,           PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD8,         PWM0,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_AD10,        NAND,            NORMAL,    NORMAL,     DISABLE),

    /* Power rails GPIO */
    DEFAULT_PINMUX(PEX_L0_PRSNT_N,  PCIE,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(PEX_L0_CLKREQ_N, PCIE,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(PEX_L1_CLKREQ_N, RSVD3,           PULL_UP,   TRISTATE,   ENABLE),
    DEFAULT_PINMUX(PEX_L1_PRSNT_N,  RSVD3,           PULL_UP,   TRISTATE,   ENABLE),
};

static NvPingroupConfig cardhu_pinmux_e1291_a04[] = {
    DEFAULT_PINMUX(GMI_AD15,        NAND,            PULL_DOWN,   NORMAL,   DISABLE),
    DEFAULT_PINMUX(ULPI_DATA5,      UARTA,           PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(ULPI_DATA6,      UARTA,           NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(SPI2_MOSI,       SPI6,            NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(DAP3_SCLK,       RSVD1,           NORMAL,    NORMAL,     DISABLE),
};

static NvPingroupConfig cardhu_pinmux_e1198[] = {
    DEFAULT_PINMUX(LCD_CS0_N,       DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_SCK,         DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_CS1_N,       DISPLAYA,        NORMAL,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(LCD_M1,          DISPLAYA,        NORMAL,    NORMAL,     ENABLE),

    DEFAULT_PINMUX(GMI_CS2_N,       RSVD1,           PULL_UP,   NORMAL,     ENABLE),
    DEFAULT_PINMUX(GMI_AD8,         PWM0,            NORMAL,    NORMAL,     DISABLE),
    DEFAULT_PINMUX(GMI_AD10,        NAND,            NORMAL,    NORMAL,     DISABLE),

    /* SPI2 */
    DEFAULT_PINMUX(SPI2_SCK,        SPI2,            PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SPI2_MOSI,       SPI2,            PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SPI2_MISO,       SPI2,            PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SPI2_CS0_N,      SPI2,            PULL_UP,    NORMAL,     ENABLE),
    DEFAULT_PINMUX(SPI2_CS2_N,      SPI2,            PULL_UP,    NORMAL,     ENABLE),
};

static NvPingroupConfig unused_pins_lowpower[] = {
    DEFAULT_PINMUX(GMI_WAIT,        NAND,           PULL_UP,    TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_ADV_N,       NAND,           NORMAL,     TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_CLK,         NAND,           NORMAL,     TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_CS3_N,       NAND,           NORMAL,     NORMAL,       DISABLE),
    DEFAULT_PINMUX(GMI_CS6_N,       SATA,           NORMAL,     NORMAL,       DISABLE),
    DEFAULT_PINMUX(GMI_CS7_N,       NAND,           PULL_UP,    NORMAL,       ENABLE),
    DEFAULT_PINMUX(GMI_AD0,         NAND,           NORMAL,     TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_AD1,         NAND,           NORMAL,     TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_AD2,         NAND,           NORMAL,     TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_AD3,         NAND,           NORMAL,     TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_AD4,         NAND,           NORMAL,     TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_AD5,         NAND,           NORMAL,     TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_AD6,         NAND,           NORMAL,     TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_AD7,         NAND,           NORMAL,     TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_AD9,         PWM1,           NORMAL,     NORMAL,       DISABLE),
    DEFAULT_PINMUX(GMI_AD11,        NAND,           NORMAL,     NORMAL,       DISABLE),
    DEFAULT_PINMUX(GMI_AD13,        NAND,           PULL_UP,    NORMAL,       ENABLE),
    DEFAULT_PINMUX(GMI_WR_N,        NAND,           NORMAL,     TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_OE_N,        NAND,           NORMAL,     TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_DQS,         NAND,           NORMAL,     TRISTATE,     DISABLE),
};
static NvPingroupConfig gmi_pins_269[] = {
    /* Continuation of table unused_pins_lowpower only for PM269 */
    // putting below pin in tristate unlike kernel since otherwise display init fails on pm269
    DEFAULT_PINMUX(GMI_CS0_N,       NAND,           PULL_UP,    TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_CS1_N,       NAND,           PULL_UP,    TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_CS2_N,       RSVD1,          NORMAL,     NORMAL,       ENABLE),
    DEFAULT_PINMUX(GMI_CS3_N,       NAND,           NORMAL,     TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_CS4_N,       NAND,           PULL_UP,    NORMAL,       ENABLE),
    DEFAULT_PINMUX(GMI_CS6_N,       SATA,           NORMAL,     TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_CS7_N,       NAND,           PULL_UP,    NORMAL,       ENABLE),
    DEFAULT_PINMUX(GMI_AD8,         PWM0,           NORMAL,     NORMAL,       DISABLE),
    DEFAULT_PINMUX(GMI_AD9,         PWM1,           NORMAL,     NORMAL,       DISABLE),
    DEFAULT_PINMUX(GMI_AD10,        NAND,           NORMAL,     NORMAL,       DISABLE),
    DEFAULT_PINMUX(GMI_AD11,        NAND,           NORMAL,     NORMAL,       DISABLE),
    DEFAULT_PINMUX(GMI_AD13,        NAND,           PULL_UP,    TRISTATE,     DISABLE),
    DEFAULT_PINMUX(GMI_AD15,        NAND,           PULL_UP,    TRISTATE,     ENABLE),
    DEFAULT_PINMUX(GMI_A16,         SPI4,           NORMAL,     NORMAL,       ENABLE),
    DEFAULT_PINMUX(GMI_A17,         SPI4,           NORMAL,     NORMAL,       ENABLE),
    DEFAULT_PINMUX(GMI_A18,         SPI4,           PULL_UP,    NORMAL,       ENABLE),
    DEFAULT_PINMUX(GMI_A19,         SPI4,           NORMAL,     NORMAL,       ENABLE),
    DEFAULT_PINMUX(GMI_RST_N,       NAND,           PULL_UP,    NORMAL,       ENABLE),
    DEFAULT_PINMUX(GMI_WP_N,        NAND,           NORMAL,     NORMAL,       ENABLE),
};
#endif

#if AVP_PINMUX == 0
static void NvI2cStatusPoll(void)
{
    NvU32 status = 0;
    NvU32 timeout = 0;

    while (timeout < 100000)
    {
        status = NV_READ32(PWRI2C_PA_BASE + I2C_STATUS);
        if (status)
            NvOdmOsWaitUS(1000);
        else
            return ;

        timeout += 1000;
    }
    NV_ASSERT(0);
}

static void get_board_info(NvOdmBoardInfo *board_info, NvU32 Addr)
{
    NvU32 Reg = 0;

    // Program the i2c expander for cardhu
    NV_WRITE32(PWRI2C_PA_BASE + I2C_CMD_ADDR0, 0x42);
    NV_WRITE32(PWRI2C_PA_BASE + I2C_CMD_DATA1, 0x03);
    NV_WRITE32(PWRI2C_PA_BASE + I2C_CNFG, 0xA04);
    NvI2cStatusPoll();

    NV_WRITE32(PWRI2C_PA_BASE + I2C_CMD_ADDR0, 0x42);
    NV_WRITE32(PWRI2C_PA_BASE + I2C_CMD_DATA1, 0x01);
    NV_WRITE32(PWRI2C_PA_BASE + I2C_CNFG, 0xA04);
    NvI2cStatusPoll();

    //Read the Board parameters from eeprom.
    NV_WRITE32(PWRI2C_PA_BASE + I2C_CMD_ADDR0, (Addr & 0xFE));
    NV_WRITE32(PWRI2C_PA_BASE + I2C_CMD_ADDR1, (Addr | 0x1));

    NV_WRITE32(PWRI2C_PA_BASE + I2C_CMD_DATA1, 0x00);
    NV_WRITE32(PWRI2C_PA_BASE + I2C_CNFG, 0x296);
    NvI2cStatusPoll();

    Reg = NV_READ32(PWRI2C_PA_BASE + I2C_CMD_DATA2);
    board_info->BoardID = (Reg >> 8) & 0xFFFF;
    board_info->SKU = (Reg >> 24) & 0xFF;

    NV_WRITE32(PWRI2C_PA_BASE + I2C_CMD_DATA1, 0x04);
    NV_WRITE32(PWRI2C_PA_BASE + I2C_CNFG, 0x296);
    NvI2cStatusPoll();

    Reg = NV_READ32(PWRI2C_PA_BASE + I2C_CMD_DATA2);
    board_info->SKU |= (Reg & 0xFF) << 8;
    board_info->Fab = (Reg >> 8) & 0xFF;
    board_info->Revision= (Reg >> 16) & 0xFF;
    board_info->MinorRevision= (Reg >> 24) & 0xFF;
}
#endif

int NvOdmPinmuxInit(void)
{
    int err = 0;
#if AVP_PINMUX == 0
    NvOdmBoardInfo BoardInfo;

    err = NvPinmuxConfigTable(cardhu_pinmux_common, NV_ARRAY_SIZE(cardhu_pinmux_common));
    NV_CHECK_PINMUX_ERROR(err);

    err = NvPinmuxDriveConfigTable(cardhu_drive_pinmux, NV_ARRAY_SIZE(cardhu_drive_pinmux));
    NV_CHECK_PINMUX_ERROR(err);

    NvOdmPeripheralGetBoardInfo(0, &BoardInfo);

    switch (BoardInfo.BoardID) {
    case NV_BOARD_E1198_ID:
        err = NvPinmuxConfigTable(cardhu_pinmux_e1198,
                    NV_ARRAY_SIZE(cardhu_pinmux_e1198));
        NV_CHECK_PINMUX_ERROR(err);

        err = NvPinmuxConfigTable(unused_pins_lowpower,
                    NV_ARRAY_SIZE(unused_pins_lowpower));
        NV_CHECK_PINMUX_ERROR(err);

        if (BoardInfo.Fab >= BOARD_FAB_A02)
        {
            err = NvPinmuxConfigTable(cardhu_pinmux_cardhu_a03,
                    NV_ARRAY_SIZE(cardhu_pinmux_cardhu_a03));
            NV_CHECK_PINMUX_ERROR(err);
        }
        break;
    case NV_BOARD_E1291_ID:
        if (BoardInfo.Fab < BOARD_FAB_A03)
        {
            err = NvPinmuxConfigTable(cardhu_pinmux_cardhu,
                    NV_ARRAY_SIZE(cardhu_pinmux_cardhu));
            NV_CHECK_PINMUX_ERROR(err);

            err = NvPinmuxConfigTable(unused_pins_lowpower,
                    NV_ARRAY_SIZE(unused_pins_lowpower));
            NV_CHECK_PINMUX_ERROR(err);
        }
        else
        {
            err = NvPinmuxConfigTable(cardhu_pinmux_cardhu_a03,
                    NV_ARRAY_SIZE(cardhu_pinmux_cardhu_a03));
            NV_CHECK_PINMUX_ERROR(err);
        }
        if (BoardInfo.Fab >= BOARD_FAB_A04)
        {
            err = NvPinmuxConfigTable(cardhu_pinmux_e1291_a04,
                    NV_ARRAY_SIZE(cardhu_pinmux_e1291_a04));
            NV_CHECK_PINMUX_ERROR(err);
        }
        break;

    case NV_BOARD_PM269_ID:
    case NV_BOARD_PM305_ID:
    case NV_BOARD_PM311_ID:
    case NV_BOARD_E1257_ID:
        err = NvPinmuxConfigTable(cardhu_pinmux_e118x,
                    NV_ARRAY_SIZE(cardhu_pinmux_e118x));
        NV_CHECK_PINMUX_ERROR(err);
        err = NvPinmuxConfigTable(unused_pins_lowpower,
                    NV_ARRAY_SIZE(unused_pins_lowpower));
        NV_CHECK_PINMUX_ERROR(err);
        err = NvPinmuxConfigTable(gmi_pins_269,
                    NV_ARRAY_SIZE(gmi_pins_269));
        NV_CHECK_PINMUX_ERROR(err);
        break;

    default:
        err = NvPinmuxConfigTable(cardhu_pinmux_e118x,
                    NV_ARRAY_SIZE(cardhu_pinmux_e118x));
        NV_CHECK_PINMUX_ERROR(err);
        break;
    }
#endif
    err = NvPinmuxConfigTable(cardhu_pinmux_avp, NV_ARRAY_SIZE(cardhu_pinmux_avp));
    NV_CHECK_PINMUX_ERROR(err);

fail:
    return err;
}
