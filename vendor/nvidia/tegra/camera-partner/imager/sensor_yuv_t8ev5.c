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
#include <cutils/log.h>
#include "sensor_yuv_t8ev5.h"
#include "t8ev5.h"

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
#define LENS_FOCAL_LENGTH (3.51f)//
#define NVODMSENSOR_ENABLE_PRINTF (0)
#define LENS_HORIZONTAL_VIEW_ANGLE (54.7f)  //These parameters are set to some value in order to pass CTS
#define LENS_VERTICAL_VIEW_ANGLE   ( 42.3f)  //We'll have to replace them with appropriate values if we get these sensor specs from some source.
//diagonal l -r 64.6
//diagonal r-l  64.6
//Aperture 2.8f
//hfov = 56.9f
//wfov = 44.1f
#define FOCUSER_POSITIONS(ctx)     ((ctx)->Config.pos_high - (ctx)->Config.pos_low)
#define MAX_POS 8
#define PREVIEW_SIZE_W 960
#define PREVIEW_SIZE_H 720

#define PER_SIZE_W PREVIEW_SIZE_W/8
#define PER_SIZE_H PREVIEW_SIZE_H/8

#define FOCUS_REGION_W 180
#define FOCUS_REGION_H 180



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

static NvOdmImagerCapabilities g_SensorYuvCaps =
{
    "t8ev5",  // (Aptina 5140)string identifier, used for debugging
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
        1, // USE !CONTINUOUS_CLOCK,
        0x9
    }, // serial settings (CSI)
    { 16, 16 }, // min blank time, shouldn't need adjustment
    0, // preferred mode, which resolution to use on startup
    0, // FOCUSER_GUID, only set if focuser code available
    0, //FLASH_LTC3216_GUID, // FLASH_GUID, only set if flash device code is available
    NVODM_IMAGER_CAPABILITIES_END
};


/**
 * A list of sensor mode and its SetModeSequence. Phase 0. Sensor Dependent.
 * When SetMode is called, it searches the list for desired resolution and
 * frame rate and write the SetModeSequence through i2c.
 */
static SensorSetModeSequence g_SensorYuvSetModeSequenceList[] =
{
     // WxH        x, y    fps   set mode sequence
    {{{2592, 1944}, {0, 0},  7, 0}, NULL, NULL},
    {{{1280, 720}, {0, 0},  30, 0}, NULL, NULL},
    {{{960, 720}, {0, 0},  30, 0}, NULL, NULL},
    {{{640, 480}, {0, 0},  30, 0}, NULL, NULL},
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
    NvBool IspSupport;
    // Phase 1 variables.
    NvOdmImagerI2cConnection I2c;
    NvU32 ModeIndex;
    NvU32 NumModes;
    NvOdmImagerPowerLevel PowerLevel;
    ImagerGpioConfig GpioConfig;
	struct t8ev5_config Config;
}SensorYuvContext;

/**
 * Static functions
 * Some compilers require these to be declared upfront.
 */
static NvBool SensorYuv_T8ev5_Open(NvOdmImagerHandle hImager);
static void SensorYuv_T8ev5_Close(NvOdmImagerHandle hImager);

static void
SensorYuv_T8ev5_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities);

static void
SensorYuv_T8ev5_ListModes(
    NvOdmImagerHandle hImager,
    NvOdmImagerSensorMode *pModes,
    NvS32 *pNumberOfModes);
    
static NvBool
SensorYuv_T8ev5_SetMode(
    NvOdmImagerHandle hImager,
    const SetModeParameters *pParameters,
    NvOdmImagerSensorMode *pSelectedMode,
    SetModeParameters *pResult);

static NvBool
SensorYuv_T8ev5_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel);

static NvBool
SensorYuv_T8ev5_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue);

static NvBool
SensorYuv_T8ev5_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue);

static void
SensorYuv_T8ev5_GetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel *pPowerLevel);


/**
 * SensorYuv_Open. Phase 1.
 * Initialize sensor yuv and its private context
 */
static NvBool SensorYuv_T8ev5_Open(NvOdmImagerHandle hImager)
{
     NVODMSENSOR_PRINTF(("#########################SensorYuv_T8ev5_Open\n"));
    SensorYuvContext *pSensorYuvContext = NULL;

    if (!hImager || !hImager->pSensor)
        return NV_FALSE;

    pSensorYuvContext =
        (SensorYuvContext*)NvOdmOsAlloc(sizeof(SensorYuvContext));
    if (!pSensorYuvContext)
        goto fail;

    NvOdmOsMemset(pSensorYuvContext, 0, sizeof(SensorYuvContext));

    pSensorYuvContext->NumModes =
        NV_ARRAY_SIZE(g_SensorYuvSetModeSequenceList);
    pSensorYuvContext->ModeIndex =
        pSensorYuvContext->NumModes + 1; // invalid mode

    pSensorYuvContext->IspSupport = NV_TRUE;

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
static void SensorYuv_T8ev5_Close(NvOdmImagerHandle hImager)
{
    SensorYuvContext *pContext = NULL;
            NVODMSENSOR_PRINTF(("#########################SensorYuv_T8ev5_Close \n"));
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
SensorYuv_T8ev5_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities)
{
    // Copy the sensor caps from g_SensorYuvCaps
    NVODMSENSOR_PRINTF(("#########################SensorYuv_T8ev5_GetCapabilities \n"));
    NvOdmOsMemcpy(pCapabilities, &g_SensorYuvCaps,
        sizeof(NvOdmImagerCapabilities));
}

/**
 * SensorYuv_ListModes. Phase 1.
 * Return a list of modes that sensor yuv supports.
 * If called with a NULL ptr to pModes, then it just returns the count
 */
static void
SensorYuv_T8ev5_ListModes(
    NvOdmImagerHandle hImager,
    NvOdmImagerSensorMode *pModes,
    NvS32 *pNumberOfModes)
{
    SensorYuvContext *pContext =
        (SensorYuvContext*)hImager->pSensor->pPrivateContext;
    NvS32 i;
NVODMSENSOR_PRINTF(("#########################SensorYuv_T8EV5_ListModes \n"));
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
SensorYuv_T8ev5_SetMode(
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

    NvOsDebugPrintf("Setting resolution to ===============>%dx%d\n",
        pParameters->Resolution.width, pParameters->Resolution.height);

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
   if(pContext->camera_fd>0)
    	Status = ioctl(pContext->camera_fd, SENSOR_IOCTL_SET_MODE, &mode);
    else{
		Status = -1;
    	}
    if (Status < 0) {
        return NV_FALSE;
    } 
	//add for keenhi end

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
SensorYuv_T8ev5_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel)
{
    NvBool Status = NV_TRUE;

    SensorYuvContext *pContext =
        (SensorYuvContext*)hImager->pSensor->pPrivateContext;
    //const NvOdmPeripheralConnectivity *pConnections;

    if (pContext->PowerLevel == PowerLevel)
        return NV_TRUE;
   switch(PowerLevel)
    {
        case NvOdmImagerPowerLevel_On:
		NvOsDebugPrintf("1#########################SensorYuv_T8ev5_SetPowerLevel Level_On\n");
	     //add for keenhi start
#ifdef O_CLOEXEC
            pContext->camera_fd = open(SENSOR_PATH, O_RDWR|O_CLOEXEC);
#else
            pContext->camera_fd = open(SENSOR_PATH, O_RDWR);
#endif
            if (pContext->camera_fd < 0) {
                NvOsDebugPrintf("Can not open camera device: %s\n",
                strerror(errno));
                Status = NV_FALSE;
            } else {
                NvOsDebugPrintf("hm2057 Camera fd open as: %d\n", pContext->camera_fd);
                Status = NV_TRUE;
            }
            if (!Status)
            {
                NvOsDebugPrintf("NvOdmImagerPowerLevel_On failed.\n");
                return NV_FALSE;
            }
	    //add for keenhi end  
            break;

        case NvOdmImagerPowerLevel_Standby:
		Status = NV_TRUE;
            break;
        case NvOdmImagerPowerLevel_Off:
	NvOsDebugPrintf("1#########################SensorYuv_T8ev5_SetPowerLevel Level_Off\n");

	     close(pContext->camera_fd);
            pContext->camera_fd = -1;
            //NvOdmOsWaitUS(100000);
            break;
        default:
            NV_ASSERT("!Unknown power level\n");
            Status = NV_TRUE;
            break;
    }

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
SensorYuv_T8ev5_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue)
{
	NvBool Status = NV_TRUE;
	SensorYuvContext *pContext =
			   (SensorYuvContext*)hImager->pSensor->pPrivateContext;
	//NVODMSENSOR_PRINTF(("#########################SensorYuv_T8ev5_SetParameter Param =%d\n",Param));
    switch(Param)
    {
        case NvOdmImagerParameter_SensorInputClock:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                                             sizeof(NvOdmImagerClockProfile));

            {
                // Verify the input clock is valid.
                NvU32 InClock =
                    ((NvOdmImagerClockProfile*)pValue)->ExternalClockKHz;
		NVODMSENSOR_PRINTF(("#########################SensorYuv_T8ev5_SetParameter InClock=%d\n",InClock));

		if (InClock < g_SensorYuvCaps.InitialSensorClockRateKHz)
                    Status = NV_FALSE;
            }
            break;
        case NvOdmImagerParameter_OptimizeResolutionChange:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvBool));

            // Sensor yuv doesn't support optimized resolution change.
            if (*((NvBool*)pValue) == NV_TRUE)
            {
                Status = NV_FALSE;
            }
            break;
	   case NvOdmImagerParameter_ISPSetting:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                    sizeof(NvOdmImagerISPSetting));
            NvOdmImagerISPSetting *pSetting = (NvOdmImagerISPSetting *)pValue;
            if (pSetting)
            {
                switch(pSetting->attribute)
                {
                  case NvCameraIspAttribute_AutoFocusTrigger:
                  {
                      NvOdmImagerISPSetting *pSetting = (NvOdmImagerISPSetting *)pValue;
                      NvOdmImagerT8ev5AfType AFType;
                      int ret = 0;
                      if (*((NvOdmImagerISPSettingDataPointer *)pSetting->pData))
                          AFType= NvOdmImagerT8ev5AfType_Trigger;
                      else
                          AFType= NvOdmImagerT8ev5AfType_Infinity;
                      // Send the I2C focus command to the Sensor
                      NvOsDebugPrintf("%s: set current af type! %d\n", __func__, AFType);
			if(pContext->camera_fd>0)
                      ret = ioctl(pContext->camera_fd, SENSOR_IOCTL_SET_AF_MODE, (NvU8)AFType);

                      if (ret < 0) {
                          NvOsDebugPrintf("%s: ioctl to set AF failed %s\n",
                                  __func__, strerror(errno));
			     Status = NV_FALSE;
                          break;
                      }
			
			  Status = NV_TRUE;
			  break;
                  }
		    case NvCameraIspAttribute_AutoFocusRegions:
                  {
			 struct t8ev5_win_pos fRegions;

			 int rets = 0;

			 #define WINDOW_OMX_USER(_f) ((int)round((_f) * (1000)))
                      NvOdmImagerISPSetting *pSetting = (NvOdmImagerISPSetting *)pValue;
			 NvCameraIspFocusingRegions *pRegions = (NvCameraIspFocusingRegions *)pSetting->pData;	 
			if(!pRegions)return NV_FALSE;
			 fRegions.left = WINDOW_OMX_USER(pRegions->regions[0].left);
			 fRegions.top= WINDOW_OMX_USER(pRegions->regions[0].top);
			 fRegions.right = WINDOW_OMX_USER(pRegions->regions[0].right);
			 fRegions.bottom= WINDOW_OMX_USER(pRegions->regions[0].bottom);

			 NvOsDebugPrintf("autoFocusRegion[%d].[%d,%d,%d,%d],numOfRegions=%ld",0,fRegions.left,fRegions.top,
			 	fRegions.right,fRegions.bottom,pRegions->numOfRegions);
			 #undef WINDOW_OMX_USER
			 //if(fRegions.left<0||fRegions.top<0||fRegions.right<=0||fRegions.bottom<=0) return NV_TRUE;
			 if((fRegions.right -fRegions.left)==0||(fRegions.bottom-fRegions.top)==0) return NV_TRUE;

			rets = ioctl(pContext->camera_fd, T8EV5_IOCTL_SET_WINDOWS_POS, &fRegions);
			if (rets < 0) {
                     		 NvOsDebugPrintf("%s: ioctl to set window pos AF failed %s\n",
                              __func__, strerror(errno));
				Status = NV_FALSE;
                     	break;
                  	}
			 
                      Status = NV_TRUE;
                      break;
		    }
                  default:break;
                  }
            }
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
                    NVODMSENSOR_PRINTF(("Shan settingType: %s\n", settingType));
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
                               NVODMSENSOR_PRINTF(("SensorYuv_T8ev5_SetParameter####################White balance - DEFAULT(%s)", pCustom->pCustomData));
				   ParameterValue = YUV_Whitebalance_Auto;
                               break;
                       }
			 
				if(pContext->camera_fd>0){
			 		ret = ioctl(pContext->camera_fd, SENSOR_IOCTL_SET_WHITE_BALANCE, &ParameterValue);   
					NVODMSENSOR_PRINTF(("SensorYuv_T8ev5_SetParameter####################White balance [%d]",ParameterValue));
				}
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
                              NVODMSENSOR_PRINTF(("SensorYuv_T8ev5_SetParameter####################Color Effect - DEFAULT(%s)", pCustom->pCustomData));
				  ParameterValue = YUV_ColorEffect_None;
                               break;
                       }
			 
				if(pContext->camera_fd>0)
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
                               NVODMSENSOR_PRINTF(("SensorYuv_T8ev5_SetParameter####################exposure - DEFAULT(%s)", pCustom->pCustomData));
				   ParameterValue = YUV_YUVExposure_Number0;
                               break;
                       }
			 
				if(pContext->camera_fd>0)
			 		ret = ioctl(pContext->camera_fd, SENSOR_IOCTL_SET_YUV_EXPOSURE, &ParameterValue);  
                    }

			else if (!strcmp(settingType,"C-F"))
                    {
                       if (strlen(pCustom->pCustomData) == 22) {
                           strncpy(tempCustomData, pCustom->pCustomData + 20, 2);
                       } else {
                           strcpy(tempCustomData, pCustom->pCustomData+4);
                       }
			NvU8 val= 0;
                       switch(atoi(tempCustomData))
                       {
                           case 1: // mono
                               NVODMSENSOR_PRINTF(("SensorYuv_T8ev5_SetParameter####################continus pic enable\n"));
				   val = 1;
                               break;
                           case 0: // sepia
                               NVODMSENSOR_PRINTF(("SensorYuv_T8ev5_SetParameter####################continus pic disable\n"));
				   val = 0;		 
                               break;
                           default:
                              NVODMSENSOR_PRINTF(("SensorYuv_T8ev5_SetParameter####################continus pic- DEFAULT(%s)", pCustom->pCustomData));
				  val = 0;
                               break;
                       }
			 
				//if(pContext->camera_fd>0)
			 		//ret = ioctl(pContext->camera_fd, T8EV5_IOCTL_SET_CONTINUOUS_MODE, &val);   
                    }else if (!strcmp(settingType,"A-F"))
                    {
                    NVODMSENSOR_PRINTF(("SensorYuv_T8ev5_SetParameter####################pCustomData=%s\n",pCustom->pCustomData));
                       if (strlen(pCustom->pCustomData) == 22) {
                           strncpy(tempCustomData, pCustom->pCustomData + 20, 2);
                       } else {
                           strcpy(tempCustomData, pCustom->pCustomData+4);
                       }
			  NvOdmImagerT8ev5AfType AFType;
			  int omxFocus = atoi(tempCustomData);
			  strcpy(tempCustomData, pCustom->pCustomData+6);
             		  int focusPos = atoi(tempCustomData);
			 NVODMSENSOR_PRINTF(("SensorYuv_T8ev5_SetParameter###############omxFocus=%d,focusPos=%d\n",omxFocus,focusPos));
			 if(omxFocus==2&&focusPos==0){//auto
			 	 AFType= NvOdmImagerT8ev5AfType_Trigger;
			 }else if(omxFocus==0&&focusPos==0){//Infinity
				AFType= NvOdmImagerT8ev5AfType_Infinity;
			 }else if(omxFocus==0&&focusPos==-103){//Macro
				AFType= NvOdmImagerT8ev5AfType_MACRO;
			 }
			 //if(pContext->camera_fd>0)
			 		//ret = ioctl(pContext->camera_fd, SENSOR_IOCTL_SET_AF_MODE, (NvU8)AFType); 
                    }
                }
            }
            Status =  NV_TRUE;
            break;
        default:
            NV_DEBUG_PRINTF(("YUV_SetParameter(): %d not supported\n", Param));
            break;
    }


    return Status;
}

/**
 * SensorYuv_GetParameter. Phase 1.
 * Get sensor specific parameters
 */
static NvBool
SensorYuv_T8ev5_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue)
{
    NvBool Status = NV_TRUE;
    SensorYuvContext *pContext =
        (SensorYuvContext*)hImager->pSensor->pPrivateContext;
NVODMSENSOR_PRINTF(("#########################SensorYuv_T8ev5_GetParameter Param =%d\n",Param));
    switch(Param)
    {
        case NvOdmImagerParameter_DeviceStatus:
			CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerDeviceStatus));
            {
                NvOdmImagerDeviceStatus *pStatus;
                pStatus = (NvOdmImagerDeviceStatus *)pValue;
                {
		       //uint8_t status;
                    //int ret;
		     pStatus->Values[0] = 0x0;
		     #if 0
                    ret = ioctl(pContext->camera_fd, SENSOR_IOCTL_GET_STATUS, &status);
                    if (ret < 0)
                    {
                        NvOsDebugPrintf("ioctl to gets status failed %s\n", strerror(errno));
                        Status = NV_FALSE;
                    }
		    #endif
                    pStatus->Count = 1;
                }
            }
            break;
 		case NvOdmImagerParameter_SensorIspSupport:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvBool));

            if (pValue)
            {
                *(NvBool*)pValue = pContext->IspSupport;
            }
            else
            {
                NvOsDebugPrintf("cannot store value in NULL pointer for"\
                    "NvOdmImagerParameter_SensorIspSupport in %s:%d\n",__FILE__,__LINE__);
                Status = NV_FALSE;
            }
            break;
        case NvOdmImagerParameter_SensorFrameRate:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvF32));

            // TODO: Get the real frame rate.
            *((NvF32*)pValue) = 30.0f;
            break;
		case NvOdmImagerParameter_FocalLength:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvF32));
            ((NvF32*)pValue)[0] = LENS_FOCAL_LENGTH;
            break;
		case NvOdmImagerParameter_HorizontalViewAngle:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvF32));
            *((NvF32*)pValue) = LENS_HORIZONTAL_VIEW_ANGLE;
            break;

        case NvOdmImagerParameter_VerticalViewAngle:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvF32));
            *((NvF32*)pValue) = LENS_VERTICAL_VIEW_ANGLE;
            break;
	#if 0
	case NvOdmImagerParameter_GetBestSensorMode:
            NV_DEBUG_PRINTF(("NvOdmImagerParameter_GetBestSensorMode\n"));
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerSensorMode));
            {
                NvU32 modeIndex = NvOdmImagerGetBestSensorMode((const NvOdmImagerSensorMode*)pValue,
                                                                g_SensorYuvSetModeSequenceList,
                                                                pContext->NumModes);
		  NvOsDebugPrintf("NvOdmImagerParameter_GetBestSensorMode modeIndex=%d\n",modeIndex);
                NvOdmOsMemcpy(pValue,
                              &g_SensorYuvSetModeSequenceList[modeIndex].Mode,
                              sizeof(NvOdmImagerSensorMode));
            }
            break;
	#endif
        case NvOdmImagerParameter_FlashCapabilities:
            Status = NV_FALSE;
	     break;

        case NvOdmImagerParameter_SensorFrameRateLimitsAtResolution:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerFrameRateLimitAtResolution));
            {
                NvOdmImagerFrameRateLimitAtResolution *pData =
                    (NvOdmImagerFrameRateLimitAtResolution *)pValue;

                pData->MinFrameRate = 30.0f;
                pData->MaxFrameRate = 30.0f;
            }
            break;

	case NvOdmImagerParameter_ISPSetting:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                    sizeof(NvOdmImagerISPSetting));
            NvOdmImagerISPSetting *pSetting = (NvOdmImagerISPSetting *)pValue;
            Status = NV_FALSE;
            if (pSetting)
            {
                switch(pSetting->attribute)
                {
                    case NvCameraIspAttribute_FocusPositionRange:
                        {
                            NvCameraIspFocusingParameters *pParams = (NvCameraIspFocusingParameters *)pSetting->pData;
                            NVODMSENSOR_PRINTF(("NvCameraIspAttribute_FocusPositionRange"));
                            pParams->sensorispAFsupport = NV_TRUE;
                        }
                        Status = NV_TRUE;
                        break;

                    case NvCameraIspAttribute_AutoFocusStatus:
                        {
                            NvU32 *pU32 = (NvU32 *)pSetting->pData;
                            NvU8 reg = 0;
				int err=0;
                            NVODMSENSOR_PRINTF(("NvCameraIspAttribute_AutoFocusStatus\n"));
				if(pContext->camera_fd>0)
                            	err = ioctl(pContext->camera_fd, SENSOR_IOCTL_GET_AF_STATUS, &reg);
                            if (err < 0)
                            {
                                NvOsDebugPrintf("%s: ioctl get AF status failed!==============> %x\n", __func__, err);
                                Status = NV_FALSE;
                            }
                            else
                            {
                                //Check AF is ready or not
                                reg = (reg >> 4);
				    // 0 : sensor ready
				    // 4 : one shot - correct
				    // 5 : one shot - incorrect
				    // >=8: busy (focusing)
                              //  0x40  0x50   0x00
                              NVODMSENSOR_PRINTF(("%s: get current af status! %d\n", __func__, reg));
                                if(reg==4){
                                    *pU32 = NvCameraIspFocusStatus_Locked;
				    }else if(reg==5)
					*pU32 = NvCameraIspFocusStatus_FailedToFind;
                                else
                                    *pU32 = NvCameraIspFocusStatus_Busy;
                                Status = NV_TRUE;
                            }
                        }
                        break;

                    default:
                        break;
                 }
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
           Status = NV_FALSE;
           break;
    }

    return Status;
}

/**
 * SensorYuv_GetPowerLevel. Phase 2.
 * Get the sensor's current power level
 */
static void
SensorYuv_T8ev5_GetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel *pPowerLevel)
{
    SensorYuvContext *pContext =
        (SensorYuvContext*)hImager->pSensor->pPrivateContext;
    *pPowerLevel = pContext->PowerLevel;
}

/**
 * SensorYuv_GetHal. Phase 1.
 * return the hal functions associated with sensor yuv
 */
NvBool SensorYuv_T8EV5_GetHal(NvOdmImagerHandle hImager)
{
    if (!hImager || !hImager->pSensor)
        return NV_FALSE;

    hImager->pSensor->pfnOpen = SensorYuv_T8ev5_Open;
    hImager->pSensor->pfnClose = SensorYuv_T8ev5_Close;
    hImager->pSensor->pfnGetCapabilities = SensorYuv_T8ev5_GetCapabilities;
    hImager->pSensor->pfnListModes = SensorYuv_T8ev5_ListModes;
    hImager->pSensor->pfnSetMode = SensorYuv_T8ev5_SetMode;
    hImager->pSensor->pfnSetPowerLevel = SensorYuv_T8ev5_SetPowerLevel;
    hImager->pSensor->pfnGetPowerLevel = SensorYuv_T8ev5_GetPowerLevel;
    hImager->pSensor->pfnSetParameter = SensorYuv_T8ev5_SetParameter;
    hImager->pSensor->pfnGetParameter = SensorYuv_T8ev5_GetParameter;

    return NV_TRUE;
}
