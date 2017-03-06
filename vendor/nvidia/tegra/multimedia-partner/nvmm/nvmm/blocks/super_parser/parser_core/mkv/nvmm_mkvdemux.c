/*
 * Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto. Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvmm_mkvdemux.h"
#include "nvmm_mkvparser_defines.h"
#include "nvmm_mkvparser.h"
#include <math.h>

#define MAX_KEYFRAME_SCAN_LIMIT 10
/*
 * Internally called function to map the sampling rate into index used by NVMM
 * This LUT is copied from the one used in MP4 parser. Function returns -1 if
 * the sampling rate is not found in the table.
 */
static NvError prvAudioFsToId (NvU32 SampleRate, NvU32 *SampleIndex)
{
    NvError Status = NvError_ParserFailure;
    static const NvU32 freqIndexTable[13] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};
    NvU32 i = 0;
    for (i = 0; i<13; i++)
    {
        if (freqIndexTable[i] == SampleRate)
        {
           Status = NvSuccess;
              *SampleIndex = i;
            break;
        }
   }
   return Status;
}

/*
 * Internally called function to convert a read 32 bit value into a double precision
 * floating point number. This is in accordance with the matroska spec and
 * implementation is copied from ffmpeg sources
 */
static NvError prvInt2Dbl (NvS64 v, NvF64 *ptr)
{
    NvError Status = NvSuccess;

    if ( (NvU64) (v + v) > 0xFFEULL << 52)
    {
        Status = NvError_BadParameter;
        goto cleanup;
    }
    *ptr = ldexp ( (NvF64) ( ( (v & ( (1LL << 52) - 1)) + (1LL << 52)) * (v >> 63 | 1)), (NvS32) ( (v >> 52 & 0x7FF) - 1075));
cleanup:
    return Status;
}

/*
 * Internally called function to convert a read 32 bit value into a floating point
 * number. This is in accordance with the matroska spec and implementation is
 * copied from ffmpeg sources
 */
static NvError prvInt2Flt (NvS32 v, NvF32 *ptr)
{
    NvError Status = NvSuccess;
    if ( (NvU32) (v + v) > 0xFF000000)
    {
        Status = NvError_BadParameter;
        goto cleanup;
    }
    *ptr = (NvF32) (ldexp ( (NvF32) ( ( (v & 0x7FFFFF) + (1 << 23)) * (v >> 31 | 1)), (NvS32) ( (v >> 23 & 0xFF) - 150)));
cleanup:
    return Status;
}

/*
 * Read an unsigned integer element of size bytes from the byte stream
 * in accordance with the matroska specification. If the stream has run out of data,
 * the matroska->done element is set to true
 */
static NvError ebmlReadUint (NvMkvDemux *matroska, NvS32 size, NvU64 *num)
{
    NvError Status = NvSuccess;
    NvS32 n = 0;
    NvU8 data;
    NVMM_CHK_ARG (matroska);
    if (size < 1 || size > 8)
    {
        Status = NvError_BadParameter;
        goto cleanup;
    }

    /* big-endian ordering; build up number */
    *num = 0;
    while (n++ < size)
    {
        Status =matroska->pPipe->cpipe.Read (matroska->cphandle, (CPbyte*) &data, 1);
        if (Status != NvSuccess)
        {
            /* EOS or actual I/O error */
            matroska->done = NV_TRUE;
            goto cleanup;
        }
        *num = (*num << 8) | data;
    }
cleanup:
    return Status;
}


/*
 * Read the next element as floating point number of size bytes from the byte stream
 * in accordance with the matroska specification. If the stream has run out of data,
 * the matroska->done element is set to true
 */
static NvError ebmlReadFloat (NvMkvDemux *matroska, NvU64 size, double *num)
{
    NvError Status = NvSuccess;
    NvS64 value = 0;
    NvU8 data;
    NvU32 n = 0;
    NVMM_CHK_ARG(matroska);

    if ( (size != 4) && (size != 8))
    {
        Status = NvError_BadParameter;
        goto cleanup;
    }

    while (n++ < size)
    {
        Status = matroska->pPipe->cpipe.Read (matroska->cphandle, (CPbyte*) &data, 1);
        if (Status != NvSuccess)
        {
            matroska->done = NV_TRUE;
            /* EOS or actual I/O error */
            goto cleanup;
        }
        value = (value << 8) | data;
    }

    switch (size)
    {
            NvF32 temp;
        case 4:
            NVMM_CHK_ERR (prvInt2Flt ( (NvS32) value, &temp));
            *num = (NvF64) temp;
            break;
        case 8:
            NVMM_CHK_ERR (prvInt2Dbl (value, num));
            break;
        default:
            Status = NvError_ParserFailure;
    }
cleanup:
    return Status;
}


/*
 * Read an EBML coded unsigned integer number from the byte stream in accordance with MKV spec.
 * Return the number of bytes consumed. Function returns -1 if an error occurs.
 * Also, file read error is indicated using matroska->done
 */
static NvError ebmlReadNum (NvMkvDemux *matroska, NvS32 max_size, NvU64 *number, NvS32 *length)
{
    NvError Status = NvSuccess;
    NvS32 len_mask = 0x80;
    NvS32 n = 1;
    NvS32 read =1;
    NvU64 total = 0;
    *length = 0;
    *number = 0;
    NVMM_CHK_ARG (matroska);

    /* The first byte tells us the length in bytes */
    NVMM_CHK_CP (matroska->pPipe->cpipe.Read (matroska->cphandle, (CPbyte*) &total, 1));

    /* get the length of the EBML number */
    while (read <= max_size && ! (total & len_mask))
    {
        read++;
        len_mask >>= 1;
    }

    //NVMM_CHK_ARG (read <= max_size)
    if (read > max_size)
    {
      Status = NvError_ParserFailure;
      goto cleanup;
    }

    /* read out length */
    total &= (len_mask - 1);
    while (n++ < read)
    {
        NvU8 byte;
        NVMM_CHK_CP (matroska->pPipe->cpipe.Read (matroska->cphandle, (CPbyte*) &byte, 1));   /* EOS or actual I/O error */
        total = (total << 8) | byte;
    }
    *number = total;
    *length = read;

cleanup:
     if (!matroska)
         matroska->done = NV_TRUE;
     return Status;
}

/*
 * Read an EBML coded signed integer number from the byte stream in accordance with MKV spec.
 * Return the number of bytes consumed. Function returns -1 if an error occurs.
 * Also, file read error is indicated using matroska->done
 */
static NvError ebmlReadSint (NvMkvDemux *matroska, NvS32 size, NvS64 *num, NvS32 *read)
{
    NvError Status = NvSuccess;
    NvU64 unum;
    NvS32 res = 0;
    *read = 0;

    /* read as unsigned number first */
    NVMM_CHK_ERR (ebmlReadNum (matroska, size, &unum, &res));

    /* make signed (weird way) */
    *num = unum - ( (1LL << (7 * res - 1)) - 1);
    *read = res;
cleanup:
    return Status;
}

/*
 * Read an EBML ID code from the byte stream in accordance with MKV spec.
 * Return the number of bytes consumed. Function returns -1 if an error occurs.
 */
static NvError ebmlReadId (NvMkvDemux *matroska, NvU64 *pid, NvS32 *read)
{
    NvError Status = NvSuccess;
    NvS32 res = 0;
    *read = 0;
    *pid = 0;
    NVMM_CHK_ERR (ebmlReadNum (matroska, 4, pid, &res));

    *pid = *pid | (1LL << (7 * res));
    *read = res;

    if ( (res < 0) && (!matroska->done))
    {
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
        "ID read seems incorrect in file - res %d\n", res));
        Status = NvError_ParserFailure;
    }
cleanup:
    return Status;
}

/*
 * Locally called function to parse the SPS and PPS out of the codec private data
 * available in the MKV header. The appropriate structure is populated in accordance
 * with NVMM requriement
 */
static NvMkvParserStatus mkvdParseAvcConfig (NvMkvDemux *matroska, NvU8* pAvcc, NvU32 avcSz)
{
    NvAVCConfigData *avcConfig = &matroska->avcConfigData;
    NvU32 consumed = 0;
    NvU8 *buffer1;
    NvS32 i = 0;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    if (pAvcc[0] != 1)
    {
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, 
            "Error in avcConfig data\n"));
        return NvMkvParserStatus_BADFILE_ERROR;
    }

    avcConfig->lengthSizeMinusOne = (pAvcc[4]) & 0x3;
    matroska->nalSize = avcConfig->lengthSizeMinusOne;
    avcConfig->noOfSequenceParamSets = (pAvcc[5]) & 0x1f;
    consumed = 6;

    buffer1 = &pAvcc[6];
    avcConfig->seqParamSetLength[0] = 0;
    for (i = 0; i < (avcConfig->noOfSequenceParamSets); i++)
    {
        avcConfig->seqParamSetLength[i] = (buffer1[0] << 8) | (buffer1[1]);
        buffer1 += 2;

        consumed += (avcConfig->seqParamSetLength[i] + 2);
        if (consumed > avcSz)
        {
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, 
                "Check the avcC for size --- SPS error\n"));
            return NvMkvParserStatus_ERROR;
        }
        if (avcConfig->seqParamSetLength[i] > 128)  // This is from the declaration of spsNalUnit[][128]!
        {
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, 
                "Check spsNalUnit definition\n"));
            return NvMkvParserStatus_ERROR;
        }
        NvOsMemcpy (avcConfig->spsNalUnit[i], buffer1, avcConfig->seqParamSetLength[i]);
        buffer1 += (avcConfig->seqParamSetLength[i]);
    }
    avcConfig->noOfPictureParamSets = (buffer1[0]);
    buffer1 += 1;
    consumed    += 1;

    avcConfig->picParamSetLength[0] = 0;
    for (i = 0; i < (avcConfig->noOfPictureParamSets); i++)
    {
        avcConfig->picParamSetLength[i] = (buffer1[0] << 8) | (buffer1[1]);
        buffer1 += 2;
        consumed    += (avcConfig->picParamSetLength[i] + 2);
        if (consumed > avcSz)
        {
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
            "Check the avcC for size --- PPS error\n"));
            return NvMkvParserStatus_ERROR;
        }
        NvOsMemcpy (avcConfig->ppsNalUnit[i], buffer1, avcConfig->picParamSetLength[i]);
        //avcConfig->EntropyType = prvParsePPS(avcConfig->ppsNalUnit[i], avcConfig->picParamSetLength[i]);
        buffer1 += (avcConfig->picParamSetLength[i]);
    }
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

    return NvMkvParserStatus_NOERROR;
}

/*
 * Function that looks up the user provided track index to find the corresponding codec in that track
 */
NvMkvCodecType mkvdCodecType (NvMkvDemux *matroska, NvS32 index)
{
    NvMatroskaTrack *tracks;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    if ( (index < 0) || (index > (NvS32) matroska->trackList.ulNumTracks))
    {
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
            "Index invalid or Number of tracks incorrect index = %d numtracks = %d\n",
            index, matroska->trackList.ulNumTracks));
        return NvMkvCodecType_NONE;
    }
    tracks = matroska->trackList.track;
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

    return tracks[index].codec_type;
}

/*
 * Function taht looks up the user provided track index to find the stream type - audio, video, subtitle
 */
NvMkvTrackTypes mkvdStreamType (NvMkvDemux *matroska, NvS32 index)
{
    NvMatroskaTrack *tracks;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    if ( (index < 0) || (index > (NvS32) matroska->trackList.ulNumTracks))
    {
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
        "Something wrong ... FIXME %d %d\n", index, matroska->trackList.ulNumTracks));
        return NvMkvTrackTypes_None;
    }
    tracks = matroska->trackList.track;

    switch (tracks[index].type)
    {
        case MATROSKA_TRACK_TYPE_VIDEO:
            return NvMkvTrackTypes_VIDEO;
            break;
        case MATROSKA_TRACK_TYPE_AUDIO:
            return NvMkvTrackTypes_AUDIO;
            break;
        case MATROSKA_TRACK_TYPE_SUBTITLE:
            return NvMkvTrackTypes_SUBTITLE;
            break;

        case MATROSKA_TRACK_TYPE_COMPLEX:
        case MATROSKA_TRACK_TYPE_LOGO:
        case MATROSKA_TRACK_TYPE_CONTROL:
            return NvMkvTrackTypes_OTHER;

        default:
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, "Unknown stream type - corrupted data ?\n"));
            return NvMkvTrackTypes_None;
    }
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

}

/* This function determines the internal MKV reference index for each track.
 * The user level index is usually a numbering strarting from zero in the order in
 * which these tracks were encountered in the byte stream. The MKV reference index
 * may be arbitrary and starts from 1.
 */
NvS32 mkvdFindIndexByNum (NvMkvDemux *matroska, NvS32 tracknum)
{
    NvMatroskaTrack *tracks = matroska->trackList.track;
    NvU32 i;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    for (i = 0; i < matroska->trackList.ulNumTracks; i++)
        if (tracks[i].num == (NvU64)tracknum)
            return (NvS32)i;
    NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, 
    "Invalid tracknumber in find_index_by_num %d\n", tracknum));
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

    return -1;
}

/*
 * This function does "mkvdFindIndexByNum" and further returns the pointer to the track to the caller.
 * The input index refers to the MKV reference index in the stream.
 * This is first mapped into the user index and then that index is used to retrieve
 * the stored track information from our structures
 */
NvMatroskaTrack *mkvdFindTrackByNum (NvMkvDemux *matroska, NvS32 num)
{
    NvMatroskaTrack *tracks = matroska->trackList.track;
    NvU32 i;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    for (i = 0; i < matroska->trackList.ulNumTracks; i++)
        if (tracks[i].num == (NvU32) num)
            return &tracks[i];

    NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, 
    "Invalid tracknumber in find_track_by_num %d\n", num));
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

    return NULL;
}

static NvError mkvdParseSyntax (NvMkvDemux *matroska, NvMkvEbml *pEbml)
{
    NvError Status = NvSuccess;
    NvS32 res = 0;
    NvU32 used = 0;
    NvU64 id;
    NvU64 length, size;
    NvU64 *ptr;
    NvBool bEbmlIdFound = NV_FALSE;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    NVMM_CHK_ERR (ebmlReadId (matroska, &id, &res));

    if (res < 0 || id != EBML_ID_HEADER)
    {
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, "Invalid EBML Header\n"));
        Status = NvError_ParserFailure;
        goto cleanup;
    }
    NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &size, &res));
    while (used < size)
    {
        NVMM_CHK_ERR (ebmlReadId (matroska, &id, &res));
        used += res;

        NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));

        used += res;
        bEbmlIdFound = NV_FALSE;
        switch (id)
        {
            case EBML_ID_DOCTYPE:
                pEbml->doctype = NvOsAlloc ( (NvU32) (length + 1));
                NVMM_CHK_MEM (pEbml->doctype);

                NVMM_CHK_CP (matroska->pPipe->cpipe.Read (matroska->cphandle, (CPbyte*) pEbml->doctype, (CPuint) length));   /* EOS or actual I/O error */
                used += (NvU32) length;
                pEbml->doctype[length] = '\0';  // terminate string
                break;
            case EBML_ID_EBMLVERSION:
                ptr = &pEbml->version;
                bEbmlIdFound = NV_TRUE;
                break;
            case EBML_ID_EBMLMAXIDLENGTH:
                ptr = &pEbml->id_length;
                bEbmlIdFound = NV_TRUE;
                break;
            case EBML_ID_EBMLMAXSIZELENGTH:
                ptr = &pEbml->max_size;
                bEbmlIdFound = NV_TRUE;
                break;
            case EBML_ID_DOCTYPEVERSION:
                ptr = &pEbml->doctype_version;
                bEbmlIdFound = NV_TRUE;
                break;
            case EBML_ID_EBMLREADVERSION:
                ptr = &pEbml->readversion;
                bEbmlIdFound = NV_TRUE;
                break;
            case EBML_ID_DOCTYPEREADVERSION:
                ptr = &pEbml->doctype_readversion;
                bEbmlIdFound = NV_TRUE;
                break;
            default:
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                    "Unknown ID found - Bad EBML syntax in header\n"));
                Status = NvError_ParserFailure;
                break;
        }
        if(bEbmlIdFound)
        {
            NVMM_CHK_ERR (ebmlReadUint (matroska, (NvU32) length, ptr)) ;
            used += (NvU32) length;
        }
    }
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

cleanup:
    return Status;
}

static NvError NvMkvGetCuePoint(NvMkvCueInfo *pCueInfo)
{
    NvError Status = NvSuccess;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    NVMM_CHK_ARG (pCueInfo);
    pCueInfo->NumSplitTables++;
    pCueInfo->SplitCueInfo[pCueInfo->NumSplitTables].pCpoints = NvOsAlloc (pCueInfo->ulMaxEntries * sizeof (NvMkvCuePoint));
    NVMM_CHK_MEM (pCueInfo->SplitCueInfo[pCueInfo->NumSplitTables].pCpoints);
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

    return Status;
}

//
static NvError mkvdAddToCueTable (NvMkvDemux *matroska, NvU64 track, NvU64 time, NvU64 clus_pos, NvU64 blk_pos)
{
    NvError Status = NvSuccess;
    NvMkvCueInfo *pCueInfo = NULL;
    NvMkvCuePoint *pCuePoint = NULL;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    NVMM_CHK_ARG (matroska);
    pCueInfo = matroska->cue;
restartAddCuePoint:
    if(pCueInfo->NumSplitTables > MAX_SPLIT_CUE_TABLES)
    {
        pCueInfo->fOverflow = NV_TRUE;
        Status = NvError_ParserFailure;
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, 
        "OverFlow: CueInfo NumSplitTables EXCEEDS MAX_SPLIT_CUE_TABLES\n"));
        goto cleanup;
    }
    if (pCueInfo->ulNumEntries < ((pCueInfo->NumSplitTables+1)*pCueInfo->ulMaxEntries))
    {
        pCuePoint = &pCueInfo->SplitCueInfo[pCueInfo->NumSplitTables].pCpoints[pCueInfo->ulNumEntries - (pCueInfo->NumSplitTables*pCueInfo->ulMaxEntries)];
        pCuePoint->cue_track = track;
        pCuePoint->cue_time = time;
        pCuePoint->cue_block_position = (NvS32) blk_pos;
        pCuePoint->cue_cluster_position = clus_pos + matroska->segment_start;  // cue position is wrt the segment data
        if (pCueInfo->SplitCueInfo[pCueInfo->NumSplitTables].num_elements == 0)
        {
            pCueInfo->SplitCueInfo[pCueInfo->NumSplitTables].StartTime = time;
        }
        pCueInfo->SplitCueInfo[pCueInfo->NumSplitTables].EndTime = time;
        pCueInfo->ulLastTime = time;
        pCueInfo->ulNumEntries++;
        pCueInfo->SplitCueInfo[pCueInfo->NumSplitTables].num_elements++;
    }
    else
    {
        NVMM_CHK_ERR (NvMkvGetCuePoint(pCueInfo));
        goto restartAddCuePoint;
    }
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

    return Status;
}
//
static NvError mkvdParseCues (NvMkvDemux *matroska, NvU32 *pUsed)
{
    NvS32 res = 0;
    NvU64 id, cp_used;
    NvU64 cp_length;
    NvU64 length;
    NvU64 ctp_used;
    NvU64 ctp_length;
    NvU64 data;
    NvU64 *ptr;
    NvBool bEbmlRead = NV_FALSE;
    NvMkvCueInfo *cue = NULL;
    NvError Status = NvSuccess;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    NVMM_CHK_ARG (matroska);
    cue = matroska->cue;

    NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Entering the parsing of cues\n"));
    ptr = &data;

    NVMM_CHK_CP (matroska->pPipe->GetPosition64 (matroska->cphandle, &cue->begin));
    cue->begin -= 4; //compensate for the 4 byte ID already read

    NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &cue->length, &res));

    *pUsed = 0;
    *pUsed += res;
    *pUsed += (NvU32) cue->length;

    // Check if this parsing has already been done
    // Not sure if multiple cues come in file
    if (matroska->cue->fParseDone)
    {
        NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, cue->length, CP_OriginCur));
        Status = NvSuccess;
     goto cleanup;
    }
    cue->used = 0;
    while (cue->used < cue->length)
    {
        NvU64 track, clus_pos, blk_pos = -1, time = 0;

        if ( (Status = ebmlReadId (matroska, &id, &res)) != NvSuccess)
        {
            matroska->done = 1;
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
            "MkvdParseCues Err: ebmlReadId Status = %x\n", Status));
            goto cleanup; ;
        }
        cue->used += res;
        if (id != MATROSKA_ID_POINTENTRY)
        {
            Status = NvError_ParserFailure;
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
            "MkvdParseCues Err: id != MATROSKA_ID_POINTENTRY\n"));
            goto cleanup;
        }
        NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &cp_length, &res));
        cue->used += res;
        cp_used = 0;
        while (cp_used < cp_length)
        {
            if ( (Status = ebmlReadId (matroska, &id, &res)) != NvSuccess)
            {
                matroska->done = 1;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, 
                "Return from read num %llu %d\n", cue->length, res));
                goto cleanup;
            }
            cp_used += res;
            switch (id)
            {
                case MATROSKA_ID_CUETIME:
                    NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
                    cp_used += res;
                    NVMM_CHK_ERR (ebmlReadUint (matroska, (NvU32) length, &time));
                    cp_used += length;
                    break;
                case MATROSKA_ID_CUETRACKPOSITION:
                    NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &ctp_length, &res));
                    cp_used += res;
                    ctp_used = 0;
                    blk_pos = -1;  // block position may not be available - then -1 indicates this
                    while (ctp_used < ctp_length)
                    {
                        if ( (Status = ebmlReadId (matroska, &id, &res)) != NvSuccess)
                        {
                            matroska->done = 1;
                            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                            "Return from read num %llu %d\n", length, res));
                            goto cleanup;
                        }
                        ctp_used += res;
                        bEbmlRead = NV_FALSE;
                        switch (id)
                        {
                            case MATROSKA_ID_CUETRACK:
                                //set the pointer
                                ptr = &track;
                                bEbmlRead = NV_TRUE;
                                break;
                            case MATROSKA_ID_CUECLUSTERPOSITION:
                                //set the pointer
                                ptr = &clus_pos;
                                bEbmlRead = NV_TRUE;
                               break;
                            case MATROSKA_ID_CUEBLOCKNUMBER:
                                //set the pointer
                                ptr = &blk_pos;
                                bEbmlRead = NV_TRUE;
                                break;
                        }
                        if(bEbmlRead)
                        {
                                NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
                                ctp_used += res;
                                NVMM_CHK_ERR (ebmlReadUint (matroska, (NvU32) length, ptr));
                                ctp_used += length;
                        }
                    }
                    NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, 
                    "Building CUE Table Entry -> [Cue - time: %llu cluspos: %d blkpos: %d]\n", 
                     time, clus_pos, blk_pos));
                    cp_used += ctp_length;
                    break;
            }
        }

        // Add this to our array of ctp
        if (mkvdAddToCueTable (matroska, track, time, clus_pos, blk_pos) != NvSuccess)
        {
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
            "Cue table entries exceeded allocated size for table\n"));
            // Continue to process the table till the end
        }
        cue->used += cp_length;
    }
    cue->fParseDone = NV_TRUE;
    matroska->cue_parse_done = NV_TRUE;
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

    return Status;
}

static NvError mkvdParseInfo (NvMkvDemux *matroska, NvU32 *pUsed)
{
    NvS32 res = 0;
    NvU64 length = 0;
    NvU64 size;
    NvU64 id;
    NvU32 info_used = 0;
    NvU64 info_pos;
    NvError Status = NvSuccess;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    NVMM_CHK_ARG (matroska);
    NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &size, &res));

    *pUsed = 0;
    *pUsed += res;
    *pUsed += (NvU32) size;

    NVMM_CHK_CP (matroska->pPipe->GetPosition64 (matroska->cphandle, &info_pos));

    // Some defaults
    matroska->time_scale = 1000000;

    while (info_used < size)
    {
        NVMM_CHK_ERR (ebmlReadId (matroska, &id, &res));
        info_used += res;
        NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
        info_used += res;

        switch (id)
        {
            case MATROSKA_ID_TIMECODESCALE:
                NVMM_CHK_ERR (ebmlReadUint (matroska, (NvU32) length, &matroska->time_scale));
                info_used += (NvU32) length;
                break;
            case MATROSKA_ID_DURATION:
                NVMM_CHK_ERR (ebmlReadFloat (matroska, length, &matroska->duration));
                info_used += (NvU32) length;
                break;
            case MATROSKA_ID_TITLE:
            case MATROSKA_ID_WRITINGAPP:
            case MATROSKA_ID_MUXINGAPP:
            case MATROSKA_ID_DATEUTC:
            case MATROSKA_ID_SEGMENTUID:
                NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, length, CP_OriginCur));
                info_used += (NvU32) length;
                break;
            default:
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, "Oops unknown case in parse info case\n"));
                Status = NvError_ParserFailure;
             goto cleanup;
        }
    }
cleanup:
    matroska->info_parse_done = NV_TRUE;
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

    return Status;
}

static NvError mkvdParseTrackVideo (NvMkvDemux *matroska, NvMatroskaTrack *track, NvU32 *pUsed)
{
    NvS32 res = 0;
    NvU64 length = 0;
    NvU64 sz = 0;
    NvU32 track_used = 0;
    NvU64 id;
    NvU64 temp = 0;
    NvU64 *ptr;
    NvU64 dummy;
    NvBool bEbmlReadUint = NV_FALSE;
    NvError Status = NvSuccess;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    NVMM_CHK_ARG (matroska);
    NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &sz, &res));

    *pUsed = res;
    *pUsed += (NvU32) sz;

    track->video.display_height = 0;
    track->video.display_width = 0;
    track->video.pixel_width = 0;
    track->video.pixel_height = 0;
    while (track_used < sz)
    {
        NVMM_CHK_ERR (ebmlReadId (matroska, &id, &res));
        track_used += res;
        bEbmlReadUint = NV_FALSE;
        switch (id)
        {
            case MATROSKA_ID_VIDEODISPLAYWIDTH:
                ptr = &track->video.display_width;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Vid disp wid\n"));
                bEbmlReadUint = NV_TRUE;
                break;
            case MATROSKA_ID_VIDEODISPLAYHEIGHT:
                ptr = &track->video.display_height;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Vid disp ht\n"));
                bEbmlReadUint = NV_TRUE;
                break;
            case MATROSKA_ID_VIDEOPIXELWIDTH:
                ptr = &track->video.pixel_width;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Vid pix wid\n"));
                bEbmlReadUint = NV_TRUE;
                break;
            case MATROSKA_ID_VIDEOPIXELHEIGHT:
                ptr = &track->video.pixel_height;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Vid pix ht\n"));
                bEbmlReadUint = NV_TRUE;
                break;
            case MATROSKA_ID_VIDEOFLAGINTERLACED:
                ptr = &temp;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Vid interlaced field\n"));
                bEbmlReadUint = NV_TRUE;
                break;
            case MATROSKA_ID_VIDEOFRAMERATE:
            case MATROSKA_ID_VIDEOPIXELCROPB:
            case MATROSKA_ID_VIDEOPIXELCROPT:
            case MATROSKA_ID_VIDEOPIXELCROPL:
            case MATROSKA_ID_VIDEOPIXELCROPR:
            case MATROSKA_ID_VIDEODISPLAYUNIT:
            case MATROSKA_ID_VIDEOSTEREOMODE:
            case  MATROSKA_ID_VIDEOSTEREOMODE_3D:
            case MATROSKA_ID_VIDEOASPECTRATIO:
            case MATROSKA_ID_VIDEOCOLORSPACE:
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, 
                "Known but unparsed ID found %llx\n", id));
                ptr = &dummy;
                bEbmlReadUint = NV_TRUE;
                break;
            default:
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, 
                "Video: Impossible to get here check ID = %llx\n", id));
                Status = NvError_ParserFailure;
                goto cleanup;
        }
        if(bEbmlReadUint)
        {
             // Read the size and then the uint
            NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
            track_used += res;
            NVMM_CHK_ERR (ebmlReadUint (matroska, (NvU32) length, ptr));
            track_used += (NvU32) length;
            if (MATROSKA_ID_VIDEOFLAGINTERLACED == id)
                track->video.interlaced = temp == 0 ? NV_FALSE : NV_TRUE;
        }
    }
    if (track->video.display_height == 0)
        track->video.display_height = track->video.pixel_height;
    if (track->video.display_width == 0)
        track->video.display_width = track->video.pixel_width;
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

    return Status;
}

static NvError mkvdParseTrackAudio (NvMkvDemux *matroska, NvMatroskaTrack *track, NvU32 *pUsed)
{
    NvS32 res = 0;
    NvU64 length = 0;
    NvU64 sz = 0;
    NvU32 track_used = 0;
    NvU64 id;
    NvU64 *ptr;
    NvError Status = NvSuccess;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    NVMM_CHK_ARG (matroska);
    NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &sz, &res));

    *pUsed = res;
    *pUsed += (NvU32) sz;

    track->audio.samplerate = 8000;
    track->audio.out_samplerate = 0;

    while (track_used < sz)
    {
       NVMM_CHK_ERR (ebmlReadId (matroska, &id, &res));
        track_used += res;
        NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
        track_used += res;
        switch (id)
        {
                NvF64 *dptr;
            case MATROSKA_ID_AUDIOSAMPLINGFREQ:
                dptr = &track->audio.samplerate;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Audio sampling rate found - "));
                goto common_float;
            case MATROSKA_ID_AUDIOOUTSAMPLINGFREQ:
                dptr = &track->audio.out_samplerate;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Audio Out sampling rate found - "));
common_float:
                NVMM_CHK_ERR (ebmlReadFloat (matroska, length, dptr));
                track_used += (NvU32) length;
                break;
            case MATROSKA_ID_AUDIOBITDEPTH:
                ptr = &track->audio.bitdepth;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Audio Bit depth found - \n"));
                goto common_uint;
            case MATROSKA_ID_AUDIOCHANNELS:
                ptr = &track->audio.channels;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Audio channels found - \n"));
                goto common_uint;
common_uint:
                NVMM_CHK_ERR (ebmlReadUint (matroska, (NvU32) length, ptr));
                track_used += (NvU32) length;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "%llu\n", *ptr));
                break;
            default:
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                "Auidio: Impossible to get here check ID = %llx\n", id));
                Status = NvError_ParserFailure;
                goto cleanup;
        }
    }

    if (track->audio.out_samplerate == 0)
        track->audio.out_samplerate  = track->audio.samplerate;
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

    return Status;
}

static NvError mkvdParseTrackEncodings (NvMkvDemux *matroska, NvMatroskaTrack *track, NvU32 *pUsed)
{
    NvS32 res = 0;
    NvU64 sz;
    NvError Status = NvSuccess;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    NVMM_CHK_ARG (matroska);
    NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &sz, &res));

    *pUsed = 0;
    *pUsed += res;
    *pUsed += (NvU32) sz;

    NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, sz, CP_OriginCur));

cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

    return Status;
}

static NvError mkvdParseTracks (NvMkvDemux *matroska, NvU32 *pUsed)
{
    NvU64 length = 0;
    NvU64 sz;
    NvU64 id;
    NvU32 used = 0;
    NvU32 track_used = 0;
    NvS32 res = 0;
    NvMatroskaTrack *tracks = NULL;
    NvU32 track_cnt = 0;
    NvMatroskaTrack *track = NULL;
    NvU64 *ptr = NULL;
    NvS8  *ucptr = NULL;
    NvU64 skip;
    NvF64 scale = 1.0;
    NvError Status = NvSuccess;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    NVMM_CHK_ARG (matroska);
    NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &sz, &res));

    *pUsed = 0;
    *pUsed += res;
    *pUsed += (NvU32) sz;

    while (track_used < sz)
    {
        NVMM_CHK_ERR (ebmlReadId (matroska, &id, &res));
        track_used += res;
        switch (id)
        {
            case MATROSKA_ID_TRACKENTRY:
                // Read a length field that follows this
                NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
                track_used += res;
                break;
            case MATROSKA_ID_TRACKNUMBER:
                tracks = NvOsRealloc (tracks, sizeof (NvMatroskaTrack) * (track_cnt + 1));
                NVMM_CHK_MEM (tracks)
                track = &tracks[track_cnt];
                NvOsMemset (track, 0, sizeof (NvMatroskaTrack));

                // Keep this the track until a new tracknumber appears, then change the index
                track_cnt++;
                // allocate memory  TBD
                // Initialize the track
                track->timescale = 1.0;
                track->flag_default = 1;
                ptr = &track->num;
                goto common_uint;
            case MATROSKA_ID_TRACKUID:
                ptr = &skip;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "UID found\n"));
                goto common_uint;
            case MATROSKA_ID_TRACKTYPE:
                if (track)
                    ptr = &track->type;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Type found\n"));
                goto common_uint;
            case MATROSKA_ID_TRACKDEFAULTDURATION:
                if (track)
                    ptr = &track->default_duration;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Def dur\n"));
                goto common_uint;
            case MATROSKA_ID_TRACKFLAGDEFAULT:
                if (track)
                    ptr = &track->flag_default;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Def flag\n"));
                goto common_uint;
common_uint:
                // Read the size and then the uint
                NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
                track_used += res;
                NVMM_CHK_ERR (ebmlReadUint (matroska, (NvU32) length, ptr));
                track_used += (NvU32) length;
                break;
            case MATROSKA_ID_TRACKTIMECODESCALE:
                NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
                track_used += res;
                NVMM_CHK_ERR (ebmlReadFloat (matroska, length, &scale));
                if (track)
                {
                    track->timescale = (NvF32) scale;
                    NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "%f\n", track->timescale));
                }
                track_used += (NvU32) length;
                break;
            case MATROSKA_ID_CODECID:
                if (track)
                    ucptr = track->codec_id;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Reached codec id in track\n"));
                goto common_utf8;
            case MATROSKA_ID_TRACKLANGUAGE:
                ucptr  = track->language; // arbit size of 256 bytes
common_utf8:
                // Read the size and then the uint
                NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
                track_used += res;
                if (length < MATROSKA_STRING_LENGTH)
                {
                    Status = matroska->pPipe->cpipe.Read (matroska->cphandle, (CPbyte*) ucptr, (CPuint) length);  /* EOS or actual I/O error */
                    if (Status != NvSuccess)
                    {
                        matroska->done = NV_TRUE;
                        goto cleanup;
                    }
                }
                else
                {
                    NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_WARN, 
                    "Length of title or language string is greater than %d bytes - ignoring\n", length));
                    NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, length, CP_OriginCur));
                }
                ucptr[length] = '\0'; //Add end of string
                track_used += (NvU32) length;
                break;
            case MATROSKA_ID_TRACKVIDEO:
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "track video found\n"));
                NVMM_CHK_ERR (mkvdParseTrackVideo (matroska, track, &used));
                track_used += used;
                break;
            case MATROSKA_ID_TRACKAUDIO:
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Track audio found\n"));
                NVMM_CHK_ERR (mkvdParseTrackAudio (matroska, track, &used));
                track_used += used;
                break;
            case MATROSKA_ID_CODECPRIVATE:
                // Read the size
                NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
                track_used += res;
                // Allocate
                if (track)
                {
                    track->codec_prv_size = (NvU32) length;
                    track->codec_prv_data = (NvU8 *) NvOsAlloc ( (NvU32) length);
                    NVMM_CHK_MEM (track->codec_prv_data);
                }
                // Get the binary data
                Status = matroska->pPipe->cpipe.Read (matroska->cphandle, (CPbyte*) track->codec_prv_data, (CPuint) length);  /* EOS or actual I/O error */
                if (Status != NvSuccess)
                {
                    matroska->done = NV_TRUE;
                    goto cleanup;
                }
                track_used += (NvU32) length;
                break;
            case MATROSKA_ID_TRACKCONTENTENCODINGS:
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Track encoding found\n"));
                NVMM_CHK_ERR (mkvdParseTrackEncodings (matroska, track, &used));
                track_used += used;
                break;
            default: // do a skip on all the rest
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Unparsed IDs found - %llx\n", id));
                NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
                track_used += res;
                track_used += (NvU32) length;
                NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, length, CP_OriginCur));
                break;
        }
    }
    matroska->trackList.ulNumTracks = track_cnt;
    matroska->trackList.track = NvOsAlloc (sizeof (NvMatroskaTrack) * track_cnt);
    NVMM_CHK_MEM (matroska->trackList.track)
    NvOsMemcpy (matroska->trackList.track, tracks, sizeof (NvMatroskaTrack) * track_cnt);

cleanup:
    if (tracks)
    {
        NvOsFree (tracks);
        tracks = NULL;
    }
    matroska->track_parse_done = NV_TRUE;
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

    return Status;
}

static NvError MkvAddToClusterList(NvMkvDemux *matroska, NvU64 seek_pos)
{
    NvError Status = NvSuccess;

    NvMkvClusterNode *pClusterQ = NULL;
    NvU32 i = 0;
    NVMM_CHK_ARG (matroska);
    pClusterQ = matroska->clusterList.Q;

    if (matroska->clusterList.Q ==NULL)
    {
        matroska->clusterList.Q = NvOsAlloc(sizeof(NvMkvClusterNode));
        pClusterQ = matroska->clusterList.Q;
    }
    else
    {
        for (i=0; i < matroska->clusterList.QSz-1; i++)
        {
            if (pClusterQ)
            {
                pClusterQ = (NvMkvClusterNode *)pClusterQ->nxt;
            }
        }
        if (pClusterQ)
        {
            pClusterQ->nxt = NvOsAlloc (sizeof(NvMkvClusterNode));
            pClusterQ = (NvMkvClusterNode *)pClusterQ->nxt;
        }
    }
    NVMM_CHK_MEM (pClusterQ);
    pClusterQ ->node.position = seek_pos;
    pClusterQ->nxt = NULL;
    matroska->clusterList.QSz++;

cleanup:
    return Status;
}


static NvError mkvdParseSeekhead (NvMkvDemux *matroska, NvU32 *pUsed)
{
    NvS32 res = 0;
    NvU64 length;
    NvU64 sz;
    NvU32 seek_used = 0;
    NvU64 id;
    NvU64 *ptr;
    NvU64 seek_length = 0;
    NvU64 seek_id = 0;
    NvU64 seek_pos;
    NvBool bEbmlRead = NV_FALSE;
    NvError Status = NvSuccess;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    NVMM_CHK_ARG (matroska);
    NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &sz, &res));

    *pUsed = res;
    *pUsed += (NvU32) sz;

    while (seek_used < sz)
    {
        NVMM_CHK_ERR (ebmlReadId (matroska, &id, &res));
        seek_used += res;
        if (id != MATROSKA_ID_SEEKENTRY)
        {
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, "Error in seekhead parsing\n"));
            // TBD reposition at end of seek and clearly exit parsing
            Status = NvError_ParserFailure;
            goto cleanup;
        }
        NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &seek_length, &res));
        seek_used += res;
        seek_used += (NvU32) seek_length;
        while (seek_length)
        {
           NVMM_CHK_ERR (ebmlReadId (matroska, &id, &res));
            seek_length -= res;
            bEbmlRead = NV_FALSE;
            switch (id)
            {
                case MATROSKA_ID_SEEKID:
                    ptr = &seek_id;
                    bEbmlRead = NV_TRUE;
                    break;
                case MATROSKA_ID_SEEKPOSITION:
                    ptr = &seek_pos;
                    bEbmlRead = NV_TRUE;
                    break;
            }
            if(bEbmlRead)
            {
                 NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
                 seek_length -= res;
                 seek_length -= length;
                 NVMM_CHK_ERR (ebmlReadUint (matroska, (NvU32) length, ptr));
           }
        }
        if (seek_id == MATROSKA_ID_CUES)
        {
            if(!matroska->cue_start)
            {
                matroska->cue_start = seek_pos;
            }
        }
        else if (seek_id == MATROSKA_ID_CLUSTER)
        {
            // matroska->ClusterStart = seek_pos;
            NVMM_CHK_ERR (MkvAddToClusterList(matroska, seek_pos));
        }
        else if (seek_id == MATROSKA_ID_SEEKHEAD)
        {
            NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, 
            "mkvdParseSeekhead Pointing to next MATROSKA_ID_SEEKHEAD Start = %llu",  seek_pos));
            matroska->NextSeekHeadStart = seek_pos;
        }
        else if (seek_id == MATROSKA_ID_INFO)
        {
            NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, 
            "mkvdParseSeekhead :: MATROSKA_ID_INFO Start = %llu",  seek_pos));
        }
        else if (seek_id == MATROSKA_ID_TRACKS)
        {
            NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG,
            "mkvdParseSeekhead :: MATROSKA_ID_TRACKS Start = %llu",  seek_pos));
        }
    }
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

    return Status;
}

static NvError mkvdParseSegment (NvMkvDemux *matroska)
{
    NvS32 res = 0;
    NvU64 id;
    NvU64 segsize;
    NvU32 segused = 0;
    NvBool bKeepDigging = NV_TRUE;
    NvU32 used = 0;
    NvError Status = NvSuccess;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    NVMM_CHK_ARG (matroska);
    NVMM_CHK_ERR (ebmlReadId (matroska, &id, &res));
    if (id != MATROSKA_ID_SEGMENT)
    {
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
        "Invalid segment ID - ParseFailure \n"));
        Status = NvError_ParserFailure;
        goto cleanup;
    }

    NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &segsize, &res));

    matroska->segment_len = (NvU32) segsize;
    NVMM_CHK_CP (matroska->pPipe->GetPosition64 (matroska->cphandle, &matroska->segment_start));
    // Parse the segment - the segment length is all the data that follows
    while (bKeepDigging)
    {
        NvU64 size;
        res = 0;
       NVMM_CHK_ERR (ebmlReadId (matroska, &id, &res));
        segused += res;
        switch (id)
        {
            case MATROSKA_ID_INFO:
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Parsing Info\n"));
                NVMM_CHK_ERR (mkvdParseInfo (matroska, &used));
                segused += used;
                break;
            case MATROSKA_ID_TRACKS:
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Parsing Tracks\n"));
                NVMM_CHK_ERR (mkvdParseTracks (matroska, &used));
                segused += used;
                break;
            case MATROSKA_ID_CUES:
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Parsing Cues\n"));
                NVMM_CHK_ERR (mkvdParseCues (matroska, &used));
                segused += used;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Parse cues succeeded\n"));
                break;
            case MATROSKA_ID_SEEKHEAD:
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Parsing Seek\n"));
                // Copy the seekhead information into a buffer for later use
                NVMM_CHK_ERR (mkvdParseSeekhead (matroska, &used));
                segused += used;
                break;
            case EBML_ID_VOID:
            case MATROSKA_ID_CHAPTERS:
            case MATROSKA_ID_ATTACHMENTS:
            case MATROSKA_ID_TAGS:
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_WARN,
                "Skipping void chapters att tags %llx\n", id));
                // Skip these elements for now
                res = 0;
                NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &size, &res));
                segused += res;
                segused += (NvU32) size;
                NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, size, CP_OriginCur));
                break;
            case MATROSKA_ID_CLUSTER:
                matroska->has_cluster_id = 1;
                matroska->ClusterStart = matroska->segment_start + segused;
                bKeepDigging = NV_FALSE;
                break;
            default:
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, 
                "Error in parsing segment - bad ID - %llx exit \n", id));
                Status = NvError_ParserFailure;
                goto cleanup;
        }
    }
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));
    return Status;
}

// Call this function after we know how many tracks are present
static NvError mkvdInitAVqs (NvMkvDemux *matroska)
{
    NvU32 i = 0;
    NvError Status = NvSuccess;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    NVMM_CHK_ARG (matroska);
    // If number of streams is more than NVMM_MKVPARSER_MAX_TRACKS, return error
    if (matroska->trackList.ulNumTracks  > NVMM_MKVPARSER_MAX_TRACKS)
    {
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, 
        "More than max permitted streams %d > %d\n",
        matroska->trackList.ulNumTracks , NVMM_MKVPARSER_MAX_TRACKS));
        Status =  NvError_ParserFailure; // TODO:: Fix this. Limit to MaxStreams
    }
    for (i = 0; i < NVMM_MKVPARSER_MAX_TRACKS; i++)
    {
        matroska->avPktList[i].QSz = 0;
        matroska->avPktList[i].Q = NULL;
        matroska->blkList[i].QSz = 0;
        matroska->blkList[i].Q = NULL;
    }
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

    return Status;
}


/* This is the first of the parsing functions that are called at the beginning of demux. This reads the top level EBML file header
Subsequently, it reads the segment headers. In some cases, the cue may be available at the beginning of the file and may be read
right away. However, in streaming cases, traversal to another part of the file is not possible and hence cue parsing may be deferred */
NvError mkvdReadHeader (NvMkvDemux *pDemux)
{
    NvError Status = NvSuccess;

    NvMkvDemux *matroska = NULL;
    NvMatroskaTrack *tracks;
    NvMkvEbml ebml;
    NvU32 i;
    NvU32 used = 0;
    NvU64 CurrenPosition = 0;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    NvOsMemset (&ebml, 0, sizeof (ebml));
    NVMM_CHK_ARG (pDemux);

    matroska = pDemux;

    /* Read the top level EBML syntax header */
    NVMM_CHK_ERR (mkvdParseSyntax (matroska, &ebml));

    if (ebml.version > EBML_VERSION       || ebml.max_size > sizeof (NvU64)
            || ebml.id_length > sizeof (NvU32) || NvOsStrcmp (ebml.doctype, "matroska")
            || ebml.doctype_version > 2)
    {
        Status = NvError_ParserCorruptedStream;
        goto cleanup;
    }
    if (ebml.doctype)
    {
       NvOsFree (ebml.doctype);
       ebml.doctype = NULL;
    }

    NVMM_CHK_ERR (mkvdParseSegment (matroska));

    NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_INFO,
    "Duration of the content is %f\n", matroska->duration));
    NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_WARN, "Ignoring tags, Attachments TBD\n"));
    NVMM_CHK_CP (matroska->pPipe->GetPosition64 (matroska->cphandle, &CurrenPosition));
    if (matroska->cue_start && !matroska->cue_parse_done /* && !streaming */)
    {

        /* TBD - We may need to check for streaming or alternately if the cue table is within a "readable" distance */
        /* Must check with Mobile team to see how streaming works */

        NvS32 res = 0;
        NvU64 id;
        NVMM_CHK_CP (matroska->pPipe->GetPosition64 (matroska->cphandle, &matroska->save_pos));

        // If we are on streaming mode, this shouldnt be done !!! TBD
        NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, (matroska->segment_start + matroska->cue_start), CP_OriginBegin));
        NVMM_CHK_ERR (ebmlReadId (matroska, &id, &res));
        if (id != MATROSKA_ID_CUES)
        {
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, "Parse Cues Error - ID Found %llx\n", id));
            goto repos_after_cue;
        }
        NVMM_CHK_ERR (mkvdInitAVqs (matroska));
        if (mkvdParseCues(matroska, &used) != NvSuccess)
        {
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, "Parse Cues Failed\n", id));
            goto repos_after_cue;
        }
repos_after_cue:
        NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, matroska->save_pos, CP_OriginBegin));
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, 
        "Set the position back to the starting point %llx\n", matroska->save_pos));
    }
    /* Check if there are any tracks available in the stream */
    if (0 == matroska->trackList.ulNumTracks)
    {
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_WARN, "No playable tracks found\n"));
        Status = NvError_ParserFailure;
        goto cleanup;
    }
    /* After tracks are available, set up the track specific data queues */
    NVMM_CHK_ERR (mkvdInitAVqs (matroska));
    /* Initialize the codecs from the track information */
    tracks = matroska->trackList.track;
    for (i = 0; i < matroska->trackList.ulNumTracks; i++)
    {
        NvMatroskaTrack *track = &tracks[i];
        /* Apply some sanity checks. */
        if (track->type != NvMkvTrackTypes_VIDEO &&
            track->type != NvMkvTrackTypes_AUDIO &&
            track->type != NvMkvTrackTypes_SUBTITLE)
        {
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_WARN,
            "Unknown or unsupported track type \n", track->type));
            continue;
        }
        if (track->codec_id == NULL)
            continue;

        track->user_enable = NV_FALSE;
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_INFO, "Codec is : %s\n", track->codec_id));
        if ((track->type == NvMkvTrackTypes_AUDIO) && (!matroska->stream_has_audio))
        {
            if (!track->audio.out_samplerate)
            {
                track->audio.out_samplerate = track->audio.samplerate;
            }
            if (!NvOsStrncmp ( (const char *) track->codec_id, "A_AAC", 5))
            {
                NvMkvAacConfigurationData *aacData = NULL;
                track->codec_type =  NvMkvCodecType_AAC;
                track->user_enable = NV_TRUE;
                // Populate AAC TrackInfo
                aacData = &matroska->aacConfigurationData;
                NvOsMemset (aacData, 0, sizeof (NvMkvAacConfigurationData));
                aacData->objectType = MPEG4_AUDIO;
                aacData->samplingFreq = (NvU32) track->audio.samplerate;
                aacData->noOfChannels = (NvU32) track->audio.channels;
                NVMM_CHK_ERR (prvAudioFsToId ( (NvU32)track->audio.samplerate, &aacData->samplingFreqIndex));
                // TBD: None of this information is available in mkv files
                aacData->bitrate = 0;               //TBD
                aacData->channelConfiguration = 0;  //TBD
                aacData->sampleSize = 16;           //TBD
                matroska->stream_has_audio = NV_TRUE;
                matroska->AudioStreamId = (NvU32)track->num;
            }
            else if (!NvOsStrncmp ( (const char *) track->codec_id, "A_MPEG/L3", 9))
            {
                NvMkvMp3ConfigurationData *mp3Data = NULL;
                track->codec_type =  NvMkvCodecType_MP3;
                track->user_enable = NV_TRUE;
                mp3Data = &matroska->mp3ConfigurationData;
                NVMM_CHK_ERR (prvAudioFsToId ( (NvU32)track->audio.samplerate, &mp3Data->samplingFreqIndex));
                mp3Data->samplingFreq = (NvU32) track->audio.samplerate;
                mp3Data->noOfChannels = (NvU32) track->audio.channels;
                mp3Data->sampleSize = 16;
                mp3Data->channelConfiguration = 0;
                matroska->stream_has_audio = NV_TRUE;
                matroska->AudioStreamId = (NvU32)track->num;
            }
            else if (!NvOsStrncmp ( (const char *) track->codec_id, "A_VORBIS", 8))
            {
                track->codec_type =  NvMkvCodecType_VORBIS;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, 
                "Found Vorbis audio - Not supported - Ignoring track\n"));
            }
            else if (!NvOsStrncmp ( (const char *) track->codec_id, "A_AC3", 5))
            {
                NvMkvAc3ConfigurationData *ac3Data = &matroska->ac3ConfigurationData;
                track->user_enable = NV_TRUE;
                NVMM_CHK_ERR (prvAudioFsToId ( (NvU32)track->audio.samplerate, &ac3Data->samplingFreqIndex));
                ac3Data->samplingFreq = (NvU32) track->audio.samplerate;
                ac3Data->noOfChannels = (NvU32) track->audio.channels;
                ac3Data->sampleSize = 16;
                ac3Data->channelConfiguration = 0;
                matroska->stream_has_audio = NV_TRUE;
                matroska->AudioStreamId = (NvU32)track->num;
                track->codec_type =  NvMkvCodecType_AC3;
            }
            else if ( (!NvOsStrncmp ( (const char *) track->codec_id, "A_DTS", 5)) ||
                (!NvOsStrncmp ( (const char *) track->codec_id, "A_EAC3", 6)) ||
                (!NvOsStrncmp ( (const char *) track->codec_id, "A_FLAC", 6)) ||
                (!NvOsStrncmp ( (const char *) track->codec_id, "A_MPEG/L2", 9)) ||
                (!NvOsStrncmp ( (const char *) track->codec_id, "A_MPEG/L2", 9)))
            {
                    NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                    "Found %s type audio - Not supported - Ignoring track\n", track->codec_id));
            }
            else
            {
                    NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                    "Ignoring Unknown audio of type %s\n", track->codec_id));
            }
        }
        else if ((track->type == NvMkvTrackTypes_VIDEO) && (!matroska->stream_has_video))
        {
            if ((!track->default_duration) && track->video.frame_rate)
                track->default_duration = (NvU64) (1000000000 / track->video.frame_rate);
            if (!track->video.display_width)
                track->video.display_width = track->video.pixel_width;
            if (!track->video.display_height)
                track->video.display_height = track->video.pixel_height;

            if (!NvOsStrncmp ( (const char *) track->codec_id, "V_MPEG4/ISO/AVC", 15))
            {
                track->codec_type =  NvMkvCodecType_AVC;
                track->user_enable = NV_TRUE;
                matroska->stream_has_video = NV_TRUE;
                matroska->VideoStreamId = (NvU32)track->num;

                // TBD we may need a Status check to see if this data doesnt match what we expected to see
                if (mkvdParseAvcConfig (matroska, track->codec_prv_data, track->codec_prv_size) < 0)
                {
                    Status = NvError_ParserFailure;
                    goto cleanup;
                }
            }
            else if ((!NvOsStrncmp ( (const char *) track->codec_id, "V_MS/VFW/FOURCC", 15)) 
            || (!NvOsStrncmp ( (const char *) track->codec_id, "V_MPEG4/ISO/ASP", 15)))
            {
                track->codec_type =  NvMkvCodecType_MPEG4;
                track->user_enable = NV_TRUE;
                matroska->stream_has_video = NV_TRUE;
                matroska->VideoStreamId = (NvU32)track->num;
            }
            else if (!NvOsStrncmp ( (const char *) track->codec_id, "V_MPEG2", 7))
            {
                track->codec_type =  NvMkvCodecType_MPEG2;
                track->user_enable = NV_TRUE;
                matroska->stream_has_video = NV_TRUE;
                matroska->VideoStreamId = (NvU32)track->num;
            }
            else
            {
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                "Ignoring Unknown video of type %s\n", track->codec_id));
            }
        }
        else if (track->type == MATROSKA_TRACK_TYPE_SUBTITLE)
        {
            matroska->stream_has_tits = NV_TRUE;
        }
    }
    // TBD : We ignore chapters for now
    // TBD : We ignore Index for now
    // TBD : We ignore attachments for now
    Status = NvMkvScanIFrame(matroska);
    if (Status != NvSuccess)
    {
        NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,
        "NvMkvScanIFrame Status = %x\n", Status));
        Status = NvSuccess;
    }
    NVMM_CHK_ERR (mkvdInitAVqs (matroska));
    matroska->cluster->state = MKV_CLUSTER_NEW_OPEN;
    matroska->has_cluster_id = 1;
    matroska->seek_keyframe = NV_FALSE;
    matroska->seek_timecode_found = 0;
    Status = (matroska->pPipe->SetPosition64 (matroska->cphandle, CurrenPosition, CP_OriginBegin));
    if (Status != NvSuccess)
    {
        Status = NvSuccess;
    }
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER,NVLOG_DEBUG, "MkvReadHdr Status = %x - Mkv->done = %x\n",
    Status, matroska->done));
    if (ebml.doctype)
    {
        NvOsFree (ebml.doctype);
        ebml.doctype = NULL;
    }
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));
    return Status;
}

NvError mkvdSetPosReinit (NvMkvDemux *matroska)
{
    NvError Status = NvSuccess;
    NvU32 i = 0;
    NvMkvAVPNode *temp0;
    NvMkvBlkNode *temp1;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    // Number of tracks that are present - clear all those queues first - release memory
    for (i = 0; i < matroska->trackList.ulNumTracks; i++)
    {

        while (matroska->avPktList[i].QSz)
        {
            // Free the used node and house-keeping on list
            matroska->avPktList[i].QSz--;
            NvOsFree (matroska->avPktList[i].Q->node); // Free the packet
            temp0 = matroska->avPktList[i].Q->nxt;
            NvOsFree (matroska->avPktList[i].Q);
            matroska->avPktList[i].Q = temp0;
        }
        while (matroska->blkList[i].QSz)
        {
            // Free the used node and house-keeping on list
            matroska->blkList[i].QSz--;
            NvOsFree (matroska->blkList[i].Q->node); // Free the packet
            temp1 = matroska->blkList[i].Q->nxt;
            NvOsFree (matroska->blkList[i].Q);
            matroska->blkList[i].Q = temp1;
        }
    }
    for (i = 0; i < NVMM_MKVPARSER_MAX_TRACKS; i++)
    {
        matroska->avPktList[i].QSz = 0;
        matroska->avPktList[i].Q = NULL;

        matroska->blkList[i].QSz = 0;
        matroska->blkList[i].Q = NULL;
    }
    NvOsMemset (matroska->cluster, 0, sizeof (NvMkvParseCluster));

    matroska->cluster->state = MKV_CLUSTER_NEW_OPEN;
    matroska->prev_pkt.pts = AV_NOPTS_VALUE;
    matroska->prev_pkt.stream_index = -1;
    matroska->seek_keyframe = NV_FALSE;
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

    return Status;

}

static NvError mkvdAVNewPacket (NvMkvAVPacket *pkt, NvU64 pos, NvS32 size)
{
    NvError Status = NvSuccess;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    NVMM_CHK_MEM (pkt && pos);
    if (size  == -1)
    {
        Status =  NvError_BadParameter;
    }
    pkt->pts   = AV_NOPTS_VALUE;
    pkt->duration = 0;
    pkt->flags = 0;
    pkt->stream_index = 0;
    pkt->pos = pos;
    pkt->size = size;
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));
    return Status;
}

// info needs to be the appropriate Q after indexing into the array of structures
static NvError mkvdAVqAddPacket (NvMkvAVPktsInfo* info, NvMkvAVPacket *pkt)
{
    // Aer some checks needed here?
    NvError Status = NvSuccess;
    NvMkvAVPNode *p = NULL;
    NvU32 i = 0;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    NVMM_CHK_ARG (info);
    p = info->Q;
    if (!p)
    {
        info->Q = NvOsAlloc (sizeof (NvMkvAVPNode));
        p = info->Q;
    }
    else
    {
        for (i = 0; i < info->QSz - 1; i++)
        {
            if (!p)
            {
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, 
                "mkvdAVqAddPacket error - pointer NULL\n"));
            }
            if (p)
            {
                p = p->nxt;
            }
        }
        if (p)
        {
            p->nxt = NvOsAlloc (sizeof (NvMkvAVPNode));
            p = p->nxt;
     }
    }
    NVMM_CHK_MEM (p)
    p->node = pkt;
    p->nxt = NULL;
    info->QSz++;
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));
    return Status;
}

static NvError mkvdBlkAddPacket (NvMkvBlocksInfo *info, NvMkvBlock *pkt)
{
    NvMkvBlkNode *p = NULL;
    NvMkvBlock *blk = NULL;
    NvU32 i = 0;
    NvError Status = NvSuccess;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    NVMM_CHK_ARG (info);
    p = info->Q;

    if (!p)
    {
        info->Q = NvOsAlloc (sizeof (NvMkvBlkNode));
        p = info->Q;
    }
    else
    {
        // Find the last node
        for (i = 0; i < info->QSz - 1; i++)
        {
            if (p)
            {
                p = (NvMkvBlkNode *) p->nxt;
            }
        }
        if (p)
        {
            p->nxt = NvOsAlloc (sizeof (NvMkvBlkNode));
            p = (NvMkvBlkNode *) p->nxt;
        }
    }
    NVMM_CHK_MEM (p);
    blk = NvOsAlloc (sizeof (NvMkvBlock));
    NVMM_CHK_MEM (blk);
    NvOsMemcpy (blk, pkt, sizeof (NvMkvBlock));
    p->node = blk;
    p->nxt = NULL;
    info->QSz++;
cleanup:
    if(!blk)
    {
        if( p)
        {
            NvOsFree (p);
            p = NULL;
       }
    }
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));
    return Status;
}

NvError mkvdFetchData (NvMkvDemux *matroska, NvMkvAVPacket *pkt, NvU8 *data, NvU32 *szAvail)
{
    NvError Status = NvSuccess;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    NVMM_CHK_ARG (matroska && pkt && data);

    // Fetch data from the specified AV packet and copy into the ptr provided - check against sz of buffer available
    if  (pkt->size > (NvS32)*szAvail)
    {
        Status = NvError_InSufficientBufferSize;
        goto cleanup;
    }
    NVMM_CHK_CP (matroska->pPipe->GetPosition64 (matroska->cphandle, &matroska->save_pos));
    NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, pkt->pos, CP_OriginBegin));
    if (matroska->pPipe->cpipe.Read (matroska->cphandle, (CPbyte*) data, pkt->size) != NvSuccess)
    {
        // TBD - Needs revisit when implementing error handling
        matroska->done = NV_TRUE;
        Status = NvError_ParserFailure;
        goto cleanup;
    }
    NVMM_CHK_CP(matroska->pPipe->SetPosition64 (matroska->cphandle, matroska->save_pos, CP_OriginBegin));
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));
    return Status;
}

static NvError mkvdParseKeyFrameBlock (NvMkvDemux *matroska, NvMkvBlock *block)
{
    // cluster_timecode is available in block itself - use from there
    NvU64 timecode = AV_NOPTS_VALUE;
    NvMatroskaTrack *track;
    NvS32 res = 0;
    NvMkvAVPacket *pkt = NULL;
    NvS16 block_time;
    NvU32 *lace_size = NULL;
    NvS32 n, flags, laces = 0;
    NvU64 num;
    NvU64 tracknum;
    NvU8 scratch[16];
    NvS32 is_keyframe = 0;
    NvU64 position = 0;
    NvError Status = NvSuccess;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER,NVLOG_INFO, "++%s[%d]", __FUNCTION__, __LINE__));
    NVMM_CHK_ARG (matroska);
    // Save current position, then reposition into the file to point to the block to be parsed
    NVMM_CHK_CP (matroska->pPipe->GetPosition64 (matroska->cphandle, &matroska->save_pos));

    // Set the position to the block data
    // This position must reflect the location after the ID and size fields
    NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, block->bin_pos, CP_OriginBegin));
    position = block->bin_pos;
    NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &tracknum, &res));
    position += res;
    block->bin_size -= res;
    track = mkvdFindTrackByNum (matroska, (NvS32) tracknum);
    if (track->type == NvMkvTrackTypes_VIDEO)
    {
        if ( (block->bin_size < 3) || !track)
        {
            // TBD - Needs revisit when implementing error handling
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, "Invalid stream %d\n", tracknum));
            Status = NvSuccess; //NvError_ParserFailure;
            goto cleanup;
        }
        // TODO How to discard a block from a particualr track --- need to be fixed
        if (block->duration == (NvU64) AV_NOPTS_VALUE)
        {
            block->duration = (NvU64) ( (NvF32) track->default_duration / (NvF32) matroska->time_scale);
        }
        NVMM_CHK_CP (matroska->pPipe->cpipe.Read (matroska->cphandle, (CPbyte*) scratch, 3));
        // TBD jump to a label, where we set filepos to the saved value
        block_time = scratch[0];
        block_time <<= 8;
        block_time |= scratch[1];
        // int8 Flags
        flags = scratch[2];
        position += 3;
        block->bin_size -= 3;
        // If block reference was not sent, then it is a key frame (Matroska v1)
        // flags may also be used in Matroska v2
        // block->reference = 0: indicates key frame.
        is_keyframe = (!block->reference) || (flags & 0x80 ? 1 : 0);
        if (is_keyframe)
        {
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG,
            "Keyframe Found = %x - flags = %x - block_reference = %x\n",
            is_keyframe, flags, block->reference));
        }
        if ( (block->cluster_timecode != (NvU64) - 1) && ( (block_time >= 0) ||
           ((NvS64) block->cluster_timecode >= -block_time)))
        {
            timecode = block->cluster_timecode + block_time;
        }
        if (!is_keyframe)
        {
            matroska->seek_timecode_found = timecode;
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG,
            "Returning since correct key frame is not located\n"));
            NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, matroska->save_pos, CP_OriginBegin));
            // TBD: Should this be NvSuccess;
            goto cleanup;
        }
        matroska->seek_keyframe = NV_FALSE;
        matroska->seek_timecode = 0;
        matroska->seek_tracknum = 0;
        matroska->seek_blocknum = -1;
        res = 0;
        switch ( (flags & 0x06) >> 1)
        {
        case 0x0: /* no lacing */
            laces = 1;
            lace_size = NvOsAlloc (sizeof (int));
            NVMM_CHK_MEM (lace_size);
            // Add check here
            lace_size[0] = block->bin_size;
            break;
        case 0x1: /* Xiph lacing */
        case 0x2: /* fixed-size lacing */
        case 0x3: /* EBML lacing */
            NVMM_CHK_CP (matroska->pPipe->cpipe.Read (matroska->cphandle, (CPbyte*) scratch, 1));
            position += 1;
            block->bin_size -= 1;
            laces = scratch[0] + 1;
            lace_size = NvOsAlloc (laces * sizeof (int));
            NVMM_CHK_MEM (lace_size);
            NvOsMemset (lace_size, 0, laces * sizeof (int));
            switch ( (flags & 0x06) >> 1)
            {
            case 0x1: /* Xiph lacing */
            {
                NvU32 total = 0;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER,NVLOG_INFO, "In Xiph encode\n"));
                for (n = 0; res == 0 && n < laces - 1; n++)
                {
                    while (1)
                    {
                        if (block->bin_size == 0)
                        {
                            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                            "Xiph encode , res = -1\n"));
                            Status = NvError_ParserFailure;
                            break;
                        }
                        if (matroska->pPipe->cpipe.Read (matroska->cphandle, (CPbyte*) scratch, 1) != NvSuccess)
                        {
                            Status = NvError_ParserFailure;
                            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                            "Xiph lacing - Parser ReadFailure\n"));
                            break;
                        }
                        position++;
                        block->bin_size -= 1;
                        lace_size[n] += scratch[0];
                        if (scratch[0] != 0xff)
                            break;
                    }
                    total += lace_size[n];
                }
                lace_size[n] = block->bin_size - total;
                break;
            }
            case 0x2: /* fixed-size lacing */
                for (n = 0; n < laces; n++)
                {
                    lace_size[n] = block->bin_size / laces;
                }
                break;
            case 0x3: /* EBML lacing */
            {
                NvU32 total;
                if ((Status = ebmlReadNum (matroska, 8, &num, &res)) != NvSuccess)
                {
                    NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                    "EBML lacing err- ebmlReadNum Status = %x\n", Status));
                    break;
                }
                position += res;
                block->bin_size -= res;
                total = lace_size[0] = (NvU32) num;
                for (n = 1; res >= 0 && n < laces - 1; n++)
                {
                    NvS64 snum = 0;
                    Status= ebmlReadSint (matroska, 4, &snum, &res);
                    position += res;
                    block->bin_size -= res;
                    if ((res < 0) || (Status != NvSuccess))
                    {
                        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                        "EBML lacing err - ebmlReadSint Status = %x\n", Status));
                        break;
                    }
                    lace_size[n] = lace_size[n - 1] + (NvS32) snum;
                    total += lace_size[n];
                }
                lace_size[n] = block->bin_size - total;
                break;
            }
        }
        break;
        }
        if ((res < 0) || (Status != NvSuccess))
        {
            if (lace_size)
            {
                NvOsFree (lace_size);
                lace_size = NULL;
            }
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
            "ERORR Parse Lacing Status = %x\n", Status));
            goto cleanup;
        }
        res = 0;
        // Add the laced data into the AVPackets Q
        if (res == 0)
        {
            for (n = 0; n < laces; n++)
            {
                NvU32 pkt_size = lace_size[n];
                if (pkt_size >= matroska->largestKeyFrameSize)
                {
                    matroska->largestKeyFrameSize = pkt_size;
                    matroska->largestKeyTimeStamp = timecode;
                    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,
                    "ScanEntry = %d -  KeyFrameSize = %d - KeyTimeStamp = %llu\n",
                    matroska->ScanEntry, matroska->largestKeyFrameSize, matroska->largestKeyTimeStamp));
                }
                if(++matroska->ScanEntry > MAX_KEYFRAME_SCAN_LIMIT)
                {
                    matroska->IdentifiedKeyThumbNailFrame = NV_TRUE;
                    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG,
                    "matroska-ScanEntry exceeds MAX_KEYFRAME_SCAN_LIMIT\n"));
                    break;
                }
            }
        }
        // parse_done: Restore the position back
        NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, matroska->save_pos, CP_OriginBegin));
    }
    else
    {
        NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, matroska->save_pos, CP_OriginBegin));
    }
cleanup:
    if ((res < 0) || (Status != NvSuccess))
    {
        if (pkt)
        {
            NvOsFree (pkt);
            pkt = NULL;
        }
    }
    if (lace_size)
    {
        NvOsFree (lace_size);
        lace_size = NULL;
    }
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER,NVLOG_INFO, "--%s[%d]", __FUNCTION__, __LINE__));
    return Status;
}

static NvError mkvdExtractAndQSimpleBlock (NvMkvDemux *matroska, 
    NvMkvParseCluster *pCluster,
    NvMkvParseClusterData ReadType)
{
    NvError Status = NvSuccess;
    NvS32 res = 0;
    NvU64 length = 0;
    NvMkvBlock block;
    NvS64 tracknum = -1;
    NvMatroskaTrack *tracks;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    NvOsMemset (&block, 0, sizeof (NvMkvBlock));
    NVMM_CHK_ARG (matroska && pCluster);

    block.cluster_timecode = pCluster->timecode;
    block.duration = (NvU64) AV_NOPTS_VALUE;
    {
        NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
        block.blk_size = block.bin_size = (NvU32)length;
        pCluster->used += res;
        // save bin_pos as well right now. This is the data for the block parsing to be done ahead
        NVMM_CHK_CP (matroska->pPipe->GetPosition64 (matroska->cphandle, &block.bin_pos));
        // This bin_pos reflects the position in the block after the EBML ID and size.
        // It points to the beginning of the binary data
        // Below, read the track number, but dont move the bin_pos above
        NVMM_CHK_ERR (ebmlReadNum (matroska, 8, (NvU64*) & tracknum, &res));
        res = /* block.bin_pos + */block.bin_size - res;
        // reposition to the end of this block's binary data - without reading it for now
        NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, res, CP_OriginCur));
    }
    pCluster->used += block.blk_size;
    // default set 1: For simple block reference =1 indicated non-key frames and
    // 0 indicates key frame. Set 0 for key frame on seek.
    block.reference = 1;
    tracknum = mkvdFindIndexByNum (matroska, (NvS32) tracknum);
    tracks = &matroska->trackList.track[tracknum];
    if ( (tracknum != -1) && block.bin_size && tracks->user_enable)
    {
        if(NvMkvParseClusterData_KEY_FRAME_SCAN == ReadType)
        {
            NVMM_CHK_ERR (mkvdParseKeyFrameBlock (matroska, &block));
        }
        else
        {
            // We have a block and we have its track. Add it to the appropriate queue
            NVMM_CHK_ERR (mkvdBlkAddPacket (&matroska->blkList[tracknum], &block));
        }
    }
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));
    return Status;
}


static NvError mkvdExtractAndQBlock (NvMkvDemux *matroska, 
    NvMkvParseCluster *pCluster,
    NvMkvParseClusterData ReadType)
{
    NvError Status = NvSuccess;
    NvS32 res = 0;
    NvU64 length = 0;
    NvMkvBlock block;
    NvS32 size;
    NvS64 tracknum = -1;
    NvMatroskaTrack *tracks;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    NvOsMemset (&block, 0, sizeof (NvMkvBlock));
    NVMM_CHK_ARG (matroska && pCluster);
    // keep a copy because this block may be parsed after a new cluster has been opened for some other streams's sake
    block.cluster_timecode = pCluster->timecode;
    block.duration = (NvU64) AV_NOPTS_VALUE;
    NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
    block.blk_size = (NvU32) length;
    pCluster->used += res;
    size = block.blk_size;
    while (size > 0)
    {
        NvU64 id;
        NVMM_CHK_ERR (ebmlReadId (matroska, &id, &res));
        size -= res;
        switch (id)
        {
            case MATROSKA_ID_BLOCKDURATION:
                NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
                size -= res;
                NVMM_CHK_ERR (ebmlReadUint (matroska, (NvU32) length, &block.duration));
                size -= (NvU32) length;
                break;
            case MATROSKA_ID_BLOCKREFERENCE:
                NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
                size -= res;
                NVMM_CHK_ERR (ebmlReadUint (matroska, (NvU32) length, &block.reference));
                // This is a signed integer according to specification. So, convert
                block.reference = block.reference - ( (1LL << 8 * length) - 1);
                size -= (NvU32) length;
                break;
            case MATROSKA_ID_BLOCK:
                NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
                block.bin_size = (NvU32) length;
                // save bin_pos as well right now. This is the data for the block parsing to be done ahead
                size -= res;
                size -= block.bin_size;
                NVMM_CHK_CP (matroska->pPipe->GetPosition64 (matroska->cphandle, &block.bin_pos));
                // This bin_pos reflects the position in the block after the EBML ID and size.
                // It points to the beginning of the binary data
                // Below, read the track number, but dont move the bin_pos above
                NVMM_CHK_ERR (ebmlReadNum (matroska, 8, (NvU64*) & tracknum, &res));
                res = /* block.bin_pos + */block.bin_size - res;
                // reposition to the end of this block's binary data - without reading it for now
                NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, res, CP_OriginCur));
                break;
        }
    }
    pCluster->used += block.blk_size;
    // We have a block and we have its track. Add it to the appropriate queue
    // Find the corresponding internal "user" tracknum
    if (tracknum == -1)
    {
        Status = NvError_UnSupportedStream;
        goto cleanup;
    }

    tracknum = mkvdFindIndexByNum (matroska, (NvS32) tracknum);
    tracks = &matroska->trackList.track[tracknum];
    if ( (tracknum != -1) && block.bin_size && tracks->user_enable)
    {
        if(NvMkvParseClusterData_KEY_FRAME_SCAN == ReadType)
        {
            NVMM_CHK_ERR (mkvdParseKeyFrameBlock (matroska, &block));
        }
        else
        {
            NVMM_CHK_ERR (mkvdBlkAddPacket (&matroska->blkList[tracknum], &block));
        }
    }
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));
    return Status;
}

NvError mkvdScanClusterTimeCode (NvMkvDemux *matroska, NvU64 SeekTime, NvU64 *ClusterTime, 
    NvU64 *ClusterPosition, NvBool SeekHeadPresent,
    NvMkvParseClusterData ReadType)
{
    /*
    0. Run this whole thing in a while(1) loop and break out
       a. If we find a block, we break... if we run out of blocks, then we set state to
    1. First check how many blocks were supposed to be here.. have we parsed out all the blocks ? If so, goto 4
    2. Read the next block ID, size, (other information), identify the block stream by reading into the data a bit
    3. Move from MatroskaBlock type into NvMkvBlock type of structure by calling add block function
    4. Set state to MKV_CLUSTER_NEW_OPEN at the beginning of file parsing - basically ID parsing only (This is to distinguish other IDs that come in between)
    5. Fix all return values
    */
    NvError Status = NvSuccess;
    NvMkvParseCluster *cluster;
    NvBool bKeepParsing = NV_TRUE;
    NvU64 length;
    NvU32 used = 0;
    NvU64 bytesleft = 0;
    NvBool bClusterUnits = NV_FALSE;
    NvU64 PreviousClusterTime =0;
    NvU64 PreviousClusterPosition = 0;
    NvU64 SeekPosition = 0;
    NvMkvCluster *node = NULL;
    NvMkvClusterNode *Q = NULL;
    NvU32 i=0;
    NvS32 res = 0;
    NvU64 id = 0;

    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    NVMM_CHK_ARG (matroska);
    cluster = matroska->cluster;
    /* Check if we are at the end of file */
    NVMM_CHK_CP (matroska->pPipe->GetAvailableBytes (matroska->cphandle, &bytesleft));
    if (bytesleft == 0)
    {
        if(NvMkvParseClusterData_KEY_FRAME_SCAN == ReadType)
        {
            matroska->IdentifiedKeyThumbNailFrame = NV_TRUE;
            goto cleanup;
        }
        else
        {
            matroska->done = NV_TRUE;
        }
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
        "Non_cue mkvdAddBlockFromCluster bytesleft == 0 EOS\n"));
    }

    if(SeekHeadPresent)
    {
        Q = matroska->clusterList.Q;
        NVMM_CHK_ARG (Q);
        node = &Q->node;
        Q = (NvMkvClusterNode *)Q->nxt;
        SeekPosition = matroska->segment_start  + node->position;
        matroska->has_cluster_id = 0;
    }
    else
    {
        SeekPosition = matroska->ClusterStart;
        matroska->has_cluster_id = 1;
    }
    NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, (CPint64)SeekPosition, CP_OriginBegin));

    do
    {
        res = 0;
        id = 0;
        bClusterUnits = NV_FALSE;
        switch (cluster->state)
        {
            case MKV_CLUSTER_NEW_OPEN:
                // We seem to need a new cluster parse... lets do that
                // Read the ID from the file. This must be the top-level ID
                if (!matroska->has_cluster_id)
                {
                    if ((Status = ebmlReadId (matroska, &id, &res)) != NvSuccess)
                    {
                        matroska->done = 1;
                        goto cleanup;
                    }
                }
                else
                {
                    id = MATROSKA_ID_CLUSTER;
                    matroska->has_cluster_id = 0;
                }
                switch (id)
                {
                    case MATROSKA_ID_CLUSTER:
                        cluster->timecode = (NvU64) AV_NOPTS_VALUE; // Default
                        NVMM_CHK_CP (matroska->pPipe->GetPosition64 (matroska->cphandle, &cluster->begin));
                        // The following number in the stream is the length of the cluster
                        NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &cluster->length, &res));
                        //cluster->used += res;
                        cluster->used = 0;
                        cluster->state = MKV_CLUSTER_PARSE_BODY;
                        cluster->end = cluster->begin + cluster->length + res;
                        cluster->begin -= 4; //compensate for the 4 byte ID already read
                        break;
                        // Usually, the following are all parsed in the beginning of the file in ReadHeader
                        // For all the following, read the size field and skip the data completely (if it has already been parsed)
                    case MATROSKA_ID_CUES:
                        NVMM_CHK_ERR (mkvdParseCues (matroska, &used));
                        break;
                    case MATROSKA_ID_SEEKHEAD:
                    case MATROSKA_ID_TAGS:
                    case MATROSKA_ID_INFO:
                        NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
                        // Skip this data for now
                        NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, length, CP_OriginCur));
                        break;
                }
                break;
            case MKV_CLUSTER_PARSE_BODY:
                if (cluster->used < cluster->length)
                {
                    NvU64 length = 0;
                    NvU64 *ptr = NULL;
                    NVMM_CHK_ERR (ebmlReadId (matroska, &id, &res));
                    cluster->used += res;
                    bClusterUnits = NV_FALSE;
                    switch (id)
                    {
                        case MATROSKA_ID_CLUSTERTIMECODE:
                            ptr = &cluster->timecode;
                            NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
                            cluster->used += res;
                            NVMM_CHK_ERR (ebmlReadUint (matroska, (NvU32) length, ptr));
                            cluster->used += (NvU32)length;
                            if (PreviousClusterTime <= SeekTime && SeekTime < cluster->timecode)
                            {
                               if(NvMkvParseClusterData_KEY_FRAME_SCAN == ReadType)
                               {
                                   *ClusterTime = cluster->timecode;
                                   *ClusterPosition = cluster->begin;
                               }
                               else
                               {
                                   if (SeekTime > ((cluster->timecode + PreviousClusterTime)>>1))
                                   {
                                       *ClusterTime = cluster->timecode;
                                       *ClusterPosition = cluster->begin;
                                   }
                                   else
                                   {
                                       *ClusterTime = PreviousClusterTime;
                                       *ClusterPosition  = PreviousClusterPosition;
                                   }
                                }
                                bKeepParsing = NV_FALSE;
                                goto cleanup;
                            }
                            else
                            {
                                *ClusterTime = PreviousClusterTime =cluster->timecode;
                                *ClusterPosition  = PreviousClusterPosition = cluster->begin;
                                if(SeekHeadPresent)
                                {
                                    i++;
                                    if (i >= matroska->clusterList.QSz-1)
                                    {
                                        break;
                                    }
                                    node = &Q->node;
                                    Q = (NvMkvClusterNode *)Q->nxt;
                                    SeekPosition = matroska->segment_start  + node->position;
                                    NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, (CPint64)SeekPosition, CP_OriginBegin));
                                }
                                else
                                {
                                    NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, cluster->end , CP_OriginBegin));
                                }
                            }
                            cluster->state = MKV_CLUSTER_NEW_OPEN;
                            break;
                        case MATROSKA_ID_CLUSTERPOSITION:
                        case MATROSKA_ID_CLUSTERPREVSIZE:
                        case MATROSKA_ID_BLOCKGROUP:
                        case MATROSKA_ID_SIMPLEBLOCK:
                            NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, res, CP_OriginCur));
                            break;
                        default:
                            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                            "Bad ID found in add block from cluster - %llu of size %d?\n", id, res));
                            break;
                    }

                }
                else
                {
                    cluster->state = MKV_CLUSTER_NEW_OPEN;
                    break;
                }
        }
    }while (bKeepParsing || (i >= matroska->clusterList.QSz-1));
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));
    if(bKeepParsing)
    {
         Status = NvError_ParserFailure;
    }
    return Status;
}

// This function adds NUM_BLOCKS_FROM_CLUSTER at one go. Basically, call parse cluster again with correct state set
// Each time a parse cluster is called, a block gets added onto the matroska->blkList[index]
NvError mkvdAddBlockFromCluster (NvMkvDemux *matroska, NvMkvParseClusterData ReadType)
{
    /*
    0. Run this whole thing in a while(1) loop and break out
       a. If we find a block, we break... if we run out of blocks, then we set state to
    1. First check how many blocks were supposed to be here.. have we parsed out all the blocks ? If so, goto 4
    2. Read the next block ID, size, (other information), identify the block stream by reading into the data a bit
    3. Move from MatroskaBlock type into NvMkvBlock type of structure by calling add block function
    4. Set state to MKV_CLUSTER_NEW_OPEN at the beginning of file parsing - basically ID parsing only (This is to distinguish other IDs that come in between)
    5. Fix all return values
    */
    NvError Status = NvSuccess;
    NvMkvParseCluster *cluster;
    NvBool bKeepParsing = NV_TRUE;
    NvU64 length;
    NvU64 bytesleft = 0;
    NvBool bClusterUnits = NV_FALSE;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    NVMM_CHK_ARG (matroska);

    cluster = matroska->cluster;
    /* Check if we are at the end of file */
    NVMM_CHK_CP (matroska->pPipe->GetAvailableBytes (matroska->cphandle, &bytesleft));
    if (bytesleft == 0)
    {
        if(NvMkvParseClusterData_KEY_FRAME_SCAN == ReadType)
        {
            matroska->IdentifiedKeyThumbNailFrame = NV_TRUE;
            goto cleanup;
        }
        else
        {
            matroska->done = NV_TRUE;
        }
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
        "mkvdAddBlockFromCluster bytesleft == 0 EOS\n"));
    }

    if (matroska->done)
    {
        Status = NvError_ParserEndOfStream;
        goto cleanup;
    }
    do
    {
        NvS32 res = 0;
        NvU64 id = 0;
        bClusterUnits = NV_FALSE;
        switch (cluster->state)
        {
            case MKV_CLUSTER_NEW_OPEN:
                // We seem to need a new cluster parse... lets do that
                // Read the ID from the file. This must be the top-level ID
                if (!matroska->has_cluster_id)
                {
                    if ( (Status = ebmlReadId (matroska, &id, &res)) != NvSuccess)
                    {
                        matroska->done = 1;
                        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                        "MKV_CLUSTER_NEW_OPEN: Error ebmlReadId Status = %x\n", Status));
                        goto cleanup;
                    }
                }
                else
                {
                    id = MATROSKA_ID_CLUSTER;
                    matroska->has_cluster_id = 0;
                }
                switch (id)
                {
                    case MATROSKA_ID_CLUSTER:
                        cluster->timecode = (NvU64) AV_NOPTS_VALUE; // Default
                        NVMM_CHK_CP (matroska->pPipe->GetPosition64 (matroska->cphandle, &cluster->begin));
                        cluster->begin -= 4; //compensate for the 4 byte ID already read
                        // The following number in the stream is the length of the cluster
                        NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &cluster->length, &res));
                        cluster->used = 0;
                        cluster->state = MKV_CLUSTER_PARSE_BODY;
                        cluster->end = cluster->begin + cluster->length;
                        break;
                        // Usually, the following are all parsed in the beginning of the file in ReadHeader
                        // For all the following, read the size field and skip the data completely (if it has already been parsed)
                    case MATROSKA_ID_CUES:
                    case MATROSKA_ID_SEEKHEAD:
                    case MATROSKA_ID_TAGS:
                    case MATROSKA_ID_INFO:
                        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_INFO,
                        "MKV_CLUSTER_NEW_OPEN: SKIP MATROSKA_ID = %x\n", id));
                        NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
                        // Skip this data for now
                        NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, length, CP_OriginCur));
                        break;
                }
                break;
            case MKV_CLUSTER_PARSE_BODY:
                if (cluster->used < cluster->length)
                {
                    NvU64 length = 0;
                    NvU64 *ptr = NULL;
                    NVMM_CHK_ERR (ebmlReadId (matroska, &id, &res));
                    cluster->used += res;
                    bClusterUnits = NV_FALSE;
                    switch (id)
                    {
                        case MATROSKA_ID_CLUSTERTIMECODE:
                            ptr = &cluster->timecode;
                            bClusterUnits = NV_TRUE;
                            break;
                        case MATROSKA_ID_CLUSTERPOSITION:
                            ptr = &cluster->position;
                            bClusterUnits = NV_TRUE;
                            break;
                        case MATROSKA_ID_CLUSTERPREVSIZE:
                            ptr = &cluster->prevsize;
                            bClusterUnits = NV_TRUE;
                            break;
                        case MATROSKA_ID_BLOCKGROUP:
                            NVMM_CHK_ERR (mkvdExtractAndQBlock (matroska, cluster, ReadType));
                            cluster->used += res;
                            if(NvMkvParseClusterData_KEY_FRAME_SCAN == ReadType)
                            {
                                if(matroska->IdentifiedKeyThumbNailFrame)
                                {
                                    bKeepParsing = NV_FALSE;
                                }
                            }
                            else
                            {
                                bKeepParsing = NV_FALSE;
                            }
                            break;
                        case MATROSKA_ID_SIMPLEBLOCK:
                            NVMM_CHK_ERR (mkvdExtractAndQSimpleBlock (matroska, cluster, ReadType));
                            if(NvMkvParseClusterData_KEY_FRAME_SCAN == ReadType)
                            {
                                if(matroska->IdentifiedKeyThumbNailFrame)
                                {
                                    bKeepParsing = NV_FALSE;
                                }
                            }
                            else
                            {
                                bKeepParsing = NV_FALSE;
                            }
                            break;
                        default:
                            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                            "Bad ID found in add block from cluster - %llu of size %d?\n", id, res));
                            break;
                    }

                    if(bClusterUnits)
                    {
                            NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &length, &res));
                            cluster->used += res;
                            NVMM_CHK_ERR (ebmlReadUint (matroska, (NvU32) length, ptr));
                            cluster->used += (NvU32)length;
                    }
                }
                else
                {
                    cluster->state = MKV_CLUSTER_NEW_OPEN;
                    break;
                }
        }
    }
    while (bKeepParsing && (MKV_CLUSTER_NEW_OPEN != cluster->state));
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));

    return Status;
}

static NvError mkvdParseBlock (NvMkvDemux *matroska, NvMkvBlock *block)
{
    // cluster_timecode is available in block itself - use from there
    NvU64 timecode = AV_NOPTS_VALUE;
    NvMatroskaTrack *track;
    NvS32 res = 0;
    NvMkvAVPacket *pkt = NULL;
    NvS16 block_time;
    NvU32 *lace_size = NULL;
    NvS32 n, flags, laces = 0;
    NvU64 num;
    NvU64 tracknum;
    NvU8 scratch[16];
    NvS32 is_keyframe = 0;
    NvU64 position = 0;
    NvError Status = NvSuccess;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    NVMM_CHK_ARG (matroska);
    // Save current position, then reposition into the file to point to the block to be parsed
    NVMM_CHK_CP (matroska->pPipe->GetPosition64 (matroska->cphandle, &matroska->save_pos));

    // Set the position to the block data
    // This position must reflect the location after the ID and size fields
    NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, block->bin_pos, CP_OriginBegin));
    position = block->bin_pos;
    NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &tracknum, &res));
    position += res;
    block->bin_size -= res;
    track = mkvdFindTrackByNum (matroska, (NvS32) tracknum);
    if ( (block->bin_size < 3) || !track)
    {
        // TBD - Needs revisit when implementing error handling
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, "Invalid stream %d\n", tracknum));
        Status = NvSuccess; //NvError_ParserFailure;
        goto cleanup;
    }
    // TODO How to discard a block from a particualr track --- need to be fixed

    if (block->duration == (NvU64) AV_NOPTS_VALUE)
        block->duration = (NvU64) ( (NvF32) track->default_duration / (NvF32) matroska->time_scale);

    NVMM_CHK_CP (matroska->pPipe->cpipe.Read (matroska->cphandle, (CPbyte*) scratch, 3));
    // TBD jump to a label, where we set filepos to the saved value
    block_time = scratch[0];
    block_time <<= 8;
    block_time |= scratch[1];
    // int8 Flags
    flags = scratch[2];
    position += 3;
    block->bin_size -= 3;
    // If block reference was not sent, then it is a key frame (Matroska v1)
    // flags may also be used in Matroska v2

    if (track->type == NvMkvTrackTypes_VIDEO)
    {
        // block->reference = 0: indicates key frame.
        is_keyframe = (!block->reference) || (flags & 0x80 ? 1 : 0);
        if (is_keyframe)
        {
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_INFO,
            "Keyframe Found = %x - flags = %x - block_reference = %x\n",
            is_keyframe, flags, block->reference));
        }
    }

    if ( (block->cluster_timecode != (NvU64) - 1) && ( (block_time >= 0) || ( (NvS64) block->cluster_timecode >= -block_time)))
    {
        timecode = block->cluster_timecode + block_time;
        track->end_timecode = (track->end_timecode > timecode + block->duration) ? track->end_timecode : timecode + block->duration;
    }
    if (matroska->seek_keyframe && (track->type != NvMkvTrackTypes_SUBTITLE))
    {
        if (!is_keyframe || (timecode < matroska->seek_timecode))
        {
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_INFO,
            "Returning since correct key frame is not located\n"));
            NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, matroska->save_pos, CP_OriginBegin));
            // TBD: Should this be NvSuccess;
            goto cleanup;
        }
        matroska->seek_keyframe = NV_FALSE;
        matroska->seek_timecode = 0;
        matroska->seek_tracknum = 0;
        matroska->seek_blocknum = -1;
    }
    res = 0;
    switch ( (flags & 0x06) >> 1)
    {
        case 0x0: /* no lacing */
            laces = 1;
            lace_size = NvOsAlloc (sizeof (int));
            NVMM_CHK_MEM (lace_size);
            // Add check here
            lace_size[0] = block->bin_size;
            break;
        case 0x1: /* Xiph lacing */
        case 0x2: /* fixed-size lacing */
        case 0x3: /* EBML lacing */
            NVMM_CHK_CP (matroska->pPipe->cpipe.Read (matroska->cphandle, (CPbyte*) scratch, 1));
            position += 1;
            block->bin_size -= 1;
            laces = scratch[0] + 1;
            lace_size = NvOsAlloc (laces * sizeof (int));
            NVMM_CHK_MEM (lace_size);
            NvOsMemset (lace_size, 0, laces * sizeof (int));
            switch ( (flags & 0x06) >> 1)
            {
                case 0x1: /* Xiph lacing */
                {
                    NvU32 total = 0;
                    NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "In Xiph encode\n"));

                    for (n = 0; res == 0 && n < laces - 1; n++)
                    {
                        while (1)
                        {
                            if (block->bin_size == 0)
                            {
                                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, 
                                "Xiph encode , res = -1\n"));
                                Status = NvError_ParserFailure;
                                break;
                            }
                            if (matroska->pPipe->cpipe.Read (matroska->cphandle, (CPbyte*) scratch, 1) != NvSuccess)
                            {
                                // TBD - Needs revisit when implementing error handling
                                Status = NvError_ParserFailure;
                                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                                "Xiph lacing - Parser ReadFailure\n"));
                                break;
                            }
                            position++;
                            block->bin_size -= 1;
                            lace_size[n] += scratch[0];

                            if (scratch[0] != 0xff)
                                break;
                        }
                        total += lace_size[n];
                    }
                    lace_size[n] = block->bin_size - total;
                    break;
                }
                case 0x2: /* fixed-size lacing */
                    for (n = 0; n < laces; n++)
                    {
                        lace_size[n] = block->bin_size / laces;
                    }
                    break;
                case 0x3: /* EBML lacing */
                {
                    NvU32 total;
                    if ((Status = ebmlReadNum (matroska, 8, &num, &res)) != NvSuccess)
                    {
                        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                        "EBML lacing err- ebmlReadNum Status = %x\n", Status));
                        break;
                    }
                    position += res;
                    block->bin_size -= res;
                    total = lace_size[0] = (NvU32) num;
                    for (n = 1; res >= 0 && n < laces - 1; n++)
                    {
                        NvS64 snum = 0;
                        Status= ebmlReadSint (matroska, 4, &snum, &res);
                        position += res;
                        block->bin_size -= res;
                        if ((res < 0) || (Status != NvSuccess))
                        {
                            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                            "EBML lacing err - ebmlReadSint Status = %x\n", Status));
                            break;
                        }
                        lace_size[n] = lace_size[n - 1] + (NvS32) snum;
                        total += lace_size[n];
                    }
                    lace_size[n] = block->bin_size - total;
                    break;
                }
            }
            break;
    }
    if ((res < 0) || (Status != NvSuccess))
    {
        if (lace_size)
        {
            NvOsFree (lace_size);
            lace_size = NULL;
        }
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
        "ERORR Parse Lacing Status = %x\n", Status));
        goto cleanup;
    }
    res = 0;
    NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "Lace size -  "));
    for (n = 0; n < laces; n++)
    {
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_DEBUG, "%d  ", lace_size[n]));
    }
    // Add the laced data into the AVPackets Q
    if (res == 0)
    {
        for (n = 0; n < laces; n++)
        {
            NvS32 pkt_size = lace_size[n];

            pkt = NvOsAlloc (sizeof (NvMkvAVPacket));           // Add check here
            NVMM_CHK_MEM(pkt);

            if ( (Status = mkvdAVNewPacket (pkt, position, pkt_size)) != NvSuccess)
            {
                n = laces - 1;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                "EBML mkvdAVNewPacket Err Status = %x\n", Status));
                break;
            }
            if (n == 0)
                pkt->flags = is_keyframe;
            pkt->stream_index = mkvdFindIndexByNum (matroska, (NvS32) tracknum);
            pkt->pts = timecode;
            if (track->type != NvMkvTrackTypes_SUBTITLE)
                pkt->duration = block->duration;
            if ( (Status = mkvdAVqAddPacket (&matroska->avPktList[pkt->stream_index], pkt)) != NvSuccess)
            {
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                "EBML mkvdAVqAddPacket Err Status = %x\n", Status));
                break;
            }
            if ( (timecode != (NvU64) AV_NOPTS_VALUE) &&
                    (matroska->prev_pkt.pts == timecode) &&
                    (matroska->prev_pkt.stream_index == pkt->stream_index))
            {
                matroska->prev_pkt.complete = NV_FALSE;
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, 
                "Split packet payloads NOT handled [%llu %d] [%llu %d] %llu \n",
                                   matroska->prev_pkt.pts, matroska->prev_pkt.stream_index, timecode, pkt->stream_index));
                Status = NvError_ParserFailure;
                break;
            }
            NvOsMemcpy (&matroska->prev_pkt, pkt, sizeof (NvMkvAVPacket));
            matroska->prev_pkt.complete = NV_TRUE;
            if (timecode != (NvU64) AV_NOPTS_VALUE)
                timecode = block->duration ? timecode + block->duration : (NvU64) AV_NOPTS_VALUE;
            position += pkt_size;

        }
    }
    // parse_done:
    // Restore the position back
    NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, matroska->save_pos, CP_OriginBegin));

cleanup:

    if ((res < 0) || (Status != NvSuccess))
    {
        if (pkt)
        {
            NvOsFree (pkt);
            pkt = NULL;
        }
    }
    if (lace_size)
    {
        NvOsFree (lace_size);
        lace_size = NULL;
    }
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));
    return Status;
}

static NvError mkvdAddPktFromBlock (NvMkvDemux *matroska, NvS32 index)
{
    NvError Status = NvSuccess;
    NvMkvBlock *block;
    NvMkvBlkNode *temp;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    NVMM_CHK_ARG (matroska);

    // Check if there are any blocks queued on the blockQ
    // If there are blocks, then parse the blocks from that Q
    if (matroska->blkList[index].QSz)
    {
        block = matroska->blkList[index].Q->node;
        // Free the used node and house-keeping on list
        temp = matroska->blkList[index].Q->nxt;
        NVMM_CHK_ERR (mkvdParseBlock (matroska, block));
        matroska->blkList[index].QSz--;
        NvOsFree (matroska->blkList[index].Q->node); // This is the block itself
        NvOsFree (matroska->blkList[index].Q);
        matroska->blkList[index].Q = temp;
    }
    else
    {
        // We have no block on this index in the blockList. So, add blocks from the open cluster
        // TBD --- Bascially, everytime we call add blocks, it adds an arbitrary 3 blocks 
        // (we want to keep file access spread minimum - this may be changed however)
        NVMM_CHK_ERR (mkvdAddBlockFromCluster(matroska, NvMkvParseClusterData_ADD_BLOCKQ));
        if (matroska->blkList[index].QSz)
        {
            block = matroska->blkList[index].Q->node;
            // Free the used node and house-keeping on list
            temp = matroska->blkList[index].Q->nxt;
            NVMM_CHK_ERR (mkvdParseBlock (matroska, block));
            matroska->blkList[index].QSz--;
            NvOsFree (matroska->blkList[index].Q->node); // This is the block itself
            NvOsFree (matroska->blkList[index].Q);
            matroska->blkList[index].Q = temp;
        }
    }
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));
    return Status;
}


// Assumes that memory has been allocated in pkt and points to valid memory location => an av packet
NvError mkvdGetPacket (NvMkvDemux *matroska, NvS32 index, NvMkvAVPacket *pkt)
{
    NvError Status = NvSuccess;
    NvMkvAVPNode *temp;
    NvBool bKeepDigging = NV_TRUE;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    NVMM_CHK_ARG (matroska && pkt);

    // This function must check if packets are available on the AVQ first
    // If packets are not present, it must call mkvdParseBlock()
    while (NV_TRUE == bKeepDigging)
    {
        if (matroska->avPktList[index].QSz)
        {

            NvOsMemcpy (pkt, matroska->avPktList[index].Q->node, sizeof (NvMkvAVPacket));
            // Free the used node and house-keeping on list
            matroska->avPktList[index].QSz--;
            temp = matroska->avPktList[index].Q->nxt;
            NvOsFree (matroska->avPktList[index].Q->node); // This is the NvMkvAVPacket that needs to be freed
            NvOsFree (matroska->avPktList[index].Q);
            matroska->avPktList[index].Q = temp;
            bKeepDigging = NV_FALSE;
        }
        else if (!matroska->done)
        {
            // Lets get to hauling some serious stuff now!
            if  ( ((Status = mkvdAddPktFromBlock (matroska, index)) != NvSuccess) && (!matroska->done))
            {
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                "Error in mkvdAddPktFromBlock - bitstream corruption?\n"));
                // There may be something on the AVQ or the pktQ... needs flushing
                goto cleanup;
            }
            else if (matroska->done)
            {// We may need to check if all the queues are empty before returning EOF for playback till last byte
                NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
                "mkvdAddPktFromBlock - EOS:Matroska done\n"));
                Status = NvError_ParserEndOfStream; //ParserEndOfStream
                goto cleanup;
            }
        }
        else
        {
            Status = NvError_ParserFailure;
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR,
            "mkvdGetPacket ParserFailure\n"));
            goto cleanup;
        }
    }
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));
    return Status;
}

static NvError mkvdParseAndSeekBlock (NvMkvDemux *matroska, NvMkvBlock *block, NvBool *pSeekDone)
{

    NvError Status = NvSuccess;
    // cluster_timecode is available in block itself - use from there
    NvU64 timecode = AV_NOPTS_VALUE;
    NvMatroskaTrack *track;
    NvS32 res = 0;
    NvS16 block_time;
    NvS32 flags = 0;
    NvU64 tracknum;
    NvU8 scratch[16];
    NvS32 is_keyframe = 0;
    NvU64 position = 0;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    NVMM_CHK_ARG (matroska);
    // Save current position, then reposition into the file to point to the block to be parsed
    NVMM_CHK_CP (matroska->pPipe->GetPosition64 (matroska->cphandle, &matroska->save_pos));
    // Set the position to the block data
    // This position must reflect the location after the ID and size fields
    NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, block->bin_pos, CP_OriginBegin));
    position = block->bin_pos;
    NVMM_CHK_ERR (ebmlReadNum (matroska, 8, &tracknum, &res));
    position += res;
    track = mkvdFindTrackByNum (matroska, (NvS32) tracknum);
    if ( (block->bin_size < 3) || !track)
    {
        // TBD - Needs revisit when implementing error handling
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_ERROR, "Invalid stream %d\n", tracknum));
        Status = NvError_ParserCorruptedStream;
        goto cleanup;
    }
    if (block->duration == (NvU64) AV_NOPTS_VALUE)
        block->duration = (NvU64) ( (NvF32) track->default_duration / (NvF32) matroska->time_scale);

    NVMM_CHK_CP (matroska->pPipe->cpipe.Read (matroska->cphandle, (CPbyte*) scratch, 3));
    // TBD - Needs revisit when implementing error handling
    block_time = scratch[0];
    block_time <<= 8;
    block_time |= scratch[1];
    // int8 Flags
    flags = scratch[2];
    position += 3;
    // If block reference was not sent, then it is a key frame (Matroska v1)
    // flags may also be used in Matroska v2
    if (track->type == NvMkvTrackTypes_VIDEO)
    {
        is_keyframe = (!block->reference) || (flags & 0x80 ? 1 : 0);
        if (is_keyframe)
        {
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_INFO,
            "MkvdParseAndSeekBlock: is_keyframe found = 0x%x - flags = %x\n", is_keyframe,flags));
        }
    }
    if (track->type == NvMkvTrackTypes_AUDIO)   // For audio, set keyframe as 1 always
    {
        is_keyframe = 1;
    }
    if ( (block->cluster_timecode != (NvU64) - 1) && ( (block_time >= 0) || ( (NvS64) block->cluster_timecode >= -block_time)))
    {
        timecode = block->cluster_timecode + block_time;
        track->end_timecode = (track->end_timecode > timecode + block->duration) ? track->end_timecode : timecode + block->duration;
    }

    if (matroska->seek_keyframe && (track->type != NvMkvTrackTypes_SUBTITLE))
    {
        if (!is_keyframe || (timecode < matroska->seek_timecode))
        {
            NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, matroska->save_pos, CP_OriginBegin));
            Status = NvSuccess;
            NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_INFO,
            "MkvdParseAndSeekBlock:Key_frame_not_found: matroska->save_pos = %l\n",matroska->save_pos));
            goto cleanup;
        }
        matroska->seek_keyframe = NV_FALSE;
        matroska->seek_timecode = 0;
        matroska->seek_tracknum = 0;
        matroska->seek_blocknum = -1;
        matroska->seek_timecode_found = timecode;
        NV_LOGGER_PRINT ( (NVLOG_MKV_PARSER, NVLOG_INFO,
        "MkvdParseAndSeekBlock:KeyFound: seek_timecode = %llu - save_position = %ll\n",
        timecode, matroska->save_pos));
        *pSeekDone = NV_TRUE;
    }
    NVMM_CHK_CP (matroska->pPipe->SetPosition64 (matroska->cphandle, matroska->save_pos, CP_OriginBegin));
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));
    return Status;
}

/* This function opens the cluster at which setposition function has set the file pointer at *
 * Parses the cluster with the appropriate track index to arrive at the correct seeked block *
 */
NvError mkvdSeekToDesiredBlock (NvMkvDemux *matroska, NvS32 index)
{
    NvError Status = NvSuccess;
    NvMkvBlock *block;
    NvBool seek_complete = NV_FALSE;
    NvU32 i;
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));
    NVMM_CHK_ARG (index != -1);

    /* Parse the cluster and add block from it onto the block queues irrespective of the track for now */
    while (!seek_complete)
    {
        NVMM_CHK_ERR (mkvdAddBlockFromCluster (matroska, NvMkvParseClusterData_ADD_BLOCKQ));
        if (matroska->blkList[index].QSz)
        {
            NvMkvBlkNode *temp;
            block = matroska->blkList[index].Q->node;
            // Free the used node and house-keeping on list
            temp = matroska->blkList[index].Q->nxt;
            NVMM_CHK_ERR (mkvdParseAndSeekBlock (matroska, block, &seek_complete));
            if (!seek_complete)
            {
                matroska->blkList[index].QSz--;
                NvOsFree (matroska->blkList[index].Q->node); // This is the block itself
                NvOsFree (matroska->blkList[index].Q);
                matroska->blkList[index].Q = temp;
            }
        }
    }
    // Free any blocks that might have been queued from other tracks
    for (i = 0; i < matroska->trackList.ulNumTracks; i++)
    {
        if (i == (NvU32) index)
            continue;
        while (matroska->blkList[i].QSz)
        {
            NvMkvBlkNode *temp;
            temp = matroska->blkList[i].Q->nxt;
            matroska->blkList[i].QSz--;
            NvOsFree (matroska->blkList[i].Q->node); // This is the block itself
            NvOsFree (matroska->blkList[i].Q);
            matroska->blkList[i].Q = temp;
        }
    }
cleanup:
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));
    return Status;
}


NvMkvParserStatus mkvdEnableDisableTrackNum (NvMkvDemux *matroska, NvS32 index, NvBool enadis)
{
    NV_LOGGER_PRINT((NVLOG_MKV_PARSER, NVLOG_DEBUG, "++%s[%d]", __FUNCTION__, __LINE__));

    if ( (index < 0) || (index > (NvS32) matroska->trackList.ulNumTracks))
        return NvMkvParserStatus_INVALID_DATA;

    matroska->trackList.track[index].user_enable = enadis;

    // If it is a request to disabe a track, clear all the queues associated with this track
    if (enadis == NV_FALSE)
    {
        NvMkvAVPNode *temp0;
        NvMkvBlkNode *temp1;

        while (matroska->avPktList[index].QSz)
        {
            // Free the used node and house-keeping on list
            matroska->avPktList[index].QSz--;
            NvOsFree (matroska->avPktList[index].Q->node); // Free the packet
            temp0 = matroska->avPktList[index].Q->nxt;
            NvOsFree (matroska->avPktList[index].Q);
            matroska->avPktList[index].Q = temp0;
        }

        while (matroska->blkList[index].QSz)
        {
            // Free the used node and house-keeping on list
            matroska->blkList[index].QSz--;
            NvOsFree (matroska->blkList[index].Q->node); // Free the packet
            temp1 = matroska->blkList[index].Q->nxt;
            NvOsFree (matroska->blkList[index].Q);
            matroska->blkList[index].Q = temp1;
        }

    }
    NV_LOGGER_PRINT ((NVLOG_MKV_PARSER, NVLOG_DEBUG, "--%s[%d]", __FUNCTION__, __LINE__));
    return NvMkvParserStatus_NOERROR;
}

/* Function to determine if Video exists in the stream. */
NvBool mkvdDoesVideoExist (NvMkvDemux *matroska)
{
    return matroska->stream_has_video;
}


