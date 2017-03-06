/* Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an
 * express license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include <string.h>
#include "NVOMX_IndexExtensions.h"
#include "NvxCompMisc.h"

typedef struct {
    OMX_STRING                  compName;
    NVX_H264_DECODE_INFO        supported;
} NVX_COMPONENT_CAP_H264_DEC;

NVX_COMPONENT_CAP_H264_DEC  Nvx_CompnentCapH264DecAP16[] = {
    { "OMX.Nvidia.h264.decode", { OMX_FALSE, OMX_FALSE, 1280, 720 } },
    { "OMX.Nvidia.h264.decode", { OMX_FALSE, OMX_TRUE ,  720, 480 } },
    { 0 },
};

NVX_COMPONENT_CAP_H264_DEC  Nvx_CompnentCapH264DecAP20[] = {
    { "OMX.Nvidia.h264.decode", { OMX_FALSE, OMX_FALSE, 1920, 1080 } },
    { "OMX.Nvidia.h264.decode", { OMX_FALSE, OMX_TRUE , 1280,  720 } },
    { 0 },
};

// read 1 byte from NALU, discard emulation_prevention_three_byte.
static OMX_U32 NVOMX_ReadByte1(NVX_OMX_NALU_BIT_STREAM *bitstm)
{
    OMX_U32 byte = 0;
    if (bitstm->currentPtr >= bitstm->dataEnd)
        return 0;

    byte = *(bitstm->currentPtr)++;

    if (byte == 0)
    {
        bitstm->zeroCount++;

        if (bitstm->zeroCount == 2 &&
            bitstm->currentPtr < bitstm->dataEnd &&
            *(bitstm->currentPtr) == 0x03)
        {
            bitstm->zeroCount = 0;
            bitstm->currentPtr++;
        }
    }
    else
    {
        bitstm->zeroCount = 0;
    }

    return byte;
}

// read 1 bit
static OMX_U32 NVOMX_ReadBit1(NVX_OMX_NALU_BIT_STREAM *bitstm)
{
    if (bitstm->bitCount == 0)
    {
        bitstm->currentBuffer  = NVOMX_ReadByte1(bitstm) << 24;
        bitstm->currentBuffer |= NVOMX_ReadByte1(bitstm) << 16;
        bitstm->currentBuffer |= NVOMX_ReadByte1(bitstm) <<  8;
        bitstm->currentBuffer |= NVOMX_ReadByte1(bitstm);
        bitstm->bitCount = 32;
    }
    bitstm->bitCount--;
    return ( (bitstm->currentBuffer << (31 - bitstm->bitCount)) >> 31);
}

// reads n bitstm
static OMX_U32 NVOMX_ReadBits(NVX_OMX_NALU_BIT_STREAM *bitstm, int count)
{
    OMX_U32 retval;

    if (count <= 0)
        return 0;

    if (count >= bitstm->bitCount)
    {
        retval = 0;
        if (bitstm->bitCount)
        {
            retval = ( (bitstm->currentBuffer << (32 - bitstm->bitCount)) >> (32 - count) );
            count -= bitstm->bitCount;
        }

        bitstm->currentBuffer  = NVOMX_ReadByte1(bitstm) << 24;
        bitstm->currentBuffer |= NVOMX_ReadByte1(bitstm) << 16;
        bitstm->currentBuffer |= NVOMX_ReadByte1(bitstm) <<  8;
        bitstm->currentBuffer |= NVOMX_ReadByte1(bitstm);
        bitstm->bitCount = 32 - count;

        if (count)
        {
            retval |= ( bitstm->currentBuffer >> (32 - count) );
        }
    }
    else
    {
        retval = ( (bitstm->currentBuffer << (32 - bitstm->bitCount)) >> (32 - count) );
        bitstm->bitCount -= count;
    }
    return retval;
}

// read ue(v)
static OMX_U32 NVOMX_ReadUE(NVX_OMX_NALU_BIT_STREAM *bitstm)
{
    OMX_U32 codeNum = 0;
    while (NVOMX_ReadBit1(bitstm) == 0 && codeNum < 32)
        codeNum++;

    return NVOMX_ReadBits(bitstm, codeNum) + ((1 << codeNum) - 1);
}

// read se(v)
static OMX_S32 NVOMX_ReadSE(NVX_OMX_NALU_BIT_STREAM *bitstm)
{
    OMX_U32 codeNum = NVOMX_ReadUE(bitstm);
    OMX_S32 sval = (codeNum + 1) >> 1;
    if (codeNum & 1)
        return sval;
    return -sval;
}

// read scaling list and discard it
static void NVOMX_ScalingList(NVX_OMX_NALU_BIT_STREAM *bitstm, int size)
{
    int id;
    int lastScale = 8;
    int nextScale = 8;
    for (id = 0; id < size; id++)
    {
        int delta_scale;
        if (nextScale != 0)
        {
            delta_scale = NVOMX_ReadSE(bitstm);
            nextScale = (lastScale + delta_scale + 256) % 256;
        }
        lastScale = (nextScale == 0) ? lastScale : nextScale;
    }
    return;
}

// parse sps
OMX_BOOL NVOMX_ParseSPS(OMX_U8* data, int len, NVX_OMX_SPS *pSPS)
{
    int id;
    OMX_U32 chroma_format_idc = 1;
    OMX_U32 pic_order_cnt_type;
    OMX_U32 pic_width_in_mbs_minus1;
    OMX_U32 pic_height_in_map_units_minus1;
    OMX_U32 frame_mbs_only_flag;

    NVX_OMX_NALU_BIT_STREAM nalu = {0};

    if (len <= 1 || !data)
        return OMX_FALSE;

    id = *data++;        // first byte nal_unit_type
    len--;

    if ( (id & 0x1f) != 7)
        return OMX_FALSE;

    nalu.data = data;
    nalu.size = len;
    nalu.dataEnd = data + len;
    nalu.currentPtr = data;

    pSPS->profile_idc = NVOMX_ReadBits(&nalu, 8);

    pSPS->constraint_set0123_flag = NVOMX_ReadBits(&nalu, 4);
    /*reserved_zero_4bits   = */ NVOMX_ReadBits(&nalu, 4);

    pSPS->level_idc = NVOMX_ReadBits(&nalu, 8);
    /*seq_parameter_set_id  = */ NVOMX_ReadUE(&nalu);

    if (66 != pSPS->profile_idc &&
        77 != pSPS->profile_idc &&
        88 != pSPS->profile_idc)
    {
        chroma_format_idc = NVOMX_ReadUE(&nalu);

        if(3 == chroma_format_idc)
        {
            /*residual_colour_transform_flag        = */ NVOMX_ReadBit1(&nalu);
        }
        /*bit_depth_luma_minus8                 = */ NVOMX_ReadUE(&nalu);
        /*bit_depth_chroma_minus8               = */ NVOMX_ReadUE(&nalu);
        /*qpprime_y_zero_transform_bypass_flag  = */ NVOMX_ReadBit1(&nalu);
        if (NVOMX_ReadBit1(&nalu))    // seq_scaling_matrix_present_flag
        {
            for (id = 0; id < 8; id++)
            {
                if (NVOMX_ReadBit1(&nalu)) // seq_scaling_list_present_flag
                {
                    if(id < 6)
                    {
                        NVOMX_ScalingList(&nalu, 16);
                    }
                    else
                    {
                        NVOMX_ScalingList(&nalu, 64);
                    }
                }
            }
        }
    }

    if (83 == pSPS->profile_idc /*AVCDEC_PROFILE_SCALABLE_0*/ ||
        86 == pSPS->profile_idc /*AVCDEC_PROFILE_SCALABLE_1*/ )
    {
        // some data here;
        return OMX_FALSE;
    }

    /*log2_max_frame_num_minus4 = */ NVOMX_ReadUE(&nalu);
    pic_order_cnt_type = NVOMX_ReadUE(&nalu);
    if (pic_order_cnt_type == 0)
    {
        /*log2_max_pic_order_cnt_lsb_minus4 = */ NVOMX_ReadUE(&nalu);
    }
    else if(pic_order_cnt_type == 1)
    {
        OMX_U32 num_ref_frames_in_pic_order_cnt_cycle;
        /*delta_pic_order_always_zero_flag = */ NVOMX_ReadBit1(&nalu);
        /*offset_for_non_ref_pic = */           NVOMX_ReadSE(&nalu);
        /*offset_for_top_to_bottom_field = */   NVOMX_ReadSE(&nalu);
        num_ref_frames_in_pic_order_cnt_cycle = NVOMX_ReadUE(&nalu);
        for (id = 0; id < (int)num_ref_frames_in_pic_order_cnt_cycle; id++)
        {
            /*offset_for_ref_frame = */ NVOMX_ReadSE(&nalu);
        }
    }
    /*num_ref_frames = */NVOMX_ReadUE(&nalu);
    /*gaps_in_frame_num_value_allowed_flag = */NVOMX_ReadBit1(&nalu);
    pic_width_in_mbs_minus1 = NVOMX_ReadUE(&nalu);
    pic_height_in_map_units_minus1 = NVOMX_ReadUE(&nalu);
    frame_mbs_only_flag = NVOMX_ReadBit1(&nalu);
    if (!frame_mbs_only_flag)
        /*mb_adaptive_frame_field_flag = */ NVOMX_ReadBit1(&nalu);
    /*direct_8x8_inference_flag = */ NVOMX_ReadBit1(&nalu);

    pSPS->nWidth = (pic_width_in_mbs_minus1 + 1) << 4;
    pSPS->nHeight = ((pic_height_in_map_units_minus1 + 1) * ((frame_mbs_only_flag)? 1 : 2)) << 4;

    if (NVOMX_ReadBit1(&nalu)) // frame_cropping_flag
    {
        /*frame_crop_left_offset   */ NVOMX_ReadUE(&nalu);
        /*frame_crop_right_offset  */ NVOMX_ReadUE(&nalu);
        /*frame_crop_top_offset    */ NVOMX_ReadUE(&nalu);
        /*frame_crop_bottom_offset */ NVOMX_ReadUE(&nalu);
    }

    /*vui_parameters_present_flag */ NVOMX_ReadBit1(&nalu);

    return OMX_TRUE;
}

// parse pps
static OMX_BOOL NVOMX_ParsePPS(OMX_U8* data, int len, NVX_OMX_PPS *pPPS)
{
    int id;
    NVX_OMX_NALU_BIT_STREAM nalu = {0};

    if (len <= 1 || !data)
        return OMX_FALSE;

    id = *data++;        // first byte nal_unit_type
    len--;

    if ( (id & 0x1f) != 8)
        return OMX_FALSE;

    nalu.data = data;
    nalu.size = len;
    nalu.dataEnd = data + len;
    nalu.currentPtr = data;

    /*pic_parameter_set_id = */ NVOMX_ReadUE(&nalu);
    /*seq_parameter_set_id = */ NVOMX_ReadUE(&nalu);

    pPPS->entropy_coding_mode_flag = NVOMX_ReadBit1(&nalu);

    // more parsing to follow
    return OMX_TRUE;
}

static OMX_STRING NVOMX_FindCompH264(NVX_STREAM_PLATFORM_INFO *pStreamInfo)
{
    NVX_H264_DECODE_INFO *pH264Info = &pStreamInfo->streamInfo.h264;
    NVX_COMPONENT_CAP_H264_DEC *pCompCapH264 = Nvx_CompnentCapH264DecAP16;

    if (pH264Info->bUseSPSAndPPS)
    {
        int i;
        pH264Info->bHasCABAC = OMX_FALSE;
        pH264Info->nWidth = 0;
        pH264Info->nHeight = 0;
        for (i = 0; i < (int)pH264Info->nSPSCount; i++)
        {
            NVX_OMX_SPS sps = {0};
            if (OMX_FALSE == NVOMX_ParseSPS(pH264Info->SPSNAUL[i], pH264Info->SPSNAULLen[i], &sps))
                return NULL;
            if (sps.profile_idc != 66 && sps.profile_idc != 77 &&
                !(sps.constraint_set0123_flag & 8) &&
                !(sps.constraint_set0123_flag & 4))
            {
                return NULL;
            }
            if (sps.nWidth > pH264Info->nWidth)
                pH264Info->nWidth = sps.nWidth;
            if (sps.nHeight > pH264Info->nHeight)
                pH264Info->nHeight = sps.nHeight;
        }
        for (i = 0; i < (int)pH264Info->nPPSCount; i++)
        {
            NVX_OMX_PPS pps = {0};
            if (OMX_FALSE == NVOMX_ParsePPS(pH264Info->PPSNAUL[i], pH264Info->PPSNAULLen[i], &pps))
                return NULL;
            if (pps.entropy_coding_mode_flag)
            {
                pH264Info->bHasCABAC = OMX_TRUE;
                break;
            }
        }
    }

    if (pStreamInfo->nPlatform == 1)
    {
        pCompCapH264 = Nvx_CompnentCapH264DecAP20;
    }

    while (pCompCapH264->compName)
    {
        NVX_H264_DECODE_INFO *pCompCap = &pCompCapH264->supported;

        if ( ((pH264Info->bHasCABAC && pCompCap->bHasCABAC) || !pH264Info->bHasCABAC) &&
            pCompCap->nWidth >= pH264Info->nWidth &&
            pCompCap->nHeight >= pH264Info->nHeight)
        {
            return pCompCapH264->compName;
        }

        pCompCapH264++;
    }

    return NULL;
}

OMX_API OMX_ERRORTYPE NVOMX_FindComponentName(
    OMX_IN  NVX_STREAM_PLATFORM_INFO *pStreamInfo,
    OMX_OUT OMX_STRING  *compName)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    *compName = NULL;

    switch (pStreamInfo->eStreamType)
    {
    case NvxStreamType_H264:
        *compName = NVOMX_FindCompH264(pStreamInfo);
        break;

    default:
        break;
    }

    return eError;
}


