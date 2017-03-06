/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */


#include "nvodm_imager_util.h"
#include "nvodm_query_gpio.h"
#include "nvodm_imager_i2c.h"
#include "nvodm_imager_guids.h"
#include "nvodm_services.h"

#define I2C_WAIT_LIMIT (100)
#define I2CSPEED (100)

#define WORKAROUND_FOR_NVBUG_508581 1

static void AssignGpio(
    ImagerGpio *pOutGpios,
    NvU32 PinMappings,
    ImagerGpio NewGpio);


NvBool
WriteI2CSequence8(
    NvOdmImagerI2cConnection *pI2c,
    DevCtrlReg16 *pSequence,
    NvBool FastSetMode)
{
    NvBool FastSetModeRegs = NV_FALSE;
    DevCtrlReg16 *ptr = pSequence;
    NvOdmI2cStatus Status;
    NvBool Success = NV_TRUE;

    if (!pI2c || !pI2c->hOdmI2c || !pSequence)
        return NV_FALSE;

    while (ptr->RegAddr != SEQUENCE_END)
    {
        switch(ptr->RegAddr)
        {
            case SEQUENCE_WAIT_MS:
                if (!FastSetMode || FastSetModeRegs)
                {
                    NvOdmOsWaitUS(ptr->RegValue * 1000);
                }
                break;

            case SEQUENCE_FAST_SETMODE_START:
                FastSetModeRegs = NV_TRUE;
                break;

            case SEQUENCE_FAST_SETMODE_END:
                FastSetModeRegs = NV_FALSE;
                break;

            default:
                if (!FastSetMode || FastSetModeRegs)
                {
                    Status = NvOdmImagerI2cWrite16Data8(pI2c, ptr->RegAddr, (NvU8)ptr->RegValue);
                    if (Status != NvOdmI2cStatus_Success)
                        Success = NV_FALSE;
                }
                break;
        }
        ptr += 1;
    }

    return Success;
}
NvBool WriteI2CSequence(NvOdmImagerI2cConnection *pI2c, DevCtrlReg16 *pSequence)
{
    DevCtrlReg16 *ptr = pSequence;
    NvOdmI2cStatus status;
    NvBool success = NV_TRUE;

    if (!pI2c || !pI2c->hOdmI2c || !pSequence)
        return NV_FALSE;

    while (ptr->RegAddr != SEQUENCE_END)
    {
        // delay?
        if (ptr->RegAddr == SEQUENCE_WAIT_MS)
        {
            NvOdmOsWaitUS(ptr->RegValue * 1000);
        }
        // I2C writes
        else
        {
            status = NvOdmImagerI2cWrite16(pI2c, ptr->RegAddr, ptr->RegValue);
            if (status != NvOdmI2cStatus_Success)
                success = NV_FALSE;
        }

        ptr += 1;
    }

    NvOdmOsWaitUS(333000);

    return success;
}

static void AssignGpio(
    ImagerGpio *pOutGpios,
    NvU32 PinMappings,
    ImagerGpio NewGpio)
{
    NvU32 Code, tmp;

    NewGpio.enable = NV_TRUE;
    Code = PinMappings >> (NewGpio.pin * NVODM_CAMERA_GPIO_PIN_WIDTH);
    tmp = Code & (NVODM_CAMERA_GPIO_PIN_MASK <<
                  NVODM_CAMERA_GPIO_PIN_SHIFT);
    switch (tmp)
    {
        case NVODM_CAMERA_RESET(0):
            NewGpio.activeHigh = NV_TRUE;
            pOutGpios[NvOdmImagerGpio_Reset] = NewGpio;
            break;
        case NVODM_CAMERA_RESET_AL(0):
            NewGpio.activeHigh = NV_FALSE;
            pOutGpios[NvOdmImagerGpio_Reset] = NewGpio;
            break;
        case NVODM_CAMERA_POWERDOWN(0):
            NewGpio.activeHigh = NV_TRUE;
            pOutGpios[NvOdmImagerGpio_Powerdown] = NewGpio;
            break;
        case NVODM_CAMERA_POWERDOWN_AL(0):
            NewGpio.activeHigh = NV_FALSE;
            pOutGpios[NvOdmImagerGpio_Powerdown] = NewGpio;
            break;
        case NVODM_CAMERA_FLASH_LOW(0):
            NewGpio.activeHigh = NV_TRUE;
            pOutGpios[NvOdmImagerGpio_FlashLow] = NewGpio;
            break;
        case NVODM_CAMERA_FLASH_HIGH(0):
            NewGpio.activeHigh = NV_TRUE;
            pOutGpios[NvOdmImagerGpio_FlashHigh] = NewGpio;
            break;
        case NVODM_CAMERA_MCLK(0):
            if (NewGpio.pin == 0)
                pOutGpios[NvOdmImagerGpio_MCLK] = NewGpio;
            else if (NewGpio.pin == 6)
                pOutGpios[NvOdmImagerGpio_PWM] = NewGpio;
            else
                NV_ASSERT(!"Unknown GPIO config");
            break;
        case NVODM_CAMERA_UNUSED:
            break;
        default:
            NV_ASSERT(!"Undefined gpio");
    }
}


/**
 * SetPowerLevelWithPeripheralConnectivity
 *
 * If PowerLevel == NvOdmImagerPowerLevel_On, this function parses nvodm
 * peripheral connectivity information in pConnections and stores the gpio
 * config in pGpioConfig which can be used later for powering down.
 * If PowerLevel == NvOdmImagerPowerLevel_Off, this function uses the gpio
 * config in pGpioConfig to power down and releases the resources allocated
 * during powering on.
 * If PowerLevel == NvOdmImagerPowerLevel_Standby, this function uses the
 * gpio config in pGpioConfig for standby.
 *
 * @param pConnections NvOdmPeripheralConnectivity
 * @param pI2c a pointer to NvOdmImagerI2cConnection. This struct will be
 *             initialized while powering on. For standby mode, it's used
 *             as an input parameter. While powering off, this struct will
 *             be filled with 0.
 * @param pGpioConfig a pointer to ImagerGpioConfig. This struct will be
 *             initialized while powering on. For standby mode, it's used
 *             as an input parameter. While powering off, this struct will
 *             be filled with 0.
 * @param PowerLevel the power level.
 * @retval NV_TRUE on success
 *
 */

NvBool
SetPowerLevelWithPeripheralConnectivity(
    const NvOdmPeripheralConnectivity *pConnections,
    NvOdmImagerI2cConnection *pI2c,
    ImagerGpioConfig *pGpioConfig,
    NvOdmImagerPowerLevel PowerLevel)
{
    NvOdmServicesPmuHandle hPmu = NULL;
    NvU32 i = 0;
    NvU32 UnassignedGpioCount = 0;
    ImagerGpio UnassignedGpio[NvOdmImagerGpio_Num];

    if (!pConnections || !pI2c || !pGpioConfig)
        return NV_FALSE;

    NvOdmOsMemset(&UnassignedGpio[0], 0, sizeof(UnassignedGpio));

    switch(PowerLevel)
    {
        case NvOdmImagerPowerLevel_On:
            NvOdmOsMemset(pGpioConfig, 0, sizeof(ImagerGpioConfig));

            hPmu = NvOdmServicesPmuOpen();
            if (!hPmu)
                return NV_FALSE;

            // start parsing the connectivity information
            for (i = 0; i < pConnections->NumAddress; i++)
            {
                switch(pConnections->AddressList[i].Interface)
                {
                    case NvOdmIoModule_Vdd:
                        {
                            NvOdmServicesPmuVddRailCapabilities Cap;
                            NvU32 Settle_us;
                            // address is the vdd rail id
                            NvOdmServicesPmuGetCapabilities(hPmu,
                               pConnections->AddressList[i].Address,
                               &Cap);
                            // set the rail voltage to the recommended
                            NvOdmServicesPmuSetVoltage(hPmu,
                               pConnections->AddressList[i].Address,
                               Cap.requestMilliVolts,
                               &Settle_us);
                            // wait for rail to settle
                            NvOdmOsWaitUS(Settle_us);
                        }
                        break;
                    case NvOdmIoModule_I2c:
                        {
                            pI2c->DeviceAddr =
                                pConnections->AddressList[i].Address;
                            pI2c->I2cPort =
                                pConnections->AddressList[i].Instance;

                            if (pI2c->I2cPort != NVODM_IMAGER_I2C_PORT)
                            {
                                NvOdmOsDebugPrintf("Warning, you are not ");
                                NvOdmOsDebugPrintf("using the I2c port that ");
                                NvOdmOsDebugPrintf("is for the imager\n");
                            }
                        }
                        break;
                    case NvOdmIoModule_Gpio:
                        {
#if WORKAROUND_FOR_NVBUG_508581
  #define NVODM_PORT(x) ((x) - 'a')
                            if(pConnections->AddressList[i].Instance == NVODM_PORT('t')) {
                                ImagerGpio NewGpio;

                                NvOdmOsMemset(&NewGpio, 0, sizeof(ImagerGpio));
                                NewGpio.port =
                                 pConnections->AddressList[i].Instance;
                                NewGpio.pin =
                                 pConnections->AddressList[i].Address;
                                AssignGpio(pGpioConfig->Gpios,
                                    pGpioConfig->PinMappings, NewGpio);
                                NewGpio.enable = NV_TRUE;
                                NewGpio.activeHigh = NV_TRUE;
                                pGpioConfig->Gpios[NvOdmImagerGpio_Powerdown] = NewGpio;

                            }
                            else {
                                if (pGpioConfig->PinMappings == 0)
                                {
                                    NV_ASSERT(UnassignedGpioCount <
                                              NvOdmImagerGpio_Num);
                                    UnassignedGpio[UnassignedGpioCount].port =
                                        pConnections->AddressList[i].Instance;
                                    UnassignedGpio[UnassignedGpioCount].pin =
                                        pConnections->AddressList[i].Address;
                                    UnassignedGpioCount++;
                                }
                                else
                                {
                                    ImagerGpio NewGpio;

                                    NvOdmOsMemset(&NewGpio, 0, sizeof(ImagerGpio));
                                    NewGpio.port =
                                     pConnections->AddressList[i].Instance;
                                    NewGpio.pin =
                                     pConnections->AddressList[i].Address;
                                    AssignGpio(pGpioConfig->Gpios,
                                        pGpioConfig->PinMappings, NewGpio);
                                }
                            }
#endif
                        }
                        break;
                    case NvOdmIoModule_VideoInput:
                        {
                            NvU32 Code;
#if WORKAROUND_FOR_NVBUG_508581
                            if (pGpioConfig->PinMappings != 1) {
#endif
                                NV_ASSERT(pGpioConfig->PinMappings == 0);
                                pGpioConfig->PinMappings =
                                    pConnections->AddressList[i].Address;
                                while (UnassignedGpioCount)
                                {
                                    UnassignedGpioCount--;
                                    AssignGpio(pGpioConfig->Gpios,
                                        pGpioConfig->PinMappings,
                                        UnassignedGpio[UnassignedGpioCount]);
                                }
                                // Check the data connection
                                Code = ((pGpioConfig->PinMappings >>
                                         NVODM_CAMERA_DATA_PIN_SHIFT)
                                        & NVODM_CAMERA_DATA_PIN_MASK);
                                switch (Code)
                                {
                                    case NVODM_CAMERA_PARALLEL_VD0_TO_VD9:
                                    case NVODM_CAMERA_PARALLEL_VD0_TO_VD7:
                                    case NVODM_CAMERA_SERIAL_CSI_D1A:
                                    case NVODM_CAMERA_SERIAL_CSI_D2A:
                                    case NVODM_CAMERA_SERIAL_CSI_D1A_D2A:
                                    case NVODM_CAMERA_SERIAL_CSI_D1B:
                                    default:
                                        break;
                                }
#if WORKAROUND_FOR_NVBUG_508581
                            }
#endif
                        }
                        break;
                    case NvOdmIoModule_ExternalClock:
                        // No need to handle this, I guess?
                        break;
                    default:
                        NV_ASSERT(!"Unknown NvOdmIoAddress used");
                        break;
                }
            }

            NvOdmServicesPmuClose(hPmu);

            // Need to do I2cOpen after pmu close, rather than up in the
            // switch statement.  So says a CE.
            {
#ifndef SET_KERNEL_PINMUX
                const NvU32 *pI2cConfigs;
                NvU32 NumI2cConfigs;

                NvOdmQueryPinMux(NvOdmIoModule_I2c, &pI2cConfigs, &NumI2cConfigs);

                if (pI2c->I2cPort > NumI2cConfigs)
                {
                    NV_ASSERT(!"Invalid i2c port");
                    return NV_FALSE;
                }

                if (pI2cConfigs[pI2c->I2cPort] == NvOdmI2cPinMap_Multiplexed)
                    pI2c->hOdmI2c = NvOdmI2cPinMuxOpen(NvOdmIoModule_I2c,
                                                       pI2c->I2cPort,
                                                       NvOdmI2cPinMap_Config1); // Concorde2
                else
#endif
                    pI2c->hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, pI2c->I2cPort);

                if (!pI2c->hOdmI2c)
                {
                    NV_ASSERT(!"Cannot get i2c handle");
                    return NV_FALSE;
                }
            }

            if (pGpioConfig->Gpios[NvOdmImagerGpio_Powerdown].enable)
            {
                const NvOdmGpioPinInfo *s_gpioPinTable;
                NvU32 gpioPinCount;
                NvU32 Port, Pin;
                NvOdmGpioPinActiveState Level;

                s_gpioPinTable = NvOdmQueryGpioPinMap(NvOdmGpioPinGroup_Vi, 0,
                    &gpioPinCount);

                if (gpioPinCount < 1) {
                    NV_ASSERT("! pin count is less than 1");
                }

                Port = s_gpioPinTable->Port;
                Pin = s_gpioPinTable->Pin;
                Level = s_gpioPinTable->activeState;


                if (pGpioConfig->hGpio == NULL)
                    pGpioConfig->hGpio = NvOdmGpioOpen();
                if (pGpioConfig->PinsUsed >= MAX_GPIO_PINS)
                {
                    NV_ASSERT(
                        !"Not enough handles in the context for this pin");
                    return NV_FALSE;
                }
                pGpioConfig->hPin[pGpioConfig->PinsUsed] =
                    NvOdmGpioAcquirePinHandle(pGpioConfig->hGpio, Port, Pin);
                NvOdmGpioConfig(pGpioConfig->hGpio, pGpioConfig->hPin[pGpioConfig->PinsUsed], NvOdmGpioPinMode_Output);
                //NvOdmOsWaitUS(500000); // wait 500ms
                NvOdmGpioSetState(pGpioConfig->hGpio,
                    pGpioConfig->hPin[pGpioConfig->PinsUsed], Level);
                pGpioConfig->PinType[pGpioConfig->PinsUsed] = NvOdmImagerGpio_Powerdown;
                //NvOdmOsWaitUS(500000); // wait 500ms
                pGpioConfig->PinsUsed++;
            }

            if (pGpioConfig->Gpios[NvOdmImagerGpio_Reset].enable)
            {
                NvU32 Port = pGpioConfig->Gpios[NvOdmImagerGpio_Reset].port;
                NvU32 Pin = pGpioConfig->Gpios[NvOdmImagerGpio_Reset].pin;
                NvOdmGpioPinActiveState LevelA, LevelB;
                LevelA = NvOdmGpioPinActiveState_Low;
                LevelB = NvOdmGpioPinActiveState_High;
                if (!pGpioConfig->Gpios[NvOdmImagerGpio_Reset].activeHigh)
                {
                    LevelA = NvOdmGpioPinActiveState_High;
                    LevelB = NvOdmGpioPinActiveState_Low;
                }

                if (pGpioConfig->hGpio == NULL)
                    pGpioConfig->hGpio = NvOdmGpioOpen();
                if (pGpioConfig->PinsUsed >= MAX_GPIO_PINS)
                {
                    NV_ASSERT(
                        !"Not enough handles in the context for this pin");
                    return NV_FALSE;
                }
                pGpioConfig->hPin[pGpioConfig->PinsUsed] =
                    NvOdmGpioAcquirePinHandle(pGpioConfig->hGpio, Port, Pin);
                NvOdmGpioSetState(pGpioConfig->hGpio,
                    pGpioConfig->hPin[pGpioConfig->PinsUsed], LevelA);
                NvOdmOsWaitUS(1000); // wait 1 ms
                NvOdmGpioSetState(pGpioConfig->hGpio,
                    pGpioConfig->hPin[pGpioConfig->PinsUsed], LevelB);
                NvOdmOsWaitUS(1000); // wait 1 ms
                NvOdmGpioSetState(pGpioConfig->hGpio,
                    pGpioConfig->hPin[pGpioConfig->PinsUsed], LevelA);
                pGpioConfig->PinType[pGpioConfig->PinsUsed] = NvOdmImagerGpio_Reset;
                pGpioConfig->PinsUsed++;
            }

            NvOdmOsWaitUS(1000); // allow pin changes to settle
            break;

        case NvOdmImagerPowerLevel_Standby:
            if (pGpioConfig->Gpios[NvOdmImagerGpio_Powerdown].enable)
            {
                NvOdmGpioPinActiveState Level;
                NvOdmGpioGetState(pGpioConfig->hGpio, pGpioConfig->hPin[0],
                    (NvU32 *)&Level);
                NvOdmGpioSetState(pGpioConfig->hGpio, pGpioConfig->hPin[0],
                    !Level);
            }

            break;
        case NvOdmImagerPowerLevel_Off:
            // Turn off PMU voltages
            hPmu = NvOdmServicesPmuOpen();
            if (!hPmu)
                return NV_FALSE;

            // Turn off VDD
            for (i = 0; i < pConnections->NumAddress; i++)
            {
                switch (pConnections->AddressList[i].Interface)
                {
                    case NvOdmIoModule_Vdd:
                        {
                            NvU32 Settle_us;
                            NvU32 Millivolts;
                            /* set the rail voltage to the recommended */
                            NvOdmServicesPmuSetVoltage(hPmu,
                               pConnections->AddressList[i].Address,
                               NVODM_VOLTAGE_OFF,
                               &Settle_us);
                            /* wait for rail to settle */
                            NvOdmOsWaitUS(Settle_us);
                            /* make sure it is off */
                            NvOdmServicesPmuGetVoltage(hPmu,
                               pConnections->AddressList[i].Address,
                               &Millivolts);
                        }
                        break;
                    default:
                        break;
                }
            }
            NvOdmServicesPmuClose(hPmu);
            /// Required delay
            NvOdmOsWaitUS(5000);

            // Reset the sensor
            for (i = 0; i < 2; i++)
            {
                if (pGpioConfig->hPin[i])
                {
                    NvOdmGpioPinActiveState Level;
                    NvOdmGpioGetState(pGpioConfig->hGpio, pGpioConfig->hPin[i],
                        (NvU32 *)&Level);
                    NvOdmGpioSetState(pGpioConfig->hGpio, pGpioConfig->hPin[i],
                        !Level);
                }
                NvOdmGpioReleasePinHandle(pGpioConfig->hGpio,
                    pGpioConfig->hPin[i]);
            }

            // release the resources
            NvOdmGpioClose(pGpioConfig->hGpio);
            NvOdmI2cClose(pI2c->hOdmI2c);
            NvOdmOsMemset(pGpioConfig, 0, sizeof(ImagerGpioConfig));
            NvOdmOsMemset(pI2c, 0, sizeof(NvOdmImagerI2cConnection));

            break;
        default:
            return NV_FALSE;
    }

    return NV_TRUE;
}

