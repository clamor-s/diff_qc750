/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#ifndef INCLUDED_NVCAMERA_HDR_H
#define INCLUDED_NVCAMERA_HDR_H


#include "nvcommon.h"
#include "nverror.h"

#if defined(__cplusplus)
extern "C"
{
#endif

// creates a new instance of the HDR processing class, writes the
// value to the "key" variable that the client passes in.
// any future interactions with the HDR processing class should include the
// key is returned from this function.  the alg will use that key to determine
// which instance of the class the client is interacting with.
NvError NvCameraHdrAlloc(NvU32 *key);

// sets up an instance of the HDR processing class, specified by "key", to take
// input buffers of the format specified by "Width", "Height", "Pitch_Y",
// "Pitch_UV".  the algorithm will require "NumberOfImages" buffers as input before
// it can compose an output.
NvError NvCameraHdrInit(NvU32 key, NvU32 Width, NvU32 Height, NvU32 Pitch_Y, NvU32 Pitch_UV, NvU32 NumberOfImages);

// give an input buffer to the HDR algorithm.  sends it to the instance of the
// alg processing class specified by "key".  pointers to the Y, U, and V buffers
// are specified by "Y", "U", and "V".  these buffers will be read according
// to the width/height/pitch params set in NvCameraHdrInit.  the buffers
// will be copied into a local buffer, so that clients can recycle them
// as soon as the call to NvCameraHdrAddImageBuffer completes.
NvError NvCameraHdrAddImageBuffer(NvU32 key, NvU8 *Y, NvU8 *U, NvU8 *V);

// called by the client once the proper number of images have been sent to the
// HDR class via NvCameraHdrAddImageBuffer.  runs composition on the instance
// of the HDR processing class specified by "key".  "Y", "U", and "V" are
// pointers to the Y, U, and V planes of the output buffer.  output buffer size
// should match input buffer size specified by the NvCameraHdrInit call.
NvError NvCameraHdrCompose(NvU32 key, NvU8 *Y, NvU8 *U, NvU8 *V);

// resets the state of the HDR processing class specified by "key"
NvError NvCameraHdrResetProcessing(NvU32 key);

// frees all resources associated with HDR processing class specified by "key"
NvError NvCameraHdrFree(NvU32 key);

#if defined(__cplusplus)
}
#endif

#endif

