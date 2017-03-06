/*
* Copyright (c) 2009-2010 NVIDIA Corporation.  All rights reserved.
*
* NVIDIA Corporation and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA Corporation is strictly prohibited.
*/

/**
 * @file nvddk_audiohub.h
 *
 * NvDdkAudioHub is NVIDIA's audiohub interface for controlling and managing
 * audiohub connections between various devices
 *
*/

#ifndef INCLUDED_NVDDK_AUDIOHUB_H
#define INCLUDED_NVDDK_AUDIOHUB_H

#include "nvodm_audiocodec.h"
#include "nvodm_query.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 *  NvDdkAudioHubHandle is an opaque context to the NvDdkAudioHub interface
 */
typedef struct NvDdkAudioHubRec*  NvDdkAudioHubHandle;

/**
 * NvDdkAudioConnectionLine structure is used to specify the
 * devices connections needed based on use case
 *
 * Most of the needed use case are specified in the query file
 * based on the NvOdmAudioConnectionIndex enum specified by query header file
 * Passed any needed NvOdmAudioConnectionIndex as IndexToConnectionTable
 * variable. If the connection line is used for any specific use case.
 * Otherwise specify the IndexToConnectionTable variable as
 * NvOdmAudioConnectionIndex_Unknown
 *
 * In addition to the audio device connection table specified using the
 * NvOdmAudioConnectionIndex enum, if that particular connection requires
 * to do recording the needed signalType source has to specified in the
 * SignalType variable and the PortIndex of the source along with it.
 *
*/
typedef struct NvDdkAudioConnectionLineRec
{
    /**Specify the source type used for recording or any special case */
    NvOdmAudioSignalType SignalType;

    /**Specify the portIndex of the Source used in the Signaltype */
    NvU32  PortIndex;

    /** Boolean to specify to enable or disable the connection line*/
    NvBool IsEnable;

    /** Specify the NvOdmAudioConnectionIndex enum based on the use case or
     * Specify NvOdmAudioConnectionIndex_Unknown if connection index is used
    */
    NvU32  IndexToConnectionTable;

}NvDdkAudioConnectionLine;

/**
 * @brief  This function must be called before using any other functions given
 * to access audio hub connection.
 * Allocate the memory for the NvDdkAudiohub structure.
 * Set up the internal registers and controllers needed for hub support.
 * Setup the default connection supporting the music path
 *
 *
 * @see NvDdkAudioHubOpen()
 *
 * @param phHub Specifies a pointer to a variable in which the hub handle is
 * stored.
 *
 * @retval NvSuccess Indicates the controller successfully initialized.
 * @retval NvError_InsufficientMemory Indicates that function fails to allocate
 * the memory for handle.
 * @retval NvError_MemoryMapFailed Indicates that the memory mapping for
 * controller register failed.
 */
NvError NvDdkAudioHubOpen(NvDdkAudioHubHandle* pHub);

/**
 * @brief Closes the Audio Hub.
 *
 * This method must be called when the client has completed using NvDdkAudioHub
 * Any resources allocated for this instance of the interface are released.
 *
 * @param hHub handle to the Audio Hub structure
 *
 ** @retval NvSuccess Indicates the controller successfully uninitialize.
 */
NvError NvDdkAudioHubClose(NvDdkAudioHubHandle hHub);

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
* @retval NvError_Failed Indicates that function fails to return
* the connectionLine
*/
NvError NvDdkAudioHubGetConnection(
        NvDdkAudioHubHandle hHub,
        NvDdkAudioConnectionLine* pConnectionLine);

/**
 * @brief Function that help to do the audio device connections and necessary
 * changes in the source or destination properties based on the connection
 * request.
 *
 * pConnectionLine IndexToConnectionTable variable specify the type of enum,
 * which is used to describe the connection as per use case.
 * First Check the IndexToConnectionTable is a valid enum.
 * Once it is a valid enum,obtain the corresponding connection Table from the
 * nvodm_query file. The connection line will specify whether the Source or
 * destination are acting as Master or Slave.
 * In addition to setting the device connection based on the table,
 * also need to set the Source/Destination properties like samplerate/ bitrate
 * / master /slave modes accordingly.
 * Once the table connection are taken care of, then check whether this
 * connection need any recording path enabled.
 * Check SignalType & PortIndex variables of the Structure to use
 * appropriate record source.
 * if IndexToConnectionTable is not a valid,  this is will be uses as a special
 * request for having a MicIn/LineIn or Speaker enable/disablerequest
 *
 * On disabling the connection line, the audio device connection and properties
 * are set back to the default Music path connection.
 *
 * @param hHub handle which is allocated by the driver after calling
 * the NvDdkAudioHubOpen()
 * @param pConnectionLine pointer to the ConnectionLine structure which the
 *   is used to establish the needed connections
 *
 * @retval NvSuccess Indicates the operation succeeded.
 * @retval NvError_NotSupported Indicates that requested functionality is not
 * supported.
 */
NvError NvDdkAudioHubSetConnection(
        NvDdkAudioHubHandle hHub,
        NvDdkAudioConnectionLine* pConnectionLine);

/**
 * @brief Power mode call for Audio Hub controller. This will help to set
 * the state of the dap port to normal or tristate
 *
 * To reduce the power consumption set to tristate or normal
 * based on the powermode.
 *
 * @param hHub handle which is allocated by the driver after calling
 * the NvDdkAudioHubOpen()
 * @param IsTriStateMode Set whether the Hub port to normal or tristate
 * based on powermode

 * @retval NvSuccess Indicates the operation succeeded.
 * @retval NvError_NotSupported Indicates that requested functionality is not
 * supported.
 */
NvError NvDdkAudioHubPowerMode(
        NvDdkAudioHubHandle hHub,
        NvBool IsTriStateMode);

#if defined(__cplusplus)
}
#endif

#endif //INCLUDED_NVDDK_AUDIODAP_H


