/*
 * Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto. Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/********************************************************************************/
#include "nvmm_mkvparser_defines.h"
#include "nvmm_mkvparser.h"
#include "nvmm_mkvdemux.h"

static void helper_bswap2 (NvU8* p)
{
    NvU8 t = *p;
    *p = * (p + 3);
    * (p + 3) = t;
    t = *++p;
    *p = * (p + 1);
    * (p + 1) = t;
    return;
}

NvError NvMkvInitializeDemux (NvMkvDemux * pMatroska, CPhandle hContent, CP_PIPETYPE_EXTENDED *pPipe, NvBool bStreaming)
{
    NvError Status = NvSuccess;
    NVMM_CHK_ARG (pMatroska && pPipe);

    pMatroska->pPipe = pPipe;
    pMatroska->cphandle = hContent;
    pMatroska->bPipeIsStreaming = bStreaming;
    pMatroska->time_scale =  TICKS_PER_SECOND;
    pMatroska->cluster = NvOsAlloc (sizeof (NvMkvParseCluster));
    NVMM_CHK_MEM (pMatroska->cluster);
    NvOsMemset (pMatroska->cluster, 0, sizeof (NvMkvParseCluster));
    pMatroska->cue = NvOsAlloc (sizeof (NvMkvCueInfo));
    NVMM_CHK_MEM (pMatroska->cue);
    NvOsMemset (pMatroska->cue, 0, sizeof (NvMkvCueInfo));
    pMatroska->cluster->state = MKV_CLUSTER_NEW_OPEN;
    pMatroska->cue->ulNumEntries = 0;
    pMatroska->cue->ulMaxEntries = MAX_CUE_ENTRIES_PER_SPLIT_TABLE; //512;
    pMatroska->cue->fOverflow = NV_FALSE;
    pMatroska->cue->fParseDone = NV_FALSE;
    pMatroska->cue->begin = pMatroska->cue->used = pMatroska->cue->length = 0;
    /* Initialize the array */
    pMatroska->cue->SplitCueInfo[0].pCpoints = NvOsAlloc (pMatroska->cue->ulMaxEntries * sizeof (NvMkvCuePoint));
    NVMM_CHK_MEM ( pMatroska->cue->SplitCueInfo[0].pCpoints);
    NvOsMemset ( pMatroska->cue->SplitCueInfo[0].pCpoints, 0, pMatroska->cue->ulMaxEntries*sizeof (NvMkvCuePoint));
    pMatroska->prev_pkt.pts = AV_NOPTS_VALUE;
    pMatroska->prev_pkt.stream_index = -1;
    pMatroska->seek_keyframe = NV_FALSE;
    pMatroska->last_pts = AV_NOPTS_VALUE;
cleanup:
    return Status;
}

NvError NvMkvInitializeParser (CPhandle hContent, CP_PIPETYPE_EXTENDED *pPipe, NvMkvReaderHandle *pMkvReaderHandle, NvBool bStreaming)
{

    NvError Status = NvSuccess;
    NVMM_CHK_ARG(pMkvReaderHandle && hContent && pPipe)
    // Create State
    pMkvReaderHandle->pDemux = (NvMkvDemux *) NvOsAlloc (sizeof (NvMkvDemux));
    NVMM_CHK_MEM (pMkvReaderHandle->pDemux);
    NvOsMemset(pMkvReaderHandle->pDemux , 0, sizeof(NvMkvDemux));
    // initialize demux object state
    NVMM_CHK_ERR (NvMkvInitializeDemux (pMkvReaderHandle->pDemux, hContent, pPipe, bStreaming));
    pMkvReaderHandle->bFirstFrame = NV_TRUE;
    pMkvReaderHandle->bStreaming = bStreaming;
cleanup:
    return Status;
}

NvS32 NvMkvReaderGetTracksInfo (NvMkvReaderHandle * handle, NvS32 index)
{

    NvMkvDemux *matroska = (NvMkvDemux *) handle->pDemux;
    NvMatroskaTrack *tracks = matroska->trackList.track;
    NvS32 matroska_codec;

    if (index > (NvS32) matroska->trackList.ulNumTracks)
        return NvMkvCodecType_NONE;

    matroska_codec =  tracks[index].user_enable ? mkvdCodecType (matroska, index) : NvMkvCodecType_NONE;
    return matroska_codec;
}

static NvError NvMkvReaderGetKeyFrame (NvMkvReaderHandle * handle, NvS32 direction)
{
    NvError Status = NvSuccess;
    NvMkvDemux *matroska;
    NvMkvCueInfo *pCue;
    NvU32 i = 0, j = 0;
    NvBool bFound = NV_FALSE;
    NvS32 ulCueIndex = -1;
    NvS32 tracknum = -1;
    NVMM_CHK_ARG (handle &&  handle->pDemux && handle->pDemux->cue);

    matroska = (NvMkvDemux *) handle->pDemux;
    pCue = matroska->cue;

    // direction == -1 is backwards
    // direction == +1 is forwards
    // return EOF reached, BOF reached or OK or Fail
    NVMM_CHK_ARG (matroska->last_pts != AV_NOPTS_VALUE);

    if (matroska->cue->fParseDone)                   // Check if cue table exists and has been parsed out
    {
        // Ensure that we are on a video track
        // Identify last played timestamp on the video track (?)
        // Search the nearest timestamp in the cue table depending on forward or backward
        // Call set position function with that timestamp as input
        if (direction > 0)
        {
            // Search for the closest time in the array
            for ( j=0; j <= matroska->cue->NumSplitTables; j++)
            {
                NvU32 MaxEntries = (pCue->SplitCueInfo[j].num_elements > pCue->ulMaxEntries) ?  pCue->ulMaxEntries : pCue->SplitCueInfo[j].num_elements;

                for (i = 0; i < MaxEntries ; i++)
                {
                    if ( pCue->SplitCueInfo[j].pCpoints[i].cue_time > matroska->last_pts)
                    {
                        bFound = NV_TRUE;
                        ulCueIndex = i;
                        break;
                    }
                }
                if(bFound)
                {
                    break;
                }
            }
            if (!bFound)
            {
                // End of file reached but no keyframe found beyond this point
                Status = NvError_ParserEndOfStream;
                goto cleanup;
            }
        }
        else
        {
            for ( j=0; j <= matroska->cue->NumSplitTables; j++)
            {
                NvU32 MaxEntries = (pCue->SplitCueInfo[j].num_elements > pCue->ulMaxEntries) ?  pCue->ulMaxEntries : pCue->SplitCueInfo[j].num_elements;
                for (i = MaxEntries- 1; i > 0; i--)
                {
                    if ( pCue->SplitCueInfo[j].pCpoints[i].cue_time < matroska->last_pts)
                    {
                        bFound = NV_TRUE;
                        ulCueIndex = i;
                        break;
                    }
                }
                if(bFound)
                {
                    break;
                }
            }
            if (!bFound)
            {
                // Begin of file reached but no keyframe found
                Status = NvError_ParserEndOfStream;
                goto cleanup;
            }
        }
    }
    else                    // Regular search method not implemented yet
    {
        NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_ERROR,"ParserFailure from set pos\n"));
        Status = NvError_ParserFailure;
        goto cleanup;
    }

    // We have found the cluster. Lets set the position to that point, clear our queues, internal state machines
    if (bFound && (ulCueIndex >= 0))
    {
        NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, (CPint64) pCue->SplitCueInfo[j].pCpoints[ulCueIndex].cue_cluster_position, CP_OriginBegin));
        NVMM_CHK_ERR (mkvdSetPosReinit (matroska));
        matroska->seek_keyframe = NV_TRUE;
        matroska->seek_timecode = pCue->SplitCueInfo[j].pCpoints[ulCueIndex].cue_time;
        matroska->seek_tracknum = (NvS32)pCue->SplitCueInfo[j].pCpoints[ulCueIndex].cue_track;
        matroska->seek_blocknum = pCue->SplitCueInfo[j].pCpoints[ulCueIndex].cue_block_position;
        tracknum = mkvdFindIndexByNum (matroska, matroska->seek_tracknum);
        NVMM_CHK_ERR (mkvdSeekToDesiredBlock (matroska, tracknum));
    }
cleanup:

    return Status;
}

static NvError NvMkvScanCuePoint( NvU64 *pSeekTime, NvMkvDemux *pMatroska, NvU32 *EntryCueIndex, NvU32 *SplitTableIndex)

{
    NvError Status = NvError_ParserFailure;
    NvU32 i = 0, j = 0;
    NvU32 MaxEntries =0;

    NvMkvCueInfo *pCue = NULL;
    NVMM_CHK_ARG (pMatroska && pMatroska->cue);
    pCue = pMatroska->cue;

    // Search for the closest time in the array
    for (j=0; j <= pCue->NumSplitTables; j++)
    {
        if (*pSeekTime < pCue->SplitCueInfo[j].EndTime)
        {
            MaxEntries = (pCue->SplitCueInfo[j].num_elements > pCue->ulMaxEntries) ?  pCue->ulMaxEntries : pCue->SplitCueInfo[j].num_elements;
            for (i = 1; i < MaxEntries ; i++)
            {
                if (*pSeekTime < pCue->SplitCueInfo[j].pCpoints[i].cue_time)
                {
                    *EntryCueIndex = i;
                    *SplitTableIndex = j;
                    Status = NvSuccess;
                    goto cleanup;
                }
            }
        }
        else if ((&pCue->SplitCueInfo[j+1]) && ((pCue->SplitCueInfo[j].EndTime <= *pSeekTime) && (*pSeekTime < pCue->SplitCueInfo[j+1].StartTime)))
        {
            if (*pSeekTime > (pCue->SplitCueInfo[j].EndTime + pCue->SplitCueInfo[j+1].StartTime) >> 1)
            {
                *EntryCueIndex = 0;
                *SplitTableIndex = j+1;
            }
            else
            {
                MaxEntries = (pCue->SplitCueInfo[j].num_elements > pCue->ulMaxEntries) ?  pCue->ulMaxEntries : pCue->SplitCueInfo[j].num_elements;
                *EntryCueIndex = MaxEntries-1;
                *SplitTableIndex = j;
            }
            Status = NvSuccess;
            goto cleanup;
        }
    }
cleanup:
    return Status;
}

NvError NvMkvScanIFrame (NvMkvDemux *matroska)
{
    NvError Status = NvSuccess;
    NvMkvCueInfo *pCue = NULL;
    NvBool bFound = NV_FALSE;
    NvU32 CueIndex = 0;
    NvU32 SplitTableIndex = 0;
    NvU64 ClusterTime =0;
    NvU64 ClusterPosition = 0;
    NvU64 SeekTime = 0;
    NvBool IdentifiedKeyFrame = NV_FALSE;
    NvBool SeekHeadPresent = NV_FALSE;
    NVMM_CHK_ARG (matroska);
    pCue = matroska->cue;

    while(!IdentifiedKeyFrame)
    {
        NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_INFO,
        "NvMkvScanIFrame - ReqSeekTime = %llu\n", SeekTime));
        if (matroska->cue->fParseDone) // Cue present
        {
            if ((SeekTime > pCue->ulLastTime) || (matroska->stream_has_video != NV_TRUE))
            {
                goto cleanup;
            }
            NVMM_CHK_ERR ((NvMkvScanCuePoint(&SeekTime, matroska, &CueIndex, &SplitTableIndex)));
            NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_INFO,
            "NvMkvScanIFrame - T1 -%llu ReqTime - %llu loc: %llu\n",
            pCue->SplitCueInfo[SplitTableIndex].pCpoints[CueIndex].cue_time,
            SeekTime, pCue->SplitCueInfo[SplitTableIndex].pCpoints[CueIndex].cue_cluster_position));
            bFound = NV_TRUE;
            // Found the cluster. Lets set the position to that point, clear our queues, internal state machines
            NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle,
            (CPint64) pCue->SplitCueInfo[SplitTableIndex].pCpoints[CueIndex].cue_cluster_position, CP_OriginBegin));
            NVMM_CHK_ERR (mkvdSetPosReinit (matroska));
            // Update the seek_timecode with new cue point for scan
            matroska->seek_timecode = pCue->SplitCueInfo[SplitTableIndex].pCpoints[CueIndex].cue_time;
            NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_INFO,
            "NvMkvScanIFrame: seek_timecode = %llu\n", matroska->seek_timecode));
            matroska->seek_tracknum = (NvS32)pCue->SplitCueInfo[SplitTableIndex].pCpoints[CueIndex].cue_track;
            matroska->seek_blocknum = pCue->SplitCueInfo[SplitTableIndex].pCpoints[CueIndex].cue_block_position;
        }
        else // Regular search method for NonCue content.
        {
            if ((SeekTime  > (NvU64)matroska->duration) || (matroska->stream_has_video != NV_TRUE))
            {
                NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_INFO,
                "NvMkvScanIFrame Non_Cue Time exceeds Total Duartion: ReqTime = %llu - matroska->duration = %llu\n",
                SeekTime, matroska->duration));
                goto cleanup;
            }
            NVMM_CHK_ERR (mkvdSetPosReinit (matroska));
            if (matroska->clusterList.QSz > 1)
            {
                SeekHeadPresent = NV_TRUE;
            }
            NVMM_CHK_ERR (mkvdScanClusterTimeCode(matroska, SeekTime, &ClusterTime, &ClusterPosition,
            SeekHeadPresent, NvMkvParseClusterData_KEY_FRAME_SCAN));
            matroska->cluster->state = MKV_CLUSTER_NEW_OPEN;
            NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, (CPint64) ClusterPosition, CP_OriginBegin));
            matroska->seek_timecode = ClusterTime;
            matroska->seek_tracknum = (NvS32)matroska->VideoStreamId;
            bFound = NV_TRUE;
        }
        if(bFound)
        {
            matroska->has_cluster_id = 0;
            matroska->seek_keyframe = NV_FALSE;
            matroska->seek_timecode_found = 0;
            NVMM_CHK_ERR (mkvdAddBlockFromCluster(matroska, NvMkvParseClusterData_KEY_FRAME_SCAN));
            SeekTime = (NvU64)matroska->seek_timecode_found;
            if(matroska->IdentifiedKeyThumbNailFrame)
            {
                IdentifiedKeyFrame = NV_TRUE;
                NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,
                "MKVParser:ScanIFrame: IdentifiedKeyFrame: Status = %x  - matroska->largestKeyTimeStamp  %llu\n",
                Status, matroska->largestKeyTimeStamp ));
            }
        }
    }
cleanup:
    return Status;
}
//
NvError NvMkvReaderSetPosition (NvMMMkvParserCoreContext* pContext, NvU64 *pSeekTime)
{
    NvError Status = NvSuccess;
    NvMkvDemux *matroska = NULL;
    NvMkvCueInfo *pCue = NULL;
    NvU64 time0 = 0, time1 = 0;
    NvBool bFound = NV_FALSE;
    NvMkvReaderHandle * handle = NULL;
    NvU32 CueIndex = 0;
    NvU32 SplitTableIndex = 0;
    NvS32 tracknum = -1;
    NvU64 MediaTimeScale = 0;
    NvU64 ClusterTime =0;
    NvU64 ClusterPosition = 0;
    NvU64 SeekTime = 0;
    NvU32 i = 0;
    NvBool IsAudio = NV_FALSE;
    NvMkvTrackTypes trackType;
    NvBool SeekHeadPresent = NV_FALSE;
    NvBool audio_only = NV_FALSE;
    NVMM_CHK_ARG (pContext && pContext->pMkvReaderHandle && pContext->pMkvReaderHandle->pDemux);
    handle = pContext->pMkvReaderHandle;
    matroska = (NvMkvDemux *) handle->pDemux;
    pCue = matroska->cue;

    SeekTime = *pSeekTime;
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,
    "SetPosition - InputSeekTime = %llu\n", *pSeekTime));
    // First convert this time from application provided time units in (GHz /ns) into mkv specific time unit
    // TBD -- index chosen must match the track we are seeking on !!!
    for(i=0; i< matroska->trackList.ulNumTracks; i++)
    {
        trackType = (NvMkvTrackTypes) NvMkvReaderGetTracksInfo (handle , i) ;
        if (NVMKV_ISTRACKVIDEO (trackType))
        {
            MediaTimeScale = NvMkvReaderGetMediaTimeScale (handle, i);
            if (MediaTimeScale)
            {
                SeekTime = (*pSeekTime)/MediaTimeScale;
                pContext->ParseState[i] = MKVPARSE_HEADER;
            }
            break;
        }
        else if (NVMKV_ISTRACKAUDIO (trackType) && (!IsAudio))
        {
            MediaTimeScale = NvMkvReaderGetMediaTimeScale (handle, i);
            if (MediaTimeScale)
            {
                SeekTime = (*pSeekTime)/MediaTimeScale;
                pContext->ParseState[i] = MKVPARSE_HEADER;
                IsAudio = NV_TRUE;
            }
        }
    }
    if (matroska->cue->fParseDone) // Check if cue table exists and has been parsed out
    {
        NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,
        "SetPosition: pCue->ulLastTime = %llu - matroska->duration = %llu\n",
        pCue->ulLastTime, matroska->duration));

        if (SeekTime > pCue->ulLastTime)
        {
            *pSeekTime = (NvU64)(pCue->ulLastTime*(NvU64)MediaTimeScale);
            matroska->done = NV_TRUE;
            // Simply return EOF is reached
            Status = NvError_ParserEndOfStream;
            goto cleanup;
        }
        NVMM_CHK_ERR ((NvMkvScanCuePoint(&SeekTime, matroska, &CueIndex, &SplitTableIndex)));

        if (matroska->stream_has_video)
        {
            NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,
            "SetPosition - Found : CueIndex - %d SplitTableIndx -%d - NumEntries%d\n",
            CueIndex, SplitTableIndex, pCue->ulNumEntries));
            time1 = pCue->SplitCueInfo[SplitTableIndex].pCpoints[CueIndex].cue_time;
            if (CueIndex == 0)
            {
                time0 = time1;
            }
            else
            {
                time0 = pCue->SplitCueInfo[SplitTableIndex].pCpoints[CueIndex-1].cue_time;
            }
            NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,
            "SetPosition - T0 - %llu T1 -%llu ReqTime - %llu loc: %llu\n",
            time0, time1, SeekTime,
            pCue->SplitCueInfo[SplitTableIndex].pCpoints[CueIndex].cue_cluster_position));
            if (SeekTime > (time0 + time1) >> 1)
            {
                SeekTime = time1;
            }
            else
            {
                SeekTime = time0;
                if (CueIndex)
                {
                    CueIndex--;
                }
            }
            bFound = NV_TRUE;
        }
        else if (matroska->stream_has_audio)
        {
            // Audio only file. So do a fine seek on the audio track
            audio_only = NV_TRUE;
            bFound = NV_TRUE;
            if (CueIndex)
            {
                CueIndex--;
            }
        }
        // We have found the cluster. Lets set the position to that point, clear our queues, internal state machines
        if (bFound)
        {
            NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, (CPint64) pCue->SplitCueInfo[SplitTableIndex].pCpoints[CueIndex].cue_cluster_position, CP_OriginBegin));
            NVMM_CHK_ERR (mkvdSetPosReinit (matroska));
            matroska->seek_timecode = audio_only ? SeekTime : pCue->SplitCueInfo[SplitTableIndex].pCpoints[CueIndex].cue_time;
            NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,
            "SetPosition: seek_timecode = %llu\n", matroska->seek_timecode));
            matroska->seek_tracknum = (NvS32)pCue->SplitCueInfo[SplitTableIndex].pCpoints[CueIndex].cue_track;
            matroska->seek_blocknum = pCue->SplitCueInfo[SplitTableIndex].pCpoints[CueIndex].cue_block_position;
        }
    }
    else // Regular search method not implemented yet
    {
        NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,
        "SetPosition Non_Cue seek: ReqTime = %llu\n", SeekTime));
        if(SeekTime  > (NvU64)matroska->duration)
        {
            NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,
            "SetPosition Non_Cue Time exceeds Total Duartion: ReqTime = %llu - matroska->duration = %llu\n",
            *pSeekTime, matroska->duration));
            *pSeekTime = (NvU64)((NvU64)matroska->duration*MediaTimeScale);
            matroska->done = NV_TRUE;
            // Simply return EOF is reached
            Status = NvError_ParserEndOfStream;
            goto cleanup;
        }
        NVMM_CHK_ERR (mkvdSetPosReinit (matroska));
        if (matroska->clusterList.QSz > 1)
        {
            SeekHeadPresent = NV_TRUE;
        }
        NVMM_CHK_ERR (mkvdScanClusterTimeCode(matroska, SeekTime, &ClusterTime,
        &ClusterPosition, SeekHeadPresent, NvMkvParseClusterData_SEEK_NON_CUE));
        matroska->cluster->state = MKV_CLUSTER_NEW_OPEN;
        NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, (CPint64) ClusterPosition, CP_OriginBegin));
        matroska->seek_timecode = audio_only ? SeekTime : ClusterTime;
        matroska->seek_tracknum = (NvS32)matroska->VideoStreamId;
        bFound = NV_TRUE;
    }
    if(bFound)
    {
        matroska->has_cluster_id = 0;
        matroska->seek_keyframe = NV_TRUE;
        matroska->seek_timecode_found = 0;
        tracknum = mkvdFindIndexByNum (matroska, matroska->seek_tracknum);
        NVMM_CHK_ERR (mkvdSeekToDesiredBlock (matroska, tracknum));
        *pSeekTime = (NvU64)matroska->seek_timecode_found*MediaTimeScale;
    }

cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,
    "SetPosition: Status = %x - seek_timecode_found = %llu - UpdatedSeekTime: %llu\n",
    Status, matroska->seek_timecode_found, *pSeekTime));
    return Status;
}


/***   Function to get audio properties  ***/

NvMkvParserStatus NvMkvGetAudioProps (NvMMStreamInfo* currentStream,
                                      NvMkvReaderHandle* handle,
                                      NvU32 trackIndex,
                                      NvS32* maxBitRate)
{
    NvMkvDemux *matroska = (NvMkvDemux *) handle->pDemux;

    NvMatroskaTrack *track = &matroska->trackList.track[trackIndex];

    if (trackIndex > matroska->trackList.ulNumTracks)
        return NvMkvParserStatus_ERROR;

    if (track->type != MATROSKA_TRACK_TYPE_AUDIO)
        return NvMkvParserStatus_ERROR;

    currentStream->NvMMStream_Props.AudioProps.SampleRate = (NvU32) track->audio.samplerate;
    currentStream->NvMMStream_Props.AudioProps.NChannels = (NvU32) track->audio.channels;
    currentStream->NvMMStream_Props.AudioProps.BitRate  = 128000;    // TBD FIXME
    currentStream->TotalTime = (NvU64) matroska->duration;

    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, 
    "audio stream properties - Fs: %llf Ch: %llu Frm Duration %d\n",
    track->audio.samplerate, track->audio.channels, track->default_duration));

    // TBD Max bit rate calculation needed - mkv doesnt provide this
    //  *maxBitRate =  track->audio.coded_framesize;

    currentStream->NvMMStream_Props.AudioProps.BitsPerSample = (NvU32) track->audio.bitdepth;

    return NvMkvParserStatus_NOERROR;
}

NvMkvParserStatus NvMkvGetVideoProps (NvMMStreamInfo* currentStream,
                                      NvMkvReaderHandle* handle,
                                      NvU32 trackIndex)
{
    NvMkvDemux *matroska = (NvMkvDemux *) handle->pDemux;
    NvMatroskaTrack *track = &matroska->trackList.track[trackIndex];


    if ( (trackIndex > matroska->trackList.ulNumTracks) || (track->type != MATROSKA_TRACK_TYPE_VIDEO))
        return NvMkvParserStatus_ERROR;

    currentStream->NvMMStream_Props.VideoProps.Width = (NvU32) track->video.pixel_width;
    currentStream->NvMMStream_Props.VideoProps.Height = (NvU32) track->video.pixel_height;
    currentStream->NvMMStream_Props.VideoProps.Fps = (NvU32) (track->video.frame_rate * FLOAT_MULTIPLE_FACTOR);

    currentStream->NvMMStream_Props.VideoProps.AspectRatioX = (NvU32) (track->video.pixel_height * track->video.display_width);
    currentStream->NvMMStream_Props.VideoProps.AspectRatioY = (NvU32) (track->video.pixel_width * track->video.display_height);
    currentStream->TotalTime = (NvU64) matroska->duration;

    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, 
    "video frame duration %llu\n", track->default_duration));


    // TBD Bit rate calculation needed - mkv doesnt provide this
    // currentStream.NvMMStream_Props.VideoProps.VideoBitRate ????

    return NvMkvParserStatus_NOERROR;
}

NvError NvMkvFileParse (NvMkvReaderHandle * pMkvHandle)
{
    NvError Status = NvSuccess;
    NVMM_CHK_ARG (pMkvHandle && pMkvHandle->pDemux);
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    NVMM_CHK_ERR (NvMkvFileHeaderParse (pMkvHandle->pDemux));
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

cleanup:
    return Status;
}


NvU32 NvMkvReaderGetNumTracks (NvMkvReaderHandle * handle)
{
    NvMkvDemux *demux;
    if (NULL == handle)
    {
        return 0;
    }
    demux = handle->pDemux;
    return demux->trackList.ulNumTracks;
}

/* This function does a basic level check to see if the handle corresponds to an MKV file */
NvError NvMkvFileCheck (NvMkvDemux *pDemux)
{
    NvError Status = NvSuccess;

    NvU32 hlen = 0;
    NvU32 len_mask = 0x80;
    NvU32 size = 1;
    NvU32 n = 1;
    static const char probe_data[] = "matroska";
    NvU8 *data = NULL;
    NvU32 hdr;
    NvU32 cnt = 0;
    NvU32 rdsz;
    NVMM_CHK_ARG (pDemux);

    // Read the first 64 bytes of the file. We may need less, but still so
    // Arbitrarily set read
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "NvMkvFileCheck Start\n"));
    rdsz = 64;
    data = (NvU8 *) NvOsAlloc (rdsz);
    NVMM_CHK_MEM (data);
    NVMM_CHK_CP (pDemux->pPipe->cpipe.Read (pDemux->cphandle, (CPbyte*) data, rdsz));

    /* First four bytes - EBML header */
    NvOsMemcpy (&hdr, &data[cnt], 4);
    cnt += 4;
    helper_bswap2((NvU8 *)&hdr);
    NVMM_CHK_ARG (hdr == EBML_ID_HEADER);

    hlen = data[cnt];
    cnt++;

    /* determine length of header */
    while (size <= 8 && ! (hlen & len_mask))
    {
        size++;
        len_mask >>= 1;
    }
    NVMM_CHK_ARG (size <= 8);

    hlen &= (len_mask - 1);
    while (n++ < size)
    {
        hlen = (hlen << 8) | data[cnt++];
    }

    /* Does our first read contain the whole header? */
    if (rdsz < (4 + size + hlen))
    {
        // read more data as requried
        NvOsFree (data);
        rdsz = 4 + size + hlen;
        data = (NvU8 *) NvOsAlloc (rdsz);
        NVMM_CHK_MEM (data);
        // Goto the beginning of the file
        NVMM_CHK_CP (pDemux->pPipe->SetPosition64 (pDemux->cphandle, (CPint64) 0, CP_OriginBegin));
        NVMM_CHK_CP (pDemux->pPipe->cpipe.Read (pDemux->cphandle, (CPbyte*) data, rdsz));
    }

    /* Parse through the initial header to check if it contains the document type 'matroska'. */
    for (n = 4 + size; n <= (4 + size + hlen - (sizeof (probe_data) - 1)); n++)
    {
        if (!NvOsMemcmp (&data[n], probe_data, sizeof (probe_data) - 1))
        {
            // match found
            Status = NvSuccess;
            break;
        }
    }

    NVMM_CHK_CP (pDemux->pPipe->SetPosition64 (pDemux->cphandle, (CPint64) 0, CP_OriginBegin));
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "NvMkvFileCheck End\n"));
cleanup:
    if (data)
    {
        NvOsFree (data);
    }
    return Status;
}

NvError NvMkvFileHeaderParse (NvMkvDemux *pDemux)
{
    NvError Status = NvSuccess;

    NVMM_CHK_ERR (NvMkvFileCheck (pDemux));

    Status = mkvdReadHeader (pDemux);
cleanup:

    return Status;
}
NvError NvMkvReaderGetPacket (NvMkvReaderHandle * handle, NvU32 index , void * buffer , NvU32 *frameSize, NvU64 *pts)
{
    NvError Status = NvSuccess;

    NvMkvDemux *matroska;
    NvMkvAVPacket avpkt;

    NVMM_CHK_ARG (handle && handle->pDemux && buffer);

    matroska= handle->pDemux;
    if (handle->rate > 1000)
    {
        NVMM_CHK_ERR (NvMkvReaderGetKeyFrame (handle, 1));
    }
    else if (handle->rate < 0)
    {
        NVMM_CHK_ERR (NvMkvReaderGetKeyFrame (handle, -1));
    }
    else if (handle->rate == 0)
    {
        goto cleanup;
    }

    if ( (Status = mkvdGetPacket (matroska, index, &avpkt)) != NvSuccess)
    {
        *frameSize = 0;
        *pts = 0;
        if (matroska->done)
            Status = NvError_ParserEndOfStream;
        else
            Status = NvError_ParserFailure;
        goto cleanup;
    }
    NVMM_CHK_ERR (mkvdFetchData (matroska, &avpkt, buffer, frameSize));

    // Convert pts into nano seconds using matroska scale values
    *pts = avpkt.pts * NvMkvReaderGetMediaTimeScale (handle, index);
    *frameSize = avpkt.size;

    matroska->last_pts = avpkt.pts;
cleanup:
    return Status;
}

NvU64 NvMkvReaderGetMediaTimeScale (NvMkvReaderHandle * handle, NvS32 index)
{

    NvMkvDemux *matroska ;
    NvMatroskaTrack *track;

    matroska = (NvMkvDemux *) handle->pDemux;
    track = matroska->trackList.track;

    return (NvU64) (track[index].timescale * matroska->time_scale + 0.5);
}

NvMkvParserStatus NvMkvReaderGetNALSize (NvMkvReaderHandle * handle, NvS32 index , NvS32 * nalSize)
{
    NvMkvDemux *matroska;
    NvS32 Status = NvMkvParserStatus_NOERROR;

    matroska = (NvMkvDemux *) handle->pDemux;

    switch (NvMkvReaderGetTracksInfo (handle, index))
    {
        case NvMkvCodecType_AVC:
            *nalSize =  matroska->nalSize;
            break;
        default:
            Status = NvMkvParserStatus_ERROR;
            break;
    }
    return (Status);
}


void *NvMkvReaderGetDecConfig (NvMkvReaderHandle * handle, NvS32 index, NvS32 * size)
{
    NvMkvDemux *matroska = (NvMkvDemux *) handle->pDemux;
    NvMatroskaTrack *track = matroska->trackList.track;

    if (index > (NvS32) matroska->trackList.ulNumTracks)
        return NULL;

    *size = track[index].codec_prv_size;
    return (void *) (track[index].codec_prv_data);
}

void NvMkvReleaseReaderHandle (NvMkvReaderHandle * handle)
{

    NvMkvDemux * matroska;
    NvU32 i = 0;
    if (handle)
    {
        matroska = (NvMkvDemux *) handle->pDemux;
        if (matroska)
        {
            mkvdSetPosReinit (matroska);

            // Free Cue data
            for (i=0; i <= matroska->cue->NumSplitTables; i++)
            {
                if (matroska->cue->SplitCueInfo[i].pCpoints)
                {
                    NvOsFree (matroska->cue->SplitCueInfo[i].pCpoints);
                    matroska->cue->SplitCueInfo[i].pCpoints = NULL;
                }
            }
            if (matroska->cue)
            {
                NvOsFree (matroska->cue);
                matroska->cue = NULL;
            }
            // Free track data
            for (i = 0; i < matroska->trackList.ulNumTracks; i++)
            {
                if (matroska->trackList.track[i].codec_prv_data)
                {
                    NvOsFree (matroska->trackList.track[i].codec_prv_data);
                    matroska->trackList.track[i].codec_prv_data = NULL;
                }
            }

            if (matroska->trackList.track) 
            {
                NvOsFree (matroska->trackList.track);
                matroska->trackList.track = NULL;
            }
            if (matroska->cluster)
            {
                NvOsFree (matroska->cluster);
                matroska->cluster = NULL;
            }
            if (handle->pDemux)
            {
                NvOsFree (handle->pDemux);
                handle->pDemux = NULL;
            }
        }
        NvOsFree (handle);
    }
}

NvMkvParserStatus NvMkvDisableTrackNum (NvMkvReaderHandle * handle , NvS32 index)
{
    NvMkvDemux *matroska = (NvMkvDemux *) handle->pDemux;
    return mkvdEnableDisableTrackNum (matroska, index, NV_FALSE);
}

NvMkvParserStatus NvMkvEnableTrackNum (NvMkvReaderHandle * handle, NvS32 index)
{
    NvMkvDemux *matroska = (NvMkvDemux *) handle->pDemux;
    return mkvdEnableDisableTrackNum (matroska, index, NV_TRUE);
}


