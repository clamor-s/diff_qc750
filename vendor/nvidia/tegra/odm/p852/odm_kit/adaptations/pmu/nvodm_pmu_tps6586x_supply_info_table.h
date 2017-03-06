/*
 * Copyright (c) 2009-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef TPS6586x_SUPPLY_INFO_TABLE_H_
#define TPS6586x_SUPPLY_INFO_TABLE_H_

#if defined(__cplusplus)
extern "C"
{
#endif

#define PMUGUID NV_ODM_GUID('t','p','s','6','5','8','6','x')

/* FIXME: modify this table according to your schematics */
#define V_CORE      TPS6586xPmuSupply_DCD0
#define V_1V8       TPS6586xPmuSupply_DCD1
#define LCD_2V8     TPS6586xPmuSupply_LDO0
#define V_1V2       TPS6586xPmuSupply_LDO1
#define V_RTC       TPS6586xPmuSupply_LDO2
#define V_CAM_1V8   TPS6586xPmuSupply_LDO3
#define V_CODEC_1V8 TPS6586xPmuSupply_LDO4
#define V_CAM_2V8   TPS6586xPmuSupply_LDO5
#define V_3V3       TPS6586xPmuSupply_LDO6
#define V_SDIO      TPS6586xPmuSupply_LDO7
#define V_2V8       TPS6586xPmuSupply_LDO8
#define V_2V5       TPS6586xPmuSupply_LDO9
#define V_25V       TPS6586xPmuSupply_WHITE_LED
#define V_CHARGE    TPS6586xPmuSupply_DCD2
#define V_SOC       TPS6586xPmuSupply_SoC
#define V_MODEM     V_1V8   /* Alias for V_1V8 */
#define V_GND       TPS6586xPmuSupply_Invalid
#define V_INVALID   TPS6586xPmuSupply_Invalid
#define VRAILCOUNT  TPS6586xPmuSupply_Num

typedef enum
{
    TPS6586xPmuSupply_Invalid = 0x0,

    //DCD0
    TPS6586xPmuSupply_DCD0,

    //DCD1
    TPS6586xPmuSupply_DCD1,

    //DCD2
    TPS6586xPmuSupply_DCD2,


    //LDO0
    TPS6586xPmuSupply_LDO0,

    //LDO1
    TPS6586xPmuSupply_LDO1,

    //LDO2
    TPS6586xPmuSupply_LDO2,

    //LDO3
    TPS6586xPmuSupply_LDO3,

    //LDO4
    TPS6586xPmuSupply_LDO4,

    //LDO5
    TPS6586xPmuSupply_LDO5,

    //LDO6
    TPS6586xPmuSupply_LDO6,

    //LDO7
    TPS6586xPmuSupply_LDO7,

    //LDO8
    TPS6586xPmuSupply_LDO8,

    //LDO9
    TPS6586xPmuSupply_LDO9,

    //RTC_OUT
    TPS6586xPmuSupply_RTC_OUT,

    //RED1
    TPS6586xPmuSupply_RED1,

    //GREEN1
    TPS6586xPmuSupply_GREEN1,

    //BLUE1
    TPS6586xPmuSupply_BLUE1,

    //RED2
    TPS6586xPmuSupply_RED2,

    //GREEN2
    TPS6586xPmuSupply_GREEN2,

    //BLUE2
    TPS6586xPmuSupply_BLUE2,

    //LED_PWM
    TPS6586xPmuSupply_LED_PWM,

    //PWM
    TPS6586xPmuSupply_PWM,

    //White LED(SW3)
    TPS6586xPmuSupply_WHITE_LED,

    //SOC
    TPS6586xPmuSupply_SoC,

    TPS6586xPmuSupply_Num,

    TPS6586xPmuSupply_Force32 = 0x7FFFFFFF
} TPS6586xPmuSupply;

#if defined(__cplusplus)
}
#endif

#endif /* TPS6586x_SUPPLY_INFO_TABLE_H_ */
