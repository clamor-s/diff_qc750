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
 * @file
 * @brief <b>NVIDIA Driver Development Kit: 
 *         SPDIF NvDDK APIs</b>
 *
 * @b Description: Interface file for NvDdk SPDIF driver.
 * 
 */
 
#ifndef INCLUDED_NVDDK_SPDIF_H
#define INCLUDED_NVDDK_SPDIF_H

/**
 * @defgroup nvddk_spdif Sony/Philiphs digital interface (SPDIF) Driver API
 * 
 * This is the SPDIF driver interface.
 * This interface provides the SPDIF data communication channel configuration,
 * basic data transfer (receive and transmit), receive and transmit data flow.
 * The read and write interface can be blocking/non blocking type based on the
 * passed parameter.  The read and write request can be queued to make the data
 * receive/transmit continuation.
 *
 * @ingroup nvddk_modules
 * @{
 */

#include "nvcommon.h"
#include "nvos.h"
#include "nvrm_init.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** NvDdkSpdifHandle is an opaque context to the NvDdkSpdifRec interface
 */
typedef struct NvDdkSpdifRec *NvDdkSpdifHandle;

/**
 * @brief Defines the placing of valid bits in a Spdif data word. Spdif data word
 * is length of SpdifDataWordSizeInBits. The valid bits can start from lsb side
 * or msb side or it can be in packed format.
 * 
 */
typedef enum
{
    /// Valid bit starts from lsb in a given Spdif data word.
    NvDdkSpdifDataWordValidBits_StartFromLsb = 0x1,

    /// Valid bit starts from msb in a given Spdif data word.
    NvDdkSpdifDataWordValidBits_StartFromMsb,

    /// The Spdif data word contain the right and left channel of data in the 
    /// packed format. The half of the data is for the left channel and half of
    /// the data is for right channel.
    /// The Valid bits in the Spdif data word should be half of the Spdif data word
    // for the packed format.
    NvDdkSpdifDataWordValidBits_InPackedFormat,

    NvDdkSpdifDataWordValidBits_Force32 = 0x7FFFFFFF
} NvDdkSpdifDataWordValidBits;

/**
 * @brief Combines the buffer addresses, requested size and the transfer status.
 * 
 */
typedef struct
{
    /// Holds the Physical address of the data buffer.
    /// The address should be alligned to the SpdifDataWordSizeInBits from 
    /// capability structure.
    NvRmPhysAddr BufferPhysicalAddress;
    
    /// Holds the number of bytes Requested for transfer. The number of bytes requested 
    /// should be multiple of block length. As per Spdif standard, 192 samples are referred as one block.
    /// Each block has 24 bytes of channel data and 32 bytes of user data associated.
    NvU32 BytesRequested;
    
    /// Holds the virtual address of the channel status buffer.
    NvU32* pChannelStatusBuffer;
    
    /// Holds the virtual address of the user data buffer.
    NvU32* pUserDataBuffer;
    
    /// Holds the block count, which gives the channel and user data buffer size 
    /// in corresponding units. For one block of data, the channel status data length is 24bytes 
    /// and User data length is 32bytes. If BlockCount is 2, the memory allocated for pChannelStatusBuffer
    /// and pUserDataBuffer are 48 and 64 bytes respectively.
    NvU32 BlockCount;
    
    /// Holds the number of bytes transferred yet.
    NvU32 BytesTransferred;
    
    /// Hold the information about the recent transfer status
    NvError TransferStatus;
} NvDdkSpdifClientBuffer;

/**
 * @brief Combines the capability and limitation of the Spdif driver.
 * 
 */
typedef struct
{
    /// Holds the Total number of Spdif channel supported by the driver.
    NvU32 TotalNumberOfSpdifChannel;

    /// Holds the Spdif channel instance Id for which this capability structure 
    /// contains the data.
    NvU32 SpdifChannelId;

    /// Holds whether packed mode is supported or not  i.e. half of the data is
    /// for right channel and half of the data is for left channel. The Spdif 
    /// data word size will be of SPDIFDataWordSizeInBits from capability 
    /// structure.
    NvBool IsValidBitsInPackedModeSupported;

    /// Holds whether valid bits in the Spdif data word start from Msb is 
    /// supported or not.
    NvBool IsValidBitsStartsFromMsbSupported;

    /// Holds whether valid bits in the Spdif data word start from Lsb is 
    /// supported or not.
    NvBool IsValidBitsStartsFromLsbSupported;

    /// Holds whether Mono data format is supported or not  i.e. single Spdif
    //data 
    /// word which can go to both right and left channel.
    NvBool IsMonoDataFormatSupported;

    /// Holds whether loopback mode is supported or not.This can be used for
    /// self test.
    NvBool IsLoopbackSupported;

    /// Holds whether the channel data transmitting/receving is supported or not.
    NvBool IsChannelDataSupported;

    /// Holds whether the user data transmitting/receving is supported or not.
    NvBool IsUserDataSupported;

    /// Holds the information for the Spdif data word size in bits.  The client 
    /// should passed the data buffer such that each Spdif data word should match
    /// with this value.
    /// All passed physical address and bytes requested size should be aligned 
    /// to this value.
    NvU32 SpdifDataWordSizeInBits;
} NvDdkSpdifDriverCapability;

/**
 * @brief Combines the play and record configuration parameter. This will
 * depend on the decoder of the songs or the recording configuration for a
 * given songs.
 *           
 */
typedef struct
{
    /// Holds whether the data is mono data format or not.
    NvBool IsMonoDataFormat;

    /// Holds the whether the valid data bits in data buffer is start from
    /// lsb or msb or in packed format.  If is packed format
    /// (NvDdkSPDIFDataWordValidBits_Packed) then half of the data of size
    /// DataBufferWidth (In caps structure) is for left channel
    /// and half of the data of size DataBufferWidth (In caps structure) is
    /// for right channel. In this case the SpdifDataWIdth should be half of
    /// the DataBufferWidth.
    NvDdkSpdifDataWordValidBits SpdifDataWordFormat;

    /// Holds the valid bits in Spdif data for one sample.
    /// If packed mode is selected then it should be half of the
    //DataBufferWidth.
    /// If non packed mode is selected then the valid data bits will be taken 
    /// from the LSB side of data buffer or msb side of the data buffer based 
    /// on SpdifDataBufferFormat.
    NvU32 ValidBitsInSpdifDataWord;

    /// Holds the sampling rate. Based on sampling rate it generates the clock 
    /// and sends the data if it is in master mode. This parameter will be 
    /// ignored when slave mode is selected.
    NvU32 SamplingRate;
} NvDdkSpdifDataProperty;

/**
 * @brief  Gets the Spdif driver capabilities. Client can get the capabilities of
 * the driver to support the different functionalities and configure the data 
 * communication.
 *
 * Client will allocate the memory for the capability structure and send the
 * pointer to this function.  This function will copy the capabilities into
 * capability structure pointed by the passed pointer.
 * 
 * Client can get the total number of spdif channel by passing the ChanenlId as
 * 0.  If client passed ChannelId as 0 with valid pointer to
 * pSpdifDriverCapabilities then this function will fill the TotalNumber Of Spdif
 * channel in the corresponding member of pSpdifDriverCapabilities structure.
 *
 * @see NvDdkSpdifDriverCapability
 *
 * @param hDevice Handle to the Rm device which is required by Ddk to acquire 
 * the capability from the device capability structure.
 * @param ChannelId Specifies a channel id for which capabilities are required.
 * Pass 0 if the total number of Spdif channel is required to query.
 * @param pSpdifDriverCapabilities Specifies a pointer to a variable in which the
 * spdif driver capability will be stored.
 * 
 * @retval NvSuccess Indicates the operation is success.
 * @retval NvError_NotSupported Indicates that the channel Id is more than 
 * supported channel Id.
 *
 */
NvError 
NvDdkSpdifGetCapabilities(
    NvRmDeviceHandle hDevice,
    NvU32 ChannelId,
    NvDdkSpdifDriverCapability * const pSpdifDriverCapabilities);

/**
 * @brief  Opens the Spdif channel and returns a handle of the opened channel to
 * the client. This is specific to an spdif channel Id. This handle should be
 * passed to other API's. This function initializes the controller, enables the
 * clock to the Spdif controller and puts the driver into determined state.
 *
 * It returns the same existing handle if it is already opened.
 *
 * @see NvDdkSpdifClose()
 *
 * @param hDevice Handle to the Rm device which is required by Ddk to 
 * acquire the resources from RM.
 * @param SpdifChannelId Specifies the Spdif channel ID for which handle is
 * required. Valid channel Id start from 0 and maximum upto the 
 * TotalNumberOfSpdifChannel -1.
 * @param phSpdif Specifies a pointer to a variable in which the handle is
 * stored.
 * 
 * @retval NvSuccess Indicates the controller successfully initialized.
 * @retval NvError_InsufficientMemory Indicates that function fails to allocate 
 * the memory for handle.
 * @retval NvError_MemoryMapFailed Indicates that the memory mapping for 
 * controller register failed.
 * @retval NvError_MutexCreateFailed Indicates that the creation of mutex
 * failed. 
 * Mutex is required to provide the thread safety.
 * @retval NvError_SemaphoreCreateFailed Indicates that the creation of 
 * semaphore failed. Semaphore is required to provide the synchronous
 * operation.
 */
NvError 
NvDdkSpdifOpen(
    NvRmDeviceHandle hDevice,
    NvU32 SpdifChannelId,
    NvDdkSpdifHandle *phSpdif);

/**
 * @brief  Deinitialize the spdif channel, disable the clock and releases the Spdif
 * handle. It also releases all the OS resources which were allocated when 
 * creating the handle.
 *
 * @note The number of close () should be same as number of open() to
 * completely release of the resources. If number of close is not equal to
 * number of open() calls then it will not relases the allocated resources.
 *
 * @see NvDdkSpdifOpen()
 * 
 * @param hSpdif The Spdif handle which is allocated by the driver after calling 
 * the NvDdkSpdifOpen() 
 * 
 */
void NvDdkSpdifClose(NvDdkSpdifHandle hSpdif);

/**
 * @brief  Gets the Spdif data property.
 *
 * @see NvDdkSpdifOpen
 * @see NvDdkSpdifDataProperty
 * @see NvDdkSpdifSetDataProperty
 *
 * @param hSpdif The Spdif handle which is allocated by the driver after calling 
 * the NvDdkSpdifOpen() 
 * @param pSpdifDataProperty Pointer to the Spdif data property structure where the
 * current configured data property will be stored.
 *
 * @retval NvSuccess Indicates the operation succeeded.
 * @retval NvError_NotSupported Indicates the requested property is not
 * supported.
 * @retval NvError_InvalidState Indicates the Spdif is busy and currently it is 
 * transmitting/receiving the data.
 *
 */
NvError 
NvDdkSpdifGetDataProperty(
    NvDdkSpdifHandle hSpdif, 
    NvDdkSpdifDataProperty* const pSpdifDataProperty);

/**
 * @brief  Sets the Spdif data property. This configures the driver to
 * send/Receive
 * the data as per client requirement.
 * The client is advised to call first NvDdkSpdifGetDataProperty() to get
 * the default value of all the member and then change only those member which
 * need to be configured and then call this function.
 *
 * @see NvDdkSpdifOpen
 * @see NvDdkSpdifDataProperty
 * @see NvDdkSpdifGetDataProperty
 * 
 * @param hSpdif The Spdif handle which is allocated by the driver after calling 
 * the NvDdkSpdifOpen() 
 * @param pSpdifDataProperty Pointer to the Spdif data property structure which
 * have  
 * data property parameters.
 *
 * @retval NvSuccess Indicates the operation succeeded.
 * @retval NvError_NotSupported Indicates the requested property is not
 * supported.
 * @retval NvError_InvalidState Indicates the Spdif is busy and currently it is 
 * transmitting/receiving the data.
 *
 */
NvError 
NvDdkSpdifSetDataProperty(
    NvDdkSpdifHandle hSpdif, 
    const NvDdkSpdifDataProperty* pSpdifDataProperty);

/**
 * @brief  Starts the read operation and store the received data in the buffer
 * provided. This is blocking type/non blocking type call based on the argument
 * passed.  If No wait is selected then notification (through signalling the
 * semaphore) will be generated to the caller after the data transfer
 * completed.
 *
 * The read request will be queued, means the client can call the read() even 
 * if the previous request is not completed.
 * 
 * @see NvDdkSpdifOpen
 * @see NvDdkSpdifReadAbort
 * @see NvDdkSpdifGetAsynchReadTransferInfo
 *
 * @param hSpdif The Spdif handle which is allocated by the driver after calling
 * the 
 * NvDdkSpdifOpen() 
 * @param ReceiveBufferPhyAddress Physical address of the receive buffer where 
 * the received data will be stored. The address should be alligned to the 
 * I2sDataWordSizeInBits from capability structure. The data is directly
 * filled in the physical memory pointing to this address.
 * @param pBytesRead A pointer to the variable which stores the number of bytes
 * requested to read when it is called and store the actual number of bytes
 * read when return from the function.
 * @param WaitTimeoutInMilliSeconds The time need to wait in milliseconds. If
 * it is zero then it will be returned immediately as asynchronous operation.
 * If is non zero then it will wait for a requested timeout. If it is
 * NV_WAIT_INFINITE then it will wait for infinitely till transaction
 * completes.
 * @param AsynchSemaphoreId The semaphore Id which need to be signal if client
 * is requested  for asynchronous operation.  If it NULL then it will not
 * notify to the client and the next request will be continued from the request
 * queue.
 *
 * @retval NvSuccess Indicates the operation succeeded.
 * @retval NvError_NotInitialized Indicates that the Spdif channel is not opened. 
 * @retval NvError_Timeout Indicates the operation is not completed in a given 
 * timeout.
 * @retval NvError_SPDIFOverrun Indicates that overrun error occur during
 * receiving 
 * of the data.
 */
NvError 
NvDdkSpdifRead(
    NvDdkSpdifHandle hSpdif, 
    NvRmPhysAddr ReceiveBufferPhyAddress,
    NvU32 *pBytesRead,
    NvU32 WaitTimeoutInMilliSeconds,
    NvOsSemaphoreHandle AsynchSemaphoreId);

/**
 * @brief  Aborts the Spdif read operation. 
 *
 * This routine stop the ongoing data operation and kill all read request 
 * related to this handle.
 *           
 * @see NvDdkSpdifOpen
 * @see NvDdkSpdifRead
 * @see NvDdkSpdifGetAsynchReadTransferInfo
 *
 * @param hSpdif The Spdif handle which is allocated by the driver after calling
 * the 
 * NvDdkSpdifOpen() 
 * 
 */
void NvDdkSpdifReadAbort(NvDdkSpdifHandle hSpdif);

/**
 * @brief Get the receiving information which was happened recently. This 
 * function can be called if the client wants the asynchronous operation and 
 * after getting the signalled of the semaphore from DDK, he wants to know the 
 * receiving status and other information. The client need to pass the pointer 
 * to the client buffer where the last received buffer information will be 
 * copied. 
 *
 * @see NvDdkSpdifOpen
 * @see NvDdkSpdifRead
 * @see NvDdkSpdifReadAbort
 *
 * @note This function does not allocate the memory to store the last buffer 
 * information. So client need to allocate the memory and pass the pointer so 
 * that this function can copy the information.
 * 
 * @param hSpdif Handle to the Spdif context which is allocated from GetContext.
 * @param pSPDIFReceivedBufferInfo Pointer to the structure variable where 
 * the recent receiving status regarding the buffer will be stored.
 *
 * @retval NvSuccess Indicates the operation succeeded.
 * @retval NvError_NotInitialized Indicates that the Spdif channel is not opened. 
 * @retval NvError_InvalidState Indicates that there is no read call made by 
 * client.
 */
 
NvError 
NvDdkSpdifGetAsynchReadTransferInfo(
    NvDdkSpdifHandle hSpdif, 
    NvDdkSpdifClientBuffer *pSPDIFReceivedBufferInfo);

/**
 * @brief  Start transmitting the data which is available at the buffer
 * provided. 
 * This is blocking type/non blocking  type call based on the argument passed.
 * If zero timeout is selected then notification will be generated to the
 * caller 
 * after completes the data transmission.
 * If non-zero timeout is selected then it will wait maximum for a given
 * timeout 
 * for data transfer completion. It can also wait for forever based on the 
 * argument passed. 
 * 
 * The write operation can be called even if previous request has not been 
 * completed. It will queue the request.
 *
 * @see NvDdkSpdifOpen
 * @see NvDdkSpdifWriteAbort
 * @see NvDdkSpdifWritePause
 * @see NvDdkSpdifWriteResume
 * @see NvDdkSpdifGetAsynchWriteTransferInfo
 *
 * @param hSpdif The Spdif handle which is allocated by the driver after calling
 * the 
 * NvDdkSpdifOpen() 
 * @param TransmitBufferPhyAddress Physical address of the transmit buffer
 * where transmitted datas are stored.The address should be alligned to the 
 * I2sDataWordSizeInBits from capability structure. The data is directly
 * taken from the physical memory pointing to this address.
 * @param pBytesWritten A pointer to the variable which stores the number of 
 * bytes requested to write when it is called and store the actual number of 
 * bytes write when return from the function.
 * @param WaitTimeoutInMilliSeconds The time need to wait in milliseconds. If
 * it is zero then it will be returned immediately as asynchronous operation.
 * If is non zero then it will wait for a requested timeout. If it is
 * NV_WAIT_INFINITE then it will wait for infinitely till transaction
 * completes.
 * @param AsynchSemaphoreId The semaphore Id which need to be signal if client 
 * is requested  for asynchronous operation.  
 *
 * @retval NvSuccess Indicates the operation succeeded.
 * @retval NvError_NotInitialized Indicates that the spdif channel is not opened. 
 * @retval NvError_Timeout Indicates the operation is not completed in a given 
 * timeout.
 */

NvError 
NvDdkSpdifWrite(
    NvDdkSpdifHandle hSpdif, 
    NvRmPhysAddr TransmitBufferPhyAddress,
    NvU32 *pBytesWritten,
    NvU32 WaitTimeoutInMilliSeconds,
    NvOsSemaphoreHandle AsynchSemaphoreId);

/**
 * @brief  Aborts the Spdif write. 
 * This routine stop transmitting the data, kill all request for data send.
 *           
 * @see NvDdkSpdifOpen
 * @see NvDdkSpdifWrite
 * @see NvDdkSpdifWritePause
 * @see NvDdkSpdifWriteResume
 * @see NvDdkSpdifGetAsynchWriteTransferInfo
 *
 * @param hSpdif The Spdif handle which is allocated by the driver after calling
 * the 
 * NvDdkSpdifOpen() 
 * 
 */
void NvDdkSpdifWriteAbort(NvDdkSpdifHandle hSpdif);

/**
 * @brief Get the transmitting information which was happened recently. This
 * function can be called if the client wants the asynchronous operation and
 * after getting the signalled of the semaphore from DDK, he wants to know the
 * transmit status and other information. The client need to pass the pointer
 * to the client buffer where the last received buffer information will be
 * copied. 
 *
 * @note This function does not allocate the memory to store the last buffer 
 * information. So client need to allocate the memory and pass the pointer so  
 * that this function can copy the information.
 *
 * @see NvDdkSpdifOpen
 * @see NvDdkSpdifWriteAbort
 * @see NvDdkSpdifWritePause
 * @see NvDdkSpdifWriteResume
 * 
 * @param hSpdif Handle to the Spdif context which is allocated from GetContext.
 * @param pSPDIFSentBufferInfo Pointer to the structure variable where the 
 * recent transmit information regarding the buffer will be stored.
 *
 * @retval NvSuccess Indicates the operation succeeded.
 * @retval NvError_NotInitialized Indicates that the Spdif channel is not opened. 
 * @retval NvError_InvalidState Indicates that there is no write call made by 
 * client or write operation was aborted.
 */
     
NvError 
NvDdkSpdifGetAsynchWriteTransferInfo(
    NvDdkSpdifHandle hSpdif, 
    NvDdkSpdifClientBuffer *pSPDIFSentBufferInfo);

/**
 * @brief  Pause the Spdif write. The data transmission will be just paused and 
 * all state will be saved. User can resume to continue sending the data.
 * 
 * @see NvDdkSpdifOpen
 * @see NvDdkSpdifWrite
 * @see NvDdkSpdifWriteAbort
 * @see NvDdkSpdifWriteResume
 *
 * @param hSpdif The Spdif handle which is allocated by the driver after calling
 * the 
 * NvDdkSpdifOpen() 
 *
 * @retval NvSuccess Indicates the operation succeeded.
 * @retval NvError_NotInitialized Indicates that the Spdif channel is not opened. 
 * @retval NvError_InvalidState Indicates that there is no write call made by 
 * client or write operation was aborted.  
 */
NvError NvDdkSpdifWritePause(NvDdkSpdifHandle hSpdif);

/**
 * @brief  Resume the Spdif write. The data transmission will be continued from
 * paused state.This should be called after calling the NvDdkSpdifWritePause() to
 * resume the transmission.
 * 
 * @see NvDdkSpdifOpen
 * @see NvDdkSpdifWrite
 * @see NvDdkSpdifWriteAbort
 * @see NvDdkSpdifWritePause
 *
 * @param hSpdif The Spdif handle which is allocated by the driver after calling
 * the 
 * NvDdkSpdifOpen() 
 *
 * @retval NvSuccess Indicates the operation succeeded.
 * @retval NvError_NotInitialized Indicates that the Spdif channel is not opened. 
 * @retval NvError_InvalidState Indicates that there is no write call made by 
 * client or write operation was aborted.  
 */
NvError NvDdkSpdifWriteResume(NvDdkSpdifHandle hSpdif);

/**
 * Enable/disable the loopback mode for the spdif channel.
 *
 * @param hSpdif The Spdif handle which is allocated by the driver after calling
 * the NvDdkSpdifOpen() 
 * @param IsEnable Tells whether the loop back mode is enabled or not. If it
 * is NV_TRUE then the loop back mode is enabled.
 * @retval NvSuccess Indicates the operation succeeded.
 */
NvError NvDdkSpdifSetLoopbackMode(NvDdkSpdifHandle hSpdif, NvBool IsEnable);

/**
 * Set the continuous double buffering mode of playback for the audio stream.
 * it will inform the client when first half of the  buffer is tranferred by 
 * signalling the semaphore. It continues the transmitting of the second half 
 * of the buffer. once this part of buffer is transmitted, it signals the 
 * another semaphore and continue the transmitting of first part of the buffer
 * and repeats this operation till it stopped.
 * 
 * @see NvDdkSpdifOpen
 * @see NvDdkSpdifWrite
 *
 * @param hSpdif The Spdif handle which is allocated by the driver after calling
 * the NvDdkSpdifOpen() 
 * @param IsEnable Tells whether the continuous mode is enabled or not. If it
 * is NV_TRUE then same buffer will be continuously send.
 * @param pTransmitBuffer Pointer to the transmit buffer.
 * @param AsynchSemaphoreId1 Client created semaphore Id which will be 
 * signalled when first half of the buffer is transmitted.
 * @param AsynchSemaphoreId2 Client created semaphore Id which will be 
 * signalled when second half of the buffer is transmitted.
 *
 * @retval NvSuccess Indicates the operation succeeded.
 * @retval NvError_NotInitialized Indicates that the Spdif channel is not opened. 
 */
NvError 
NvDdkSpdifSetContinuousDoubleBufferingMode(
    NvDdkSpdifHandle hSpdif, 
    NvBool IsEnable,
    NvDdkSpdifClientBuffer* pTransmitBuffer,
    NvOsSemaphoreHandle AsynchSemaphoreId1,
    NvOsSemaphoreHandle AsynchSemaphoreId2);

/**
 * @brief Suspend mode entry function for Spdif driver. Powers down the controller
 * to enter suspend state. System is going down here and any error returned 
 * cannot be interpreted to stop the system from going down.
 *
 * @param hSpdif The Spdif handle which is allocated by the driver after calling
 * the NvDdkSpdifOpen() 
 *
 */
NvError NvDdkSpdifSuspend(NvDdkSpdifHandle hSpdif);

/**
 * @brief Suspend mode exit function for Spdif driver. This will resume the controller
 * from the suspend state. Here the system needs to come up quickly and we 
 * cannot afford to interpret the error and stop system from coming up.
 *
 * @param hSpdif The Spdif handle which is allocated by the driver after calling
 * the NvDdkSpdifOpen() 
 *
 */
NvError NvDdkSpdifResume(NvDdkSpdifHandle hSpdif);


/** @} */

#if defined(__cplusplus)
}
#endif

#endif  // INCLUDED_NVDDK_SPDIF_H

