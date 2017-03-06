/*
 * @file include/linux/dmt.h
 * @brief DMARD03 & DMARD05 & DMARD06 & DMARD07 & DMARD08 g-sensor Linux device driver
 * @author Domintech Technology Co., Ltd (http://www.domintech.com.tw)
 * @version 1.33a
 * @date 2012/6/2
 *
 * @section LICENSE
 *
 *  Copyright 2011 Domintech Technology Co., Ltd
 *
 * 	This software is licensed under the terms of the GNU General Public
 * 	License version 2, as published by the Free Software Foundation, and
 * 	may be copied, distributed, and modified under those terms.
 *
 * 	This program is distributed in the hope that it will be useful,
 * 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * 	GNU General Public License for more details.
 *
 *
 */
#ifndef DMT_H
#define DMT_H

#include <linux/ioctl.h>
#define CHIP_ENABLE    137 // panda GPIO
//#define DMT_DEBUG_DATA	1
#define DMT_DEBUG_DATA 		0

#define AUTO_CALIBRATION	0
//#define AUTO_CALIBRATION	0
/* Input Device Name  */
#define INPUT_NAME_ACC	"DMT_accel"

#if (defined(CONFIG_SENSORS_DMARD05) || defined(CONFIG_SENSORS_DMARD05_MODULE))
#define DEVICE_I2C_NAME 			"dmard05"	/* Device name for DMARD05 misc. device */
#define DEFAULT_SENSITIVITY 		64
#define X_OUT_REG 					0x41		
#define SW_RESET_REG				0x53
#define WHO_AM_I_REG				0x0f
#define CONTROL_REGISTER_2_REG 		0x45
#define WHO_AM_I_VALUE 				0x05
#define CONTROL_REGISTER_2_VALUE	0x00	// NO_FILTER
//#define CONTROL_REGISTER_2_VALUE	0x10	// HIGH_PASS_FILTER	
//#define CONTROL_REGISTER_2_VALUE	0x20	// LOW_PASS_FILTER

#elif (defined(CONFIG_SENSORS_DMARD06) || defined(CONFIG_SENSORS_DMARD06_MODULE))
#define DEVICE_I2C_NAME 			"dmard06"	/* Device name for DMARD06 misc. device */
#define DEFAULT_SENSITIVITY 		32
#define X_OUT_REG 					0x41		
#define SW_RESET_REG				0x53
#define WHO_AM_I_REG				0x0f
#define CONTROL_REGISTER_2_REG 		0x45
#define WHO_AM_I_VALUE 				0x06
//#define CONTROL_REGISTER_2_VALUE	0x00	// NO_FILTER
//#define CONTROL_REGISTER_2_VALUE	0x10	// HIGH_PASS_FILTER	
#define CONTROL_REGISTER_2_VALUE	0x20	// LOW_PASS_FILTER

#elif (defined(CONFIG_SENSORS_DMARD07) || defined(CONFIG_SENSORS_DMARD07_MODULE))
#define DEVICE_I2C_NAME 			"dmard07"	/* Device name for DMARD07 misc. device */
#define DEFAULT_SENSITIVITY 		64
#define X_OUT_REG 					0x41		
#define SW_RESET_REG				0x53
#define WHO_AM_I_REG				0x0f
#define CONTROL_REGISTER_2_REG 		0x45
#define WHO_AM_I_VALUE 				0x07
#define CONTROL_REGISTER_2_VALUE	0x00	// NO_FILTER
//#define CONTROL_REGISTER_2_VALUE	0x10	// HIGH_PASS_FILTER	
//#define CONTROL_REGISTER_2_VALUE	0x20	// LOW_PASS_FILTER

#elif (defined(CONFIG_SENSORS_DMARD03) || defined(CONFIG_SENSORS_DMARD03_MODULE))
#define DEVICE_I2C_NAME 			"dmard03"	/* Device name for DMARD03 misc. device */
#define DEFAULT_SENSITIVITY			256
#define CHIP_ID_REG					0x0a
#define CHIP_ID_VALUE				0x88
#define BANDWIDTH_REG				0x08
//#define BANDWIDTH_REG_VALUE			0x07	// enable Tms AverageOrder(1)No Filter
//#define BANDWIDTH_REG_VALUE			0x06	// enable Tms AverageOrder(2)
//#define BANDWIDTH_REG_VALUE			0x05	// enable Tms AverageOrder(4)
//#define BANDWIDTH_REG_VALUE			0x04	// enable Tms AverageOrder(8)
#define BANDWIDTH_REG_VALUE			0x03	// disable Tms AverageOrder(1)No Filter(default)
//#define BANDWIDTH_REG_VALUE			0x02	// disable Tms AverageOrder(2)
//#define BANDWIDTH_REG_VALUE			0x01	// disable Tms Tms AverageOrder(4)
//#define BANDWIDTH_REG_VALUE			0x00	// disable Tms Tms AverageOrder(8)
#define ACC_REG						0x02

#elif (defined(CONFIG_SENSORS_DMARD08) || defined(CONFIG_SENSORS_DMARD08_MODULE))
#define DEVICE_I2C_NAME 			"dmard08"	/* Device name for DMARD08 misc. device */
#define DEFAULT_SENSITIVITY			256
#define CHIP_ID_REG					0x0a
#define CHIP_ID_VALUE				0x88
#define BANDWIDTH_REG				0x08
//#define BANDWIDTH_REG_VALUE			0x07	// enable Tms AverageOrder(1)No Filter
//#define BANDWIDTH_REG_VALUE			0x06	// enable Tms AverageOrder(2)
//#define BANDWIDTH_REG_VALUE			0x05	// enable Tms AverageOrder(4)
//#define BANDWIDTH_REG_VALUE			0x04	// enable Tms AverageOrder(8)
#define BANDWIDTH_REG_VALUE			0x03	// disable Tms AverageOrder(1)No Filter(default)
//#define BANDWIDTH_REG_VALUE			0x02	// disable Tms AverageOrder(2)
//#define BANDWIDTH_REG_VALUE			0x01	// disable Tms Tms AverageOrder(4)
//#define BANDWIDTH_REG_VALUE			0x00	// disable Tms Tms AverageOrder(8)
#define ACC_REG						0x02
#endif

#if DMT_DEBUG_DATA
#define IN_FUNC_MSG printk(KERN_INFO "@DMT@ In %s\n", __func__)
#define PRINT_X_Y_Z(x, y, z) printk(KERN_INFO "@DMT@ X/Y/Z axis: %04d , %04d , %04d\n", (x), (y), (z))
#define PRINT_OFFSET(x, y, z) printk(KERN_INFO "@offset@  X/Y/Z axis: %04d , %04d , %04d\n",offset.x,offset.y,offset.z)
#define DMT_DATA(dev, ...) dev_dbg((dev), ##__VA_ARGS__)
#else
#define IN_FUNC_MSG
#define PRINT_X_Y_Z(x, y, z)
#define PRINT_OFFSET(x, y, z)
#define DMT_DATA(dev, ...)
#endif

//g-senor layout configuration, choose one of the following configuration
//#define CONFIG_GSEN_LAYOUT_PAT_1
//#define CONFIG_GSEN_LAYOUT_PAT_2
//#define CONFIG_GSEN_LAYOUT_PAT_3
//#define CONFIG_GSEN_LAYOUT_PAT_4
//#define CONFIG_GSEN_LAYOUT_PAT_5
#define CONFIG_GSEN_LAYOUT_PAT_6
//#define CONFIG_GSEN_LAYOUT_PAT_7
//#define CONFIG_GSEN_LAYOUT_PAT_8

#define CONFIG_GSEN_CALIBRATION_GRAVITY_ON_Z_NEGATIVE 1
#define CONFIG_GSEN_CALIBRATION_GRAVITY_ON_Z_POSITIVE 2
#define CONFIG_GSEN_CALIBRATION_GRAVITY_ON_Y_NEGATIVE 3
#define CONFIG_GSEN_CALIBRATION_GRAVITY_ON_Y_POSITIVE 4
#define CONFIG_GSEN_CALIBRATION_GRAVITY_ON_X_NEGATIVE 5
#define CONFIG_GSEN_CALIBRATION_GRAVITY_ON_X_POSITIVE 6

#define AVG_NUM 16

#define IOCTL_MAGIC  0x09
#define SENSOR_DATA_SIZE 3                           

#define SENSOR_RESET    		_IO(IOCTL_MAGIC, 0)
#define SENSOR_CALIBRATION   	_IOWR(IOCTL_MAGIC,  1, int[SENSOR_DATA_SIZE])
#define SENSOR_GET_OFFSET  		_IOR(IOCTL_MAGIC,  2, int[SENSOR_DATA_SIZE])
#define SENSOR_SET_OFFSET  		_IOWR(IOCTL_MAGIC,  3, int[SENSOR_DATA_SIZE])
#define SENSOR_READ_ACCEL_XYZ  	_IOR(IOCTL_MAGIC,  4, int[SENSOR_DATA_SIZE])
#define SENSOR_SETYPR  			_IOW(IOCTL_MAGIC,  5, int[SENSOR_DATA_SIZE])
#define SENSOR_GET_OPEN_STATUS	_IO(IOCTL_MAGIC,  6)
#define SENSOR_GET_CLOSE_STATUS	_IO(IOCTL_MAGIC,  7)
#define SENSOR_GET_DELAY		_IOR(IOCTL_MAGIC,  8, unsigned int*)

#define SENSOR_MAXNR 8

#endif               
