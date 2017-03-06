/*
 * Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto. Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */


#ifndef INCLUDED_NVMM_MKVDEMUX_H
#define INCLUDED_NVMM_MKVDEMUX_H

#include "nvcommon.h"
#include "nvmm.h"
#include "nvmm_parser.h"
#include "nvmm_mkvparser_defines.h"
#include "nvlocalfilecontentpipe.h"

typedef enum NvMkvParseClusterDataRec
{
    NvMkvParseClusterData_KEY_FRAME_SCAN,
    NvMkvParseClusterData_SEEK_NON_CUE,
    NvMkvParseClusterData_ADD_BLOCKQ
} NvMkvParseClusterData;

enum
{
    MAX_SPLIT_CUE_TABLES  = 16,
    MAX_CUE_ENTRIES_PER_SPLIT_TABLE = 512,
};

// COMMON DEFINES
#define AV_NOPTS_VALUE          (/*(NvS64)*/0x8000000000000000ULL)
enum
{
    AVERROR_OK = 0,
    AVERROR_INVALIDDATA = -1,
    AVERROR_EOF = -2,
    AVERROR_MEMORY_ERROR = -3,
    AVERROR_GETPOS_FAIL = -4,
    AVERROR_SETPOS_FAIL = -5,
}
;
// MKV PARSER SRUCTURE PROTOTYPES
enum
{
    MKV_CLUSTER_NEW_OPEN = 0,
    MKV_CLUSTER_PARSE_BODY = 1,
} ;

enum
{
    MATROSKA_TRACK_TYPE_NONE = 0x0,
    MATROSKA_TRACK_TYPE_VIDEO = 0x1,
    MATROSKA_TRACK_TYPE_AUDIO = 0x2,
    MATROSKA_TRACK_TYPE_COMPLEX = 0x3,
    MATROSKA_TRACK_TYPE_LOGO = 0x10,
    MATROSKA_TRACK_TYPE_SUBTITLE = 0x11,
    MATROSKA_TRACK_TYPE_CONTROL = 0x20,
};
typedef struct NvMkvEbmlRec
{
    NvU64 version;
    NvU64 max_size;
    NvU64 id_length;
    char *doctype;
    NvU64 doctype_version;
    NvU64 readversion;
    NvU64 doctype_readversion;
} NvMkvEbml;

typedef struct NvMKvAVPacketRec
{
    NvU64 pts;        //Presentation timestamp in time_base units. Can be AV_NOPTS_VALUE if it is not stored in the file
    int   size;
    int   stream_index;
    int   flags;
    NvU64   duration;  //Duration of this packet in time_base units, 0 if unknown.Equals next_pts - this_pts in presentation order.
    void  *priv;
    NvS64 pos;                            ///< byte position in stream, -1 if unknown
    NvBool complete;
} NvMkvAVPacket;

typedef struct NvMkvAudioTrackRec
{
    NvF64 samplerate;
    NvF64 out_samplerate;
    NvU64 bitdepth;
    NvU64 channels;
} NvMkvAudioTrack;

typedef struct NvMkvVideoTrackRec
{
    double   frame_rate;
    NvU64 display_width;
    NvU64 display_height;
    NvU64 pixel_width;
    NvU64 pixel_height;
    NvBool interlaced;
    NvU64 fourcc;
} NvMkvVideoTrack;

#define MATROSKA_STRING_LENGTH 128

typedef struct NvMatroskaTrackRec
{
    NvBool user_enable;   // user specified enable/disable
    NvF32 timescale;
    NvS32 codec_type;
    NvS32 codec_prv_size;
    NvU64 end_timecode;
    NvU64 num;
    NvU64 type;
    NvU64 default_duration;
    NvU64 flag_default;   // media specified enable/disable
    NvS8 codec_id[MATROSKA_STRING_LENGTH];  // fixed at 128. While reading, ignore if greater..
    NvU8* codec_prv_data;
    NvS8 language[MATROSKA_STRING_LENGTH];

    NvMkvAudioTrack audio;
    NvMkvVideoTrack video;

    void *encodings;
} NvMatroskaTrack;

typedef struct NvMkvTrackListRec
{
    NvU32 ulNumTracks;
    NvMatroskaTrack *track;
}NvMkvTrackList;

typedef struct NvMkvPktsQueueInfoRec
{
    NvU32 Qsz;
    NvMkvAVPacket *Q[256];
} NvMkvPktsQueueInfo;

typedef struct NvMkvCodecRec
{
    NvMkvCodecType codec_id;
    NvU32          codec_tag;
    NvU32          width;
    NvU32          height;
    NvU32          sample_rate;
    NvU32          channels;
} NvMkvCodec;

typedef struct NvMkvBlockRec
{
    NvU64  duration;
    NvU64  reference;
    NvU64  bin_pos;  // position of the binary data corresponding to this block in the file
    NvU32  bin_size; // size of the corresponding binary data
    NvU32  blk_size;
    NvU64 cluster_timecode;
} NvMkvBlock;

typedef struct NvMkvAVPNodeRec
{
    NvMkvAVPacket *node;
    struct NvMkvAVPNodeRec *nxt;
} NvMkvAVPNode;

typedef struct NvMkvBlkNodeRec
{
    NvMkvBlock *node;
    struct NvMkvBlkNodeRec *nxt;
} NvMkvBlkNode;

typedef struct NvMkvAVPktsInfoRec
{
    NvU32 QSz;
    NvMkvAVPNode *Q;
} NvMkvAVPktsInfo;

typedef struct NvMkvBlkInfoRec
{
    NvU32 QSz;
    NvMkvBlkNode *Q;
} NvMkvBlocksInfo;

///
typedef struct NvMkvClusterRec
{
    NvU64 position;
    NvU64 timecode;
}NvMkvCluster;

typedef struct NvMkvClusterNodeRec
{
    NvMkvCluster node;
    struct NvMkvClusterNode *nxt;
}NvMkvClusterNode;

typedef struct NvMkvClusterListRec
{
   NvU32 QSz;
   NvMkvClusterNode *Q;
}NvMkvClusterList;

///
typedef struct NvMkvParseClusterRec
{
    NvBool cluster_open;
    NvBool all_blocks_done;

    NvU64  begin;
    NvU64  end;
    NvU32  state;
    NvU64  length;
    NvU32  used;

    NvBool found_block;
    NvU64 position;
    NvU64 timecode;
    NvU64 prevsize;
} NvMkvParseCluster;

typedef struct NvMkvParseCueRec
{
    NvU64 begin;
    NvU64 used;
    NvU64 length;
} NvMkvParseCue;

typedef struct NvMkvCuePointRec
{
    NvU64 cue_track;
    NvU64 cue_block_position;
    NvU64 cue_time;
    NvU64 cue_cluster_position;
} NvMkvCuePoint;

typedef struct NvMkvSplitCueInfoRec
{
    NvMkvCuePoint *pCpoints;
    NvU64 StartTime;
    NvU64 EndTime;
    NvU32 num_elements;
} NvMkvSplitCueInfo;

typedef struct NvMkvCueInfoRec
{
    NvBool fParseDone;  // If parsing of this is already over
    NvU64 begin;
    NvU64 used;
    NvU64 length;
    NvU32 ulNumEntries;
    NvU32 ulMaxEntries;
    NvBool fOverflow;
    NvU64 ulLastTime;
    //
    NvU32 NumSplitTables;
    NvMkvSplitCueInfo  SplitCueInfo[MAX_SPLIT_CUE_TABLES];
    //
} NvMkvCueInfo;

/* This is the mother structure for the complete parser */
typedef struct NvMkvDemuxRec
{
    CPhandle cphandle;
    CP_PIPETYPE_EXTENDED *pPipe;
    NvBool bPipeIsStreaming;    // We may use this to avoid long jummps in file
    /* EBML stuff */
    NvU8 *title;                // title found during EBML header parse
    NvU64 time_scale;          // Matroska file-level scale value
    double duration;            // Set to default 1000,000
    /* byte position of the segment inside the stream */
    NvU64 segment_start;
    NvU64 cue_start;
    NvU64 NextSeekHeadStart;
    NvU64 ClusterStart;
    NvU32 segment_len;
    /* Pointer to previous packet to identify cases where data gets split across blocks */
    NvMkvAVPacket prev_pkt;
    NvBool done;
    NvBool has_cluster_id;
    /* Information to Seek to when parsing blocks */
    NvBool seek_keyframe;
    NvU64 seek_timecode;
    NvS32 seek_tracknum;
    NvU64 seek_blocknum;
    NvU64 seek_timecode_found;
    NvMkvCodec codec;
    // These are codec specific and need to be cleaned up! A union perhaps ?
    NvMkvAacConfigurationData aacConfigurationData;
    NvMkvMp3ConfigurationData mp3ConfigurationData;
    NvMkvAc3ConfigurationData ac3ConfigurationData;
    NvAVCConfigData avcConfigData;
    NvMkvAVPktsInfo avPktList[NVMM_MKVPARSER_MAX_TRACKS];
    NvMkvBlocksInfo blkList[NVMM_MKVPARSER_MAX_TRACKS];
    NvMkvClusterList clusterList;

    NvMkvParseCluster *cluster;
    NvMkvCueInfo *cue;
    NvMkvTrackList trackList;
    NvMkvPktsQueueInfo audQInfo;
    NvMkvPktsQueueInfo vidQInfo;
    NvU32 nalSize;
    NvU64 save_pos;
    // We may parse these at segment parsing or these may reappear for parsing through seekhead.
    // To avoid needless multiple parsing, set these flags upon succesful parse
    NvBool cue_parse_done;
    NvBool info_parse_done;
    NvBool track_parse_done;
    NvBool stream_has_audio;   /* Audio present in stream */
    NvBool stream_has_video;   /* Video present in stream */
    NvU32 VideoStreamId;
    NvU32 AudioStreamId;
    NvBool stream_has_tits;    /* Sub titles present ;) */
    NvU64 last_pts;
    NvU32 largestKeyFrameSize;
    NvU32 ScanEntry;
    NvU64 largestKeyTimeStamp; // For thumbnail generation
    NvBool IdentifiedKeyThumbNailFrame;
} NvMkvDemux;


/*****************************************************************************
 *                                  FUNCTION PROTOTYPES                                                          *
 *****************************************************************************/
NvBool mkvdDoesVideoExist (
    NvMkvDemux *matroska);

NvS32  mkvdFindIndexByNum (
    NvMkvDemux *matroska,
    NvS32 tracknum);

NvError mkvdSeekToDesiredBlock (
    NvMkvDemux *matroska, 
    NvS32 index);

NvError mkvdReadHeader (
    NvMkvDemux * demux);

NvError mkvdSetPosReinit (
    NvMkvDemux * matroska);

NvError mkvdGetPacket (
    NvMkvDemux * matroska,
    NvS32 index, 
    NvMkvAVPacket * pkt);

NvError mkvdFetchData (
    NvMkvDemux * matroska,
    NvMkvAVPacket * pkt,
    NvU8 * data, 
    NvU32 * szAvail);

NvMkvParserStatus mkvdEnableDisableTrackNum (
    NvMkvDemux *matroska, 
    NvS32 index,
    NvBool enadis);

NvMkvTrackTypes   mkvdStreamType (
    NvMkvDemux *matroska,
    NvS32 index);

NvMkvCodecType mkvdCodecType (
    NvMkvDemux *matroska, 
    NvS32 index);

NvMatroskaTrack*   mkvdFindTrackByNum (
    NvMkvDemux *matroska,
    NvS32 num);

NvError mkvdScanClusterTimeCode(
    NvMkvDemux *matroska,
    NvU64 SeekTime,
    NvU64 *ClusterTime,
    NvU64 *ClusterPosition,
    NvBool SeekHeadPresent,
    NvMkvParseClusterData ReadType);

NvError mkvdAddBlockFromCluster (
    NvMkvDemux *matroska,
    NvMkvParseClusterData ReadType);

#endif /* INCLUDED_NVMM_MKVDEMUX_H*/


