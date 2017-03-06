/*
 * Copyright (c) 2008 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
 #ifndef INCLUDED_NV3P_PRIVATE_H
 #define INCLUDED_NV3P_PRIVATE_H

#include "nv3p_transport.h"
#if defined(__cplusplus)
extern "C"
{
#endif

NvError Nv3pTransportReopenAp20(Nv3pTransportHandle *hTrans);

NvError Nv3pTransportOpenAp20(Nv3pTransportHandle *hTrans, NvU32 TimeOut);

void Nv3pTransportCloseAp20(Nv3pTransportHandle hTrans);

NvError
Nv3pTransportSendAp20(
    Nv3pTransportHandle hTrans,
    NvU8 *data,
    NvU32 length,
    NvU32 flags,
    NvU32 TimeOut);

NvError
Nv3pTransportReceiveAp20(
    Nv3pTransportHandle hTrans,
    NvU8 *data,
    NvU32 length,
    NvU32 *received,
    NvU32 flags,
    NvU32 TimeOut);

NvOdmUsbChargerType
Nv3pTransportUsbfGetChargerTypeAp20(
    Nv3pTransportHandle *hTrans,
    NvU32 instance );

NvBool
Nv3pTransportUsbfCableConnectedAp20(
    Nv3pTransportHandle *hTrans,
    NvU32 instance );

void
Nv3pTransportSetChargingModeAp20(
    NvBool ChargingMode);



NvError Nv3pTransportReopenT30(Nv3pTransportHandle *hTrans);

NvError Nv3pTransportOpenT30(Nv3pTransportHandle *hTrans, NvU32 TimeOut);

void Nv3pTransportCloseT30(Nv3pTransportHandle hTrans);

NvError
Nv3pTransportReceiveIfCompleteT30(
    Nv3pTransportHandle hTrans,
    NvU8 *data,
    NvU32 length,
    NvU32 *received);

NvError
Nv3pTransportReceiveIfCompleteAp20(
    Nv3pTransportHandle hTrans,
    NvU8 *data,
    NvU32 length,
    NvU32 *received);

NvError
Nv3pTransportSendT30(
    Nv3pTransportHandle hTrans,
    NvU8 *data,
    NvU32 length,
    NvU32 flags,
    NvU32 TimeOut);

NvError
Nv3pTransportReceiveT30(
    Nv3pTransportHandle hTrans,
    NvU8 *data,
    NvU32 length,
    NvU32 *received,
    NvU32 flags,
    NvU32 TimeOut);

NvOdmUsbChargerType
Nv3pTransportUsbfGetChargerTypeT30(
    Nv3pTransportHandle *hTrans,
    NvU32 instance );

NvBool
Nv3pTransportUsbfCableConnectedT30(
    Nv3pTransportHandle *hTrans,
    NvU32 instance );

void
Nv3pTransportSetChargingModeT30(
    NvBool ChargingMode);


#if defined(__cplusplus)
}
#endif

#endif //INCLUDED_NV3P_PRIVATE_H

