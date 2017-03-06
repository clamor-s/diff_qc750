/*
 * Copyright (c) 2007-2012, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_query_gpio.h"
#include "nvodm_query_pinmux_gpio.h"
#include "nvodm_services.h"
#include "nvodm_query_discovery.h"
#include "nvodm_query.h"

#define NVODM_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define NVODM_PORT(x) ((x) - 'a')

#define NV_BOARD_E1186_ID  0x0B56
#define NV_BOARD_E1187_ID  0x0B57
#define NV_BOARD_E1198_ID  0x0B62
#define NV_BOARD_E1291_ID  0x0C5B
#define NV_BOARD_PM269_ID  0x0245
#define NV_BOARD_PM305_ID  0x0305
#define NV_BOARD_PM311_ID  0x030B

/* Display Board */
#define NV_DISPLAY_BOARD_PM313_ID   0x030D

static const NvOdmGpioPinInfo s_display[] = {
    /* NOTE: fpgas do not have gpio pin muxing */

    /* Panel 0 -- sony vga */
    { NVODM_PORT('m'), 3, NvOdmGpioPinActiveState_Low },
    { NVODM_PORT('b'), 2, NvOdmGpioPinActiveState_Low },
    { NVODM_PORT('n'), 4, NvOdmGpioPinActiveState_Low },
    { NVODM_PORT('j'), 3, NvOdmGpioPinActiveState_Low },
    { NVODM_PORT('j'), 4, NvOdmGpioPinActiveState_Low },
    // this pin is not needed for ap15
    {NVODM_GPIO_INVALID_PORT, NVODM_GPIO_INVALID_PIN,
        NvOdmGpioPinActiveState_Low},

    /* Panel 1 -- samtek */
    {NVODM_GPIO_INVALID_PORT, NVODM_GPIO_INVALID_PIN,
        NvOdmGpioPinActiveState_Low},
    {NVODM_GPIO_INVALID_PORT, NVODM_GPIO_INVALID_PIN,
        NvOdmGpioPinActiveState_Low},
    {NVODM_GPIO_INVALID_PORT, NVODM_GPIO_INVALID_PIN,
        NvOdmGpioPinActiveState_Low},
    {NVODM_GPIO_INVALID_PORT, NVODM_GPIO_INVALID_PIN,
        NvOdmGpioPinActiveState_Low},

    /* Panel 2 -- sharp wvga */
    { NVODM_PORT('v'), 7, NvOdmGpioPinActiveState_Low },

    /* Panel 3 -- sharp qvga */
    { NVODM_PORT('n'), 6, NvOdmGpioPinActiveState_High },   // LCD_DC0
    { NVODM_PORT('n'), 4, NvOdmGpioPinActiveState_Low },    // LCD_CS0
    { NVODM_PORT('b'), 3, NvOdmGpioPinActiveState_Low },    // LCD_PCLK
    { NVODM_PORT('b'), 2, NvOdmGpioPinActiveState_Low },    // LCD_PWR0
    { NVODM_PORT('e'), 0, NvOdmGpioPinActiveState_High },   // LCD_D0
    { NVODM_PORT('e'), 1, NvOdmGpioPinActiveState_High },   // LCD_D1
    { NVODM_PORT('e'), 2, NvOdmGpioPinActiveState_High },   // LCD_D2
    { NVODM_PORT('e'), 3, NvOdmGpioPinActiveState_High },   // LCD_D3
    { NVODM_PORT('e'), 4, NvOdmGpioPinActiveState_High },   // LCD_D4
    { NVODM_PORT('e'), 5, NvOdmGpioPinActiveState_High },   // LCD_D5
    { NVODM_PORT('e'), 6, NvOdmGpioPinActiveState_High },   // LCD_D6
    { NVODM_PORT('e'), 7, NvOdmGpioPinActiveState_High },   // LCD_D7
    { NVODM_PORT('f'), 0, NvOdmGpioPinActiveState_High },   // LCD_D8
    { NVODM_PORT('f'), 1, NvOdmGpioPinActiveState_High },   // LCD_D9
    { NVODM_PORT('f'), 2, NvOdmGpioPinActiveState_High },   // LCD_D10
    { NVODM_PORT('f'), 3, NvOdmGpioPinActiveState_High },   // LCD_D11
    { NVODM_PORT('f'), 4, NvOdmGpioPinActiveState_High },   // LCD_D12
    { NVODM_PORT('f'), 5, NvOdmGpioPinActiveState_High },   // LCD_D13
    { NVODM_PORT('f'), 6, NvOdmGpioPinActiveState_High },   // LCD_D14
    { NVODM_PORT('f'), 7, NvOdmGpioPinActiveState_High },   // LCD_D15
    { NVODM_PORT('m'), 3, NvOdmGpioPinActiveState_High },   // LCD_D19

     /* Panel 4 -- auo */
     { NVODM_PORT('v'), 7, NvOdmGpioPinActiveState_Low },

    /* Panel 5 -- Cardhu E1162 LVDS interface */
    { NVODM_PORT('l'), 2, NvOdmGpioPinActiveState_High },   // Enable (LVDS_SHTDN_N) (LO:OFF, HI:ON)
    { NVODM_PORT('k'), 3, NvOdmGpioPinActiveState_High },   // EN_VDD_BL
    { NVODM_PORT('h'), 2, NvOdmGpioPinActiveState_High },   // LCD_BL_EN
    { NVODM_PORT('l'), 4, NvOdmGpioPinActiveState_High },   // EN_VDD_PNL
    { NVODM_PORT('h'), 0, NvOdmGpioPinActiveState_High },   // LCD_BL_PWM

    /* Panel 6 -- Cardhu E1162 LVDS interface for PM269 board*/
    { NVODM_PORT('n'), 6, NvOdmGpioPinActiveState_High },   // Enable (LVDS_SHTDN_N) (LO:OFF, HI:ON)
    { NVODM_PORT('h'), 3, NvOdmGpioPinActiveState_High },   // EN_VDD_BL
    { NVODM_PORT('h'), 2, NvOdmGpioPinActiveState_High },   // LCD_BL_EN
    { NVODM_PORT('w'), 1, NvOdmGpioPinActiveState_High },   // EN_VDD_PNL
    { NVODM_PORT('h'), 0, NvOdmGpioPinActiveState_High },   // LCD_BL_PWM
};

static const NvOdmGpioPinInfo s_LvdsDisplay_base[] = {
    { NVODM_PORT('l'), 2, NvOdmGpioPinActiveState_High },   // Enable (LVDS_SHTDN_N) (LO:OFF, HI:ON)
    { NVODM_PORT('k'), 3, NvOdmGpioPinActiveState_High },   // EN_VDD_BL
    { NVODM_PORT('h'), 2, NvOdmGpioPinActiveState_High },   // LCD_BL_EN
    { NVODM_PORT('l'), 4, NvOdmGpioPinActiveState_High },   // EN_VDD_PNL
    { NVODM_PORT('h'), 0, NvOdmGpioPinActiveState_High },   // LCD_BL_PWM
};

static const NvOdmGpioPinInfo s_LvdsDisplay_pm269[] = {
    { NVODM_PORT('n'), 6, NvOdmGpioPinActiveState_High },   // Enable (LVDS_SHTDN_N) (LO:OFF, HI:ON)
    { NVODM_PORT('h'), 3, NvOdmGpioPinActiveState_High },   // EN_VDD_BL
    { NVODM_PORT('h'), 2, NvOdmGpioPinActiveState_High },   // LCD_BL_EN
    { NVODM_PORT('w'), 1, NvOdmGpioPinActiveState_High },   // EN_VDD_PNL
    { NVODM_PORT('h'), 0, NvOdmGpioPinActiveState_High },   // LCD_BL_PWM
};

/* PM313 RGB-to-LVDS display interface board */
static const NvOdmGpioPinInfo s_LvdsDisplay_pm313[] = {
    { NVODM_PORT('l'), 2, NvOdmGpioPinActiveState_High },   // Enable (LVDS_SHTDN_N) (LO:OFF, HI:ON)
    { NVODM_PORT('k'), 3, NvOdmGpioPinActiveState_High },   // EN_VDD_BL
    { NVODM_PORT('h'), 2, NvOdmGpioPinActiveState_High },   // LCD_BL_EN
    { NVODM_PORT('h'), 3, NvOdmGpioPinActiveState_High },   // EN_VDD_PNL
    { NVODM_PORT('h'), 0, NvOdmGpioPinActiveState_Low },    // LCD_BL_PWM
    { NVODM_PORT('w'), 0, NvOdmGpioPinActiveState_High },   // R_FDE
    { NVODM_PORT('n'), 4, NvOdmGpioPinActiveState_High },   // R_FB
    { NVODM_PORT('z'), 4, NvOdmGpioPinActiveState_High },   // MODE0
    { NVODM_PORT('w'), 1, NvOdmGpioPinActiveState_Low },    // MODE1
    { NVODM_PORT('n'), 6, NvOdmGpioPinActiveState_High },   // BPP(Low:24bpp, High:18bpp)
};

static const NvOdmGpioPinInfo s_usb[] = {
    {NVODM_PORT('c'), 7, NvOdmGpioPinActiveState_Low},
};

static const NvOdmGpioPinInfo s_Sdio0[] = {
    {NVODM_PORT('v'), 4, NvOdmGpioPinActiveState_Low},    // Card Detect for SDIo instance 0
};

static const NvOdmGpioPinInfo s_Sdio2[] = {
    {NVODM_PORT('v'), 5, NvOdmGpioPinActiveState_Low},    // Card Detect for SDIO instance 2
    {NVODM_PORT('v'), 6, NvOdmGpioPinActiveState_Low},    // Write Protect for SDIO instance 2
};

static const NvOdmGpioPinInfo s_Ide[] = {
    {NVODM_PORT('i'), 6, NvOdmGpioPinActiveState_High},
};

static const NvOdmGpioPinInfo s_oem[] = {
    // N721 Power
    {NVODM_PORT('v'), 0, NvOdmGpioPinActiveState_High},
    // N721 Reset
    {NVODM_PORT('v'), 1, NvOdmGpioPinActiveState_Low},
    {NVODM_GPIO_INVALID_PORT, NVODM_GPIO_INVALID_PIN,
        NvOdmGpioPinActiveState_Low},
};

static const NvOdmGpioPinInfo s_test[] = {
    {NVODM_GPIO_INVALID_PORT, NVODM_GPIO_INVALID_PIN,
        NvOdmGpioPinActiveState_Low},
};

static const NvOdmGpioPinInfo s_spi_ethernet[] = {
    {NVODM_PORT('k'), 6, NvOdmGpioPinActiveState_Low}
};

/* Scroll wheel ON-OFF, Select, Input1 and Input 2 */
static const NvOdmGpioPinInfo s_ScrollWheel[] = {
    {NVODM_PORT('r'), 3, NvOdmGpioPinActiveState_Low},
    {NVODM_PORT('q'), 5, NvOdmGpioPinActiveState_Low},
    {NVODM_PORT('q'), 4, NvOdmGpioPinActiveState_Low},
    {NVODM_PORT('q'), 3, NvOdmGpioPinActiveState_Low}
};

// Gpio Pin key info - uses NvEc scan codes
static const NvOdmGpioPinInfo s_GpioPinKeyInfo[] = {
    {0xE04B, 10, NV_TRUE}, /* LEFT */
    {0xE04D, 10, NV_TRUE}, /* RIGHT */
    {0x1C, 10, NV_TRUE},   /* ENTER */
};

// Gpio based keypad on E1198
static const NvOdmGpioPinInfo s_GpioKeyBoard_e1198[] = {
    {NVODM_PORT('q'), 0, NvOdmGpioPinActiveState_Low, (void *)&s_GpioPinKeyInfo[0]},
    {NVODM_PORT('q'), 1, NvOdmGpioPinActiveState_Low, (void *)&s_GpioPinKeyInfo[1]},
    {NVODM_PORT('q'), 2, NvOdmGpioPinActiveState_Low, (void *)&s_GpioPinKeyInfo[2]},
};

// Gpio based keypad on E1291
static const NvOdmGpioPinInfo s_GpioKeyBoard_e1291[] = {
    {NVODM_PORT('r'), 0, NvOdmGpioPinActiveState_Low, (void *)&s_GpioPinKeyInfo[2]},
    {NVODM_PORT('r'), 1, NvOdmGpioPinActiveState_Low, (void *)&s_GpioPinKeyInfo[0]},
    {NVODM_PORT('r'), 2, NvOdmGpioPinActiveState_Low, (void *)&s_GpioPinKeyInfo[1]},
};


static const NvOdmGpioPinInfo s_Bluetooth[] = {
    // Bluetooth Reset
    {NVODM_PORT('u'), 0, NvOdmGpioPinActiveState_Low}
};

static const NvOdmGpioPinInfo s_Wlan[] = {
    // Wlan Power
    {NVODM_PORT('v'), 1, NvOdmGpioPinActiveState_Low},
    // Wlan Reset
    {NVODM_PORT('v'), 2, NvOdmGpioPinActiveState_Low},
};

static const NvOdmGpioPinInfo s_hdmi[] =
{
    /* hdmi hot-plug interrupt pin */
    { NVODM_PORT('n'), 7, NvOdmGpioPinActiveState_High},
};

const NvOdmGpioPinInfo *NvOdmQueryGpioPinMap(NvOdmGpioPinGroup Group,
    NvU32 Instance, NvU32 *pCount)
{
    NvBool isPm313;
    NvOdmQueryBoardPersonality Personality;
    NvOdmBoardInfo BoardInfo;

    isPm313 = NvOdmPeripheralGetBoardInfo(NV_DISPLAY_BOARD_PM313_ID, &BoardInfo);

    NvOdmOsMemset(&BoardInfo, 0, sizeof(NvOdmBoardInfo));

    Personality = NvOdmQueryBoardPersonality_ScrollWheel_WiFi;

    NvOdmPeripheralGetBoardInfo(0, &BoardInfo);

    switch (Group)
    {
        case NvOdmGpioPinGroup_Display:
            *pCount = NVODM_ARRAY_SIZE(s_display);
            return s_display;

        case NvOdmGpioPinGroup_LvdsDisplay:
            if (isPm313 == NV_TRUE) {
                *pCount = NVODM_ARRAY_SIZE(s_LvdsDisplay_pm313);
                return s_LvdsDisplay_pm313;
            }

            if ((BoardInfo.BoardID == NV_BOARD_PM269_ID) ||
                  (BoardInfo.BoardID == NV_BOARD_PM305_ID) ||
                  (BoardInfo.BoardID == NV_BOARD_PM311_ID))
            {
                *pCount = NVODM_ARRAY_SIZE(s_LvdsDisplay_pm269);
                 return s_LvdsDisplay_pm269;
            }
            *pCount = NVODM_ARRAY_SIZE(s_LvdsDisplay_base);
            return s_LvdsDisplay_base;

        case NvOdmGpioPinGroup_keypadColumns:
        case NvOdmGpioPinGroup_keypadRows:
        case NvOdmGpioPinGroup_keypadSpecialKeys:
        case NvOdmGpioPinGroup_Hsmmc:
            *pCount = 0;
            return NULL;

        case NvOdmGpioPinGroup_Sdio:
            if (Instance == 0)
            {
                *pCount = NVODM_ARRAY_SIZE(s_Sdio0);
                return s_Sdio0;
            }
            else if (Instance == 2)
            {
                *pCount = NVODM_ARRAY_SIZE(s_Sdio2);
                return s_Sdio2;
            }
            else
            {
                *pCount = 0;
                return NULL;
            }
        case NvOdmGpioPinGroup_ScrollWheel:
            // Support scroll wheel for non E1198/E1291/PM269/PM311 board
            if ((BoardInfo.BoardID != NV_BOARD_E1198_ID) &&
                    (BoardInfo.BoardID != NV_BOARD_E1291_ID) &&
                    (BoardInfo.BoardID != NV_BOARD_PM269_ID) &&
                    (BoardInfo.BoardID != NV_BOARD_PM311_ID))
            {
                *pCount = NVODM_ARRAY_SIZE(s_ScrollWheel);
                return s_ScrollWheel;
            }
            *pCount = 0;
            return NULL;

        case NvOdmGpioPinGroup_keypadMisc:
            // Support keypad for E1198/E1291 board
            if (BoardInfo.BoardID == NV_BOARD_E1198_ID)
            {
                *pCount = NV_ARRAY_SIZE(s_GpioKeyBoard_e1198);
                return s_GpioKeyBoard_e1198;
            }
            else if (BoardInfo.BoardID == NV_BOARD_E1291_ID)
            {
                *pCount = NV_ARRAY_SIZE(s_GpioKeyBoard_e1291);
                return s_GpioKeyBoard_e1291;
            }
            *pCount = 0;
            return NULL;

        case NvOdmGpioPinGroup_Usb:
            *pCount = NVODM_ARRAY_SIZE(s_usb);
            return s_usb;

        case NvOdmGpioPinGroup_Ide:
            *pCount = NVODM_ARRAY_SIZE(s_Ide);
            return s_Ide;

        case NvOdmGpioPinGroup_OEM:
            *pCount = NVODM_ARRAY_SIZE(s_oem);
            return s_oem;

        case NvOdmGpioPinGroup_Test:
            *pCount = NVODM_ARRAY_SIZE(s_test);
            return s_test;

        case NvOdmGpioPinGroup_MioEthernet:
            *pCount = 0;
            return NULL;

        case NvOdmGpioPinGroup_NandFlash:
            // On AP15 Fpga, WP is always tied High. Hence not supported.
            *pCount = 0;
            return NULL;

        case NvOdmGpioPinGroup_Mio:
            *pCount = 0;
            return NULL;

        case NvOdmGpioPinGroup_Bluetooth:
            *pCount = NVODM_ARRAY_SIZE(s_Bluetooth);
            return s_Bluetooth;

        case NvOdmGpioPinGroup_Wlan:
            *pCount = NVODM_ARRAY_SIZE(s_Wlan);
            return s_Wlan;

        case NvOdmGpioPinGroup_SpiEthernet:
            *pCount = NVODM_ARRAY_SIZE(s_spi_ethernet);
            return s_spi_ethernet;

        case NvOdmGpioPinGroup_Hdmi:
            *pCount = NVODM_ARRAY_SIZE(s_hdmi);
            return s_hdmi;

        default:
            *pCount = 0;
            return NULL;
    }
}
