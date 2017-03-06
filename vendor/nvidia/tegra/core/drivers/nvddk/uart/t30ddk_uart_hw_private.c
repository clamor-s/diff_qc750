/*
 * Copyright (c) 2007 - 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * @brief <b>nVIDIA Driver Development Kit:
 *           Private functions for the uart hw access</b>
 *
 * @b Description:  Implements  the private interfacing functions for t30 uart
 * hw interface.
 *
 */

#include "nvrm_drf.h"
#include "ddkuart_hw_private.h"
#include "nvrm_hardware_access.h"
#include "nvassert.h"

// hardware includes
#include "t30/aruart.h"

#define UART_REG_READ32(pUartHwRegsVirtBaseAdd, reg) \
        NV_READ32((pUartHwRegsVirtBaseAdd) + ((UART_##reg##_0)/4))

#define UART_REG_WRITE32(pUartHwRegsVirtBaseAdd, reg, val) \
    do { \
        NV_WRITE32((((pUartHwRegsVirtBaseAdd) + ((UART_##reg##_0)/4))), (val)); \
    } while (0)

/**
 * Reset the uart fifo register.
 */
static void
HwResetFifo(
    UartHwRegisters *pUartHwRegs,
    UartDataDirection FifoType)
{
    NvU32 FcrValue;

    // Get the Fcr register value.
    FcrValue = pUartHwRegs->FcrRegister;

    // Get the fifo type whether receive and transmit fifo.
    // Reset the rx or tx fifo.
    if (FifoType & UartDataDirection_Receive)
    {
        FcrValue = NV_FLD_SET_DRF_DEF(UART, IIR_FCR, RX_CLR, CLEAR, FcrValue);
        UART_REG_WRITE32(pUartHwRegs->pRegsVirtBaseAdd, IIR_FCR, FcrValue);
    }

    if (FifoType & UartDataDirection_Transmit)
    {
        FcrValue = NV_FLD_SET_DRF_DEF(UART, IIR_FCR, TX_CLR, CLEAR, FcrValue);
        UART_REG_WRITE32(pUartHwRegs->pRegsVirtBaseAdd, IIR_FCR, FcrValue);
    }
}


static NvBool HwIsBreakSignalDetected(UartHwRegisters *pUartHwRegs)
{
    NvBool IsBreakDetected = NV_FALSE;
    NvU32 LsrValue = UART_REG_READ32(pUartHwRegs->pRegsVirtBaseAdd, LSR);
    if (LsrValue & NV_DRF_DEF(UART, LSR, BRK, BREAK))
    {
        IsBreakDetected =  NV_TRUE;

        // Again read the LSR and if there is fifo error with data then reset
        // here
        LsrValue = UART_REG_READ32(pUartHwRegs->pRegsVirtBaseAdd, LSR);
        if (LsrValue & NV_DRF_DEF(UART, LSR, FIFOE, ERR))
        {
            if (LsrValue & NV_DRF_DEF(UART, LSR, RDR, DATA_IN_FIFO))
                return IsBreakDetected;

            // Fifo error without rx data
            HwResetFifo(pUartHwRegs, UartDataDirection_Receive);
        }
    }

    return IsBreakDetected;
}


/**
 * Write into the transmit fifo register.
 * Returns the number of bytes written.
 */
static NvU32
HwPollingWriteInTransmitFifo(
    UartHwRegisters *pUartHwRegs,
    NvU8 *pTxBuff,
    NvU32 BytesRequested)
{
    NvU32 BytesToWrite = BytesRequested;
    NvU32 TransmitChar;
    NvU32 LsrValue;

    while (BytesToWrite)
    {
        LsrValue = UART_REG_READ32(pUartHwRegs->pRegsVirtBaseAdd, LSR);
        if ((LsrValue & (NV_DRF_DEF(UART, LSR, TX_FIFO_FULL, FULL))))
            break;
        TransmitChar = (NvU32)(*pTxBuff);
        UART_REG_WRITE32(pUartHwRegs->pRegsVirtBaseAdd, THR_DLAB_0, TransmitChar);
        pTxBuff++;
        BytesToWrite--;
    }
    return (BytesRequested - BytesToWrite);
}

/**
 * Write into the transmit fifo register.
 * Returns the number of bytes written.
 */
static NvU32
HwWriteInTransmitFifo(
    UartHwRegisters *pUartHwRegs,
    NvU8 *pTxBuff,
    NvU32 BytesRequested)
{
    NvU32 BytesToWrite;
    NvU32 TransmitChar;
    NvU32 DataWritten = 0;
    NvU32 BytesRemain;

    // This function is called once the trigger level interrupt is reach.
    // Write the data same as the trigger level without checking the fifo status.
    BytesToWrite = NV_MIN(BytesRequested, pUartHwRegs->TransmitTrigLevel);

    while (BytesToWrite)
    {
        TransmitChar = (NvU32)(*pTxBuff);
        UART_REG_WRITE32(pUartHwRegs->pRegsVirtBaseAdd, THR_DLAB_0, TransmitChar);
        pTxBuff++;
        BytesToWrite--;
        DataWritten++;
    }

    // Use polling to wrte if still some data ned to write.
    BytesRemain = BytesRequested - DataWritten;
    if (BytesRemain) {
        DataWritten += HwPollingWriteInTransmitFifo(pUartHwRegs, pTxBuff,
                                                                BytesRemain);
    }
    return DataWritten;
}

/**
 * Initialize the uart hw interface for the give major, minor version.
 */
void
NvDdkPrivUartInitUartHwInterface_1_3(
    UartHwInterface *pUartInterface,
    NvU32 Major,
    NvU32 Minor)
{
    if ((Major != 1) && (Minor != 3))
        return;

    pUartInterface->HwResetFifoFxn                  = HwResetFifo;
    pUartInterface->HwWriteInTransmitFifoFxn        = HwWriteInTransmitFifo;
    pUartInterface->HwPollingWriteInTransmitFifoFxn = HwPollingWriteInTransmitFifo;
    pUartInterface->HwIsBreakSignalDetectedFxn      = HwIsBreakSignalDetected;
}
