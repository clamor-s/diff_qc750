/*
* Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
*
* NVIDIA Corporation and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA Corporation is strictly prohibited.
*/ 

/**
 * @file nvddk_audiodap.h
 *
 * NvDdkAudioDap is NVIDIA's audiodap interface for controlling and managing
 * dap connections between various devices
 *
*/

#ifndef INCLUDED_NVDDK_AUDIODAP_H
#define INCLUDED_NVDDK_AUDIODAP_H

#include "nvodm_audiocodec.h"
#include "nvodm_query.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/** 
 *  NvDdkAudioDapHandle is an opaque context to the NvDdkAudioDap interface
 */
typedef struct NvDdkAudioDapRec*  NvDdkAudioDapHandle;

/** 
 * NvDdkAudioDapConnectionLine structure is used to specify the
 * Dap connection lines needed based on use case
 *
 * Most of the needed use case are specified in the query file
 * based on the NvOdmDapConnectionIndex enum specified by query header file
 * Passed any needed NvOdmDapConnectionIndex as IndexToConnectionTable variable
 * if the connection line is used for any specific use case. Otherwise spesify the
 * IndexToConnectionTable variable as NvOdmDapConnectionIndex_Unknown
 *
 * In addition to the dap connection table specified using the NvOdmDapConnectionIndex
 * enum, if that particular connection requires to do recording the needed signalType source
 * has to specified in the SignalType variable and the PortIndex of the source along with it.
 *
*/
typedef struct NvDdkAudioDapConnectionLineRec
{
    /**Specify the source type used for recording or any special case */
    NvOdmAudioSignalType SignalType;

    /**Specify the portIndex of the Source used in the Signaltype */
    NvU32  PortIndex;

    /** Boolean to specify to enable or disable the connection line*/
    NvBool IsEnable;

    /** Specify the NvOdmDapConnectionIndex enum based on the use case or
     * Specify NvOdmDapConnectionIndex_Unknown if connection index is used
    */
    NvU32  IndexToConnectionTable;

}NvDdkAudioDapConnectionLine;

/**
 * @brief  This function must be called before using any other functions given to 
 * access audio dap connection. 
 * Allocate the memory for the NvDdkAudioDap structure.
 * Set up the das/dap registers and controllers.
 * Query done for codec to verify whether to use VoiceDac or not, whether the codec is
 * Master or Slave by default.
 * Parse the Default DapPropertyTable and determine the corresponding Dap type as HifiCodec
 * or VoiceCodec , etc.
 * Setup the default connection supporting the music path
 *
 *
 * @see NvDdkAudioDapClose()
 *
 * @param hRmDevice handle to the Rm device which is required by Dap connection to 
 * acquire the resources from RM.
 * @param hDriverI2s handle of the opened I2S channel.
 * @param I2sInstanceID InstanceID used for I2S channel
 * @param hAudioCodec handle of the opened AudioCodec
 * @param CodecInstanceID InstanceID used for the AudioCodec 
 * @param phDap Specifies a pointer to a variable in which the dap handle is
 * stored.
 * 
 * @retval NvSuccess Indicates the controller successfully initialized.
 * @retval NvError_InsufficientMemory Indicates that function fails to allocate 
 * the memory for handle.
 * @retval NvError_MemoryMapFailed Indicates that the memory mapping for 
 * controller register failed.
 */
NvError NvDdkAudioDapOpen(NvRmDeviceHandle hRmDevice,
                     void *hDriverI2s,
                     NvU32 I2sInstanceID,
                     NvOdmAudioCodecHandle hAudioCodec,
                     NvU32 CodecInstanceID,
                     NvDdkAudioDapHandle* phDap);

/**
 * @brief Closes the Audio Dap.
 *
 * This method must be called when the client has completed using NvDdkAudioDap.
 * Any resources allocated for this instance of the interface are released.
 *
 * @param hDisp handle to the Audio dap structure
 *
 */
void NvDdkAudioDapClose(NvDdkAudioDapHandle hDap);

/**
*  @brief Function that help to return particular connectionLine
*
*  @param hDap Handle opened using the NvDdkAudioDapOpen call
*  @param pConnectionLine pointer to the ConnectionLine structure which the
*   ddk can return the connectionLine
*
* @retval NvSuccess Indicates the controller successfully returned.
* @retval NvError_NotSupported Indicates the requested property is not
* supported.
* @retval NvError_Failed Indicates that function fails to return the connectionLine
*/
NvError NvDdkAudioDapGetConnection(NvDdkAudioDapHandle hDap, NvDdkAudioDapConnectionLine* pConnectionLine);

/**
 * @brief Function that help to do the dap connections and necessary changes in the
 * source or destination properties based on the connection request.
 *
 * pConnectionLine IndexToConnectionTable variable specify the type of enum which is used to 
 * describe the connection as per use case.
 * First Check the IndexToConnectionTable is a valid enum
 * Once it is a valid enum , obtain the corresponding connection Table from the nvodm_query file.
 * The connection line will specify whether the Source or destination are acting as Master or Slave
 * In addition to setting the Dap connection based on the table, also need to set the Source/Destination
 * properties like samplerate/ bitrate / master /slave modes accordingly.
 * Once the table connection are taken care of, then check whether this connection need any recording path enabled.
 * Check SignalType & PortIndex variables of the Structure to use appropriate record source
 * if IndexToConnectionTable is not a valid,  this is will be uses as a special request
 * for having a MicIn/LineIn or Speaker enable/disable request
 * 
 * On disabling the connection line, the dap connection and properties are set back to the
 * default Music path connection
 *
 * @param hDap handle which is allocated by the driver after calling
 * the NvDdkAudioDapOpen() 
 * @param pConnectionLine pointer to the ConnectionLine structure which the
 *   is used to establish the needed connections
 *
 * @retval NvSuccess Indicates the operation succeeded.
 * @retval NvError_NotSupported Indicates that requested functionality is not 
 * supported.
 */
NvError NvDdkAudioDapSetConnection(NvDdkAudioDapHandle hDap, NvDdkAudioDapConnectionLine* pConnectionLine);

/**
 * @brief Power mode call for Dap controller. This will help to set the state
 * of the dap port to normal or tristate
 *
 * To reduce the power consumption set the HifiCodec Dapport to tristate or normal
 * based on the powermode.
 *
 * @param hDap handle which is allocated by the driver after calling
 * the NvDdkAudioDapOpen() 
 * @param IsTriStateMode Set whether the dap port to normal or tristate based on powermode

 * @retval NvSuccess Indicates the operation succeeded.
 * @retval NvError_NotSupported Indicates that requested functionality is not 
 * supported.
 */
NvError NvDdkAudioDapPowerMode(NvDdkAudioDapHandle hDap, NvBool IsTriStateMode);

#if defined(__cplusplus)
}
#endif

#endif //INCLUDED_NVDDK_AUDIODAP_H


