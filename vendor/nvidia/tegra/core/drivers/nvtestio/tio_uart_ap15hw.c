/*
 * Copyright (c) 2007 - 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

//###########################################################################
//############################### INCLUDES ##################################
//###########################################################################

#include "tio_local.h"
#include "arclk_rst.h"
#include "aruart.h"
#include "artimer.h"
#include "nvrm_hardware_access.h"
#include "nvrm_drf.h"
#include "nvboot_clocks.h"

#if NV_TIO_USE_NVRM
#include "nvodm_query_pinmux.h"
#include "nvrm_pinmux.h"
#endif

//###########################################################################
//############################### DEFINES ###################################
//###########################################################################

// OS clock tick
#define TIO_CLOCK_DEFAULT (5000)

// timer access
#define NV_TRN_TIMER_1_BASE (0x60005000)
#define NV_TRN_TIMER_1      (NV_TRN_TIMER_1_BASE + TIMER_TMR_PTV_0)
#define NV_ADDRESS_MAP_CLK_RST_BASE 0x60006000

//===========================================================================
// UART register access and defines
//===========================================================================
#define NV_TRN_UARTA_BASE   0x70006000
#define NV_TRN_UARTB_BASE   0x70006040
#define NV_TRN_UARTC_BASE   0x70006080

#define NV_TRN_UARTA_RST_BASE       0x60006004
#define NV_TRN_UARTA_CLK_BASE       0x60006010
#define NV_TRN_CLK_OSC_BASE         0x60006050
#define NV_TRN_PINMXA_BASE          0x70000080
#define NV_TRN_PINMXC_BASE          0x70000088
#define NV_TRN_PULLUP_CFG_BASE      0x700000AC
#define NV_TRN_PAD_CFG_BASE         0x700008AC
#define NV_TRN_UARTA_TRISTATE_BASE  0x70000018
#define NV_TRN_UARTA_CLK_SET_BASE   0x60006178

#define NV_TRN_UARTA_CLK_RST_OFFSET 0x40
#define NV_TRN_UARTA_CLK_SRC_PLLC0  0x00000000
#define NV_TRN_UARTA_CLK_SRC_CLKM   0xC0000000
#define NV_TRN_UARTA_PINMXA_SET     0x0A
#define NV_TRN_UARTA_PINMXC_SET     0x50000
#define NV_TRN_UARTA_TRISTATE_SET   0x40000
#define NV_TRN_PULLUP_CFG_SET       0x02
#define NV_TRN_PAD_CFG_SET          0x1008

//#define NV_READ32(x) (*(volatile NvU32 *)x)

#define PLLP_BY_HALF (432000/2)
//###########################################################################
//############################### GLOBALS ###################################
//###########################################################################

typedef enum
{
    NvTrn_RegOp_None=0,
    NvTrn_RegOp_And,
    NvTrn_RegOp_Or
}RegOperation;

#define NV_TIO_UART_HW_DEBUG 1
#if NV_TIO_UART_HW_DEBUG
volatile struct {
    volatile NvU8 ier;
    volatile NvU8 iir;
    volatile NvU8 lcr;
    volatile NvU8 mcr;
    volatile NvU8 lsr;
    volatile NvU8 msr;
    volatile NvU8 spr;
    volatile NvU8 csr;
    volatile NvU8 asr;
} uartregs;
#endif

#if NV_TIO_USE_NVRM
static NvRmDeviceHandle s_uart_rm;
static int s_uart_open_cnt = 0;
#endif

//###########################################################################
//############################### CODE ######################################
//###########################################################################

#if 0
//===========================================================================
// NvTrBootedFromUart() - true if booted from uart
//===========================================================================
static int NvTioBootedFromUart(void)
{
    // TODO: HACK: FIXME!!
#if 0
    volatile NvBootInfoTable *bit = (volatile NvBootInfoTable *)0x40000000;

    //NV_ASSERT(bit->BootRomVersion == 0x0001000);
    //NV_ASSERT(bit->DataVersion    == 0x0001000);
    //NV_ASSERT(bit->RcmVersion     == 0x0001000);

    if (bit->BootType != NvBootType_Uart)
        return 0;
    if (bit->PrimaryDevice != NvBootDevType_Irom)
        return 0;
    if (bit->SecondaryDevice != NvBootDevType_Uart)
        return 0;
#else
    volatile NvU32 *bit = (volatile NvU32 *)0x40000000;

    //NV_ASSERT(bit[0] == 0x0001000);
    //NV_ASSERT(bit[1] == 0x0001000);
    //NV_ASSERT(bit[2] == 0x0001000);

    if (bit[3] != 3)
        return 0;
    if (bit[4] != 5)
        return 0;
    if (bit[5] != 6)
        return 0;
#endif

    return 1;
}
#endif

//===========================================================================
// NvRegWrite32() - write 32 bit register
//===========================================================================
static NV_INLINE void NvRegWrite32(NvU32 reg, NvU32 value, RegOperation op)
{
    if(op == NvTrn_RegOp_None)
        *((volatile NvU32 *)(reg)) = value;
    else if(op == NvTrn_RegOp_And)
        *((volatile NvU32 *)(reg)) &= value;
    else if(op == NvTrn_RegOp_Or)
        *((volatile NvU32 *)(reg)) |= value;
}

//===========================================================================
// NvTrWrite8() - write 8 bit register
//===========================================================================
static NV_INLINE void NvTrWrite8(NvTioStream *stream, NvU32 offset, NvU8 value)
{
    *(stream->f.regs + offset) = value;
}

//===========================================================================
// NvTrRead8() - read 8 bit register
//===========================================================================
static NV_INLINE NvU8 NvTrRead8(NvTioStream *stream, NvU32 offset)
{
    return *(stream->f.regs + offset);
}

//===========================================================================
// NvTioKickDog() - disable WDT to prevent assert
//===========================================================================
static NV_INLINE void NvTioKickDog(void)
{
    NvU32 reg;
    reg =  NV_DRF_DEF( TIMER, TMR_PTV, EN, ENABLE)
         | NV_DRF_NUM( TIMER, TMR_PTV, TMR_PTV, TIO_CLOCK_DEFAULT )
         | NV_DRF_DEF( TIMER, TMR_PTV, PER, ENABLE);
    NvRegWrite32(NV_TRN_TIMER_1, reg, NvTrn_RegOp_None);
}

//===========================================================================
// NvTrUartTxReady() - true if not ready for next char
//===========================================================================
static NV_INLINE int NvTrUartTxReady(NvTioStream *stream)
{
    return (NvTrRead8(stream, UART_LSR_0) & UART_LSR_0_THRE_FIELD);
}

//===========================================================================
// NvTrUartRxReady() - true if character has been received
//===========================================================================
static NV_INLINE int NvTrUartRxReady(NvTioStream *stream)
{
#if NV_TIO_UART_HW_DEBUG
    uartregs.ier = NvTrRead8(stream, UART_IER_DLAB_0_0);
    uartregs.iir = NvTrRead8(stream, UART_IIR_FCR_0);
    uartregs.lcr = NvTrRead8(stream, UART_LCR_0);
    uartregs.mcr = NvTrRead8(stream, UART_MCR_0);
    uartregs.lsr = NvTrRead8(stream, UART_LSR_0);
    uartregs.msr = NvTrRead8(stream, UART_MSR_0);
    uartregs.spr = NvTrRead8(stream, UART_SPR_0);
    uartregs.csr = NvTrRead8(stream, UART_IRDA_CSR_0);
    uartregs.asr = NvTrRead8(stream, UART_ASR_0);
#endif

    return (NvTrRead8(stream, UART_LSR_0) & UART_LSR_0_RDR_FIELD);
}

//===========================================================================
// NvTrUartTx() - transmit a character
//===========================================================================
static NV_INLINE void NvTrUartTx(NvTioStream *stream, NvU8 c)
{
    NvTrWrite8(stream, UART_THR_DLAB_0_0, c);
}

//===========================================================================
// NvTrUartRx() - receive a character
//===========================================================================
static NV_INLINE NvU8 NvTrUartRx(NvTioStream *stream)
{
    return NvTrRead8(stream, UART_THR_DLAB_0_0);
}

//===========================================================================
// NvTrAp15UartCheckPath() - check filename to see if it is a uart port
//===========================================================================
static NvError NvTrAp15UartCheckPath(const char *path)
{
    //
    // return NvSuccess if path is a uart port
    //
    if (!NvTioOsStrncmp(path, "uart:", 5))
        return NvSuccess;
    if (!NvTioOsStrncmp(path, "debug:", 6))
        return NvSuccess;
    return DBERR(NvError_BadValue);
}

//===========================================================================
// NvTrAp15UartClose()
//===========================================================================
static void NvTrAp15UartClose(NvTioStream *stream)
{
#if NV_TIO_USE_NVRM
    //
    // release RM handle
    //
    if (--s_uart_open_cnt == 0) {
        NvRmClose(s_uart_rm);
        s_uart_rm = 0;
    }
#else
    // flush uart port
    while(NvTrUartRxReady(stream))
        NvTrUartRx(stream);
#endif
}

static void NvTrInitPllP(void)
{
    NvU32 osc;
    NvU32 reg;
    NvU8  divisor[4] = {0xD, 0x4, 0xC, 0x1A};
    NvU32 dividend[4] = {432, 90, 432, 432};

    osc = NV_READ32(NV_TRN_CLK_OSC_BASE) >> 30;
    reg = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, ENABLE)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, dividend[osc])
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, divisor[osc]);
    NV_WRITE32(NV_ADDRESS_MAP_CLK_RST_BASE+CLK_RST_CONTROLLER_PLLP_BASE_0, reg);
    reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, ENABLE, reg);
    NV_WRITE32(NV_ADDRESS_MAP_CLK_RST_BASE+CLK_RST_CONTROLLER_PLLP_BASE_0, reg);
    reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, DISABLE, reg);
    NV_WRITE32(NV_ADDRESS_MAP_CLK_RST_BASE+CLK_RST_CONTROLLER_PLLP_BASE_0, reg);
    /* wait until stable */
    //NvBootUtilWaitUS(NVBOOT_CLOCKS_PLL_STABILIZATION_DELAY) ;
    NvOsWaitUS(300);
}

static void NvTrAp15UartHwInit()
{
    NvU32 uartClkSrc = NV_TRN_UARTA_CLK_SRC_CLKM;
    /* Reset UART Controller */
    NvRegWrite32(NV_TRN_UARTA_RST_BASE,
                 NV_TRN_UARTA_CLK_RST_OFFSET,
                 NvTrn_RegOp_Or);
    NvRegWrite32(NV_TRN_UARTA_RST_BASE,
                 ~(NV_TRN_UARTA_CLK_RST_OFFSET),
                 NvTrn_RegOp_And);
    /* enable PllP for UARTA */
    if (NvTioGlob.uartBaud == 115200)
    {
        NvTrInitPllP();
        uartClkSrc = NV_TRN_UARTA_CLK_SRC_PLLC0;
    }
    /* Configure Clock */
    NvRegWrite32(NV_TRN_UARTA_CLK_SET_BASE,
                 uartClkSrc,
                 NvTrn_RegOp_None);
    /* Enable Clock */
    NvRegWrite32(NV_TRN_UARTA_CLK_BASE,
                 NV_TRN_UARTA_CLK_RST_OFFSET,
                 NvTrn_RegOp_Or);
    /*Set Muxes and clear tristate*/
    NvRegWrite32(NV_TRN_PINMXA_BASE,
                 NV_TRN_UARTA_PINMXA_SET,
                 NvTrn_RegOp_Or);
    NvRegWrite32(NV_TRN_PINMXC_BASE,
                 NV_TRN_UARTA_PINMXC_SET,
                 NvTrn_RegOp_Or);
    NvRegWrite32(NV_TRN_UARTA_TRISTATE_BASE,
                 ~(NV_TRN_UARTA_TRISTATE_SET),
                 NvTrn_RegOp_And);
    /* set pullup and pad config */
    NvRegWrite32(NV_TRN_PAD_CFG_BASE,
                 NV_TRN_PAD_CFG_SET,
                 NvTrn_RegOp_Or);
    NvRegWrite32(NV_TRN_PULLUP_CFG_BASE,
                 NV_TRN_PULLUP_CFG_SET,
                 NvTrn_RegOp_Or);
}
//===========================================================================
// NvTrAp15UartFopen()
//===========================================================================
static NvError NvTrAp15UartFopen(
                    const char *path,
                    NvU32 flags,
                    NvTioStream *stream)
{
    NvU8 reg = 117; // this is for 115200 assuming 216M PllP0 is clock source
    NvU8 cnt = 0;
    NvU8 divisor[4] =  {0xE, 0x15, 0xD, 0x1C};
#if NV_TIO_USE_NVRM
    NvError err;

    //
    // get RM handle
    //
    if (s_uart_open_cnt==0 || s_uart_rm==0) {
        err = NvRmOpen(&s_uart_rm,0);
        if (err)
            return err;
        s_uart_open_cnt=1;
    }

#ifndef SET_KERNEL_PINMUX
     err = NvRmSetModuleTristate(s_uart_rm,
         NVRM_MODULE_ID(NvRmModuleID_Uart,0), NV_FALSE);

        if (err)
            return err;
#endif
#else
    NvTrAp15UartHwInit();
#endif




    // TODO: support access to other UARTs
    stream->f.regs = (unsigned char volatile *)NV_TRN_UARTA_BASE;



    //
    // FROM BOOTROM:
    //            UARTA UARTB
    // 0x70006000: 0x00         UART_THR_DLAB_0_0   data for Rx/Tx
    // 0x70006004: 0x00         UART_IER_DLAB_0_0   interrupt enable
    // 0x70006008: 0xc1         UART_IIR_FCR_0      interrupt ID
    // 0x7000600c: 0x03         UART_LCR_0          line control
    // 0x70006010: 0x02         UART_MCR_0          modem control
    // 0x70006014: 0x60         UART_LSR_0          line status
    // 0x70006018: 0xfb 0x11    UART_MSR_0          modem status
    // 0x7000601c: 0x00         UART_SPR_0          scratch pad
    // 0x70006020: 0x00         UART_IRDA_CSR_0     IrDA Pulse Coding
    // 0x70006024: 0x03         UART_ASR_0          auto sense baud
    //
    //  IER Bits r/w
    //    7:6 reserved
    //    5   interrupt enable for end of Rx data
    //    4   interrupt enable for Rx fifo timeout
    //    3   interrupt enable for MSI
    //    2   interrupt enable for RXS
    //    1   interrupt enable for THR
    //    0   interrupt enable for RHR
    //
    //  IIR Bits READ
    //    7:6 0=16450 mode    1=16550 mode
    //    5:4 reserved
    //    4   interrupt flag for Rx fifo
    //    3:1 priority pending
    //    0   ?pointer to ISR?
    //
    //  IIR Bits WRITE
    //    7:6 Rx Fifo full count trigger
    //              00 = cnt>=1
    //              01 = cnt>=4
    //              10 = cnt>=8
    //              11 = cnt>=12
    //    5:4 Tx Fifo full count trigger
    //              00 = cnt>=1
    //              01 = cnt>=4
    //              10 = cnt>=8
    //              11 = cnt>=12
    //    3   DMA - 0=no change
    //    2   TX_CLR  - write 1 to clear TX
    //    1   RX_CLR  - write 1 to clear RX
    //    0   1=enable fifo
    //
    //  LCR Bits r/w
    //    7   baud rate counter latch
    //    6   break control (0= do not transmit break)
    //    5   set parity:  1=parity      0=no parity
    //    4   even parity: 1=even parity 0=odd parity
    //    3   parity:      1=parity      0=no parity
    //    2   stop bits: 1=2 stop bits
    //    1:0 word length: 0=5  1=6  2=7  3=8
    //
    //  MCR Bits r/w
    //    7   reserved
    //    6   RTS HW flow control  0=disable   1=enable
    //    5   CTS HW flow control  0=disable   1=enable
    //    4   Loopback debug       0=disable   1=enable
    //    3   nOUT2 polarity
    //    2   nOUT1 polarity
    //    1   0=force RTS hi
    //    0   1=force DTR hi
    //
    if (NvTioGlob.uartBaud == 57600)
    {
        reg = NV_READ32(NV_TRN_CLK_OSC_BASE) >> 30;
        reg = divisor[reg];
    }
    NvTrWrite8(stream, UART_LCR_0,        0x80);
    NvTrWrite8(stream, UART_THR_DLAB_0_0, reg);
    NvTrWrite8(stream, UART_IER_DLAB_0_0, 0x00);
    NvTrWrite8(stream, UART_LCR_0,        0x00);
    NvTrWrite8(stream, UART_IIR_FCR_0,    0x37);
    NvTrWrite8(stream, UART_IER_DLAB_0_0, 0x00);
    NvTrWrite8(stream, UART_LCR_0,        0x03);  // 8N1
#if NV_TIO_UART_HW_FLOWCTRL
    //NvTrWrite8(stream, UART_MCR_0,        0x62);
    NvTrWrite8(stream, UART_MCR_0,        0x61);
#else
    NvTrWrite8(stream, UART_MCR_0,        0x02);
#endif
    NvTrWrite8(stream, UART_MSR_0,        0x00);
    NvTrWrite8(stream, UART_SPR_0,        0x00);
    NvTrWrite8(stream, UART_IRDA_CSR_0,   0x00);
    NvTrWrite8(stream, UART_ASR_0,        0x00);

    NvTrWrite8(stream, UART_IIR_FCR_0,    0x31);

    //
    // Flush any old characters out of the FIFO
    //
    while(NvTrUartRxReady(stream))
        (void)NvTrUartRx(stream);

#if NV_TIO_UART_CONNECT_SEND_CR
    //
    // Send initial carriage return to signal connection
    //
    {
        while(!NvTrUartTxReady(stream));
#if NV_TIO_CHAR_DELAY_US>0
        NvOsWaitUS(NV_TIO_CHAR_DELAY_US);
#endif
        NvTrUartTx(stream, '\r');
        while(!NvTrUartTxReady(stream));
#if NV_TIO_CHAR_DELAY_US>0
        NvOsWaitUS(NV_TIO_CHAR_DELAY_US);
#endif
        NvTrUartTx(stream, '\n');
    }
#endif

    return NvSuccess;
}

//===========================================================================
// NvTrAp15UartFwrite()
//===========================================================================
static NvError NvTrAp15UartFwrite(
                    NvTioStream *stream,
                    const void *ptr,
                    size_t size)
{
    const NvU8 *p = ptr;
    while(size--) {
        while(!NvTrUartTxReady(stream));
#if NV_TIO_CHAR_DELAY_US>0
        NvOsWaitUS(NV_TIO_CHAR_DELAY_US);
#endif
        NvTrUartTx(stream, *p);
        p++;
    }
    return NvSuccess;
}

#if 1
//===========================================================================
// NvTrAp15UartFread()
//===========================================================================
static NvError NvTrAp15UartFread(
                    NvTioStream *stream,
                    void        *buf,
                    size_t       buf_count,
                    size_t      *recv_count,
                    NvU32        timeout_msec)
{
    NvU8 *p   = buf;

    //
    // Timeout immediately
    //
    if (!timeout_msec) {
        while(buf_count--) {
            if (!NvTrUartRxReady(stream)) {
                if (buf != p)
                    goto done;
                *recv_count = 0;
                return DBERR(NvError_Timeout);
            }
            *(p++) = NvTrUartRx(stream);
        }

    //
    // Timeout never
    //
    } else if (timeout_msec == NV_WAIT_INFINITE) {
        while(buf_count--) {
            while(!NvTrUartRxReady(stream)) {
                if (buf != p)
                    goto done;
                NvTioKickDog();
            }
            *(p++) = NvTrUartRx(stream);
        }

    //
    // timeout_msec
    //
    } else {
        NvU32 t0 = NvOsGetTimeMS();
        NvU32 cnt=0;
        while(buf_count--) {
            while(!NvTrUartRxReady(stream)) {
                if (buf != p)
                    goto done;
                if (cnt++>100) {
                    NvU32 d = NvOsGetTimeMS() - t0;
                    if (d >= timeout_msec)
                        return DBERR(NvError_Timeout);
                    timeout_msec -= d;
                    t0 += d;
                    cnt = 0;
                }
                NvTioKickDog();
            }
            *(p++) = NvTrUartRx(stream);
        }
    }
done:
    *recv_count = p - (NvU8*)buf;
    return DBERR(NvSuccess);
}
#else
//===========================================================================
// NvTrAp15UartFread()
//===========================================================================
static NvError NvTrAp15UartFread(
                    NvTioStream *stream,
                    void        *buf,
                    size_t       buf_count,
                    size_t      *recv_count,
                    NvU32        timeout_msec)
{
    NvU8 *p   = buf;
    // TODO: FIXME: handle timeout_msec (only 0 and INFINITE work now)
    if (!timeout_msec) {
        if (!NvTrUartRxReady(stream)) {
            *recv_count = 0;
            return DBERR(NvError_Timeout);
        }
    }
    while(buf_count--) {
        while(!NvTrUartRxReady(stream)) {
            if (buf != p)
                goto done;
        }

        *(p++) = NvTrUartRx(stream);
    }
done:
    *recv_count = p - (NvU8*)buf;
    return DBERR(NvSuccess);
}
#endif

//===========================================================================
// NvTioRegisterUart()
//===========================================================================
void NvTioRegisterUart(void)
{
    static NvTioStreamOps ops = {0};


    if (!ops.sopCheckPath) {
#if NV_TIO_USE_NVRM
        ops.sopName        = "Ap15_uart_nvrm";
#else
        ops.sopName        = "Ap15_uart";
#endif
        ops.sopCheckPath   = NvTrAp15UartCheckPath;
        ops.sopFopen       = NvTrAp15UartFopen;
        ops.sopFread       = NvTrAp15UartFread;
        ops.sopFwrite      = NvTrAp15UartFwrite;
        ops.sopClose       = NvTrAp15UartClose;

        // TODO: FIXME: HACK!!! only allow UART connect if booted from UART
        //if (NvTioBootedFromUart()) {
            NvTioRegisterOps(&ops);
        //}
    }
}

