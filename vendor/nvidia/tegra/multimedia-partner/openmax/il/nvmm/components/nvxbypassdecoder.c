/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All Rights Reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

/** nvxbypassdecoder.c */

/* =========================================================================
 *                     I N C L U D E S
 * ========================================================================= */

#include <OMX_Core.h>
#include "nvassert.h"
#include "common/NvxTrace.h"
#include "components/common/NvxComponent.h"
#include "components/common/NvxPort.h"
#include "nvmm/components/common/NvxCompReg.h"
#include "nvfxmath.h"
#include "NvxTrace.h"
#include "string.h"
#include "inttypes.h"

/* =========================================================================
 *                     D E F I N E S
 * ========================================================================= */
#define ERROR_OK                (0)
#define ERROR_BAD_SYNC          (-1)
#define ERROR_BAD_FRAME         (-2)

#define MAX_INPUT_BUFFERS   5
#define MAX_OUTPUT_BUFFERS  5

// For AC3 max frame size is 2^11 words = (2048*2)
// For DTS max frame size is 2^14. But sometimes frames come with extra padding
#define MIN_INPUT_BUFFERSIZE (16384*2)


// For AC3 2-Ch stream (1536*2*2)
// For DTS 2-Ch stream (128*32*2*2)

#define MIN_OUTPUT_BUFFERSIZE (128*32*2*2)

#define PREAMBLE_SIZE_BYTES 8

/* Number of internal frame buffers (AUDIO memory) */
static const int s_nvx_num_ports            = 2;
static const int s_nvx_port_input           = 0;
static const int s_nvx_port_output          = 1;

static OMX_ERRORTYPE NvxBypassDecoderAcquireResources(OMX_IN NvxComponent *pComponent);
static OMX_ERRORTYPE NvxBypassDecoderReleaseResources(OMX_IN NvxComponent *pComponent);

/* Bitstream State information */
typedef struct
{
    unsigned int   *wordPtr;           /* pointer to buffer where to write */
                                        /*   next byte                      */
    unsigned int   word;                /* next byte to write in buffer     */
    unsigned int   bit_count;           /* counts encoded bits              */
    unsigned int   word2;
    unsigned int   validbits;          /* number of valid bits in byte     */

} MOD_BUF_PTR;

/* Bypass Decoder State information */
typedef struct SNvxBypassDecoderData
{
    OMX_BOOL bInitialized;
    OMX_BOOL bOverrideTimestamp;

    MOD_BUF_PTR mbp;

    OMX_BOOL bFirstBuffer;
    OMX_BOOL bFrameOK;
    OMX_BOOL bSilenceOutput;

    OMX_AUDIO_CODINGTYPE inputCodingType;
    OMX_AUDIO_CODINGTYPE outputCodingType;
    OMX_AUDIO_CODINGTYPE newOutputCodingType;

    OMX_U32 nChannels;
    OMX_U32 nSampleRate;

    OMX_U32 outputBytes;
    OMX_U32 bytesConsumed;
    NvU64   timeStamp;

} SNvxBypassDecoderData;

/* =========================================================================
 *                     P R O T O T Y P E S
 * ========================================================================= */

static OMX_ERRORTYPE NvxBypassDecoderOpen( SNvxBypassDecoderData * pNvxBypassDecoder, NvxComponent *pComponent );
static OMX_ERRORTYPE NvxBypassDecoderClose( SNvxBypassDecoderData * pNvxBypassDecoder );

/* =========================================================================
 *                     F U N C T I O N S
 * ========================================================================= */


/* =========================================================================
 * Bit parsing functions
 * ========================================================================= */

static void init_read_mod_buf(MOD_BUF_PTR *mbp, unsigned int *buffer, int size);
static unsigned int read_mod_buf(MOD_BUF_PTR *mbp, unsigned int no_of_bits );

#define CBB_Get(mbp, bits)    (read_mod_buf(mbp, bits))

static void init_read_mod_buf(MOD_BUF_PTR *mbp, unsigned int *buffer, int size)
{
    mbp->validbits = 0;
    mbp->bit_count  = 0;
    mbp->word = 0;
    mbp->word2 = (*buffer << 24) | ((*buffer & 0xff00) << 8) | ((*buffer >> 8) & 0xff00) | (*buffer >> 24);
    mbp->wordPtr = buffer + 1;
}

/****************************************************************************
***
***    read_mod_buf()
***
***    reads 'no_of_bits' bits from the bitstream buffer 'mbp'
***    in the variable 'value'
***
***    M.Dietz     FhG IIS/INEL
***
****************************************************************************/

static unsigned int read_mod_buf(MOD_BUF_PTR *mbp, unsigned int no_of_bits )
{
    unsigned int value;
    unsigned int tmp;

    if (no_of_bits == 0)
    {
        return(0);
    }

    mbp->bit_count += no_of_bits;

    if(no_of_bits <= mbp->validbits)
    {
        value = (mbp->word >> (32 - no_of_bits));
        mbp->validbits -= no_of_bits ;
        mbp->word <<= no_of_bits;
        return (value);
    }

    no_of_bits = 32 - no_of_bits;
    value = (mbp->word | (mbp->word2 >> mbp->validbits)) >> no_of_bits;
    mbp->validbits += no_of_bits ;
    mbp->word = mbp->word2 << (32 -  mbp->validbits);

    tmp = *mbp->wordPtr ++;
    mbp->word2 = ((tmp << 24) | ((tmp & 0xff00)<<8) | ((tmp >> 8) & 0xff00) | (tmp >> 24));
    return(value);
}


/* =========================================================================
 * AC3 Parsing functions
 * ========================================================================= */


static const struct {
    const uint32_t bitrate;
    const uint32_t framesize[3];
} framesize_table[38] = {
    { 32,    { 64, 69, 96}},
    { 32,    { 64, 70, 96}},
    { 40,    { 80, 87, 120}},
    { 40,    { 80, 88, 120}},
    { 48,    { 96, 104, 144}},
    { 48,    { 96, 105, 144}},
    { 56,    { 112, 121, 168}},
    { 56,    { 112, 122, 168}},
    { 64,    { 128, 139, 192}},
    { 64,    { 128, 140, 192}},
    { 80,    { 160, 174, 240}},
    { 80,    { 160, 175, 240}},
    { 96,    { 192, 208, 288}},
    { 96,    { 192, 209, 288}},
    { 112,   { 224, 243, 336}},
    { 112,   { 224, 244, 336}},
    { 128,   { 256, 278, 384}},
    { 128,   { 256, 279, 384}},
    { 160,   { 320, 348, 480}},
    { 160,   { 320, 349, 480}},
    { 192,   { 384, 417, 576}},
    { 192,   { 384, 418, 576}},
    { 224,   { 448, 487, 672}},
    { 224,   { 448, 488, 672}},
    { 256,   { 512, 557, 768}},
    { 256,   { 512, 558, 768}},
    { 320,   { 640, 696, 960}},
    { 320,   { 640, 697, 960}},
    { 384,   { 768, 835, 1152}},
    { 384,   { 768, 836, 1152}},
    { 448,   { 896, 975, 1344}},
    { 448,   { 896, 976, 1344}},
    { 512,   { 1024, 1114, 1536}},
    { 512,   { 1024, 1115, 1536}},
    { 576,   { 1152, 1253, 1728}},
    { 576,   { 1152, 1254, 1728}},
    { 640,   { 1280, 1393, 1920}},
    { 640,   { 1280, 1394, 1920}}
};

static const uint32_t samplerates[4] = { 48000, 44100, 32000, 0 };
static const uint32_t acmod_channels[8] = { 2, 1, 2, 3, 3, 4, 4, 5 };
static const uint32_t numblks[4] = { 1, 2, 3, 6 };

static NvBool ParseAC3Frame(SNvxBypassDecoderData *pContext,
                            uint8_t* inptr,
                            size_t inputsize,
                            int32_t* framesize,
                            int32_t* rate,
                            int32_t* ch,
                            int32_t* blk,
                            int32_t* bsmod)
{
    int32_t lfe_on = 0;
    int32_t bmod = 0;
    int32_t acmod = 0;
    uint8_t fscod;
    uint8_t frmsizecod;

    init_read_mod_buf(&pContext->mbp , (unsigned int *)inptr, inputsize);

    CBB_Get(&pContext->mbp, 32); // skip processed data

    fscod = CBB_Get(&pContext->mbp, 2);
    frmsizecod = CBB_Get(&pContext->mbp, 6);

    if (fscod == 3)
    {
        NvOsDebugPrintf("BypassDecoder: invalid fscod %d\n" , fscod);
        return NV_FALSE;
    }

    if (frmsizecod >= 38)
    {
        NvOsDebugPrintf("BypassDecoder: invalid frmsizecod %d\n" , frmsizecod);
        return NV_FALSE;
    }

    CBB_Get(&pContext->mbp, 5); // skip bsid
    bmod = CBB_Get(&pContext->mbp, 3);

    acmod = CBB_Get(&pContext->mbp, 3);
    if ((acmod & 0x1) && (acmod != 0x1))
    {
        // if 3 front channels, skip 2 bits
        CBB_Get(&pContext->mbp, 2);
    }
    if (acmod & 0x4)
    {
        // if a surround channel exists, skip 2 bits
        CBB_Get(&pContext->mbp, 2);
    }
    if (acmod == 0x2)
    {
        // if in 2/0 mode, skip 2 bits
        CBB_Get(&pContext->mbp, 2);
    }

    lfe_on = CBB_Get(&pContext->mbp, 1);

    //NvOsDebugPrintf("AC3 frame: framesize %d bytes  rate %d  channels %d  nblocks %d \n",
    //   framesize_table[frmsizecod].framesize[fscod] * 2, samplerates[fscod],
    //   acmod_channels[acmod] + lfe_on, 6);

    if(framesize)
        *framesize = framesize_table[frmsizecod].framesize[fscod] * 2;

    if(rate)
        *rate = samplerates[fscod];

    if(ch)
        *ch = acmod_channels[acmod] + lfe_on;

    if(blk)
        *blk = 6;

    if (bsmod)
        *bsmod = bmod;

    return NV_TRUE;
}

static NvBool ParseEAC3Frame(SNvxBypassDecoderData *pContext,
                            uint8_t * inptr,
                            size_t inputsize,
                            int32_t* framesize,
                            int32_t* rate,
                            int32_t* ch,
                            int32_t* blk,
                            int32_t* bsmod)
{
    uint8_t lfe_on = 0;
    uint8_t acmod = 0;
    uint8_t fscod2, samplerate, nblks, numblkcode, strmtype;
    uint8_t lframesize, fscod;

    init_read_mod_buf(&pContext->mbp , (unsigned int *)inptr, inputsize);

    CBB_Get(&pContext->mbp, 16); // skip 16 bits
    strmtype = CBB_Get(&pContext->mbp, 2);

    if (strmtype == 3)
    {
        NvOsDebugPrintf("BypassDecoder: invalid sreamtype %d in EAC3\n", strmtype);
        return NV_FALSE;
    }

    if (strmtype == 1)
    {
        NvOsDebugPrintf("BypassDecoder: ignoring dependent sreamtype %d in EAC3\n", strmtype);
        return NV_FALSE;
    }

    CBB_Get(&pContext->mbp, 3);
    lframesize = CBB_Get(&pContext->mbp, 11);
    fscod = CBB_Get(&pContext->mbp, 2);
    if (fscod == 3)
    {
        fscod2 = CBB_Get(&pContext->mbp, 2);
        if (fscod2 == 3)
        {
            NvOsDebugPrintf("BypassDecoder: invalid fscod2 : %d\n", fscod2);
            return NV_FALSE;
        }
        samplerate = samplerates[fscod2] / 2;
        nblks = 6;
    }
    else
    {
        numblkcode = CBB_Get(&pContext->mbp, 2);
        samplerate = samplerates[fscod];
        nblks = numblks[numblkcode];
    }

    acmod = CBB_Get(&pContext->mbp, 3);
    lfe_on = CBB_Get(&pContext->mbp, 1);

    //NvOsDebugPrintf("BypassDecoder: EAC3 frame: framesize %d bytes  rate %d  channels %d  nblocks %d \n",
    //  (lframesize + 1) * 2 , samplerate, acmod_channels[acmod] + lfe_on, nblks);

    if (framesize)
        *framesize = (lframesize + 1) * 2;

    if (rate)
        *rate = samplerate;

    if (ch)
        *ch = acmod_channels[acmod] + lfe_on;

    if (blk)
        *blk = nblks;

    if (bsmod)
        *bsmod = 0; // TODO : update bsmod from stream

    return NV_TRUE;
}

static NvS32 parseAC3(SNvxBypassDecoderData *pContext, /* in/out */
                    const NvU8 *inptr, /* in */
                    NvS32 inputSize, /* in */
                    NvS32 *outputSize, /* out */
                    NvS32 *bsmod, /* out */
                    NvBool *needByteSwap, /* out */
                    NvS32 *rate, /* out */
                    NvS32 *channels, /* out */
                    NvS32 *framesize) /* out */
{
    NvS32 blk;
    NvU8 header[10];
    NvU8 bsid;
    uint16_t syncword;

    if (inputSize < 10)
    {
        NvOsDebugPrintf("BypassDecoder: incomplete AC3 frame\n");
        return ERROR_BAD_FRAME;
    }

    NvOsMemcpy(header, inptr, sizeof(header));

    syncword = (header[0] << 8) | header[1];

    if (syncword == 0x0B77)
    {
        *needByteSwap = NV_TRUE;
    }
    else if (syncword == 0x770B)
    {
        *needByteSwap = NV_FALSE;
    }
    else
    {
        NvOsDebugPrintf("BypassDecoder: invalid sync word 0x%x\n", syncword);
        return ERROR_BAD_SYNC;
    }

    if (!(*needByteSwap))
    {
        NvU8 temp;
        NvU32 i;
        for (i=0; i<sizeof(header); i+=2)
        {
            temp = header[i];
            header[i] = header[i+1];
            header[i+1] = temp;
        }
    }

    init_read_mod_buf(&pContext->mbp , (unsigned int *)header, inputSize);

    CBB_Get(&pContext->mbp, 16); // skip syncword
    CBB_Get(&pContext->mbp, 24); // skep CRC + fscod + frmsizecod
    bsid = CBB_Get(&pContext->mbp, 5);;

    if (bsid <= 10)
    {
        // parse ac3 frame header
        if(!ParseAC3Frame(pContext, header, inputSize, framesize, rate, channels, &blk, bsmod))
            return ERROR_BAD_FRAME;
    }
    else if (bsid <= 16)
    {
        // parse the eac3 frame header
        if(!ParseEAC3Frame(pContext, header, inputSize, framesize, rate, channels, &blk, bsmod))
            return ERROR_BAD_FRAME;
    }
    else
    {
        //NvOsDebugPrintf("BypassDecoder: Invalid bsid %d\n", bsid);
        return ERROR_BAD_FRAME;
    }

    *outputSize = 2 * 256 * blk * 2;

    return ERROR_OK;
}

static void packAC3Frame(const uint8_t *inpptr8,
                        uint8_t *outptr8,
                        uint32_t framesize,
                        uint32_t outputsize,
                        uint8_t bsmod,
                        NvBool byteSwap)
{
    uint16_t *outptr16 = (uint16_t *)outptr8;

    // AC3 packing as per IEC61937 2000
    outptr16[0] = 0xF872; // Pa
    outptr16[1] = 0x4E1F; // Pb

    // Pc = 0000 0000 0000 0001
    outptr16[2] = (bsmod << 9) | 0x1;

    // Pd = payload length in bits
    outptr16[3] = framesize * 8;

    if (byteSwap)
    {
        // copy encoded frame with byte swapping
        uint32_t i;
        uint8_t *dst = outptr8 + PREAMBLE_SIZE_BYTES;
        for (i=0; i<framesize; i+=2)
        {
            dst[i] = inpptr8[i+1];
            dst[i+1] = inpptr8[i];
        }
    }
    else
    {
        // copy encoded frame as it is
        NvOsMemcpy(outptr8 + PREAMBLE_SIZE_BYTES, inpptr8,framesize);
    }

    // stuff zeros
    NvOsMemset(outptr8 + PREAMBLE_SIZE_BYTES + framesize,
                0,
                outputsize - (PREAMBLE_SIZE_BYTES + framesize));
}


static void BypassProcessAC3(SNvxBypassDecoderData *pContext,
                                OMX_U8 *inptr8,
                                OMX_U32 inputSize,
                                OMX_U8 *outptr8,
                                OMX_U32 outputSize)
{
    NvS32 error;
    NvS32 totalOutputSize, bsmod, sampleRate, channels, framesize;
    NvBool needByteSwap;

    totalOutputSize = bsmod = sampleRate = channels = framesize = 0;
    needByteSwap = NV_FALSE;

    error = parseAC3(pContext,
                        inptr8,
                        inputSize,
                        &totalOutputSize,
                        &bsmod,
                        &needByteSwap,
                        &sampleRate,
                        &channels,
                        &framesize);
    if (ERROR_OK != error)
    {
        NvOsDebugPrintf("BypassDecoder: Invalid AC3 frame, skipping\n");
        pContext->outputBytes = 0;
        pContext->bytesConsumed = inputSize;
        pContext->bFrameOK = OMX_FALSE;
    }
    else
    {
        //NvOsDebugPrintf("BypassDecoder: found AC3 frame: framesize: %d rate: %d channels: %d bsmod: %d needByteSwap: %d",
        //            framesize, sampleRate, channels, bsmod, needByteSwap);

        if (OMX_AUDIO_CodingPCM != pContext->outputCodingType)
        {
            packAC3Frame(inptr8, outptr8, framesize, totalOutputSize, bsmod, needByteSwap);
        }
        else
        {
            NvOsMemset(outptr8, 0, totalOutputSize);
        }

        //NvOsDebugPrintf("BypassDecoder: pContext->OutputAudioFormat %d\n", pContext->OutputAudioFormat);

        pContext->nSampleRate = sampleRate;
        pContext->nChannels = 2;
        pContext->outputBytes = totalOutputSize;
        pContext->bytesConsumed = framesize;
        pContext->bFrameOK = OMX_TRUE;
    }
}


/* =========================================================================
 * DTS Parsing functions
 * ========================================================================= */

static const int dts_rate[16] =
{
  0,
  8000,
  16000,
  32000,
  64000,
  128000,
  11025,
  22050,
  44100,
  88200,
  176400,
  12000,
  24000,
  48000,
  96000,
  192000
};

static NvS32 parseDTS(SNvxBypassDecoderData *pContext, /* in/out */
                    const NvU8 *inptr, /* in */
                    NvS32 inputSize, /* in */
                    NvS32 *outputSize, /* out */
                    NvS32 *nsamples, /* out */
                    NvBool *needByteSwap, /* out */
                    NvBool *isFormat14bit, /* out */
                    NvS32 *rate, /* out */
                    NvS32 *channels, /* out */
                    NvS32 *framesize) /* out */
{
    NvU32 syncword, ftype, nblks, srate_code;
    NvU8 header[12];

    if (inputSize < 12)
    {
        NvOsDebugPrintf("BypassDecoder: incomplete DTS frame\n");
        return ERROR_BAD_FRAME;
    }

    NvOsMemcpy(header, inptr, sizeof(header));

    syncword = header[0] << 24 | header[1] << 16 |
                        header[2] << 8 | header[3];

    switch (syncword)
    {
    /* 14 bits LE */
    case 0xff1f00e8:
        /* frame type should be 1 (normal) */
        if ((header[4]&0xf0) != 0xf0 || header[5] != 0x07)
            return -1;
        *isFormat14bit = NV_TRUE;
        *needByteSwap = NV_FALSE;
        break;

    /* 14 bits BE */
    case 0x1fffe800:
        /* frame type should be 1 (normal) */
        if (header[4] != 0x07 || (header[5]&0xf0) != 0xf0)
            return -1;
        *isFormat14bit = NV_TRUE;
        *needByteSwap = NV_TRUE;
        break;

    /* 16 bits LE */
    case 0xfe7f0180:
        *isFormat14bit = NV_FALSE;
        *needByteSwap = NV_FALSE;
        break;

    /* 16 bits BE */
    case 0x7ffe8001:
        *isFormat14bit = NV_FALSE;
        *needByteSwap = NV_TRUE;
        break;

    default:
        NvOsDebugPrintf("BypassDecoder: invalid sync word 0x%x\n", syncword);
        return ERROR_BAD_SYNC;
    }

    if (!(*needByteSwap))
    {
        NvU8 temp;
        NvU32 i;
        for (i=0; i<sizeof(header); i+=2)
        {
            temp = header[i];
            header[i] = header[i+1];
            header[i+1] = temp;
        }
    }

    init_read_mod_buf(&pContext->mbp , (unsigned int *)header, inputSize);

    if (*isFormat14bit)
    {
        NvOsDebugPrintf("DTS: 14-bit format not supported\n");
        return ERROR_BAD_FRAME;
    }
    else
    {
        CBB_Get(&pContext->mbp, 32); // skip syncword
        ftype = CBB_Get(&pContext->mbp, 1); // frame type

        if (ftype != 1)
        {
            NvOsDebugPrintf("DTS: skipping termination frame\n");
            return ERROR_BAD_FRAME;
        }

        CBB_Get(&pContext->mbp, 5); // skip deficit sample count
        CBB_Get(&pContext->mbp, 1); // skip CRC bit

        nblks = CBB_Get(&pContext->mbp, 7);
        nblks++;
        if(nblks != 8 && nblks != 16 && nblks != 32 && nblks != 64 &&
            nblks != 128)
        {
            NvOsDebugPrintf("DTS: invalid nblks %d \n", nblks);
            return ERROR_BAD_FRAME;
        }
        *nsamples = nblks * 32;

        *framesize = CBB_Get(&pContext->mbp, 14);
        if (*framesize < 95 || *framesize > 8191)
        {
            NvOsDebugPrintf("DTS: invalid fsize %d \n", *framesize);
            return ERROR_BAD_FRAME;
        }
        (*framesize)++;

        if (*framesize > inputSize)
        {
            NvOsDebugPrintf("DTS: incomplete frame framesize %d inputsize %d\n",
                                *framesize, inputSize);
            return ERROR_BAD_FRAME;
        }

        CBB_Get(&pContext->mbp, 6); // skip AMODE bits

        srate_code = CBB_Get(&pContext->mbp, 4);

        if(srate_code >= 1 && srate_code <= 15)
            *rate = dts_rate[srate_code];
        else
            *rate = 0;

        *channels = 2;
    }

    *outputSize = (*nsamples) * 2 * 2;

    return ERROR_OK;
}

static void packDTSFrame(const uint8_t *inpptr8,
                        uint8_t *outptr8,
                        uint32_t framesize,
                        uint32_t outputsize,
                        int32_t nsamples,
                        NvBool byteSwap)
{
    uint8_t *dst = outptr8 + PREAMBLE_SIZE_BYTES;
    uint16_t *outptr16 = (uint16_t *)outptr8;

    // DTS packing as per IEC61937 2000
    outptr16[0] = 0xF872; // Pa
    outptr16[1] = 0x4E1F; // Pb

    // Pc
    switch(nsamples)
    {
    case 512:
        outptr16[2] = 0x000b;      /* DTS-1 (512-sample bursts) */
        break;
    case 1024:
        outptr16[2] = 0x000c;      /* DTS-2 (1024-sample bursts) */
        break;
    case 2048:
        outptr16[2] = 0x000d;      /* DTS-3 (2048-sample bursts) */
        break;
    default:
        NvOsDebugPrintf("DTS: %d sample bursts not supported\n", nsamples);
        outptr16[2] = 0x0000;
        break;
    }

    // Pd = payload length in bits. Aligned to 2 bytes.
    outptr16[3] = ((framesize + 0x2) & ~0x1) * 8;

    if (byteSwap)
    {
        // copy encoded frame with byte swapping
        uint32_t i;

        for (i=0; i<(framesize & ~1); i+=2)
        {
            dst[i] = inpptr8[i+1];
            dst[i+1] = inpptr8[i];
        }

        if (framesize & 1)
        {
            dst[framesize-1] = 0;
            dst[framesize] = inpptr8[framesize-1];
            framesize++;
        }
    }
    else
    {
        // copy encoded frame as it is
        NvOsMemcpy(outptr8 + PREAMBLE_SIZE_BYTES, inpptr8, framesize);
    }

    // stuff zeros
    NvOsMemset(outptr8 + PREAMBLE_SIZE_BYTES + framesize,
                0,
                outputsize - (PREAMBLE_SIZE_BYTES + framesize));
}

static void BypassProcessDTS(SNvxBypassDecoderData *pContext,
                                OMX_U8 *inptr8,
                                OMX_U32 inputSize,
                                OMX_U8 *outptr8,
                                OMX_U32 outputSize)
{
    NvS32 error;
    NvS32 totalOutputSize, sampleRate, channels, framesize, nsamples;
    NvBool needByteSwap, isFormat14bit;

    totalOutputSize = sampleRate = channels = framesize = nsamples = 0;
    needByteSwap = isFormat14bit = NV_FALSE;

    error = parseDTS(pContext,
                        inptr8,
                        inputSize,
                        &totalOutputSize,
                        &nsamples,
                        &needByteSwap,
                        &isFormat14bit,
                        &sampleRate,
                        &channels,
                        &framesize);

    if (ERROR_OK != error)
    {
        NvOsDebugPrintf("BypassDecoder: Invalid DTS frame, skipping\n");
        if (ERROR_BAD_SYNC == error)
        {
            pContext->outputBytes = 0;
            pContext->bytesConsumed = 4; // consume syncword
        }
        else
        {
            pContext->outputBytes = 0;
            pContext->bytesConsumed = inputSize;
        }
        pContext->bFrameOK = OMX_FALSE;
    }
    else
    {
        //NvOsDebugPrintf("BypassDecoder: found DTS frame: framesize: %d rate: %d channels: %d nsamples: %d needByteSwap: %d",
          //          framesize, sampleRate, channels, nsamples, needByteSwap);

        if (OMX_AUDIO_CodingPCM != pContext->outputCodingType)
        {
            packDTSFrame(inptr8, outptr8, framesize, totalOutputSize, nsamples, needByteSwap);
        }
        else
        {
            NvOsMemset(outptr8, 0, totalOutputSize);
        }

        //NvOsDebugPrintf("BypassDecoder: pContext->OutputAudioFormat %d\n", pContext->OutputAudioFormat);

        pContext->nSampleRate = sampleRate;
        pContext->nChannels = 2;
        pContext->outputBytes = totalOutputSize;
        pContext->bytesConsumed = framesize;
        pContext->bFrameOK = OMX_TRUE;
    }
}


/* =========================================================================
 * Bypass Decoder functions
 * ========================================================================= */

static OMX_ERRORTYPE NvxBypassDecoderDeInit(NvxComponent *pNvComp)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    NVXTRACE((NVXT_CALLGRAPH, pNvComp->eObjectType, "+NvxBypassDecoderDeInit\n"));

    NvOsFree(pNvComp->pComponentData);
    pNvComp->pComponentData = 0;
    return eError;
}

static OMX_ERRORTYPE NvxBypassDecoderGetParameter(OMX_IN NvxComponent *pComponent, OMX_IN OMX_INDEXTYPE nParamIndex, OMX_INOUT OMX_PTR pComponentParameterStructure)
{
    SNvxBypassDecoderData *pNvxBypassDecoder;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    NVXTRACE((NVXT_CALLGRAPH, pComponent->eObjectType, "+NvxBypassDecoderGetParameter 0x%x \n", nParamIndex));

    pNvxBypassDecoder = (SNvxBypassDecoderData *)pComponent->pComponentData;

    switch (nParamIndex)
    {
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDef =
            (OMX_PARAM_PORTDEFINITIONTYPE *)pComponentParameterStructure;

        NvxComponentBaseGetParameter(pComponent, nParamIndex, pComponentParameterStructure);
        if (s_nvx_port_output == (NvS32)pPortDef->nPortIndex)
        {
            NvOsMemcpy(&pPortDef->format.audio,
                        &pComponent->pPorts[s_nvx_port_output].oPortDef.format.audio,
                        sizeof(OMX_AUDIO_PORTDEFINITIONTYPE));

            pPortDef->format.audio.eEncoding = pNvxBypassDecoder->outputCodingType;
        }
        else
        {
            eError = OMX_ErrorNone;
        }
        break;
    }
    case OMX_IndexParamAudioPcm:
    {
        OMX_AUDIO_PARAM_PCMMODETYPE *pcmParams =
            (OMX_AUDIO_PARAM_PCMMODETYPE *)pComponentParameterStructure;

        if ((NvU32)s_nvx_port_output == pcmParams->nPortIndex)
        {
            pcmParams->eNumData = OMX_NumericalDataSigned;
            pcmParams->eEndian = OMX_EndianLittle;
            pcmParams->bInterleaved = OMX_TRUE;
            pcmParams->nBitPerSample = 16;
            pcmParams->ePCMMode = OMX_AUDIO_PCMModeLinear;
            pcmParams->eChannelMapping[0] = OMX_AUDIO_ChannelLF;
            pcmParams->eChannelMapping[1] = OMX_AUDIO_ChannelRF;
            pcmParams->nChannels = 2;
            pcmParams->nSamplingRate = pNvxBypassDecoder->nSampleRate;
        }
        else
        {
            eError = OMX_ErrorBadParameter;
        }
        break;
    }
    case NVX_IndexParamAudioAc3:
    {
        NVX_AUDIO_PARAM_AC3TYPE *pAc3Param =
            (NVX_AUDIO_PARAM_AC3TYPE *)pComponentParameterStructure;

        if ((NvU32)s_nvx_port_output == pAc3Param->nPortIndex)
        {
            // as per HDMI spec 1.3a, section 7.6, IEC61937 stream should use
            // 2 channel format
            pAc3Param->nChannels = 2;
            pAc3Param->nSampleRate = pNvxBypassDecoder->nSampleRate;
        }
        else
        {
            eError = OMX_ErrorBadParameter;
        }
        break;
    }
    case NVX_IndexParamAudioDts:
    {
        NVX_AUDIO_PARAM_DTSTYPE *pDtsaram =
            (NVX_AUDIO_PARAM_DTSTYPE *)pComponentParameterStructure;

        if ((NvU32)s_nvx_port_output == pDtsaram->nPortIndex)
        {
            // as per HDMI spec 1.3a, section 7.6, IEC61937 stream should use
            // 2 channel format
            pDtsaram->nChannels = 2;
            pDtsaram->nSampleRate = pNvxBypassDecoder->nSampleRate;
        }
        else
        {
            eError = OMX_ErrorBadParameter;
        }
        break;
    }
    default:
        eError = NvxComponentBaseGetParameter(pComponent, nParamIndex, pComponentParameterStructure);
    };

    return eError;
}

static OMX_ERRORTYPE NvxBypassDecoderSetParameter(OMX_IN NvxComponent *pComponent, OMX_IN OMX_INDEXTYPE nIndex, OMX_IN OMX_PTR pComponentParameterStructure)
{
    SNvxBypassDecoderData *pNvxBypassDecoder;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    NV_ASSERT(pComponent && pComponentParameterStructure);

    NVXTRACE((NVXT_CALLGRAPH, pComponent->eObjectType, "+NvxBypassDecoderSetParameter 0x%x \n", nIndex));

    pNvxBypassDecoder = (SNvxBypassDecoderData *)pComponent->pComponentData;
    switch(nIndex)
    {
    case OMX_IndexParamStandardComponentRole:
    {
        const OMX_PARAM_COMPONENTROLETYPE *roleParams =
            (const OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;

        NvOsDebugPrintf("BypassDecoder: setting component role: %s", roleParams->cRole);

        if (!strcmp((const char*)roleParams->cRole, "audio_decoder.ac3"))
            pNvxBypassDecoder->inputCodingType = (OMX_AUDIO_CODINGTYPE)NVX_AUDIO_CodingAC3;
        else if (!strcmp((const char*)roleParams->cRole, "audio_decoder.dts"))
            pNvxBypassDecoder->inputCodingType = (OMX_AUDIO_CODINGTYPE)NVX_AUDIO_CodingDTS;
        else
            NvOsDebugPrintf("BypassDecoder: Unsupported component role: %s", roleParams->cRole);
        break;
    }
    default:
        eError = NvxComponentBaseSetParameter(pComponent, nIndex, pComponentParameterStructure);
    }
    return eError;
}

static OMX_ERRORTYPE NvxBypassDecoderGetConfig(OMX_IN NvxComponent* pNvComp, OMX_IN OMX_INDEXTYPE nIndex, OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    SNvxBypassDecoderData *pNvxBypassDecoder;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    NV_ASSERT(pNvComp && pComponentConfigStructure);

    NVXTRACE((NVXT_CALLGRAPH, pNvComp->eObjectType, "+NvxBypassDecoderGetConfig\n"));

    pNvxBypassDecoder = (SNvxBypassDecoderData *)pNvComp->pComponentData;
    switch (nIndex)
    {
    case NVX_IndexConfigAudioSilenceOutput:
    {
        OMX_CONFIG_BOOLEANTYPE *pSilenceOutput;
        pSilenceOutput = (OMX_CONFIG_BOOLEANTYPE*)pComponentConfigStructure;
        if (pSilenceOutput->nSize == sizeof(OMX_CONFIG_BOOLEANTYPE))
        {
            pSilenceOutput->bEnabled = pNvxBypassDecoder->bSilenceOutput;
        }
        else
        {
            eError = OMX_ErrorBadParameter;
        }
        break;
    }
    default:
        eError = NvxComponentBaseGetConfig( pNvComp, nIndex, pComponentConfigStructure);
        break;
    }
    return eError;
}

static OMX_ERRORTYPE NvxBypassDecoderSetConfig(OMX_IN NvxComponent* pNvComp, OMX_IN OMX_INDEXTYPE nIndex, OMX_IN OMX_PTR pComponentConfigStructure)
{
    SNvxBypassDecoderData *pNvxBypassDecoder;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    NV_ASSERT(pNvComp && pComponentConfigStructure);

    NVXTRACE((NVXT_CALLGRAPH, pNvComp->eObjectType, "+NvxBypassDecoderSetConfig 0x%x \n", nIndex));

    pNvxBypassDecoder = (SNvxBypassDecoderData *)pNvComp->pComponentData;
    switch (nIndex)
    {
    case NVX_IndexConfigAudioSilenceOutput:
    {
        OMX_CONFIG_BOOLEANTYPE *pSilenceOutput =
            (OMX_CONFIG_BOOLEANTYPE *)pComponentConfigStructure;
        if (pSilenceOutput->nSize == sizeof(OMX_CONFIG_BOOLEANTYPE))
        {
            pNvxBypassDecoder->bSilenceOutput = pSilenceOutput->bEnabled;
            if (pSilenceOutput->bEnabled)
            {
                pNvxBypassDecoder->newOutputCodingType = OMX_AUDIO_CodingPCM;
            }
            else
            {
                pNvxBypassDecoder->newOutputCodingType = pNvxBypassDecoder->inputCodingType;
            }
            NvOsDebugPrintf("BypassDecoder: Setting output coding type to 0x%x", pNvxBypassDecoder->newOutputCodingType);
        }
        else
        {
            eError = OMX_ErrorBadParameter;
        }
        break;
    }
    default:
            eError = NvxComponentBaseSetConfig( pNvComp, nIndex, pComponentConfigStructure);
    }
    return eError;
}

static OMX_ERRORTYPE NvxBypassDecoderFlush(OMX_IN NvxComponent *pComponent, OMX_U32 nPort)
{
    SNvxBypassDecoderData *pNvxBypassDecoder;

    NV_ASSERT(pComponent && pComponent->pComponentData);

    NVXTRACE((NVXT_CALLGRAPH, pComponent->eObjectType, "+NvxAudioDecoderFlush\n"));

    pNvxBypassDecoder = (SNvxBypassDecoderData *)pComponent->pComponentData;
    pNvxBypassDecoder->bOverrideTimestamp = OMX_TRUE;
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE NvxBypassDecoderWorkerFunction(OMX_IN NvxComponent *pComponent, OMX_IN OMX_BOOL bAllPortsReady, OMX_OUT OMX_BOOL *pbMoreWork, OMX_INOUT NvxTimeMs* puMaxMsecToNextCall)
{
    SNvxBypassDecoderData *pNvxBypassDecoder;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    NvxPort *pPortIn = &pComponent->pPorts[s_nvx_port_input];
    NvxPort *pPortOut = &pComponent->pPorts[s_nvx_port_output];
    OMX_BUFFERHEADERTYPE *pInBuffer, *pOutBuffer;

    NVXTRACE((NVXT_WORKER, pComponent->eObjectType, "+NvxBypassDecoderWorkerFunction\n"));

    *pbMoreWork = bAllPortsReady;

    pNvxBypassDecoder = (SNvxBypassDecoderData *)pComponent->pComponentData;

    if (!bAllPortsReady)
    {
        //NvOsDebugPrintf("BypassDecoder: all ports not ready\n");
        return OMX_ErrorNone;
    }

    pInBuffer = pPortIn->pCurrentBufferHdr;
    pOutBuffer = pPortOut->pCurrentBufferHdr;

    if (NVX_AUDIO_CodingAC3 == pNvxBypassDecoder->inputCodingType)
    {
        BypassProcessAC3(pNvxBypassDecoder,
                    pInBuffer->pBuffer + pInBuffer->nOffset,
                    pInBuffer->nFilledLen,
                    pOutBuffer->pBuffer + pOutBuffer->nOffset,
                    pOutBuffer->nAllocLen);
    }
    else if (NVX_AUDIO_CodingDTS == pNvxBypassDecoder->inputCodingType)
    {
        BypassProcessDTS(pNvxBypassDecoder,
                    pInBuffer->pBuffer + pInBuffer->nOffset,
                    pInBuffer->nFilledLen,
                    pOutBuffer->pBuffer + pOutBuffer->nOffset,
                    pOutBuffer->nAllocLen);
    }
    else
        NvOsDebugPrintf("Invalid coding type %x", pNvxBypassDecoder->inputCodingType);

    if ((pNvxBypassDecoder->bFrameOK && pNvxBypassDecoder->outputBytes > 0) ||
        (pInBuffer->nFlags & OMX_BUFFERFLAG_EOS))
    {
        pOutBuffer->nFilledLen = pNvxBypassDecoder->outputBytes;
        if (NVX_AUDIO_CodingAC3 == pNvxBypassDecoder->inputCodingType) {
            if (pNvxBypassDecoder->bOverrideTimestamp == OMX_TRUE) {
                pNvxBypassDecoder->timeStamp = pInBuffer->nTimeStamp;
                pOutBuffer->nTimeStamp = pInBuffer->nTimeStamp;
                pNvxBypassDecoder->bOverrideTimestamp = OMX_FALSE;
            } else {
                NvU64 timeStamp = ((NvU64)pNvxBypassDecoder->outputBytes * 1000000)/(2 * pNvxBypassDecoder->nChannels * pNvxBypassDecoder->nSampleRate);
                pNvxBypassDecoder->timeStamp += (timeStamp);
                pOutBuffer->nTimeStamp = pNvxBypassDecoder->timeStamp;
            }
        } else {
            pOutBuffer->nTimeStamp = pInBuffer->nTimeStamp;
        }

        pOutBuffer->nFlags = pInBuffer->nFlags;
        if (OMX_AUDIO_CodingPCM != pNvxBypassDecoder->outputCodingType)
            pOutBuffer->nFlags |= OMX_BUFFERFLAG_COMPRESSED;

        //NvOsDebugPrintf("BypassDecoder: release output buffer size: %d\n", pOutBuffer->nFilledLen);

        /* release output buffer */
        NvxPortDeliverFullBuffer(pPortOut, pOutBuffer);
    }
    else
    {
        NvOsDebugPrintf("BypassDecoder: Error in frame, skipping\n");
    }

    if (pInBuffer->nFlags & OMX_BUFFERFLAG_EOS)
    {
        pInBuffer->nFilledLen = 0;
        pInBuffer->nOffset = 0;
    }
    else
    {
        pInBuffer->nFilledLen -= pNvxBypassDecoder->bytesConsumed;
        pInBuffer->nOffset += pNvxBypassDecoder->bytesConsumed;
    }

    if (pInBuffer->nFilledLen == 0)
    {
        //NvOsDebugPrintf("BypassDecoder: release input buffer\n");
        /* Done with the input buffer, so now release it */
        NvxPortReleaseBuffer(pPortIn, pInBuffer);
    }

    if (pNvxBypassDecoder->newOutputCodingType != pNvxBypassDecoder->outputCodingType)
    {
        pNvxBypassDecoder->outputCodingType = pNvxBypassDecoder->newOutputCodingType;
        NvxSendEvent(pComponent,
                    OMX_EventPortSettingsChanged,
                    s_nvx_port_output,
                    OMX_IndexParamAudioPcm,
                    NULL);
        NvOsDebugPrintf("BypassDecoder: Format changed to 0x%x\n", pNvxBypassDecoder->outputCodingType);
    }

    return eError;
}

static OMX_ERRORTYPE NvxBypassDecoderAcquireResources(OMX_IN NvxComponent *pComponent)
{
    SNvxBypassDecoderData *pNvxBypassDecoder;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    NV_ASSERT(pComponent);

    NVXTRACE((NVXT_CALLGRAPH, pComponent->eObjectType, "+NvxBypassDecoderAcquireResources\n"));

    /* Get Aac Decoder instance */
    pNvxBypassDecoder = (SNvxBypassDecoderData *)pComponent->pComponentData;
    NV_ASSERT(pNvxBypassDecoder);

    eError = NvxComponentBaseAcquireResources(pComponent);
    if (eError != OMX_ErrorNone)
    {
        NVXTRACE((NVXT_ERROR,pComponent->eObjectType, "ERROR:NvxBypassDecoderAcquireResources:"
           ":eError = %x:[%s(%d)]\n",eError,__FILE__, __LINE__));
        return eError;
    }

    /* Open handles and setup frames */
    eError = NvxBypassDecoderOpen( pNvxBypassDecoder, pComponent );

    return eError;
}

static OMX_ERRORTYPE NvxBypassDecoderReleaseResources(OMX_IN NvxComponent *pComponent)
{
    SNvxBypassDecoderData *pNvxBypassDecoder;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    NV_ASSERT(pComponent);

    NVXTRACE((NVXT_CALLGRAPH, pComponent->eObjectType, "+NvxBypassDecoderReleaseResources\n"));

    /* Get Aac Decoder instance */
    pNvxBypassDecoder = (SNvxBypassDecoderData *)pComponent->pComponentData;
    NV_ASSERT(pNvxBypassDecoder);

    eError = NvxBypassDecoderClose(pNvxBypassDecoder);
    NvxCheckError(eError, NvxComponentBaseReleaseResources(pComponent));
    return eError;
}


/* =========================================================================
 *                     I N T E R N A L
 * ========================================================================= */

OMX_ERRORTYPE NvxBypassDecoderOpen( SNvxBypassDecoderData * pNvxBypassDecoder, NvxComponent *pComponent )
{
    NVXTRACE(( NVXT_CALLGRAPH, NVXT_AUDIO_DECODER, "+NvxBypassDecoderOpen\n"));

    pNvxBypassDecoder->bFirstBuffer = OMX_TRUE;
    pNvxBypassDecoder->bInitialized = OMX_TRUE;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE NvxBypassDecoderClose(SNvxBypassDecoderData * pNvxBypassDecoder)
{
    NVXTRACE((NVXT_CALLGRAPH, NVXT_AUDIO_DECODER, "+NvxBypassDecoderClose\n"));

    if (!pNvxBypassDecoder->bInitialized)
        return OMX_ErrorNone;

    pNvxBypassDecoder->bInitialized = OMX_FALSE;
    return OMX_ErrorNone;
}

/******************
 * Init functions *
 ******************/
OMX_ERRORTYPE NvxBypassDecoderInit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    NvxComponent* pThis;
    SNvxBypassDecoderData *pNvxBypassDecoder = NULL;

    OMX_AUDIO_PARAM_PCMMODETYPE  *pNvxPcmParameters;

    eError = NvxComponentCreate(hComponent, s_nvx_num_ports, &pThis);
    if (eError != OMX_ErrorNone)
    {
        NVXTRACE((NVXT_ERROR,pThis->eObjectType,"ERROR:NvxCommonBypassDecoderInit:"
            ":eError = %d:[%s(%d)]\n",eError,__FILE__, __LINE__));
        return eError;
    }

    NvxAddResource(pThis, NVX_AUDIO_MEDIAPROCESSOR_RES);

    pThis->eObjectType = NVXT_AUDIO_DECODER;

    pNvxBypassDecoder = NvOsAlloc(sizeof(SNvxBypassDecoderData));
    if (!pNvxBypassDecoder)
    {
        NVXTRACE((NVXT_ERROR,pThis->eObjectType,"ERROR:NvxCommonBypassDecoderInit:"
            ":SNvxAudioData = %d:[%s(%d)]\n",pNvxBypassDecoder,__FILE__, __LINE__));
        return OMX_ErrorInsufficientResources;
    }

    NvOsMemset(pNvxBypassDecoder, 0, sizeof(SNvxBypassDecoderData));

    pNvxBypassDecoder->bFirstBuffer = OMX_TRUE;
    pNvxBypassDecoder->inputCodingType = NVX_AUDIO_CodingAC3;
    pNvxBypassDecoder->outputCodingType = OMX_AUDIO_CodingPCM;
    pNvxBypassDecoder->newOutputCodingType = OMX_AUDIO_CodingPCM;
    pNvxBypassDecoder->nChannels = 2;
    pNvxBypassDecoder->nSampleRate = 48000;
    pNvxBypassDecoder->bOverrideTimestamp = OMX_TRUE;

    pThis->pComponentData = pNvxBypassDecoder;

    pThis->pComponentName = "OMX.Nvidia.bypass.decoder";
    pThis->nComponentRoles = 1;
    pThis->sComponentRoles[0] = "audio_decoder.ac3";

    pThis->DeInit             = NvxBypassDecoderDeInit;
    pThis->GetParameter       = NvxBypassDecoderGetParameter;
    pThis->SetParameter       = NvxBypassDecoderSetParameter;
    pThis->GetConfig          = NvxBypassDecoderGetConfig;
    pThis->SetConfig          = NvxBypassDecoderSetConfig;
    pThis->WorkerFunction     = NvxBypassDecoderWorkerFunction;
    pThis->AcquireResources   = NvxBypassDecoderAcquireResources;
    pThis->ReleaseResources   = NvxBypassDecoderReleaseResources;
    pThis->Flush              = NvxBypassDecoderFlush;
    pThis->pPorts[s_nvx_port_output].oPortDef.nPortIndex = s_nvx_port_output;

    NvxPortInitAudio(&pThis->pPorts[s_nvx_port_input], OMX_DirInput, MAX_INPUT_BUFFERS, MIN_INPUT_BUFFERSIZE, pNvxBypassDecoder->inputCodingType);
    NvxPortInitAudio(&pThis->pPorts[s_nvx_port_output], OMX_DirOutput, MAX_OUTPUT_BUFFERS, MIN_OUTPUT_BUFFERSIZE, OMX_AUDIO_CodingPCM);

    /* Initialize default parameters - required of a standard component */
    ALLOC_STRUCT(pThis, pNvxPcmParameters, OMX_AUDIO_PARAM_PCMMODETYPE);
    if (!pNvxPcmParameters)
        return OMX_ErrorInsufficientResources;

    //Initialize the default values for the standard component requirement.
    pNvxPcmParameters->nPortIndex = pThis->pPorts[s_nvx_port_output].oPortDef.nPortIndex;
    pNvxPcmParameters->nChannels = 2;
    pNvxPcmParameters->eNumData = OMX_NumericalDataSigned;
    pNvxPcmParameters->nSamplingRate = 48000;
    pNvxPcmParameters->ePCMMode = OMX_AUDIO_PCMModeLinear;
    pNvxPcmParameters->bInterleaved = OMX_TRUE;
    pNvxPcmParameters->nBitPerSample = 16;
    pNvxPcmParameters->eChannelMapping[0] = OMX_AUDIO_ChannelLF;
    pNvxPcmParameters->eChannelMapping[1] = OMX_AUDIO_ChannelRF;

    pThis->pPorts[s_nvx_port_output].pPortPrivate = pNvxPcmParameters;

    return eError;
}

