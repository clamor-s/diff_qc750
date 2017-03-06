/*
 * Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an
 * express license agreement from NVIDIA Corporation is strictly prohibited.
 */
#ifndef INCLUDED_NVMM_MKVPARSER_DEFINES_H
#define INCLUDED_NVMM_MKVPARSER_DEFINES_H

#include "nvos.h"
#include "nvmm_common.h"

#define NVLOG_CLIENT NVLOG_MKV_PARSER // Required for logging information

/************************ Standard matroska defines ****************************/
/* EBML version supported */
#define EBML_VERSION 1
/* top-level master-IDs */
#define EBML_ID_HEADER             0x1A45DFA3
/* IDs in the HEADER master */
#define EBML_ID_EBMLVERSION        0x4286
#define EBML_ID_EBMLREADVERSION    0x42F7
#define EBML_ID_EBMLMAXIDLENGTH    0x42F2
#define EBML_ID_EBMLMAXSIZELENGTH  0x42F3
#define EBML_ID_DOCTYPE            0x4282
#define EBML_ID_DOCTYPEVERSION     0x4287
#define EBML_ID_DOCTYPEREADVERSION 0x4285
/* general EBML types */
#define EBML_ID_VOID               0xEC
#define EBML_ID_CRC32              0xBF
/*
 * Matroska element IDs, max. 32 bits
 */
/* toplevel segment */
#define MATROSKA_ID_SEGMENT    0x18538067
/* Matroska top-level master IDs */
#define MATROSKA_ID_INFO       0x1549A966
#define MATROSKA_ID_TRACKS     0x1654AE6B
#define MATROSKA_ID_CUES       0x1C53BB6B
#define MATROSKA_ID_TAGS       0x1254C367
#define MATROSKA_ID_SEEKHEAD   0x114D9B74
#define MATROSKA_ID_ATTACHMENTS 0x1941A469
#define MATROSKA_ID_CLUSTER    0x1F43B675
#define MATROSKA_ID_CHAPTERS   0x1043A770
/* IDs in the info master */
#define MATROSKA_ID_TIMECODESCALE 0x2AD7B1
#define MATROSKA_ID_DURATION   0x4489
#define MATROSKA_ID_TITLE      0x7BA9
#define MATROSKA_ID_WRITINGAPP 0x5741
#define MATROSKA_ID_MUXINGAPP  0x4D80
#define MATROSKA_ID_DATEUTC    0x4461
#define MATROSKA_ID_SEGMENTUID 0x73A4
/* ID in the tracks master */
#define MATROSKA_ID_TRACKENTRY 0xAE
/* IDs in the trackentry master */
#define MATROSKA_ID_TRACKNUMBER 0xD7
#define MATROSKA_ID_TRACKUID   0x73C5
#define MATROSKA_ID_TRACKTYPE  0x83
#define MATROSKA_ID_TRACKAUDIO 0xE1
#define MATROSKA_ID_TRACKVIDEO 0xE0
#define MATROSKA_ID_CODECID    0x86
#define MATROSKA_ID_CODECPRIVATE 0x63A2
#define MATROSKA_ID_CODECNAME  0x258688
#define MATROSKA_ID_CODECINFOURL 0x3B4040
#define MATROSKA_ID_CODECDOWNLOADURL 0x26B240
#define MATROSKA_ID_CODECDECODEALL 0xAA
#define MATROSKA_ID_TRACKNAME  0x536E
#define MATROSKA_ID_TRACKLANGUAGE 0x22B59C
#define MATROSKA_ID_TRACKFLAGENABLED 0xB9
#define MATROSKA_ID_TRACKFLAGDEFAULT 0x88
#define MATROSKA_ID_TRACKFLAGFORCED 0x55AA
#define MATROSKA_ID_TRACKFLAGLACING 0x9C
#define MATROSKA_ID_TRACKMINCACHE 0x6DE7
#define MATROSKA_ID_TRACKMAXCACHE 0x6DF8
#define MATROSKA_ID_TRACKDEFAULTDURATION 0x23E383
#define MATROSKA_ID_TRACKCONTENTENCODINGS 0x6D80
#define MATROSKA_ID_TRACKCONTENTENCODING 0x6240
#define MATROSKA_ID_TRACKTIMECODESCALE 0x23314F
#define MATROSKA_ID_TRACKMAXBLKADDID 0x55EE
/* IDs in the trackvideo master */
#define MATROSKA_ID_VIDEOFRAMERATE 0x2383E3
#define MATROSKA_ID_VIDEODISPLAYWIDTH 0x54B0
#define MATROSKA_ID_VIDEODISPLAYHEIGHT 0x54BA
#define MATROSKA_ID_VIDEOPIXELWIDTH 0xB0
#define MATROSKA_ID_VIDEOPIXELHEIGHT 0xBA
#define MATROSKA_ID_VIDEOPIXELCROPB 0x54AA
#define MATROSKA_ID_VIDEOPIXELCROPT 0x54BB
#define MATROSKA_ID_VIDEOPIXELCROPL 0x54CC
#define MATROSKA_ID_VIDEOPIXELCROPR 0x54DD
#define MATROSKA_ID_VIDEODISPLAYUNIT 0x54B2
#define MATROSKA_ID_VIDEOFLAGINTERLACED 0x9A
#define MATROSKA_ID_VIDEOSTEREOMODE 0x53B9
#define MATROSKA_ID_VIDEOSTEREOMODE_3D 0x53B8
#define MATROSKA_ID_VIDEOASPECTRATIO 0x54B3
#define MATROSKA_ID_VIDEOCOLORSPACE 0x2EB524
/* IDs in the trackaudio master */
#define MATROSKA_ID_AUDIOSAMPLINGFREQ 0xB5
#define MATROSKA_ID_AUDIOOUTSAMPLINGFREQ 0x78B5
#define MATROSKA_ID_AUDIOBITDEPTH 0x6264
#define MATROSKA_ID_AUDIOCHANNELS 0x9F
/* IDs in the content encoding master */
#define MATROSKA_ID_ENCODINGORDER 0x5031
#define MATROSKA_ID_ENCODINGSCOPE 0x5032
#define MATROSKA_ID_ENCODINGTYPE 0x5033
#define MATROSKA_ID_ENCODINGCOMPRESSION 0x5034
#define MATROSKA_ID_ENCODINGCOMPALGO 0x4254
#define MATROSKA_ID_ENCODINGCOMPSETTINGS 0x4255
/* ID in the cues master */
#define MATROSKA_ID_POINTENTRY 0xBB
/* IDs in the pointentry master */
#define MATROSKA_ID_CUETIME    0xB3
#define MATROSKA_ID_CUETRACKPOSITION 0xB7
/* IDs in the cuetrackposition master */
#define MATROSKA_ID_CUETRACK   0xF7
#define MATROSKA_ID_CUECLUSTERPOSITION 0xF1
#define MATROSKA_ID_CUEBLOCKNUMBER 0x5378
/* IDs in the tags master */
#define MATROSKA_ID_TAG                 0x7373
#define MATROSKA_ID_SIMPLETAG           0x67C8
#define MATROSKA_ID_TAGNAME             0x45A3
#define MATROSKA_ID_TAGSTRING           0x4487
#define MATROSKA_ID_TAGLANG             0x447A
#define MATROSKA_ID_TAGDEFAULT          0x44B4
#define MATROSKA_ID_TAGTARGETS          0x63C0
/* IDs in the seekhead master */
#define MATROSKA_ID_SEEKENTRY  0x4DBB
/* IDs in the seekpoint master */
#define MATROSKA_ID_SEEKID     0x53AB
#define MATROSKA_ID_SEEKPOSITION 0x53AC
/* IDs in the cluster master */
#define MATROSKA_ID_CLUSTERTIMECODE 0xE7
#define MATROSKA_ID_CLUSTERPOSITION 0xA7
#define MATROSKA_ID_CLUSTERPREVSIZE 0xAB
#define MATROSKA_ID_BLOCKGROUP 0xA0
#define MATROSKA_ID_SIMPLEBLOCK 0xA3
/* IDs in the blockgroup master */
#define MATROSKA_ID_BLOCK      0xA1
#define MATROSKA_ID_BLOCKDURATION 0x9B
#define MATROSKA_ID_BLOCKREFERENCE 0xFB
/* IDs in the attachments master */
#define MATROSKA_ID_ATTACHEDFILE        0x61A7
#define MATROSKA_ID_FILEDESC            0x467E
#define MATROSKA_ID_FILENAME            0x466E
#define MATROSKA_ID_FILEMIMETYPE        0x4660
#define MATROSKA_ID_FILEDATA            0x465C
#define MATROSKA_ID_FILEUID             0x46AE
/* IDs in the chapters master */
#define MATROSKA_ID_EDITIONENTRY        0x45B9
#define MATROSKA_ID_CHAPTERATOM         0xB6
#define MATROSKA_ID_CHAPTERTIMESTART    0x91
#define MATROSKA_ID_CHAPTERTIMEEND      0x92
#define MATROSKA_ID_CHAPTERDISPLAY      0x80
#define MATROSKA_ID_CHAPSTRING          0x85
#define MATROSKA_ID_CHAPLANG            0x437C
#define MATROSKA_ID_EDITIONUID          0x45BC
#define MATROSKA_ID_EDITIONFLAGHIDDEN   0x45BD
#define MATROSKA_ID_EDITIONFLAGDEFAULT  0x45DB
#define MATROSKA_ID_EDITIONFLAGORDERED  0x45DD
#define MATROSKA_ID_CHAPTERUID          0x73C4
#define MATROSKA_ID_CHAPTERFLAGHIDDEN   0x98
#define MATROSKA_ID_CHAPTERFLAGENABLED  0x4598
#define MATROSKA_ID_CHAPTERPHYSEQUIV    0x63C3
/**
 * Allowed maximum sps and pps sets
 */
#define MAX_SPS_PARAM_SETS  2
#define MAX_PPS_PARAM_SETS  2
#define TICKS_PER_SECOND 1000000
/**
 * Maximum no of tracks allowed in the file
 */
#define NVMM_MKVPARSER_MAX_TRACKS          16
#define BUFSIZE 1024*10 //Data read size for framing info

#define MPEG4_AUDIO         0x40
#define MPEG2AACLC_AUDIO    0x67
#define MP3_AUDIO           0x6B
#define MPEG4_QCELP         0xE1
#define MPEG4_EVRC          0xD1
#define NVMKV_ISTRACKAUDIO(x) ((x == NvMkvCodecType_AAC)   || (x == NvMkvCodecType_MP3)   || (x == NvMkvCodecType_AACSBR) || (x == NvMkvCodecType_VORBIS))
#define NVMKV_ISTRACKVIDEO(x) ((x == NvMkvCodecType_MPEG2) || (x == NvMkvCodecType_MPEG4) || (x == NvMkvCodecType_AVC))
/************************* Structure prototypes *******************************/
/**
 * Structure to store the AAC audio track configuration.
 */
typedef struct NvMkvAacConfigurationDataRec
{
    NvU32 objectType;
    NvU32 samplingFreqIndex;
    NvU32 samplingFreq;
    NvU32 noOfChannels;
    NvU32 sampleSize;
    NvU32 channelConfiguration;
    NvU32 FrameLengthFlag;
    NvU32 DependsOnCoreCoder;
    NvU32 CoreCoderDelay;
    NvU32 ExtensionFlag;
    NvU32 sbrPresentFlag;
    NvU32 extensionAudioObjectType;
    NvU32 extensionSamplingFreqIndex;
    NvU32 extensionSamplingFreq;
    NvU32 epConfig;
    NvU32 directMapping;
    NvU32 bitrate;
} NvMkvAacConfigurationData;

typedef struct NvMkvMp3ConfigurationDataRec
{
    NvU32 samplingFreqIndex;
    NvU32 samplingFreq;
    NvU32 noOfChannels;
    NvU32 sampleSize;
    NvU32 channelConfiguration;
} NvMkvMp3ConfigurationData;

typedef struct NvMkvAc3ConfigurationDataRec
{
    NvU32 samplingFreqIndex;
    NvU32 samplingFreq;
    NvU32 noOfChannels;
    NvU32 sampleSize;
    NvU32 channelConfiguration;
} NvMkvAc3ConfigurationData;

typedef struct NvAVCConfigDataRec
{
    NvS32 ES_ID;
    NvS32 dataRefIndex;
    NvS32 Width;
    NvS32 Height;
    NvS32 HorizontalResolution;
    NvS32 VerticalResolution;
    NvS32 Data_size;
    NvS32 FrameCount;
    NvS32 field1;
    NvS32 field2;
    NvS32 gamma;
    NvS32 Depth;
    NvS32 ColorTableID;
    NvS32 configVersion;
    NvS32 profileIndication;
    NvS32 profileCompatibility;
    NvS32 levelIndication;
    NvS32 lengthSizeMinusOne;
    NvS32 noOfSequenceParamSets;
    NvS32 seqParamSetLength[MAX_SPS_PARAM_SETS];
    NvU8 spsNalUnit[MAX_SPS_PARAM_SETS][128]; // 128 bytes is arbitrary: Need sufficient storage
    NvS32 noOfPictureParamSets;
    NvS32 picParamSetLength[MAX_PPS_PARAM_SETS];
    NvU8 ppsNalUnit[MAX_PPS_PARAM_SETS][128]; // 128 bytes is arbitrary: Need sufficient storage
} NvAVCConfigData;

/**
 * FF REW Status  .
 */
typedef enum
{
    NvParserFFREWStatus_FF,             /**< Forward mode */
    NvParserFFREWStatus_REW,            /**< Rewind mode */
    NvParserFFREWStatus_PLAY,            /**< Play mode */
    NvParserFFREWStatus_Force32 = 0x7FFFFFFF
} NvParserFFREWStatus;

/**
 * Error conditions.
 */
typedef enum
{
    NvMkvParserStatus_NOERROR = 0,                            /**< No error */
    NvMkvParserStatus_INVALID_DATA = -1,
    NvMkvParserStatus_PIPE_ERROR = -2,
    NvMkvParserStatus_MEMORY_ERROR    = -3,
    NvMKvParserStatus_GETPOS_FAIL = -4,
    NvMkvParserStatus_SETPOS_FAIL = -5,
    NvMkvParserStatus_ERROR = -10,                             /**< Encountered a general non recoverable error */
    NvMkvParserStatus_BADFILE_ERROR = -11,
    NvMkvParserStatus_BAD_AUDIO_SAMPLE_RATE = -12,
    NvMkvParserStatus_SEGMENT_ERROR = -12,
    NvMkvParserStatus_NoTracksFound = -13,
    NvMkvParserStatus_AddCueFailed = -14,
    NvMkvParserStatus_READ_EBML_ID_FAIL = -15,
    NvMkvParserStatus_READ_EBML_NUM_FAIL = -16,
    NvMkvParserStatus_READ_EBML_UINT_FAIL = -17,
    NvMkvParserStatus_PARSE_AVC_FAIL = -18,
    NvMkvParserStatus_EOF = -19,
    NvMkvParserStatus_SEEK_EOF = -20,
    NvMkvParserStatus_SEEK_BOF = -21,
    NvMkvParserStatus_SEEK_ERR = -22,
    NvMkvParserStatus_BAD_RATE = -23,
    NvMkvParserStatus_NOT_IMPLEMENTED = -24,
} NvMkvParserStatus;

/************************************************************************/
/**
 * Track Type
 */
// This needs to be fixed - TBD
// Demuxer uses matroska defines while these cannot be exposed to the parser or core layers
// So, the hardcodes in this enum *MUST NOT* be changed
typedef enum
{
    NvMkvTrackTypes_None = 0x0,       /* DONOT CHANGE */
    NvMkvTrackTypes_VIDEO = 0x1,      /* DONOT CHANGE */
    NvMkvTrackTypes_AUDIO = 0x2,      /* DONOT CHANGE */
    NvMkvTrackTypes_SUBTITLE = 0x11,  /* DONOT CHANGE */
    NvMkvTrackTypes_OTHER = 0x21      /* Arbitrary*/
} NvMkvTrackTypes;


typedef enum
{
    MATROSKA_TRACK_ENCODING_COMP_ZLIB        = 0,
    MATROSKA_TRACK_ENCODING_COMP_BZLIB       = 1,
    MATROSKA_TRACK_ENCODING_COMP_LZO         = 2,
    MATROSKA_TRACK_ENCODING_COMP_HEADERSTRIP = 3,
} MatroskaTrackEncodingCompAlgo;

/**
* Codec type
**/
typedef enum
{
    NvMkvCodecType_NONE,
    NvMkvCodecType_AAC,
    NvMkvCodecType_AACSBR,
    NvMkvCodecType_MP3,
    NvMkvCodecType_VORBIS,
    NvMkvCodecType_AC3,
    NvMkvCodecType_EAC3,
    NvMKvCodecType_DTS,
    NvMKvCodecType_FLAC,
    NvMKvCodecType_MP1,
    NvMKvCodecType_MP2,
    NvMKvCodecType_MPEG1,
    NvMkvCodecType_AVC,
    NvMkvCodecType_MPEG4,
    NvMkvCodecType_MPEG2,
    NvMkvCodecType_UNKNOWN
} NvMkvCodecType;

#endif //INCLUDED_NVMM_MP4PARSER_DEFINES_H


