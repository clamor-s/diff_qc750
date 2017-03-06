/**
 * Copyright (c) 2012 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef __OV5640_H__
#define __OV5640_H__

#include <linux/ioctl.h>  /* For IOCTL macros */

#define OV5640_IOCTL_SET_SENSOR_MODE _IOW('o', 1, struct ov5640_mode)
#define OV5640_IOCTL_GET_SENSOR_STATUS _IOR('o', 2, __u8)
#define OV5640_IOCTL_GET_CONFIG _IOR('o', 3, struct ov5640_config)
#define OV5640_IOCTL_SET_FPOSITION _IOW('o', 4, __u32)
#define OV5640_IOCTL_POWER_LEVEL _IOW('o', 5, __u32)
#define OV5640_IOCTL_GET_AF_STATUS _IOR('o', 6, __u8)
#define OV5640_IOCTL_SET_AF_MODE _IOR('o', 7, __u8)
#define OV5640_IOCTL_GET_SENSOR_MODE _IOR('o', 8, struct ov5640_mode)

#define OV5640_POWER_LEVEL_OFF 0
#define OV5640_POWER_LEVEL_ON 1
#define OV5640_POWER_LEVEL_SUS 2

struct ov5640_config {
 __u32 settle_time;
 __u32 pos_low;
 __u32 pos_high;
 float focal_length;
 float fnumber;
};

struct ov5640_mode {
 int xres;
 int yres;
};

typedef enum
{
 NvOdmImagerOv5640AfType_Infinity = 0,
 NvOdmImagerOv5640AfType_Trigger,
 NvOdmImagerOv5640AfType_Force8 = 0xff
} NvOdmImagerOv5640AfType;

#endif  /* __OV5640_H__ */
