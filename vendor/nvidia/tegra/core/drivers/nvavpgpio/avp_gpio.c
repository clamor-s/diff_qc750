/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
/*
 * This driver is independent of rm and can be used across anywhere in
 * the bootloader
 */
#include "avp_gpio.h"
#include "nvos.h"
#include "argpio.h"
#include "nvassert.h"
#include "arapb_misc.h"
#include "aos.h"
#include "nvrm_drf.h"
#include "nvrm_hardware_access.h"

#define MISC_PA_BASE              0x70000000
#define NV_ADDRESS_MAP_GPIO1_BASE 0x6000D000
#define GPIO_PINS_PER_PORT                 8
#define NV_GPIO_PORT_REG_SIZE   (GPIO_CNF_1 - GPIO_CNF_0)
#define AVP_GPIO_MASKED_WRITE(Base, MaskOffset, Port, Reg, Pin, Value) \
    do \
    { \
        NV_WRITE32((Base) + \
            ((Port) * NV_GPIO_PORT_REG_SIZE) + (MaskOffset) + \
            GPIO_##Reg##_0, \
            (((1<<((Pin)+ GPIO_PINS_PER_PORT)) | ((Value) << (Pin))))); \
    } while (0)

// Gpio register read/write macros
#define AVP_GPIO_REGR(Base, Port, Reg, ReadData) \
    do  \
    {   \
        ReadData = NV_READ32((Base) + ((Port) * NV_GPIO_PORT_REG_SIZE) + (GPIO_##Reg##_0)); \
    } while (0)

#define AVP_GPIO_REGW(Base, MaskOffset, Port, Reg, Data2Write ) \
    do \
    { \
        NV_WRITE32((Base) + ((Port) * NV_GPIO_PORT_REG_SIZE) + (MaskOffset) + \
        (GPIO_##Reg##_0), (Data2Write)); \
    } while (0)

static NvAvpGpioCaps s_t30_caps = {8, 4, GPIO_PINS_PER_PORT,
        NVRM_GPIO_CAP_FEAT_EDGE_INTR /* (SEE BUG# 366493) */, 0x80};

/**
 * @brief Provides a handle for gpio
 */
NvError
NvAvpGpioOpen(
    NvAvpGpioHandle* phGpio)
{
    NvU32 tmp;
    NvU32 id;

    if (!phGpio)
        return NvError_BadValue;

    //FIXME: will be extended for other platforms as well
    tmp = AOS_REGR( MISC_PA_BASE + APB_MISC_GP_HIDREV_0 );
    id = NV_DRF_VAL( APB_MISC_GP, HIDREV, CHIPID, tmp );

    if (id != 0x30)
        return NvError_NotSupported;

    phGpio->caps = &s_t30_caps;
    return NvSuccess;
}

/**
 * @brief Provides a handle for gpio pin
 */
NvError
NvAvpGpioAcquirePinHandle(
        NvAvpGpioHandle *phGpio,
        NvU32 Port,
        NvU32 PinNumber,
        NvAvpGpioPinHandle *phPin)
{
    NvU32 MaxPorts;

    if (!phGpio || !phPin)
        return NvError_BadValue;

    NV_ASSERT(4 == phGpio->caps->PortsPerInstances);
    MaxPorts = phGpio->caps->Instances * 4;

    if ((Port > MaxPorts) || (PinNumber > phGpio->caps->PinsPerPort))
    {
        NV_ASSERT(!" Illegal port or pin  number. ");
        return NvError_BadValue;
    }

    phPin->Instance = Port >> 2;
    phPin->Port      = Port & 0x3;
    phPin->PinNumber = PinNumber;
    phPin->InstanceBaseaddr = NV_ADDRESS_MAP_GPIO1_BASE + (Port >> 2) * 0x100;
    return NvSuccess;
}

/**
 * @brief Releases the handle for gpio pin
 */
NvError NvAvpGpioReleasePinHandle(
        NvAvpGpioHandle *phGpio,
        NvAvpGpioPinHandle *phPin)
{
    if (!phGpio || !phPin)
        return NvError_BadValue;

    AVP_GPIO_MASKED_WRITE(phPin->InstanceBaseaddr, phGpio->caps->MaskedOffset,
        phPin->Port, CNF, phPin->PinNumber, 0);
    return NvSuccess;
}

/**
 * @brief Read gpio pin
 */
NvError NvAvpGpioReadPin(
        NvAvpGpioHandle *phGpio,
        NvAvpGpioPinHandle *phPin,
        NvAvpGpioPinState *pPinState)
{
    NvU32 RegValue;

    if (!phGpio || !phPin)
        return NvError_BadValue;

    AVP_GPIO_REGR(phPin->InstanceBaseaddr, phPin->Port, OE, RegValue);
    if (RegValue & (1 << phPin->PinNumber))
    {
        AVP_GPIO_REGR(phPin->InstanceBaseaddr, phPin->Port, OUT, RegValue);
    }
    else
    {
        AVP_GPIO_REGR(phPin->InstanceBaseaddr, phPin->Port, IN, RegValue);
    }
    *pPinState = (RegValue >> phPin->PinNumber) & 0x1;
    return NvSuccess;
}

/**
 * @brief Write gpio pin
 */
NvError NvAvpGpioWritePin(
        NvAvpGpioHandle *phGpio,
        NvAvpGpioPinHandle *phPin,
        NvAvpGpioPinState *pPinState)
{
    NvU32 updateVec = 0;

    if (!phGpio || !phPin)
        return NvError_BadValue;

    updateVec = (1 << (phPin->PinNumber + GPIO_PINS_PER_PORT ));
    updateVec |= ((*pPinState & 0x1) << phPin->PinNumber);

    AVP_GPIO_REGW(phPin->InstanceBaseaddr, phGpio->caps->MaskedOffset,
        phPin->Port, OUT, updateVec);
    return NvSuccess;
}

/**
 * @brief Configure gpio pin
 */
NvError NvAvpGpioConfigPin(
        NvAvpGpioHandle *phGpio,
        NvAvpGpioPinHandle *phPin,
        NvAvpGpioPinMode Mode)
{
    if (!phGpio || !phPin)
        return NvError_BadValue;

    /* Don't try to collapse this switch as the ordering of the register
     * writes matter. */
    switch (Mode)
    {
        case NvAvpGpioPinMode_Output:
            AVP_GPIO_MASKED_WRITE(phPin->InstanceBaseaddr, phGpio->caps->MaskedOffset,
                phPin->Port, OE, phPin->PinNumber, 1);
            AVP_GPIO_MASKED_WRITE(phPin->InstanceBaseaddr, phGpio->caps->MaskedOffset,
                phPin->Port, CNF, phPin->PinNumber, 1);
            break;

        case NvAvpGpioPinMode_InputData:
            AVP_GPIO_MASKED_WRITE(phPin->InstanceBaseaddr, phGpio->caps->MaskedOffset,
                 phPin->Port, OE, phPin->PinNumber, 0);
            AVP_GPIO_MASKED_WRITE(phPin->InstanceBaseaddr, phGpio->caps->MaskedOffset,
                 phPin->Port, CNF, phPin->PinNumber, 1);
            break;

        default:
            NV_ASSERT(!"Invalid gpio mode");
            return NvError_NotSupported;
            break;
    }
    return NvSuccess;
}
