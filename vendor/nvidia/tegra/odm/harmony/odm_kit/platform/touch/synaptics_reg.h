/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef SYNAPTICS_REG_HEADER
#define SYNAPTICS_REG_HEADER

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * RMI4 now has different functions divided into different register groups -
 * data, control, command and query. Register mapping has been divided into
 * pages of 256 registers each. Each register address is of 16 bits, out of
 * which the top 8-bits are formed from the page address and the lower 8-bits
 * form the actual register address on that page. We use Page 00 for accessing
 * the 2D Touchpad registers for this Synaptics device. Page 00 contains more
 * than one functions but we are only using the Touchpad and the device config
 * functions.
 */

// RMI data registers
#define RMI_DEVICE_STATUS                           0x0013 // F01_RMI_DATA00
#define RMI_INTERRUPT_STATUS                        0x0014 // F01_RMI_DATA01.00

// 2D Sensor data registers
#define TP_FINGER_STATE                             0x0015 // F11_2D_DATA00.00
#define TP_X_POS_11_4_FINGER_0                      0x0016 // F11_2D_DATA01.00
#define TP_Y_POS_11_4_FINGER_0                      0x0017 // F11_2D_DATA02.00
#define TP_Y_X_POS_3_0_FINGER_0                     0x0018 // F11_2D_DATA03.00
#define TP_WY_WX_FINGER_0                           0x0019 // F11_2D_DATA04.00
#define TP_Z_FINGER_0                               0x001A // F11_2D_DATA05.00
#define TP_X_POS_11_4_FINGER_1                      0x001B // F11_2D_DATA01.01
#define TP_Y_POS_11_4_FINGER_1                      0x001C // F11_2D_DATA02.02
#define TP_Y_X_POS_3_0_FINGER_1                     0x001D // F11_2D_DATA03.03
#define TP_WY_WX_FINGER_1                           0x001E // F11_2D_DATA04.04
#define TP_Z_FINGER_1                               0x001F // F11_2D_DATA05.05
#define TP_GESTURE_FLAGS_0                          0x0020 // F11_2D_DATA08
#define TP_GESTURE_FLAGS_1                          0x0021 // F11_2D_DATA09
#define TP_PINCH_MOTION_X_FLICK_DISTANCE            0x0022 // F11_2D_DATA10
#define TP_ROTATION_MOTION_Y_FLICK_DISTANCE         0x0023 // F11_2D_DATA11
#define TP_FINGER_SEPARATION_FLICK_TIME             0x0024 // F11_2D_DATA12
#define TP_LEFT_RIGHT_CLICK                         0x0025 // F30_LED_DATA00

// RMI control registers
#define RMI_DEVICE_CONTROL                          0x0026 // F01_RMI_CTRL00
#define RMI_INTERRUPT_ENABLE_0                      0x0027 // F01_RMI_CTRL01.00

// 2d control registers
#define TP_REPORT_MODE                              0x0028 // F11_2D_CTRL00
#define TP_PALM_REJECT                              0x0029 // F11_2D_CTRL01
#define TP_DELTA_X_THRESHOLD                        0x002A // F11_2D_CTRL02
#define TP_DELTA_Y_THRESHOLD                        0x002B // F11_2D_CTRL03
#define TP_VELOCITY                                 0x002C // F11_2D_CTRL04
#define TP_ACCELERATION                             0x002D // F11_2D_CTRL05
#define TP_MAX_X_POSITION_7_0                       0x002E // F11_2D_CTRL06
#define TP_MAX_X_POSITION_11_8                      0x002F // F11_2D_CTRL07
#define TP_MAX_Y_POSITION_7_0                       0x0030 // F11_2D_CTRL08
#define TP_MAX_Y_POSITION_11_8                      0x0031 // F11_2D_CTRL09
#define TP_GESTURE_ENABLES_1                        0x0032 // F11_2D_CTRL10
#define TP_SENSOR_MAP_0                             0x0033 // F11_2D_CTRL12.00
#define TP_SENSOR_MAP_1                             0x0034 // F11_2D_CTRL12.01
#define TP_SENSOR_MAP_2                             0x0035 // F11_2D_CTRL12.02
#define TP_SENSOR_MAP_3                             0x0036 // F11_2D_CTRL12.03
#define TP_SENSOR_MAP_4                             0x0037 // F11_2D_CTRL12.04
#define TP_SENSOR_MAP_5                             0x0038 // F11_2D_CTRL12.05
#define TP_SENSOR_MAP_6                             0x0039 // F11_2D_CTRL12.06
#define TP_SENSOR_MAP_7                             0x003A // F11_2D_CTRL12.07
#define TP_SENSOR_MAP_8                             0x003B // F11_2D_CTRL12.08
#define TP_SENSOR_MAP_9                             0x003C // F11_2D_CTRL12.09
#define TP_SENSOR_MAP_10                            0x003D // F11_2D_CTRL12.10
#define TP_SENSOR_MAP_11                            0x003E // F11_2D_CTRL12.11
#define TP_SENSOR_MAP_12                            0x003F // F11_2D_CTRL12.12
#define TP_SENSOR_MAP_13                            0x0040 // F11_2D_CTRL12.13
#define TP_SENSOR_MAP_14                            0x0041 // F11_2D_CTRL12.14
#define TP_SENSOR_MAP_15                            0x0042 // F11_2D_CTRL12.15
#define TP_SENSOR_MAP_16                            0x0043 // F11_2D_CTRL12.16
#define TP_SENSOR_MAP_17                            0x0044 // F11_2D_CTRL12.17
#define TP_SENSOR_MAP_18                            0x0045 // F11_2D_CTRL12.18
#define TP_SENSOR_MAP_19                            0x0046 // F11_2D_CTRL12.19
#define TP_SENSOR_MAP_20                            0x0047 // F11_2D_CTRL12.20
#define TP_SENSOR_MAP_21                            0x0048 // F11_2D_CTRL12.21
#define TP_SENSOR_MAP_22                            0x0049 // F11_2D_CTRL12.22
#define TP_MAX_TAP_TIME                             0x0061 // F11_2D_CTRL15
#define TP_MIN_PRESS_TIME                           0x0062 // F11_2D_CTRL16
#define TP_MAX_TAP_DISTANCE                         0x0063 // F11_2D_CTRL17
#define TP_MIN_FLICK_DISTANCE                       0x0064 // F11_2D_CTRL18
#define TP_MIN_FLICK_SPEED                          0x0065 // F11_2D_CTRL19

// RMI command register
#define RMI_DEVICE_COMMAND                          0x0069 // F01_RMI_CMD00

// 2D command register
#define TP_COMMAND                                  0x006A // F11_2D_CMD00

// RMI query registers
#define RMI_MANF_ID_QUERY                           0x0074 // F01_RMI_QUERY00
#define RMI_PRODUCT_PROPS_QUERY                     0x0075 // F01_RMI_QUERY01
#define RMI_CUSTOMER_FAMILY_QUERY                   0x0076 // F01_RMI_QUERY02
#define RMI_FIRMWARE_REVISION_QUERY                 0x0077 // F01_RMI_QUERY03
#define RMI_DEVICE_SERIALIZATION_QUERY_0            0x0078 // F01_RMI_QUERY04
#define RMI_DEVICE_SERIALIZATION_QUERY_1            0x0079 // F01_RMI_QUERY05
#define RMI_DEVICE_SERIALIZATION_QUERY_2            0x007A // F01_RMI_QUERY06
#define RMI_DEVICE_SERIALIZATION_QUERY_3            0x007B // F01_RMI_QUERY07
#define RMI_DEVICE_SERIALIZATION_QUERY_4            0x007C // F01_RMI_QUERY08
#define RMI_DEVICE_SERIALIZATION_QUERY_5            0x007D // F01_RMI_QUERY09
#define RMI_DEVICE_SERIALIZATION_QUERY_6            0x007E // F01_RMI_QUERY10
#define RMI_PRODUCT_ID_QUERY_0                      0x007F // F01_RMI_QUERY11
#define RMI_PRODUCT_ID_QUERY_1                      0x0080 // F01_RMI_QUERY12
#define RMI_PRODUCT_ID_QUERY_2                      0x0081 // F01_RMI_QUERY13
#define RMI_PRODUCT_ID_QUERY_3                      0x0082 // F01_RMI_QUERY14
#define RMI_PRODUCT_ID_QUERY_4                      0x0083 // F01_RMI_QUERY15
#define RMI_PRODUCT_ID_QUERY_5                      0x0084 // F01_RMI_QUERY16
#define RMI_PRODUCT_ID_QUERY_6                      0x0085 // F01_RMI_QUERY17
#define RMI_PRODUCT_ID_QUERY_7                      0x0086 // F01_RMI_QUERY18
#define RMI_PRODUCT_ID_QUERY_8                      0x0087 // F01_RMI_QUERY19
#define RMI_PRODUCT_ID_QUERY_9                      0x0088 // F01_RMI_QUERY20

// 2D query registers
#define TP_PER_DEVICE_QUERY                         0x0089 // F11_2D_QUERY00
#define TP_REPORTING_MODE_QUERY                     0x008A // F11_2D_QUERY01
#define TP_NUMBER_OF_X_ELECTRODES_QUERY             0x008B // F11_2D_QUERY02
#define TP_NUMBER_OF_Y_ELECTRODES_QUERY             0x008C // F11_2D_QUERY03
#define TP_MAXIMUM_ELECTRODES_QUERY                 0x008D // F11_2D_QUERY04
#define TP_ABSOLUTE_QUERY                           0x008E // F11_2D_QUERY05
#define TP_GESTURE_QUERY                            0x008F // F11_2D_QUERY07
#define TP_QUERY_8                                  0x0090 // F11_2D_QUERY08

// GPIO query registers
#define TP_LR_GPIO_QUERY                            0x0091 // F30_LED_QUERY00
#define TP_GPIO_COUNT                               0x0092 // F30_LED_QUERY01

/* Register bit masks */

// Device Status register bit masks
#define DEVICE_STATUS_STATUS_CODE_BIT_MASK          0x000F
#define DEVICE_STATUS_FLASH_PROG_BIT_MASK           1 << 6
#define DEVICE_STATUS_UNCONFIGURED_BIT_MASK         1 << 7

// Interrupt Status register bit masks
#define INT_STATUS_FLASH_BIT_MASK                   1 << 0
#define INT_STATUS_STATUS_BIT_MASK                  1 << 1
#define INT_STATUS_ABS0_BIT_MASK                    1 << 2
#define INT_STATUS_GPIO_BIT_MASK                    1 << 3

// X-Y position register bit masks
#define TP_X_POSITION_BIT_MASK                      0x0F
#define TP_Y_POSITION_BIT_MASK                      0xF0

// X-Y width register bit masks
#define TP_X_WIDTH_BIT_MASK                         0x0F
#define TP_Y_WIDTH_BIT_MASK                         0xF0

// Gesture flags register 0 bit masks
#define TP_GESTURE_0_SINGLE_TAP_BIT_MASK            1 << 0
#define TP_GESTURE_0_TAP_N_HOLD_BIT_MASK            1 << 1
#define TP_GESTURE_0_DOUBLE_TAP_BIT_MASK            1 << 2
#define TP_GESTURE_0_EARLY_TAP_BIT_MASK             1 << 3
#define TP_GESTURE_0_FLICK_BIT_MASK                 1 << 4
#define TP_GESTURE_0_PRESS_BIT_MASK                 1 << 5
#define TP_GESTURE_0_PINCH_BIT_MASK                 1 << 6

// Gesture flags register 1 bit masks
#define TP_GESTURE_1_PALM_REJECT_BIT_MASK           1 << 0
#define TP_GESTURE_1_ROTATE_BIT_MASK                1 << 1
#define TP_GESTURE_1_FINGER_COUNT_BIT_MASK          7 << 5

// RMI Device Control register bit masks
#define DEVICE_CTRL_SLEEP_MODE_BIT_MASK             0x3
#define DEVICE_CTRL_NO_SLEEP_BIT_MASK               1 << 2
#define DEVICE_CTRL_REPORT_RATE_BIT_MASK            1 << 6
#define DEVICE_CTRL_CONFIGURED_BIT_MASK             1 << 7

// sleep modes
#define SLEEP_MODE_NORMAL                           0x0
#define SLEEP_MODE_SENSOR_SLEEP                     0x1

// Interrupt Enable 0 register bit masks
#define INT_ENABLE_0_FLASH_EN_BIT_MASK              1 << 0
#define INT_ENABLE_0_STATUS_EN_BIT_MASK             1 << 1
#define INT_ENABLE_0_ABS0_EN_BIT_MASK               1 << 2
#define INT_ENABLE_0_GPIO_EN_BIT_MASK               1 << 3

// 2D Report mode register bit masks
#define TP_REPORT_MODE_REPORTING_MODE_BIT_MASK      0x7
#define TP_REPORT_MODE_ABS_POS_FILTER_BIT_MASK      1 << 3
#define TP_REPORT_MODE_REL_POS_FILTER_BIT_MASK      1 << 4
#define TP_REPORT_MODE_RELATIVE_BALLISTIC_BIT_MASK  1 << 4

// 2D Palm Reject register bit mask
#define TP_PALM_REJECT_THRESHOLD_BIT_MASK           0xF

// 2D Max X Position (11:8) register bit mask
#define TP_MAX_X_POS_11_8_BIT_MASK                  0xF

// 2D Max Y Position (11:8) register bit mask
#define TP_MAX_Y_POS_11_8_BIT_MASK                  0xF

// 2D Gesture Enables 1 register bit mask
#define TP_GESTURE_INT_ENABLE_SINGLE_TAP_BIT_MASK   1 << 0
#define TP_GESTURE_INT_ENABLE_TAP_N_HOLD_BIT_MASK   1 << 1
#define TP_GESTURE_INT_ENABLE_DOUBLE_TAP_BIT_MASK   1 << 2
#define TP_GESTURE_INT_ENABLE_EARLY_TAP_BIT_MASK    1 << 3
#define TP_GESTURE_INT_ENABLE_FLICK_BIT_MASK        1 << 4
#define TP_GESTURE_INT_ENABLE_PRESS_BIT_MASK        1 << 5
#define TP_GESTURE_INT_ENABLE_PINCH_BIT_MASK        1 << 6

// 2D Sensor Map register bit mask
#define TP_SENSOR_MAP_MAP_BIT_MASK                  0x1F
#define TP_SENSOR_MAP_X_Y_SEL_BIT_MASK              1 << 7

// RMI Device Command register bit mask
#define RMI_DEVICE_COMMAND_RESET_BIT_MASK           0x1

// 2D Command register bit mask
#define TP_COMAMND_REZERO_BIT_MASK                  0x1

// RMI Product Properties Query register bit mask
#define RMI_PRODUCT_PROPS_CUSTOM_MAP_BIT_MASK       0x1
#define RMI_PRODUCT_PROPS_NON_COMPLIANT_BIT_MASK    0x2

// RMI Device Serialization query register bit masks
#define DEVICE_SER_QUERY_YEAR_BIT_MASK              0x1F
#define DEVICE_SER_QUERY_MONTH_BIT_MASK             0x0F
#define DEVICE_SER_QUERY_DAY_BIT_MASK               0x1F
#define DEVICE_SER_QUERY_DAY_BIT_MASK               0x1F
#define DEVICE_SER_QUERY_DAY_BIT_MASK               0x1F
#define DEVICE_SER_QUERY_TESTER_ID_HI_BIT_MASK      0x7F
#define DEVICE_SER_QUERY_TESTER_ID_LOW_BIT_MASK     0x7F
#define DEVICE_SER_QUERY_SERIAL_NO_HI_BIT_MASK      0x7F
#define DEVICE_SER_QUERY_SERIAL_NO_LOW_BIT_MASK     0x7F

// RMI Product ID query register bit masks
#define PROD_ID_QUERY_CHARACTER_BIT_MASK            0x7F

// 2D per device query register bit mask
#define TP_PER_DEVICE_QUERY_NUM_SENSORS_BIT_MASK    0x7

// 2D Reporting Mode register bit mask
#define TP_REPORTING_MODE_FINGERS_BIT_MASK          0x7
#define TP_REPORTING_MODE_HAS_REL_MODE_BIT_MASK     1 << 3
#define TP_REPORTING_MODE_HAS_ABS_MODE_BIT_MASK     1 << 4
#define TP_REPORTING_MODE_HAS_GESTURES_BIT_MASK     1 << 5
#define TP_REPORTING_MODE_HAS_SENSITIVITY_BIT_MASK  1 << 6
#define TP_REPORTING_MODE_CONFIGURABLE_BIT_MASK     1 << 7

// 2D number of electrodes bit masks
#define TP_NUM_OF_X_ELECTRODES_BIT_MASK             0x1F
#define TP_NUM_OF_Y_ELECTRODES_BIT_MASK             0x1F
#define TP_MAX_NUM_OF_ELECTRODES_BIT_MASK           0x1F

// 2D Absolute query register bit mask
#define TP_ABS_QUERY_DATA_SIZE_BIT_MASK             0x3
#define TP_ABS_QUERY_ANCHORED_FINGER_BIT_MASK       1 << 2

// 2D Gesture Query register bits masks
#define TP_GESTURE_QUERY_HAS_SINGLE_TAP_BIT_MASK    1 << 0
#define TP_GESTURE_QUERY_HAS_TAP_N_HOLD_BIT_MASK    1 << 1
#define TP_GESTURE_QUERY_HAS_DOUBLE_TAP_BIT_MASK    1 << 2
#define TP_GESTURE_QUERY_HAS_EARLY_TAP_BIT_MASK     1 << 3
#define TP_GESTURE_QUERY_HAS_FLICK_BIT_MASK         1 << 4
#define TP_GESTURE_QUERY_HAS_PRESS_BIT_MASK         1 << 5
#define TP_GESTURE_QUERY_HAS_PINCH_BIT_MASK         1 << 6
#define TP_GESTURE_QUERY_HAS_XY_DISTANCE_BIT_MASK   1 << 7

// 2D query 8 register bit masks
#define TP_QUERY_8_HAS_PALM_REJECT_BIT_MASK         1 << 0
#define TP_QUERY_8_HAS_ROTATE_BIT_MASK              1 << 1

#if defined(__cplusplus)
}
#endif

#endif //SYNAPTICS_REG_HEADER
