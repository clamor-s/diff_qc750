/* Copyright (c) 2012,  NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of the NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __JTEGRA_H__
#define __JTEGRA_H__

#include "jinclude.h"
#include "jpeglib.h"

#ifdef TEGRA_ACCELERATE
GLOBAL(j_tegra_mgr_ptr)
jpegTegraDecoderMgrCreate (j_decompress_ptr cinfo);

GLOBAL(j_tegra_mgr_ptr)
jpegTegraEncoderMgrCreate (j_compress_ptr cinfo);

GLOBAL(void)
jpegTegraMgrDestroy (j_common_ptr cinfo);

GLOBAL(void)
jpegTegraMgrDestroy (j_common_ptr cinfo);

GLOBAL(boolean)
jpegTegraEncoderGetBits (j_compress_ptr cinfo, unsigned int *numBytesAvailable);

GLOBAL(boolean)
jpegTegraEncoderAccelerationCheck (j_compress_ptr cinfo);

GLOBAL(void)
jpegTegraEncoderSurfaceCheck(j_compress_ptr cinfo);

GLOBAL(boolean)
jpegTegraEncoderCompress(j_compress_ptr cinfo);

GLOBAL(boolean)
jpegTegraEncoderRGB2YUVSurf(j_compress_ptr cinfo, unsigned char *scanlines);

GLOBAL(boolean)
jpegTegraDecoderAccelerationCheck (j_decompress_ptr cinfo);

GLOBAL(void)
jpegTegraDecoderResize (j_decompress_ptr cinfo);

GLOBAL(void)
jpegTegraDecoderSurfaceResize (j_decompress_ptr cinfo);

GLOBAL(boolean)
jpegTegraDecoderRender (j_decompress_ptr cinfo, unsigned int *outWidth, unsigned int *outHeight);

GLOBAL(unsigned int)
jpegTegraDecoderRenderWait (j_decompress_ptr cinfo);

GLOBAL(unsigned int)
jpegTegraDecoderOutputHeight (j_decompress_ptr cinfo);

GLOBAL(void)
jpegTegraDecoderSetParams (j_decompress_ptr cinfo);

GLOBAL(void)
jpegTegraEncoderSetBuffer(j_compress_ptr cinfo, unsigned char *buffer);

GLOBAL(void)
jpegTegraDecoderSetBuffer (j_decompress_ptr cinfo,
        unsigned char *buffer,
        unsigned int bytes_in_buffer);
#endif /* TEGRA_ACCELERATE */

#endif
