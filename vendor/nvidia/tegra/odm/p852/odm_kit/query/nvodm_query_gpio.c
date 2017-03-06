/*
 * Copyright (c) 2007-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_query_gpio.h"
#include "nvodm_query.h"
#include "nvodm_services.h"
//#include "nvodm_keylist_reserved.h"
//#include "tegra_devkit_custopt.h"
#include "nvrm_drf.h"

#define NVODM_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define NVODM_PORT(x) ((x) - 'a')

// NVTODO: Do we need this? Should it be in s_sdio tables?
static const NvOdmGpioPinInfo s_vi[] = {
    {NVODM_PORT('q'), 1, NvOdmGpioPinActiveState_High}, // 3V3SD_ENA
};

static const NvOdmGpioPinInfo s_display[] = {
    /* NOTE: fpgas do not have gpio pin muxing */
    // NVTODO: verify this table.

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

    /* Panel 5 -- p852/e1155 RGB interface */
    { NVODM_PORT('q'), 6, NvOdmGpioPinActiveState_Low },   // KB_COL6
    { NVODM_PORT('c'), 1, NvOdmGpioPinActiveState_Low },
};

//static const NvOdmGpioPinInfo s_hdmi[] =
//{
    ///* hdmi hot-plug interrupt pin */
    //// NVTODO: Remove this?
    //{ NVODM_PORT('n'), 7, NvOdmGpioPinActiveState_High},
//};

// NVTODO: Add support to pin GPIO_PQ0 (overcurrent detect) and
// GPIO_PQ1 (3v3sd_ena) (affecting all SDIO instances)
// NVTODO: Add support for SDIO1_LED (GPIO_PV2)
static const NvOdmGpioPinInfo s_sdio0[] = {
    /* sdio1 on schematic */
    {NVODM_PORT('v'), 0, NvOdmGpioPinActiveState_Low},  /* card detect */
    {NVODM_PORT('v'), 1, NvOdmGpioPinActiveState_High}, /* write protect */
};

// NVTODO: Add support for SDIO2_LED (GPIO_PD6)
static const NvOdmGpioPinInfo s_sdio1[] = {
    /* sdio2 on schematic */
    {NVODM_PORT('d'), 7, NvOdmGpioPinActiveState_Low},  /* card detect */
    {NVODM_PORT('t'), 4, NvOdmGpioPinActiveState_High}, /* write protect */
};

static const NvOdmGpioPinInfo s_usb[] = {
    {NVODM_PORT('w'), 1, NvOdmGpioPinActiveState_Low},
    {NVODM_PORT('b'), 2, NvOdmGpioPinActiveState_Low},
    {NVODM_GPIO_INVALID_PORT, NVODM_GPIO_INVALID_PIN,
     NvOdmGpioPinActiveState_Low},                       // USB Cable ID
};

// NVTODO: Remove
//static const NvOdmGpioPinInfo s_Ide[] = {
//    {NVODM_PORT('i'), 6, NvOdmGpioPinActiveState_High},
//};

static const NvOdmGpioPinInfo s_NandFlash[] = {
    {NVODM_PORT('c'), 7, NvOdmGpioPinActiveState_High}, /* write protect */
};


// NVTODO: Remove
//static const NvOdmGpioPinInfo s_oem[] = {
    //// N721 Power
    //{NVODM_PORT('v'), 0, NvOdmGpioPinActiveState_High},
    //// N721 Reset
    //{NVODM_PORT('v'), 1, NvOdmGpioPinActiveState_Low},
    //{NVODM_GPIO_INVALID_PORT, NVODM_GPIO_INVALID_PIN,
        //NvOdmGpioPinActiveState_Low},
//};

// NVTODO: Remove
//static const NvOdmGpioPinInfo s_test[] = {
    //{NVODM_GPIO_INVALID_PORT, NVODM_GPIO_INVALID_PIN,
        //NvOdmGpioPinActiveState_Low},
//};

// NVTODO: Verify this
//static const NvOdmGpioPinInfo s_spi_ethernet[] = {
    //{NVODM_PORT('l'), 7, NvOdmGpioPinActiveState_Low}
//};

// NVTODO: Is this used at all?
//static const NvOdmGpioPinInfo s_Bluetooth[] = {
    //// Bluetooth Reset
    //{NVODM_PORT('u'), 0, NvOdmGpioPinActiveState_Low}
//};

// NVTODO: Is this used at all?
//static const NvOdmGpioPinInfo s_Wlan[] = {
    //// Wlan Power
    //{NVODM_PORT('u'), 1, NvOdmGpioPinActiveState_Low},
    //// Wlan Reset
    //{NVODM_PORT('u'), 2, NvOdmGpioPinActiveState_Low}
//};

static const NvOdmGpioPinInfo s_Power[] = {
    // on/off button (GPIO_PT5)
    {NVODM_PORT('t'), 5, NvOdmGpioPinActiveState_Low},
    // end of list
    { 0, 0, 0 },
};

const NvOdmGpioPinInfo *NvOdmQueryGpioPinMap(NvOdmGpioPinGroup Group,
    NvU32 Instance, NvU32 *pCount)
{
    switch (Group)
    {
        case NvOdmGpioPinGroup_Display:
            *pCount = NVODM_ARRAY_SIZE(s_display);
            return s_display;

        //case NvOdmGpioPinGroup_Hdmi:
            //*pCount = NVODM_ARRAY_SIZE(s_hdmi);
            //return s_hdmi;

        case NvOdmGpioPinGroup_keypadColumns:
        case NvOdmGpioPinGroup_keypadRows:
        case NvOdmGpioPinGroup_keypadSpecialKeys:
        case NvOdmGpioPinGroup_keypadMisc:
        case NvOdmGpioPinGroup_Hsmmc:
            *pCount = 0;
            return NULL;

        case NvOdmGpioPinGroup_ScrollWheel:
            *pCount = 0;
            return NULL;

        case NvOdmGpioPinGroup_Sdio:
            if (Instance == 0)
            {
                *pCount = NVODM_ARRAY_SIZE(s_sdio0);
                return s_sdio0;
            }
            else if (Instance == 1)
            {
                *pCount = NVODM_ARRAY_SIZE(s_sdio1);
                return s_sdio1;
            }
            else
            {
                *pCount = 0;
                return NULL;
            }

        case NvOdmGpioPinGroup_Usb:
            if (Instance == 2)
            {
                *pCount = NVODM_ARRAY_SIZE(s_usb);
                return s_usb;
            }
            else
            {
                *pCount = 0;
                return NULL;
            }

//        case NvOdmGpioPinGroup_Ide:
//            *pCount = NVODM_ARRAY_SIZE(s_Ide);
//            return s_Ide;

//        case NvOdmGpioPinGroup_OEM:
//            *pCount = NVODM_ARRAY_SIZE(s_oem);
//            return s_oem;

//        case NvOdmGpioPinGroup_Test:
//            *pCount = NVODM_ARRAY_SIZE(s_test);
//            return s_test;

//        case NvOdmGpioPinGroup_MioEthernet:
//            *pCount = 0;
//            return NULL;

        case NvOdmGpioPinGroup_NandFlash:
            *pCount = NVODM_ARRAY_SIZE(s_NandFlash);
            return s_NandFlash;

//        case NvOdmGpioPinGroup_Mio:
//            *pCount = 0;
//            return NULL;

//        case NvOdmGpioPinGroup_Bluetooth:
//                *pCount = NVODM_ARRAY_SIZE(s_Bluetooth);
//                return s_Bluetooth;

//        case NvOdmGpioPinGroup_Wlan:
//            *pCount = 0;
//            return NULL;

//        case NvOdmGpioPinGroup_SpiEthernet:
//            if (NvOdmQueryDownloadTransport() ==
//                NvOdmDownloadTransport_SpiEthernet)
//            {
//                *pCount = NVODM_ARRAY_SIZE(s_spi_ethernet);
//                return s_spi_ethernet;
//            }
//            else
//            {
//                *pCount = 0;
//                return NULL;
//            }

        case NvOdmGpioPinGroup_Vi:
            *pCount = NVODM_ARRAY_SIZE(s_vi);
            return s_vi;

        case NvOdmGpioPinGroup_Power:
            *pCount = NVODM_ARRAY_SIZE(s_Power);
            return s_Power;

        default:
            *pCount = 0;
            return NULL;
    }
}

