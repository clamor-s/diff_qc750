/*
 * Copyright (c) 2009 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/* This file implements weakly-linked default versions of the ODM query
 * functions for the Nv3P USB transport descriptors
 */

#include "nvcommon.h"
#include "nv3p_transport_usb_descriptors.h"

/* HACKHACKHACK -- To allow clients of this interface to override the
 * device & config descriptors with client-specific data, an ODM function
 * is locally declared here and weakly-linked.  This trick only works for
 * RVDS and aos-gcc builds, but that's sufficient for Android.  If we need
 * to support this more generally in the future, this should be cleaned up. */

#if defined(__arm) || defined(__GNUC__)

#include "nvodm_query_nv3p.h"

NV_WEAK NvBool NvOdmQueryUsbConfigDescriptorInterface(
    NvU32* pIfcClass,
    NvU32* pIfcSubclass,
    NvU32* pIfcProtocol);

NV_WEAK NvBool NvOdmQueryUsbDeviceDescriptorIds(
    NvU32 *pVendorId,
    NvU32 *pProductId,
    NvU32 *pBcdDeviceId);

NV_WEAK NvU8 *NvOdmQueryUsbProductIdString(void);

NV_WEAK NvU8 *NvOdmQueryUsbSerialNumString(void);

NvOdmUsbDeviceDescFn Nv3pUsbGetDeviceDescFn(void)
{
    return NvOdmQueryUsbDeviceDescriptorIds;
}
NvOdmUsbConfigDescFn Nv3pUsbGetConfigDescFn(void)
{
    return NvOdmQueryUsbConfigDescriptorInterface;
}

NvOdmUsbProductIdFn Nv3pUsbGetProductIdFn(void)
{
    return NvOdmQueryUsbProductIdString;
}

NvOdmUsbSerialNumFn Nv3pUsbGetSerialNumFn(void)
{
    return NvOdmQueryUsbSerialNumString;
}
#else
NvOdmUsbDeviceDescFn Nv3pUsbGetDeviceDescFn(void)
{
    return NULL;
}
NvOdmUsbConfigDescFn Nv3pUsbGetConfigDescFn(void)
{
    return NULL;
}

NvOdmUsbProductIdFn Nv3pUsbGetProductIdFn(void)
{
    return NULL;
}

NvOdmUsbSerialNumFn Nv3pUsbGetSerialNumFn(void)
{
    return NULL;
}
#endif



