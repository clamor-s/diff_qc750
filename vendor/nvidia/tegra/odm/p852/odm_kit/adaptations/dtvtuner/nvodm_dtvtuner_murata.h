/*
 * Copyright (c) 2007-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVODM_TVTUNER_MURATA_H
#define INCLUDED_NVODM_TVTUNER_MURATA_H

#if defined(__cplusplus)
extern "C"
{
#endif

/** Murata SUMUDDJ-LS057 ISDB-T tuner. */
#define DTV_MURATA_057_GUID     (NV_ODM_GUID('m','u','r','a','t','a','5','7'))

void NvOdmMurataSetPowerLevel(
        NvOdmDeviceHWContext *pHW,
        NvOdmDtvtunerPowerLevel PowerLevel);

NvBool NvOdmMurataGetCap(
          NvOdmDeviceHWContext *pHW,
          NvOdmDtvtunerCap *pCap);

NvBool NvOdmMurataInit(
          NvOdmDeviceHWContext *pHW,
          NvOdmISDBTSegment seg,
          NvU32 channel);

NvBool NvOdmMurataSetSegmentChannel(
         NvOdmDeviceHWContext *pHW,
         NvOdmISDBTSegment seg,
         NvU32 channel,
         NvU32 PID);

NvBool NvOdmMurataSetChannel(
         NvOdmDeviceHWContext *pHW,
         NvU32 channel,
         NvU32 PID);

NvBool NvOdmMurataGetSignalStat(
         NvOdmDeviceHWContext *pHW,
         NvOdmDtvtunerStat *SignalStat);

#if defined(__cplusplus)
}
#endif

/** @} */

#endif // INCLUDED_NVODM_TVTUNER_MURATA_H

