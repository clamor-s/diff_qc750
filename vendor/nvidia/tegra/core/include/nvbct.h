/*
 * Copyright (c) 2008-2012, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

/**
 * @file
 * <b>NVIDIA Boot Configuration Table Management Framework</b>
 *
 * @b Description: This file declares the APIs for interacting with
 *    the Boot Configuration Table (BCT).
 */

#ifndef INCLUDED_NVBCT_H
#define INCLUDED_NVBCT_H

#include "nvcommon.h"
#include "nverror.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * @defgroup boot_config_mgmt_group BCT Management Framework
 * Declares the APIs for interacting with
 * the Boot Configuration Table (BCT).
 * @{
 */

//FIXME: Need a cleaner way to obtain the odm option
//offset within the customerdata field of the BCT.
//This offset is a function of the chip as well
//as some pending bugs.
#define NV_AUXDATA_STRUCT_SIZE_AP15 28
#define NV_ODM_OPTION_OFFSET_AP15 (NVBOOT_BCT_CUSTOMER_DATA_SIZE-NV_AUXDATA_STRUCT_SIZE_AP15)
/**
 * Data elements of the BCT which can be queried and set via
 * the NvBct API
 */
typedef enum
{
    /// Version of BCT structure
    NvBctDataType_Version,

    /// Device parameters with which the secondary boot device controller is
    /// initialized
    NvBctDataType_BootDeviceConfigInfo,

    /// Number of valid device parameter sets
    NvBctDataType_NumValidBootDeviceConfigs,

    /// SDRAM parameters with which the SDRAM controller is initialized
    NvBctDataType_SdramConfigInfo,

    /// Number of valid SDRAM parameter sets
    NvBctDataType_NumValidSdramConfigs,

    /// Generic Attribute for specified BL instance
    NvBctDataType_BootLoaderAttribute,

    /// Version number for specified BL instance
    NvBctDataType_BootLoaderVersion,

    /// Partition id of the BCT
    NvBctDataType_BctPartitionId,

    /// Start Block for specified BL instance
    NvBctDataType_BootLoaderStartBlock,

    /// Start Sector for specified BL instance
    NvBctDataType_BootLoaderStartSector,

    /// Length (in bytes) of specified BL instance
    NvBctDataType_BootLoaderLength,

    /// Load Address for specified BL instance
    NvBctDataType_BootLoaderLoadAddress,

    /// Entry Point for specified BL instance
    NvBctDataType_BootLoaderEntryPoint,

    /// Crypto Hash for specified BL instance
    NvBctDataType_BootLoaderCryptoHash,

    /// Number of Boot Loaders that are enabled for booting
    NvBctDataType_NumEnabledBootLoaders,

    /// Bad block table
    NvBctDataType_BadBlockTable,

    /// Partition size
    NvBctDataType_PartitionSize,

    /// Specifies the size of a physical block on the secondary boot device
    /// in log2(bytes).
    NvBctDataType_BootDeviceBlockSizeLog2,

    /// Specifies the size of a page on the secondary boot device in log2(bytes).
    NvBctDataType_BootDevicePageSizeLog2,

    /// Specifies a region of data available to customers of the BR.  This data
    /// region is primarily used by a manufacturing utility or BL to store
    /// useful information that needs to be shared among manufacturing utility,
    /// BL, and OS image.  BR only provides framework and does not use this
    /// data.
    ///
    /// @note Some of this space has already been allocated for use
    /// by NVIDIA.
    NvBctDataType_AuxData,
    NvBctDataType_AuxDataAligned,
    // Version of customer data stored in BCT
    NvBctDataType_CustomerDataVersion,
    // Boot device type stored in BCT customer data
    NvBctDataType_DevParamsType,

    /// Specifies the signature for the BCT structure.
    NvBctDataType_CryptoHash,

    /// Specifies random data used in lieu of a formal initial vector when
    /// encrypting and/or computing a hash for the BCT.
    NvBctDataType_CryptoSalt,

    /// Specifies extent of BCT data to be hashed
    NvBctDataType_HashDataOffset,
    NvBctDataType_HashDataLength,

    /// Customer defined configuration parameter
    NvBctDataType_OdmOption,

    /// Specifies entire BCT
    NvBctDataType_FullContents,

    /// Specifies size of bct
    NvBctDataType_BctSize,

    NvBctDataType_Num,

    /// Specifies a reserved area at the end of the BCT that must be
    /// filled with the padding pattern.
    NvBctDataType_Reserved,

    /// Specifies the type of device for parameter set DevParams[i].
    NvBctDataType_DevType,
    /// Partition Hash Enums
    NvBctDataType_HashedPartition_PartId,
    /// Crypto Hash for specified Partition
    NvBctDataType_HashedPartition_CryptoHash,
    /// Enable Failback
    NvBctDataType_EnableFailback,
    /// One time flashable infos
    NvBctDataType_InternalInfoOneTimeRaw,
    NvBctDataType_InternalInfoVersion,

    NvBctDataType_NumValidDevType,

    NvBctDataType_Max = 0x7fffffff
} NvBctDataType;

typedef enum
{
    NvBctBootDevice_Nand8,
    NvBctBootDevice_Emmc,
    NvBctBootDevice_SpiNor,
    NvBctBootDevice_Nand16,
    NvBctBootDevice_Current,
    NvBctBootDevice_Num,
    NvBctBootDevice_Max = 0x7fffffff
} NvBctBootDevice;


/**
 * BCT auxiliary information.
 */
typedef struct NvBctAuxInfoRec
{
    /// BCT page table starting sector
    NvU16      StartLogicalSector;

    /// BCT page table number of logical sectors
    NvU16      NumLogicalSectors;

} NvBctAuxInfo;



typedef struct NvBctRec *NvBctHandle;

typedef struct NvBctAuxInternalDataRec
{
    /* The CustomerOption need to be the first in this structure */

    /* Do not change the position of the CustomerOption as the miniloader
     * and the BL will have to be in sync
     * FIXME: use a common header to prevent this
     */
    NvU32 CustomerOption;
    NvU8  BctPartitionId;
} NvBctAuxInternalData;

/**
 * Create a handle to the specified BCT.
 *
 * The supplied buffer must contain a BCT, or if Buffer is NULL then the
 * system's default BCT (the BCT used by the Boot ROM to boot the chip) will be
 * used.
 *
 * This routine must be called before any of the other NvBct*() API's.  The
 * handle must be passed as an argument to all subsequent NvBct*() invocations.
 *
 * The size of the BCT can be queried by specifying a NULL phBct and a non-NULL
 * Size.  In this case, the routine will set *Size equal to the BCT size and
 * return NvSuccess.
 *
 * @param Size size of Buffer, in bytes
 * @param Buffer buffer containing a BCT, or NULL to select the system's default
 *        BCT
 * @param phBct pointer to handle for invoking subsequent operations on the BCT;
 *        can be specified as NULL to query BCT size
 *
 * @retval NvSuccess BCT handle creation successful or BCT size query successful
 * @retval NvError_InvalidAddress Illegal pointer value
 * @retval NvError_InsufficientMemory Not enough memory to allocate handle
 * @retval NvError_BadParameter No BCT found in Buffer
 */

NvError
NvBctInit(
    NvU32 *Size,
    void *Buffer,
    NvBctHandle *phBct);

/**
 * Release BCT handle and associated resources (but not the BCT itself).
 *
 * @param hBct handle to BCT instance
 */

void
NvBctDeinit(
    NvBctHandle hBct);

/**
 * Retrieve data from the BCT.
 *
 * This API allows the size of the data element to be retrieved, as well as the
 * number of instances of the data element present in the BCT.
 *
 * To retrieve that value of a given data element, allocate a Data buffer large
 * enough to hold the requested data, set *Size to the size of the buffer, and
 * set *Instance to the instance number of interest.
 *
 * To retrieve the size and number of instances of a given type of data element,
 * specified a *Size of zero and a non-NULL Instance (pointer).  As a result,
 * NvBctGetData() will set *Size to the actual size of the data element and
 * *Instance to the number of available instances of the data element in the
 * BCT, then report NvSuccess.
 *
 * @param hBct handle to BCT instance
 * @param DataType type of data to obtain from BCT
 * @param Size pointer to size of Data buffer; set *Size to zero in order to
 *        retrieve the data element size and number of instances
 * @param Instance pointer to instance number of the data element to retrieve
 * @param Data buffer for storing the retrieved instance of the data element
 *
 * @retval NvSuccess BCT data element retrieved successfully
 * @retval NvError_InsufficientMemory data buffer (*Size) too small
 * @retval NvError_InvalidAddress Illegal pointer value
 * @retval NvError_BadParameter Unknown data type, or illegal instance number
 */

NvError
NvBctGetData(
    NvBctHandle hBct,
    NvBctDataType DataType,
    NvU32 *Size,
    NvU32 *Instance,
    void *Data);

/**
 * Set data in the BCT.
 *
 * This API works like NvBctGetData() to retrieve the size and number of
 * instances of the data element in the BCT.
 *
 * To set the value of a given instance of the data element, set *Size to the size
 * of the data element, *Instance to the desired instance, and provide the data
 * value in the Data buffer.
 *
 * @param hBct handle to BCT instance
 * @param DataType type of data to set in the BCT
 * @param Size pointer to size of Data buffer; set *Size to zero in order to
 *        retrieve the data element size and number of instances
 * @param Instance pointer to the instance number of the data element to set
 * @param Data buffer for storing the desired value of the data element
 *
 * @retval NvSuccess BCT data element set successfully
 * @retval NvError_InsufficientMemory data buffer (*Size) too small
 * @retval NvError_InvalidAddress Illegal pointer value
 * @retval NvError_BadParameter Unknown data type, or illegal instance number
 * @retval NvError_InvalidState Data type is read-only
 */

NvError
NvBctSetData(
    NvBctHandle hBct,
    NvBctDataType DataType,
    NvU32 *Size,
    NvU32 *Instance,
    void *Data);
/** @} */

#if defined(__cplusplus)
}
#endif  /* __cplusplus */

#endif // INCLUDED_NVBCT_H
