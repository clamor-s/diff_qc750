/*
 * Copyright (c) 2007 - 2011 NVIDIA Corporation.  All rights reserved.
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

#if TARGET_SOC_t30 == 1
  #include "arusb_otg.h"
  #include "nvboot_clocks.h"
  #include "nvboot_clocks_int.h"
  #include "nvboot_hardware_access_int.h"
  #include "nvboot_reset_int.h"
  #include "nvboot_usbf_int.h"
  #include "usbf/nvboot_usbf_hw.h"
  #include "nvboot_util_int.h"
  #include "nvrm_drf.h"
  #include "arclk_rst.h"
  #include "tio_local.h"
#else
  #include "ap15/arusb_otg.h"
  #include "ap15/include/nvboot_error.h"
  #include "ap15/nvboot_clocks.h"
  #include "ap15/include/nvboot_clocks_int.h"
  #include "ap15/include/nvboot_hardware_access_int.h"
  #include "ap15/include/nvboot_reset_int.h"
  #include "ap15/include/nvboot_usbf_int.h"
  #include "ap15/usbf/nvboot_usbf_local.h"
  #include "ap15/include/nvboot_util_int.h"
  #include "nvrm_drf.h"
  #include "ap15/arclk_rst.h"
  #include "tio_local.h"
#endif

#define NV_ADDRESS_MAP_USB_BASE             0xC5000000
#define NV_ADDRESS_MAP_PPSB_CLK_RST_BASE    0x60006000

#undef USB_REG_RD
#define USB_REG_RD(reg)\
    NV_READ32(NV_ADDRESS_MAP_USB_BASE + USB2_CONTROLLER_USB2D_##reg##_0)

#undef USB_REG_WR
#define USB_REG_WR(reg, data)\
    NV_WRITE32((NV_ADDRESS_MAP_USB_BASE + USB2_CONTROLLER_USB2D_##reg##_0), data)

//###########################################################################
//############################### DEFINES ###################################
//###########################################################################


#define NV_TIO_USB_RC_BUFFER_IDX  0
#define NV_TIO_USB_TX_BUFFER_IDX  1

#define NV_TIO_WAIT_AFTER_CLOSE   200 // 200

#if 0
#define DBUI(x) NvTrAp15SendDebugMessage(x)
#else
#define DBUI(x)
#endif
#if 0
#define DBU(x) NvTrAp15SendDebugMessage(x)
#else
#define DBU(x)
#endif

//###########################################################################
//############################### TYPEDEFS ##################################
//###########################################################################

typedef enum NvTrAp15UsbReadStateEnum
{
    NvTrAp15UsbReadState_idle,
    NvTrAp15UsbReadState_pending,
} NvTrAp15UsbReadState;

typedef struct NvTrAp15UsbStateRec
{
    NvU8  isInit;           
    NvU8  isCableConnected; 
    NvU8  isOpen;
    NvU8  isSmallSegment;   // first seg. is always small, last one may be small

    NvU32 bufferIndex;      // index of USB receive buffer
    NvU32 inReadBuffer;     // data already in receive buffer
    NvU8* inReadBufferStart;// pointer to start of those data

    NvTrAp15UsbReadState readState;
} NvTrAp15UsbState;

//###########################################################################
//############################### GLOBALS ###################################
//###########################################################################

/* Read and write must use separate buffers */
__align(4096) NvU8 gs_readBufferMemory[NV_TIO_USB_MAX_SEGMENT_SIZE];
__align(4096) NvU8 gs_writeBufferMemory[NV_TIO_USB_MAX_SEGMENT_SIZE];

/* Pointers to a pair of 4k aligned buffers */
static NvU8* gs_buffer[2] = {
    &gs_readBufferMemory[0],
    &gs_writeBufferMemory[0]
};

// Notice that this sets isInit, isCableConnected, and all other state to zero.
static NvTrAp15UsbState gs_UsbState = { 0 }; 

// See if the header is "small enough". If not, the variable holding the header
// should not be allocated on stack (e.g. declare it static).
//NV_CT_ASSERT(sizeof(NvTioUsbMsgHeader)<=8);

//###########################################################################
//############################### CODE ######################################
//###########################################################################

// XXX
// This function shouldn't have to poke clock registers directly
#define CLK_RST_BASE NV_ADDRESS_MAP_PPSB_CLK_RST_BASE
void
HackBoostSysClk()
{
    NvU32 reg;
    
    // enable PLLP if it's not already running.
    reg = NV_READ32( CLK_RST_BASE + CLK_RST_CONTROLLER_PLLP_BASE_0 );
    if ( !NV_DRF_VAL( CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, reg ) )
    {
        reg |= NV_DRF_DEF( CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, ENABLE);
        NV_WRITE32( CLK_RST_BASE + CLK_RST_CONTROLLER_PLLP_BASE_0, reg);
    }

    // wait until PLLP is stable
    NvBootUtilWaitUS(NVBOOT_CLOCKS_PLL_STABILIZATION_DELAY) ;
    NV_WRITE32( CLK_RST_BASE + CLK_RST_CONTROLLER_PLLP_BASE_0,
                reg & ~CLK_RST_CONTROLLER_PLLP_BASE_0_PLLP_BYPASS_FIELD
                | NV_DRF_DEF( CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, DISABLE ) );

    // bump sys clock to 108MHz ( PLLP_out4 = 432MHz / 4 )
    reg = NV_DRF_DEF( CLK_RST_CONTROLLER, SCLK_BURST_POLICY,
                      SYS_STATE, RUN )
        | NV_DRF_DEF( CLK_RST_CONTROLLER, SCLK_BURST_POLICY,
                      SWAKEUP_RUN_SOURCE, PLLP_OUT4 );
    NV_WRITE32( CLK_RST_BASE + CLK_RST_CONTROLLER_SCLK_BURST_POLICY_0, reg );    }

/**
* @result NV_TRUE The USB is set up correctly and is ready to
* transmit and receive messages.
* @param pUsbBuffer pointer to a memory buffer into which data to the host 
*        will be stored. This buffer must be 4K bytes in IRAM and must be
*        aligned to 4K bytes. Otherwise it will assert.
* @result NV_FALSE The USB failed to set up correctly.
*/
static NvBool NvTrAp15UsbSetup(NvU8 *pUsbBuffer)
{
    int timeout;

    if (!gs_UsbState.isInit)
    {
        // bump AVP to 108 MHz
        // USB can lock up the chip if system clock isn't running fast enough
        HackBoostSysClk();

        // Initialize the USB controller
        NvBootUsbfInit();

        gs_UsbState.isInit = 1;
    }

    for (timeout=50; timeout; timeout--)        // 20 is too short time
    {
        // Look for cable connection
        if (NvBootUsbfIsCableConnected())
        {
            // Start enumeration
            if (NvBootUsbfStartEnumeration(pUsbBuffer) == NvBootError_Success)
            {
                // Success fully enumerated
                return NV_TRUE;
            }
            else
            {
                // Failed to enumerate.
                return NV_FALSE;
            }
        }
        NvOsWaitUS(1000);
    }

    // Cable not connected
    return NV_FALSE;
}

static NvError NvTrAp15UsbWriteSegment(
                                       const void *ptr,
                                       NvU32 payload
#if NV_TIO_USB_USE_HEADERS
                                       ,NvTioUsbMsgHeader* msgHeader
#endif
                                       )
{
    NvU8* dst = gs_buffer[NV_TIO_USB_TX_BUFFER_IDX];
    NvU32 bytes2Send;
    NvBootUsbfEpStatus EpStatus;

#if NV_TIO_USB_USE_HEADERS
    bytes2Send = payload <= NV_TIO_USB_SMALL_SEGMENT_PAYLOAD
                                        ? NV_TIO_USB_SMALL_SEGMENT_SIZE
                                        : NV_TIO_USB_LARGE_SEGMENT_SIZE;
    msgHeader->payload = payload;
    NvOsMemcpy(dst, msgHeader, sizeof(NvTioUsbMsgHeader));
    dst += sizeof(NvTioUsbMsgHeader);
#else
    bytes2Send = payload;
    if (bytes2Send>NV_TIO_USB_MAX_SEGMENT_SIZE)
        return DBERR(NvError_InsufficientMemory);
#endif

    NvOsMemcpy(dst, ptr, payload);
    NvBootUsbfTransmitStart(gs_buffer[NV_TIO_USB_TX_BUFFER_IDX], bytes2Send); 
    do {
        EpStatus = NvBootUsbfQueryEpStatus(NvBootUsbfEndPoint_BulkIn);
    } while (EpStatus == NvBootUsbfEpStatus_TxfrActive);

    if (EpStatus == NvBootUsbfEpStatus_TxfrComplete)
    {
        if (NvBootUsbfGetBytesTransmitted() != bytes2Send)
            return DBERR(NvError_FileWriteFailed);
    }
    else
    {
        // Look for cable connection
        if (NvBootUsbfIsCableConnected())
        {
            // if cable is present then Stall out endpoint saying host
            // not send any more data to us.
            NvBootUsbfEpSetStalledState(NvBootUsbfEndPoint_BulkOut, NV_TRUE);
            return DBERR(NvError_UsbfEpStalled);
        }
        gs_UsbState.isCableConnected = 0;
        return DBERR(NvError_UsbfCableDisConnected);
    }

    return NvSuccess;
}

#if NV_TIO_USB_USE_HEADERS
static void NvTrAp15SendDebugMessage(char* msg)
{
    NvTioUsbMsgHeader msgHeader;

    NvOsMemset(&msgHeader, 0, sizeof(NvTioUsbMsgHeader));
    msgHeader.isDebug = NV_TRUE;
    NvTrAp15UsbWriteSegment(msg, NvOsStrlen(msg)+1, &msgHeader);
}
#endif

static NvError NvTrAp15UsbReadSegment(
                                      NvU32* buf_count,
                                      NvU32  timeout_msec)
{
    NvBootUsbfEpStatus EpStatus;
    NvU32 t0 = NvOsGetTimeMS();
    NvU32 cnt=0;

    if (gs_UsbState.readState == NvTrAp15UsbReadState_idle)
    {
        DBU(("NvTrAp15UsbReadSegment: new read"));
        NvBootUsbfReceiveStart(gs_buffer[NV_TIO_USB_RC_BUFFER_IDX], *buf_count);
    }
    else
    {
        DBU(("NvTrAp15UsbReadSegment: read pending"));
    }

    gs_UsbState.readState = NvTrAp15UsbReadState_pending;

    for (;;)
    {
        EpStatus = NvBootUsbfQueryEpStatus(NvBootUsbfEndPoint_BulkOut);
        if (EpStatus != NvBootUsbfEpStatus_TxfrActive)
            break;

        DBUI(("NvTrAp15UsbReadSegment: looping"));
        // Timeout immediately
        if (!timeout_msec)
        {
            DBUI(("NvTrAp15UsbReadSegment: timeout immediately"));
            return DBERR(NvError_Timeout);
        }

        // Timeout never
        if (timeout_msec == NV_WAIT_INFINITE)
            continue;

        // timeout_msec
        if (cnt++ > 100) {
            NvU32 d = NvOsGetTimeMS() - t0;
            if (d >= timeout_msec)
                return DBERR(NvError_Timeout);
            timeout_msec -= d;
            t0 += d;
            cnt = 0;
        }
    }

    gs_UsbState.readState = NvTrAp15UsbReadState_idle;

    if (EpStatus == NvBootUsbfEpStatus_TxfrComplete)
    {
        *buf_count = NvBootUsbfGetBytesReceived();
        DBU(("NvTrAp15UsbReadSegment: off loop complete"));
    }
    else
    {   
        if (EpStatus == NvBootUsbfEpStatus_TxfrIdle)
            DBUI(("NvTrAp15UsbReadSegment: off loop idle"));
        else if (EpStatus == NvBootUsbfEpStatus_TxfrFail)
            DBUI(("NvTrAp15UsbReadSegment: off loop fail"));
        else if (EpStatus == NvBootUsbfEpStatus_Stalled)
            DBUI(("NvTrAp15UsbReadSegment: off loop stalled"));
        else if (EpStatus == NvBootUsbfEpStatus_NotConfigured)
            DBUI(("NvTrAp15UsbReadSegment: off loop not configured"));

        return DBERR(NvError_UsbfTxfrFail);
    }

    return NvSuccess;
}

//===========================================================================
// NvTrAp15UsbCheckPath() - check filename to see if it is a usb port
//===========================================================================
static NvError NvTrAp15UsbCheckPath(const char *path)
{
    //
    // return NvSuccess if path is a usb port
    //
    if (!NvTioOsStrncmp(path, "usb:", 4))
        return NvSuccess;
    if (!NvTioOsStrncmp(path, "debug:", 6))
        return NvSuccess;
    return DBERR(NvError_BadValue);
}

//===========================================================================
// NvTrAp15UsbClose()
//===========================================================================
void NvBootUsbfClose(void)
{  
    // Clear device address
    USB_REG_WR(PERIODICLISTBASE, USB_DEF_RESET_VAL(PERIODICLISTBASE));
    // Clear setup token, by reading and wrting back the same value
    USB_REG_WR(ENDPTSETUPSTAT, USB_REG_RD(ENDPTSETUPSTAT));
    // Clear endpoint complete status bits.by reading and writing back
    USB_REG_WR(ENDPTCOMPLETE, USB_REG_RD(ENDPTCOMPLETE));

    // STOP the USB controller
    USB_REG_UPDATE_DEF(USBCMD, RS, STOP);
    // Set the USB mode to the IDLE before reset
    USB_REG_UPDATE_DEF(USBMODE, CM, IDLE);

    // Disable all USB interrupts
    USB_REG_WR(USBINTR, USB_DEF_RESET_VAL(USBINTR));
    USB_REG_WR(OTGSC, USB_DEF_RESET_VAL(OTGSC));

    // Reset the USB controller
    NvBootResetSetEnable(NvBootResetDeviceId_UsbId, NV_TRUE);
    NvBootResetSetEnable(NvBootResetDeviceId_UsbId, NV_FALSE);

    // Disable clock to the USB controller
    NvBootClocksSetEnable(NvBootClocksClockId_UsbId, NV_FALSE);

    // Stop PLLU clock
    NvBootClocksStopPll(NvBootClocksPllId_PllU);

    // Wait to provide time for the host to drop its connection.
    NvOsSleepMS(NV_TIO_WAIT_AFTER_CLOSE);
}

static void NvTrAp15UsbClose(NvTioStream *stream)
{
    // TODO: cancel all pending transfers
    NvBootUsbfClose();

    gs_UsbState.isOpen = 0;

    // Wait for 1s to give the host a chance to remove the device before
    // subsequent USB operations.
    // do not use NvOsSleepMS(1000), that will cause AOS to suspend !!!
    NvOsWaitUS(1000000);
}

//===========================================================================
// NvTrAp15UsbFopen()
//===========================================================================
static NvError NvTrAp15UsbFopen(
                    const char *path,
                    NvU32 flags,
                    NvTioStream *stream)
{
    if (gs_UsbState.isOpen)
        return NvSuccess;

    gs_UsbState.bufferIndex = NV_TIO_USB_RC_BUFFER_IDX;

    // Check the cable connection for very first time
    if (!gs_UsbState.isCableConnected)
        gs_UsbState.isCableConnected =
            NvTrAp15UsbSetup(gs_buffer[gs_UsbState.bufferIndex]);

    if (!gs_UsbState.isCableConnected)
        return DBERR(NvError_UsbfCableDisConnected);
    
    gs_UsbState.isOpen = 1;
    gs_UsbState.inReadBuffer = 0;

    gs_UsbState.isSmallSegment = NV_TRUE;
    gs_UsbState.readState = NvTrAp15UsbReadState_idle;

    DBU(("NvTrAp15UsbFopen"));

    return NvSuccess;
}

//===========================================================================
// NvTrAp15UsbFwrite()
//===========================================================================
static NvError NvTrAp15UsbFwrite(
                    NvTioStream *stream,
                    const void *buf,
                    size_t size)
{
    NvError result;
    NvU8* src = (NvU8*)buf;
#if NV_TIO_USB_USE_HEADERS
    NvU8 isSmallSegment = NV_TRUE;
    NvTioUsbMsgHeader msgHeader;

    NvOsMemset(&msgHeader, 0, sizeof(NvTioUsbMsgHeader));
#endif

    while (size)
    {
        NvU32 payload;

        DBU(("NvTrAp15UsbFwrite: call NvTrAp15UsbWriteSegment\n"));
#if NV_TIO_USB_USE_HEADERS
        payload = isSmallSegment ? NV_TIO_USB_SMALL_SEGMENT_PAYLOAD
                                 : NV_TIO_USB_LARGE_SEGMENT_PAYLOAD;
        payload = NV_MIN(size, payload);

        msgHeader.isDebug = NV_FALSE;
        msgHeader.isNextSegmentSmall =
                (size - msgHeader.payload <= NV_TIO_USB_SMALL_SEGMENT_PAYLOAD);
        isSmallSegment = msgHeader.isNextSegmentSmall;
        result = NvTrAp15UsbWriteSegment(src, payload, &msgHeader);
#else
        payload = NV_MIN(size, 64);
        result = NvTrAp15UsbWriteSegment(src, payload);
#endif

        if (result)
            return DBERR(result);
        src += payload;
        size -= payload;
    }

    DBU(("NvTrAp15UsbFwrite: return success\n"));
    return NvSuccess;
}

//===========================================================================
// NvTrAp15UsbFread()
//===========================================================================
static NvError NvTrAp15UsbFread(
                    NvTioStream *stream,
                    void        *buf,
                    size_t       buf_count,
                    size_t      *recv_count,
                    NvU32        timeout_msec)
{
    NvError err;
    NvU8* dst = (NvU8*)buf;

    *recv_count = 0;

    while (buf_count)
    {
        NvU32 bytes2Receive;

        if (gs_UsbState.inReadBuffer)
        {
            NvU32 bytesToCopy = NV_MIN(buf_count, gs_UsbState.inReadBuffer);
            DBU(("NvTrAp15UsbFread: copy from segment"));
            NvOsMemcpy(dst, gs_UsbState.inReadBufferStart, bytesToCopy);
            *recv_count += bytesToCopy;
            dst += bytesToCopy;
            buf_count -= bytesToCopy;
            gs_UsbState.inReadBufferStart += bytesToCopy;
            gs_UsbState.inReadBuffer -= bytesToCopy;
        }

        // FIXME - check logic; may need to indicate last segment in a message
        //  to avoid need to timeout, because NvTio tries to fill the buffer.
        //  Setting timeout to 0 if something was already received should do
        //  the trick
        if (!buf_count)
        {
            DBU(("NvTrAp15UsbFread: copied, returning"));
            break;
        }
        if (*recv_count)
            timeout_msec = 0;

#if NV_TIO_USB_USE_HEADERS
        bytes2Receive = gs_UsbState.isSmallSegment
                            ? NV_TIO_USB_SMALL_SEGMENT_SIZE
                            : NV_TIO_USB_LARGE_SEGMENT_SIZE;
#else
        bytes2Receive = NV_TIO_USB_MAX_SEGMENT_SIZE;
#endif

        DBU(("NvTrAp15UsbFread: segment read start"));
        err = NvTrAp15UsbReadSegment(&bytes2Receive,
                                     timeout_msec);
        switch (err)
        {
            case NvError_Timeout: DBU(("NvError_Timeout")); break;
            case NvError_UsbfTxfrFail: DBU(("NvError_UsbfTxfrFail")); break;
            case NvSuccess: DBU(("NvSuccess")); break;
            default: DBU(("UNKNOWN!")); break;
        }
        if (err)
        {
            if (err == NvError_Timeout && *recv_count)
                return NvSuccess;
            else
                return DBERR(err);
        }

        DBU(("NvTrAp15UsbFread: setup new segment"));

#if NV_TIO_USB_USE_HEADERS
        {
            NvTioUsbMsgHeader* msgHeader;
            
            msgHeader = (NvTioUsbMsgHeader*)gs_buffer[NV_TIO_USB_RC_BUFFER_IDX];
            gs_UsbState.inReadBufferStart = gs_buffer[NV_TIO_USB_RC_BUFFER_IDX]
                                                    + sizeof(NvTioUsbMsgHeader);
            gs_UsbState.inReadBuffer = msgHeader->payload;
            gs_UsbState.isSmallSegment = msgHeader->isNextSegmentSmall;
        }
#else
        gs_UsbState.inReadBufferStart = gs_buffer[NV_TIO_USB_RC_BUFFER_IDX];
        gs_UsbState.inReadBuffer = bytes2Receive;
#endif
    }

    return NvSuccess;
}

//===========================================================================
// NvTioRegisterUsb()
//===========================================================================
void NvTioRegisterUsb(void)
{
    static NvTioStreamOps ops = {0};

    //defer the hardware init until the first fopen

    if (!ops.sopCheckPath) {
        ops.sopName        = "Ap15_usb";
        ops.sopCheckPath   = NvTrAp15UsbCheckPath;
        ops.sopFopen       = NvTrAp15UsbFopen;
        ops.sopFread       = NvTrAp15UsbFread;
        ops.sopFwrite      = NvTrAp15UsbFwrite;
        ops.sopClose       = NvTrAp15UsbClose;

        NvTioRegisterOps(&ops);
    }
}
