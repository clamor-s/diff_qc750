/*
 * Copyright (c) 2010-2012, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

/**
 * @file
 * <b>NVIDIA Driver Development Kit: Tegra Fuse Programming API</b>
 *
 */

#ifndef INCLUDED_NVDDK_FUSE_H
#define INCLUDED_NVDDK_FUSE_H

/**
 * @defgroup nvddk_fuse_group Fuse Programming APIs
 *
 * Fuse burning is supported by two separate interfaces:
 * - <a href="#sysfs_if" class="el"> Kernel module file system (sysfs)
 * interface</a>--Exposes a clean and easy-to-use file system interface
 * for programming fuses; the Kernel interface is not available in the
 * boot loader.
 * - <a href="#nvddk_if" class="el">NVIDIA Driver Development Kit interface</a>
 * (declared in this file)--Provides direct NvDDK function calls to enable
 * fuse programming; the NvDDK interface is available in the boot loader and
 * as part of the main system image.
 *
 * @warning For factory production, after programming the Secure Boot Key (SBK),
 * it is critical that the factory software reads back the SBK to ensure that
 * the value is programmed as desired. This readback verification MUST be done
 * prior to programming the ODM secure mode fuse. Once this fuse is burned,
 * there is no way to recover the SBK. For example, if the SBK somehow were
 * programmed incorrectly, or if the host factory software were to lose the key,
 * there would be no way to recover. There are many other scenarios possible,
 * but the important thing to recognize is that the factory software must
 * ensure both that 1) the key is correctly programmed to the device by a
 * separate readback mechanism, and that 2) it is correctly saved to your
 * external database. Failure to manage this properly will result in secure
 * non-functional units. The only way to recover these units will be
 * to physically remove the Tegra application processor and replace it.
 *
 * <hr>
 * <a name="sysfs_if"></a><h2>File System Interface (sysfs)</h2>
 *
 * This topic includes the following sections:
 * - Fuses as a File System
 * - Limitations
 * - Available Fuses
 * - Fuse Sizes
 * - Fuse Read Example
 * - Fuse Write Example
 *
 * <h3>Fuses as a File System</h3>
 *
 * The fuse burning/reading subsystem is implemented as a system file system,
 * with each accessible fuse type exposed as a separate file in the following
 * directory:
 *
 * <pre>
 *     /sys/firmware/fuse/                                            </pre>
 *
 * Each file uses ASCII hexadecimal digits for input and output with no
 * preceeding 0x indicator, and no minimum field width. For example, if reading
 * a fuse register/field that is 0000000, the data will be shown as 0. Likewise,
 * programming a fuse register/field need not include preceeding zeros.
 *
 * Access to these files is like access to any other file for reading/writing,
 * with access via scripts or standard file system calls.
 *
 * @warning Burning fuses is an irreversible process, which burns "1's" into the
 * selected bits. This means that while you can re-burn additional bits at any
 * future time, you cannot reset bits back to their '0' state.
 *
 * <h3>Limitations</h3>
 *
 * This software has been tested on NVIDIA&reg; Tegra(TM) 250 Series ("Harmony")
 * and AP20 ("Whistler") platforms. Most of the code is in place to support any
 * platform, with the following exception: PMU driver. The change for PMU driver
 * includes:
 *
 * - An update for the Whistler PMU driver for the MAX8907b PMIC
 * - Allocating a rail for the fuse voltage rail control, and
 * - Adding the fuse rail into the query mechanism (which for Whistler is found
 * in \c nvodm_query_discovery_e1116_addresses.h).
 *
 * Alternatively, it may be possible to simply physically tie the fuse voltage
 * high, but this possibility is untested. However, it should be reasonably
 * easy to add support for any target.
 *
 * @note This software is currently under development, and this early release
 * is intended to enable customers early access to some fuse burning capability
 * with the understanding that the interface is subject to change and is not
 * completed. Because of this, it is important to verify the desired fuses
 * are configured and the board is functional using the settings <b><em>before
 * burning large batches of chips</em></b>.
 *
 * Bytes are written to fuses in the order they are given; specific big or
 * little endian is not enforced. For user-defined fields, byte ordering may not
 * matter as long as the interface is consistent when reading/writing. However,
 * for multi-byte system fuses (like \a SecBootDeviceSelect) the byte order is
 * critical for the system. For example, on Whistler using EMMC, setting the \a
 * SecBootDeviceSelect field to 0x0025 may be appropriate.
 *
 * The \a SpareBits field is reserved for NVIDIA use; reads/writes to this
 * field are disallowed.
 *
 * Once the ODM production fuse is burned, the following operations are
 * disallowed:
 * - reading the SecureBootKey
 * - writing fuses
 *
 * <h3>Available Fuses</h3>
 *
 * The following list shows the available fuse files:
 *
 * <pre>
 *
 * # ls -l /sys/firmware/fuse
 *
 *  -rw-r----- root     root        4096 2012-03-07 02:26 device_key
 * -rw-r----- root     root         4096 2012-03-07 02:26 ignore_dev_sel_straps
 * -rw-r----- root     root         4096 2012-03-07 02:26 jtag_disable
 * -rw-r----- root     root         4096 2012-03-07 02:26 odm_production_mode
 * -rw-r----- root     root         4096 2012-03-07 02:26 odm_reserved
 * -rw-r----- root     root         4096 2012-03-07 02:26 sec_boot_dev_cfg
 * -rw-r----- root     root         4096 2012-03-07 02:26 sec_boot_dev_sel
 * -rw-r----- root     root         4096 2012-03-07 02:26 secure_boot_key
 * -rw-r----- root     root         4096 2012-03-07 02:26 sw_reserved
 *
 * </pre>
 *
 * <h3>Fuse Sizes</h3>
 *
 * The following shows the fuse sizes, in bits:
 * - device_key = 32 bits
 * - ignore_dev_sel_straps = 1 bit
 * - jtag_disable = 1 bit
 * - odm_production_mode = 1 bit
 * - odm_reserved = 256 bits
 * - sec_boot_dev_cfg = 16 bits
 * - sec_boot_dev_sel = 3 bits
 * - secure_boot_key = 128 bits
 * - sw_reserved = 4 bits
 *
 * <h3>Fuse Read Example</h3>
 *
 * <pre>
 *
 * $ adb shell
 * # cat < /sys/firmware/fuse/ReservedOdm
 * 0#                                                                 </pre>
 *
 * <h3>Fuse Write Example</h3>
 *
 * <pre>
 * $ adb shell
 * # echo 1A34 > /sys/firmware/fuse/ReservedOdm                       </pre>
 *
 * <hr>
 * <a name="nvddk_if"></a><h2>NvDdk Interface</h2>
 *
 * This topic includes the following sections:
 * - NvDdk Fuse Library Usage
 * - Fuse Programming Example Code
 * - Fuse API Documentation (Enumerations, Functions, etc.)
 *
 * <h3>NvDdk Library Usage</h3>
 *
 * - Calling the NvDdkDisableFuseProgram() API before exiting the boot loader
 *    disables further fuse programming until the next system reset. (See the
 *    example code.)
 * - It is advised that you disable the JTAG while using this code in
 *    the final version.
 * - Enabling clock and voltage to program the fuse is out of scope of this
 *    library. You must ensure that these are enabled prior to using the library
 *    functions.
 * - The library works on physical mapped memory or on virtual mapped with
 *    one-to-one mapping for the fuse registers.
 *
 * <h3>Example Fuse Programming Code</h3>
 *
 * The following example shows one way to use Fuse Programming APIs.
 * @note You must ensure fuse clock and fuse voltage is enabled prior
 * to programming fuses.
 *
 * @code
 * //
 * // Fuse Programming Example
 * //
 * #include "nvddk_fuse.h"
 * #include "nvtest.h"
 * #include "nvrm_init.h"
 * #include "nvrm_pmu.h"
 * #include "nvodm_query_discovery.h"
 * #include "nvodm_services.h"
 *
 * #define DISABLE_PROGRMING_TEST 0
 *
 * static NvError NvddkFuseProgramTest(NvTestApplicationHandle h )
 * {
 *     NvError err = NvSuccess;
 *     err = NvDdkFuseProgram();
 *     if(err != NvError_Success)
 *     {
 *         NvOsDebugPrintf("NvDdkFuseProgram failed \n");
 *         return err;
 *     }
 *     // Check fuse sense.
 *     NvDdkFuseSense();
 *     // Verify the fuses and return the result.
 *     err = NvDdkFuseVerify();
 *     if(err != NvError_Success)
 *         NvOsDebugPrintf("NvDdkFuseVerify failed \n");
 *     return err;
 * }
 *
 * static NvError PrepareFuseData(void)
 * {
 *     // Initialize argument sizes to zero to perform initial queries.
 *     NvU32 BootDevSel_Size = 0;
 *     NvU32 BootDevConfig_Size = 0;
 *     NvU32 ResevOdm_size = 0;
 *     NvU32 size = 0;
 *
 *     // Specify values to be programmed.
 *     NvU32 BootDevConfig_Data = 0x9; // Set device config value to 0x9.
 *     NvU32 BootDevSel_Data = 0x1;     // Set boot select to 0x1 for NAND.
 *     NvU8 ResevOdm_Data[32] =  {0xEF,0xCD,0xAB,0x89,
 *                                                     0x78,0x56,0x34,0x12,
 *                                                     0xa,0xb,0xc,0xd,
 *                                                     0xAA,0xBB,0xCC,0xDD,
 *                                                     0,0,0,0,
 *                                                     0,0,0,0,
 *                                                     0x78,0x56,0x34,0x12,
 *                                                     0x78,0x56,0x34,0x12};
 *
 *     NvU8 skpDevSelStrap_data = 1;
 *     NvError e;
 *
 *     // Query the sizes of the fuse values.
 *     e = NvDdkFuseGet(NvDdkFuseDataType_SecBootDeviceSelect,
 *                                    &BootDevSel_Data, &BootDevSel_Size);
 *     if (e != NvError_Success) return e;
 *
 *     e = NvDdkFuseGet(NvDdkFuseDataType_SecBootDeviceConfig,
 *                                    &BootDevConfig_Data, &BootDevConfig_Size);
 *     if (e != NvError_Success) return e;
 *
 * #ifdef DISABLE_PROGRMING_TEST
 *     NvDdkDisableFuseProgram();
 * #endif
 *
 *     e = NvDdkFuseGet(NvDdkFuseDataType_ReservedOdm,
 *                                    &ResevOdm_Data, &ResevOdm_size);
 *     if (e != NvError_Success) return e;
 *
 *     e = NvDdkFuseGet(NvDdkFuseDataType_SkipDevSelStraps,
 *                                    &skpDevSelStrap_data, &size);
 *     if (e != NvError_Success) return e;
 *
 *
 *     // Set the fuse values.
 *     e = NvDdkFuseSet(NvDdkFuseDataType_SecBootDeviceSelect,
 *                                    &BootDevSel_Data, &BootDevSel_Size);
 *     if (e != NvError_Success) return e;
 *
 *     e = NvDdkFuseSet(NvDdkFuseDataType_SecBootDeviceConfig,
 *                                    &BootDevConfig_Data, &BootDevConfig_Size);
 *     if (e != NvError_Success) return e;
 *
 *     e = NvDdkFuseSet(NvDdkFuseDataType_SkipDevSelStraps,
 *                                    &skpDevSelStrap_data, &size);
 *     if (e != NvError_Success) return e;
 *
 *
 *     e = NvDdkFuseSet(NvDdkFuseDataType_ReservedOdm,
 *                                    &ResevOdm_Data, &ResevOdm_size);
 *     if (e != NvError_Success) return e;
 *
 *     return e;
 * }
 *
 *
 * NvError NvTestMain(int argc, char *argv[])
 * {
 *     NvTestApplication h;
 *     NvError err;
 *
 *     NVTEST_INIT( &h );
 *     // Enable fuse clock and fuse voltage prior to programming fuses.
 *     err = PrepareFuseData();
 *     if( err != NvSuccess)
 *         return err;
 *
 *     NvOdmOsWaitUS(10000);
 *
 *     NVTEST_RUN(&h, NvddkFuseProgramTest);
 *
 *     NVTEST_RESULT( &h );
 * }
 * @endcode
 *
 * @ingroup nvddk_modules
 * @{
 */

#if defined(__cplusplus)
extern "C"
{
#endif


#include "nvcommon.h"
#include "nverror.h"


/* ----------- Program/Verify API's -------------- */

/**
 * Defines types of fuse data to set or query.
 */
typedef enum
{
    /// Specifies a default (unset) value.
    NvDdkFuseDataType_None = 0,
    /// Specifies a device key (DK).
    NvDdkFuseDataType_DeviceKey,

    /**
     * Specifies a JTAG disable flag:
     * - NV_TRUE specifies to permanently disable JTAG-based debugging.
     * - NV_FALSE indicates that JTAG-based debugging is not permanently
     *   disabled.
     */
    NvDdkFuseDataType_JtagDisable,

    /**
     * Specifies a key programmed flag (read-only):
     * - NV_TRUE indicates that either the SBK or the DK value is nonzero.
     * - NV_FALSE indicates that both the SBK and DK values are zero (0).
     *
     * @note Once the ODM production fuse is set to NV_TRUE, applications
     * can no longer read back the SBK or DK value; however, by using
     * this \c KeyProgrammed query it is still possible to determine
     * whether or not the SBK or DK has been programmed to a nonzero value.
     */
    NvDdkFuseDataType_KeyProgrammed,

    /**
     * Specifies an ODM production flag:
     * - NV_TRUE specifies the chip is in ODM production mode.
     * - NV_FALSE indicates that the chip is not in ODM production mode.
     */
    NvDdkFuseDataType_OdmProduction,

    /**
     * Specifies a secondary boot device configuration.
     * This value is chip-dependent.
     * For Tegra APX 2600, use the NVBOOT_FUSE_*_CONFIG_*
     * defines from:
     * <pre>
     *  /src/drivers/hwinc/ap15/nvboot_fuse.h
     * </pre>
     */
    NvDdkFuseDataType_SecBootDeviceConfig,

    /** Specifies a secure boot key (SBK). */
    NvDdkFuseDataType_SecureBootKey,

    /**
     * Specifies a stock-keeping unit (read-only).
     * This value is chip-dependent.
     * See chip-specific documentation for legal values.
     */
    NvDdkFuseDataType_Sku,

    /**
     * Specifies spare fuse bits (read-only).
     * Reserved for future use by NVIDIA.
     */
    NvDdkFuseDataType_SpareBits,

    /**
     * Specifies software reserved fuse bits (read-only).
     * Reserved for future use by NVIDIA.
     */
    NvDdkFuseDataType_SwReserved,

    /**
     * Specifies skip device select straps (applies to Tegra 200 series only):
     * - NV_TRUE specifies to ignore the device selection straps setting
     *   and that the boot device is specified via the
     *   \c SecBootDeviceSelect and \c SecBootDeviceConfig fuse settings.
     * - NV_FALSE indicates that the boot device is specified via the device
     *   selection straps setting.
     */
    NvDdkFuseDataType_SkipDevSelStraps,

    /**
     * Specifies a secondary boot device selection.
     * This value is chip-dependent.
     * The chip-independent version of this data is
     * ::NvDdkFuseDataType_SecBootDeviceSelect.
     * For Tegra APX 2600, use the \c NvBootFuseBootDevice enum
     * values found at:
     * <pre>
     *  /src/drivers/hwinc/ap15/nvboot_fuse.h
     * </pre>
     */
    NvDdkFuseDataType_SecBootDeviceSelectRaw,

    /**
     * Specifies raw field for reserved the ODM.
     * This value is ODM-specific. Reserved for customers.
     */
    NvDdkFuseDataType_ReservedOdm,

    /**
     * Specifies the pacakge info type for the chip.
     * See chip-specific documentation for legal values.
     */
    NvDdkFuseDataType_PackageInfo,

    /**
     * Specifies the unique ID for the chip.
     */
    NvDdkFuseDataType_Uid,
    /**
     * Specifies a secondary boot device selection.
     * This value is chip-independent and is described in the
     * \c nvddk_bootdevices.h file. The chip-dependent version of this data is
     * ::NvDdkFuseDataType_SecBootDeviceSelectRaw.
     * For Tegra APX 2600, the values for \c NvDdkFuseDataType_SecBootDeviceSelect and
     * \c NvDdkFuseDataType_SecBootDeviceSelectRaw are identical.
     */
    NvDdkFuseDataType_SecBootDeviceSelect,

    /**
     * Specifies the operating mode with modes
     * from the ::NvDdkFuseOperatingMode enumeration.
     */
    NvDdkFuseDataType_OpMode,
    /**
     * Specifies secondary boot device controller instance.
     */
    NvDdkFuseDataType_SecBootDevInst,
    /** The following must be last. */
    NvDdkFuseDataType_Num,
    /** Ignore -- Forces compilers to make 32-bit enums. */
    NvDdkFuseDataType_Force32 = 0x7FFFFFFF
} NvDdkFuseDataType;

/**
 * specifies different operating modes.
 */
typedef enum
{
    NvDdkFuseOperatingMode_None = 0,
    /**
     * Preproduction mode
     */
    NvDdkFuseOperatingMode_Preproduction,
    /**
     * Failrue Analysis mode
     */
    NvDdkFuseOperatingMode_FailureAnalysis,
    /**
     * Nvproduction mode
     */
    NvDdkFuseOperatingMode_NvProduction,
    /**
     * Odm production secure mode
     */
    NvDdkFuseOperatingMode_OdmProductionSecure,
    /**
     * Odm Production mode
     */
    NvDdkFuseOperatingMode_OdmProductionOpen,
    NvDdkFuseOperatingMode_Max, /* Must appear after the last legal item */
    NvDdkFuseOperatingMode_Force32 = 0x7fffffff
} NvDdkFuseOperatingMode;


/**
 * Gets a value from the fuse registers.
 *
 * @pre Do not call this function while the programming power is applied!
 *
 * After programming fuses and removing the programming power,
 * NvDdkFuseSense() must be called to read the new values.
 *
 * By passing a size of 0, the caller is requesting to be told the
 * expected size.
 *
 * Treatment of fuse data depends upon its size:
 * - if \a *pSize == 1, treat \a *pData as an NvU8
 * - if \a *pSize == 2, treat \a *pData as an NvU16
 * - if \a *pSize == 4, treat \a *pData as an NvU32
 * - else, treat \a *pData as an array of NvU8 values (i.e., NvU8[]).
 */
NvError NvDdkFuseGet(NvDdkFuseDataType Type, void *pData, NvU32 *pSize);

/**
 * Schedules fuses to be programmed to the specified values when the next
 * NvDdkFuseProgram() operation is performed.
 *
 * @note Attempting to program the ODM production fuse at the same
 * time as the SBK or DK causes an error, because it is not
 * possible to verify that the SBK or DK were programmed correctly.
 * After triggering this error, all further attempts to set fuse
 * values will fail, as will \c NvDdkFuseProgram, until NvDdkFuseClear()
 * has been called.
 *
 * By passing a size of 0, the caller is requesting to be told the
 * expected size.
 *
 * Treatment of fuse data depends upon its size:
 * - if \a *pSize == 1, treat \a *pData as an NvU8
 * - if \a *pSize == 2, treat \a *pData as an NvU16
 * - if \a *pSize == 4, treat \a *pData as an NvU32
 * - else, treat \a *pData as an array of NvU8 values (i.e., NvU8[]).
 *
 * @retval NvError_BadValue If other than "reserved ODM fuse" is set in ODM
 *             production  mode.
 * @retval NvError_AccessDenied If programming to fuse registers is disabled.
 */
NvError NvDdkFuseSet(NvDdkFuseDataType Type, void *pData, NvU32 *pSize);

/**
 * Reads the current fuse data into the fuse registers.
 *
 * \c NvDdkFuseSense must be called at least once, either:
 * - After programming fuses and removing the programming power,
 * - Prior to calling NvDdkFuseVerify(), or
 * - Prior to calling NvDdkFuseGet().
 */
void NvDdkFuseSense(void);

/**
 * Programs all fuses based on cache data changed via the NvDdkFuseSet() API.
 *
 * @pre Prior to invoking this routine, the caller is responsible for supplying
 * valid fuse programming voltage.
 *
 * @retval NvError_AccessDenied If programming to fuse registers is disabled.
 * @return An error if an invalid combination of fuse values was provided.
 */
NvError NvDdkFuseProgram(void);

/**
 * Verify all fuses scheduled via the NvDdkFuseSet() API.
 *
 * @pre Prior to invoking this routine, the caller is responsible for ensuring
 * that fuse programming voltage is removed and subsequently calling
 * NvDdkFuseSense().
 */
NvError NvDdkFuseVerify(void);

/**
 * Clears the cache of fuse data, once NvDdkFuseProgram() and NvDdkFuseVerify()
 * API are called to clear all buffers.
 */
void NvDdkFuseClear(void);

/**
 * Disables further fuse programming until the next system reset.
 */
void NvDdkDisableFuseProgram(void);

#if defined(__cplusplus)
}
#endif

/** @} */
#endif // INCLUDED_NVDDK_FUSE_H

