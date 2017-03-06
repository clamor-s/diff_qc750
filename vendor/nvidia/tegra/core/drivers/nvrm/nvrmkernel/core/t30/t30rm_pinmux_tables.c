/*
 * Copyright (c) 2009-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvrm_pinmux.h"
#include "nvrm_drf.h"
#include "nvassert.h"
#include "nvrm_hwintf.h"
#include "t30/arapb_misc.h"
#include "t30/arclk_rst.h"
#include "t30/arapbpm.h"
#include "t30/t30rm_pinmux_utils.h"
#include "nvrm_clocks.h"
#include "nvodm_query_pinmux.h"


#ifndef SET_KERNEL_PINMUX
//  FIXME:  None of the modules have reset configurations, yet.  This should
//  be fixed.

static const NvU32 s_T30MuxUart1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    UNCONFIG(UART2_RTS_N, DISABLE, UARTA, UARTB),
    UNCONFIG(UART2_CTS_N, DISABLE, UARTA, UARTB),
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(UART2_RTS_N,ENABLE,GMI),
    CONFIG(UART2_CTS_N,ENABLE,GMI),
    CONFIG(ULPI_DATA0,DISABLE,UARTA), /* UA3_TXD:o */
    CONFIG(ULPI_DATA1,ENABLE,UARTA),  /* UA3_RXD:i */
    CONFIG(ULPI_DATA2,ENABLE,UARTA),  /* UA3_CTS:i */
    CONFIG(ULPI_DATA3,DISABLE,UARTA), /* UA3_RTS:o */
    CONFIG(ULPI_DATA4,ENABLE,UARTA),  /* UA3_RI:i */
    CONFIG(ULPI_DATA5,ENABLE,UARTA),  /* UA3_DCD:i */
    CONFIG(ULPI_DATA6,ENABLE,UARTA),  /* UA3_DSR:i */
    CONFIG(ULPI_DATA7,DISABLE,UARTA), /* UA3_DTR:o */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(SDMMC1_DAT3,DISABLE,UARTA), /* UA3_TXD:o */
    CONFIG(SDMMC1_DAT2,ENABLE,UARTA),  /* UA3_RXD:i */
    CONFIG(SDMMC1_CMD,ENABLE,UARTA),   /* UA3_CTS:i */
    CONFIG(SDMMC1_DAT0,DISABLE,UARTA), /* UA3_RTS:o */
    CONFIG(SDMMC1_DAT1,ENABLE,UARTA),  /* UA3_RI:i */
    CONFIG(GPIO_PU6,ENABLE,UARTA),     /* UA3_DSR:i */
    CONFIG(SDMMC1_CLK,DISABLE,UARTA),  /* UA3_DTR:o */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 3                  */
    /*------------------------------------------*/
    CONFIG(UART2_RTS_N,DISABLE,UARTA), /* UA3_TXD:o */
    CONFIG(UART2_CTS_N,ENABLE,UARTA),  /* UA3_RXD:i */
    CONFIG(UART2_RXD,ENABLE,UARTA),    /* UA3_CTS:i */
    CONFIG(UART2_TXD,DISABLE,UARTA),   /* UA3_RTS:o */
    CONFIG(GPIO_PU5,ENABLE,UARTA),     /* UA3_RI:i */
    CONFIG(GPIO_PU4,DISABLE,UARTA),    /* UA3_DTR:o */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 4                  */
    /*------------------------------------------*/
    CONFIG(GPIO_PU0,DISABLE,UARTA), /* UA3_TXD:o */
    CONFIG(GPIO_PU1,ENABLE,UARTA),  /* UA3_RXD:i */
    CONFIG(GPIO_PU2,ENABLE,UARTA),  /* UA3_CTS:i */
    CONFIG(GPIO_PU3,DISABLE,UARTA), /* UA3_RTS:o */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 5                  */
    /*------------------------------------------*/
    CONFIG(SDMMC3_CLK,DISABLE,UARTA), /* UA3_TXD:o */
    CONFIG(SDMMC3_CMD,ENABLE,UARTA),  /* UA3_RXD:i */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 6                  */
    /*------------------------------------------*/
    CONFIG(ULPI_DATA0,DISABLE,UARTA), /* UA3_TXD:o */
    CONFIG(ULPI_DATA1,ENABLE,UARTA),  /* UA3_RXD:i */
    CONFIG(ULPI_DATA2,ENABLE,UARTA),  /* UA3_CTS:i */
    CONFIG(ULPI_DATA3,DISABLE,UARTA), /* UA3_RTS:o */
    CONFIGEND(),


    MODULEDONE(),
};

static const NvU32 s_T30MuxUart2[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(UART2_TXD,DISABLE,IRDA),    /* IR3_TXD:o */
    CONFIG(UART2_RXD,ENABLE,IRDA),     /* IR3_RXD:i */
    CONFIG(UART2_RTS_N,DISABLE,UARTB), /* UB3_RTS:o */
    CONFIG(UART2_CTS_N,ENABLE,UARTB),  /* UB3_CTS:i */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(UART2_TXD,DISABLE,IRDA),    /* IR3_TXD:o */
    CONFIG(UART2_RXD,ENABLE,IRDA),     /* IR3_RXD:i */
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 3                  */
    /*------------------------------------------*/
    CONFIG(UART2_RTS_N,DISABLE,UARTA), /* UA3_TXD:o */
    CONFIG(UART2_CTS_N,ENABLE,UARTA),  /* UA3_RXD:i */
    CONFIG(UART2_RXD,ENABLE,UARTA),    /* UA3_CTS:i */
    CONFIG(UART2_TXD,DISABLE,UARTA),   /* UA3_RTS:o */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxUart3[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(UART3_TXD,DISABLE,UARTC),   /* UC3_TXD:o */
    CONFIG(UART3_RXD,ENABLE,UARTC),    /* UC3_RXD:i */
    CONFIG(UART3_CTS_N,ENABLE,UARTC),  /* UC3_CTS:i */
    CONFIG(UART3_RTS_N,DISABLE,UARTC), /* UC3_RTS:o */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxUart4[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(ULPI_CLK,DISABLE,UARTD), /* UD3_TXD:o */
    CONFIG(ULPI_DIR,ENABLE,UARTD),  /* UD3_RXD:i */
    CONFIG(ULPI_STP,DISABLE,UARTD), /* UD3_RTS:o */
    CONFIG(ULPI_NXT,ENABLE,UARTD),  /* UD3_CTS:i */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(GMI_A16,DISABLE,UARTD), /* UD3_TXD:o */
    CONFIG(GMI_A17,ENABLE,UARTD),  /* UD3_RXD:i */
    CONFIG(GMI_A19,DISABLE,UARTD), /* UD3_RTS:o */
    CONFIG(GMI_A18,ENABLE,UARTD),  /* UD3_CTS:i */
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 3                  */
    /*------------------------------------------*/
    CONFIG(ULPI_CLK,DISABLE,UARTD), /* UD3_TXD:o */
    CONFIG(ULPI_DIR,ENABLE,UARTD),  /* UD3_RXD:i */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxUart5[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(SDMMC1_DAT3,DISABLE,UARTE), /* UE3_TXD:o */
    CONFIG(SDMMC1_DAT2,ENABLE,UARTE),  /* UE3_RXD:i */
    CONFIG(SDMMC1_DAT1,ENABLE,UARTE),  /* UE3_CTS:i */
    CONFIG(SDMMC1_DAT0,DISABLE,UARTE), /* UE3_RTS:o */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(SDMMC4_DAT0,DISABLE,UARTE), /* UE3_TXD:o */
    CONFIG(SDMMC4_DAT1,ENABLE,UARTE),  /* UE3_RXD:i */
    CONFIG(SDMMC4_DAT2,ENABLE,UARTE),  /* UE3_CTS:i */
    CONFIG(SDMMC4_DAT3,DISABLE,UARTE), /* UE3_RTS:o */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32* s_T30MuxUart[] = {
    &s_T30MuxUart1[0],
    &s_T30MuxUart2[0],
    &s_T30MuxUart3[0],
    &s_T30MuxUart4[0],
    &s_T30MuxUart5[0],
    NULL,
};

static const NvU32 s_T30MuxSpi1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    UNCONFIG(ULPI_CLK, DISABLE, SPI1, RSVD),
    UNCONFIG(ULPI_DIR, DISABLE, SPI1, RSVD),
    UNCONFIG(ULPI_NXT, DISABLE, SPI1, RSVD),
    UNCONFIG(ULPI_STP, DISABLE, SPI1, RSVD),
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(ULPI_STP, ENABLE, SPI1), /* SPI1A_CS0:b */
    CONFIG(ULPI_NXT, ENABLE, SPI1), /* SPI1A_SCK:b */
    CONFIG(ULPI_DIR, ENABLE, SPI1), /* SPI1A_DIN:b */
    CONFIG(ULPI_CLK, ENABLE, SPI1), /* SPI1A_DOUT:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(SPI1_CS0_N, ENABLE, SPI1), /* SPI1B_CS0:b */
    CONFIG(SPI1_SCK, ENABLE, SPI1),   /* SPI1B_SCK:b */
    CONFIG(SPI1_MISO, ENABLE, SPI1),  /* SPI1B_DIN:b */
    CONFIG(SPI1_MOSI, ENABLE, SPI1),  /* SPI1B_DOUT:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxSpi2[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(ULPI_DATA6, ENABLE, SPI2), /* SPI2A_SCK:b */
    CONFIG(ULPI_DATA5, ENABLE, SPI2), /* SPI2A_DIN:b */
    CONFIG(ULPI_DATA4, ENABLE, SPI2), /* SPI2A_DOUT:b */
    CONFIG(ULPI_DATA7, ENABLE, SPI2), /* SPI2A_CS1:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(SPI2_CS0_N, ENABLE, SPI2), /* SPI2B_CS0:b */
    CONFIG(SPI2_SCK, ENABLE, SPI2),   /* SPI2B_SCK:b */
    CONFIG(SPI2_MISO, ENABLE, SPI2),  /* SPI2B_DIN:b */
    CONFIG(SPI2_MOSI, ENABLE, SPI2),  /* SPI2B_DOUT:b */
    CONFIG(SPI2_CS1_N, ENABLE, SPI2), /* SPI2B_CS1:b */
    CONFIG(SPI2_CS2_N, ENABLE, SPI2), /* SPI2B_CS2:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 3                  */
    /*------------------------------------------*/
    CONFIG(SPI1_SCK, ENABLE, SPI2_ALT),   /* SPI2C_SCK:b */
    CONFIG(SPI1_MISO, ENABLE, SPI2),      /* SPI2C_DIN:b */
    CONFIG(SPI1_MOSI, ENABLE, SPI2_ALT),  /* SPI2C_DOUT:b */
    CONFIG(SPI1_CS0_N, ENABLE, SPI2_ALT), /* SPI2C_CS1:b */
    CONFIG(SPI2_CS1_N, ENABLE, SPI2_ALT), /* SPI2C_CS2:b */
    CONFIG(SPI2_CS2_N, ENABLE, SPI2_ALT), /* SPI2C_CS3:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 4                  */
    /*------------------------------------------*/
    CONFIG(SDMMC3_DAT6, ENABLE, SPI2), /* SPI2D_CS0:b */
    CONFIG(SDMMC3_CMD, ENABLE, SPI2),  /* SPI2D_SCK:b */
    CONFIG(SDMMC3_DAT4, ENABLE, SPI2), /* SPI2D_DIN:b */
    CONFIG(SDMMC3_DAT5, ENABLE, SPI2), /* SPI2D_DOUT:b */
    CONFIG(SDMMC3_DAT7, ENABLE, SPI2), /* SPI2D_CS1:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 5                  */
    /*------------------------------------------*/
    CONFIG(SPI1_SCK, ENABLE, SPI2),   /* SPI2E_SCK:b */
    CONFIG(SPI1_MOSI, ENABLE, SPI2),  /* SPI2E_DOUT:b */
    CONFIG(SPI1_CS0_N, ENABLE, SPI2), /* SPI2E_CS1:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxSpi3[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(SDMMC4_DAT3, ENABLE, SPI3), /* SPI3A_CS0:b */
    CONFIG(SDMMC4_DAT0, ENABLE, SPI3), /* SPI3A_SCK:b */
    CONFIG(SDMMC4_DAT2, ENABLE, SPI3), /* SPI3A_DIN:b */
    CONFIG(SDMMC4_DAT1, ENABLE, SPI3), /* SPI3A_DOUT:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(SPI2_SCK, ENABLE, SPI3),   /* SPI3B_SCK:b */
    CONFIG(SPI2_MISO, ENABLE, SPI3),  /* SPI3B_DIN:b */
    CONFIG(SPI2_MOSI, ENABLE, SPI3),  /* SPI3B_DOUT:b */
    CONFIG(SPI2_CS0_N, ENABLE, SPI3), /* SPI3B_CS1:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 3                  */
    /*------------------------------------------*/
    CONFIG(SPI2_CS2_N, ENABLE, SPI3), /* SPI3C_CS0:b */
    CONFIG(SPI2_CS1_N, ENABLE, SPI3), /* SPI3C_SCK:b */
    CONFIG(SPI1_MISO, ENABLE, SPI3),  /* SPI3C_DIN:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 4                  */
    /*------------------------------------------*/
    CONFIG(SDMMC3_DAT2, ENABLE, SPI3), /* SPI3D_CS0:b */
    CONFIG(SDMMC3_CLK, ENABLE, SPI3),  /* SPI3D_SCK:b */
    CONFIG(SDMMC3_DAT0, ENABLE, SPI3), /* SPI3D_DIN:b */
    CONFIG(SDMMC3_DAT1, ENABLE, SPI3), /* SPI3D_DOUT:b */
    CONFIG(SDMMC3_DAT3, ENABLE, SPI3), /* SPI3D_CS1:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 5                  */
    /*------------------------------------------*/
    CONFIG(ULPI_DATA2, ENABLE, SPI3), /* SPI3E_SCK:b */
    CONFIG(ULPI_DATA1, ENABLE, SPI3), /* SPI3E_DIN:b */
    CONFIG(ULPI_DATA0, ENABLE, SPI3), /* SPI3E_DOUT:b */
    CONFIG(ULPI_DATA3, ENABLE, SPI3), /* SPI3E_CS1:b */
    CONFIGEND(),

    MODULEDONE(),
};


static const NvU32 s_T30MuxSpi4[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(SDMMC3_DAT6,ENABLE,SPI4), /* SPI4A_CS0:b */
    CONFIG(SDMMC3_DAT5,ENABLE,SPI4), /* SPI4A_SCK:b */
    CONFIG(SDMMC3_DAT4,ENABLE,SPI4), /* SPI4A_DIN:b */
    CONFIG(SDMMC3_DAT7,ENABLE,SPI4), /* SPI4A_DOUT:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(UART2_TXD,ENABLE,SPI4),   /* SPI4B_SCK:b */
    CONFIG(UART2_RTS_N,ENABLE,SPI4), /* SPI4B_DIN:b */
    CONFIG(UART2_RXD,ENABLE,SPI4),   /* SPI4B_DOUT:b */
    CONFIG(UART2_CTS_N,ENABLE,SPI4), /* SPI4B_CS1:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 3                  */
    /*------------------------------------------*/
    CONFIG(GMI_A16,ENABLE,SPI4), /* SPI4C_SCK:b */
    CONFIG(GMI_A18,ENABLE,SPI4), /* SPI4C_DIN:b */
    CONFIG(GMI_A17,ENABLE,SPI4), /* SPI4C_DOUT:b */
    CONFIG(GMI_A19,ENABLE,SPI4), /* SPI4C_CS1:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxSpi5[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(LCD_SCK,ENABLE,SPI5),   /* SPI5A_SCK:b */
    CONFIG(LCD_SDIN,ENABLE,SPI5),  /* SPI5A_DIN:b */
    CONFIG(LCD_SDOUT,ENABLE,SPI5), /* SPI5A_DOUT:b */
    CONFIG(LCD_CS0_N,ENABLE,SPI5), /* SPI5A_CS2:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(LCD_WR_N,ENABLE,SPI5),  /* SPI5B_SCK:b */
    CONFIG(LCD_PWR2,ENABLE,SPI5),  /* SPI5B_DIN:b */
    CONFIG(LCD_PWR0,ENABLE,SPI5),  /* SPI5B_DOUT:b */
    CONFIG(LCD_CS1_N,ENABLE,SPI5), /* SPI5B_CS3:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxSpi6[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(SPI2_CS0_N, ENABLE, SPI6), /* SPI6A_CS0:b */
    CONFIG(SPI2_SCK, ENABLE, SPI6),   /* SPI6A_SCK:b */
    CONFIG(SPI2_MISO, ENABLE, SPI6),  /* SPI6A_DIN:b */
    CONFIG(SPI2_MOSI, ENABLE, SPI6),  /* SPI6A_DOUT:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxSpi[] = {
    &s_T30MuxSpi1[0],
    &s_T30MuxSpi2[0],
    &s_T30MuxSpi3[0],
    &s_T30MuxSpi4[0],
    &s_T30MuxSpi5[0],
    &s_T30MuxSpi6[0],
    NULL,
};


static const NvU32 s_T30MuxTwc1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxTwc[] = {
    &s_T30MuxTwc1[0],
    NULL,
};

static const NvU32 s_T30MuxI2c1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    UNCONFIG(GEN1_I2C_SDA, DISABLE, I2C1, RSVD1),
    UNCONFIG(GEN1_I2C_SCL, DISABLE, I2C1, RSVD2),
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(GEN1_I2C_SCL, ENABLE, I2C1), /* I2C1_CLK:b */
    CONFIG(GEN1_I2C_SDA, ENABLE, I2C1), /* I2C1_DAT:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(SPDIF_OUT, ENABLE, I2C1), /* I2C1_CLK:b */
    CONFIG(SPDIF_IN, ENABLE, I2C1),  /* I2C1_DAT:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 3                  */
    /*------------------------------------------*/
    CONFIG(SPI2_CS1_N, ENABLE, I2C1), /* I2C1_CLK:b */
    CONFIG(SPI2_CS2_N, ENABLE, I2C1), /* I2C1_DAT:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxI2c2[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(GEN2_I2C_SCL, ENABLE, I2C2), /* I2C2_CLK:b */
    CONFIG(GEN2_I2C_SDA, ENABLE, I2C2), /* I2C2_DAT:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxI2c3[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(SDMMC4_CMD, ENABLE, I2C3),  /* I2C3_CLK:b */
    CONFIG(SDMMC4_DAT4, ENABLE, I2C3), /* I2C3_DAT:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(CAM_I2C_SCL, ENABLE, I2C3), /* I2C3_CLK:b */
    CONFIG(CAM_I2C_SDA,ENABLE,I2C3),   /* I2C3_DAT:b */

    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxI2c4[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(DDC_SCL, ENABLE, I2C4), /* I2C4_CLK:b */
    CONFIG(DDC_SDA, ENABLE, I2C4), /* I2C4_DAT:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxI2c5[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(PWR_I2C_SCL, ENABLE, I2CPWR), /* I2CPMU_CLK:b */
    CONFIG(PWR_I2C_SDA, ENABLE, I2CPWR), /* I2CPMU_DAT:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxI2c[] = {
    &s_T30MuxI2c1[0],
    &s_T30MuxI2c2[0],
    &s_T30MuxI2c3[0],
    &s_T30MuxI2c4[0],
    &s_T30MuxI2c5[0],
    NULL,
};

static const NvU32 s_T30MuxUlpi1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(ULPI_DATA0, ENABLE, ULPI), /* ulpi_data0:b */
    CONFIG(ULPI_DATA1, ENABLE, ULPI), /* ulpi_data1:b */
    CONFIG(ULPI_DATA2, ENABLE, ULPI), /* ulpi_data2:b */
    CONFIG(ULPI_DATA3, ENABLE, ULPI), /* ulpi_data3:b */
    CONFIG(ULPI_DATA4, ENABLE, ULPI), /* ulpi_data4:b */
    CONFIG(ULPI_DATA5, ENABLE, ULPI), /* ulpi_data5:b */
    CONFIG(ULPI_DATA6, ENABLE, ULPI), /* ulpi_data6:b */
    CONFIG(ULPI_DATA7, ENABLE, ULPI), /* ulpi_data7:b */
    CONFIG(ULPI_CLK, ENABLE, ULPI),   /* ulpi_clock:b */
    CONFIG(ULPI_DIR, ENABLE, ULPI),   /* ulpi_dir:b */
    CONFIG(ULPI_STP, ENABLE, ULPI),   /* ulpi_stp:b */
    CONFIG(ULPI_NXT, ENABLE, ULPI),   /* ulpi_nxt:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxUlpi[] = {
    &s_T30MuxUlpi1[0],
    NULL,
};

static const NvU32 s_T30MuxSdio1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(SDMMC1_CMD, ENABLE, SDMMC1),  /* SDMMC1_CMD:b */
    CONFIG(SDMMC1_CLK, ENABLE, SDMMC1),  /* SDMMC1_SCLK:b */
    CONFIG(SDMMC1_DAT0, ENABLE, SDMMC1), /* SDMMC1_DAT0:b */
    CONFIG(SDMMC1_DAT1, ENABLE, SDMMC1), /* SDMMC1_DAT1:b */
    CONFIG(SDMMC1_DAT2, ENABLE, SDMMC1), /* SDMMC1_DAT2:b */
    CONFIG(SDMMC1_DAT3, ENABLE, SDMMC1), /* SDMMC1_DAT3:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxSdio2[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(VI_D1, ENABLE, SDMMC2),   /* VISDMMC2_CMD:b */
    CONFIG(VI_PCLK, ENABLE, SDMMC2), /* VISDMMC2_SCLK:b */
    CONFIG(VI_D2, ENABLE, SDMMC2),   /* VISDMMC2_DAT0:b */
    CONFIG(VI_D3, ENABLE, SDMMC2),   /* VISDMMC2_DAT1:b */
    CONFIG(VI_D4, ENABLE, SDMMC2),   /* VISDMMC2_DAT2:b */
    CONFIG(VI_D5, ENABLE, SDMMC2),   /* VISDMMC2_DAT3:b */
    CONFIG(VI_D6, ENABLE, SDMMC2),   /* VISDMMC2_DAT4:b */
    CONFIG(VI_D7, ENABLE, SDMMC2),   /* VISDMMC2_DAT5:b */
    CONFIG(VI_D8, ENABLE, SDMMC2),   /* VISDMMC2_DAT6:b */
    CONFIG(VI_D9, ENABLE, SDMMC2),   /* VISDMMC2_DAT7:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(KB_ROW11, ENABLE, SDMMC2), /* KBCSDMMC2_CMD:b */
    CONFIG(KB_ROW10, ENABLE, SDMMC2), /* KBCSDMMC2_SCLK:b */
    CONFIG(KB_ROW12, ENABLE, SDMMC2), /* KBCSDMMC2_DAT0:b */
    CONFIG(KB_ROW13, ENABLE, SDMMC2), /* KBCSDMMC2_DAT1:b */
    CONFIG(KB_ROW14, ENABLE, SDMMC2), /* KBCSDMMC2_DAT2:b */
    CONFIG(KB_ROW15, ENABLE, SDMMC2), /* KBCSDMMC2_DAT3:b */
    CONFIG(KB_ROW6, ENABLE, SDMMC2),  /* KBCSDMMC2_DAT4:b */
    CONFIG(KB_ROW7, ENABLE, SDMMC2),  /* KBCSDMMC2_DAT5:b */
    CONFIG(KB_ROW8, ENABLE, SDMMC2),  /* KBCSDMMC2_DAT6:b */
    CONFIG(KB_ROW9, ENABLE, SDMMC2),  /* KBCSDMMC2_DAT7:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 3                  */
    /*------------------------------------------*/
    CONFIG(DAP1_FS, ENABLE, SDMMC2),      /* DAPSDMMC2_CMD:b */
    CONFIG(DAP1_SCLK, ENABLE, SDMMC2),    /* DAPSDMMC2_SCLK:b */
    CONFIG(DAP1_DIN, ENABLE, SDMMC2),     /* DAPSDMMC2_DAT0:b */
    CONFIG(DAP1_DOUT, ENABLE, SDMMC2),    /* DAPSDMMC2_DAT1:b */
    CONFIG(SPDIF_OUT, ENABLE, DAPSDMMC2), /* DAPSDMMC2_DAT2:b */
    CONFIG(SPDIF_IN, ENABLE, DAPSDMMC2),  /* DAPSDMMC2_DAT3:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxSdio3[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(SDMMC3_CMD, ENABLE, SDMMC3),  /* SDMMC3_CMD:b */
    CONFIG(SDMMC3_CLK, ENABLE, SDMMC3),  /* SDMMC3_SCLK:b */
    CONFIG(SDMMC3_DAT0, ENABLE, SDMMC3), /* SDMMC3_DAT0:b */
    CONFIG(SDMMC3_DAT1, ENABLE, SDMMC3), /* SDMMC3_DAT1:b */
    CONFIG(SDMMC3_DAT2, ENABLE, SDMMC3), /* SDMMC3_DAT2:b */
    CONFIG(SDMMC3_DAT3, ENABLE, SDMMC3), /* SDMMC3_DAT3:b */
    CONFIG(SDMMC3_DAT4, ENABLE, SDMMC3), /* SDMMC3_DAT4:b */
    CONFIG(SDMMC3_DAT5, ENABLE, SDMMC3), /* SDMMC3_DAT5:b */
    CONFIG(SDMMC3_DAT6, ENABLE, SDMMC3), /* SDMMC3_DAT6:b */
    CONFIG(SDMMC3_DAT7, ENABLE, SDMMC3), /* SDMMC3_DAT7:b */
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(SDMMC3_CMD, ENABLE, SDMMC3),  /* SDMMC3_CMD:b */
    CONFIG(SDMMC3_CLK, ENABLE, SDMMC3),  /* SDMMC3_SCLK:b */
    CONFIG(SDMMC3_DAT0, ENABLE, SDMMC3), /* SDMMC3_DAT0:b */
    CONFIG(SDMMC3_DAT1, ENABLE, SDMMC3), /* SDMMC3_DAT1:b */
    CONFIG(SDMMC3_DAT2, ENABLE, SDMMC3), /* SDMMC3_DAT2:b */
    CONFIG(SDMMC3_DAT3, ENABLE, SDMMC3), /* SDMMC3_DAT3:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxSdio4[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(SDMMC4_CMD, ENABLE, SDMMC4),  /* SDMMC4_CMD:b */
    CONFIG(SDMMC4_CLK, ENABLE, SDMMC4),  /* SDMMC4_SCLK:b */
    CONFIG(SDMMC4_DAT0, ENABLE, SDMMC4), /* SDMMC4_DAT0:b */
    CONFIG(SDMMC4_DAT1, ENABLE, SDMMC4), /* SDMMC4_DAT1:b */
    CONFIG(SDMMC4_DAT2, ENABLE, SDMMC4), /* SDMMC4_DAT2:b */
    CONFIG(SDMMC4_DAT3, ENABLE, SDMMC4), /* SDMMC4_DAT3:b */
    CONFIG(SDMMC4_DAT4, ENABLE, SDMMC4), /* SDMMC4_DAT4:b */
    CONFIG(SDMMC4_DAT5, ENABLE, SDMMC4), /* SDMMC4_DAT5:b */
    CONFIG(SDMMC4_DAT6, ENABLE, SDMMC4), /* SDMMC4_DAT6:b */
    CONFIG(SDMMC4_DAT7, ENABLE, SDMMC4), /* SDMMC4_DAT7:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(GPIO_PCC1, ENABLE, POPSDMMC4),   /* POPSDMMC4_CMD:b */
    CONFIG(CAM_MCLK, ENABLE, POPSDMMC4),    /* POPSDMMC4_SCLK:b */
    CONFIG(GPIO_PBB0, ENABLE, POPSDMMC4),   /* POPSDMMC4_DAT0:b */
    CONFIG(CAM_I2C_SCL, ENABLE, POPSDMMC4), /* POPSDMMC4_DAT1:b */
    CONFIG(CAM_I2C_SDA, ENABLE, POPSDMMC4), /* POPSDMMC4_DAT2:b */
    CONFIG(GPIO_PBB3, ENABLE, POPSDMMC4),   /* POPSDMMC4_DAT3:b */
    CONFIG(GPIO_PBB4, ENABLE, POPSDMMC4),   /* POPSDMMC4_DAT4:b */
    CONFIG(GPIO_PBB5, ENABLE, POPSDMMC4),   /* POPSDMMC4_DAT5:b */
    CONFIG(GPIO_PBB6, ENABLE, POPSDMMC4),   /* POPSDMMC4_DAT6:b */
    CONFIG(GPIO_PBB7, ENABLE, POPSDMMC4),   /* POPSDMMC4_DAT7:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxSdio[] = {
    &s_T30MuxSdio1[0],
    &s_T30MuxSdio2[0],
    &s_T30MuxSdio3[0],
    &s_T30MuxSdio4[0],
    NULL,
};

static const NvU32 s_T30MuxSpdif1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(UART2_RXD, DISABLE, SPDIF), /* SPDIF_OUT:o */
    CONFIG(UART2_TXD, ENABLE, SPDIF),  /* SPDIF_IN:i */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(SPDIF_OUT, DISABLE, SPDIF), /* SPDIF_OUT:o */
    CONFIG(SPDIF_IN, ENABLE, SPDIF),   /* SPDIF_IN:i */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 3                  */
    /*------------------------------------------*/
    CONFIG(SDMMC3_DAT7, DISABLE, SPDIF), /* SPDIF_OUT:o */
    CONFIG(SDMMC3_DAT6, ENABLE, SPDIF),  /* SPDIF_IN:i */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxSpdif[] = {
    &s_T30MuxSpdif1[0],
    NULL,
};

static const NvU32 s_T30MuxHsi1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(ULPI_DATA0, DISABLE, HSI), /* HSI_TX_DATA:o */
    CONFIG(ULPI_DATA1, ENABLE, HSI),  /* HSI_RX_DATA:i */
    CONFIG(ULPI_DATA2, ENABLE, HSI),  /* HSI_TX_READY:i */
    CONFIG(ULPI_DATA3, DISABLE, HSI), /* HSI_RX_READY:o */
    CONFIG(ULPI_DATA4, ENABLE, HSI),  /* HSI_RX_WAKE:i */
    CONFIG(ULPI_DATA5, DISABLE, HSI), /* HSI_TX_WAKE:o */
    CONFIG(ULPI_DATA6, ENABLE, HSI),  /* HSI_RX_FLAG:i */
    CONFIG(ULPI_DATA7, DISABLE, HSI), /* HSI_TX_FLAG:o */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxHsi[] = {
    &s_T30MuxHsi1[0],
    NULL,
};

static const NvU32 s_T30MuxHdmi1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    //CONFIG(HDMI_RSET, ENABLE, ), /* HDMI_RSET:i */
    //CONFIG(HDMI_PROBE, DISABLE, ), /* HDMI_PROBE:o */
    //CONFIG(HDMI_TXCN, DISABLE, ), /* HDMI_TXCN:o */
    //CONFIG(HDMI_TXCP, DISABLE, ), /* HDMI_TXCP:o */
    //CONFIG(HDMI_TXD0N, DISABLE, ), /* HDMI_TXD0N:o */
    //CONFIG(HDMI_TXD0P, DISABLE, ), /* HDMI_TXD0P:o */
    //CONFIG(HDMI_TXD1N, DISABLE, ), /* HDMI_TXD1N:o */
    //CONFIG(HDMI_TXD1P, DISABLE, ), /* HDMI_TXD1P:o */
    //CONFIG(HDMI_TXD2N, DISABLE, ), /* HDMI_TXD2N:o */
    //CONFIG(HDMI_TXD2P, DISABLE, ), /* HDMI_TXD2P:o */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxHdmi[] = {
    &s_T30MuxHdmi1[0],
    NULL,
};

static const NvU32 s_T30MuxPwm1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(UART3_RTS_N, DISABLE, PWM0), /* PM3_PWM0:o */
    CONFIG(GPIO_PU4, DISABLE, PWM1),    /* PM3_PWM1:o */
    CONFIG(GPIO_PU5, DISABLE, PWM2),    /* PM3_PWM2:o */
    CONFIG(GPIO_PU6, DISABLE, PWM3),    /* PM3_PWM3:o */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(GPIO_PU3, DISABLE, PWM0), /* PM3_PWM0:o */
    CONFIG(GMI_AD9, DISABLE, PWM1),  /* PM3_PWM1:o */
    CONFIG(GMI_AD10, DISABLE, PWM2), /* PM3_PWM2:o */
    CONFIG(GMI_AD11, DISABLE, PWM3), /* PM3_PWM3:o */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 3                  */
    /*------------------------------------------*/
    CONFIG(GMI_AD8, DISABLE, PWM0),     /* PM3_PWM0:o */
    CONFIG(SDMMC3_DAT4, DISABLE, PWM1), /* PM3_PWM1:o */
    CONFIG(SDMMC3_CLK, DISABLE, PWM2),  /* PM3_PWM2:o */
    CONFIG(SDMMC3_CMD, DISABLE, PWM3),  /* PM3_PWM3:o */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 4                  */
    /*------------------------------------------*/
    CONFIG(SDMMC3_DAT5, DISABLE, PWM0), /* PM3_PWM0:o */
    CONFIG(SDMMC3_DAT2, DISABLE, PWM1), /* PM3_PWM1:o */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 5                  */
    /*------------------------------------------*/
    CONFIG(SDMMC3_DAT3, DISABLE, PWM0), /* PM3_PWM0:o */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxPwm[] = {
    &s_T30MuxPwm1[0],
    NULL,
};

static const NvU32 s_T30MuxAta1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxAta[] = {
    &s_T30MuxAta1[0],
    NULL,
};

static const NvU32 s_T30MuxNand1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(GMI_CS2_N, DISABLE, NAND),     /* NAND_CE0:o */
    CONFIG(GMI_CS3_N, DISABLE, NAND),     /* NAND_CE1:o */
    CONFIG(GMI_CS4_N, DISABLE, NAND),     /* NAND_CE2:o */
    CONFIG(GMI_IORDY, DISABLE, NAND),     /* NAND_CE3:o */
    CONFIG(GMI_CS6_N, DISABLE, NAND_ALT), /* NAND_CE4:o */
    CONFIG(GMI_CS7_N, DISABLE, NAND_ALT), /* NAND_CE5:o */
    CONFIG(GMI_CS0_N, DISABLE, NAND),     /* NAND_CE6:o */
    CONFIG(GMI_CS1_N, DISABLE, NAND),     /* NAND_CE7:o */
    CONFIG(GMI_CLK, DISABLE, NAND),       /* NAND_CLE:o */
    CONFIG(GMI_ADV_N, DISABLE, NAND),     /* NAND_ALE:o */
    CONFIG(GMI_WR_N, DISABLE, NAND),      /* NAND_WE:o */
    CONFIG(GMI_OE_N, DISABLE, NAND),      /* NAND_RE:o */
    CONFIG(GMI_WAIT, ENABLE, NAND),       /* NAND_BSY0:i */
    CONFIG(GMI_DQS, ENABLE, NAND),        /* NAND_DQS:b */
    CONFIG(GMI_AD0, ENABLE, NAND),        /* NAND_D0:b */
    CONFIG(GMI_AD1, ENABLE, NAND),        /* NAND_D1:b */
    CONFIG(GMI_AD2, ENABLE, NAND),        /* NAND_D2:b */
    CONFIG(GMI_AD3, ENABLE, NAND),        /* NAND_D3:b */
    CONFIG(GMI_AD4, ENABLE, NAND),        /* NAND_D4:b */
    CONFIG(GMI_AD5, ENABLE, NAND),        /* NAND_D5:b */
    CONFIG(GMI_AD6, ENABLE, NAND),        /* NAND_D6:b */
    CONFIG(GMI_AD7, ENABLE, NAND),        /* NAND_D7:b */
    CONFIG(GMI_AD8, ENABLE, NAND),        /* NAND_D8:b */
    CONFIG(GMI_AD9, ENABLE, NAND),        /* NAND_D9:b */
    CONFIG(GMI_AD10, ENABLE, NAND),       /* NAND_D10:b */
    CONFIG(GMI_AD11, ENABLE, NAND),       /* NAND_D11:b */
    CONFIG(GMI_AD12, ENABLE, NAND),       /* NAND_D12:b */
    CONFIG(GMI_AD13, ENABLE, NAND),       /* NAND_D13:b */
    CONFIG(GMI_AD14, ENABLE, NAND),       /* NAND_D14:b */
    CONFIG(GMI_AD15, ENABLE, NAND),       /* NAND_D15:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(KB_COL5, DISABLE, NAND),       /* KBCNAND_CE0:o */
    CONFIG(KB_COL6, DISABLE, NAND),       /* KBCNAND_CE1:o */
    CONFIG(GMI_WP_N, DISABLE, NAND),      /* NAND_CE5:o */
    CONFIG(SDMMC4_CLK, DISABLE, NAND),    /* NAND_CE6:o */
    CONFIG(GMI_RST_N, DISABLE, NAND_ALT), /* NAND_CLE:o */
    CONFIG(KB_COL1, DISABLE, NAND),       /* KBCNAND_ALE:o */
    CONFIG(KB_COL3, DISABLE, NAND),       /* KBCNAND_WE:o */
    CONFIG(KB_COL4, DISABLE, NAND),       /* KBCNAND_RE:o */
    CONFIG(KB_COL0, ENABLE, NAND),        /* KBCNAND_BSY0:i */
    CONFIG(KB_COL7, ENABLE, NAND),        /* KBCNAND_DQS:b */
    CONFIG(KB_ROW0, ENABLE, NAND),        /* KBCNAND_D0:b */
    CONFIG(KB_ROW1, ENABLE, NAND),        /* KBCNAND_D1:b */
    CONFIG(KB_ROW2, ENABLE, NAND),        /* KBCNAND_D2:b */
    CONFIG(KB_ROW3, ENABLE, NAND),        /* KBCNAND_D3:b */
    CONFIG(KB_ROW4, ENABLE, NAND),        /* KBCNAND_D4:b */
    CONFIG(KB_ROW5, ENABLE, NAND),        /* KBCNAND_D5:b */
    CONFIG(KB_ROW6, ENABLE, NAND),        /* KBCNAND_D6:b */
    CONFIG(KB_ROW7, ENABLE, NAND),        /* KBCNAND_D7:b */
    CONFIG(KB_ROW8, ENABLE, NAND),        /* KBCNAND_D8:b */
    CONFIG(KB_ROW9, ENABLE, NAND),        /* KBCNAND_D9:b */
    CONFIG(KB_ROW10, ENABLE, NAND),       /* KBCNAND_D10:b */
    CONFIG(KB_ROW11, ENABLE, NAND),       /* KBCNAND_D11:b */
    CONFIG(KB_ROW12, ENABLE, NAND),       /* KBCNAND_D12:b */
    CONFIG(KB_ROW13, ENABLE, NAND),       /* KBCNAND_D13:b */
    CONFIG(KB_ROW14, ENABLE, NAND),       /* KBCNAND_D14:b */
    CONFIG(KB_ROW15, ENABLE, NAND),       /* KBCNAND_D15:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 3                  */
    /*------------------------------------------*/
    CONFIG(SDMMC4_CMD, DISABLE, NAND), /* NAND_CLE:o */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 4                  */
    /*------------------------------------------*/
    CONFIG(KB_COL2, DISABLE, NAND), /* KBCNAND_CLE:o */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxNand[] = {
    &s_T30MuxNand1[0],
    NULL,
};

static const NvU32 s_T30MuxDap1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(DAP1_FS, ENABLE, I2S0),   /* I2S0_LRCK:b */
    CONFIG(DAP1_DIN, ENABLE, I2S0),  /* I2S0_SDATA_IN:i */
    CONFIG(DAP1_DOUT, ENABLE, I2S0), /* I2S0_SDATA_OUT:b */
    CONFIG(DAP1_SCLK, ENABLE, I2S0), /* I2S0_SCLK:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxDap2[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(DAP2_FS, ENABLE, I2S1),   /* I2S1_LRCK:b */
    CONFIG(DAP2_DIN, ENABLE, I2S1),  /* I2S1_SDATA_IN:i */
    CONFIG(DAP2_DOUT, ENABLE, I2S1), /* I2S1_SDATA_OUT:b */
    CONFIG(DAP2_SCLK, ENABLE, I2S1), /* I2S1_SCLK:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxDap3[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(DAP3_FS, ENABLE, I2S2),   /* I2S2_LRCK:b */
    CONFIG(DAP3_DIN, ENABLE, I2S2),  /* I2S2_SDATA_IN:i */
    CONFIG(DAP3_DOUT, ENABLE, I2S2), /* I2S2_SDATA_OUT:b */
    CONFIG(DAP3_SCLK, ENABLE, I2S2), /* I2S2_SCLK:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxDap4[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(DAP4_FS, ENABLE, I2S3),   /* I2S3_LRCK:b */
    CONFIG(DAP4_DIN, ENABLE, I2S3),  /* I2S3_SDATA_IN:i */
    CONFIG(DAP4_DOUT, ENABLE, I2S3), /* I2S3_SDATA_OUT:b */
    CONFIG(DAP4_SCLK, ENABLE, I2S3), /* I2S3_SCLK:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxDap5[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(SDMMC4_DAT4, ENABLE, I2S4), /* I2S4_LRCK:b */
    CONFIG(SDMMC4_DAT5, ENABLE, I2S4), /* I2S4_SDATA_IN:i */
    CONFIG(SDMMC4_DAT6, ENABLE, I2S4), /* I2S4_SDATA_OUT:b */
    CONFIG(SDMMC4_DAT7, ENABLE, I2S4), /* I2S4_SCLK:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(GPIO_PCC1, ENABLE, I2S4), /* POP_I2S4_LRCK:b */
    CONFIG(GPIO_PBB0, ENABLE, I2S4), /* POP_I2S4_SDATA_IN:i */
    CONFIG(GPIO_PBB7, ENABLE, I2S4), /* POP_I2S4_SDATA_OUT:b */
    CONFIG(GPIO_PCC2, ENABLE, I2S4), /* POP_I2S4_SCLK:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxDap[] = {
    &s_T30MuxDap1[0],
    &s_T30MuxDap2[0],
    &s_T30MuxDap3[0],
    &s_T30MuxDap4[0],
    &s_T30MuxDap5[0],
    NULL,
};

static const NvU32 s_T30MuxKbd1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(KB_ROW0, ENABLE, KBC),  /* kbc0:b */
    CONFIG(KB_ROW1, ENABLE, KBC),  /* kbc1:b */
    CONFIG(KB_ROW2, ENABLE, KBC),  /* kbc2:b */
    CONFIG(KB_ROW3, ENABLE, KBC),  /* kbc3:b */
    CONFIG(KB_ROW4, ENABLE, KBC),  /* kbc4:b */
    CONFIG(KB_ROW5, ENABLE, KBC),  /* kbc5:b */
    CONFIG(KB_ROW6, ENABLE, KBC),  /* kbc6:b */
    CONFIG(KB_ROW7, ENABLE, KBC),  /* kbc7:b */
    CONFIG(KB_ROW8, ENABLE, KBC),  /* kbc8:b */
    CONFIG(KB_ROW9, ENABLE, KBC),  /* kbc9:b */
    CONFIG(KB_ROW10, ENABLE, KBC), /* kbc10:b */
    CONFIG(KB_ROW11, ENABLE, KBC), /* kbc11:b */
    CONFIG(KB_ROW12, ENABLE, KBC), /* kbc12:b */
    CONFIG(KB_ROW13, ENABLE, KBC), /* kbc13:b */
    CONFIG(KB_ROW14, ENABLE, KBC), /* kbc14:b */
    CONFIG(KB_ROW15, ENABLE, KBC), /* kbc15:b */
    CONFIG(KB_COL0, ENABLE, KBC),  /* kbc16:b */
    CONFIG(KB_COL1, ENABLE, KBC),  /* kbc17:b */
    CONFIG(KB_COL2, ENABLE, KBC),  /* kbc18:b */
    CONFIG(KB_COL3, ENABLE, KBC),  /* kbc19:b */
    CONFIG(KB_COL4, ENABLE, KBC),  /* kbc20:b */
    CONFIG(KB_COL5, ENABLE, KBC),  /* kbc21:b */
    CONFIG(KB_COL6, ENABLE, KBC),  /* kbc22:b */
    CONFIG(KB_COL7, ENABLE, KBC),  /* kbc23:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxKbd[] = {
    &s_T30MuxKbd1[0],
    NULL,
};

static const NvU32 s_T30MuxHdcp1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(LCD_PWR2, ENABLE, HDMI),  /* hdcp_sck:b */
    CONFIG(LCD_SDOUT, ENABLE, HDMI), /* hdcp_sda:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(LCD_WR_N, ENABLE, HDMI), /* hdcp_sck:b */
    CONFIG(LCD_PWR0, ENABLE, HDMI), /* hdcp_sda:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 3                  */
    /*------------------------------------------*/
    CONFIG(LCD_SCK, ENABLE, HDMI),      /* hdcp_sck:b */
    CONFIG(GEN2_I2C_SDA, ENABLE, HDMI), /* hdcp_sda:b */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 4                  */
    /*------------------------------------------*/
    CONFIG(GEN2_I2C_SCL, ENABLE, HDMI), /* hdcp_sck:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxHdcp[] = {
    &s_T30MuxHdcp1[0],
    NULL,
};

static const NvU32 s_T30MuxSnor1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(UART2_RTS_N, DISABLE, GMI),   /* GMI_A0:o */
    CONFIG(UART2_CTS_N, DISABLE, GMI),   /* GMI_A1:o */
    CONFIG(UART3_TXD, DISABLE, GMI),     /* GMI_A2:o */
    CONFIG(UART3_RXD, DISABLE, GMI),     /* GMI_A3:o */
    CONFIG(UART3_RTS_N, DISABLE, GMI),   /* GMI_A4:o */
    CONFIG(UART3_CTS_N, DISABLE, GMI),   /* GMI_A5:o */
    CONFIG(GPIO_PU0, DISABLE, GMI),      /* GMI_A6:o */
    CONFIG(GPIO_PU1, DISABLE, GMI),      /* GMI_A7:o */
    CONFIG(GPIO_PU2, DISABLE, GMI),      /* GMI_A8:o */
    CONFIG(GPIO_PU3, DISABLE, GMI),      /* GMI_A9:o */
    CONFIG(GPIO_PU4, DISABLE, GMI),      /* GMI_A10:o */
    CONFIG(GPIO_PU5, DISABLE, GMI),      /* GMI_A11:o */
    CONFIG(GPIO_PU6, DISABLE, GMI),      /* GMI_A12:o */
    CONFIG(DAP4_FS, DISABLE, GMI),       /* GMI_A13:o */
    CONFIG(DAP4_DIN, DISABLE, GMI),      /* GMI_A14:o */
    CONFIG(DAP4_DOUT, DISABLE, GMI),     /* GMI_A15:o */
    CONFIG(DAP4_SCLK, DISABLE, GMI),     /* GMI_A16:o */
    CONFIG(DAP2_FS, DISABLE, GMI),       /* GMI_A17:o */
    CONFIG(DAP2_SCLK, DISABLE, GMI),     /* GMI_A18:o */
    CONFIG(DAP2_DIN, DISABLE, GMI),      /* GMI_A19:o */
    CONFIG(DAP2_DOUT, DISABLE, GMI),     /* GMI_A20:o */
    CONFIG(SPI2_MOSI, DISABLE, GMI),     /* GMI_A21:o */
    CONFIG(SPI2_MISO, DISABLE, GMI),     /* GMI_A22:o */
    CONFIG(SPI2_SCK, DISABLE, GMI),      /* GMI_A23:o */
    CONFIG(SPI2_CS0_N, DISABLE, GMI),    /* GMI_A24:o */
    CONFIG(SPI1_MOSI, DISABLE, GMI),     /* GMI_A25:o */
    CONFIG(SPI1_SCK, DISABLE, GMI),      /* GMI_A26:o */
    CONFIG(SPI1_CS0_N, DISABLE, GMI),    /* GMI_A27:o */
    CONFIG(GMI_AD0, ENABLE, GMI),        /* GMI_AD00:b */
    CONFIG(GMI_AD1, ENABLE, GMI),        /* GMI_AD01:b */
    CONFIG(GMI_AD2, ENABLE, GMI),        /* GMI_AD02:b */
    CONFIG(GMI_AD3, ENABLE, GMI),        /* GMI_AD03:b */
    CONFIG(GMI_AD4, ENABLE, GMI),        /* GMI_AD04:b */
    CONFIG(GMI_AD5, ENABLE, GMI),        /* GMI_AD05:b */
    CONFIG(GMI_AD6, ENABLE, GMI),        /* GMI_AD06:b */
    CONFIG(GMI_AD7, ENABLE, GMI),        /* GMI_AD07:b */
    CONFIG(GMI_AD8, ENABLE, GMI),        /* GMI_AD08:b */
    CONFIG(GMI_AD9, ENABLE, GMI),        /* GMI_AD09:b */
    CONFIG(GMI_AD10, ENABLE, GMI),       /* GMI_AD10:b */
    CONFIG(GMI_AD11, ENABLE, GMI),       /* GMI_AD11:b */
    CONFIG(GMI_AD12, ENABLE, GMI),       /* GMI_AD12:b */
    CONFIG(GMI_AD13, ENABLE, GMI),       /* GMI_AD13:b */
    CONFIG(GMI_AD14, ENABLE, GMI),       /* GMI_AD14:b */
    CONFIG(GMI_AD15, ENABLE, GMI),       /* GMI_AD15:b */
    CONFIG(GMI_A16, ENABLE, GMI),        /* GMI_AD16:b */
    CONFIG(GMI_A17, ENABLE, GMI),        /* GMI_AD17:b */
    CONFIG(GMI_A18, ENABLE, GMI),        /* GMI_AD18:b */
    CONFIG(GMI_A19, ENABLE, GMI),        /* GMI_AD19:b */
    CONFIG(SDMMC4_DAT0, ENABLE, GMI),    /* GMI_AD20:b */
    CONFIG(SDMMC4_DAT1, ENABLE, GMI),    /* GMI_AD21:b */
    CONFIG(SDMMC4_DAT2, ENABLE, GMI),    /* GMI_AD22:b */
    CONFIG(SDMMC4_DAT3, ENABLE, GMI),    /* GMI_AD23:b */
    CONFIG(SDMMC4_DAT4, ENABLE, GMI),    /* GMI_AD24:b */
    CONFIG(SDMMC4_DAT5, ENABLE, GMI),    /* GMI_AD25:b */
    CONFIG(SDMMC4_DAT6, ENABLE, GMI),    /* GMI_AD26:b */
    CONFIG(SDMMC4_DAT7, ENABLE, GMI),    /* GMI_AD27:b */
    CONFIG(DAP1_FS, ENABLE, GMI),        /* GMI_D28:b */
    CONFIG(DAP1_DIN, ENABLE, GMI),       /* GMI_D29:b */
    CONFIG(DAP1_DOUT, ENABLE, GMI),      /* GMI_D30:b */
    CONFIG(DAP1_SCLK, ENABLE, GMI),      /* GMI_D31:b */
    CONFIG(GMI_CS0_N, DISABLE, GMI),     /* GMI_CS0:o */
    CONFIG(GMI_CS1_N, DISABLE, GMI),     /* GMI_CS1:o */
    CONFIG(GMI_CS2_N, DISABLE, GMI),     /* GMI_CS2:o */
    CONFIG(GMI_CS3_N, DISABLE, GMI),     /* GMI_CS3:o */
    CONFIG(GMI_CS4_N, DISABLE, GMI),     /* GMI_CS4:o */
    CONFIG(SDMMC4_CLK, DISABLE, GMI),    /* GMI_CS5:o */
    CONFIG(GMI_CS6_N, DISABLE, GMI),     /* GMI_CS6:o */
    CONFIG(GMI_CS7_N, DISABLE, GMI),     /* GMI_CS7:o */
    CONFIG(GMI_CLK, DISABLE, GMI),       /* GMI_CLK:o */
    CONFIG(GMI_ADV_N, DISABLE, GMI),     /* GMI_ADV_N:o */
    CONFIG(SDMMC4_CMD, DISABLE, GMI),    /* GMI_DPD:o */
    CONFIG(GMI_OE_N, DISABLE, GMI),      /* GMI_HIOR:o */
    CONFIG(GMI_WR_N, DISABLE, GMI),      /* GMI_HIOW:o */
    CONFIG(GMI_CS3_N, ENABLE, GMI_ALT),  /* GMI_INT1:i */
    CONFIG(GMI_IORDY, ENABLE, GMI),      /* GMI_IORDY:i */
    CONFIG(GMI_CS7_N, DISABLE, GMI_ALT), /* GMI_RST_N:o */
    CONFIG(GMI_WAIT, ENABLE, GMI),       /* GMI_WAIT:i */
    CONFIG(GMI_WP_N, DISABLE, GMI),      /* GMI_WP_N:o */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(GEN2_I2C_SCL, DISABLE, GMI), /* GMI_CS6:o */
    CONFIG(GEN2_I2C_SDA, DISABLE, GMI), /* GMI_CS7:o */
    CONFIG(GMI_WP_N, ENABLE, GMI_ALT),  /* GMI_INT1:i */
    CONFIG(GMI_RST_N, DISABLE, GMI),    /* GMI_RST_N:o */
    CONFIG(GMI_DQS, ENABLE, GMI),       /* GMI_WAIT:i */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 3                  */
    /*------------------------------------------*/
    CONFIG(UART2_RTS_N, DISABLE, GMI),   /* GMI_A0:o */
    CONFIG(UART2_CTS_N, DISABLE, GMI),   /* GMI_A1:o */
    CONFIG(UART3_TXD, DISABLE, GMI),     /* GMI_A2:o */
    CONFIG(UART3_RXD, DISABLE, GMI),     /* GMI_A3:o */
    CONFIG(UART3_RTS_N, DISABLE, GMI),   /* GMI_A4:o */
    CONFIG(UART3_CTS_N, DISABLE, GMI),   /* GMI_A5:o */
    CONFIG(GPIO_PU0, DISABLE, GMI),      /* GMI_A6:o */
    CONFIG(GPIO_PU1, DISABLE, GMI),      /* GMI_A7:o */
    CONFIG(GPIO_PU2, DISABLE, GMI),      /* GMI_A8:o */
    CONFIG(GPIO_PU3, DISABLE, GMI),      /* GMI_A9:o */
    CONFIG(GPIO_PU4, DISABLE, GMI),      /* GMI_A10:o */
    CONFIG(GPIO_PU5, DISABLE, GMI),      /* GMI_A11:o */
    CONFIG(GPIO_PU6, DISABLE, GMI),      /* GMI_A12:o */
    CONFIG(DAP4_FS, DISABLE, GMI),       /* GMI_A13:o */
    CONFIG(DAP4_DIN, DISABLE, GMI),      /* GMI_A14:o */
    CONFIG(DAP4_DOUT, DISABLE, GMI),     /* GMI_A15:o */
    CONFIG(GMI_CS0_N, DISABLE, GMI),     /* GMI_CS0:o */
    CONFIG(GMI_CS1_N, DISABLE, GMI),     /* GMI_CS1:o */
    CONFIG(GMI_CS2_N, DISABLE, GMI),     /* GMI_CS2:o */
    CONFIG(GMI_CS3_N, DISABLE, GMI),     /* GMI_CS3:o */
    CONFIG(GMI_CS4_N, DISABLE, GMI),     /* GMI_CS4:o */
    CONFIG(SDMMC4_CLK, DISABLE, GMI),    /* GMI_CS5:o */
    CONFIG(GMI_CS6_N, DISABLE, GMI),     /* GMI_CS6:o */
    CONFIG(GMI_CS7_N, DISABLE, GMI),     /* GMI_CS7:o */
    CONFIG(GMI_CLK, DISABLE, GMI),       /* GMI_CLK:o */
    CONFIG(GMI_ADV_N, DISABLE, GMI),     /* GMI_ADV_N:o */
    CONFIG(SDMMC4_CMD, DISABLE, GMI),    /* GMI_DPD:o */
    CONFIG(GMI_OE_N, DISABLE, GMI),      /* GMI_HIOR:o */
    CONFIG(GMI_WR_N, DISABLE, GMI),      /* GMI_HIOW:o */
    CONFIG(GMI_CS3_N, ENABLE, GMI_ALT),  /* GMI_INT1:i */
    CONFIG(GMI_A16, ENABLE, GMI_ALT),    /* GMI_INT2:i */
    CONFIG(GMI_IORDY, ENABLE, GMI),      /* GMI_IORDY:i */
    CONFIG(GMI_CS7_N, DISABLE, GMI_ALT), /* GMI_RST_N:o */
    CONFIG(GMI_WAIT, ENABLE, GMI),       /* GMI_WAIT:i */
    CONFIG(GMI_WP_N, DISABLE, GMI),      /* GMI_WP_N:o */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 4                  */
    /*------------------------------------------*/
    CONFIG(GEN2_I2C_SCL, DISABLE, GMI), /* GMI_CS6:o */
    CONFIG(GEN2_I2C_SDA, DISABLE, GMI), /* GMI_CS7:o */
    CONFIG(GMI_WP_N, ENABLE, GMI_ALT),  /* GMI_INT1:i */
    CONFIG(GMI_RST_N, DISABLE, GMI),    /* GMI_RST_N:o */
    CONFIG(GMI_DQS, ENABLE, GMI),       /* GMI_WAIT:i */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 5                  */
    /*------------------------------------------*/
    CONFIG(UART2_RTS_N, DISABLE, GMI),   /* GMI_A0:o */
    CONFIG(UART2_CTS_N, DISABLE, GMI),   /* GMI_A1:o */
    CONFIG(UART3_TXD, DISABLE, GMI),     /* GMI_A2:o */
    CONFIG(UART3_RXD, DISABLE, GMI),     /* GMI_A3:o */
    CONFIG(UART3_RTS_N, DISABLE, GMI),   /* GMI_A4:o */
    CONFIG(UART3_CTS_N, DISABLE, GMI),   /* GMI_A5:o */
    CONFIG(GPIO_PU0, DISABLE, GMI),      /* GMI_A6:o */
    CONFIG(GPIO_PU1, DISABLE, GMI),      /* GMI_A7:o */
    CONFIG(GPIO_PU2, DISABLE, GMI),      /* GMI_A8:o */
    CONFIG(GPIO_PU3, DISABLE, GMI),      /* GMI_A9:o */
    CONFIG(GPIO_PU4, DISABLE, GMI),      /* GMI_A10:o */
    CONFIG(GPIO_PU5, DISABLE, GMI),      /* GMI_A11:o */
    CONFIG(GPIO_PU6, DISABLE, GMI),      /* GMI_A12:o */
    CONFIG(DAP4_FS, DISABLE, GMI),       /* GMI_A13:o */
    CONFIG(DAP4_DIN, DISABLE, GMI),      /* GMI_A14:o */
    CONFIG(DAP4_DOUT, DISABLE, GMI),     /* GMI_A15:o */
    CONFIG(DAP4_SCLK, DISABLE, GMI),     /* GMI_A16:o */
    CONFIG(DAP2_FS, DISABLE, GMI),       /* GMI_A17:o */
    CONFIG(DAP2_SCLK, DISABLE, GMI),     /* GMI_A18:o */
    CONFIG(DAP2_DIN, DISABLE, GMI),      /* GMI_A19:o */
    CONFIG(DAP2_DOUT, DISABLE, GMI),     /* GMI_A20:o */
    CONFIG(SPI2_MOSI, DISABLE, GMI),     /* GMI_A21:o */
    CONFIG(SPI2_MISO, DISABLE, GMI),     /* GMI_A22:o */
    CONFIG(SPI2_SCK, DISABLE, GMI),      /* GMI_A23:o */
    CONFIG(SPI2_CS0_N, DISABLE, GMI),    /* GMI_A24:o */
    CONFIG(SPI1_MOSI, DISABLE, GMI),     /* GMI_A25:o */
   // CONFIG(SPI1_SCK, DISABLE, GMI),    /* GMI_A26:o */
   // CONFIG(SPI1_CS0_N, DISABLE, GMI),  /* GMI_A27:o */
    CONFIG(GMI_AD0, ENABLE, GMI),        /* GMI_AD00:b */
    CONFIG(GMI_AD1, ENABLE, GMI),        /* GMI_AD01:b */
    CONFIG(GMI_AD2, ENABLE, GMI),        /* GMI_AD02:b */
    CONFIG(GMI_AD3, ENABLE, GMI),        /* GMI_AD03:b */
    CONFIG(GMI_AD4, ENABLE, GMI),        /* GMI_AD04:b */
    CONFIG(GMI_AD5, ENABLE, GMI),        /* GMI_AD05:b */
    CONFIG(GMI_AD6, ENABLE, GMI),        /* GMI_AD06:b */
    CONFIG(GMI_AD7, ENABLE, GMI),        /* GMI_AD07:b */
    CONFIG(GMI_AD8, ENABLE, GMI),        /* GMI_AD08:b */
    CONFIG(GMI_AD9, ENABLE, GMI),        /* GMI_AD09:b */
    CONFIG(GMI_AD10, ENABLE, GMI),       /* GMI_AD10:b */
    CONFIG(GMI_AD11, ENABLE, GMI),       /* GMI_AD11:b */
    CONFIG(GMI_AD12, ENABLE, GMI),       /* GMI_AD12:b */
    CONFIG(GMI_AD13, ENABLE, GMI),       /* GMI_AD13:b */
    CONFIG(GMI_AD14, ENABLE, GMI),       /* GMI_AD14:b */
    CONFIG(GMI_AD15, ENABLE, GMI),       /* GMI_AD15:b */
    CONFIG(GMI_CS0_N, DISABLE, GMI),     /* GMI_CS0:o */
    //CONFIG(GMI_CLK, DISABLE, GMI),       /* GMI_CLK:o */
    //CONFIG(GMI_ADV_N, DISABLE, GMI),     /* GMI_ADV_N:o */
    CONFIG(GMI_OE_N, DISABLE, GMI),      /* GMI_HIOR:o */
    CONFIG(GMI_WR_N, DISABLE, GMI),      /* GMI_HIOW:o */
    CONFIG(GMI_WAIT, ENABLE, GMI),       /* GMI_WAIT:i */
    CONFIG(GMI_WP_N, DISABLE, GMI),      /* GMI_WP_N:o */
    CONFIG(GMI_RST_N, DISABLE, GMI),
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 6                  */
    /*------------------------------------------*/
    CONFIG(SDMMC4_DAT0, ENABLE, GMI),
    CONFIG(SDMMC4_DAT1, ENABLE, GMI),
    CONFIG(SDMMC4_DAT2, ENABLE, GMI),
    CONFIG(SDMMC4_DAT3, ENABLE, GMI),
    CONFIG(SDMMC4_DAT4, ENABLE, GMI),
    CONFIG(SDMMC4_DAT5, ENABLE, GMI),
    CONFIG(SDMMC4_DAT6, ENABLE, GMI),
    CONFIG(GMI_A16, ENABLE, GMI),
    CONFIG(GMI_A17, ENABLE, GMI),
    CONFIG(GMI_A18, ENABLE, GMI),
    CONFIG(GMI_A19, ENABLE, GMI),
    CONFIG(GMI_AD0, ENABLE, GMI),
    CONFIG(GMI_AD1, ENABLE, GMI),
    CONFIG(GMI_AD2, ENABLE, GMI),
    CONFIG(GMI_AD3, ENABLE, GMI),
    CONFIG(GMI_AD4, ENABLE, GMI),
    CONFIG(GMI_AD5, ENABLE, GMI),
    CONFIG(GMI_AD6, ENABLE, GMI),
    CONFIG(GMI_AD7, ENABLE, GMI),
    CONFIG(GMI_AD8, ENABLE, GMI),
    CONFIG(GMI_AD9, ENABLE, GMI),
    CONFIG(GMI_AD10, ENABLE, GMI),
    CONFIG(GMI_AD11, ENABLE, GMI),
    CONFIG(GMI_AD12, ENABLE, GMI),
    CONFIG(GMI_AD13, ENABLE, GMI),
    CONFIG(GMI_AD14, ENABLE, GMI),
    CONFIG(GMI_AD15, ENABLE, GMI),
    CONFIG(GMI_ADV_N, DISABLE, GMI),
    CONFIG(GMI_CLK, DISABLE, GMI),
    CONFIG(GMI_CS0_N, DISABLE, GMI),
    CONFIG(GMI_OE_N, DISABLE, GMI),
    CONFIG(GMI_RST_N, DISABLE, GMI),
    CONFIG(GMI_WAIT, DISABLE, GMI),
    CONFIG(GMI_WP_N, DISABLE, GMI),
    CONFIG(GMI_WR_N, DISABLE, GMI),
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxSnor[] = {
    &s_T30MuxSnor1[0],
    NULL,
};

static const NvU32 s_T30MuxMio1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(KB_ROW11, DISABLE, MIO), /* MIO_MA00:o */
    CONFIG(KB_COL6, DISABLE, MIO),  /* MIO_WR_N:o */
    CONFIG(KB_COL7, DISABLE, MIO),  /* MIO_RD_N:o */
    CONFIG(KB_ROW10, DISABLE, MIO), /* MIO_CS1_N:o */
    CONFIG(KB_ROW12, ENABLE, MIO),  /* MIO_MD0:b */
    CONFIG(KB_ROW13, ENABLE, MIO),  /* MIO_MD1:b */
    CONFIG(KB_ROW14, ENABLE, MIO),  /* MIO_MD2:b */
    CONFIG(KB_ROW15, ENABLE, MIO),  /* MIO_MD3:b */
    CONFIG(KB_ROW6, ENABLE, MIO),   /* MIO_MD4:b */
    CONFIG(KB_ROW7, ENABLE, MIO),   /* MIO_MD5:b */
    CONFIG(KB_ROW8, ENABLE, MIO),   /* MIO_MD6:b */
    CONFIG(KB_ROW9, ENABLE, MIO),   /* MIO_MD7:b */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxMio[] = {
    &s_T30MuxMio1[0],
    NULL,
};

static const NvU32 s_T30MuxExtClock1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/

    CONFIG(CLK1_REQ, ENABLE, DAP1), /* dap mlk1 req */
    CONFIG(CLK1_OUT, ENABLE, EXTPERIPH1),  /* dap_mclk3 */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxExtClock2[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(CLK2_REQ, ENABLE, DAP), /* dap mlk2 req */
    CONFIG(CLK2_OUT, ENABLE, EXTPERIPH2),  /* dap_mclk3 */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxExtClock3[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(CLK3_REQ, ENABLE, DEV3), /* dap mlk3 req */
    CONFIG(CLK3_OUT, ENABLE, EXTPERIPH3),  /* dap_mclk3 */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxExtClock[] = {
    &s_T30MuxExtClock1[0],
    &s_T30MuxExtClock2[0],
    &s_T30MuxExtClock3[0],
    NULL,
};

static const NvU32 s_T30MuxVi1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxVi[] = {
    &s_T30MuxVi1[0],
    NULL,
};

const NvU32 s_T30MuxBacklightDisplay1Pwm0[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIGEND(),

    MODULEDONE(),
};

const NvU32 s_T30MuxBacklightDisplay1Pwm1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIGEND(),

    MODULEDONE(),
};

const NvU32 s_T30MuxBacklightDisplay2Pwm0[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIGEND(),

    MODULEDONE(),
};

const NvU32 s_T30MuxBacklightDisplay2Pwm1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIGEND(),

    MODULEDONE(),
};

const NvU32* s_T30MuxBacklight[] = {
    &s_T30MuxBacklightDisplay1Pwm0[0],
    &s_T30MuxBacklightDisplay1Pwm1[0],
    &s_T30MuxBacklightDisplay2Pwm0[0],
    &s_T30MuxBacklightDisplay2Pwm1[0],
    NULL,
};

static const NvU32 s_T30MuxDisplay1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(LCD_D0, ENABLE, DISPLAYA),
    CONFIG(LCD_D1, ENABLE, DISPLAYA),
    CONFIG(LCD_D2, ENABLE, DISPLAYA),
    CONFIG(LCD_D3, ENABLE, DISPLAYA),
    CONFIG(LCD_D4, ENABLE, DISPLAYA),
    CONFIG(LCD_D5, ENABLE, DISPLAYA),
    CONFIG(LCD_D6, ENABLE, DISPLAYA),
    CONFIG(LCD_D7, ENABLE, DISPLAYA),
    CONFIG(LCD_D8, ENABLE, DISPLAYA),
    CONFIG(LCD_D9, ENABLE, DISPLAYA),
    CONFIG(LCD_D10, ENABLE, DISPLAYA),
    CONFIG(LCD_D11, ENABLE, DISPLAYA),
    CONFIG(LCD_D12, ENABLE, DISPLAYA),
    CONFIG(LCD_D13, ENABLE, DISPLAYA),
    CONFIG(LCD_D14, ENABLE, DISPLAYA),
    CONFIG(LCD_D15, ENABLE, DISPLAYA),
    CONFIG(LCD_D16, ENABLE, DISPLAYA),
    CONFIG(LCD_D17, ENABLE, DISPLAYA),
    CONFIG(LCD_D18, ENABLE, DISPLAYA),
    CONFIG(LCD_D19, ENABLE, DISPLAYA),
    CONFIG(LCD_D20, ENABLE, DISPLAYA),
    CONFIG(LCD_D21, ENABLE, DISPLAYA),
    CONFIG(LCD_D22, ENABLE, DISPLAYA),
    CONFIG(LCD_D23, ENABLE, DISPLAYA),
    CONFIG(LCD_VSYNC, ENABLE, DISPLAYA),
    CONFIG(LCD_HSYNC, ENABLE, DISPLAYA),
    CONFIG(LCD_PCLK, ENABLE, DISPLAYA),
    CONFIG(LCD_DE, ENABLE, DISPLAYA),
    CONFIG(LCD_WR_N, ENABLE, DISPLAYA),
    CONFIG(LCD_M1, ENABLE, DISPLAYA),
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(LCD_D10, ENABLE, DISPLAYA),
    CONFIG(LCD_D11, ENABLE, DISPLAYA),
    CONFIG(LCD_D12, ENABLE, DISPLAYA),
    CONFIG(LCD_D13, ENABLE, DISPLAYA),
    CONFIG(LCD_D14, ENABLE, DISPLAYA),
    CONFIG(LCD_D15, ENABLE, DISPLAYA),
    CONFIG(LCD_D16, ENABLE, DISPLAYA),
    CONFIG(LCD_D17, ENABLE, DISPLAYA),
    CONFIG(LCD_D18, ENABLE, DISPLAYA),
    CONFIG(LCD_D20, ENABLE, DISPLAYA),
    CONFIG(LCD_WR_N, ENABLE, DISPLAYA),
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 s_T30MuxDisplay2[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(LCD_D0, ENABLE, DISPLAYB),
    CONFIG(LCD_D1, ENABLE, DISPLAYB),
    CONFIG(LCD_D2, ENABLE, DISPLAYB),
    CONFIG(LCD_D3, ENABLE, DISPLAYB),
    CONFIG(LCD_D4, ENABLE, DISPLAYB),
    CONFIG(LCD_D5, ENABLE, DISPLAYB),
    CONFIG(LCD_D6, ENABLE, DISPLAYB),
    CONFIG(LCD_D7, ENABLE, DISPLAYB),
    CONFIG(LCD_D8, ENABLE, DISPLAYB),
    CONFIG(LCD_D9, ENABLE, DISPLAYB),
    CONFIG(LCD_D10, ENABLE, DISPLAYB),
    CONFIG(LCD_D11, ENABLE, DISPLAYB),
    CONFIG(LCD_D12, ENABLE, DISPLAYB),
    CONFIG(LCD_D13, ENABLE, DISPLAYB),
    CONFIG(LCD_D14, ENABLE, DISPLAYB),
    CONFIG(LCD_D15, ENABLE, DISPLAYB),
    CONFIG(LCD_D16, ENABLE, DISPLAYB),
    CONFIG(LCD_D17, ENABLE, DISPLAYB),
    CONFIG(LCD_D18, ENABLE, DISPLAYB),
    CONFIG(LCD_D19, ENABLE, DISPLAYB),
    CONFIG(LCD_D20, ENABLE, DISPLAYB),
    CONFIG(LCD_D21, ENABLE, DISPLAYB),
    CONFIG(LCD_D22, ENABLE, DISPLAYB),
    CONFIG(LCD_D23, ENABLE, DISPLAYB),
    CONFIG(LCD_VSYNC, ENABLE, DISPLAYB),
    CONFIG(LCD_HSYNC, ENABLE, DISPLAYB),
    CONFIG(LCD_PCLK, ENABLE, DISPLAYB),
    CONFIG(LCD_DE, ENABLE, DISPLAYB),
    CONFIG(LCD_WR_N, ENABLE, DISPLAYB),
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(LCD_D0, ENABLE, DISPLAYB),
    CONFIG(LCD_D1, ENABLE, DISPLAYB),
    CONFIG(LCD_D2, ENABLE, DISPLAYB),
    CONFIG(LCD_D3, ENABLE, DISPLAYB),
    CONFIG(LCD_D4, ENABLE, DISPLAYB),
    CONFIG(LCD_D5, ENABLE, DISPLAYB),
    CONFIG(LCD_D6, ENABLE, DISPLAYB),
    CONFIG(LCD_D7, ENABLE, DISPLAYB),
    CONFIG(LCD_VSYNC, ENABLE, DISPLAYB),
    CONFIG(LCD_HSYNC, ENABLE, DISPLAYB),
    CONFIG(LCD_PCLK, ENABLE, DISPLAYB),
    CONFIG(LCD_DE, ENABLE, DISPLAYB),
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32* s_T30MuxDisplay[] = {
    &s_T30MuxDisplay1[0],
    &s_T30MuxDisplay2[0],
    NULL
};

static const NvU32 s_T30MuxCrt1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(CRT_VSYNC, DISABLE, CRT), /* CRT_VSYNC:o */
    CONFIG(CRT_HSYNC, DISABLE, CRT), /* CRT_HSYNC:o */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxCrt[] = {
    &s_T30MuxCrt1[0],
    NULL,
};

#if 0
static const NvU32 s_T30MuxTvo1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxTvo[] = {
    &s_T30MuxTvo1[0],
    NULL,
};
#endif

static const NvU32 s_T30MuxEtm1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(KB_ROW4, DISABLE, TRACE), /* traceclk:o */
    CONFIG(KB_ROW5, DISABLE, TRACE), /* tracectl:o */
    CONFIG(KB_COL0, DISABLE, TRACE), /* tracedata0:o */
    CONFIG(KB_COL1, DISABLE, TRACE), /* tracedata1:o */
    CONFIG(KB_COL2, DISABLE, TRACE), /* tracedata2:o */
    CONFIG(KB_COL3, DISABLE, TRACE), /* tracedata3:o */
    CONFIG(KB_COL4, DISABLE, TRACE), /* tracedata4:o */
    CONFIG(KB_COL5, DISABLE, TRACE), /* tracedata5:o */
    CONFIG(KB_COL6, DISABLE, TRACE), /* tracedata6:o */
    CONFIG(KB_COL7, DISABLE, TRACE), /* tracedata7:o */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxEtm[] = {
    &s_T30MuxEtm1[0],
    NULL,
};

static const NvU32 s_T30MuxOwr1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(OWR, ENABLE, OWR),       /* OWR:b */
    CONFIG(GPIO_PV2, DISABLE, OWR), /* owr_pctlz:o */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 2                  */
    /*------------------------------------------*/
    CONFIG(GPIO_PU0, DISABLE, OWR), /* owr_pctlz:o */
    CONFIGEND(),
    /*------------------------------------------*/
    /*                config 3                  */
    /*------------------------------------------*/
    CONFIG(KB_ROW5, DISABLE, OWR), /* owr_pctlz:o */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxOwr[] = {
    &s_T30MuxOwr1[0],
    NULL,
};

static const NvU32 s_T30MuxPcie1[] = {
    /*------------------------------------------*/
    /*                unconfig                  */
    /*------------------------------------------*/
    CONFIGEND(),

    /*------------------------------------------*/
    /*                config 1                  */
    /*------------------------------------------*/
    CONFIG(PEX_L0_CLKREQ_N, ENABLE, PCIE), /* pe0_clkreq_l:i */
    CONFIG(PEX_L0_PRSNT_N, ENABLE, PCIE),  /* pe0_prsnt_l:i */
    CONFIG(PEX_L0_RST_N, DISABLE, PCIE),   /* pe0_rst_l:o */
    CONFIG(PEX_L1_CLKREQ_N, ENABLE, PCIE), /* pe1_clkreq_l:i */
    CONFIG(PEX_L1_PRSNT_N, ENABLE, PCIE),  /* pe1_prsnt_l:i */
    CONFIG(PEX_L1_RST_N, DISABLE, PCIE),   /* pe1_rst_l:o */
    CONFIG(PEX_WAKE_N, ENABLE, PCIE),      /* pe_wake_l:i */
    CONFIG(PEX_L2_CLKREQ_N, ENABLE, PCIE), /* pe2_clkreq_l:i */
    CONFIG(PEX_L2_PRSNT_N, ENABLE, PCIE),  /* pe2_prsnt_l:i */
    CONFIG(PEX_L2_RST_N, DISABLE, PCIE),   /* pe2_rst_l:o */
    CONFIGEND(),

    MODULEDONE(),
};

static const NvU32 *s_T30MuxPcie[] = {
    &s_T30MuxPcie1[0],
    NULL,
};

static const NvU32** s_T30MuxControllers[] = {
    &s_T30MuxAta[0],
    &s_T30MuxCrt[0],
    NULL, // no options for CSI
    &s_T30MuxDap[0],
    &s_T30MuxDisplay[0],
    NULL, // no options for DSI
    NULL, // no options for GPIO
    &s_T30MuxHdcp[0],
    &s_T30MuxHdmi[0],
    &s_T30MuxHsi[0],
    NULL, // No options for HSMMC
    NULL, // no options for I2S
    &s_T30MuxI2c[0],
    NULL,
    &s_T30MuxKbd[0],
    &s_T30MuxMio[0],
    &s_T30MuxNand[0],
    &s_T30MuxPwm[0],
    &s_T30MuxSdio[0],
    NULL, // sflash is not supported on t30
    NULL, // no options for Slink
    &s_T30MuxSpdif[0],
    &s_T30MuxSpi[0],
    &s_T30MuxTwc[0],
    NULL, //  no options for TVO
    &s_T30MuxUart[0],
    NULL, //  no options for USB
    NULL, //  no options for VDD
    &s_T30MuxVi[0],
    NULL, //  no options for XIO
    &s_T30MuxExtClock[0],
    &s_T30MuxUlpi[0],
    &s_T30MuxOwr[0],
    &s_T30MuxSnor[0], //  SyncNOR
    &s_T30MuxPcie[0],
    &s_T30MuxEtm[0],
    NULL, //  no options for TSENSor
    &s_T30MuxBacklight[0],
};

NV_CT_ASSERT(NV_ARRAY_SIZE(s_T30MuxControllers)==NvOdmIoModule_Num);

const NvU32***
NvRmT30GetPinMuxConfigs(NvRmDeviceHandle hDevice)
{
    NV_ASSERT(hDevice);
    return (const NvU32***) s_T30MuxControllers;
}
#endif

/* Define the GPIO port/pin to tristate mappings */

const NvU16 g_T30GpioPadMapping[] =
{
    //  Port A
    GPIO_TRISTATE(CLK_32K_OUT), GPIO_TRISTATE(UART3_CTS_N), GPIO_TRISTATE(DAP2_FS), GPIO_TRISTATE(DAP2_SCLK),
    GPIO_TRISTATE(DAP2_DIN), GPIO_TRISTATE(DAP2_DOUT), GPIO_TRISTATE(SDMMC3_CLK), GPIO_TRISTATE(SDMMC3_CMD),
    //  Port B
    GPIO_TRISTATE(GMI_A17), GPIO_TRISTATE(GMI_A18), GPIO_TRISTATE(LCD_PWR0), GPIO_TRISTATE(LCD_PCLK),
    GPIO_TRISTATE(SDMMC3_DAT3), GPIO_TRISTATE(SDMMC3_DAT2), GPIO_TRISTATE(SDMMC3_DAT1), GPIO_TRISTATE(SDMMC3_DAT0),
    //  Port C
    GPIO_TRISTATE(UART3_RTS_N), GPIO_TRISTATE(LCD_PWR1), GPIO_TRISTATE(UART2_TXD), GPIO_TRISTATE(UART2_RXD),
    GPIO_TRISTATE(GEN1_I2C_SCL), GPIO_TRISTATE(GEN1_I2C_SDA), GPIO_TRISTATE(LCD_PWR2), GPIO_TRISTATE(GMI_WP_N),
    //  Port D
    GPIO_TRISTATE(SDMMC3_DAT5), GPIO_TRISTATE(SDMMC3_DAT4), GPIO_TRISTATE(LCD_DC1), GPIO_TRISTATE(SDMMC3_DAT6),
    GPIO_TRISTATE(SDMMC3_DAT7), GPIO_TRISTATE(VI_D1), GPIO_TRISTATE(VI_VSYNC), GPIO_TRISTATE(VI_HSYNC),
    //  Port E
    GPIO_TRISTATE(LCD_D0), GPIO_TRISTATE(LCD_D1), GPIO_TRISTATE(LCD_D2), GPIO_TRISTATE(LCD_D3),
    GPIO_TRISTATE(LCD_D4), GPIO_TRISTATE(LCD_D5), GPIO_TRISTATE(LCD_D6), GPIO_TRISTATE(LCD_D7),
    //  Port F
    GPIO_TRISTATE(LCD_D8),GPIO_TRISTATE(LCD_D9),GPIO_TRISTATE(LCD_D10), GPIO_TRISTATE(LCD_D11),
    GPIO_TRISTATE(LCD_D12),GPIO_TRISTATE(LCD_D13),GPIO_TRISTATE(LCD_D14), GPIO_TRISTATE(LCD_D15),
    //  Port G
    GPIO_TRISTATE(GMI_AD0), GPIO_TRISTATE(GMI_AD1),GPIO_TRISTATE(GMI_AD2), GPIO_TRISTATE(GMI_AD3),
    GPIO_TRISTATE(GMI_AD4), GPIO_TRISTATE(GMI_AD5),GPIO_TRISTATE(GMI_AD6), GPIO_TRISTATE(GMI_AD7),
    //  Port H
    GPIO_TRISTATE(GMI_AD8), GPIO_TRISTATE(GMI_AD9),GPIO_TRISTATE(GMI_AD10), GPIO_TRISTATE(GMI_AD11),
    GPIO_TRISTATE(GMI_AD12), GPIO_TRISTATE(GMI_AD13),GPIO_TRISTATE(GMI_AD14), GPIO_TRISTATE(GMI_AD15),
    //  Port I
    GPIO_TRISTATE(GMI_WR_N),GPIO_TRISTATE(GMI_OE_N), GPIO_TRISTATE(GMI_DQS), GPIO_TRISTATE(GMI_CS6_N),
    GPIO_TRISTATE(GMI_RST_N),GPIO_TRISTATE(GMI_IORDY), GPIO_TRISTATE(GMI_CS7_N), GPIO_TRISTATE(GMI_WAIT),
    //  Port J
    GPIO_TRISTATE(GMI_CS0_N),GPIO_TRISTATE(LCD_DE), GPIO_TRISTATE(GMI_CS1_N), GPIO_TRISTATE(LCD_HSYNC),
    GPIO_TRISTATE(LCD_VSYNC),GPIO_TRISTATE(UART2_CTS_N), GPIO_TRISTATE(UART2_RTS_N), GPIO_TRISTATE(GMI_A16),
    //  Port K
    GPIO_TRISTATE(GMI_ADV_N),GPIO_TRISTATE(GMI_CLK), GPIO_TRISTATE(GMI_CS4_N), GPIO_TRISTATE(GMI_CS2_N),
    GPIO_TRISTATE(GMI_CS3_N),GPIO_TRISTATE(SPDIF_OUT), GPIO_TRISTATE(SPDIF_IN), GPIO_TRISTATE(GMI_A19),
    //  Port L
    GPIO_TRISTATE(VI_D2),GPIO_TRISTATE(VI_D3), GPIO_TRISTATE(VI_D4), GPIO_TRISTATE(VI_D5),
    GPIO_TRISTATE(VI_D6),GPIO_TRISTATE(VI_D7), GPIO_TRISTATE(VI_D8), GPIO_TRISTATE(VI_D9),
    //  Port M
    GPIO_TRISTATE(LCD_D16),GPIO_TRISTATE(LCD_D17), GPIO_TRISTATE(LCD_D18), GPIO_TRISTATE(LCD_D19),
    GPIO_TRISTATE(LCD_D20),GPIO_TRISTATE(LCD_D21), GPIO_TRISTATE(LCD_D22), GPIO_TRISTATE(LCD_D23),
    //  Port N
    GPIO_TRISTATE(DAP1_FS),GPIO_TRISTATE(DAP1_DIN), GPIO_TRISTATE(DAP1_DOUT), GPIO_TRISTATE(DAP1_SCLK),
    GPIO_TRISTATE(LCD_CS0_N),GPIO_TRISTATE(LCD_SDOUT), GPIO_TRISTATE(LCD_DC0), GPIO_TRISTATE(HDMI_INT),
    //  Port O
    GPIO_TRISTATE(ULPI_DATA7),GPIO_TRISTATE(ULPI_DATA0), GPIO_TRISTATE(ULPI_DATA1), GPIO_TRISTATE(ULPI_DATA2),
    GPIO_TRISTATE(ULPI_DATA3),GPIO_TRISTATE(ULPI_DATA4), GPIO_TRISTATE(ULPI_DATA5), GPIO_TRISTATE(ULPI_DATA6),
    //  Port P
    GPIO_TRISTATE(DAP3_FS),GPIO_TRISTATE(DAP3_DIN), GPIO_TRISTATE(DAP3_DOUT), GPIO_TRISTATE(DAP3_SCLK),
    GPIO_TRISTATE(DAP4_FS),GPIO_TRISTATE(DAP4_DIN), GPIO_TRISTATE(DAP4_DOUT), GPIO_TRISTATE(DAP4_SCLK),
    //  Port Q
    GPIO_TRISTATE(KB_COL0),GPIO_TRISTATE(KB_COL1), GPIO_TRISTATE(KB_COL2), GPIO_TRISTATE(KB_COL3),
    GPIO_TRISTATE(KB_COL4),GPIO_TRISTATE(KB_COL5), GPIO_TRISTATE(KB_COL6), GPIO_TRISTATE(KB_COL7),
    //  Port R
    GPIO_TRISTATE(KB_ROW0),GPIO_TRISTATE(KB_ROW1), GPIO_TRISTATE(KB_ROW2), GPIO_TRISTATE(KB_ROW3),
    GPIO_TRISTATE(KB_ROW4),GPIO_TRISTATE(KB_ROW5), GPIO_TRISTATE(KB_ROW6), GPIO_TRISTATE(KB_ROW7),
    //  Port S
    GPIO_TRISTATE(KB_ROW8),GPIO_TRISTATE(KB_ROW9), GPIO_TRISTATE(KB_ROW10), GPIO_TRISTATE(KB_ROW11),
    GPIO_TRISTATE(KB_ROW12),GPIO_TRISTATE(KB_ROW13), GPIO_TRISTATE(KB_ROW14), GPIO_TRISTATE(KB_ROW15),
    //  Port T
    GPIO_TRISTATE(VI_PCLK), GPIO_TRISTATE(VI_MCLK), GPIO_TRISTATE(VI_D10), GPIO_TRISTATE(VI_D11),
    GPIO_TRISTATE(VI_D0), GPIO_TRISTATE(GEN2_I2C_SCL), GPIO_TRISTATE(GEN2_I2C_SDA), GPIO_TRISTATE(SDMMC4_CMD),
    //  Port U
    GPIO_TRISTATE(GPIO_PU0), GPIO_TRISTATE(GPIO_PU1), GPIO_TRISTATE(GPIO_PU2), GPIO_TRISTATE(GPIO_PU3),
    GPIO_TRISTATE(GPIO_PU4), GPIO_TRISTATE(GPIO_PU5), GPIO_TRISTATE(GPIO_PU6), GPIO_TRISTATE(JTAG_RTCK),
    //  Port V
    GPIO_TRISTATE(GPIO_PV0), GPIO_TRISTATE(GPIO_PV1), GPIO_TRISTATE(GPIO_PV2), GPIO_TRISTATE(GPIO_PV3),
    GPIO_TRISTATE(DDC_SCL), GPIO_TRISTATE(DDC_SDA), GPIO_TRISTATE(CRT_HSYNC), GPIO_TRISTATE(CRT_VSYNC),
    //  Port W
    GPIO_TRISTATE(LCD_CS1_N), GPIO_TRISTATE(LCD_M1), GPIO_TRISTATE(SPI2_CS1_N), GPIO_TRISTATE(SPI2_CS2_N),
    GPIO_TRISTATE(CLK1_OUT), GPIO_TRISTATE(CLK2_OUT),GPIO_TRISTATE(UART3_TXD),GPIO_TRISTATE(UART3_RXD),
    //  Port X
    GPIO_TRISTATE(SPI2_MOSI),GPIO_TRISTATE(SPI2_MISO),GPIO_TRISTATE(SPI2_SCK),GPIO_TRISTATE(SPI2_CS0_N),
    GPIO_TRISTATE(SPI1_MOSI),GPIO_TRISTATE(SPI1_SCK),GPIO_TRISTATE(SPI1_CS0_N),GPIO_TRISTATE(SPI1_MISO),
    //  Port Y
    GPIO_TRISTATE(ULPI_CLK),GPIO_TRISTATE(ULPI_DIR),GPIO_TRISTATE(ULPI_NXT),GPIO_TRISTATE(ULPI_STP),
    GPIO_TRISTATE(SDMMC1_DAT3),GPIO_TRISTATE(SDMMC1_DAT2),GPIO_TRISTATE(SDMMC1_DAT1),GPIO_TRISTATE(SDMMC1_DAT0),
    //  Port Z
    GPIO_TRISTATE(SDMMC1_CLK),GPIO_TRISTATE(SDMMC1_CMD),GPIO_TRISTATE(LCD_SDIN),GPIO_TRISTATE(LCD_WR_N),
    GPIO_TRISTATE(LCD_SCK), GPIO_TRISTATE(SYS_CLK_REQ), GPIO_TRISTATE(PWR_I2C_SCL), GPIO_TRISTATE(PWR_I2C_SDA),
    //  Port AA
    GPIO_TRISTATE(SDMMC4_DAT0), GPIO_TRISTATE(SDMMC4_DAT1), GPIO_TRISTATE(SDMMC4_DAT2), GPIO_TRISTATE(SDMMC4_DAT3),
    GPIO_TRISTATE(SDMMC4_DAT4), GPIO_TRISTATE(SDMMC4_DAT5), GPIO_TRISTATE(SDMMC4_DAT6), GPIO_TRISTATE(SDMMC4_DAT7),
    //  Port BB
    GPIO_TRISTATE(GPIO_PBB0), GPIO_TRISTATE(CAM_I2C_SCL), GPIO_TRISTATE(CAM_I2C_SDA), GPIO_TRISTATE(GPIO_PBB3),
    GPIO_TRISTATE(GPIO_PBB4), GPIO_TRISTATE(GPIO_PBB5), GPIO_TRISTATE(GPIO_PBB6), GPIO_TRISTATE(GPIO_PBB7),
    //  Port CC
    GPIO_TRISTATE(CAM_MCLK), GPIO_TRISTATE(GPIO_PCC1), GPIO_TRISTATE(GPIO_PCC2), GPIO_TRISTATE(SDMMC4_RST_N),
     GPIO_TRISTATE(SDMMC4_CLK), GPIO_TRISTATE(CLK2_REQ), GPIO_TRISTATE(PEX_L2_RST_N), GPIO_TRISTATE(PEX_L2_CLKREQ_N),
    //  Port DD
     GPIO_TRISTATE(PEX_L0_PRSNT_N), GPIO_TRISTATE(PEX_L0_RST_N), GPIO_TRISTATE(PEX_L0_CLKREQ_N), GPIO_TRISTATE(PEX_WAKE_N),
     GPIO_TRISTATE(PEX_L1_PRSNT_N), GPIO_TRISTATE(PEX_L1_RST_N), GPIO_TRISTATE(PEX_L1_CLKREQ_N), GPIO_TRISTATE(PEX_L2_PRSNT_N),
    //  Port EE
     GPIO_TRISTATE(CLK3_OUT), GPIO_TRISTATE(CLK3_REQ), GPIO_TRISTATE(CLK1_REQ)
    // Port FF is still not part of the pinout spec. will add if there are any new special purpose pins.
};

NvBool
NvRmT30GetPinForGpio(NvRmDeviceHandle hDevice,
                           NvU32 Port,
                           NvU32 Pin,
                           NvU32 *pMapping)
{
    const NvU32 GpiosPerPort = 8;
    NvU32 Index = Port*GpiosPerPort + Pin;

    if ((Pin >= GpiosPerPort) || (Index >= NV_ARRAY_SIZE(g_T30GpioPadMapping)))
        return NV_FALSE;

    *pMapping = (NvU32)g_T30GpioPadMapping[Index];
    return NV_TRUE;
}

#ifndef SET_KERNEL_PINMUX
// T30 specific pinmux register.
#define T30_PINMIX_REGISTER_ADDRESS_START PINMUX_AUX_ULPI_DATA0_0
#define T30_PINMUX_REGISTER_0_PUPD_RANGE PINMUX_AUX_ULPI_DATA0_0_PUPD_RANGE
#define T30_PINMUX_REGISTER_0_BIT6_RANGE 5:4
#define T30_PINMUX_REGISTER_0_BIT7_RANGE 7:6
#define T30_PINMUX_REGISTER_0_BIT8_RANGE 9:8
void
NvRmT30SetPinAttribute(
    NvRmDeviceHandle hDevice,
    NvOdmPinAttrib *pPinAttribTable,
    NvU32 count)
{
    NvU32  PinCntrlReg;
    NvU32 i;
    NvOdmPinAttrib *pAttribTable = pPinAttribTable;
    NvU32 Offset;
    NvU32 Value;
    NvU32 bit8;
    NvU32 bit7;
    NvU32 bit6;
    NvU32 bit5;
    NvU32 bit4;

    if (!pPinAttribTable || !count)
        return;

    for (i = 0; i < count; ++i, pAttribTable++)
    {
        Offset = pAttribTable->ConfigRegister & 0xFFFF;
        Value = pAttribTable->Value;
        if (Offset >= T30_PINMIX_REGISTER_ADDRESS_START)
        {
            Value = NV_DRF_VAL(T30_PINMUX, REGISTER, PUPD, pAttribTable->Value);
            PinCntrlReg = NV_REGR(hDevice, NVRM_MODULE_ID(NvRmModuleID_Misc, 0), Offset);
            Value = NV_FLD_SET_DRF_NUM(T30_PINMUX, REGISTER, PUPD, Value, PinCntrlReg);

            /* Check for bit 4 (TRISTATE) modification */
            bit4 = pAttribTable->Value & (1 << 4);
            if (bit4)
                Value |= (1 << 4);

            /* Check for bit 5 (INPUT ENABLE/DISABLE) modification */
            bit5 = pAttribTable->Value & (1 << 5);
            if (bit5)
                Value |= (1 << 5);

            /* Check for bit 6 modification */
            bit6 = NV_DRF_VAL(T30_PINMUX, REGISTER, BIT6, pAttribTable->Value);
            if (bit6 & 0x2)
            {
                 Value &= ~(1 << 6);
                 Value |= ((bit6 & 1) << 6);
            }

            /* Check for bit 7 modification */
            bit7 = NV_DRF_VAL(T30_PINMUX, REGISTER, BIT7, pAttribTable->Value);
            if (bit7 & 0x2)
            {
                 Value &= ~(1 << 7);
                 Value |= ((bit7 & 1) << 7);
            }

            /* Check for bit 8 modification */
            bit8 = NV_DRF_VAL(T30_PINMUX, REGISTER, BIT8, pAttribTable->Value);
            if (bit8 & 0x2)
            {
                 Value &= ~(1 << 8);
                 Value |= ((bit8 & 1) << 8);
            }
        }
        NV_REGW(hDevice, NVRM_MODULE_ID( NvRmModuleID_Misc, 0 ), Offset, Value);
    }
}

NvError
NvRmPrivT30GetModuleInterfaceCaps(
    NvOdmIoModule Module,
    NvU32 Instance,
    NvU32 PinMap,
    void *pCaps)
{
    switch (Module)
    {
    case NvOdmIoModule_Sdio:
        if (Instance == 1)
        {
            if (PinMap == NvOdmSdioPinMap_Config1 || PinMap == NvOdmSdioPinMap_Config2)
                ((NvRmModuleSdmmcInterfaceCaps *)pCaps)->MmcInterfaceWidth = 8;
            else if (PinMap == NvOdmSdioPinMap_Config3)
                ((NvRmModuleSdmmcInterfaceCaps *)pCaps)->MmcInterfaceWidth = 4;
            else
            {
                NV_ASSERT(NV_FALSE);
                return NvError_NotSupported;
            }
        }
        else if (Instance==2 && PinMap==NvOdmSdioPinMap_Config1)
            ((NvRmModuleSdmmcInterfaceCaps *)pCaps)->MmcInterfaceWidth = 8;
        else if (Instance==3 && (PinMap==NvOdmSdioPinMap_Config1 || PinMap==NvOdmSdioPinMap_Config2))
            ((NvRmModuleSdmmcInterfaceCaps *)pCaps)->MmcInterfaceWidth = 8;
        else if (Instance==0 && PinMap==NvOdmSdioPinMap_Config1)
            ((NvRmModuleSdmmcInterfaceCaps *)pCaps)->MmcInterfaceWidth = 4;
        else
        {
            ((NvRmModuleSdmmcInterfaceCaps *)pCaps)->MmcInterfaceWidth = 0;
            return NvError_NotSupported;
        }
        return NvError_Success;

    case NvOdmIoModule_Pwm:
        if (Instance == 0 && (PinMap == NvOdmPwmPinMap_Config1))
            ((NvRmModulePwmInterfaceCaps *)pCaps)->PwmOutputIdSupported = 15;
        else if (Instance == 0 && (PinMap == NvOdmPwmPinMap_Config2))
            ((NvRmModulePwmInterfaceCaps *)pCaps)->PwmOutputIdSupported = 15;
        else if (Instance == 0 && (PinMap == NvOdmPwmPinMap_Config3))
            ((NvRmModulePwmInterfaceCaps *)pCaps)->PwmOutputIdSupported = 15;
        else if (Instance == 0 && (PinMap == NvOdmPwmPinMap_Config4))
            ((NvRmModulePwmInterfaceCaps *)pCaps)->PwmOutputIdSupported = 3;
        else if (Instance == 0 && (PinMap == NvOdmPwmPinMap_Config5))
            ((NvRmModulePwmInterfaceCaps *)pCaps)->PwmOutputIdSupported = 1;
        else
        {
            ((NvRmModulePwmInterfaceCaps *)pCaps)->PwmOutputIdSupported = 0;
            return NvError_NotSupported;
        }
        return NvError_Success;
    case NvOdmIoModule_Nand:
        if (Instance == 0 && (PinMap == NvOdmNandPinMap_Config1 || PinMap == NvOdmNandPinMap_Config2))
        {
            ((NvRmModuleNandInterfaceCaps*)pCaps)->IsCombRbsyMode = NV_TRUE;
            ((NvRmModuleNandInterfaceCaps*)pCaps)->NandInterfaceWidth = 16;
        }
        else
        {
            NV_ASSERT(NV_FALSE);
            return NvError_NotSupported;
        }
        return NvSuccess;
    case NvOdmIoModule_Uart:
        if (Instance == 0)
        {
            if (PinMap == NvOdmUartPinMap_Config1)
                ((NvRmModuleUartInterfaceCaps *)pCaps)->NumberOfInterfaceLines = 8;
            else if (PinMap == NvOdmUartPinMap_Config2)
                ((NvRmModuleUartInterfaceCaps *)pCaps)->NumberOfInterfaceLines = 7;
            else if (PinMap == NvOdmUartPinMap_Config3)
                ((NvRmModuleUartInterfaceCaps *)pCaps)->NumberOfInterfaceLines = 6;
            else if (PinMap == NvOdmUartPinMap_Config4)
                ((NvRmModuleUartInterfaceCaps *)pCaps)->NumberOfInterfaceLines = 4;
            else if (PinMap == NvOdmUartPinMap_Config5)
                ((NvRmModuleUartInterfaceCaps *)pCaps)->NumberOfInterfaceLines = 2;
            else
                ((NvRmModuleUartInterfaceCaps *)pCaps)->NumberOfInterfaceLines = 0;
        }
        else if ((Instance == 1) || (Instance == 2))
        {
            if (PinMap == NvOdmUartPinMap_Config1)
                ((NvRmModuleUartInterfaceCaps *)pCaps)->NumberOfInterfaceLines = 4;
            else
                ((NvRmModuleUartInterfaceCaps *)pCaps)->NumberOfInterfaceLines = 0;
        }
        else if ((Instance == 3) || (Instance == 4))
        {
            if ((PinMap == NvOdmUartPinMap_Config1) || (PinMap == NvOdmUartPinMap_Config2))
                ((NvRmModuleUartInterfaceCaps *)pCaps)->NumberOfInterfaceLines = 4;
            else
                ((NvRmModuleUartInterfaceCaps *)pCaps)->NumberOfInterfaceLines = 0;
        }
        else
        {
            NV_ASSERT(NV_FALSE);
            return NvError_NotSupported;
        }
        return NvSuccess;

    default:
        break;
    }

    return NvError_NotSupported;
}
#endif

NvU32
NvRmPrivT30GetExternalClockSourceFreq(
    NvRmDeviceHandle hDevice,
    const NvU32* Inst,
    NvU32 Config)
{
    // Not required.
     return 0;
}

void NvRmPrivT30EnableExternalClockSource(
    NvRmDeviceHandle hDevice,
    const NvU32* Inst,
    NvU32 Config,
    NvBool ClockState)
{
    NvU32 ForceClkShift = 0, RegData = 0;
    NvU32 Instance = 0;
    NvU32 MuxRegOffset = NV_DRF_VAL(MUX,ENTRY, MUX_REG_OFFSET, *(++Inst));

    MuxRegOffset = (PINMUX_AUX_ULPI_DATA0_0 + 4*MuxRegOffset);

    switch (MuxRegOffset)
    {
        case PINMUX_AUX_CLK1_OUT_0: // EXTPERIPH1
              ForceClkShift = APBDEV_PMC_CLK_OUT_CNTRL_0_CLK1_FORCE_EN_SHIFT;
              Instance = 0;
             break;
        case PINMUX_AUX_CLK2_OUT_0: // EXTPERIPH2
              ForceClkShift = APBDEV_PMC_CLK_OUT_CNTRL_0_CLK2_FORCE_EN_SHIFT;
              Instance = 1;
            break;
        case PINMUX_AUX_CLK3_OUT_0: // EXTPERIPH3
             ForceClkShift = APBDEV_PMC_CLK_OUT_CNTRL_0_CLK3_FORCE_EN_SHIFT;
              Instance = 2;
            break;
        default:
            return;
    }

    if (!NvOdmQueryIsClkReqPinUsed(Instance))
    {
        //Force to clock to run regardless of the accept req.
        RegData =  NV_REGR(hDevice, NVRM_MODULE_ID( NvRmModuleID_Pmif, 0 ), APBDEV_PMC_CLK_OUT_CNTRL_0);
        NV_REGW(hDevice, NVRM_MODULE_ID( NvRmModuleID_Pmif, 0 ),
                APBDEV_PMC_CLK_OUT_CNTRL_0, ( RegData | (1UL << ForceClkShift)));
    }

    NvRmPowerModuleClockControl(hDevice,
            NVRM_MODULE_ID(NvRmModuleID_ExtPeriphClk, Instance), 0, NV_TRUE);

    // Remove reset for this module.
    NvRmModuleReset( hDevice, NVRM_MODULE_ID(NvRmModuleID_ExtPeriphClk, Instance));
}

