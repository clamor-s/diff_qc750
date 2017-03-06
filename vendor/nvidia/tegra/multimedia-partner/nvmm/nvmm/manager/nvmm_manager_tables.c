/* Copyright (c) 2006 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an
 * express license agreement from NVIDIA Corporation is strictly prohibited.
 */


#include "nvmm_manager_internal.h"
#include "nvmm.h"

// Table of all available resource profiles
NvBlockProfile BlockProfileTable[] =
{
    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_FileReader,         // element type
        NvBlockCat_Other,                 // category
        NvMMLocale_CPU,                   // default locale of the element
        NvBlockFlag_Default,              // flags
        0, 0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_SuperParser,        // element type
        NvBlockCat_Parser,                // category
        NvMMLocale_CPU,                   // default locale of the element
        NvBlockFlag_Default,              // flags
        0, 0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_AudioMixer,         // element type
        NvBlockCat_Other,                 // category
        NvMMLocale_AVP,                   // default locale of the element
        (NvBlockFlag_Default |
         NvBlockFlag_UseCustomStack),     // flags
        2048,                             // stack size
        0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_SinkAudio,          // element type
        NvBlockCat_Renderer,              // category
        NvMMLocale_CPU,                   // default locale of the element
        NvBlockFlag_Default,              // flags
        0, 0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_DecAAC,             // element type
        NvBlockCat_AudioDecoder,          // category
        NvMMLocale_AVP,                   // default locale of the element
        (NvBlockFlag_Default |
         NvBlockFlag_HWA |
         NvBlockFlag_CPU_OK |
         NvBlockFlag_UseGreedyIramAlloc), // flags
        2048,                             // stack size
        0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_DecWAV,             // element type
        NvBlockCat_AudioDecoder,          // category
        NvMMLocale_AVP,                   // default locale of the element
        NvBlockFlag_Default,              // flags
        2048,                             // stack size
        0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_DecMP3,             // element type
        NvBlockCat_AudioDecoder,          // category
        NvMMLocale_AVP,                   // default locale of the element
        (NvBlockFlag_Default |
         NvBlockFlag_HWA |
         NvBlockFlag_CPU_OK |
         NvBlockFlag_UseCustomStack |
         NvBlockFlag_UseGreedyIramAlloc), // flags
        3072,                             // stack size
        0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_DecWMA,             // element type
        NvBlockCat_AudioDecoder,          // category
        NvMMLocale_CPU,                   // default locale of the element
        (NvBlockFlag_Default |
         NvBlockFlag_HWA |
         NvBlockFlag_CPU_OK |
         NvBlockFlag_UseCustomStack |
         NvBlockFlag_UseGreedyIramAlloc), // flags
        3072,                             // stack size
        0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_DecWMAPRO,          // element type
        NvBlockCat_AudioDecoder,          // category
        NvMMLocale_AVP,                   // default locale of the element
        (NvBlockFlag_Default |
         NvBlockFlag_HWA |
         NvBlockFlag_CPU_OK),             // flags
        2048,                             // stack size
        0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_3gpFileReader,      // element type
        NvBlockCat_Other,                 // category
        NvMMLocale_CPU,                   // default locale of the element
        (NvBlockFlag_Default),            // flags
        0, 0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_DecMPEG4,           // element type
        NvBlockCat_VideoDecoder,          // category
        NvMMLocale_CPU,                   // default locale of the element
        (NvBlockFlag_Default |
         NvBlockFlag_HWA),                // flags
        0, 0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_DecH264,            // element type
        NvBlockCat_VideoDecoder,          // category
        NvMMLocale_CPU,                   // default locale of the element
        NvBlockFlag_Default,              // flags
        0, 0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_DecH264AVP,         // element type
        NvBlockCat_VideoDecoder,          // category
        NvMMLocale_AVP,                   // default locale of the element
        (NvBlockFlag_Default |
         NvBlockFlag_HWA |
         NvBlockFlag_UseCustomStack),     // flags
        2048,                             // stack size
        0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_DecMPEG4AVP,        // element type
        NvBlockCat_VideoDecoder,          // category
        NvMMLocale_AVP,                   // default locale of the element
        NvBlockFlag_Default,              // flags
        0, 0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_DecVC1,             // element type
        NvBlockCat_VideoDecoder,          // category
        NvMMLocale_AVP,                   // default locale of the element
        (NvBlockFlag_Default |
         NvBlockFlag_HWA),                // flags
        0, 0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_DecVC1_2x,          // element type
        NvBlockCat_VideoDecoder,          // category
        NvMMLocale_AVP,                   // default locale of the element
        (NvBlockFlag_Default |
         NvBlockFlag_UseNewBlockType |
         NvBlockFlag_HWA),                // flags
        0, 0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_DecH264_2x,         // element type
        NvBlockCat_VideoDecoder,          // category
        NvMMLocale_AVP,                   // default locale of the element
        (NvBlockFlag_Default |
         NvBlockFlag_UseNewBlockType |
         NvBlockFlag_HWA),                // flags
        0, 0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_DecMPEG2,           // element type
        NvBlockCat_VideoDecoder,          // category
        NvMMLocale_AVP,                   // default locale of the element
        (NvBlockFlag_Default |
         NvBlockFlag_UseNewBlockType |
         NvBlockFlag_HWA),                // flags
        0, 0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_DecMPEG2VideoVld,   // element type
        NvBlockCat_VideoDecoder,          // category
        NvMMLocale_CPU,                   // default locale of the element
        (NvBlockFlag_Default)             // flags
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_SinkVideo,          // element type
        NvBlockCat_Other,                 // category
        NvMMLocale_CPU,                   // default locale of the element
        NvBlockFlag_Default,              // flags
        0, 0, 0, 0, 0, 0
    },

    {
        sizeof(NvBlockProfile),           // size

        NvMMBlockType_UnKnown,            // element type
        NvBlockCat_Other,                 // category
        0,                                // default locale of the element
        NvBlockFlag_Default,              // flags
        0, 0, 0, 0, 0, 0
    },
};




const NvU32 NumResourceProfiles = (sizeof(BlockProfileTable)/sizeof(NvBlockProfile));


