/*
 * Copyright (c) 2010-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

/**
 * @file
 * <b>NVIDIA Microboot Interface</b>
 *
 * @b Description: Defines the interface for writing the ODM-configurable parts
 *                 of Microboot.
 */

#ifndef INCLUDED_MICROBOOT_H
#define INCLUDED_MICROBOOT_H

#include "nvcommon.h"
#include "nvodm_query.h"
#include "nvuart.h"

/**
 * @defgroup nv_microboot Microboot APIs
 *
 * This is the Microboot interface, which supports low-power booting.
 * @{
 */

/**
 * @brief Main function of Microboot. This function is ODM-configurable.
 */
void NvMicrobootMain(void);

/**
 * @brief Detects the USB charger.
 *
 * Detects the USB charger according to the "USB Battery Charging
 * Specification" version 1.0. The result can be read through
 * NvMicrobootUsbfGetChargerType().
 *
 * @returns NV_TRUE if an USB charger is connected
 */
NvBool NvMicrobootDetectUsbCharger(void);

/**
 * @brief Gets the USB enumerator from the USB host.
 *
 * @returns NV_TRUE if the configuration is accepted by the USB host.
 */
NvBool NvMicrobootUsbEnumerate(void);

/**
 * @brief Reads out the result of NvMicrobootDetectUsbCharger().
 *
 * @returns The charger type in ::NvOdmUsbChargerType.
 */
NvOdmUsbChargerType NvMicrobootUsbfGetChargerType(void);

/**
 * @brief Tries to read one character from debug UART.
 *
 * @returns The character if it was ready, or 0 otherwise.
 */
int NvMicrobootUartPoll(void);

/**
 * @brief Prints the message on debug UART.
 *
 * @param format A pointer to the format; the same as printf().
 */
void NvMicrobootPrintf(const char *format, ...);

#define MICROBOOT_DEBUG_ENABLE 1
#define MICROBOOT_DEBUG_TIME_ENABLE 1

#if MICROBOOT_DEBUG_ENABLE
  #define MICROBOOT_PRINTF(X) NvMicrobootPrintf X
  #define MICROBOOT_PRINT_U8(X) NvMicrobootPrintU8 X
  #define MICROBOOT_PRINT_U32(X) NvMicrobootPrintU32 X
#else
  #define MICROBOOT_PRINTF(X)
  #define MICROBOOT_PRINT_U8(X)
  #define MICROBOOT_PRINT_U32(X)
#endif

extern NV_ALIGN(4) NvU8 g_NvMicrobootUsbDeviceDescriptor[];
extern NV_ALIGN(4) NvU8 g_NvMicrobootUsbConfigDescriptor[];
extern NV_ALIGN(4) NvU8 g_NvMicrobootUsbManufacturerID[];
extern NV_ALIGN(4) NvU8 g_NvMicrobootUsbProductID[];
extern NV_ALIGN(4) NvU8 g_NvMicrobootUsbSerialNumber[];
extern NV_ALIGN(4) NvU8 g_NvMicrobootUsbLanguageID[];
extern NV_ALIGN(4) NvU8 g_NvMicrobootUsbDeviceQualifier[];
extern NV_ALIGN(4) NvU8 g_NvMicrobootUsbDeviceStatus[];
/** @} */
#endif // INCLUDED_MICROBOOT_H
