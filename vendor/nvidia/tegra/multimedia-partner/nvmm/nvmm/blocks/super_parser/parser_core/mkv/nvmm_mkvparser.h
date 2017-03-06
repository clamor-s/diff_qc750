/*
 * Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto. Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVMM_MKVPARSER_H
#define INCLUDED_NVMM_MKVPARSER_H

#include "nvmm.h"
#include "nvmm_parser.h"
#include "nvmm_mkvdemux.h"

typedef enum NvMMMkvParseState
{
    MKVPARSE_HEADER,
    MKVPARSE_DATA,
} NvMMMkvParseState;

typedef struct NvMkvReaderHandleRec
{
    NvMkvDemux *pDemux;
    NvParserFFREWStatus ffRewStatus;
    NvU32 audiofps;
    // Signifies if video stream is present in the given track
    NvBool IsVideoStreamPresent;
    NvS32 rate;
    NvS32 audioFrameCounter;
    NvS32 videoFrameCounter;
    NvS32 seekVideoFrameCounter;
    NvS32 tempCount;
    NvU32 maxAudioFrameSize;
    NvU32 maxVideoFrameSize;
    NvBool bStreaming;
    NvU32 bufferIncrement;
    NvBool bFirstFrame;
    NvU64 currFrameOffset;
    NvU64 prevFrameOffset;
} NvMkvReaderHandle;

/* Context for Mkv Parser (reader) core */
typedef struct NvMMMkvParserCoreContextRec
{
    NvMkvReaderHandle *pMkvReaderHandle;
    CPhandle cphandle;
    CP_PIPETYPE_EXTENDED *pPipe;
    NvMMAttrib_ParseRate rate;
    NvMMAttrib_ParsePosition position;
    NvMMMkvParseState ParseState[NVMM_MKVPARSER_MAX_TRACKS];
    NvU32 audiofps[NVMM_MKVPARSER_MAX_TRACKS];
    NvU32 videofps;
    NvU64 videoDuration;
    NvU64 audioDuration;
    NvU64 totalDuration;
    // specifies vertical size of pixel aspect ratio
    NvU32 Width;
    // specifies horizontal size of pixel aspect ratio
    NvU32 Height;
    // Max Video Frame size
    NvU32 MaxVidFrameSize;
    // Audio Bitrate
    NvU32 AudioBitrate;
    // Indicates file reached to EOF while seeking
    NvBool bFileReachedEOF;
    NvU64 CurrentTimeStamp;
} NvMMMkvParserCoreContext;

/* API functions called from core parser */
NvError NvMkvInitializeParser (
    CPhandle hContent,
    CP_PIPETYPE_EXTENDED *pPipe,
    NvMkvReaderHandle *pMkvReaderHandle,
    NvBool bStreaming) ;

NvError NvMkvInitializeDemux (
    NvMkvDemux * self,
    CPhandle hContent,
    CP_PIPETYPE_EXTENDED *pPipe,
    NvBool bStreaming);

NvError NvMkvFileParse (NvMkvReaderHandle * handle);

NvMkvParserStatus NvMkvGetAudioProps (
    NvMMStreamInfo* currentStream,
    NvMkvReaderHandle* handle,
    NvU32 trackIndex,
    NvS32* maxBitRate);

NvMkvParserStatus NvMkvGetVideoProps (
    NvMMStreamInfo* currentStream,
    NvMkvReaderHandle* handle,
    NvU32 trackIndex);

NvError NvMkvReaderGetPacket (
    NvMkvReaderHandle * handle,
    NvU32 index ,
    void * buffer,
    NvU32 *frameSize,
    NvU64 *pts);

void NvMkvReleaseReaderHandle (NvMkvReaderHandle * handle);

NvU32 NvMkvReaderGetNumTracks (NvMkvReaderHandle * handle);

NvS32  NvMkvReaderGetTracksInfo (
    NvMkvReaderHandle * handle,
    NvS32 index);

NvError NvMkvReaderSetPosition (
    NvMMMkvParserCoreContext *pContext,
    NvU64 *pSeekTime) ;

/* Internally called */
NvError NvMkvFileHeaderParse (NvMkvDemux *demux);

NvError NvMkvFileCheck (NvMkvDemux *demux);

NvMkvParserStatus NvMKVReaderSetTrack (
    NvMkvReaderHandle * handle,
    NvS32 index) ;

NvMkvParserStatus NvMkvReaderGetNALSize (
    NvMkvReaderHandle * handle,
    NvS32 index,
    NvS32 * nalSize);

NvMkvParserStatus NvMkvDisableTrackNum (
    NvMkvReaderHandle * handle,
    NvS32 index);

NvMkvParserStatus NvMkvEnableTrackNum (
    NvMkvReaderHandle * handle,
    NvS32 index);

NvU64 NvMkvReaderGetMediaTimeScale (
    NvMkvReaderHandle * handle,
    NvS32 index);

void *NvMkvReaderGetDecConfig (
    NvMkvReaderHandle * handle,
    NvS32 index,
    NvS32 * size);

void NvReleaseMkvReaderHandle (NvMkvReaderHandle * handle);

/* For Thumbnail display based on largest I frame  */
NvError NvMkvScanIFrame (NvMkvDemux *matroska);

#endif // INCLUDED_NVMM_MKVPARSER_H


