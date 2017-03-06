/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */


/* This is used as a common error concealment code for all the video decoders
 */

#ifndef INCLUDED_NVMM_VIDEODEC_EC_H
#define INCLUDED_NVMM_VIDEODEC_EC_H

/**
 *  @defgroup nvmm NVIDIA Multimedia APIs
 */


/**
 *  @defgroup nvmm_modules Multimedia Codec APIs
 *
 *  @ingroup nvmm
 * @{
 */

#include "nvmm.h"
#include "nvassert.h"
#include "nvos.h"
#include "nvrm_init.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/**
* @brief Defines the various types of frames to be used for decoding.
*
* @since NVDDK 1.0
* @par Header:
* Declared in %nvmm_video_ec.h
* @ingroup ??
*/
typedef enum
{
    // Specifies the Intra Frame.
    NvMMVideoDecEC_FrameType_I = 0x1,

    // Specifies the Inter Frame
    NvMMVideoDecEC_FrameType_P = 0x2,

    // Specifies the B Frmae
    NvMMVideoDecEC_FrameType_B = 0x4,

    // Max should always be the last value, used to pad the enum to 32bits
    NvMMVideoDecEC_FrameType_Force32                     = 0x7FFFFFFF
} NvMMVideoDecEC_FrameType;



/**
* @brief Defines the various types of MV resolution used in codecs.
*
* @since NVDDK 1.0
* @par Header:
* Declared in %nvmm_video_ec.h
* @ingroup ??
*/
typedef enum
{
    // Specifies the Intra Frame.
    NvMMVideoDecEC_MV_IntegerPel = 0x1,

    // Specifies the Inter Frame
    NvMMVideoDecEC_MV_HalfPel = 0x2,

    // Specifies the B Frmae
    NvMMVideoDecEC_MV_QuarterPel = 0x4,

    // Max should always be the last value, used to pad the enum to 32bits
    NvMMVideoDecEC_MV_Force32                     = 0x7FFFFFFF
} NvMMVideoDecEC_MV;


/**
* @brief Defines the decoding status of macroblocks. This might is used during error concealment
*
* @since NVDDK 1.0
* @par Header:
* Declared in %nvmm_video_ec.h
* @ingroup ??
*/

typedef enum
{
    // current macroblock status is not decoded
    NvMMVideoDecEC_CurrentMBStatus_Decoded = 0x0,

    // current macroblock status is decoded
    NvMMVideoDecEC_CurrentMBStatus_NonDecoded = 0x1,

    // current macroblock status is to be used for regularizing
    NvMMVideoDecEC_CurrentMBStatus_Regularize = 0x2,

    // current macroblock status is coded as Inter
    NvMMVideoDecEC_CurrentMBStatus_Inter = 0x4,

    // Max should always be the last value, used to pad the enum to 32bits
    NvMMVideoDecEC_CurrentMBStatus_Force32                     = 0x7FFFFFFF

} NvMMVideoDecEC_CurrentMBStatus;



/**
 * @brief the macroes to be used while Error Concealment
 *
 * @since NVDDK 1.0
 * @par Header:
 * Declared in %nvmm_video_ec.h
 * @ingroup ??
 */

// minimum size of macroblock partition for MV involved is 8x8
#define MB_PART_8X8     1  
// minimum size of macroblock partition for MV involved is 4x4
#define MB_PART_4X4     2
// uninitialized MV value if it cannot be concealed
#define BLOCK_NOT_CONCEALED 0x7F7F7F7F
// macroes for search range detection 
#define K                   1
#define UP                  1
#define DOWN                2
#define LEFT                3
#define RIGHT               4
#define TOP_LEFT            5
#define TOP_RIGHT           6
#define BOTTOM_LEFT         7
#define BOTTOM_RIGHT        8
#define SEARCH_RANGE        4
#define SEARCH_RANGE_MVRI       6
#define MB_OUT_OF_RANGE     65535
#define MAXMVPOS            1023
#define MINMVPOS            -1024
#define NOT_FOUND           321
#define CHK_NOT_FOUND       321
#define BLOCK_FOUND         15
 
/**
 * @brief Defines the structure for holding the parameters required to carry out generic 
 * Error Concelament.
 *
 * @since NVDDK 1.0
 * @par Header:
 * Declared in %nvmm_video_ec.h
 * @ingroup ??
 */
typedef struct
{
    // minimum MV partition invloved
    NvU32   MinMVPart;
    // Width of picture in terms of MB
    NvU32   MBWidth;
    // Height of picture in terms of MB
    NvU32   MBHeight;
    // Height of picture in terms of MB
    NvU32   MBCount;
    // X Motion Vector for Current Frame
    NvS32   *pCurrMV;
    // X Motion Vector for Previous Frame
    NvS32   *pPrevMV;
    // X Motion Vector for Previous to Previous Frame
    NvS32   *pPrevToPrevMV;
    // temp MBstatus to store the status before EC
    NvU32   *tempMBStatus;
    // current decoding status of the macroblock
    NvU32   *MBStatus;
    // previous decoding status of the macroblock
    NvU32   *PrevMBStatus;
    // previous to previous decoding status of the macroblock
    NvU32   *PrevToPrevMBStatus;
    NvU32  *MBStatusForEC;
    // current Frame Number
    NvU32   CurrentFrameNumber;
    // current Frame Type
    NvU32   CurrentFrameType;
    // previous Frame Type
    NvU32   PreviousFrameType;
    // current x pos of MB
    NvS32   MBCol;
    // current y pos of MB
    NvS32   MBRow;
    // surface index for each macroblock
    NvU32   *RefIndex;
    // Number of Motion Vectors required to be stored per macroblock
    NvU32   NumberOfMVs;
    // Flag to indicate whether the individual block is the allocator for the MV array
    NvU32   FlagAllocMVArray;
    // Dividing factor for MV for mapping MV to MB
    NvU32   MVDivFactor;
}NvMMVideoDecECContext, *NvMMVideoDecECContextHandle;


/**
 *  Initialize the Error Concealment related data struture
 */
NvError NvMMVideoDecOpenEC(void **phErrorConcelment, NvU32 MBCount, NvU32 FlagAllocMVArray,
            NvU32 NumberOfMVs);
/*
 *  Call Error Concealment Algorithm for the entire frame. This Function will take care of concealing all the 
 *  macroblocks which are lost at the end of the frame
 */
void NvMMVideoDecConcealFrame(void *hErrorConcelment);

/**
 *  UnInitialize the Error Concealment related datata struture
 */
void NvMMVideoDecDeInitEC(NvMMVideoDecECContextHandle *phErrorConcelment);


/** The Input to this function is previous two frame motion vectors and 
 *  the output of this function is Motion Vectors for the macroblocks for 
 *  which  MBDoneFlag is MBNOTDONE. 
 */
void NvMMVideoDecECFirstPass(NvMMVideoDecECContextHandle  pECContext);
/** This is the second pass of the algorithm which does MVRI. 
 */
void NvMMVideoDecECSecondPass(NvMMVideoDecECContextHandle  pECContext);
/** This is the second pass of the algorithm which does smoothening using MVRI. 
 */
void NvMMVideoDecECThirdPass(NvMMVideoDecECContextHandle  pECContext);
/** fn finds the corrupted region & provids MB no.s of ends of corrupted area 
 */
void NvMMVideoDecECGetProcRegion(NvMMVideoDecECContextHandle  pECContext, NvU16 mb, 
                                 NvS16 *pTop_left_MB, NvS16 *pBottom_right_MB);
/** fn provides avg MV for a 4x4 block from previous two frames
 */
void NvMMVideoDecECGetMVHPred(NvMMVideoDecECContextHandle  pECContext, 
                              NvU16 mbcount, NvU8 partidx, NvU8 subpartidx, NvS16 *mvx, NvS16 *mvy);
/** from the MV provided, calculate the MB no and Part/subpart no in prev frm for given position in curr frm 
 *  MB no, partidx and subpartidx are written at consecutive locations pointed by *MbInOperation
 */
void NvMMVideoDecECMVToMBMap(NvMMVideoDecECContextHandle  pECContext, 
                             NvS16 mvx,NvS16 mvy,NvU16 mbcount,NvU16 *MbInOperation, 
                             NvU8 partidx, NvU8 subpartidx);
/** In this function Macroblocks are marked as BLOCK_NOT_CONCEALED if
  * Output of MVH contains less than two motion vectors or motion vector 
  * diff with in a macroblock is greater than a threshold 
  */
void NvMMVideoDecECRegularizeMVs(NvMMVideoDecECContextHandle  pECContext, NvU16 MBcnt1, NvU16 MBcnt2);
/** Algorithm to implement MVRI
 */
void NvMMVideoDecECMVRIWrapper(NvMMVideoDecECContextHandle pECContext,
                               NvU16 mb, NvU8 MBpartidx);
/** Block Based Motion Vector Rational Interpolation
 */
void 
NvMMVideoDecECMVRIBlock(NvMMVideoDecECContextHandle pECContext, NvS16 iCurrentMBNumber, 
                                     NvS16 *MV_x, NvS16 *MV_y,
                                     NvS16 top_MB, NvS16 top_right_MB, NvS16 top_left_MB, 
                                     NvS16 bottom_MB, NvS16 bottom_right_MB,
                                     NvS16 bottom_left_MB, NvU8 top_MB_partidx,
                                     NvU8 top_right_MB_partidx, NvU8 top_left_MB_partidx,
                                     NvU8 bottom_MB_partidx, NvU8 bottom_right_MB_partidx,
                                     NvU8 bottom_left_MB_partidx);
/** searches the blocks to be used by MVRI algorithm
 */
NvS16 NvMMVideoDecECsearchBlock(NvMMVideoDecECContextHandle pECContext, NvS32 search_MB_no, 
                NvU8 *MBpartIdx, NvS16 search_direction);
/** Finding the best referience index for selecting the reference frame
 */
NvU8  NvMMVideoDecECGetRefIdx(NvMMVideoDecECContextHandle pECContext, NvU8 *RefIdx, NvU8 N);
/** Evaluating weights for the motion vectors
 */
NvS32 NvMMVideoDecECWeight(NvMMVideoDecECContextHandle pECContext, NvS16 MVx1,NvS16 MVx2,NvS16 MVy1,NvS16 MVy2);



#if defined(__cplusplus)
}
#endif

/** @} */

#endif // INCLUDED_NVMM_VIDEODECBLOCK_H
