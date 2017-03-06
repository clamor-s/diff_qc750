/*
 * Copyright (c) 2009-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * <b>NVIDIA Tegra ODM Kit:
 *         Definition of bitfields inside the BCT customer option</b>
 *
 * @b Description: Defines the board-specific bitfields of the
 *                 BCT customer option parameter, for NVIDIA
 *                 Tegra development platforms.
 *
 *                 This file pertains to P852/E1155.
 */

#ifndef NVIDIA_TEGRA_DEVKIT_CUSTOPT_H
#define NVIDIA_TEGRA_DEVKIT_CUSTOPT_H

#if defined(__cplusplus)
extern "C"
{
#endif

//---------- BOARD PERSONALITIES (BEGIN) ----------//
// No personalities on P852.
//---------- BOARD PERSONALITIES (END) ----------//

/// Download transport
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_TRANSPORT_RANGE      10:8
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_TRANSPORT_DEFAULT    0x0UL
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_TRANSPORT_NONE       0x1UL
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_TRANSPORT_UART       0x2UL
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_TRANSPORT_USB        0x3UL
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_TRANSPORT_ETHERNET   0x4UL

/// Transport option (bus selector), for UART and Ethernet transport
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_UART_OPTION_RANGE   12:11
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_UART_OPTION_DEFAULT 0x0UL
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_UART_OPTION_A       0x1UL
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_UART_OPTION_B       0x2UL
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_UART_OPTION_C       0x3UL

#define TEGRA_DEVKIT_BCT_CUSTOPT_0_ETHERNET_OPTION_RANGE   12:11
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_ETHERNET_OPTION_DEFAULT 0x0UL
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_ETHERNET_OPTION_SPI     0x1UL

#define TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_OPTION_RANGE    17:15
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_OPTION_DEFAULT  0
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_OPTION_UARTA    0
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_OPTION_UARTB    1
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_OPTION_UARTC    2
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_OPTION_UARTD    3
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_OPTION_UARTE    4

#define TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_RANGE           19:18
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_DEFAULT         0
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_NONE            1
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_DCC             2
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_CONSOLE_UART            3

// display options
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_DISPLAY_OPTION_RANGE      22:20
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_DISPLAY_OPTION_DEFAULT    0x0UL
// embedded panel (lvds, dsi, etc)
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_DISPLAY_OPTION_EMBEDDED   0x0UL
// no panels (external or embedded)
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_DISPLAY_OPTION_NULL       0x1UL
// use hdmi as the primary
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_DISPLAY_OPTION_HDMI       0x2UL
// use crt as the primary
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_DISPLAY_OPTION_CRT        0x3UL

// Enable DHCP
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_DHCP_RANGE           23:23
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_DHCP_DEFAULT         0x0UL
#define TEGRA_DEVKIT_BCT_CUSTOPT_0_DHCP_ENABLE          0x1UL

/// Carveout RAM
#define TEGRA_DEVKIT_BCT_CARVEOUT_0_MEMORY_RANGE    27:24
#define TEGRA_DEVKIT_BCT_CARVEOUT_0_MEMORY_DEFAULT  0x0UL
#define TEGRA_DEVKIT_BCT_CARVEOUT_0_MEMORY_1        0x1UL
#define TEGRA_DEVKIT_BCT_CARVEOUT_0_MEMORY_2        0x2UL
#define TEGRA_DEVKIT_BCT_CARVEOUT_0_MEMORY_3        0x3UL
#define TEGRA_DEVKIT_BCT_CARVEOUT_0_MEMORY_4        0x4UL
#define TEGRA_DEVKIT_BCT_CARVEOUT_0_MEMORY_5        0x5UL
#define TEGRA_DEVKIT_BCT_CARVEOUT_0_MEMORY_6        0x6UL
#define TEGRA_DEVKIT_BCT_CARVEOUT_0_MEMORY_7        0x7UL
#define TEGRA_DEVKIT_BCT_CARVEOUT_0_MEMORY_8        0x8UL //32 MB
#define TEGRA_DEVKIT_BCT_CARVEOUT_0_MEMORY_9        0x9UL //48 MB
#define TEGRA_DEVKIT_BCT_CARVEOUT_0_MEMORY_A        0xaUL //64 MB
#define TEGRA_DEVKIT_BCT_CARVEOUT_0_MEMORY_B        0xbUL //128 MB
#define TEGRA_DEVKIT_BCT_CARVEOUT_0_MEMORY_C        0xcUL //256 MB

/// Total RAM
#define TEGRA_DEVKIT_BCT_SYSTEM_0_MEMORY_RANGE    31:28
#define TEGRA_DEVKIT_BCT_SYSTEM_0_MEMORY_DEFAULT  0x0UL // 512 MB
#define TEGRA_DEVKIT_BCT_SYSTEM_0_MEMORY_1        0x1UL // 256 MB
#define TEGRA_DEVKIT_BCT_SYSTEM_0_MEMORY_2        0x2UL // 512 MB
#define TEGRA_DEVKIT_BCT_SYSTEM_0_MEMORY_3        0x3UL // 1 GB
#define TEGRA_DEVKIT_BCT_SYSTEM_0_MEMORY_4        0x4UL
#define TEGRA_DEVKIT_BCT_SYSTEM_0_MEMORY_5        0x5UL
#define TEGRA_DEVKIT_BCT_SYSTEM_0_MEMORY_6        0x6UL
#define TEGRA_DEVKIT_BCT_SYSTEM_0_MEMORY_7        0x7UL
#define TEGRA_DEVKIT_BCT_SYSTEM_0_MEMORY_8        0x8UL

#if defined(__cplusplus)
}
#endif

#endif
