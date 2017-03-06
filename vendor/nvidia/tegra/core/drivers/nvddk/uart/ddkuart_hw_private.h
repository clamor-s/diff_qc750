/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
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
 *           Private functions for the uart Ddk driver</b>
 *
 * @b Description:  Defines the private interfacing functions for the uart
 * hw interface.
 *
 */

#ifndef INCLUDED_DDKUART_HW_PRIVATE_H
#define INCLUDED_DDKUART_HW_PRIVATE_H

/**
 * @defgroup nvddk_uart Universal Asynchronous Receiver Transmitter (UART)
 * Controller hw interface API
 *
 * This is the Universal Asynchronous Receiver Transmitter (UART) hw interface
 * controller api.
 *
 * @ingroup nvddk_modules
 * @{
 *
 */

#include "nvcommon.h"
#include "nvddk_uart.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Combines the definition of the uart register and modem signals.
 */
typedef struct UartHwRegistersRec
{
    // Channel instance Id.
    NvU32 ChannelId;
    
    // Virtual base address of the uart hw register.
    NvU32 *pRegsVirtBaseAdd;

    // Physical base address of the uart hw register.
    NvRmPhysAddr RegsPhyBaseAdd;

    // Register bank size.
    NvU32 BankSize;

    // Uart shadow registers to keep the s/w value of registers

    // Currently programmed Fcr register value.
    NvU32 FcrRegister;

    // Currently programmed Ier register value.
    NvU32 IerRegister;

    // Currently programmed Lcr register value.
    NvU32 LcrRegister;

    // Currently programmed Mcr register value.
    NvU32 McrRegister;

    // Currently programmed irda csr register value.
    NvU32 IrdaCsrRegister;

    // Currently programmed Dlm register value.
    NvU32 DlmRegister;

    // Currently programmed Dll register value.
    NvU32 DllRegister;

    // Tells whether the dma mode is selected or not.
    NvBool IsDmaMode;

    // Modem signal status, 1 on the particular location shows the state to high.
    NvU32 ModemSignalSatus;
    
    // Tells whether the Rts hw flow control is  supported or not
    NvBool IsRtsHwFlowSupported;

    // Tells whether the end of data interrupt is  supported or not
    NvBool IsEndOfDataIntSupport;

    // Store the RTS line status
    NvBool IsRtsActive;

    NvBool IsDtrActive;
    
    NvU32 TransmitTrigLevel;
} UartHwRegisters;

/**
 * Combines the uart interrupt sources.
 */
typedef enum
{
    // No Uart interrupt source.
    UartHwInterruptSource_None = 0x0,

    // Receive Uart interrupt source.
    UartHwInterruptSource_Receive,

    // Receive error Uart interrupt source.
    UartHwInterruptSource_ReceiveError,

    // Rx timeout interrupt source.
    UartHwInterruptSource_RxTimeout,

    // End of data interrupt source.
    UartHwInterruptSource_EndOfData,

    // Transmit interrupt source.
    UartHwInterruptSource_Transmit,

    // Modem control interrupt source.
    UartHwInterruptSource_ModemControl,

    UartHwInterruptSource_Force32 = 0x7FFFFFFF
} UartHwInterruptSource;

/**
 * Combines the uart hw data direction.
 */
typedef enum
{
    // Invalid fifo type.
    UartDataDirection_Invalid = 0x0,

    // Receive direction.
    UartDataDirection_Receive = 0x1,

    // Transmit direction.
    UartDataDirection_Transmit = 0x2,

    // Both, receive and transmit data direction.
    UartDataDirection_Both = 0x3,

    UartDataDirection_Force32 = 0x7FFFFFFF
} UartDataDirection;

typedef struct
{
    /**
     * Initialize the uart hw registers parameters. It does not write anything
     * on the controller register.
     */
    void 
    (* HwRegisterInitializeFxn)(
        NvU32 CommChannelId, 
        UartHwRegisters *pUartHwRegs);
        
    /**
     * Initialize the uart fifos.
     */
    void (* HwInitFifoFxn)(UartHwRegisters *pUartHwRegs);
    
    /**
     * Get the clock source frequency required for the given baud rate.
     */
    void
    (* HwGetReqdClockSourceFreqFxn)(
        NvU32 BaudRate,
        NvU32 *pMinClockFreqReqd,
        NvU32 *pClockFreqReqd,
        NvU32 *pMaxClockFreqReqd);
    
    /**
     * Set the baud rate of uart communication.
     */
    NvError
    (* HwSetBaudRateFxn)(
        UartHwRegisters *pUartHwRegs,
        NvU32 BaudRate,
        NvU32 ClockSourceFreq);
    /**
     * Set the parity of uart communication.
     */
    NvError
    (* HwSetParityBitFxn)(
        UartHwRegisters *pUartHwRegs,
        NvDdkUartParity ParityBit);
    
    /**
     * Set the data length of uart communication.
     */
    NvError
    (* HwSetDataLengthFxn)(
        UartHwRegisters *pUartHwRegs,
        NvU32 DataLength);
    
    /**
     * Set the stop bit of uart communication.
     */
    NvError (* HwSetStopBitFxn)(
        UartHwRegisters *pUartHwRegs,
        NvDdkUartStopBit StopBit);
    
    /**
     * Start/stop sending the break control signal.
     */
    void (* HwSetBreakControlSignalFxn)(
        UartHwRegisters *pUartHwRegs,
        NvBool IsStart);
    
    /**
     * Reset the uart fifos.
     */
    void
    (* HwResetFifoFxn)(
        UartHwRegisters *pUartHwRegs,
        UartDataDirection FifoType);
    
    /**
     * Set the fifo trigger level of uart fifo.
     */
    void
    (* HwSetFifoTriggerLevelFxn)(
        UartHwRegisters *pUartHwRegs,
        UartDataDirection FifoType,
        NvU32 TriggerLevel);
    
    /**
     * Set the dma mode operation of uart
     */
    void 
    (* HwSetDmaModeFxn)(
        UartHwRegisters *pUartHwRegs, 
        NvBool IsEnable);
    
    /**
     * Write into the transmit fifo register with the trigger level.
     * This function should be call when tx trigger level is reached.
     */
    NvU32
    (* HwWriteInTransmitFifoFxn)(
        UartHwRegisters *pUartHwRegs,
        NvU8 *pTxBuff,
        NvU32 BytesRequested);
    
    /**
     * Write into the transmit fifo register and check the fifo status before
     * writing into the tx fifo.
     * With every write, the fifo status will be checked.
     */
    NvU32
    (* HwPollingWriteInTransmitFifoFxn)(
        UartHwRegisters *pUartHwRegs,
        NvU8 *pTxBuff,
        NvU32 BytesRequested);
        
    /**
     * Check whether the Transmit fiof is empty or not.
     * Returns TRUE if the Tx fifo is empty otherwise returns FALSE.
     */
    NvBool (* HwIsTransmitFifoEmptyFxn)(UartHwRegisters *pUartHwRegs);
    
    /**
     * Check whether break condition is received or not.
     */
    NvBool (* HwIsBreakSignalDetectedFxn)(UartHwRegisters *pUartHwRegs);
    
    /**
     * Get the Rx fifo status whether data is available in the rx fifo or not.
     */
    NvBool (* HwIsDataAvailableInFifoFxn)(UartHwRegisters *pUartHwRegs);
    
    /**
     * Read the data from the receive fifo.
     */
    NvError
    (* HwReadFromReceiveFifoFxn)(
        UartHwRegisters *pUartHwRegs,
        NvU8 *pRxBuff,
        NvU32 MaxBytesRequested,
        NvU32 *bytesRead);
    
    /**
     * Clear all the uart interrupt.
     */
    void (* HwClearAllIntFxn)(UartHwRegisters *pUartHwRegs);
    
    /**
     * Configure the receive interrupt.
     */
    void (* HwSetReceiveInterruptFxn)(
        UartHwRegisters *pUartHwRegs,
        NvBool IsEnable,
        NvBool IsRxUsingApbDma);
    
    /**
     * Configure the transmit interrupt.
     */
    void (* HwSetTransmitInterruptFxn)(
        UartHwRegisters *pUartHwRegs,
        NvBool IsEnable);
    
    /**
     * Configure the modem control interrupt.
     */
    void (* HwSetModemControlInterruptFxn)(
        UartHwRegisters *pUartHwRegs,
        NvBool IsEnable);
    
    /**
     * Get the interrupt reason.
     */
    UartHwInterruptSource
    (* HwGetInterruptReasonFxn)(
        UartHwRegisters *pUartHwRegs);
    
    /**
     * Enable/disable the Irda coding.
     */
    void
    (* HwSetIrdaCodingFxn)(
        UartHwRegisters *pUartHwRegs,
        NvBool IsEnable);
    
    /**
     * Set the output modem signals to high or low.
     */
    void
    (* HwSetModemSignalFxn)(
        UartHwRegisters *pUartHwRegs,
        NvDdkUartSignalName SignalName,
        NvBool IsActive);
    
    /**
     * Get the CTS status from the line.
     */
    NvBool (* HwIsCtsActiveFxn)(UartHwRegisters *pUartHwRegs);
    
    /**
     * Read the modem signal level and update the state of the signal in the
     * uart hw register structure.
     */
    void (* HwUpdateModemSignalFxn)(UartHwRegisters *pUartHwRegs);
    
    /**
     * Enable/disable the hw flow control for transmitting the signal.
     * If it is enabled then it wil only transmit the data if CTS line is high.
     */
    void
    (* HwSetHWFlowControlFxn)(
        UartHwRegisters *pUartHwRegs,
        NvBool IsEnable);
    
    /**
     * Reconfigure the uart controller register. The shadow register content is
     * written into the uart controller register.
     */
    void (* HwReconfigureRegistersFxn)(UartHwRegisters *pUartHwRegs);

} UartHwInterface, *UartHwInterfaceHandle;


/**
 * Initialize the uart hw interface for the give major, minor version
 */
void 
NvDdkPrivUartInitUartHwInterface(
    UartHwInterface *pUartInterface,
    NvU32 Major,
    NvU32 Minor);
    
void 
NvDdkPrivUartInitUartHwInterface_1_3(
    UartHwInterface *pUartInterface,
    NvU32 Major,
    NvU32 Minor);


/** @}*/

#if defined(__cplusplus)
}
#endif

#endif  // INCLUDED_DDKUART_HW_PRIVATE_H
