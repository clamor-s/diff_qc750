/*
 * Copyright (c) 2008 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
 #include <errno.h>
#include <fcntl.h>
//#include "sensor_yuv.h"
#include "nvassert.h" 
#include "imager_util.h"
//#include <cutils/log.h>
#include "sensor_yuv_mt9p111.h"
#include "mt9p111.h"

/****************************************************************************
 * Implementing for a New Sensor:
 * 
 * When implementing for a new sensor from this template, there are four 
 * phases:
 * Phase 0: Gather sensor information. In phase 0, we should gather the sensor
 *          information for bringup such as sensor capabilities, set mode i2c
 *          write sequence, NvOdmIoAddress in nvodm_query_discovery_imager.h,
 *          and so on.
 * Phase 1: Bring up a sensor. After phase 1, we should be able to do still
 *          captures. Since this is a yuv sensor, there won't be many
 *          interactions between camera and sensor.
 * Phase 2: Fully functioning sensor. After phase 2, the sensor should be fully
 *          functioning as described in nvodm_imager.h
 *
 * To help implementing for a new sensor, the template code will be marked as
 * Phase 0, 1, 2
 ****************************************************************************/

#define CAMERA_VERSION "1.1.5"
#define LENS_FOCAL_LENGTH (10.0f)
#define NVODMSENSOR_ENABLE_PRINTF (1)

#if NVODMSENSOR_ENABLE_PRINTF
    #define NVODMSENSOR_PRINTF(x)   NvOsDebugPrintf x
#else
    #define NVODMSENSOR_PRINTF(x)   do {} while(0)
#endif

/**
 * ----------------------------------------------------------
 *  Start of Phase 0 code
 * ----------------------------------------------------------
 */

/**
 * Sensor Specific Variables/Defines. Phase 0.
 *
 * If a new sensor code is created from this template, the following
 * variable/defines should be created for the new sensor specifically.
 * 1. A sensor capabilities (NvOdmImagerCapabilities) for this specific sensor
 * 2. A sensor set mode sequence list  (an array of SensorSetModeSequence)
 *    consisting of pairs of a sensor mode and corresponding set mode sequence
 * 3. A set mode sequence for each mode defined in 2.
 */

/**
 * Sensor yuv's capabilities. Phase 0. Sensor Dependent.
 * This is the first thing the camera driver requests. The information here
 * is used to setup the proper interface code (CSI, VIP, pixel type), start
 * up the clocks at the proper speeds, and so forth.
 * For Phase 0, one could start with these below as an example for a yuv
 * sensor and just update clocks from what is given in the reference documents
 * for that device. The FAE may have specified the clocks when giving the
 * reccommended i2c settings.
 */
//renn update file
#if(BUILD_FOR_GC0308 == 1)
static NvOdmImagerCapabilities g_SensorYuvCaps =
{
  #if(BUILD_FOR_HM2057== 1)
     "hm2057",  // (Aptina 5140)string identifier, used for debugging
   #else
     "gc0308",  // (Aptina 5140)string identifier, used for debugging
   #endif
    
    NvOdmImagerSensorInterface_Parallel8,//NvOdmImagerSensorInterface_SerialA,
    {
       NvOdmImagerPixelType_YUYV,//NvOdmImagerPixelType_UYVY
    },
    NvOdmImagerOrientation_0_Degrees, // rotation with respect to main display
    NvOdmImagerDirection_Away,          // toward or away the main display
    6000, // initial sensor clock (in khz)
   //{{25000, 130.f/25.f}}, // preferred clock profile
  #if(BUILD_FOR_HM2057== 1)
     {{48000, 130.f/48.f}}, 
   #else
     {{24000, 130.f/24.f}}, 
   #endif
    {
        NvOdmImagerSyncEdge_Rising, // hsync polarity
       NvOdmImagerSyncEdge_Rising, // vsync polarity
        NV_FALSE,                   // uses mclk on vgp0
    }, // parallel settings (VIP)
    {0,0,0,NV_FALSE,0}, // serial settings (CSI) (not used)
    { 0, 0 }, // min blank time, shouldn't need adjustment
    0, // preferred mode, which resolution to use on startup
    0, // FOCUSER_GUID, only set if focuser code available
    0, //FLASH_LTC3216_GUID, // FLASH_GUID, only set if flash device code is available
    NVODM_IMAGER_CAPABILITIES_END
};

		
#elif(BUILD_FOR_MT9P111 == 1)
static NvOdmImagerCapabilities g_SensorYuvCaps =
{
    "MT9P111",  // (Aptina 5140)string identifier, used for debugging
    NvOdmImagerSensorInterface_SerialA,
    {
        NvOdmImagerPixelType_UYVY,
    },
    NvOdmImagerOrientation_0_Degrees, // rotation with respect to main display
    NvOdmImagerDirection_Away,          // toward or away the main display
    6000, // initial sensor clock (in khz)
   //{{25000, 130.f/25.f}}, // preferred clock profile
     {{24000, 140.f/24.f}}, 
    {
        NvOdmImagerSyncEdge_Rising, // hsync polarity
        NvOdmImagerSyncEdge_Rising, // vsync polarity
        NV_FALSE,                   // uses mclk on vgp0
    }, // parallel settings (VIP)
    { 
        NVODMSENSORMIPITIMINGS_PORT_CSI_A,
        NVODMSENSORMIPITIMINGS_DATALANES_ONE,
        NVODMSENSORMIPITIMINGS_VIRTUALCHANNELID_ONE,
        0,  
        0x4,
    }, // serial settings (CSI) 
    { 0, 0 }, // min blank time, shouldn't need adjustment
    0, // preferred mode, which resolution to use on startup
    0, // FOCUSER_GUID, only set if focuser code available
    0, //FLASH_LTC3216_GUID, // FLASH_GUID, only set if flash device code is available
    NVODM_IMAGER_CAPABILITIES_END
};
#else
static NvOdmImagerCapabilities g_SensorYuvCaps =
{
    "MT9P111",  // (Aptina 5140)string identifier, used for debugging
    NvOdmImagerSensorInterface_SerialA,
    {
        NvOdmImagerPixelType_UYVY,
    },
    NvOdmImagerOrientation_0_Degrees, // rotation with respect to main display
    NvOdmImagerDirection_Away,          // toward or away the main display
    6000, // initial sensor clock (in khz)
   //{{25000, 130.f/25.f}}, // preferred clock profile
     {{24000, 140.f/24.f}}, 
    {
        NvOdmImagerSyncEdge_Rising, // hsync polarity
        NvOdmImagerSyncEdge_Rising, // vsync polarity
        NV_FALSE,                   // uses mclk on vgp0
    }, // parallel settings (VIP)
    { 
        NVODMSENSORMIPITIMINGS_PORT_CSI_A,
        NVODMSENSORMIPITIMINGS_DATALANES_ONE,
        NVODMSENSORMIPITIMINGS_VIRTUALCHANNELID_ONE,
        0,  
        0x4,
    }, // serial settings (CSI) 
    { 0, 0 }, // min blank time, shouldn't need adjustment
    0, // preferred mode, which resolution to use on startup
    0, // FOCUSER_GUID, only set if focuser code available
    0, //FLASH_LTC3216_GUID, // FLASH_GUID, only set if flash device code is available
    NVODM_IMAGER_CAPABILITIES_END
};
#endif

#if 0
static DevCtrlReg16 SetModeSequence_2592x1944[] =
{
{SEQUENCE_END, 0x0000}
};

static DevCtrlReg16 SetModeSequence_1280x720[] =
{
{SEQUENCE_END, 0x0000}
};

static DevCtrlReg16 SetModeSequence_640x480[] =
{
{SEQUENCE_END, 0x0000}
};
static DevCtrlReg16 SetModeSequence_1600x1200[] =
{
{SEQUENCE_END, 0x0000}
};
#endif

/**
 * A list of sensor mode and its SetModeSequence. Phase 0. Sensor Dependent.
 * When SetMode is called, it searches the list for desired resolution and
 * frame rate and write the SetModeSequence through i2c.
 */
static SensorSetModeSequence g_SensorYuvSetModeSequenceList[] =
{
     // WxH        x, y    fps   set mode sequence
    #if(BUILD_FOR_HM2057== 1)
    //{{{1600, 1200}, {0, 0},  7, 1.0,20,NvOdmImagerNoCropMode}, NULL, NULL},
    {{{1280, 720}, {0, 0},  15, 1.0,20,NvOdmImagerNoCropMode}, NULL, NULL},
    {{{960, 720}, {0, 0},  15, 1.0,20,NvOdmImagerNoCropMode}, NULL, NULL},
    {{{800, 600}, {0, 0},  30, 1.0,20,NvOdmImagerNoCropMode}, NULL, NULL},
    #endif
    {{{640, 480}, {0, 0},  30, 1.0,20,NvOdmImagerNoCropMode}, NULL, NULL},
}; 

/**
 * ----------------------------------------------------------
 *  Start of Phase 1 code
 * ----------------------------------------------------------
 */

/**
 * Sensor yuv's private context. Phase 1.
 */
typedef struct SensorYuvContextRec
{
    int   camera_fd;//add for keenhi
    // Phase 1 variables.
    NvOdmImagerI2cConnection I2c;
    NvU32 ModeIndex;
    NvU32 NumModes;
    NvOdmImagerPowerLevel PowerLevel;
    ImagerGpioConfig GpioConfig;

}SensorYuvContext;

/**
 * Static functions
 * Some compilers require these to be declared upfront.
 */
static NvBool SensorYuv_MT9P111_Open(NvOdmImagerHandle hImager);
static void SensorYuv_MT9P111_Close(NvOdmImagerHandle hImager);

static void
SensorYuv_MT9P111_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities);

static void
SensorYuv_MT9P111_ListModes(
    NvOdmImagerHandle hImager,
    NvOdmImagerSensorMode *pModes,
    NvS32 *pNumberOfModes);
    
static NvBool
SensorYuv_MT9P111_SetMode(
    NvOdmImagerHandle hImager,
    const SetModeParameters *pParameters,
    NvOdmImagerSensorMode *pSelectedMode,
    SetModeParameters *pResult);

static NvBool
SensorYuv_MT9P111_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel);

static NvBool
SensorYuv_MT9P111_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue);

static NvBool
SensorYuv_MT9P111_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue);

static void
SensorYuv_MT9P111_GetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel *pPowerLevel);


/**
 * SensorYuv_Open. Phase 1.
 * Initialize sensor yuv and its private context
 */
static NvBool SensorYuv_MT9P111_Open(NvOdmImagerHandle hImager)
{
     NVODMSENSOR_PRINTF(("#########################SensorYuv_MT9P111_Open\n"));
    SensorYuvContext *pSensorYuvContext = NULL;
    NvOdmImagerI2cConnection *pI2c = NULL;

    if (!hImager || !hImager->pSensor)
        return NV_FALSE;

    pSensorYuvContext =
        (SensorYuvContext*)NvOdmOsAlloc(sizeof(SensorYuvContext));
    if (!pSensorYuvContext)
        goto fail;

    NvOdmOsMemset(pSensorYuvContext, 0, sizeof(SensorYuvContext));

    // Setup the i2c bit widths so we can call the
    // generalized write/read utility functions.
    // This is done here, since it will vary from sensor to sensor.
    pI2c = &pSensorYuvContext->I2c;
#if(BUILD_FOR_MT9P111 == 1)
    pI2c->AddressBitWidth = 16;
    pI2c->DataBitWidth = 16;
#endif
#if(BUILD_FOR_GC0308 == 1)
    pI2c->AddressBitWidth = 8;
    pI2c->DataBitWidth = 8;
#endif
    pSensorYuvContext->NumModes =
        NV_ARRAY_SIZE(g_SensorYuvSetModeSequenceList);
    pSensorYuvContext->ModeIndex =
        pSensorYuvContext->NumModes + 1; // invalid mode
    
    hImager->pSensor->pPrivateContext = pSensorYuvContext;

    return NV_TRUE;    

fail:
    NvOdmOsFree(pSensorYuvContext);
    return NV_FALSE;
}

/**
 * SensorYuv_Close. Phase 1.
 * Free the sensor's private context
 */
static void SensorYuv_MT9P111_Close(NvOdmImagerHandle hImager)
{
    SensorYuvContext *pContext = NULL;
            NVODMSENSOR_PRINTF(("#########################SensorYuv_MT9P111_Close \n"));
    if (!hImager || !hImager->pSensor || !hImager->pSensor->pPrivateContext)
        return;

    pContext = (SensorYuvContext*)hImager->pSensor->pPrivateContext;

    // cleanup
    NvOdmOsFree(pContext);
    hImager->pSensor->pPrivateContext = NULL;
}

/**
 * SensorYuv_GetCapabilities. Phase 1.
 * Returnt sensor Yuv's capabilities
 */
static void
SensorYuv_MT9P111_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities)
{
    // Copy the sensor caps from g_SensorYuvCaps
    NVODMSENSOR_PRINTF(("#########################SensorYuv_MT9P111_GetCapabilities \n"));
    NvOdmOsMemcpy(pCapabilities, &g_SensorYuvCaps,
        sizeof(NvOdmImagerCapabilities));
}

/**
 * SensorYuv_ListModes. Phase 1.
 * Return a list of modes that sensor yuv supports.
 * If called with a NULL ptr to pModes, then it just returns the count
 */
static void
SensorYuv_MT9P111_ListModes(
    NvOdmImagerHandle hImager,
    NvOdmImagerSensorMode *pModes,
    NvS32 *pNumberOfModes)
{
    SensorYuvContext *pContext =
        (SensorYuvContext*)hImager->pSensor->pPrivateContext;
    NvS32 i;
NVODMSENSOR_PRINTF(("#########################SensorYuv_MT9P111_ListModes \n"));
    if (pNumberOfModes)
    {
        *pNumberOfModes = (NvS32)pContext->NumModes;
        if (pModes)
        {
            // Copy the modes from g_SensorYuvSetModeSequenceList
            for (i = 0; i < *pNumberOfModes; i++)
                pModes[i] = g_SensorYuvSetModeSequenceList[i].Mode;
        }
    }
}

/**
 * SensorYuv_SetMode. Phase 1.
 * Set sensor yuv to the mode of the desired resolution and frame rate.
 */
static NvBool
SensorYuv_MT9P111_SetMode(
    NvOdmImagerHandle hImager,
    const SetModeParameters *pParameters,
    NvOdmImagerSensorMode *pSelectedMode,
    SetModeParameters *pResult)
{
    NvBool Status = NV_FALSE;
    SensorYuvContext *pContext =
        (SensorYuvContext*)hImager->pSensor->pPrivateContext;
    NvU32 Index;
    //NvU16 data;
	struct sensor_mode mode;

    NV_ASSERT(pParameters);

    NVODMSENSOR_PRINTF(("Setting resolution to ===============>%dx%d\n",
        pParameters->Resolution.width, pParameters->Resolution.height));

    // Find the right entrys in g_SensorYuvSetModeSequenceList that matches
    // the desired resolution and framerate
    for (Index = 0; Index < pContext->NumModes; Index++)
    {
        if ((pParameters->Resolution.width == g_SensorYuvSetModeSequenceList[Index].
             Mode.ActiveDimensions.width) &&
            (pParameters->Resolution.height == g_SensorYuvSetModeSequenceList[Index].
             Mode.ActiveDimensions.height))
            break;
    }

    // No match found
    if (Index == pContext->NumModes)
        return NV_FALSE;

    if (pSelectedMode)
    {
        *pSelectedMode = g_SensorYuvSetModeSequenceList[Index].Mode;
    }

    if (pContext->ModeIndex == Index)
        return NV_TRUE;

    // i2c writes for the set mode sequence

    NvOdmOsDebugPrintf("Setting resolution write sequence index: %d\n", Index);

	//add for keenhi start
    mode.xres = g_SensorYuvSetModeSequenceList[Index].Mode.ActiveDimensions.width;
    mode.yres = g_SensorYuvSetModeSequenceList[Index].Mode.ActiveDimensions.height;
   if(pContext->camera_fd)
    	Status = ioctl(pContext->camera_fd, SENSOR_IOCTL_SET_MODE, &mode);
    else{
		Status = -1;
    	}
    if (Status < 0) {
        return NV_FALSE;
    } 
	//add for keenhi end
/*
    Status = NvOdmImagerI2cRead(&pContext->I2c, 0xC86C, &data);
    if (Status != NvOdmI2cStatus_Success)
       return NV_FALSE;

//Aptina can support Context B fast switching capture. No need to re-program the table again when switching back to preview from capture
    if (!(data == 0x0518 && Index == 2))
    {
    Status = WriteI2CSequence(&pContext->I2c,
                 g_SensorYuvSetModeSequenceList[Index].pSequence, NV_FALSE);
    if (!Status)
        return NV_FALSE;
    }
*/
    pContext->ModeIndex = Index;

    if (pResult)
    {
        pResult->Resolution = g_SensorYuvSetModeSequenceList[Index].
                              Mode.ActiveDimensions;
        pResult->Exposure = 0;
        NvOdmOsMemset(pResult->Gains, 0, sizeof(NvF32) * 4);
    }

    return NV_TRUE;
}

/**
 * SensorYuv_SetPowerLevel. Phase 1
 * Set the sensor's power level
 */
static NvBool
SensorYuv_MT9P111_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel)
{
    NvBool Status = NV_TRUE;

    SensorYuvContext *pContext =
        (SensorYuvContext*)hImager->pSensor->pPrivateContext;
    //const NvOdmPeripheralConnectivity *pConnections;

    if (pContext->PowerLevel == PowerLevel)
        return NV_TRUE;
#if (BUILD_FOR_AOS == 0)
   switch(PowerLevel)
    {
        case NvOdmImagerPowerLevel_On:
		NVODMSENSOR_PRINTF(("1#########################SensorYuv_MT9P111_SetPowerLevel Level_On\n"));
	     //add for keenhi start
	      //LOGD("SensorYuv_MT9P111_SetPowerLeve open this camera fd");
	      #if(BUILD_FOR_HM2057== 1)
	     	pContext->camera_fd = open(HM2057_SENSOR_PATH, O_RDWR);
		#else
			pContext->camera_fd = open(GC0308_SENSOR_PATH, O_RDWR);
		#endif
            if (pContext->camera_fd < 0) 
            {
                NvOsDebugPrintf("1Can not open camera device: %s\n", strerror(errno));
                Status = NV_FALSE;
            } 
#if(BUILD_FOR_MT9P111 == 1)
	     NvOdmOsWaitUS(10000); // wait 10ms
    	     ioctl(pContext->camera_fd, SENSOR_IOCTL_SET_GET_SHADING, NULL);	     
#endif
            NVODMSENSOR_PRINTF(("Camera fd open as: %d\n", pContext->camera_fd));
	    //add for keenhi end  
            break;

        case NvOdmImagerPowerLevel_Standby:
		Status = NV_TRUE;
            break;
        case NvOdmImagerPowerLevel_Off:
	NVODMSENSOR_PRINTF(("1#########################SensorYuv_MT9P111_SetPowerLevel Level_Off\n"));
            
	     //add for keenhi start
	     close(pContext->camera_fd);
            pContext->camera_fd = -1;
	    NVODMSENSOR_PRINTF(("1SensorYuv_MT9P111_SetPowerLeve close this camera fd"));
	     //add for keenhi end
	     NvOdmOsWaitUS(100000);
            break;
        default:
            NV_ASSERT("!Unknown power level\n");
            Status = NV_TRUE;
            break;
    }
#else
NVODMSENSOR_PRINTF(("2#########################SensorYuv_MT9P111_SetPowerLevel \n"));
    NV_ASSERT(hImager->pSensor->pConnections);
    pConnections = hImager->pSensor->pConnections;

    switch(PowerLevel)
    {
        case NvOdmImagerPowerLevel_On:
            Status = SetPowerLevelWithPeripheralConnectivity(pConnections,
                         &pContext->I2c, &pContext->GpioConfig,
                         NvOdmImagerPowerLevel_On);
            if (!Status)
            {
                NvOsDebugPrintf("2NvOdmImagerPowerLevel_On failed.\n");
                return NV_FALSE;
            }
		NVODMSENSOR_PRINTF(("2#########################SensorYuv_MT9P111_SetPowerLevel Level_On\n"));
	     //add for keenhi start
	      //LOGD("SensorYuv_MT9P111_SetPowerLeve open this camera fd");
	     	#if(BUILD_FOR_HM2057== 1)
	     		pContext->camera_fd = open(HM2057_SENSOR_PATH, O_RDWR);
			#else
				pContext->camera_fd = open(GC0308_SENSOR_PATH, O_RDWR);
			#endif
            if (pContext->camera_fd < 0) 
            {
                //LOGE("Can not open camera device: %s\n", strerror(errno));
                Status = NV_FALSE;
            }         
#if(BUILD_FOR_MT9P111 == 1)
    	     ioctl(pContext->camera_fd, SENSOR_IOCTL_SET_GET_SHADING, NULL);	     
#endif
           // LOGD("Camera fd open as: %d\n", pContext->camera_fd);
	    //add for keenhi end  
            break;

        case NvOdmImagerPowerLevel_Standby:

            Status = SetPowerLevelWithPeripheralConnectivity(pConnections,
                         &pContext->I2c, &pContext->GpioConfig,
                         NvOdmImagerPowerLevel_Standby);

            break;
        case NvOdmImagerPowerLevel_Off:
	NVODMSENSOR_PRINTF(("2#########################SensorYuv_MT9P111_SetPowerLevel Level_Off\n"));
            Status = SetPowerLevelWithPeripheralConnectivity(pConnections,
                         &pContext->I2c, &pContext->GpioConfig,
                         NvOdmImagerPowerLevel_Off);
            
            // wait for the standby 0.1 sec should be enough
            NvOdmOsWaitUS(100000); 
	     //add for keenhi start
	     close(pContext->camera_fd);
            pContext->camera_fd = -1;
	    //LOGD("SensorYuv_MT9P111_SetPowerLeve close this camera fd");
	     //add for keenhi end
            break;
        default:
            NV_ASSERT("2!Unknown power level\n");
            Status = NV_TRUE;
            break;
    }
#endif
    if (Status)
        pContext->PowerLevel = PowerLevel;

    return Status;
}

/**
 * SensorYuv_SetParameter. Phase 2.
 * Set sensor specific parameters. 
 * For Phase 1. This can return NV_TRUE.
 */
static NvBool
SensorYuv_MT9P111_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue)
{
	SensorYuvContext *pContext =
			   (SensorYuvContext*)hImager->pSensor->pPrivateContext;
	//NVODMSENSOR_PRINTF(("#########################SensorYuv_MT9P111_SetParameter Param =%d\n",Param));
    switch(Param)
    {
        case NvOdmImagerParameter_SensorInputClock:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                                             sizeof(NvOdmImagerClockProfile));

            {
                // Verify the input clock is valid.
                //NvU32 InClock =
                //    ((NvOdmImagerClockProfile*)pValue)->ExternalClockKHz;
		//NVODMSENSOR_PRINTF(("#########################SensorYuv_MT9P111_SetParameter InClock=%d\n",InClock));
		#if 0
		if (InClock < g_SensorYuvCaps.InitialSensorClockRateKHz)
                    return NV_FALSE;
		#endif
            }
            break;
        case NvOdmImagerParameter_OptimizeResolutionChange:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvBool));

            //Support preview in low resolution, high frame rate
            return NV_TRUE;
            break;
       case NvOdmImagerParameter_CustomizedBlockInfo:
            NVODMSENSOR_PRINTF(("NvOdmImagerParameter_CustomizedBlockInfo\n"));

			     CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
		                    sizeof(NvOdmImagerCustomInfo));
 		NVODMSENSOR_PRINTF(("NvOdmImagerParameter_CustomizedBlockInfo========>1\n"));
            if (pValue)
            {
                NvOdmImagerCustomInfo *pCustom =
                    (NvOdmImagerCustomInfo *)pValue;
		   NVODMSENSOR_PRINTF(("NvOdmImagerParameter_CustomizedBlockInfo========>2\n"));
                if (pCustom->pCustomData && pCustom->SizeOfData)
                {
                    char settingType[15];
                    char tempCustomData[15];
		      					NvU8 ParameterValue;
		      					int ret = 0;
			NVODMSENSOR_PRINTF(("NvOdmImagerParameter_CustomizedBlockInfo========>3\n"));
                    strncpy(settingType, pCustom->pCustomData, 3);
                    settingType[3] = 0;
                    NVODMSENSOR_PRINTF(("Shan settingType: %s, camera fd = %d\n", settingType,pContext->camera_fd));
                    if (!strcmp(settingType,"W-B"))
                    {
                       if (strlen(pCustom->pCustomData) == 22) {
                           strncpy(tempCustomData, pCustom->pCustomData + 20, 2);
                       } else {
                           strcpy(tempCustomData, pCustom->pCustomData+4);
                       }
                       switch(atoi(tempCustomData))
                       {
                           case 1: // auto

				   ParameterValue = YUV_Whitebalance_Auto;
                               break;
                           case 2: // Incandescent

				    ParameterValue = YUV_Whitebalance_Incandescent;
                               break;
                           case 5: // Daylight

				   ParameterValue = YUV_Whitebalance_Daylight;
                               break;
                           case 3: // Fluorescent

				    ParameterValue = YUV_Whitebalance_Fluorescent;
                               break;
				case 6: // CloudyDaylight

				 ParameterValue = YUV_Whitebalance_CloudyDaylight;
				  break;
                           default:
                               NVODMSENSOR_PRINTF(("SensorYuv_MT9P111_SetParameter####################White balance - DEFAULT(%s)", pCustom->pCustomData));
				   ParameterValue = YUV_Whitebalance_Auto;
                               break;
                       }
			 
				if(pContext->camera_fd)
			 		ret = ioctl(pContext->camera_fd, SENSOR_IOCTL_SET_WHITE_BALANCE, &ParameterValue);   
                    }
                    else if (!strcmp(settingType,"C-E"))
                    {
                       if (strlen(pCustom->pCustomData) == 22) {
                           strncpy(tempCustomData, pCustom->pCustomData + 20, 2);
                       } else {
                           strcpy(tempCustomData, pCustom->pCustomData+4);
                       }
                       switch(atoi(tempCustomData))
                       {
                           case 3: // mono

				   ParameterValue = YUV_ColorEffect_Mono;
                               break;
                           case 7: // sepia

				   ParameterValue = YUV_ColorEffect_Sepia;		 
                               break;
                           case 4: // negative

				   ParameterValue = YUV_ColorEffect_Negative;		   
                               break;
                           case 8: // solarize

				   ParameterValue = YUV_ColorEffect_Solarize;
                               break;
                           case 6: // posterize

				   ParameterValue = YUV_ColorEffect_Posterize;			  
                               break;
			      case 5: // none

				   ParameterValue = YUV_ColorEffect_None;			  
                               break;
                           default:
                              NVODMSENSOR_PRINTF(("SensorYuv_MT9P111_SetParameter####################Color Effect - DEFAULT(%s)", pCustom->pCustomData));
				  ParameterValue = YUV_ColorEffect_None;
                               break;
                       }
			 
				if(pContext->camera_fd)
			 		ret = ioctl(pContext->camera_fd, SENSOR_IOCTL_SET_COLOR_EFFECT, &ParameterValue);   
                    }
	   	      				else if (!strcmp(settingType,"Y-E"))
                    {
                       if (strlen(pCustom->pCustomData) == 22) {
                           strncpy(tempCustomData, pCustom->pCustomData + 20, 2);
                       } else {
                           strcpy(tempCustomData, pCustom->pCustomData+4);
                       }
			  
                       switch(atoi(tempCustomData))
                       {
                           case 2: // exposure +2

				   ParameterValue = YUV_YUVExposure_Positive2;
                               break;
                           case 1: // exposure +1

				   ParameterValue = YUV_YUVExposure_Positive1;		 
                               break;
                           case 0: // exposure 0

				   ParameterValue = YUV_YUVExposure_Number0;		   
                               break;
                           case -1: // exposure -1

				   ParameterValue = YUV_YUVExposure_Negative1;
                               break;
                           case -2: // exposure -2

				   ParameterValue = YUV_YUVExposure_Negative2;			  
                               break;
                           default:
                               NVODMSENSOR_PRINTF(("SensorYuv_MT9P111_SetParameter####################exposure - DEFAULT(%s)", pCustom->pCustomData));
				   ParameterValue = YUV_YUVExposure_Number0;
                               break;
                       }
			 
				if(pContext->camera_fd)
			 		ret = ioctl(pContext->camera_fd, SENSOR_IOCTL_SET_YUV_EXPOSURE, &ParameterValue);  
                    }

                }
            }
            return NV_TRUE;
            break;


        case NvOdmImagerParameter_SelfTest:
            // not implemented.
            break;
        default:
            //NV_ASSERT(!"Not Implemented");
            break;
    }


    return NV_TRUE;
}

/**
 * SensorYuv_GetParameter. Phase 1.
 * Get sensor specific parameters
 */
static NvBool
SensorYuv_MT9P111_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue)
{

    //SensorYuvContext *pContext =
    //    (SensorYuvContext*)hImager->pSensor->pPrivateContext;
//NVODMSENSOR_PRINTF(("#########################SensorYuv_MT9P111_GetParameter Param =%d\n",Param));
    switch(Param)
    {
        case NvOdmImagerParameter_DeviceStatus:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerDeviceStatus));
            {
                NvOdmImagerDeviceStatus *pStatus;
                pStatus = (NvOdmImagerDeviceStatus *)pValue;
                {
#if(BUILD_FOR_MT9P111 == 1)
                    NvOdmImagerI2cConnection *pI2c = &pContext->I2c;
                    // Pick your own useful registers to use for debug.
                    // If the camera hangs, these register values are printed
                    // Sensor Dependent.

                    NvOdmImagerI2cWrite(pI2c, 0x098C, 0x0000);
                    NvOdmImagerI2cRead(pI2c, 0x0990, &pStatus->Values[0]);
                    NvOdmImagerI2cWrite(pI2c, 0x098C, 0x2104);
                    NvOdmImagerI2cRead(pI2c, 0x0990, &pStatus->Values[1]);
                    NvOdmImagerI2cWrite(pI2c, 0x098C, 0x2703);
                    NvOdmImagerI2cRead(pI2c, 0x0990, &pStatus->Values[2]);
                    NvOdmImagerI2cWrite(pI2c, 0x098C, 0x2705);
                    NvOdmImagerI2cRead(pI2c, 0x0990, &pStatus->Values[3]);
                    NvOdmImagerI2cWrite(pI2c, 0x098C, 0x2737);
                    NvOdmImagerI2cRead(pI2c, 0x0990, &pStatus->Values[4]);
			
                    pStatus->Count = 5;
#endif;
                }
            }
            break;
 
        case NvOdmImagerParameter_SensorFrameRate:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvF32));

            // TODO: Get the real frame rate.
            *((NvF32*)pValue) = 25.0f;
            break;
	case NvOdmImagerParameter_FocalLength:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvF32));
            ((NvF32*)pValue)[0] = LENS_FOCAL_LENGTH;
            break;
	case NvOdmImagerParameter_SensorIspSupport:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvBool));

            if (pValue)
            {

		  *(NvBool*)pValue =NV_TRUE;
            }
            else
            {
                NvOsDebugPrintf("cannot store value in NULL pointer for"\
                    "NvOdmImagerParameter_SensorIspSupport in %s:%d\n",__FILE__,__LINE__);
                //Status = NV_FALSE;
            }
            break;
        case NvOdmImagerParameter_FlashCapabilities:
            return NV_FALSE;

        case NvOdmImagerParameter_SensorFrameRateLimitsAtResolution:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerFrameRateLimitAtResolution));
            {
                NvOdmImagerFrameRateLimitAtResolution *pData =
                    (NvOdmImagerFrameRateLimitAtResolution *)pValue;

                pData->MinFrameRate = 25.0f;
                pData->MaxFrameRate = 25.0f;
            }

            break;

        case NvOdmImagerParameter_SensorExposureLimits:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvF32) * 2);

            ((NvF32*)pValue)[0] = 1.0f;
            ((NvF32*)pValue)[1] = 1.0f;

            break;

        case NvOdmImagerParameter_SensorGainLimits:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvF32) * 2);

            ((NvF32*)pValue)[0] = 1.0f;
            ((NvF32*)pValue)[1] = 1.0f;

            break;

        default:
            return NV_FALSE;
    }

    return NV_TRUE;
}

/**
 * SensorYuv_GetPowerLevel. Phase 2.
 * Get the sensor's current power level
 */
static void
SensorYuv_MT9P111_GetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel *pPowerLevel)
{
    SensorYuvContext *pContext =
        (SensorYuvContext*)hImager->pSensor->pPrivateContext;
NVODMSENSOR_PRINTF(("SensorYuv_MT9P111_GetPowerLevel::=============================>EXE\n"));
    *pPowerLevel = pContext->PowerLevel;
}

/**
 * SensorYuv_GetHal. Phase 1.
 * return the hal functions associated with sensor yuv
 */
NvBool SensorYuv_MT9P111GetHal(NvOdmImagerHandle hImager)
{
    if (!hImager || !hImager->pSensor)
        return NV_FALSE;

    hImager->pSensor->pfnOpen = SensorYuv_MT9P111_Open;
    hImager->pSensor->pfnClose = SensorYuv_MT9P111_Close;
    hImager->pSensor->pfnGetCapabilities = SensorYuv_MT9P111_GetCapabilities;
    hImager->pSensor->pfnListModes = SensorYuv_MT9P111_ListModes;
    hImager->pSensor->pfnSetMode = SensorYuv_MT9P111_SetMode;
    hImager->pSensor->pfnSetPowerLevel = SensorYuv_MT9P111_SetPowerLevel;
    hImager->pSensor->pfnGetPowerLevel = SensorYuv_MT9P111_GetPowerLevel;
    hImager->pSensor->pfnSetParameter = SensorYuv_MT9P111_SetParameter;
    hImager->pSensor->pfnGetParameter = SensorYuv_MT9P111_GetParameter;

    return NV_TRUE;
}
