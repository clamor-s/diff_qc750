/*
 * Copyright (c) 2007-2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVODM_IMAGER_I2C_H
#define INCLUDED_NVODM_IMAGER_I2C_H

#include "nvodm_services.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#define NVODM_IMAGER_I2C_PORT (1)

typedef struct NvOdmImagerI2cConnectionRec {
    NvU32 DeviceAddr;
    NvU32 I2cPort;
    NvOdmServicesI2cHandle hOdmI2c;
} NvOdmImagerI2cConnection, *PNvOdmImagerI2cConnection;

// Function declaration
NvOdmI2cStatus NvOdmImagerI2cAck(
    NvOdmImagerI2cConnection *i2cHandle);

NvOdmI2cStatus NvOdmImagerI2cWrite8(
   NvOdmImagerI2cConnection *i2cHandle,
   NvU8 Addr,
   NvU8 Data);

NvOdmI2cStatus NvOdmImagerI2cWriteBuffer16(
    NvOdmImagerI2cConnection *i2cHandle,
    NvU32 *WriteBuffer,
    NvU32 Length);

NvOdmI2cStatus NvOdmImagerI2cRead8(
   NvOdmImagerI2cConnection *i2cHandle,
   NvU8 Addr,
   NvU8 *Data);

NvOdmI2cStatus NvOdmImagerI2cRead8_w16(
    NvOdmImagerI2cConnection *i2cHandle,
    NvU16 Addr,
    NvU16 *Data);

NvOdmI2cStatus NvOdmImagerI2cWrite16(
   NvOdmImagerI2cConnection *i2cHandle,
   NvU16 Addr,
   NvU16 Data);

NvOdmI2cStatus NvOdmImagerI2cWriteDev16(
   NvOdmImagerI2cConnection *i2cHandle,
   NvU16 Data);

NvOdmI2cStatus NvOdmImagerI2cWrite16Data8(
   NvOdmImagerI2cConnection *i2cHandle,
   NvU16 Addr,
   NvU8 Data);

NvOdmI2cStatus NvOdmImagerI2cWrite16Data8_W16(
   NvOdmImagerI2cConnection *i2cHandle,
   NvU16 Addr,
   NvU16 Data);

NvOdmI2cStatus NvOdmImagerI2cRead16(
   NvOdmImagerI2cConnection *i2cHandle,
   NvU16 Addr,
   NvU16 *Data);

NvOdmI2cStatus NvOdmImagerI2cReadDev16(
   NvOdmImagerI2cConnection *i2cHandle,
   NvU16 *Data);

NvOdmI2cStatus NvOdmImagerI2cRead16Data8(
   NvOdmImagerI2cConnection *i2cHandle,
   NvU16 Addr,
   NvU8 *Data);

NvOdmI2cStatus NvOdmImagerI2cWrite32(
   NvOdmImagerI2cConnection *i2cHandle,
   NvU16 Addr,
   NvU32 Data);

NvOdmI2cStatus NvOdmImagerI2cRead32(
   NvOdmImagerI2cConnection *i2cHandle,
   NvU16 Addr,
   NvU32 *Data);

void NvOdmImagerI2cGetRegisterDump(
   NvOdmImagerI2cConnection *i2cHandle,
   NvU32 *pAddresses,
   NvU32 *pValues,
   NvU32 Count);

#if defined(__cplusplus)
}
#endif

/** @} */

#endif // INCLUDED_NVODM_IMAGER_I2C_H

