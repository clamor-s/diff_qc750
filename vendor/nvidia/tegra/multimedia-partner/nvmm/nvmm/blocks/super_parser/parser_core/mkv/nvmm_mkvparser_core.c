/*
 * Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvassert.h"
#include "nvmm_aac.h"
#include "nvmm_mp3.h"
#include "nvmm_contentpipe.h"
#include "nvmm_debug.h"
#include "nvmm_metadata.h"
#include "nvmm_parser.h"
#include "nvmm_mkvparser_core.h"
#include "nvmm_util.h"
#include "nvmm_ulp_util.h"
#include "nvmm_videodec.h"
#include "nvmm_mkvparser.h"

enum
{
    AAC_NUM_CHANNELS_SUPPORTED = 6,
    AAC_DEC_MAX_INPUT_BUFFER_SIZE = (768 * AAC_NUM_CHANNELS_SUPPORTED),
    AAC_DEC_MIN_INPUT_BUFFER_SIZE = (768 * AAC_NUM_CHANNELS_SUPPORTED),
    MP3_DEC_MIN_INPUT_BUFFER_SIZE = (2 * 1440),  // (bit_rate_bps/8)/(sample_rate/1152)
    MP3_DEC_MAX_INPUT_BUFFER_SIZE = (2 * 1440),  // lowest Fs = 32KHz and highest bit rate is 320kbps => 1440. Allow twice this size
    VORBIS_DEC_MAX_INPUT_BUFFER_SIZE = (10 * 1024), // Keep it large for vorbis headers 0, 1 and 2 which contain arbitrary sized ancillary data
    VORBIS_DEC_MIN_INPUT_BUFFER_SIZE = (10 * 1024),
    AC3_DEC_MIN_INPUT_BUFFER_SIZE = (2 * 2048),
    AC3_DEC_MAX_INPUT_BUFFER_SIZE = (2 * 2048),
    MAX_VIDEO_BUFFERS = 5,
};

static NvError
NvMkvCoreParserGetNumberOfStreams (
    NvMMParserCoreHandle hParserCore,
    NvU32 *streams)
{
    NvError status = NvSuccess;
    NvMMMkvParserCoreContext* pContext = (NvMMMkvParserCoreContext *) hParserCore->pContext;
    if (pContext == NULL)
    {
        *streams = 0;
        return NvError_ParserFailure;
    }
    *streams = NvMkvReaderGetNumTracks (pContext->pMkvReaderHandle);
    return status;
}

static NvBool
NvMkvCoreParserGetBufferRequirements (
    NvMMParserCoreHandle hParserCore,
    NvU32 StreamIndex,
    NvU32 Retry,
    NvMMNewBufferRequirementsInfo *pBufReq)
{
    NvMMMkvParserCoreContext *pContext = (NvMMMkvParserCoreContext *) hParserCore->pContext;
    NvS32 track;
    NvU32 NoOfStreams = 0;
    NvError Status = NvSuccess;
    NvBool Video_Flag = NV_FALSE;
    NvBool IsTrackTypeAudio = NV_FALSE;

    NvF32 BufferedDuration = 0.0;
    NvU32 i;
    NVMM_CHK_ARG (Retry == 0);
    NvOsMemset (pBufReq, 0, sizeof (NvMMNewBufferRequirementsInfo));
    pBufReq->structSize    = sizeof (NvMMNewBufferRequirementsInfo);
    pBufReq->event         = NvMMEvent_NewBufferRequirements;
    pBufReq->minBuffers    = 4;
    pBufReq->maxBuffers    = NVMMSTREAM_MAXBUFFERS;
    pBufReq->memorySpace   = NvMMMemoryType_SYSTEM;
    pBufReq->byteAlignment = 4;
    
    NVMM_CHK_ERR(NvMkvCoreParserGetNumberOfStreams (hParserCore, &NoOfStreams));
    // Calculate the BufferedDuration of video for given i/p buffers, buffersize and video bitrate
    if (!Video_Flag)
    {
        for (i = 0; i < NoOfStreams; i++)
        {
            track = (NvMkvTrackTypes) NvMkvReaderGetTracksInfo (pContext->pMkvReaderHandle, i);
            switch (track)
            {
                case NvMkvCodecType_MPEG4:
                case NvMkvCodecType_MPEG2:
                case NvMkvCodecType_AVC:
                    // Calculated Buffered duration of file using fps field and number of frames sent to decoder
                    if (pContext->videofps)
                    {
                        Video_Flag = NV_TRUE;
                        BufferedDuration = MAX_VIDEO_BUFFERS / ( (NvF32) pContext->videofps / FLOAT_MULTIPLE_FACTOR);
                        break;
                    }
            }
            if (Video_Flag)
                break;
        }
    }

    track = (NvMkvTrackTypes) NvMkvReaderGetTracksInfo (pContext->pMkvReaderHandle, StreamIndex);
    IsTrackTypeAudio = NV_FALSE;

    switch (track)
    {
        case NvMkvCodecType_AAC:
        case NvMkvCodecType_AACSBR:
        {
            pBufReq->minBufferSize = AAC_DEC_MIN_INPUT_BUFFER_SIZE;
            pBufReq->maxBufferSize = AAC_DEC_MAX_INPUT_BUFFER_SIZE;
            IsTrackTypeAudio = NV_TRUE;
            break;
        }
        case NvMkvCodecType_MP3:
        {
            pBufReq->minBufferSize = MP3_DEC_MIN_INPUT_BUFFER_SIZE;
            pBufReq->maxBufferSize = MP3_DEC_MAX_INPUT_BUFFER_SIZE;
            IsTrackTypeAudio = NV_TRUE;
            break;
        }
        case NvMkvCodecType_VORBIS:
        {
            pBufReq->minBufferSize = VORBIS_DEC_MIN_INPUT_BUFFER_SIZE;
            pBufReq->maxBufferSize = VORBIS_DEC_MAX_INPUT_BUFFER_SIZE;
            IsTrackTypeAudio = NV_TRUE;
            break;
        }
        case NvMkvCodecType_AC3:
        {
            pBufReq->minBufferSize = AC3_DEC_MIN_INPUT_BUFFER_SIZE;
            pBufReq->maxBufferSize = AC3_DEC_MAX_INPUT_BUFFER_SIZE;
            IsTrackTypeAudio = NV_TRUE;
            break;
        }
        case NvMkvCodecType_MPEG4:
        case NvMkvCodecType_MPEG2:
        case NvMkvCodecType_AVC:
        {
            // reduced memory mode
            if (hParserCore->bReduceVideoBuffers)
            {
                pBufReq->minBuffers = MAX_VIDEO_BUFFERS;
                pBufReq->maxBuffers = MAX_VIDEO_BUFFERS;
            }
            if (hParserCore->bEnableUlpMode == NV_TRUE)
            {
                pBufReq->minBufferSize = sizeof (NvMMOffsetList) + (sizeof (NvMMOffset) * MAX_OFFSETS);
                pBufReq->maxBufferSize = sizeof (NvMMOffsetList) + (sizeof (NvMMOffset) * MAX_OFFSETS);
                pBufReq->minBuffers = 6;
                pBufReq->maxBuffers = 6;
            }
            else
            {
                if ( (pContext->Width == 0) || (pContext->Height == 0))
                {
                    pBufReq->minBufferSize = 32768;
                    pBufReq->maxBufferSize = 512000 * 2 - 32768;
                }
                else
                {
                    if ( (pContext->Width > 320) && (pContext->Height > 240))
                    {
                        pBufReq->minBufferSize = (pContext->Width * pContext->Height * 3) >> 2;
                        pBufReq->maxBufferSize = (pContext->Width * pContext->Height * 3) >> 2;
                    }
                    else
                    {
                        pBufReq->minBufferSize = (pContext->Width * pContext->Height * 3) >> 1;
                        pBufReq->maxBufferSize = (pContext->Width * pContext->Height * 3) >> 1;
                    }
                }
                if (pBufReq->maxBufferSize < 1024)
                {
                    pBufReq->maxBufferSize = 1024;
                    pBufReq->minBufferSize = 1024;
                }
                pContext->MaxVidFrameSize = pBufReq->maxBufferSize;
            }
            break;
        }
        case NvMkvCodecType_NONE:
        case NvMkvCodecType_UNKNOWN:
            pBufReq->minBufferSize = pBufReq->maxBufferSize = 0;
            break;
        default:
            break;
    }
    if (IsTrackTypeAudio && Video_Flag)
    {
        // Calculate the number of audiobuffers required in given BufferedDuration.
        pBufReq->minBuffers = (NvU32) (BufferedDuration * pContext->audiofps[StreamIndex]);
        // Minbuffers should n't be less than 2(1 at parser and 1 at decoder)
        if (pBufReq->minBuffers < 5)
            pBufReq->minBuffers = 5;
        else if (pBufReq->minBuffers > 20)
            pBufReq->minBuffers = 20;

        pBufReq->maxBuffers = pBufReq->minBuffers;
    }

cleanup:
    return (Status != NvSuccess) ? NV_FALSE : NV_TRUE;

}

static NvError
NvMkvCoreParserClose (
    NvMMParserCoreHandle hParserCore)
{
    NvError status = NvSuccess;
    NvMMMkvParserCoreContext *pContext = (NvMMMkvParserCoreContext *) hParserCore->pContext;
    if (pContext)
    {
        if (pContext->cphandle)
        {
            if (pContext->pPipe->cpipe.Close (pContext->cphandle) != NvSuccess)
            {
                status = NvError_ParserCloseFailure;
            }
            pContext->cphandle = 0;
        }
        if (pContext->pMkvReaderHandle)
        {
            NvMkvReleaseReaderHandle (pContext->pMkvReaderHandle);
        }
        pContext->pMkvReaderHandle = NULL;
        NvOsFree (pContext);
    }
    if (hParserCore->hRmDevice)
    {
        NvRmClose (hParserCore->hRmDevice);
    }
    hParserCore->pContext = NULL;
    return status;
}

static NvError
NvMkvCoreParserOpen (
    NvMMParserCoreHandle hParserCore,
    NvString szURI)
{
    NvError Status = NvSuccess;
    NvMMMkvParserCoreContext *pContext = NULL;
    NvBool bStreaming = NV_FALSE;

    NVMM_CHK_ARG (hParserCore && szURI);
    pContext = NvOsAlloc (sizeof (NvMMMkvParserCoreContext));
    NVMM_CHK_MEM (pContext);
    NvOsMemset (pContext, 0, sizeof (NvMMMkvParserCoreContext));
    if ( (hParserCore->MinCacheSize == 0) && (hParserCore->MaxCacheSize == 0) && (hParserCore->SpareCacheSize == 0))
    {
        hParserCore->bUsingCachedCP = NV_FALSE;
        hParserCore->bEnableUlpMode = NV_FALSE;
    }
    else
    {
        hParserCore->bUsingCachedCP = NV_TRUE;
    }
    NvmmGetFileContentPipe (&pContext->pPipe);
    NVMM_CHK_ERR (pContext->pPipe->cpipe.Open (&pContext->cphandle, szURI, CP_AccessRead));
    if (pContext->pPipe->IsStreaming (pContext->cphandle))
        bStreaming = NV_TRUE;

    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,
    "Streaming status = %d\n", bStreaming));
    // Create Handle
    pContext->pMkvReaderHandle = (NvMkvReaderHandle *) NvOsAlloc (sizeof (NvMkvReaderHandle));
    NVMM_CHK_MEM (pContext->pMkvReaderHandle);
    NvOsMemset(pContext->pMkvReaderHandle, 0, sizeof(NvMkvReaderHandle));   

    NVMM_CHK_ERR (NvMkvInitializeParser (pContext->cphandle, pContext->pPipe, pContext->pMkvReaderHandle, bStreaming));
    NVMM_CHK_ERR ( NvMkvFileParse (pContext->pMkvReaderHandle));
      // If there are no tracks in file, exit
    if (0 == NvMkvReaderGetNumTracks (pContext->pMkvReaderHandle))
    {
        Status = NvError_BadParameter;
        goto cleanup;
    }
    pContext->rate.Rate  = 1000;
    hParserCore->pPipe    = pContext->pPipe;
    hParserCore->cphandle = pContext->cphandle;
    hParserCore->pContext = (void*) pContext;
    return Status;
cleanup:
    NvMkvCoreParserClose(hParserCore);
    return Status;
}

static NvError
NvMkvCoreParserGetStreamInfo (
    NvMMParserCoreHandle hParserCore,
    NvMMStreamInfo **pInfo)
{
    NvError status = NvSuccess;
    NvU32 i, totalTracks;
    NvU64 mediaTimeScale = 0;
    NvMkvTrackTypes trackType;
    NvS32 maxBitrate;
    NvBool bIsAudio, bIsVideo;
    NvMMMkvParserCoreContext* pContext = (NvMMMkvParserCoreContext *) hParserCore->pContext;
    NvMkvDemux *d = NULL;
    NvMkvReaderHandle * handle = NULL;
    NvMkvParserStatus mkvStatus = NvMkvParserStatus_NOERROR;
    NvMMStreamInfo* curStreamInfo = * (pInfo);
    //Check for pContext validity
    if ( (!pContext) || (!pContext->pMkvReaderHandle))
        return NvError_ParserFailure;

    // Get Mkv Handle
    handle = pContext->pMkvReaderHandle;
    totalTracks = NvMkvReaderGetNumTracks (handle);
    handle->IsVideoStreamPresent = NV_FALSE;
    d = (NvMkvDemux *) handle->pDemux;
    bIsAudio = bIsVideo = NV_FALSE;
    for (i = 0; i < totalTracks; i++)
    {
        trackType = (NvMkvTrackTypes) NvMkvReaderGetTracksInfo (handle , i) ;
        // Set Block Type as Mkv parser
        curStreamInfo[i].BlockType = NvMMBlockType_SuperParser;
        switch (trackType)
        {
            case NvMkvCodecType_AAC:
                if (!bIsAudio)
                {
                    curStreamInfo[i].StreamType = NvMMStreamType_AAC;
                    handle->maxAudioFrameSize = AAC_DEC_MAX_INPUT_BUFFER_SIZE;
                    mkvStatus = NvMkvGetAudioProps (&curStreamInfo[i], handle, i, &maxBitrate);
                    if (mkvStatus != NvMkvParserStatus_NOERROR)
                        status = NvError_ParserFailure;
                }
                break;
            case NvMkvCodecType_MP3:
                if (!bIsAudio)
                {
                    curStreamInfo[i].StreamType = NvMMStreamType_MP3;
                    handle->maxAudioFrameSize = MP3_DEC_MAX_INPUT_BUFFER_SIZE;
                    mkvStatus = NvMkvGetAudioProps (&curStreamInfo[i], handle, i, &maxBitrate);
                    if (mkvStatus != NvMkvParserStatus_NOERROR)
                        status = NvError_ParserFailure;
                }
                break;
            case NvMkvCodecType_AC3:
                if (!bIsAudio)
                {
                    curStreamInfo[i].StreamType = NvMMStreamType_AC3;
                    handle->maxAudioFrameSize = AC3_DEC_MAX_INPUT_BUFFER_SIZE;
                    mkvStatus = NvMkvGetAudioProps (&curStreamInfo[i], handle, i, &maxBitrate);
                    if (mkvStatus != NvMkvParserStatus_NOERROR)
                        status = NvError_ParserFailure;
                }
                break;
            case NvMkvCodecType_VORBIS:
                if (!bIsAudio)
                {
                    curStreamInfo[i].StreamType = NvMMStreamType_OGG;
                    handle->maxAudioFrameSize = VORBIS_DEC_MAX_INPUT_BUFFER_SIZE;
                    mkvStatus = NvMkvGetAudioProps (&curStreamInfo[i], handle, i, &maxBitrate);
                    if (mkvStatus != NvMkvParserStatus_NOERROR)
                        status = NvError_ParserFailure;
                }
                break;
            case NvMkvCodecType_MPEG4:
                if (!bIsVideo)
                {
                    curStreamInfo[i].StreamType = NvMMStreamType_MPEG4;
                    mkvStatus = NvMkvGetVideoProps (&curStreamInfo[i], handle, i);
                    if (mkvStatus != NvMkvParserStatus_NOERROR)
                        status = NvError_ParserFailure;
                }
                break;
            case NvMkvCodecType_MPEG2:
                if (!bIsVideo)
                {
                    curStreamInfo[i].StreamType = NvMMStreamType_MPEG2V;
                    mkvStatus = NvMkvGetVideoProps (&curStreamInfo[i], handle, i);
                    if (mkvStatus != NvMkvParserStatus_NOERROR)
                        status = NvError_ParserFailure;
                }
                break;
            case NvMkvCodecType_AVC:
                if (!bIsVideo)
                {
                    curStreamInfo[i].StreamType = NvMMStreamType_H264;
                    mkvStatus = NvMkvGetVideoProps (&curStreamInfo[i], handle, i);
                    if (mkvStatus != NvMkvParserStatus_NOERROR)
                        status = NvError_ParserFailure;
                }
                break;
            case NvMkvCodecType_NONE:
            case NvMkvCodecType_UNKNOWN:
            default:
                curStreamInfo[i].StreamType = NvMMStreamType_OTHER;
                break;
        }
        mediaTimeScale = NvMkvReaderGetMediaTimeScale (handle, i);
        curStreamInfo[i].TotalTime = curStreamInfo[i].TotalTime * mediaTimeScale / 100;
        NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,
        "Duration of track is %llu\n", curStreamInfo[i].TotalTime));
        if ( (!bIsVideo) && (NVMKV_ISTRACKVIDEO (trackType)))
        {
            pContext->videoDuration = curStreamInfo[i].TotalTime;
            handle->maxVideoFrameSize = pContext->MaxVidFrameSize;
            handle->IsVideoStreamPresent = NV_TRUE;
            pContext->ParseState[i] = MKVPARSE_HEADER;
            if (curStreamInfo[i].NvMMStream_Props.VideoProps.VideoBitRate == 0)
            {
                if (curStreamInfo[i].TotalTime != 0)
                {
                    // calculate the average bit rate somehow at this point. This may be necessary  TBD FIXME
                    // curStreamInfo[i].NvMMStream_Props.VideoProps.VideoBitRate = d->videoMediaData.avgBitRate;
                }
            }
            pContext->videofps = curStreamInfo[i].NvMMStream_Props.VideoProps.Fps;
            pContext->Width = curStreamInfo[i].NvMMStream_Props.VideoProps.Width;
            pContext->Height = curStreamInfo[i].NvMMStream_Props.VideoProps.Height;
            if (d->IdentifiedKeyThumbNailFrame || d->ScanEntry)
            {
                d->largestKeyTimeStamp = (d->largestKeyTimeStamp*mediaTimeScale)/100;
                NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,
                "MKVStreamInfo:Found Thumbnail MTS_LKTS = %llu\n", d->largestKeyTimeStamp));
            }
            // We play only one video. So set this variable to TRUE and ignore further video tracks if present.
            bIsVideo = NV_TRUE;
        }
        if ( (!bIsAudio) && (NVMKV_ISTRACKAUDIO (trackType)))
        {
            // Update Audio FPS to the reader handle
            handle->audiofps = pContext->audiofps[i];
            pContext->audioDuration = curStreamInfo[i].TotalTime;
            pContext->AudioBitrate = curStreamInfo[i].NvMMStream_Props.AudioProps.BitRate;
            pContext->ParseState[i] = MKVPARSE_HEADER;
            if (trackType == NvMkvCodecType_MPEG2)
            {
                pContext->ParseState[i] = MKVPARSE_DATA;
            }
            // We play only one audio. So set this variable to TRUE and ignore further audio tracks if present.
            bIsAudio = NV_TRUE;
        }
    }
    if (pContext->audioDuration < pContext->videoDuration)
        pContext->totalDuration = pContext->videoDuration;
    else
        pContext->totalDuration = pContext->audioDuration;

    return status;
}

static NvError
NvMkvCoreParserSetRate (
    NvMMParserCoreHandle hParserCore,
    NvS32 rate)
{
    NvMMMkvParserCoreContext* pContext = (NvMMMkvParserCoreContext *) hParserCore->pContext;
    if (pContext == NULL)
    {
        return NvError_ParserFailure;
    }
    pContext->rate.Rate = rate;
    return NvSuccess;
}

static NvS32
NvMkvCoreParserGetRate (
    NvMMParserCoreHandle hParserCore)
{
    NvMMMkvParserCoreContext* pContext = (NvMMMkvParserCoreContext *) hParserCore->pContext;
    return pContext->rate.Rate;
}

static NvError
NvMkvCoreParserGetPosition (
    NvMMParserCoreHandle hParserCore,
    NvU64 *timestamp)
{
    NvError status = NvSuccess;
    *timestamp = ( (NvMMMkvParserCoreContext *) hParserCore->pContext)->position.Position;
    return status;
}

static NvError
NvMkvCoreParserSetPosition (
    NvMMParserCoreHandle hParserCore,
    NvU64 *timestamp)
{
    NvError Status = NvSuccess;
    NvMMMkvParserCoreContext* pContext = NULL;
    NVMM_CHK_ARG (hParserCore && hParserCore->pContext);
    pContext = (NvMMMkvParserCoreContext *) hParserCore->pContext;
    // Convert ticks from app specific units in 10 MHz clock units into MKV parser specific in ns
    *timestamp = *timestamp * 100;
    Status = NvMkvReaderSetPosition(pContext, timestamp);
    *timestamp = *timestamp / 100;
    // Update the context position according to the timestamp
    ( (NvMMMkvParserCoreContext *) hParserCore->pContext)->position.Position = *timestamp;
cleanup:
    return Status;
}

static NvError
NvMkvCoreParserGetNextWorkUnit (
    NvMMParserCoreHandle hParserCore,
    NvU32 streamIndex,
    NvMMBuffer *pBuffer,
    NvU32 *size,
    NvBool *pMoreWorkPending)
{
    NvError Status = NvSuccess;
    NvMMMkvParserCoreContext* pContext;
    NvS8 *tempAVCBufferptr;
    NvU64  pts  = 0;    // presentation timestamp
    NvU64 mediaTimeScale = 0;
    NvU32 nFilledLen = pBuffer->Payload.Ref.sizeOfBufferInBytes;
    void *headerData = NULL;
    NvS32 headerSize = 0;
    NvS32 nalSize;
    NvMkvDemux *pDemux = NULL;
    NvU32 uLength;
    NvS32 track, rate = 1000;
    NvMkvReaderHandle * mkvReaderHandle;
    NvAVCConfigData *avcConfig;

    // Initially pMoreWorkPending is set to FALSE
    *pMoreWorkPending = NV_FALSE;
    NVMM_CHK_ARG (hParserCore && hParserCore->pContext);
    pContext = (NvMMMkvParserCoreContext *) hParserCore->pContext;

    pDemux = (NvMkvDemux *) pContext->pMkvReaderHandle->pDemux;
    mkvReaderHandle = pContext->pMkvReaderHandle;
    track = NvMkvReaderGetTracksInfo (mkvReaderHandle, streamIndex);
    *size = 0;
    switch (pContext->ParseState[streamIndex])
    {
        case MKVPARSE_DATA:
            rate = NvMkvCoreParserGetRate (hParserCore);
            mkvReaderHandle->rate  = rate;
            Status = NvMkvReaderGetPacket (
                mkvReaderHandle,
                streamIndex ,
                (NvS8 *) pBuffer->Payload.Ref.pMem,
                &nFilledLen,
                (NvU64*)&pts);
            if (Status == NvSuccess)
            {
                *size = nFilledLen;
                //matroska->time_scale and track->time_scale exist. use them both to get this!
                mediaTimeScale = NvMkvReaderGetMediaTimeScale (mkvReaderHandle, streamIndex);
                pBuffer->Payload.Ref.sizeOfValidDataInBytes = nFilledLen;
                // MKV provides timestamps in GHz/ns to external interface. NVMM requires in 100ns units. (converting 1GHz into 10MHz range)
                if (mediaTimeScale != 0)
                    pBuffer->PayloadInfo.TimeStamp = pts / 100;
                else
                    pBuffer->PayloadInfo.TimeStamp = 0;

                if (NVMKV_ISTRACKVIDEO (track))
                    pContext->CurrentTimeStamp = pBuffer->PayloadInfo.TimeStamp;

                pBuffer->Payload.Ref.startOfValidData = 0;
            }
            if (track == NvMkvCodecType_AVC)
            {
                nalSize = 0;
                NvMkvReaderGetNALSize (mkvReaderHandle, streamIndex , &nalSize);
                pBuffer->PayloadInfo.BufferMetaDataType = NvMBufferMetadataType_H264;
                pBuffer->PayloadInfo.BufferMetaData.H264BufferMetadata.NALUSizeFieldWidthInBytes = nalSize + 1;
                pBuffer->PayloadInfo.BufferMetaData.H264BufferMetadata.BufferFormat = NvH264BufferFormat_RTPStreamFormat;
            }
            break;
            /* Whenever the first call to this function occurs, we need to send the header info to the caller
             * Some standard prototypes exist for this information depending on codec type. The state
             * machine is moved to HEADER state after init and upon seek to start of file
             */
            /* AAC and AVC decoders have a specific structure for the header information */
        case MKVPARSE_HEADER:
            if (track == NvMkvCodecType_AVC)
            {
                tempAVCBufferptr    = (NvS8 *) pBuffer->Payload.Ref.pMem;
                avcConfig           = &pDemux->avcConfigData;

                uLength = avcConfig->seqParamSetLength[0];
                tempAVCBufferptr[0] = (NvU8) ( (uLength >> 24) & 0xFF);
                tempAVCBufferptr[1] = (NvU8) ( (uLength >> 16) & 0xFF);
                tempAVCBufferptr[2] = (NvU8) ( (uLength >> 8) & 0xFF);
                tempAVCBufferptr[3] = (NvU8) ( (uLength) & 0xFF);
                NvOsMemcpy (tempAVCBufferptr + 4, avcConfig->spsNalUnit[0], uLength);

                tempAVCBufferptr    += uLength + 4;
                uLength             = avcConfig->picParamSetLength[0];
                tempAVCBufferptr[0] = (NvU8) ( (uLength >> 24) & 0xFF);
                tempAVCBufferptr[1] = (NvU8) ( (uLength >> 16) & 0xFF);
                tempAVCBufferptr[2] = (NvU8) ( (uLength >> 8) & 0xFF);
                tempAVCBufferptr[3] = (NvU8) ( (uLength) & 0xFF);
                NvOsMemcpy (tempAVCBufferptr + 4, avcConfig->ppsNalUnit[0], uLength);
                headerSize          = avcConfig->seqParamSetLength[0] + avcConfig->picParamSetLength[0] + 8;

                pBuffer->PayloadInfo.BufferMetaDataType = NvMBufferMetadataType_H264;
                pBuffer->PayloadInfo.BufferMetaData.H264BufferMetadata.NALUSizeFieldWidthInBytes = 2;
                pBuffer->PayloadInfo.BufferMetaData.H264BufferMetadata.BufferFormat = NvH264BufferFormat_RTPStreamFormat;

            }
            else if (track == NvMkvCodecType_AAC)
            {
                NvMMAudioAacTrackInfo aacTrackInfo;
                NvMkvAacConfigurationData *aacData = NULL;
                aacData = &pDemux->aacConfigurationData;
                aacTrackInfo.objectType = aacData->objectType;
                aacTrackInfo.samplingFreqIndex = aacData->samplingFreqIndex;
                aacTrackInfo.samplingFreq = aacData->samplingFreq;
                aacTrackInfo.channelConfiguration = aacData->channelConfiguration;
                aacTrackInfo.bitRate = aacData->bitrate;
                aacTrackInfo.noOfChannels = aacData->noOfChannels;

                NvOsMemcpy (pBuffer->Payload.Ref.pMem, (NvU8*) (&aacTrackInfo), sizeof (NvMMAudioAacTrackInfo));
                headerSize = sizeof (NvMMAudioAacTrackInfo);
                NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,  "AAC audio track found\n"));
            }
            else if (track == NvMkvCodecType_VORBIS)
            {
                headerData = NvMkvReaderGetDecConfig (mkvReaderHandle, streamIndex, &headerSize);
                NvOsMemcpy (pBuffer->Payload.Ref.pMem, (NvU8*) (headerData), headerSize);
                NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,  "Vorbis track found\n"));
            }
            else
            {
                headerData = NvMkvReaderGetDecConfig (mkvReaderHandle, streamIndex, &headerSize);
                NvOsMemcpy (pBuffer->Payload.Ref.pMem, (NvU8*) (headerData), headerSize);
            }

            pBuffer->Payload.Ref.sizeOfValidDataInBytes = headerSize;
            pBuffer->Payload.Ref.startOfValidData = 0;
            pBuffer->PayloadInfo.TimeStamp = 0;
            pBuffer->PayloadInfo.BufferFlags |= NvMMBufferFlag_HeaderInfoFlag;
            *size = headerSize;
            pContext->ParseState[streamIndex] = MKVPARSE_DATA;
            break;

        default:
            Status = NvError_ParserFailedToGetData; // error we are trying to read data before setting the track
            break;
    }
cleanup:
    return Status;
}
    
static NvError
NvMkvCoreParserGetAttribute (
    NvMMParserCoreHandle hParserCore,
    NvU32 AttributeType,
    NvU32 AttributeSize,
    void *pAttribute)
{
    NvError Status = NvSuccess;
    NvMkvParserStatus err = NvMkvParserStatus_NOERROR;
    NvMkvDemux *matroska = NULL;
    NvMMMetaDataInfo *md = NULL;
    NvMMMkvParserCoreContext *pContext = NULL;
    NvU32 i, streams, avcIndex = 0;
    NvS32 track, nalLen, *tempNalLength;
    
    NVMM_CHK_ARG (hParserCore && hParserCore->pContext);
    pContext = (NvMMMkvParserCoreContext *) hParserCore->pContext;
    switch (AttributeType)
    {
        // Specifically related to H264 content.
        case NvMMVideoDecAttribute_H264NALLen:
        {
            streams = NvMkvReaderGetNumTracks (pContext->pMkvReaderHandle);
            tempNalLength = (NvS32*) pAttribute;
            for (i = 0; i < streams; i++)
            {
                track = (NvMkvTrackTypes) NvMkvReaderGetTracksInfo (pContext->pMkvReaderHandle, i);
                switch (track)
                {
                    case NvMkvCodecType_AVC:
                        avcIndex = i;
                        break;
                    default:
                        break;
                }
            }
            err = NvMkvReaderGetNALSize (pContext->pMkvReaderHandle, avcIndex, &nalLen);
            if (err != NvMkvParserStatus_NOERROR)
            {
                Status = NvError_ParserFailure;
            }
            else
            {
                *tempNalLength = (nalLen + 1);
            }
        }
        break;
        case NvMMAttribute_Metadata:
        {
            NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_INFO, "NvMMAttribute_Metadata\n"));
            md = (NvMMMetaDataInfo *) pAttribute;
            switch (md->eMetadataType)
            {
                case NvMMMetaDataInfo_ThumbnailSeekTime: // For Thumbnail generation
                {
                    if (md->pClientBuffer == NULL || (md->nBufferSize < sizeof (NvU64)))
                    {
                        md->nBufferSize = sizeof (NvU64);
                        Status = NvError_InSufficientBufferSize;
                        goto cleanup;
                    }
                    if (pContext->pMkvReaderHandle->pDemux)
                    {
                        matroska = (NvMkvDemux *) pContext->pMkvReaderHandle->pDemux;
                        NvOsMemcpy (md->pClientBuffer, &(matroska->largestKeyTimeStamp), sizeof (NvU64));
                        NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,
                        "GetAtribute: Matroska->largestKeyTimeStamp %llu\n",
                        matroska->largestKeyTimeStamp));
                        md->nBufferSize = sizeof (NvU64);
                        md->eEncodeType = NvMMMetaDataEncode_NvU64;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        break;
        default:
            break;
    }
cleanup:
    return Status;
}

static void
NvMkvCoreParserGetMaxOffsets (
    NvMMParserCoreHandle hParserCore,
    NvU32 *pMaxOffsets)
{
    *pMaxOffsets = MAX_OFFSETS;
}

NvError
NvMMCreateMkvCoreParser (
    NvMMParserCoreHandle *hParserCore,
    NvMMParserCoreCreationParameters *pParams)
{
    NvError status                     = NvSuccess;
    NvMMParserCore *pParserCore        = NULL;
    if ( (pParserCore = NvOsAlloc (sizeof (NvMMParserCore))) == NULL)
    {
        status = NvError_InsufficientMemory;
        goto NvMMMkvParserCoreOpen_err;
    }
    NvOsMemset (pParserCore, 0, sizeof (NvMMParserCore));

    if ( (status = NvRmOpen (&pParserCore->hRmDevice, 0)) != NvSuccess)
    {
        goto NvMMMkvParserCoreOpen_err;
    }
    pParserCore->Open               = NvMkvCoreParserOpen;
    pParserCore->GetNumberOfStreams = NvMkvCoreParserGetNumberOfStreams;
    pParserCore->GetStreamInfo      = NvMkvCoreParserGetStreamInfo;
    pParserCore->SetRate            = NvMkvCoreParserSetRate;
    pParserCore->GetRate            = NvMkvCoreParserGetRate;
    pParserCore->SetPosition        = NvMkvCoreParserSetPosition;
    pParserCore->GetPosition        = NvMkvCoreParserGetPosition;
    pParserCore->GetNextWorkUnit    = NvMkvCoreParserGetNextWorkUnit;
    pParserCore->Close              = NvMkvCoreParserClose;
    pParserCore->GetMaxOffsets      = NvMkvCoreParserGetMaxOffsets;
    pParserCore->GetAttribute       = NvMkvCoreParserGetAttribute;
    pParserCore->GetBufferRequirements = NvMkvCoreParserGetBufferRequirements;
    pParserCore->eCoreType          = NvMMParserCoreType_MkvParser;
    pParserCore->UlpKpiMode         = pParams->UlpKpiMode;
    pParserCore->bEnableUlpMode     = pParams->bEnableUlpMode;
    pParserCore->bReduceVideoBuffers = pParams->bReduceVideoBuffers;
    *hParserCore = pParserCore;
    return NvSuccess;
NvMMMkvParserCoreOpen_err:
    if (pParserCore)
    {
        NvOsFree (pParserCore);
        *hParserCore = NULL;
    }
    return status;
}

void
NvMMDestroyMkvCoreParser (
    NvMMParserCoreHandle hParserCore)
{
    if (hParserCore)
    {
        NvOsFree (hParserCore);
    }
}

