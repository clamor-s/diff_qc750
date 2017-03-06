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
    NvOdmQueryBoardPersonality Personality;
    NvOdmBoardInfo BoardInfo;

    NvOdmOsMemset(&BoardInfo, 0, sizeof(NvOdmBoardInfo));

    Personality = NvOdmQueryBoardPersonality_ScrollWheel_WiFi;

    NvOdmPeripheralGetBoardInfo(0, &BoardInfo);

    switch (Group)
    {
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
            *pCount = 0;
            return NULL;

        case NvOdmGpioPinGroup_keypadMisc:
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
