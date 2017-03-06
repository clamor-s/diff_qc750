/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_powerkeys.h"
#include "nvodm_query_discovery.h"
#include "nvodm_query_gpio.h"
#include "nvrm_gpio.h"

// Module debug: 0=disable, 1=enable
#define NVODM_ENABLE_PRINTF      0

#if NVODM_ENABLE_PRINTF
#define NVODM_PRINTF(x) NvOdmOsDebugPrintf x
#else
#define NVODM_PRINTF(x)
#endif

// EC interface not enabled yet for Sleep/Power Down
#define ECI_ENABLED 0

#define DEBOUNCE_TIME_MS 5 // GPIO debounce time in ms

#define MAX_POWER_KEYS 2 // Lid and Power key

#if ECI_ENABLED
// FIXME -- Varun, these should be derived from nvec.idl/nvec.h
#define CONFIGURE_REQ_PAYLOAD_SIZE 5
#define ACK_SYS_STATUS_PAYLOAD_SIZE 4
#define AP_SLEEP_REQ 1 << 2
#define AP_POWERDN_REQ 1 << 3
#endif

#define NVODM_QUERY_BOARD_ID_E1166  0x0B42
#define NVODM_QUERY_BOARD_ID_E1173  0x0B49

typedef struct NvOdmPowerKeysContextRec
{
    const NvOdmGpioPinInfo *GpioPinInfo; // GPIO info struct
    NvRmGpioPinHandle hPin[MAX_POWER_KEYS]; // GPIO pin handles
    NvRmGpioInterruptHandle GpioIntrHandle;
    NvU32 PinCount;
    NvRmDeviceHandle hRm;
    NvRmGpioHandle hGpio;
    NvOdmOsSemaphoreHandle hSema; // client semaphore
#if ECI_ENABLED
    NvEcEventRegistrationHandle hEcEventRegistration;
    NvOdmOsSemaphoreHandle hGpioEventSema;
    NvOdmOsThreadHandle hOffChipGpioThread;
    NvEcHandle hEc;
    NvOdmOsMutexHandle hMutex;
    //flag to indicate Sleep command
    NvBool APSleep;
    //flag to indicate power down command
    NvBool APPowerDown;
    // states for which we have registered with the off chip component
    NvU32 OffChipStateReq;
#endif
} NvOdmPowerKeysContext;

/* callback function which informs the client of the LID event */
static void
GpioInterruptHandler(void *args)
{
    NvOdmPowerKeysHandle hOdm = args;

    if (hOdm)
    {
        if (hOdm->hSema)
            NvOdmOsSemaphoreSignal(hOdm->hSema);
    }

    NvRmGpioInterruptDone(hOdm->GpioIntrHandle);
}

#if ECI_ENABLED
static
NvOdmOsThreadFunction HandleOffChipEvents(void *args)
{
    NvOdmPowerKeysHandle hOdm = args;
    NvError NvStatus = NvError_Success;
    NvEcEvent EcEvent = {0};
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};

    for (;;)
    {
        NvOdmOsSemaphoreWait(hOdm->hGpioEventSema);

        if (hOdm->hEcEventRegistration)
        {
            NvStatus = NvEcGetEvent(hOdm->hEcEventRegistration, &EcEvent, sizeof(NvEcEvent));
            if (NvStatus != NvError_Success)
            {
                NV_ASSERT(!"Could not receive EC event");
                continue;
            }

            if (EcEvent.NumPayloadBytes == 0)
            {
                NV_ASSERT(!"Received keyboard event with no scan codes");
                continue;
            }

            // check if we need to sleep or power down
            if (EcEvent.Payload[0] & (NvU8)AP_SLEEP_REQ)
            {
                // AP should sleep
                NvOdmOsMutexLock(hOdm->hMutex);
                hOdm->APSleep = NV_TRUE;
                NvOdmOsMutexUnlock(hOdm->hMutex);
            }
            else if (EcEvent.Payload[0] & (NvU8)AP_POWERDN_REQ)
            {
                // AP should power down
                NvOdmOsMutexLock(hOdm->hMutex);
                hOdm->APPowerDown = NV_TRUE;
                NvOdmOsMutexUnlock(hOdm->hMutex);
            }

            /* acknowledge sleep/power-dn system events */
            Request.PacketType = NvEcPacketType_Request;
            Request.RequestType = NvEcRequestResponseType_System;
            Request.RequestSubtype = (NvEcRequestResponseSubtype)NvEcSystemSubtype_AcknowledgeSystemStatus;
            Request.Payload[0] = EcEvent.Payload[0];
            Request.Payload[1] = 0;
            Request.Payload[2] = 0;
            Request.Payload[3] = 0;
            Request.NumPayloadBytes = ACK_SYS_STATUS_PAYLOAD_SIZE; // number of bytes for Configure Request

            NvStatus = NvEcSendRequest(hOdm->hEc, &Request, &Response, sizeof(Request), sizeof(Response));
            if (NvStatus != NvError_Success)
            {
                NV_ASSERT("Acknowledege command send fail");
                continue;
            }

            /* check if command passed */
            if (Response.Status != NvEcStatus_Success)
            {
                NV_ASSERT("Acknowledege command fail");
                continue;
            }

            // inform the client
            if (hOdm->hSema)
                NvOdmOsSemaphoreSignal(hOdm->hSema);
        }
    }
}

/* Register with the off chip component to receive Sleep/Power Down events */
static NvBool OffChipGpioEventsRegister(NvOdmPowerKeysHandle hOdm)
{
    NvError e = NvError_Success;
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};
    NvEcEventType EventTypes[] = {NvEcEventType_System};

    /* get nvec handle */
    e = NvEcOpen(&hOdm->hEc, 0 /* instance */);
    if (e != NvError_Success)
    {
        goto cleanup;
    }

    /* configure automatic reporting of sleep/power-dn system events */
    Request.PacketType = NvEcPacketType_Request;
    Request.RequestType = NvEcRequestResponseType_System;
    Request.RequestSubtype = (NvEcRequestResponseSubtype)NvEcSystemSubtype_ConfigureEventReporting;
    Request.Payload[0] = NVEC_SYSTEM_REPORT_ENABLE_0_ACTION_ENABLE;
    Request.Payload[1] = (NvU8)(hOdm->OffChipStateReq & 0xFF);
    Request.Payload[2] = (NvU8)(hOdm->OffChipStateReq & (0xFF << 8));
    Request.Payload[3] = (NvU8)(hOdm->OffChipStateReq & (0xFF << 16));
    Request.Payload[4] = (NvU8)(hOdm->OffChipStateReq & (0xFF << 24));
    Request.NumPayloadBytes = CONFIGURE_REQ_PAYLOAD_SIZE; // number of bytes for Configure Request

    e = NvEcSendRequest(hOdm->hEc, &Request, &Response, sizeof(Request), sizeof(Response));
    if (e != NvError_Success)
    {
        goto cleanup;
    }

    /* check if command passed */
    if (Response.Status != NvEcStatus_Success)
    {
        goto cleanup;
    }

    /* semaphore to receive events from EC */
    hOdm->hGpioEventSema = NvOdmOsSemaphoreCreate(0);
    if (!hOdm->hGpioEventSema)
    {
        goto cleanup;
    }

    /* register for system events */
    e = NvEcRegisterForEvents(
                hOdm->hEc,       // nvec handle
                &hOdm->hEcEventRegistration,
                (NvOsSemaphoreHandle)hOdm->hGpioEventSema,
                sizeof(EventTypes)/sizeof(NvEcEventType),
                EventTypes,
                1,          // currently buffer only 1 packet from ECI at a time
                sizeof(NvEcEvent));
    if (e != NvError_Success)
    {
        goto cleanup;
    }

    /* thread to handle off chip events */
    hOdm->hOffChipGpioThread = NvOdmOsThreadCreate((NvOdmOsThreadFunction)HandleOffChipEvents, (void *)hOdm);
    if (!hOdm->hOffChipGpioThread)
    {
        goto cleanup;
    }

    /* create mutex to read/write GPIO states */
    hOdm->hMutex = NvOdmOsMutexCreate();
    if (!hOdm->hMutex)
        goto cleanup;

    return NV_TRUE;

cleanup:
    if (hOdm->hEc)
    {
        (void)NvEcUnregisterForEvents(hOdm->hEcEventRegistration);
        hOdm->hEcEventRegistration = NULL;
    }

    NvOdmOsThreadJoin(hOdm->hOffChipGpioThread);
    hOdm->hOffChipGpioThread = NULL;

    NvOdmOsSemaphoreDestroy(hOdm->hGpioEventSema);
    hOdm->hGpioEventSema = NULL;

    NvEcClose(hOdm->hEc);
    hOdm->hEc = NULL;

    NvOdmOsMutexDestroy(hOdm->hMutex);
    hOdm->hMutex = NULL;

    return NV_FALSE;
}
#endif

/* Register for specific events */
NvBool NvOdmPowerKeysOpen(
                NvOdmPowerKeysHandle *phOdm,
                NvOdmOsSemaphoreHandle *phSema,
                NvU32 *pNumOfKeys)
{
    NvError e = NvError_Success;
    NvOsInterruptHandler IntrHandler = {NULL};
    NvU32 i = 0, NumOfKeys = 0;
    NvOdmBoardInfo BoardInfo;

    if (!phSema || !pNumOfKeys)
        return NV_FALSE;

    // allocate handle
    *phOdm = (NvOdmPowerKeysHandle)NvOdmOsAlloc(sizeof(NvOdmPowerKeysContext));
    if (!*phOdm)
    {
        return NV_FALSE;
    }
    NvOsMemset(*phOdm, 0, sizeof(NvOdmPowerKeysContext));

    e = NvRmOpen(&(*phOdm)->hRm, 0);
    if (e != NvError_Success)
        goto cleanup;

    e = NvRmGpioOpen((*phOdm)->hRm, &(*phOdm)->hGpio);
    if (e != NvError_Success)
        goto cleanup;

    // store the client semaphore that we need to signal
    (*phOdm)->hSema = *phSema;

    /** 
     * Check if hardware SKU supports lid closure switch. The lid 
     * closure switch is not supported if the E1166 board SKU is not 
     * found (for the satellite board) or if the board SKU is 1173. 
     */
    if ( NvOdmPeripheralGetBoardInfo(NVODM_QUERY_BOARD_ID_E1166, &BoardInfo) ||
         NvOdmPeripheralGetBoardInfo(NVODM_QUERY_BOARD_ID_E1173, &BoardInfo) )
    {
        // Check the supported GPIOs
        (*phOdm)->GpioPinInfo = NvOdmQueryGpioPinMap(
                                    NvOdmGpioPinGroup_Power,
                                    0,
                                    &(*phOdm)->PinCount);
    }
    else
    {
        // No support for lid switch
        (*phOdm)->GpioPinInfo = NULL;
    }

    if ((*phOdm)->GpioPinInfo != NULL)
    {
        IntrHandler = (NvOsInterruptHandler)GpioInterruptHandler;

        for (i = 0; i < MAX_POWER_KEYS; i++)
        {
            if (((*phOdm)->GpioPinInfo[i].Port != 0) &&
                ((*phOdm)->GpioPinInfo[i].Pin != 0))
            {
                NvRmGpioAcquirePinHandle(
                        (*phOdm)->hGpio,
                        (*phOdm)->GpioPinInfo[i].Port,
                        (*phOdm)->GpioPinInfo[i].Pin,
                        &(*phOdm)->hPin[i]);
                if (!(*phOdm)->hPin[i])
                {
                    goto cleanup;
                }

                // register to receive GPIO events
                e = NvRmGpioInterruptRegister(
                            (*phOdm)->hGpio,
                            (*phOdm)->hRm,
                            (*phOdm)->hPin[i],
                            IntrHandler,
                            NvRmGpioPinMode_InputInterruptAny,
                            *phOdm,
                            &(*phOdm)->GpioIntrHandle,
                            DEBOUNCE_TIME_MS);
                if (e != NvError_Success)
                {
                    goto cleanup;
                }

                e = NvRmGpioInterruptEnable((*phOdm)->GpioIntrHandle);
                if (e != NvError_Success)
                {
                    goto cleanup;
                }
                NumOfKeys++;
            }
#if ECI_ENABLED
            else
            {
                if (i + 1 == NvOdmPowerKeys_LidStatus)
                {
                    // register for sleep request only
                    (*phOdm)->OffChipStateReq = AP_SLEEP_REQ;
                    NumOfKeys++;
                }
                else if (i + 1 == NvOdmPowerKeys_PowerOnOff)
                {
                    // register for power down request only
                    (*phOdm)->OffChipStateReq = AP_POWERDN_REQ;
                    NumOfKeys++;
                }

                // register for off chip events
                if (!OffChipGpioEventsRegister(*phOdm))
                {
                    goto cleanup;
                }
            }
#endif
        }
    }
#if ECI_ENABLED
    else
    {
        // register for off chip events
        (*phOdm)->OffChipStateReq = AP_SLEEP_REQ | AP_POWERDN_REQ;
        if (!OffChipGpioEventsRegister(*phOdm))
        {
            goto cleanup;
        }

        NumOfKeys = MAX_POWER_KEYS; // Sleep and Power Keys
    }
#endif

    *pNumOfKeys = NumOfKeys;
    return NV_TRUE;

cleanup:
    // unregister the gpio interrupt handler
    NvRmGpioInterruptUnregister((*phOdm)->hGpio, (*phOdm)->hRm, (*phOdm)->GpioIntrHandle);
    (*phOdm)->GpioIntrHandle = NULL;

    // release pin handle
    for (i = 0; i < MAX_POWER_KEYS; i++)
    {
        NvRmGpioReleasePinHandles((*phOdm)->hGpio, &(*phOdm)->hPin[i], (*phOdm)->PinCount);
    }

    NvRmGpioClose((*phOdm)->hGpio);
    (*phOdm)->hGpio = NULL;

    NvRmClose((*phOdm)->hRm);
    (*phOdm)->hRm = NULL;

    NvOdmOsFree(phOdm);
    phOdm = NULL;

    return NV_FALSE;
}

/* Unregister the specific registered events */
void NvOdmPowerKeysClose(NvOdmPowerKeysHandle hOdm)
{
    NvU32 i = 0;

    if (hOdm)
    {
        // unregister the gpio interrupt handler
        NvRmGpioInterruptUnregister(hOdm->hGpio, hOdm->hRm, hOdm->GpioIntrHandle);
        hOdm->GpioIntrHandle = NULL;

        // release pin handle
        for (i = 0; i < MAX_POWER_KEYS; i++)
        {
            NvRmGpioReleasePinHandles(hOdm->hGpio, &hOdm->hPin[i], hOdm->PinCount);
        }

        NvRmGpioClose(hOdm->hGpio);
        hOdm->hGpio = NULL;

        NvRmClose(hOdm->hRm);
        hOdm->hRm = NULL;

#if ECI_ENABLED
        if (hOdm->hEc)
        {
            (void)NvEcUnregisterForEvents(hOdm->hEcEventRegistration);
            hOdm->hEcEventRegistration = NULL;
        }

        NvOdmOsThreadJoin(hOdm->hOffChipGpioThread);
        hOdm->hOffChipGpioThread = NULL;

        NvOdmOsSemaphoreDestroy(hOdm->hGpioEventSema);
        hOdm->hGpioEventSema = NULL;

        NvEcClose(hOdm->hEc);
        hOdm->hEc = NULL;

        NvOdmOsMutexDestroy(hOdm->hMutex);
        hOdm->hMutex = NULL;
#endif
        NvOdmOsFree(hOdm);
        hOdm = NULL;
    }
}

/* Returns the status for a specific power key */
NvBool NvOdmPowerKeysGetStatus(NvOdmPowerKeysHandle hOdm, NvOdmPowerKeysStatus *pKeyStatus)
{
    NvRmGpioPinState val = NvRmGpioPinState_Low;
    NvU32 i = 0;

    if (!pKeyStatus || !hOdm)
    {
        return NV_FALSE;
    }

    if (hOdm->GpioPinInfo)
    {
        for (i = 0; i < MAX_POWER_KEYS; i++)
        {
            if (hOdm->hPin[i])
            {
                pKeyStatus->Key = i + 1;
                NvRmGpioReadPins(hOdm->hGpio, &hOdm->hPin[i], &val, 1);
                if (val == hOdm->GpioPinInfo[i].activeState)
                    // LID closed
                    pKeyStatus->Status = NV_TRUE;
                else
                    // LID open
                    pKeyStatus->Status = NV_FALSE;
            }
#if ECI_ENABLED
            else
            {
                // get the value from off chip GPIO
                if (hOdm->APSleep)
                {
                    // LID closed
                    pKeyStatus->Key = NvOdmPowerKeys_LidStatus;
                    pKeyStatus->Status = NV_TRUE;

                    // clear status since it has been read
                    NvOdmOsMutexLock(hOdm->hMutex);
                    hOdm->APSleep = NV_FALSE;
                    NvOdmOsMutexUnlock(hOdm->hMutex);
                }
                else if (hOdm->APPowerDown)
                {
                    pKeyStatus->Key = NvOdmPowerKeys_PowerOnOff;
                    pKeyStatus->Status = NV_TRUE;

                    // clear status since it has been read
                    NvOdmOsMutexLock(hOdm->hMutex);
                    hOdm->APPowerDown = NV_FALSE;
                    NvOdmOsMutexUnlock(hOdm->hMutex);
                }
                else
                    pKeyStatus->Key = 0;
            }
#else
            else
                pKeyStatus->Key = 0;
#endif
            pKeyStatus++;
        }
    }
    else
        return NV_FALSE;

    return NV_TRUE;
}

/* Power handler */
NvBool NvOdmPowerKeysPowerHandler(NvOdmPowerKeysHandle hOdm, NvBool PowerDown)
{
#if ECI_ENABLED
    NvEcRequest Request = {0};
    NvEcResponse Response = {0};
    NvError e = NvError_Success;
#endif

    if (hOdm)
    {
        if (PowerDown)
        {
#if ECI_ENABLED
            NVODM_PRINTF(("NvOdmPowerKeysPowerHandler: Suspend Event Reporting\n"));
            /* disable automatic reporting of sleep/power-dn system events */
            Request.PacketType = NvEcPacketType_Request;
            Request.RequestType = NvEcRequestResponseType_System;
            Request.RequestSubtype = (NvEcRequestResponseSubtype)NvEcSystemSubtype_ConfigureEventReporting;
            Request.Payload[0] = NVEC_SYSTEM_REPORT_ENABLE_0_ACTION_DISABLE;
            Request.Payload[1] = hOdm->OffChipStateReq;
            Request.Payload[2] = 0;
            Request.Payload[3] = 0;
            Request.Payload[4] = 0;
            Request.NumPayloadBytes = CONFIGURE_REQ_PAYLOAD_SIZE; // number of bytes for Configure Request

            e = NvEcSendRequest(hOdm->hEc, &Request, &Response, sizeof(Request), sizeof(Response));
            if (e != NvError_Success)
            {
                return NV_FALSE;
            }

            /* check if command passed */
            if (Response.Status != NvEcStatus_Success)
            {
                return NV_FALSE;
            }
#endif
        }
        else
        {
#if ECI_ENABLED
            NVODM_PRINTF(("NvOdmPowerKeysPowerHandler: Resume Event Reporting\n"));
            /* configure automatic reporting of sleep/power-dn system events */
            Request.PacketType = NvEcPacketType_Request;
            Request.RequestType = NvEcRequestResponseType_System;
            Request.RequestSubtype = (NvEcRequestResponseSubtype)NvEcSystemSubtype_ConfigureEventReporting;
            Request.Payload[0] = NVEC_SYSTEM_REPORT_ENABLE_0_ACTION_ENABLE;
            Request.Payload[1] = hOdm->OffChipStateReq;
            Request.Payload[2] = 0;
            Request.Payload[3] = 0;
            Request.Payload[4] = 0;
            Request.NumPayloadBytes = CONFIGURE_REQ_PAYLOAD_SIZE; // number of bytes for Configure Request

            e = NvEcSendRequest(hOdm->hEc, &Request, &Response, sizeof(Request), sizeof(Response));
            if (e != NvError_Success)
            {
                return NV_FALSE;
            }

            /* check if command passed */
            if (Response.Status != NvEcStatus_Success)
            {
                return NV_FALSE;
            }
#endif
        }
    }

    // power handler should never fail
    return NV_TRUE;
}

