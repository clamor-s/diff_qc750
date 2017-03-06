/*
 * Copyright (c) 2006-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */
/**
 * @file
 * <b>NVIDIA Tegra ODM Adaptation:
 *         Imager Interface</b>
 *
 * @b Description: Defines the ODM interface for NVIDIA imager adaptation.
 *
 */

#ifndef INCLUDED_NVODM_IMAGER_H
#define INCLUDED_NVODM_IMAGER_H
#if defined(_cplusplus)
extern "C"
{
#endif
#include "nvcommon.h"
#include "nvcamera_isp.h"
#include "tegra_camera.h"
/**
 * @defgroup nvodm_imager Imager Adaptation Interface
 *
 * This API handles the abstraction of physical camera imagers, allowing
 * programming the imager whenever resolution changes occur (for ISP
 * programming), like exposure, gain, frame rate, and power level changes.
 * Other camera devices, like flash modules, focusing mechanisms,
 * shutters, IR focusing devices, and so on, could be programmed via this
 * interface as well.
 *
 * Each imager is required to implement several of the imager adaptation
 * functions.
 *
 * Open and close routines are needed for the imager ODM context,
 * as well as set mode, power level, parameter, and other routines
 * for each imager.
 *
 * @ingroup nvodm_adaptation
 * @{
 */

/**
 * Defines the SOC imager white balance parameter.
 */
typedef enum {
   NvOdmImagerWhiteBalance_Auto = 0,
   NvOdmImagerWhiteBalance_Incandescent,
   NvOdmImagerWhiteBalance_Fluorescent,
   NvOdmImagerWhiteBalance_WarmFluorescent,
   NvOdmImagerWhiteBalance_Daylight,
   NvOdmImagerWhiteBalance_CloudyDaylight,
   NvOdmImagerWhiteBalance_Shade,
   NvOdmImagerWhiteBalance_Twilight,
   NvOdmImagerWhiteBalance_Force32 = 0x7FFFFFFF,
} NvOdmImagerWhiteBalance;
/**
 * Defines the SOC imager Color Effect parameter.
 */
typedef enum {
   NvOdmImagerEffect_None = 0,
   NvOdmImagerEffect_BW,
   NvOdmImagerEffect_Negative,
   NvOdmImagerEffect_Posterize,
   NvOdmImagerEffect_Sepia,
   NvOdmImagerEffect_Solarize,
   NvOdmImagerEffect_Aqua,
   NvOdmImagerEffect_Blackboard,
   NvOdmImagerEffect_Whiteboard,
   NvOdmImagerEffect_Force32 = 0x7FFFFFFF,
} NvOdmImagerEffect;

/**
 * Defines the imager sync edge.
 */
typedef enum {
   NvOdmImagerSyncEdge_Rising = 0,
   NvOdmImagerSyncEdge_Falling = 1,
   NvOdmImagerSyncEdge_Force32 = 0x7FFFFFFF,
} NvOdmImagerSyncEdge;
/**
 * Defines parallel timings for the sensor device.
 * For your typical parallel interface, the following timings
 * for the imager are sent to the driver, so it can program the
 * hardware accordingly.
 */
typedef struct
{
  /// Does horizontal sync start on the rising or falling edge.
  NvOdmImagerSyncEdge HSyncEdge;
  /// Does vertical sync start on the rising or falling edge.
  NvOdmImagerSyncEdge VSyncEdge;
  /// Use VGP0 for MCLK. If false, the VCLK pin is used.
  NvBool MClkOnVGP0;
} NvOdmImagerParallelSensorTiming;
/**
 * Defines MIPI timings. For a MIPI imager, the following timing
 * information is sent to the driver, so it can program the hardware
 * accordingly.
 */
typedef struct
{
#define NVODMSENSORMIPITIMINGS_PORT_CSI_A (0)
#define NVODMSENSORMIPITIMINGS_PORT_CSI_B (1)
    /// Specifies MIPI-CSI port type.
    /// @deprecated; do not use.
    NvU8 CsiPort;
#define NVODMSENSORMIPITIMINGS_DATALANES_ONE     (1)
#define NVODMSENSORMIPITIMINGS_DATALANES_TWO     (2)
#define NVODMSENSORMIPITIMINGS_DATALANES_THREE   (3)
#define NVODMSENSORMIPITIMINGS_DATALANES_FOUR    (4)
    /// Specifies number of data lanes used.
    NvU8 DataLanes;
#define NVODMSENSORMIPITIMINGS_VIRTUALCHANNELID_ONE   (0)
#define NVODMSENSORMIPITIMINGS_VIRTUALCHANNELID_TWO   (1)
#define NVODMSENSORMIPITIMINGS_VIRTUALCHANNELID_THREE (2)
#define NVODMSENSORMIPITIMINGS_VIRTUALCHANNELID_FOUR  (3)

    /// Specifies virtual channel identifier.
    NvU8 VirtualChannelID;
    /// For AP15/AP16:
    /// - Discontinuous clock mode is needed for single lane
    /// - Continuous clock mode is needed for dual lane
    ///
    /// For AP20:
    /// Either mode is supported for either number of lanes.
    NvBool DiscontinuousClockMode;
    /// Specifies a value between 0 and 15 to determine how many pixel
    /// clock cycles (viclk cycles) to wait before starting to look at
    /// the data.
    NvU8 CILThresholdSettle;
} NvOdmImagerMipiSensorTiming;

/**
 * Defines imager crop modes for NvOdmImagerSensorMode::CropMode.
 */
typedef enum
{
    NvOdmImagerNoCropMode = 1,
    NvOdmImagerPartialCropMode,
    NvOdmImagerCropModeForce32 = 0x7FFFFFFF,
} NvOdmImagerCropMode;

/**
* Defines sensor mode types for NvOdmImagerSensorMode::Type.
*/
typedef enum
{
    NvOdmImagerModeType_Normal = 0, //Normal mode (preview, still, or movie)
    NvOdmImagerModeType_Preview, //Dedicated preview mode
    NvOdmImagerModeType_Movie, //Dedicated movie mode
    NvOdmImagerModeTypeForce32 = 0x7FFFFFFF,
} NvOdmImagerModeType;

/**
 * Holds mode-specific sensor information.
 */
typedef struct NvOdmImagerSensorModeInfoRec
{
    /// Specifies the scan area (left, right, top, bottom)
    /// on the sensor array that the output stream covers.
    NvRect rect;
    /// Specifies x/y scales if the on-sensor binning/scaling happens.
    NvPointF32 scale;
} NvOdmImagerSensorModeInfo;

/**
 * Holds ODM imager modes for sensor device.
 */
typedef struct NvOdmImagerSensorModeRec
{
    /// Specifies the active scan region.
    NvSize ActiveDimensions;
    /// Specifies the active start.
    NvPoint ActiveStart;
    /// Specifies the peak frame rate.
    NvF32 PeakFrameRate;
    /// Specifies the:
    /// <pre> pixel_aspect_ratio = pixel_width / pixel_height </pre>
    /// So pixel_width = k and pixel_height = t means 1 correct pixel consists
    /// of 1/k sensor output pixels in x direction and 1/t sensor output pixels
    /// in y direction. One of pixel_width and pixel_height must be 1.0 and
    /// the other one must be less than or equal to 1.0. Can be 0 for 1.0.
    NvF32 PixelAspectRatio;
    /// Specifies the sensor PLL multiplier so VI/ISP clock can be set accordingly.
    /// <pre> MCLK * PLL </pre> multipler will be calculated to set VI/ISP clock accordingly.
    /// If not specified, VI/ISP clock will set to the maximum.
    NvF32 PLL_Multiplier;

    /// Specifies the crop mode. This can be partial or full.
    /// In partial crop mode, a portion of sensor array is taken while
    /// configuring sensor mode. While in full mode, almost the entire sensor array is
    /// taken while programing the sensor mode. Thus, in crop mode, the final image looks
    /// croped with respect to the full mode image.
    NvOdmImagerCropMode CropMode;

    /// Specifies mode-specific information.
    NvOdmImagerSensorModeInfo ModeInfo;

    /// Usage type of sensor mode
    NvOdmImagerModeType Type;
} NvOdmImagerSensorMode;
/**
 * Holds a region, for use with
 * NvOdmImagerParameter_RegionUsedByCurrentResolution.
 */
typedef struct NvOdmImagerRegionRec
{
    /// Specifies the upper left corner of the region.
    NvPoint RegionStart;
    /// Specifies the horizontal/vertical downscaling used
    /// (eg., 2 for a 2x downscale, 1 for no downscaling).
    NvU32 xScale;
    NvU32 yScale;

} NvOdmImagerRegion;
/**
 * Defines camera imager types.
 * Each ODM imager implementation will support one imager type.
 * @note A system designer should be aware of the limitations of using
 * both an imager device and a bitstream device, because they may share pins.
 */
typedef enum
{
    /// Specifies standard VIP 8-pin port; used for RGB/YUV data.
    NvOdmImagerSensorInterface_Parallel8 = 1,
    /// Specifies standard VIP 10-pin port; used for Bayer data, because
    /// ISDB-T in serial mode is possible only if Parallel10 is not used.
    NvOdmImagerSensorInterface_Parallel10,
    /// Specifies MIPI-CSI, Port A.
    NvOdmImagerSensorInterface_SerialA,
    /// Specifies MIPI-CSI, Port B
    NvOdmImagerSensorInterface_SerialB,
    /// Specifies MIPI-CSI, Port A&B. This is special case for T30 only
    /// where both CSI A&B must be enabled to make use of 4 lanes.
    NvOdmImagerSensorInterface_SerialAB,
    /// Specifies CCIR protocol (implicitly Parallel8)
    NvOdmImagerSensorInterface_CCIR,
    /// Specifies not a physical imager.
    NvOdmImagerSensorInterface_Host,
    /// Temporary: CSI via host interface testing
    /// This serves no purpose in the final driver,
    /// but is needed until a platform exists that
    /// has a CSI sensor.
    NvOdmImagerSensorInterface_HostCsiA,
    NvOdmImagerSensorInterface_HostCsiB,
    NvOdmImagerSensorInterface_Num,
    NvOdmImagerSensorInterface_Force32 = 0x7FFFFFFF
} NvOdmImagerSensorInterface;

#define NVODMIMAGERPIXELTYPE_YUV   0x010
#define NVODMIMAGERPIXELTYPE_BAYER 0x100
#define NVODMIMAGERPIXELTYPE_IS_YUV(_PT) ((_PT) & NVODMIMAGERPIXELTYPE_YUV)
#define NVODMIMAGERPIXELTYPE_IS_BAYER(_PT) ((_PT) & NVODMIMAGERPIXELTYPE_BAYER)
/**
 * Defines the ODM imager pixel type.
 */
typedef enum {
    NvOdmImagerPixelType_Unspecified = 0,
    NvOdmImagerPixelType_UYVY = NVODMIMAGERPIXELTYPE_YUV,
    NvOdmImagerPixelType_VYUY,
    NvOdmImagerPixelType_YUYV,
    NvOdmImagerPixelType_YVYU,
    NvOdmImagerPixelType_BayerRGGB = NVODMIMAGERPIXELTYPE_BAYER,
    NvOdmImagerPixelType_BayerBGGR,
    NvOdmImagerPixelType_BayerGRBG,
    NvOdmImagerPixelType_BayerGBRG,
    NvOdmImagerPixelType_Force32 = 0x7FFFFFFF
} NvOdmImagerPixelType;
/// Defines the ODM imager orientation.
/// The imager orientation is with respect to the display's
/// top line scan direction. A display could be tall or wide, but
/// what really counts is in which direction it scans the pixels
/// out of memory. The camera block must place the pixels
/// in memory for this to work correctly, and so the ODM
/// must provide info about the board placement of the imager.
typedef enum {
    NvOdmImagerOrientation_0_Degrees = 0,
    NvOdmImagerOrientation_90_Degrees,
    NvOdmImagerOrientation_180_Degrees,
    NvOdmImagerOrientation_270_Degrees,
    NvOdmImagerOrientation_Force32 = 0x7FFFFFFF
} NvOdmImagerOrientation;
/// Defines the ODM imager direction.
/// The direction this particular imager is pointing with respect
/// to the main display tells the camera driver whether this imager
/// is pointed toward or away from the user. Typically, the main
/// imaging device is pointed away, and if present a secondary imager,
/// if used for video conferencing, is pointed toward.
typedef enum {
    NvOdmImagerDirection_Away = 0,
    NvOdmImagerDirection_Toward,
    NvOdmImagerDirection_Force32 = 0x7FFFFFFF
} NvOdmImagerDirection;
/// Defines the structure used to describe the mclk and pclk
/// expectations of the sensor. The sensor likely has PLL's in
/// its design, which allows it to get a low input clock and then
/// provide a higher speed output clock to achieve the needed
/// bandwidth for frame data transfer. Ideally, the clocks will differ
/// so as to avoid noise on the bus, but early versions of
/// Tegra Application Processor required the clocks be the same.
/// The capabilities structure gives which profiles the NVODM
/// supports, and \a SetParameter of ::NvOdmImagerParameter_SensorInputClock
/// will be how the camera driver communicates which one was used.
/// The ::NvOdmImagerClockProfile structure gives an input clock,
/// 24 MHz, and a clock multiplier that the PLL will do to create
/// the output clock. For instance, if the output clock is 64 MHz, then
/// the clock multiplier is 64/24. Likely, the first item in the array
/// of profiles in the capabilities structure will be used. If an older
/// Tegra Application Processor is being used, then the list will be
/// searched for a multiplier of 1.0.
typedef struct {
    NvU32 ExternalClockKHz; /**< Specifies 'mclk' the clock going into sensor. */
    NvF32 ClockMultiplier; /**< Specifies 'pclk'/'mclk' ratio due to PLL programming. */
} NvOdmImagerClockProfile;

/** The maximum length of imager identifier string. */
#define NVODMIMAGER_IDENTIFIER_MAX 32
#define NVODMSENSOR_IDENTIFIER_MAX NVODMIMAGER_IDENTIFIER_MAX
/** The maximum length of imager format string. */
#define NVODMIMAGER_FORMAT_MAX 4
#define NVODMSENSOR_FORMAT_MAX NVODMIMAGER_FORMAT_MAX
/** The list of supported clock profiles maximum array size. */
#define NVODMIMAGER_CLOCK_PROFILE_MAX 2
/**
 * Holds the ODM imager capabilities.
 */
typedef struct {
    /// Specifies imager identifiers; an example of an identifier is:
    /// "Micron 3120" or "Omnivision 9640".
    char identifier[NVODMIMAGER_IDENTIFIER_MAX];
    /// Specifies imager interface types.
    NvOdmImagerSensorInterface SensorOdmInterface;
    /// Specifies the number of pixel types; this specifies the
    /// imager's ability to output several YUV formats, as well
    /// as Bayer.
    NvOdmImagerPixelType PixelTypes[NVODMIMAGER_FORMAT_MAX];
    /// Specifies orientation and direction.
    NvOdmImagerOrientation Orientation;
    NvOdmImagerDirection Direction;
    /// Specifies the clock frequency that the sensor
    /// requires for initialization and mode changes.
    /// Also can be used for times when performance
    /// is not important (self-test, calibration, bringup).
    NvU32 InitialSensorClockRateKHz;
    ///
    /// Specifies the clock profiles supported by the imager.
    /// For instance: {24, 2.66}, {64, 1.0}. The first is preferred
    /// but the second may be used for backward compatibility.
    /// Unused profiles must be zeroed out.
    NvOdmImagerClockProfile ClockProfiles[NVODMIMAGER_CLOCK_PROFILE_MAX];
    /// Specifies parallel and serial timing settings.
    NvOdmImagerParallelSensorTiming ParallelTiming;
    NvOdmImagerMipiSensorTiming MipiTiming;
    ///
    /// Proclaim the minimum blank time that this
    /// sensor will honor. The camera driver will check this
    /// against the underlying hardware's requirements.
    /// width = horizontal blank time in pixels.
    /// height = vertical blank time in lines.
    NvSize MinimumBlankTime;
    /// Specifies the preferred mode index.
    /// After calling ListModes, the camera driver will select
    /// the preferred mode via this index. The preferred
    /// mode may be the highest resolution for the optimal frame rate.
    NvS32 PreferredModeIndex;
    /// Focuser GUID
    /// If 0, no focusing mechanism is present.
    /// Otherwise, it is a globally unique identifier for the focuser
    /// so that a particular sensor can be associated with it.
    NvU64 FocuserGUID;
    /// Flash GUID, identifies the existence (non-zero) of a
    /// flash mechanism, and gives the globally unique identifier for
    /// it so that a particular sensor can be associated with its flash.
    NvU64 FlashGUID;
    /// Sentinel value.
    NvU32 CapabilitiesEnd;
} NvOdmImagerCapabilities;
#define NVODM_IMAGER_VERSION (2)
#define NVODM_IMAGER_CAPABILITIES_END ((0x3434 << 16) | NVODM_IMAGER_VERSION)
#define NVODM_IMAGER_GET_VERSION(CAPEND) ((CAPEND) & 0xFFFF)
#define NVODM_IMAGER_GET_CAPABILITIES(CAPEND) (((CAPEND) >> 16) & 0xFFFF)

typedef enum {
    NvOdmImagerCal_Undetermined,
    NvOdmImagerCal_NotApplicable,
    NvOdmImagerCal_NotCalibrated,
    NvOdmImagerCal_WrongSensor,
    NvOdmImagerCal_Override,
    NvOdmImagerCal_Calibrated,
    NvOdmImagerCal_Force32 = 0x7FFFFFFF
} NvOdmImagerCalibrationStatus;

typedef enum {
    NvOdmImagerTestPattern_None,
    NvOdmImagerTestPattern_Colorbars,
    NvOdmImagerTestPattern_Checkerboard,
    NvOdmImagerTestPattern_Walking1s,
} NvOdmImagerTestPattern;

typedef enum {
    NvOdmImagerSOCGamma_Enable,
    NvOdmImagerSOCGamma_Disable,
} NvOdmImagerSOCGamma;

typedef enum {
    NvOdmImagerSOCSharpening_Enable,
    NvOdmImagerSOCSharpening_Disable,
} NvOdmImagerSOCSharpening;

typedef enum {
    NvOdmImagerSOCLCC_Enable,
    NvOdmImagerSOCLCC_Disable,
} NvOdmImagerSOCLCC;

/**
 * Holds the ODM imager calibration data.
 * @sa NvOdmImagerParameter_CalibrationData and
 *      NvOdmImagerParameter_CalibrationOverrides
 */
typedef struct {
    /// The \c CalibrationData string may be a pointer to a statically compiled
    /// constant, or an allocated buffer that is filled by (for example)
    /// file I/O or decryption. If \c NeedsFreeing is NV_TRUE, the caller is
    /// responsible for freeing \c CalibrationData when it is no longer needed.
    /// If \c NeedsFreeing is NV_FALSE, the caller will simply discard the pointer
    /// when it is no longer needed.
    NvBool NeedsFreeing;
    /// Points to a null-terminated string containing the calibration data.
    const char *CalibrationData;
} NvOdmImagerCalibrationData;

typedef struct
{
    unsigned long HighLines;
    unsigned long LowLines;
    unsigned long ShortedLines;
} NvOdmImagerParallelDLICheck;

typedef struct
{
    unsigned long correct_pixels;
    unsigned long incorrect_pixels;
} NvOdmImagerDLICheck;

typedef enum {
    NvOdmImager_SensorType_Unknown,
    NvOdmImager_SensorType_Raw,
    NvOdmImager_SensorType_SOC,
} NvOdmImagerSensorType;

typedef struct {
    char SensorSerialNum[13]; /**< Up to 12 char, null terminated. */
    char PartNum[9];          /**< 8 char, null terminated. */
    char LensID[2];           /**< 1 char, null terminated. */
    char ManufactureID[3];    /**< 2 char, null terminated. */
    char FactoryID[3];        /**< 2 char, null terminated. */
    char ManufactureDate[10]; /**< DDMMMYYYY, null terminated. */
    char ManufactureLine[3];  /**< 2 char, null terminated. */
    char ModuleSerialNum[9];  /**< Up to 8 char, null terminated. */
    char FocuserLiftoff[5];   /**< 5 char, null terminated. */
    char FocuserMacro[5];     /**< 5 char, null terminated. */
    char ShutterCal[33];      /**< 32 char, null terminated. */
} NvOdmImagerModuleInfo;

typedef struct {
    NvU16 liftoff;
    NvU16 macro;
} NvOdmImagerFocuserCalibration;

typedef unsigned char NvOdmImager_SOC_TargetedExposure;

/**
 * Defines the ODM imager power levels.
 */
typedef enum {
    NvOdmImagerPowerLevel_Off = 1,
    /// Specifies standby, which keeps the register state.
    NvOdmImagerPowerLevel_Standby,
    NvOdmImagerPowerLevel_On,
    /// For stereo camera capture, power up both sensors, then power down them.
    /// This ensures both sensors will start sending frames at the same time.
    NvOdmImagerPowerLevel_SyncSensors,
    NvOdmImagerPowerLevel_Force32 = 0x7FFFFFFF,
} NvOdmImagerPowerLevel;
/**
 * Enumerates different imager devices.
 */
typedef enum {
    NvOdmImagerDevice_Sensor  = 0x0,
    NvOdmImagerDevice_Focuser,
    NvOdmImagerDevice_Flash,
    NvOdmImagerDevice_Num,
    NvOdmImagerDevice_Force32 = 0x7FFFFFFF,
} NvOdmImagerDevice;
/**
 * Defines self tests.
 * Each imager device can perform its own simple
 * tests for validation of the existence and performance
 * of that device. If the test is not applicable or
 * not possible, then it should just return NV_TRUE.
 */
typedef enum {
    NvOdmImagerSelfTest_Existence = 1,
    NvOdmImagerSelfTest_DeviceId,
    NvOdmImagerSelfTest_InitValidate,
    NvOdmImagerSelfTest_FullTest,
    NvOdmImagerSelfTest_Force32 = 0x7FFFFFFF,
} NvOdmImagerSelfTest;
/**
 * Defines bitmasks for different imager devices,
 * so that subset of devices could be specified
 * for power management functions.
 */
typedef enum {
    NvOdmImagerDevice_SensorMask  = (1<<NvOdmImagerDevice_Sensor),
    NvOdmImagerDevice_FocuserMask = (1<<NvOdmImagerDevice_Focuser),
    NvOdmImagerDevice_FlashMask   = (1<<NvOdmImagerDevice_Flash),
    NvOdmImagerDeviceMask_All = (1<<NvOdmImagerDevice_Num)-1,
    NvOdmImagerDeviceMask_Force32 = 0x7FFFFFFF,
} NvOdmImagerDeviceMask;

#define NVODM_IMAGER_MAX_FLASH_LEVELS 80
#define NVODM_IMAGER_MAX_TORCH_LEVELS 64
typedef struct NvOdmImagerFlashLevelInfoRec {
    NvF32 guideNum;
    NvU32 sustainTime;
    NvF32 rechargeFactor;
} NvOdmImagerFlashLevelInfo;

typedef struct {
    NvU32 NumberOfLevels;
    NvOdmImagerFlashLevelInfo levels[NVODM_IMAGER_MAX_FLASH_LEVELS];
} NvOdmImagerFlashCapabilities;

typedef struct NvOdmImagerTorchCapabilitiesRec
{
    NvU32 NumberOfLevels;
    /// Only the one field per level, @a guideNum, for torch modes.
    NvF32 guideNum[NVODM_IMAGER_MAX_TORCH_LEVELS];
} NvOdmImagerTorchCapabilities;

/**
 * The camera driver will precisely time the trigger of the strobe
 * to be in synchronization with the exposure time of the picture
 * being taken. This is done by toggling the PinState via our internal
 * chip register controls. But, the camera driver needs to know from the
 * nvodm what to write to the register.
 * The proper trigger signal for a particular flash device could be
 * active high or active low, so the PinState interface allows for
 * specification of either.
 * PinState uses two bitfields. The first one indicates which pins need
 * to be written; the second indicates which of those pins
 * must be pulled high. For example, if you wanted pin4
 * pulled low and pin5 pulled high, you'd use:
 * <pre>
 * 0000 0000 0011 0000
 * 0000 0000 0010 0000
 * </pre>
 * This interface requires the use of the VGP pins. (VI GPIO)
 * The recommended ones are VGP3, VGP4, VGP5, and VGP6.
 * Any other Tegra GPIOs cannot be controlled by the internal hardware
 * to be triggered in relation to frame capture.
*/
typedef struct {
    /// Specifies the pins to be written.
    NvU16 mask;
    /// Specifies the pins to pull high.
    NvU16 values;
} NvOdmImagerFlashPinState;

/**
 * Defines reset type.
 */
typedef enum {
   NvOdmImagerReset_Hard = 0,
   NvOdmImagerReset_Soft,
   NvOdmImagerReset_Force32 = 0x7FFFFFFF,
} NvOdmImagerReset;

/**
 * Holds the frame rate limit at the specified resolution.
 */
typedef struct NvOdmImagerFrameRateLimitAtResolutionRec
{
    NvSize Resolution;
    NvF32 MinFrameRate;
    NvF32 MaxFrameRate;
} NvOdmImagerFrameRateLimitAtResolution;

/**
 * Holds the parameter for the NvOdmImagerSetSensorMode() function.
 */
typedef struct SetModeParametersRec
{
    /// Specifies the resolution.
    NvSize Resolution;
    /// Specifies the exposure time.
    /// If Exposure = 0.0, sensor can set the exposure to its default value.
    NvF32 Exposure;
    /// Specifies the gains.
    /// [0] for R, [1] for GR, [2] for GB, [3] for B.
    /// If any one of the gains is 0.0, sensor can set the gains to its
    /// default values.
    NvF32 Gains[4];
} SetModeParameters;

#define NVODMIMAGER_AF_FOCUSER_CODE_VERSION                  1
#define NVODMIMAGER_AF_MAX_CONFIG_SET                       10
#define NVODMIMAGER_AF_MAX_CONFIG_DIST_PAIR                 16
#define NVODMIMAGER_AF_MAX_CONFIG_DIST_PAIR_ELEMENTS         2

typedef struct {
    NvS32 fdn;
    NvS32 distance;
} NvOdmImagerAfSetDistancePair;

typedef struct {
    NvS32 posture;
    NvS32 macro;
    NvS32 hyper;
    NvS32 inf;
    NvS32 settle_time;
    NvS32 hysteresis;
    NvS32 macro_offset;
    NvS32 inf_offset;
    NvU32 num_dist_pairs;
    NvOdmImagerAfSetDistancePair dist_pair[NVODMIMAGER_AF_MAX_CONFIG_DIST_PAIR];
} NvOdmImagerAfSet;



/**
 * Defines the imager focuser capabilities.
 */
typedef struct {
    /// Version of the focuser code implemented.
    NvU32 version;

    /// Holds the focuser mechanism motion range in model-specific steps.
    NvU32 actuatorRange;

    /// Holds the guaranteed settling time in millisecs.
    NvU32 settleTime;

    /// This will be true if the range boundaries reverse which takes
    /// macro to the low and infinity to the high end.
    NvBool rangeEndsReversed;

    /// Holds the total focuser range
    /// Actual low and high are obtained from focuser ODM
    NvS32 positionActualLow;
    NvS32 positionActualHigh;

    /// Holds the working focuser range that is a subset
    /// of the total range
    /// This is tunable/calibrated parameter
    NvS32 positionWorkingLow;
    NvS32 positionWorkingHigh;

    /// Holds the slew rate as a value from 0 - 9
    /// Value of 0 : Slew rate is disable
    /// Value of 1:  Slew rate set to default value
    /// Values 2 - 9: Different custom implementations
    NvU32 slewRate;

    /// The circle of confusion is the acceptable amount of defocus/blur for
    /// a point source in pixels.
    NvU32 circleOfConfusion;

    /// Set contains inf and macro values as well as their offsets
    NvOdmImagerAfSet afConfigSet[NVODMIMAGER_AF_MAX_CONFIG_SET];
    NvU32 afConfigSetSize;

} NvOdmImagerFocuserCapabilities;


/**
 *  Holds the custom info from the imager.
 */
typedef struct {
    NvU32 SizeOfData;
    NvU8 *pCustomData;
} NvOdmImagerCustomInfo;

typedef enum StereoCameraMode NvOdmImagerStereoCameraMode;

/**
 * Holds the inherent gain at the specified resolution.
 */
typedef struct NvOdmImagerInherentGainAtResolutionRec
{
    NvSize Resolution;
    NvF32 InherentGain;
    NvBool SupportsBinningControl;
    NvBool BinningControlEnabled;
} NvOdmImagerInherentGainAtResolution;

/**
 * Holds the characteristics of the imager for bracket operations.
 */
typedef struct NvOdmImagerBracketConfigurationRec{
    NvU32 FlushCount;
    NvU32 InitialIntraFrameSkip;
    NvU32 SteadyStateIntraFrameSkip;
    NvU32 SteadyStateFrameNumer;
} NvOdmImagerBracketConfiguration;

/**
 * Holds group-hold AE sensor settings.
 *
 * Group hold enables programming the sensor with multiple settings that are
 * grouped together, which the sensor then holds off on using until the whole
 * group can be programmed.
 */
typedef struct {
    NvF32 gains[4];
    NvBool gains_enable;
    NvF32 ET;
    NvBool ET_enable;
} NvOdmImagerSensorAE;

/**
 * Defines the ODM imager parameters.
 */
typedef enum
{
    /// Specifies exposure. Value must be an NvF32.
    NvOdmImagerParameter_SensorExposure = 0,
    /// Specifies per-channel gains. Value must be an array of 4 NvF32.
    /// [0]: gain for R, [1]: gain for GR, [2]: gain for GB, [3]: gain for B.
    NvOdmImagerParameter_SensorGain,
    /// Specifies read-only frame rate. Value must be an NvF32.
    NvOdmImagerParameter_SensorFrameRate,
    /// Specifies the maximal sensor frame rate the sensor should run at.
    /// Values must be an NvF32. The actual frame rate will be clamped by
    /// ::NvOdmImagerParameter_SensorFrameRateLimits. Setting 0 means no maximal
    /// frame rate is required.
    NvOdmImagerParameter_MaxSensorFrameRate,

    /// Specifies the input clock rate and PLL's.
    /// Value must be an ::NvOdmImagerClockProfile.
    /// Clock profile has the rate as an integer, specified in kHz,
    /// and a PLL multiplier as a float.
    NvOdmImagerParameter_SensorInputClock,
    /// Specifies focus position. Value must be an NvU32,
    /// in the range [0, actuatorRange - 1]. (The upper-end
    /// limit is specified in the ::NvOdmImagerFocuserCapabilities).
    NvOdmImagerParameter_FocuserLocus,
    /// Specifies high-level flash capabilities.  Read-only.
    /// Value must be an ::NvOdmImagerFlashCapabilities.
    NvOdmImagerParameter_FlashCapabilities,
    /// Specifies the level of flash to be used. NvF32.
    NvOdmImagerParameter_FlashLevel,
    /// Specifies any changes to the VGPs that need to occur
    /// to achieve the requested flash level. Read-only.
    /// Should be queried after setting ::NvOdmImagerParameter_FlashLevel.
    /// Value must be an ::NvOdmImagerFlashPinState value.
    NvOdmImagerParameter_FlashPinState,
    /// Specifies high-level torch capabilities.  Read-only.
    /// Value must be an ::NvOdmImagerTorchCapabilities.
    NvOdmImagerParameter_TorchCapabilities,
    /// Specifies the level of torch to be used. NvF32.
    NvOdmImagerParameter_TorchLevel,
    /// Specifies current focal length of lens. Constant for non-zoom lenses.
    /// Value must be an NvF32. Read-only.
    NvOdmImagerParameter_FocalLength,
    /// Specifies maximum aperture of lens. Value must be an NvF32. Read-only.
    NvOdmImagerParameter_MaxAperture,
    /// Specifies the f-number of the lens.
    /// As the f-number of a lens is the ratio of its focal
    /// length and aperture, rounded to the nearest number
    /// on a standard scale, this parameter is provided for
    /// convenience only. Value must be an NvF32. Read-only.
    NvOdmImagerParameter_FNumber,
    /// Specifies read-only minimum/maximum exposure.
    /// Exposure is specified in seconds. Value must be an array of 2 NvF32.
    /// [0]: minimum, [1]: maximum.
    NvOdmImagerParameter_SensorExposureLimits,
    /// Specifies read-only minimum/maximum gain (ISO value).
    /// Value must be an array of 2 NvF32. [0]: minimum, [1]: maximum.
    NvOdmImagerParameter_SensorGainLimits,
    /// Specifies read-only highest/lowest framerate.
    /// Framerate is specified in FPS, i.e.,  15 FPS is less than 60 FPS.
    /// Value must be an array of 2 NvF32. [0]: minimum, [1]: maximum.
    NvOdmImagerParameter_SensorFrameRateLimits,
    /// Specifies read-only highest/lowest framerate at a certain resolution.
    /// Framerate is specified in FPS, i.e., 15 FPS is less than 60 FPS.
    /// Value must be an ::NvOdmImagerFrameRateLimitAtResolution whose
    /// \a NvOdmImagerFrameRateLimitAtResolution.Resolution is the resolution
    /// at which the frame rate range is calculated.
    NvOdmImagerParameter_SensorFrameRateLimitsAtResolution,
    /// Specifies read-only slowest/fastest output clockrate.
    /// The sensor will use the master clock (input) and possibly
    /// increase or decrease the clock when sending the pixels.
    /// Clock rate is specified in KHz. Value must be an array of 2 NvU32.
    /// [0]: minimum, [1]: maximum.
    NvOdmImagerParameter_SensorClockLimits,
    /// Specifies read-only number of frames for exposure
    /// changes to take effect. Value must be an NvU32.
    NvOdmImagerParameter_SensorExposureLatchTime,
    /// Specifies read-only area used by the current resolution,
    /// in terms of the maximum resolution.
    /// This is used by the ISP to recalculate programming that
    /// is calibrated based on the maximum resolution, such as
    /// lens shading.
    /// Type is NvOdmImagerRegion.
    NvOdmImagerParameter_RegionUsedByCurrentResolution,
    /// Fills an ::NvOdmImagerCalibrationData structure with the
    /// calibration data.
    /// Sensors must be calibrated by NVIDIA in order
    /// to properly program the image signal processing
    /// engine and get favorable image results.
    /// If this particular sensor is not to be calibrated,
    /// then the parameter should return a NULL string.
    /// If the sensor should be calibrated, but a NULL string
    /// is returned, then the image processing will be crippled.
    /// If a string for the filename is returned and file open
    /// fails, the driver will be unable to operate, so the
    /// driver will signal the problem to the application, and
    /// go into a unknown state.
    NvOdmImagerParameter_CalibrationData,
    /// Fills a NvOdmImagerCalibrationData with any overrides
    /// for the calibration data.  This can be used for factory floor
    /// or field calibration.  Optional.
    NvOdmImagerParameter_CalibrationOverrides,
    /// Specifies the self test.
    /// This will initiate a sequence of writes and reads that
    /// allows our test infrastructure to validate the existence
    /// of a well behaving device. To use, the caller calls with
    /// a self-test request, ::NvOdmImagerSelfTest.
    /// The success or failure of the self test is communicated
    /// back to the caller via the return code. NV_TRUE indicates
    /// pass, NV_FALSE indicates invalid self-test or failure.
    NvOdmImagerParameter_SelfTest,
    /// Specifies device status.
    /// During debug, sometimes it is useful to have the
    /// device return information about the device.
    /// Returned is a generic array of NvU16's which
    /// would then be correlated to what the adaptation source
    /// describes it to be. For instance, if the capture hangs,
    /// the driver (in debug mode) can spit out the values of this
    /// array, allowing the person working with the nvodm source
    /// to have visibility into the sensor state.
    /// Use a pointer to an ::NvOdmImagerDeviceStatus struct.
    NvOdmImagerParameter_DeviceStatus,
    /// Specifies test mode.
    /// If a test pattern is supported, it can be enabled/disabled
    /// through this parameter.
    /// Expects a Boolean to be passed in.
    NvOdmImagerParameter_TestMode,
    /// Specifies test values.
    /// If a self-checking test is run with the test mode enabled,
    /// some expected values at several regions in the image
    /// will be needed. Use the ::NvOdmImagerExpectedValues structure.
    /// If the TestMode being enabled results in color bars, then
    /// these expected values may describe the rectangle for each bar.
    /// The expected values are given for the 2x2 arrangement for a set
    /// of bayer pixels. The Width and Height are expected to be even
    /// values, minimum being 2 by 2. A width or height of zero denotes
    /// an end of the list of expected values in the supplied array.
    /// Each resolution can return a different result; this parameter
    /// will be re-queried if the resolution changes.
    /// Returns a const ptr to an ::NvOdmImagerExpectedValues structure.
    NvOdmImagerParameter_ExpectedValues,
    /// Specifies reset type.
    /// Setting this parameter tiggers a sensor reset. The value should
    /// be one of ::NvOdmImagerReset enumerators.
    NvOdmImagerParameter_Reset,
    /// Specifies if the sensor supports optimized resolution changes.
    /// Return NV_FALSE if it is asked to optimize but the sensor is
    /// not able to optimize resolution change.
    /// Return NV_TRUE otherwise.
    /// Value must be an ::NvBool.
    NvOdmImagerParameter_OptimizeResolutionChange,
    /// Detected Color Temperature
    /// Read-only. If the sensor supports determining the color
    /// temperature, this can serve as a hint to the AWB algorithm.
    /// Each frame where the AWB is performing calculations will
    /// get this parameter, and if \a GetParameter returns ::NV_TRUE,
    /// then the value will be fed into the algorithm. If it returns
    /// ::NV_FALSE, then the value is ignored.
    /// Units are in degrees kelvin. Example: 6000 kelvin is typical daylight.
    /// Value must be an NvU32.
    NvOdmImagerParameter_DetectedColorTemperature,

    /// Gets number of lines per second at current resolution.
    /// Parameter type must be an NvF32.
    NvOdmImagerParameter_LinesPerSecond,

    /// Provides imager-specific low-level capabilities data for a focusing
    /// device (::NvOdmImagerFocuserCapabilities structure).
    /// This can be used for factory-floor or field calibration, if necessary.
    NvOdmImagerParameter_FocuserCapabilities,

    /// (Read-only)
    /// Provides a custom block of info from the ODM imager. The block of info
    /// that is returned is private and is known to the ODM implementation.
    /// Parameter type is ::NvOdmImagerCustomInfo.
    NvOdmImagerParameter_CustomizedBlockInfo,

    /// (Read-only)
    /// Indicate if this is a stereo camera.
    NvOdmImagerParameter_StereoCapable,

    /// Select focuser stereo mode.
    NvOdmImagerParameter_FocuserStereo,

    /// Select stereo camera mode. Must be set before sensor power on.
    /// Has no effect for non-stereo camera and will return NV_FALSE.
    /// Parameter type is ::NvOdmImagerStereoCameraMode.
    NvOdmImagerParameter_StereoCameraMode,

    /// Specifies read-only inherent gain at a certain resolution.
    /// Some sensors use summation when downscaling image data.
    /// Value must be an ::NvOdmImagerInherentGainAtResolution whose
    /// \a NvOdmImagerFrameRateLimitAtResolution.Resolution is the resolution
    /// at which the inherent gain is queried.
    /// A Resolution of 0x0 will instead query the gain for the current
    /// resolution.
    /// This parameter is optional, and only needs to be supported in
    /// adaptations that are written for sensors that use summation,
    /// resulting in brighter pixels.  If the pixels during preview are
    /// at the same brightness as still (full-res), then this is likely
    /// not needed.
    NvOdmImagerParameter_SensorInherentGainAtResolution,

    /// Specifies current horizontal view angle of lens.
    /// Value must be an NvF32. Read-only.
    NvOdmImagerParameter_HorizontalViewAngle,

    /// Specifies current vertical view angle of lens.
    /// Value must be an NvF32. Read-only.
    NvOdmImagerParameter_VerticalViewAngle,

    /// Support for setting an SOC sensor's parameters, specifically the ISP
    /// parameters, is done through ISPSetting.  This parameter sends the
    /// ::NvOdmImagerISPSetting struct to pass along the settings.
    /// Please refer to nvcamera_isp.h for all the types of messages and
    /// datatypes used.
    NvOdmImagerParameter_ISPSetting,

    /// (Write-only)
    /// Specifies the current operational mode for an External ISP.
    /// When an external ISP is used, the driver's knowledge of whether
    /// we are in viewfinder-only mode, taking a picture, taking a burst
    /// capture, or shooting a video is passed down. This allows the ODM to make
    /// intelligent decisions based off of this information.
    /// Essentially, this is just a hint passed from the camera driver to the
    /// imager ODM.
    /// Parameter type is ::NvOdmImagerOperationalMode.
    NvOdmImagerParameter_OperationalMode,

    /// (Read-only)
    /// Queryies sensor ISP support.
    /// Parameter type is NvBool.
    NvOdmImagerParameter_SensorIspSupport,

    /// Support for locking the sensor's AWB algorithm.
    /// Value must be a pointer to an ::NvOdmImagerAlgLock struct.
    NvOdmImagerParameter_AWBLock,

    /// Support for locking the sensor's AE algorithm.
    /// Value must be a pointer to an ::NvOdmImagerAlgLock struct.
    NvOdmImagerParameter_AELock,

    /// Specifies when the last resolution/exposure setting becomes effective.
    NvOdmImagerParameter_SensorResChangeWaitTime,

    /// Specifies per module factory calibration data.
    NvOdmImagerParameter_FactoryCalibrationData,

    /// Specifies the sensor ID is read from the device.
    NvOdmImagerParameter_SensorID,

    /// Specifies sensor group-hold control.
    /// Parameter type is ::NvOdmImagerSensorAE for set parameter.
    /// Parameter type is NvBool for get parameter.
    NvOdmImagerParameter_SensorGroupHold,

    /// Specifies the time it takes to read out the active
    /// region.
    NvOdmImagerParameter_SensorActiveRegionReadOutTime,

    /// Allow adaptations to select the best matching sensor mode for a given
    /// resolution, framerate, and usage type.
    /// Use with NvOdmImagerGetParameter.
    /// Parameter type: NvOdmImagerSensorMode
    ///
    /// On entry, the parameter of type NvOdmImagerSensorMode will hold resolution,
    /// framerate, and type of the sensor mode to be selected. An implementation should
    /// write the best matching mode from its mode list into this same struct and
    /// return NV_TRUE. NvMM will then call NvOdmImagerSetMode with that mode.
    ///
    /// Return NV_FALSE to let NvMM perform the mode selection (default).
    NvOdmImagerParameter_GetBestSensorMode,

    /// If the \c NvOdmImagerParameter enum list is extended,
    /// put the new definitions after \c BeginVendorExtentions.
    NvOdmImagerParameter_BeginVendorExtensions = 0x10000000,

    /** For libcamera, specifies to get calibration status. */
    NvOdmImagerParameter_CalibrationStatus,

    /** For libcamera, specifies to control imager test patterns. */
    NvOdmImagerParameter_TestPattern,

    /** Retrieves module info. */
    NvOdmImagerParameter_ModuleInfo,

    /** Specifies the maximum power for strobe (during test commands). */
    NvOdmImagerParameter_FlashMaxPower,

    /** Specifies the sensor direction. */
    NvOdmImagerParameter_Direction,

    /** Specifies the sensor type. */
    NvOdmImagerParameter_SensorType,

    /** Specifies the DLI test function. */
    NvOdmImagerParameter_DLICheck,

    /** Specifies the DLI test function. */
    NvOdmImagerParameter_ParallelDLICheck,

    /** Specifies the imager bracket capabilities. */
    NvOdmImagerParameter_BracketCaps,

    // For sensors which use signle shot mode to trigger
    // and not always sends data after power on.
    // This parameter will be set in nvmm-camera
    // after csi is triggered and after this in
    // odm code sensor can be triggered.
    NvOdmImagerParameter_SensorTriggerStill,

    ///
    NvOdmImagerParameter_Num,

    /// Ignore -- Forces compilers to make 32-bit enums.
    NvOdmImagerParameter_Force32 = 0x7FFFFFFF
} NvOdmImagerParameter;

//  Defines functions to be implemented by each imager.
typedef struct NvOdmImagerRec *NvOdmImagerHandle;
#define MAX_STATUS_VALUES 160
typedef struct NvOdmImagerDeviceStatusRec
{
    NvU16 Count;
    NvU16 Values[MAX_STATUS_VALUES];
} NvOdmImagerDeviceStatus;
#define MAX_EXPECTED_VALUES 16
typedef struct NvOdmImagerRegionValueRec
{
    NvU32 X, Y;
    NvU32 Width, Height;
    /// Specifies Bayer pixel values.
    NvU16 R;
    NvU16 Gr;
    NvU16 Gb;
    NvU16 B;
} NvOdmImagerRegionValue;
typedef struct NvOdmImagerExpectedValuesRec
{
    NvOdmImagerRegionValue Values[MAX_EXPECTED_VALUES];
} NvOdmImagerExpectedValues, *NvOdmImagerExpectedValuesPtr;

/// Passes along data defined in
/// nvcamera_isp.h when the internal Tegra ISP is not used.
typedef void *NvOdmImagerISPSettingDataPointer;
typedef struct NvOdmImagerISPSettingRec
{
    NvCameraIspAttribute attribute;
    NvOdmImagerISPSettingDataPointer pData;
    NvU32 dataSize;
} NvOdmImagerISPSetting;

typedef struct NvOdmImagerAlgLockRec
{
    NvBool locked;
    NvBool relock;
} NvOdmImagerAlgLock;

/// Passes the information about the current
/// driver state.
/// - If the preview is active and video active, then the
/// \a PreviewActive and \a VideoActive Boolean values will be TRUE.
/// - If taking a picture, then \a PreviewActive may be TRUE, and \a StillCount
/// will be 1.
/// - If a burst capture, then \a StillCount will be a number larger than 1.
/// - If cancelling a burst capture, \a StillCount will be 0.
/// - If AE, AF, and AWB are active \a HalfPress will be TRUE.
/// This structure is passed to ODM when any value has changed. The structure
/// will contain the current, complete state of the driver.
typedef struct NvOdmImagerOperationModeRec
{
    NvBool PreviewActive;
    NvBool VideoActive;
    NvBool HalfPress;
    NvU32 StillCount;
} NvOdmImagerOperationalMode;


/** Gets the capabailities (the details) for the specified imager.
 * This fills out the capabilities structure with the timings (resolutions),
 * the pixel types, the name, and the type of imager.
 *
 * @param Imager The handle to the imager.
 * @param Capabilities A pointer to the capabilities structure to fill.
 */
void NvOdmImagerGetCapabilities(
        NvOdmImagerHandle Imager,
        NvOdmImagerCapabilities *Capabilities);
/** Gets the list of resolutions for sensor device.
 * This will fill the array of structures with
 * the resolution information.
 * A single structure will be selected, then passed to the
 * NvOdmImagerSetSensorMode() function to set the resolution.
 * This function is intended for query purposes only; no side
 * effects are expected.
 *
 * @param Imager The handle to the NvOdmImagerSensorMode structure.
 * @param Modes A pointer to the timings, or NULL to get only the NumberOfModes.
 * @param NumberOfModes A pointer to the number of timings.
 */
void NvOdmImagerListSensorModes(
        NvOdmImagerHandle Imager,
        NvOdmImagerSensorMode *Modes,
        NvS32 *NumberOfModes);
/** Sets the resolution and format.
 * This can be used to change the resolution or format at any time,
 * though with most imagers, it will not take effect until the next
 * frame or two.
 * The sensor may or may not support resolution changes, and may or may
 * not support arbitrary resolutions. If it cannot match the resolution,
 * the imager will return the next largest resolution than the one
 * requested.
 *
 * @param hImager The handle to the imager.
 * @param pParameters A pointer to the desired ::SetModeParameters.
 *                    After this function returns, sensor must be programed
 *                    to the desired parameters to avoid exposure blip.
 * @param pSelectedMode A pointer to the resolution that was set, and the
 *                     corresponding offset in the pixel stream to start at.
 * @param pResult A pointer to the resulting values.
 *
 * @return NV_TRUE if successful, or NV_FALSE otherwise.
 */
NvBool NvOdmImagerSetSensorMode(
        NvOdmImagerHandle hImager,
        const SetModeParameters *pParameters,
        NvOdmImagerSensorMode *pSelectedMode,
        SetModeParameters *pResult);

/** Sets the power level to on, off, or standby
 *  for a (sub)set of imager devices.
 *
 * @param Imager The handle to the imager.
 * @param Devices The bitmask for imager devices.
 * @param PowerLevel The power level to set.
 *
 * @return NV_TRUE if successful, or NV_FALSE otherwise.
 */
NvBool NvOdmImagerSetPowerLevel(
        NvOdmImagerHandle Imager,
        NvOdmImagerDeviceMask Devices,
        NvOdmImagerPowerLevel PowerLevel);
/** Gets the power level for specified imager device.
 *
 * @param Imager The handle to the imager.
 * @param Device The imager device.
 * @param PowerLevel A pointer to the returned power level.
*/
void NvOdmImagerGetPowerLevel(
        NvOdmImagerHandle Imager,
        NvOdmImagerDevice Device,
        NvOdmImagerPowerLevel *PowerLevel);
/** Sets the parameter indicating an imager setting.
 * @see NvOdmImagerParameter
 *
 * @param Imager The handle to the imager.
 * @param Param The parameter to set.
 * @param SizeOfValue The size of the value.
 * @param Value A pointer to the value.
 *
 * @return NV_TRUE if successful, or NV_FALSE otherwise.
 */
NvBool NvOdmImagerSetParameter(
        NvOdmImagerHandle Imager,
        NvOdmImagerParameter Param,
        NvS32 SizeOfValue,
        const void *Value);
/** Gets the parameter, which queries a variable-sized imager setting.
 * @see NvOdmImagerParameter
 *
 * @param Imager The handle to the imager.
 * @param Param The returned parameter.
 * @param SizeOfValue The size of the value.
 * @param Value A pointer to the value.
 *
 * @return NV_TRUE if successful, or NV_FALSE otherwise.
 */
NvBool NvOdmImagerGetParameter(
        NvOdmImagerHandle Imager,
        NvOdmImagerParameter Param,
        NvS32 SizeOfValue,
        void *Value);

/*  Functions to be implemented once  */
/**
 * The first step in using an imager is to open the imager and get a handle.
 * The supplied globally unique identifier (\a ImagerGUID) tells the Open
 * routine which imager is to be used. If ImagerGUID==0, then the imager
 * will pick what it determines to be the default. This is completely
 * subject to the NVODM imager implementation. ImagerGUID==1 selects the
 * next available default imager, likely used as a secondary device.
 * To obtain a list of GUIDs available, please use the functions in
 * nvodm_query_discovery.h, like NvOdmPeripheralEnumerate().
 * Returns NV_TRUE if successful, NV_FALSE if GUID is not recognized or
 * no imagers are available.
 */
#define NVODM_IMAGER_MAX_REAL_IMAGER_GUID 10
NvBool NvOdmImagerOpen(NvU64 ImagerGUID, NvOdmImagerHandle *phImager);
/**
 * Releases any resources and memory utilized.
 * @param hImager A handle to the imager to release.
 */
void NvOdmImagerClose(NvOdmImagerHandle hImager);

/**
 * @deprecated Please use nvodm_query_discovery.h.
 */
NvBool
NvOdmImagerGetDevices( NvS32 *Count, NvOdmImagerHandle *Imagers);
/**
 * @deprecated Please use nvodm_query_discovery.h.
 */
void
NvOdmImagerReleaseDevices( NvS32 Count, NvOdmImagerHandle *Imagers);

/** Workaround so libcamera code can access libodm_imager code directly. */
NvOdmImagerHandle NvOdmImagerGetHandle(void);

#if defined(_cplusplus)
}
#endif
/** @} */
#endif // INCLUDED_NVODM_SENSOR_H
