/*
 * Copyright (c) 2010-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
*/
#ifndef _TVMR_H
#define _TVMR_H

#include "tvmr_surface.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* forward declarations */

struct timespec;

/* defines */

#define TVMR_FLIP_QUEUE_DISPLAY_WITH_TIMESTAMP

#define TVMR_VIDEO_MIXER_FEATURE_ADVANCE1_DEINTERLACING (1<<0)

#define TVMR_VIDEO_MIXER_ATTRIBUTE_BRIGHTNESS       (1<<0)
#define TVMR_VIDEO_MIXER_ATTRIBUTE_CONTRAST         (1<<1)
#define TVMR_VIDEO_MIXER_ATTRIBUTE_SATURATION       (1<<2)
#define TVMR_VIDEO_MIXER_ATTRIBUTE_COLOR_STANDARD   (1<<3)
#define TVMR_VIDEO_MIXER_ATTRIBUTE_DEINTERLACE_TYPE (1<<4)
#define TVMR_VIDEO_MIXER_ATTRIBUTE_Y_INVERT         (1<<5)


#define TVMR_FLIP_QUEUE_ATTRIBUTE_BRIGHTNESS       (1<<0)
#define TVMR_FLIP_QUEUE_ATTRIBUTE_CONTRAST         (1<<1)
#define TVMR_FLIP_QUEUE_ATTRIBUTE_SATURATION       (1<<2)
#define TVMR_FLIP_QUEUE_ATTRIBUTE_HUE              (1<<3)
#define TVMR_FLIP_QUEUE_ATTRIBUTE_COLOR_STANDARD   (1<<4)

#define TVMR_MAX_CAPTURE_FIELDS 65  /*  (MAX_CAPTURE_FIELDS >> 1) <= 32 */

#define TVMR_MIN_CAPTURE_FRAME_BUFFERS  2
#define TVMR_MAX_CAPTURE_FRAME_BUFFERS  32

#define TVMR_VIDEO_ENCODER_FLAG_FORCE_I_FRAME  (1<<0)

/* enums */

typedef enum {
  TVMR_STATUS_OK = 0,
  TVMR_STATUS_BAD_PARAMETER,
  TVMR_STATUS_PENDING,
  TVMR_STATUS_NONE_PENDING,
  TVMR_STATUS_INSUFFICIENT_BUFFERING,
  TVMR_STATUS_TIMED_OUT,
  TVMR_STATUS_UNSUPPORTED,
  TVMR_STATUS_BAD_ALLOC,
  TVMR_STATUS_SUBMIT_ERROR
} TVMRStatus;

typedef enum {
   TVMR_PICTURE_STRUCTURE_TOP_FIELD = 0x1,
   TVMR_PICTURE_STRUCTURE_BOTTOM_FIELD = 0x2,
   TVMR_PICTURE_STRUCTURE_FRAME = 0x3
} TVMRPictureStructure;

typedef enum {
   TVMR_COLOR_STANDARD_ITUR_BT_601,
   TVMR_COLOR_STANDARD_ITUR_BT_709,
   TVMR_COLOR_STANDARD_SMPTE_240M,
   TVMR_COLOR_STANDARD_ITUR_BT_601_ER  /* extended range (JPEG) */
} TVMRColorStandard;

typedef enum {
   TVMR_DEINTERLACE_TYPE_BOB = 0,
   TVMR_DEINTERLACE_TYPE_ADVANCE1 = 1,

   TVMR_DEINTERLACE_TYPE_Force32 = 0x7FFFFFFF
} TVMRDeinterlaceType;

typedef enum {
   TVMR_CAPTURE_FORMAT_NTSC = 0,
   TVMR_CAPTURE_FORMAT_PAL = 1,
   TVMR_CAPTURE_FORMAT_VIP_NTSC = 0,
   TVMR_CAPTURE_FORMAT_VIP_PAL = 1,
   TVMR_CAPTURE_FORMAT_CSIA_1440_540 = 2,
   TVMR_CAPTURE_FORMAT_CSIA_WVGA = 3,
   TVMR_CAPTURE_FORMAT_CSIA_1024_480 = 4,
   TVMR_CAPTURE_FORMAT_CSIA_1440_542 = 5,
   TVMR_CAPTURE_FORMAT_CSIA_800_482 = 6,
   TVMR_CAPTURE_FORMAT_CSIA_1024_482 = 7
} TVMRCaptureFormat;

typedef enum {
   TVMR_VIDEO_CODEC_H264,
   TVMR_VIDEO_CODEC_H264_MVC,
   TVMR_VIDEO_CODEC_VC1,  /* SIMPLE, MAIN */
   TVMR_VIDEO_CODEC_VC1_ADVANCED,
   TVMR_VIDEO_CODEC_MPEG1,
   TVMR_VIDEO_CODEC_MPEG2,
   TVMR_VIDEO_CODEC_MPEG4,
   TVMR_VIDEO_CODEC_MJPEG
} TVMRVideoCodec;

typedef enum {
  TVMR_BLOCKING_TYPE_NEVER,
  TVMR_BLOCKING_TYPE_IF_PENDING,
  TVMR_BLOCKING_TYPE_ALWAYS
} TVMRBlockingType;

typedef enum {
    TVMRDrmNetflix = 0,
    TVMRDrmWidevine = 1
} TVMRDrmType;

typedef enum {
    TVMR_SLH_PRESENT = 0x1,
    TVMR_SPS_PRESENT = 0x2,
    TVMR_PPS_PRESENT = 0x4
} TVMRNalPresentFlag;

typedef enum {
   TVMR_CAPTURE_CSI_INTERFACE_TYPE_CSI_A,
   TVMR_CAPTURE_CSI_INTERFACE_TYPE_CSI_B,
   TVMR_CAPTURE_CSI_INTERFACE_TYPE_CSI_AB,

   TVMR_CAPTURE_INTERFACE_TYPE_Force32 = 0x7FFFFFFF
} TVMRCaptureCSIInterfaceType;

typedef enum {
   TVMR_CAPTURE_INPUT_FORMAT_TYPE_YUV420 = 0,
   TVMR_CAPTURE_INPUT_FORMAT_TYPE_YUV422,
   TVMR_CAPTURE_INPUT_FORMAT_TYPE_YUV444,
   TVMR_CAPTURE_INPUT_FORMAT_TYPE_RGB888,

   TVMR_CAPTURE_INPUT_FORMAT_TYPE_Force32 = 0x7FFFFFFF
} TVMRCaptureInputFormatType;

typedef struct {
   TVMRVideoCodec codec;
   TVMRSurfaceType sourceType;
   NvU16 encodedWidth;  /* in luma pixels */
   NvU16 encodedHeight; /* in luma pixels */
} TVMRVideoEncoder;

typedef struct {
   NvU32 targetBytesPerFrame;
   NvU16 framesPerGOP;
} TVMRVideoEncoderAttributes;

typedef struct {
   float red;
   float green;
   float blue;
   float alpha;
} TVMRColor;

#define TVMR_RENDER_FLAG_ROTATE_0   0
#define TVMR_RENDER_FLAG_ROTATE_90  1
#define TVMR_RENDER_FLAG_ROTATE_180 2
#define TVMR_RENDER_FLAG_ROTATE_270 3

#define TVMR_RENDER_FLAG_FLIP_HORIZONTAL   (1<<2)
#define TVMR_RENDER_FLAG_FLIP_VERTICAL     (1<<3)

#define TVMR_RENDER_FLAG_COLOR_PER_VERTEX  (1<<4)


typedef NvU32 TVMRTime;

#define TVMR_TIMEOUT_INFINITE 0xFFFFFFFF

/* structs */

typedef struct {
  NvS16 x0;    /*   left edge inclusive */
  NvS16 y0;    /*    top edge inclusive */
  NvS16 x1;    /*  right edge exclusive */
  NvS16 y1;    /* bottom edge exclusive */
} TVMRRect;

typedef struct {
   float brightness;
   float contrast;
   float saturation;
   TVMRColorStandard colorStandard;
   TVMRDeinterlaceType deinterlaceType;
   NvBool yInvert;
} TVMRVideoMixerAttributes;

typedef struct {
   /* nothing for now */
   NvU32 placeholder;
} TVMRDevice;

typedef struct {
  TVMRDevice *device;
  TVMRSurfaceType type;
  NvU32 features;
  NvU16 srcWidth;
  NvU16 srcHeight;
} TVMRVideoMixer;

typedef struct {
  TVMRSurfaceType type;
  TVMRSurface palette;
} TVMRPalette;

typedef struct {
   TVMRCaptureCSIInterfaceType interfaceType;
   TVMRCaptureInputFormatType inputFormatType;
   NvU16 width;
   NvU16 height;
   NvU16 startX;
   NvU16 startY;
   NvU16 extraLines;
   NvBool interlace;
   NvU32 lanes;
} TVMRCaptureCSISetting;

typedef struct {
   TVMRSurfaceType surfaceType;
   NvU16 width;
   NvU16 height;
   NvU8 numBuffers;
} TVMRCapture;

/* functions */

//  TVMRDeviceCreate
//
//  A TVMRDevice holds the infrastructure needed for a subset of TVMR
//  rendering operations, primarily ones that need 3D engine access, but
//  this infrastructure is leveraged for some other operations such as
//  TVMRVideoSurface or TVMROutputSurface creation.
//  On failure, TVMRDeviceCreate returns NULL rather than a TVMRDevice*.
//
//  Arguments:
//     deprecated - This should be NULL.

TVMRDevice *
TVMRDeviceCreate (
   void *deprecated  /* NULL */
);


//  TVMRDeviceDestroy
//
//  Destroy a previously created TVMRDevice.
//  TVMRDeviceDestroy does not destroy objects created against this
//  TVMRDevice (eg. TVMRVideoMixers or TVMRVideoSurfaces), but in most
//  cases renders them unusable. In general, objects created against this
//  TVMRDevice should be freed before the TVMRDevice.

void
TVMRDeviceDestroy (
   TVMRDevice *device
);


//  TVMRVideoMixerCreate
//
//  The principle job of the video mixer is to convert YUV data to
//  RGB and perform other postprocessing at the same time, such as
//  deinterlacing.  A TVMRVideoMixer is intended to be used with a
//  single stream as it can contain stream-dependent state.  Multiple
//  streams should use multiple TVMRVideoMixers.  On failure,
//  TVMRVideoMixerCreate returns NULL rather than a TVMRVideoMixer*.
//
//  Arguments:
//    device - Essentially, the 3D channel this video mixer will use.
//    srcFormat, srcWidth, srcHeight -
//       The width, height and format of the stream this TVMRVideoMixer
//       is to be used with.  This is needed for the up-front creation
//       of scratch surfaces required for mixer operation.  Currently,
//       the maximum supported width and height is 2048 pixels.  The
//       supported formats are:
//            TVMRSurfaceType_YV12
//            TVMRSurfaceType_YV16
//            TVMRSurfaceType_YV16_Transposed
//            TVMRSurfaceType_YV24
//            TVMRSurfaceType_NV12
//            TVMRSurfaceType_YV16x2
//
//   features - This selects which features this TVMRVideoMixer will
//       support.  This determines which scratch surfaces
//       TVMRVideoMixerCreate will create up-front.  At present,
//       the following features are supported and may be OR'd together:
//          TVMR_VIDEO_MIXER_FEATURE_ADVANCE1_DEINTERLACING


TVMRVideoMixer *
TVMRVideoMixerCreate(
   TVMRDevice *device,
   TVMRSurfaceType srcFormat,
   NvU16 srcWidth,
   NvU16 srcHeight,
   NvU32 features
);

//  TVMRVideoMixerDestroy
//
//  Destroy a previously created TVMRVideoMixer.

void
TVMRVideoMixerDestroy(
   TVMRVideoMixer *vmr
);

//  TVMRVideoMixerSetAttributes
//
//  Change TVMRVideoMixer attributes.  A pointer to a TVMRVideoMixerAttributes
//  is supplied but only the attributes specified in the attribute_mask will
//  be changed.  The following attributes and supported:
//
//  brightness - A value clamped to between -1.0 and 1.0, initialized to
//               0.0 at TVMRVideoMixer creation.  The corresponding
//               attribute mask is TVMR_VIDEO_MIXER_ATTRIBUTE_BRIGHTNESS.
//  contrast   - A value clamped to between 0.0 and 10.0, initialized to
//               1.0 at TVMRVideoMixer creation.  The corresponding
//               attribute mask is TVMR_VIDEO_MIXER_ATTRIBUTE_CONTRAST.
//  saturation - A value clamped to between 0.0 and 10.0, initialized to
//               1.0 at TVMRVideoMixer creation.  The corresponding
//               attribute mask is TVMR_VIDEO_MIXER_ATTRIBUTE_SATURATION.
//  colorStandard - One of the following:
//                    TVMR_COLOR_STANDARD_ITUR_BT_601  (default)
//                    TVMR_COLOR_STANDARD_ITUR_BT_709
//                    TVMR_COLOR_STANDARD_SMPTE_240M
//                The corresponding attribute mask is
//                TVMR_VIDEO_MIXER_ATTRIBUTE_COLOR_STANDARD .
//  deinterlaceType - One of the following:
//                       TVMR_DEINTERLACE_TYPE_BOB  (default)
//                       TVMR_DEINTERLACE_TYPE_ADVANCE1
//                The corresponding attribute mask is
//                TVMR_VIDEO_MIXER_ATTRIBUTE_DEINTERLACE_TYPE.
//                TVMR_DEINTERLACE_TYPE_ADVANCE1 is ignored if the
//                TVMRVideoMixer was not created with the
//                TVMR_VIDEO_MIXER_FEATURE_ADVANCE1_DEINTERLACING flag set.
//  yInvert - If NV_TRUE, TVMRVideoMixerRender will render the
//            TVMROutputSurface upside down.  yInvert is NV_FALSE by
//            default.

void
TVMRVideoMixerSetAttributes(
   TVMRVideoMixer *vmr,
   NvU32 attribute_mask,
   TVMRVideoMixerAttributes *vmratt
);

//  TVMRVideoMixerRender
//
//  The operation performed by TVMRVideoMixerRender will be equivalent
//  to the following operations in the specified order:
//   1) The rectangle describe by "outputRect" on the "outputSurface" will
//      be cleared to black.  Rendering in stages #2 and #3 are clipped
//      by this outputRect.
//   2) The rectangle described by "vidSrcRect" on the TVMRVideoSurface
//      "curr" will be scaled to the rectangle described by "vidDstRect"
//      within the "outputRect" on the "outputSurface" and will overwrite
//      the contents under that rectangle.  Various TVMRVideoMixer
//      attributes define the details of how the scaling and color
//      space conversion is done.
//   3) The rectangle described by "subSrcRect" on the "subpic" will
//      be scaled to the rectangle described by "subDstRect" within
//      the "outputRect" on the "outputSurface" and will blend with
//      the contents under that rectangle using the subpic's per-pixel
//      alpha values using the equation (sub*sub.alpha + dst*(1-sub.alpha)).
//
//  Arguments:
//     outputSurface, outputRect --
//        The rectangle on the outputSurface that will be modified.
//        If outputRect is NULL, a rectangle the full size of the
//        outputSurface is implied.  If the rectangle is poorly formed
//        (eg. x1 <= x0 or y1 <= y0) nothing will be drawn.  This
//        rectangle will be clipped by the dimension of the output surface.
//     pictureStructure - If TVMR_PICTURE_STRUCTURE_FRAME, the
//        TVMRVideoSurface "curr" is treated as a progressive source and
//        deinterlacing is not performed.  If TVMR_PICTURE_STRUCTURE_TOP_FIELD
//        or TVMR_PICTURE_STRUCTURE_BOTTOM_FIELD, "curr" is treated as
//        an interlaced source and deinterlacing will be performed.
//    next, curr, prev, prev2 -
//        If "curr" is NULL, the video composite (step #2 above) will
//        be skipped.  Otherwise, video compositing will be performed.
//        "next", "prev" and "prev2" are ignored unless the source is
//        interlaced (as determined by pictureStructure) and the
//        deinterlaceType is TVMR_DEINTERLACE_TYPE_ADVANCE1, in which case
//        "next", "prev" and "prev2" correspond to the TVMRVideoSurfaces
//        which hold the next, previous and previous-previous FIELDS.
//        Note that since a TVMRVideoSurface holds a frame (two fields),
//        the same surface may show up twice among next, curr, prev, prev2.
//        The pictureStructure indicates the structure of the "curr" field.
//        It is implied that prev2 would have the same pictureStructure
//        while next and prev would have the opposite pictureStructure.
//    vidSrcRect - The rectangle within "curr" to be used as the source
//        of the video composite.  This rectangle is in frame coordinates
//        even when the pictureStructure indicates a field.  If NULL,
//        a rectangle the full size of the TVMRVideoSurface is implied.
//        If the rectangle specifies pixels outside of the TVMRVideoSurface,
//        pixels from the nearest edge will be used instead.  Horizontal
//        or vertical reflection of the source can be done by swapping
//        the x0 and x1 or y0 and y1 components respectively.
//    vidDstRect - This rectangle is relative to outputRect, not the
//        origin of the outputSurface, and determines the placement of
//        the video output on the outputSurface.  If NULL, a rectangle
//        the size of the outputRect is implied.  If this rectangle is
//        poorly formed (eg x1 <= x0 or y1 <= y0) the effect will be the
//        same as providing a NULL "curr" source.
//    subpic - The subpicture source to be used in stage #3 above.  If
//        NULL, the subpicture composite is skipped.
//    subSrcRect - The rectangle within subpic to be used as the source of
//        the subpicture composite.  If NULL, a rectangle the full size of
//        the subpicture is implied.  If the rectangle specfies pixels
//        outside of the subpicture, pixels from the nearest edge will
//        be used instead.  Horizontal or vertical reflection of the source
//        can be done by swapping the x0 and x1 or y0 and y1 components
//        respectively.
//    subDstRect - This rectangle is relative to outputRect, not the
//        origin of the outputSurface, and determines the placement of the
//        subpicture blending on the outputSurface.  If NULL, a rectangle
//        the size of the outputRect is implied.  If this rectangle is
//        poorly formed (eg x1 <= x0 or y1 <= y0) the effect will be the
//        same as providing a NULL subpic.
//    fencesWait - If not NULL, fencesWait is expected to be a NULL
//        terminated list of TVMRFences.  TVMRVideoMixerRender will delay
//        rendering until after all fences in the list have been reached.
//        Typically, two fences will be present in the list: the fence
//        indicating when it's safe to overwrite the specified outputSurface
//        and the fence indicating when the rendering of the most recent
//        TVMRVideoSurface is completed.
//    fenceDone - It is always guaranteed that TVMRVideoMixerRender will
//        complete in a finite amount of time (that is, it flushes).  If
//        not NULL, fenceDone will be filled with the fence indicating
//        when this TVMRVideoMixerRender operation will complete.  This
//        will indicate when it's safe to use the outputSurface in another
//        module and when TVMRVideoMixerRender is finished reading the
//        provided TVMRVideoSurfaces.
//
//  Deinterlacing:
//   If pictureStructure is not TVMR_PICTURE_STRUCTURE_FRAME, deinterlacing
//   will be performed.  Bob deinterlacing is always available but
//   ADVANCE1 deinterlacing (TVMR_DEINTERLACE_TYPE_ADVANCE1) will
//   be used only if the following conditions are met:
//     1) The TVMRVideoMixer must be created with the
//        TVMR_VIDEO_MIXER_FEATURE_ADVANCE1_DEINTERLACING flag.
//     2) The deinterlaceType attribute must be set to
//        TVMR_DEINTERLACE_TYPE_ADVANCE1
//     3) All 4 source fields must be presented to the TVMRVideoMixer:
//        next, curr, prev and prev2.
//
//   Two deinterlace rates:  The TVMRVideoMixer supports both field-rate
//   frame-rate outputs (eg. 60 Hz and 30 Hz).  The intended rate effects
//   the algorithm used and the TVMRVideoMixer autodetects the intended
//   rate by observing the progression of the field history.  If the
//   field history progresses one field at a time, that is, next/curr/prev
//   fields become curr/prev/prev2 on the next call and the pictureStructure
//   changes parity, the field-rate algorithm is used.  If the field history
//   progresses frame at a time, that is, next/curr fields become prev/prev2
//   on the next call and the pictureStructure stays the same, the frame-
//   rate algorithm is used.  If neither rate is detected or the the three
//   conditions above are not met, deinterlacing falls back to bob
//   deinterlacing.

TVMRStatus
TVMRVideoMixerRender(
   TVMRVideoMixer *vmr,
   TVMROutputSurface *outputSurface,
   TVMRRect *outputRect,
   TVMRPictureStructure pictureStructure,
   TVMRVideoSurface *next,
   TVMRVideoSurface *curr,
   TVMRVideoSurface *prev,
   TVMRVideoSurface *prev2,
   TVMRRect *vidSrcRect,
   TVMRRect *vidDstRect,
   TVMROutputSurface *subpic,
   TVMRRect *subSrcRect,
   TVMRRect *subDstRect,
   TVMRFence *fencesWait,  /* NULL terminated list */
   TVMRFence fenceDone  /* optional */
);

#define TVMRVideoMixerRenderRGB TVMRVideoMixerRender

// TVMRVideoMixerRenderYUV
//
// TVMRVideoMixerRenderYUV is similar to TVMRVideoMixerRender except that
// the destination is a TVMRVideoSurface rather than a TVMROutputSurface
// and some functionality has been omitted.  These omissions include:
//
//  * There is no subpicture compositing because subpictures are composited
//    in RGB space and TVMRVideoMixerRenderYUV operates entirely in YUV
//    space.
//  * There is no outputRect or vidDstRect because the sub-sampled
//    chroma of YUV surfaces make sub-rect drawing awkward.  Rendering is
//    always to the full outputVideoSurface.
//  * Some TVMRVideoMixerAttributes are ignored because they are implemented
//    in the colorspace conversion portion of the pipeline that does not
//    exist in TVMRVideoMixerRenderYUV (brightness, contrast, saturation,
//    colorStandard).
//
//  Arguments:
//    outputVideoSurface
//       Only the following formats are supported:
//          TVMRSurfaceType_YV12
//          TVMRSurfaceType_YV16
//          TVMRSurfaceType_YV16_Transposed
//          TVMRSurfaceType_YV24
//    Rest of the arguments
//       Same as TVMRVideoMixerRender except that "curr" must exist or
//       an error is returned.

TVMRStatus
TVMRVideoMixerRenderYUV(
   TVMRVideoMixer *vmr,
   TVMRVideoSurface *outputVideoSurface,
   TVMRPictureStructure pictureStructure,
   TVMRVideoSurface *next,
   TVMRVideoSurface *curr,
   TVMRVideoSurface *prev,
   TVMRVideoSurface *prev2,
   TVMRRect *vidSrcRect,
   TVMRFence *fencesWait,  /* NULL terminated list */
   TVMRFence fenceDone  /* optional */
);


//  TVMRPaletteCreate
//
//  Create a 256 element ARGB palette suitable for use with
//  TVMROutputSurfacePutBitsIndexed.


TVMRPalette *
TVMRPaletteCreate (
   TVMRDevice *device
);

//  TVMRPaletteDestroy
//
//  Destroy the TVMRPalette.

void
TVMRPaletteDestroy(
  TVMRPalette *palette
);

//  TVMRPaletteLoad
//
//  Upload the palette data.
//
//  Arguments:
//    device - Required for the HW channel used for the upload.
//    palette - The TVMRPalette being updated.
//    rgba - A 256-element array of NvU32 RGBA values.   Each 32-bit
//    value has R, G, B, and A bytes packed from LSB to MSB.

void
TVMRPaletteLoad(
   TVMRDevice *device,
   TVMRPalette *palette,
   NvU32 *rgba
);

//  TVMROutputSurfacePutBitsIndexed
//
//  Update a subrectangle of the TVMROutputSurface by copying 8-bit
//  index data from a caller-provided pointer through a TVMRPalette.
//
//  Arguments:
//    device - Required for the HW channel used for the upload.
//    outSurf - The surface to be updated.
//    palette - The palette to use.
//    dstx, dsty, width, height - The rectangle on the output surface
//        to be modified.
//    srcData - A pointer to the source index data to be copied through
//       the palette.  This data is assumed to be:
//         max(srcBytePitch, width) * height
//       in size (width and height as passed to this function before
//       being clipped to the surface dimensions).
//    srcBytePitch - The byte stride between consecutive scanlines of
//       the source data.  This value is assumed to be >= width.
//    fencesWait - If not NULL, fencesWait is expected to be a NULL
//       terminated list of TVMRFences.  This function will delay
//       rendering until after all fences in the list have been reached.
//    fenceDone - If not NULL, fenceDone will be filled with the fence
//       indicating when this operation has completed.


void
TVMROutputSurfacePutBitsIndexed (
   TVMRDevice *device,
   TVMROutputSurface *outSurf,
   TVMRPalette *palette,
   NvU16 dstx,
   NvU16 dsty,
   NvU16 width,
   NvU16 height,
   unsigned char *srcData,
   NvU32 srcBytePitch,
   TVMRFence *fencesWait,  /* NULL terminated list */
   TVMRFence fenceDone  /* optional */
);

//  TVMROutputSurfaceRender
//
//  Draw a rectangle on the destination surface.  The general pipeline
//  is as follows:
//
//  1) The srcRect is extracted from the srcSurface.
//  2) The extracted source is rotated  0, 90, 180 or 270 degrees
//        according to the flags.
//  3) The rotated source is component-wise multiplied by a smooth-
//     shaded quad with a (potentially) different color at each vertex.
//  4) The result is scaled and rendered at the dstRect on the dstSurface.
//
//  Arguments:
//     device - Required for the HW channel used for rendering.
//     dstSurface - The surface to be rendered.
//     dstRect - The rectangle on the dstSurface to be modified.  If NULL,
//        a rectangle the full size of the dstSurface is implied.
//     srcSurface - The source surface to be used.  If NULL, a source
//        surface with components all 1.0 is implied.
//     srcRect - The rectangle from the source to be used.  If NULL, a
//        rectangle the full size of the srcSurface is implied.  This field
//        is ignored if srcSurface is NULL.
//     colors - A pointer to a single TVMRColor, an array of four TVMRColors,
//        or NULL.  If TVMR_RENDER_FLAG_COLOR_PER_VERTEX is set, TVMR will
//        read an array of four colors and treat them as the colors
//        corresponding to the UpperLeft, UpperRight, LowerRight and
//        LowerLeft corners of the post-rotated source (ie. indices
//        0, 1, 2 and 3 run clockwise from the upper left corner).  If
//        TVMR_RENDER_FLAG_COLOR_PER_VERTEX is not set, NVVA will use the
//        single TVMRColor for all four corners.  If colors is NULL then
//        red, green, blue and alpha values of 1.0 will be used.
//    flags - Any one of the following:
//           TVMR_RENDER_FLAG_ROTATE_0
//           TVMR_RENDER_FLAG_ROTATE_90
//           TVMR_RENDER_FLAG_ROTATE_180
//           TVMR_RENDER_FLAG_ROTATE_270
//     and optionally OR in either or both of the following:
//           TVMR_RENDER_FLAG_FLIP_HORIZONTAL
//           TVMR_RENDER_FLAG_FLIP_VERTICAL
//
//       Specify a clockwise rotation of the source in degrees.
//       Horizontal and vertical flipping can be specified via these
//       flags as well as by swapping source coordinates.  Flipping is
//       performed before rotating.
//
//       may be OR'd together with:
//           TVMR_RENDER_FLAG_COLOR_PER_VERTEX
//       If set, the colors array holds 4 colors rather than one.
//    fencesWait - If not NULL, fencesWait is expected to be a NULL
//       terminated list of TVMRFences.  This function will delay
//       rendering until after all fences in the list have been reached.
//    fenceDone - If not NULL, fenceDone will be filled with the fence
//       indicating when this operation has completed.


void
TVMROutputSurfaceRender (
   TVMRDevice *device,
   TVMROutputSurface *dstSurface,
   TVMRRect *dstRect,
   TVMROutputSurface *srcSurface,
   TVMRRect *srcRect,
   TVMRColor *colors,
   NvU32 flags,
   TVMRFence *fencesWait,  /* NULL terminated list */
   TVMRFence fenceDone  /* optional */
);

// TVMROutputSurfaceToVideoSurface
//
// Copy a rectangle from an output surface to a rectangle on a video surface.
//
//  Arguments:
//  device - Required to provide the HW channel used for rendering.
//  dstSurface - The video surface to be rendered.  Presently, only the
//    following surface types are supported:
//      TVMRSurfaceType_YV12
//      TVMRSurfaceType_YV16
//      TVMRSurfaceType_YV16_Transposed
//      TVMRSurfaceType_YV24
//  dstRect - The rectangle on the dstSurface to be modified.  If NULL,
//    a rectangle the full size of the dstSurface is implied.  If this
//    rectangle is not well-formed (x1 >= x0 && y1 >= y0) nothing will be
//    drawn.
//  srcSurface - The output surface to copy from.
//  srcRect - The rectangle on the srcSurface to copy from.  If NULL, a
//    rectangle the full size of the srcSurface is implied.  If the rectangle
//    specifies pixels from outside of the surface, pixels from the nearest
//    edge will be used instead.  Horizontal and/or vertical flipping of the
//    source can be accomplished by swapping the left/right and/or top/bottom
//    coordinates of the rectangle.
//  colorStandard - One of the following:
//    TVMR_COLOR_STANDARD_ITUR_BT_601
//    TVMR_COLOR_STANDARD_ITUR_BT_709
//    TVMR_COLOR_STANDARD_ITUR_BT_601_ER
//  fencesWait - If not NULL, fencesWait is expected to be a NULL
//    terminated list of TVMRFences.  TVMROutputSurfaceToVideoSurface
//    will delay rendering until after all fences in the list have been
//    reached.
//  fenceDone - It is always guaranteed that TVMROutputSurfaceToVideoSurface
//    will complete in a finite amount of time.  If not NULL, fenceDone
//    will be filled with a fence indicating when the operation has
//    completed.

TVMRStatus
TVMROutputSurfaceToVideoSurface (
   TVMRDevice *device,
   TVMRVideoSurface *dstSurface,
   TVMRRect *dstRect,
   TVMROutputSurface *srcSurface,
   TVMRRect *srcRect,
   TVMRColorStandard colorStandard,
   TVMRFence *fencesWait,  /* NULL terminated list */
   TVMRFence fenceDone  /* optional */
);

//  TVMROutputSurfaceCompositeBluray
//
//  Composite together background, presentation and interactive
//  graphics planes.  This function will be equivalent to the following
//  sequence:
//   1) If backgroundSurface is not NULL, the backgroundSrcRect is scaled
//      to the dstSurface size and copied to the dstSurface.  If
//      backgroundSurface is NULL, the dstSurface is filled with the
//      backgroundColor.
//   2) If videoPresent==NV_TRUE the area on the dstSurface described by
//      vidRect is made transparent, punching a hole through the background.
//   3) If graphics0 is not NULL, the area on that surface described by
//      srcRect0 is scaled to the dstSurface size and composited on top
//      of the dstSurface.
//   3) If graphics1 is not NULL, the area on that surface described by
//      srcRect1 is scaled to the dstSurface size and composited on top
//      of the dstSurface.
//
//   The output of TVMROutputSurfaceCompositeBluray will always produce
//   premultiplied alpha.
//
//  Arguments:
//    device - Required for the HW channel used for rendering.
//    dstSurface - The output surface to be rendered.
//    backgroundSurface -  If not NULL, the background surface to be used.
//    backgroundSrcRect - Valid only if backgroundSurface is not NULL, the
//      portion the background surface that will be scaled to the dstSurface
//      size.  If backgroundSrcRect is NULL, a rectangle the full size of
//      the backgroundSurface is implied.
//    backgroundColor - Valid only if backgroundSurface is not NULL, the
//      color with which to fill the dstSurface.  If backgroundColor is
//      NULL, a color of ARGB = {1.0, 1.0, 1.0, 1.0} is implied.
//    videoPresent  - If NV_TRUE, the area described by vidRect on the
//      dstSurface will be made transparent.
//    vidRect - Valid only if videoPresent==NV_TRUE.  If vidRect is NULL,
//      a rectangle the full size of the dstSurface is implied.
//    graphics0 - If not NULL, the rectangle described by srcRect0 will be
//      scaled to the dstSurface size and blended on top of the dstSurface.
//    srcRect0 - Valid only if graphics0 is not NULL.  If srcRect0 is NULL,
//      a rectangle the size of graphics0 is implied.
//    graphics1 - If not NULL, the rectangle described by srcRect1 will be
//      scaled to the dstSurface size and blended on top of the dstSurface.
//    srcRect1 - Valid only if graphics1 is not NULL.  If srcRect1 is NULL,
//      a rectangle the size of graphics1 is implied.
//    premult0, premult1 - specifies that graphics0 and graphics1 respectively
//      have had their color components premultiplied by alpha.
//    fencesWait - If not NULL, fencesWait is expected to be a NULL
//      terminated list of TVMRFences.  This function will delay
//      rendering until after all fences in the list have been reached.
//    fenceDone - If not NULL, fenceDone will be filled with the fence
//      indicating when this operation has completed.
//    yInvert - If set, the output will be mirrored vertically.



void
TVMROutputSurfaceCompositeBluray (
   TVMRDevice *device,
   TVMROutputSurface *dstSurface,
   TVMROutputSurface *backgroundSurface,
   TVMRRect *backgroundSrcRect,
   TVMRColor *backgroundColor,
   NvBool videoPresent,
   TVMRRect *vidRect,
   TVMROutputSurface *graphics0,
   NvBool premult0,
   TVMRRect *srcRect0,
   TVMROutputSurface *graphics1,
   NvBool premult1,
   TVMRRect *srcRect1,
   TVMRFence *fencesWait,  /* NULL terminated list */
   TVMRFence fenceDone, /* optional */
   NvBool yInvert
);

//  TVMROutputSurfaceCreate
//
//  Allocate a 32-bit RGBA output surface.  Upon creation, the contents
//  are in an undefined state.
//
//  Arguments:
//    device - The device's resource manager connection is used for the
//             allocation.
//    width, height - Maximum size is 2048 x 2048 pixels.
//    map - If non-zero, the TVMROutputSurface.surface->mapping will point
//          to a CPU-addressable mapping of the surface.

TVMROutputSurface *
TVMROutputSurfaceCreate (
   TVMRDevice *device,
   NvU16 width,
   NvU16 height,
   NvBool map
);

//  TVMROutputSurfaceDestroy
//
//  Destroy the TVMROutputSurface.

void
TVMROutputSurfaceDestroy (
   TVMROutputSurface *outputSurf
);

//  TVMRFenceCreate
//
//  Create a TVMRFence.   A TVMRFence is an NvRmFence*.  TVMRFenceCreate
//  creates a NvRmFence, initializes it and returns a pointer to it.
//  This function exists only so that the caller does not need to
//  be aware of NvRm types.

TVMRFence TVMRFenceCreate (void);

//  TVMRFenceDestroy
//
//  Destroy the TVMRFence.

void TVMRFenceDestroy(TVMRFence fence);


//  TVMRFenceBlock
//  TVMRFenceBlockTimeout
//
//  Block until the TVMRFence has been reached.  TVMRFenceBlockTimeout
//  is the same as TVMRFenceBlock but with a millisecond timeout.
//  TVMRFenceBlockTimeout returns 0 if it times out, 1 otherwise.
//  NOTE: we should change this to return a TVMRStatus.

void TVMRFenceBlock(TVMRDevice *device, TVMRFence fence);

int
TVMRFenceBlockTimeout(
   TVMRDevice *device,
   TVMRFence fence,
   TVMRTime millisecondTimeout);

//  TVMRFenceCopy
//
//  Copy the TVMRFence.  This functions exists only so that the caller
//  does not need to be aware of NvRm types.

void TVMRFenceCopy(TVMRFence dest, TVMRFence src);

// TVMRVideoSurfaceCreate
//
//  Allocate a TVMRVideoSurface.  Upon creation, the contents are in an
//  undefined state.
//
//  Arguments:
//    device - The device's resource manager connection is used for the
//             allocation.
//    width, height - Maximum size is 2048 x 2048 pixels (luma dimensions).
//    type - The supported types are:
//            TVMRSurfaceType_YV12
//            TVMRSurfaceType_YV16
//            TVMRSurfaceType_YV16_Transposed
//            TVMRSurfaceType_YV24
//            TVMRSurfaceType_NV12
//            TVMRSurfaceType_YV16x2


TVMRVideoSurface *
TVMRVideoSurfaceCreate (
   TVMRDevice *device,
   TVMRSurfaceType type,
   NvU16 width,
   NvU16 height,
   NvBool map
);

// TVMRVideoSurfaceDestroy
//
//  Destroy the TVMRVideoSurface

void TVMRVideoSurfaceDestroy(TVMRVideoSurface *videoSurf);

// TVMRVideoSurfaceCompositeBluray
//
//  This operation is equivalent to the following sequence:
//  1) Overwrite the full "destination" surface with "primarySrcRect"
//     from the "primary" surface scaled up to the full size of the
//     "destination" surface.
//  2) If the "secondary" surface is not NULL, scale "secondarySrcRect"
//     from the "secondary" surface and composite at the location on
//     the "destination" surface described by "secondaryDstRect".  If
//     lumaKeyEnable==NV_TRUE, "secondary" luma pixels less than or
//     equal to luma_key_upper_limit will be treated as transparent.
//     Otherwise, the "secondary" surface is considered to be fully
//     opaque.
//
//  Arguments:
//    device - Required for the HW channel used for rendering.
//    destination - The video surface to be rendered.  At this time
//       only surfaces of type TVMRSurfaceType_YV12 are supported.
//    primary - The primary video surface.  At this time only surfaces
//       of type TVMRSurfaceType_YV12 are supported.
//    primarySrcRect - The rectangle from the primary surface to be
//       scaled.  If NULL, a rectangle the full size of the primary surface
//       is implied.
//    secondary - If not NULL, the secondary surface to be composited
//       on top of the primary surface in the destination.  At this time
//       only surfaces of type TVMRSurfaceType_YV12 are supported.
//    secondarySrcRect - The rectangle from the secondary surface to
//       be scaled.  If NULL, a rectangle the full size of the primary
//       surface is implied.
//    secondaryDstRect - The location on the destination where the
//       secondary surface is to be composited.  If NULL, a rectangle the
//       full size of the destination surface is implied.
//    lumaKeyEnable, luma_key_upper_limit -  If lumaKeyEnable==NV_TRUE,
//       secondary luminance pixels less than or equal to luma_key_upper_limit
//       will be considered transparent.  They are opaque otherwise.
//    fencesWait - If not NULL, fencesWait is expected to be a NULL
//       terminated list of TVMRFences.  This function will delay
//       rendering until after all fences in the list have been reached.
//    fenceDone - If not NULL, fenceDone will be filled with the fence
//       indicating when this operation has completed.


TVMRStatus
TVMRVideoSurfaceCompositeBluray (
   TVMRDevice *device,
   TVMRVideoSurface *destination,
   TVMRVideoSurface *primary,
   TVMRRect *primarySrcRect,
   TVMRVideoSurface *secondary,
   TVMRRect *secondarySrcRect,
   TVMRRect *secondaryDstRect,
   NvBool lumaKeyEnable,
   NvU8 luma_key_upper_limit,
   TVMRFence *fencesWait,  /* NULL terminated list */
   TVMRFence fenceDone  /* optional */
);

//  TVMRCaptureCreate
//
//   Create a capture object.  It does not require a TVMRDevice
//  because it is completely independent from the TVMRDevice and
//  may be accessed by a different thread.  TVMRCaptureCreate()
//  will return NULL if there was an allocation error.
//
//   format - Any one of the following:
//           TVMR_CAPTURE_FORMAT_VIP_NTSC
//           TVMR_CAPTURE_FORMAT_VIP_PAL
//           TVMR_CAPTURE_FORMAT_CSIA_1440_540
//           TVMR_CAPTURE_FORMAT_CSIA_WVGA
//           TVMR_CAPTURE_FORMAT_CSIA_1024_480
//           TVMR_CAPTURE_FORMAT_CSIA_1440_542
//           TVMR_CAPTURE_FORMAT_CSIA_800x482
//           TVMR_CAPTURE_FORMAT_CSIA_1024_482
//
//   surfaceType -
//      Must be TVMRSurfaceType_YV16x2 for TVMR_CAPTURE_FORMAT_VIP_*,
//      or TVMRSurfaceType_R8G8B8A8 for TVMR_CAPTURE_FORMAT_CSIA_*.
//
//   numBuffers -
//      Number of frame buffers used in the internal ring.  Must be at
//      least TVMR_MIN_CAPTURE_FRAME_BUFFERS and no more than
//      TVMR_MAX_CAPTURE_FRAME_BUFFERS.
//
//   map -
//      If the TVMRVideoSurfaces returned by TVMRCaptureGetFrame
//      should have CPU mappings.
TVMRCapture *
TVMRCaptureCreate (
   TVMRCaptureFormat format,
   TVMRSurfaceType surfaceType,
   NvU8 numBuffers,
   NvBool map
);

//  TVMRCaptureDestroy
//
//   Destroy a capture object.
void TVMRCaptureDestroy (TVMRCapture *capture);

// TVMRCaptureListOutputSurfaces
//
//   Get output surfaces used in TVMRCapture. If the surfaceType passed to
// TVMRCaptureCreate() is not TVMRSurfaceType_R8G8B8A8, this function returns
// TVMR_STATUS_UNSUPPORTED.
//
//  Arguments:
//    capture - TVMRCapture* returned by TVMRCaptureCreate(). If capture is
//       NULL, this function returns TVMR_STATUS_BAD_PARAMETER.
//    surfaces - Array of TVMROutputSurface* to store the TVMROutputSurface
//       pointers used for capturing frames. If surfaces is NULL, this
//       function returns TVMR_STATUS_BAD_PARAMETER.
//    countInOut - a pointer to the maximum count to store TVMROutputSurface*
//       in surfaces. When the dereferenced value of countInOut is greater
//       than the numBuffers passed to TVMRCaptureCreate(), the value of
//       numBuffers will be stored in countInOut. If countInOut is NULL,
//       this function returns TVMR_STATUS_BAD_PARAMETER.
TVMRStatus
TVMRCaptureListOutputSurfaces (
   TVMRCapture *capture,
   TVMROutputSurface *surfaces[],
   NvU8 *countInOut
);

//  TVMRCaptureStart
//
//   Start capturing
void TVMRCaptureStart(TVMRCapture *capture);

//  TVMRCaptureStop
//
//   Stop capturing
void TVMRCaptureStop(TVMRCapture *capture);

// TVMRCaptureGetFrameYUV
//
//   This function will block until a frame (two buffers) is available
// or until the timeout (in milliseconds) has been reached.  To block
// without a timeout specify TVMR_TIMEOUT_INFINITE.
// The returned TVMRVideoSurface should be passed back to the TVMRCapture
// object using TVMRCaptureReturnFrameYUV() after it has been processed.
// TVMRCaptureGetFrameYUV() returns NULL if capture is not running (was
// stopped or never started) or if there are insufficient buffers in
// the internal pool, meaning that too few TVMRVideoSurfaces have
// been returned to the capture object.  When TVMRCaptureGetFrameYUV()
// returns a TVMRVideoSurface, that surface is idle and ready
// for immediate use.  Each TVMRVideoSurface returned by
// TVMRCaptureGetFrameYUV() corresponds to two buffers removed from the
// internal capture pool.  There must be at least two buffers left
// in the pool for TVMRCaptureGetFrameYUV() to succeed, so if one
// allocated the TVMRCapture object with only two buffers, the
// TVMRVideoSurface returned by TVMRCaptureGetFrame() must be
// returned via TVMRCaptureReturnFrameYUV() before TVMRCaptureGetFrameYUV()
// can succeed again.
TVMRVideoSurface *
TVMRCaptureGetFrameYUV(TVMRCapture *capture, TVMRTime millisecondTimeout);

// TVMRCaptureGetFrameRGB
//
//   This function will block until a frame (two buffers) is available
// or until the timeout (in milliseconds) has been reached.  To block
// without a timeout specify TVMR_TIMEOUT_INFINITE.
// The returned TVMROutputSurface should be passed back to the TVMRCapture
// object using TVMRCaptureReturnFrameRGB() after it has been processed.
// TVMRCaptureGetFrameRGB() returns NULL if capture is not running (was
// stopped or never started) or if there are insufficient buffers in
// the internal pool, meaning that too few TVMROutputSurfaces have
// been returned to the capture object.  When TVMRCaptureGetFrameRGB()
// returns a TVMROutputSurface, that surface is idle and ready
// for immediate use.  Each TVMROutputSurface returned by
// TVMRCaptureGetFrameRGB() corresponds to two buffers removed from the
// internal capture pool.  There must be at least two buffers left
// in the pool for TVMRCaptureGetFrameRGB() to succeed, so if one
// allocated the TVMRCapture object with only two buffers, the
// TVMROutputSurface returned by TVMRCaptureGetFrame() must be
// returned via TVMRCaptureReturnFrameRGB() before TVMRCaptureGetFrameRGB()
// can succeed again.
TVMROutputSurface *
TVMRCaptureGetFrameRGB(TVMRCapture *capture, TVMRTime millisecondTimeout);

// TVMRCaptureReturnFrameYUV
//
//    Return a surface back to the capture pool.  If fenceDone is not
// NULL, TVMR will wait for the fence before capturing a new frame
// into it.  This fence should reflect when post-capture processing
// on that TVMRVideoSurface will be finished.  TVMRCaptureReturnFrameYUV()
// returns an error if the surface passed is not from the TVMRCapture
// pool.
TVMRStatus
TVMRCaptureReturnFrameYUV (
   TVMRCapture *capture,
   TVMRVideoSurface *surf,
   TVMRFence fenceDone
);

// TVMRCaptureReturnFrameRGB
//
//    Return a surface back to the capture pool.  If fenceDone is not
// NULL, TVMR will wait for the fence before capturing a new frame
// into it.  This fence should reflect when post-capture processing
// on that TVMROutputSurface will be finished.  TVMRCaptureReturnFrameRGB()
// returns an error if the surface passed is not from the TVMRCapture
// pool.
TVMRStatus
TVMRCaptureReturnFrameRGB (
   TVMRCapture *capture,
   TVMROutputSurface *surf,
   TVMRFence fenceDone
);

TVMRCapture *
TVMRCaptureCreateCSI(
   TVMRCaptureCSISetting *setting,
   TVMRSurfaceType surfaceType,
   NvU8 numBuffers,
   NvBool map
);

// obsolete
TVMRVideoSurface *
TVMRCaptureGetFrame(TVMRCapture *capture, TVMRTime millisecondTimeout);

// obsolete
TVMRStatus
TVMRCaptureReturnFrame (
   TVMRCapture *capture,
   TVMRVideoSurface *surf,
   TVMRFence fenceDone
);


// TVMRVideoEncoderCreate
//
//   Create an encoder object capable of turning a stream of surfaces
// of the "inputFormat" into a bitstream of the specified "codec".
// Surfaces are feed to the encoder with TVMRVideoEncoderFeedFrameRGB/YUV()
// and bitstream buffers are retrieved with TVMRVideoEncoderGetBits().
//
//   codec
//      Currently only TVMR_VIDEO_CODEC_H264 is supported.
//
//   mbWidth, mbHeight
//      The dimensions of the output stream in macroblocks.
//
//   inputFormat
//      May be one of the following:
//         TVMRSurfaceType_R8G8B8A8 (use TVMRVideoEncoderFeedFrameRGB)
//         TVMRSurfaceType_YV12 (use TVMRVideoEncoderFeedFrameYUV)
//
//   maxInputBuffering
//      This determines how many frames may be in the encode pipeline
//      at any time.  For example, if maxInputBuffering==1,
//      TVMRVideoEncoderFeedFrameRGB() will block until the previous
//      encode has completed.  If maxInputBuffering==2,
//      TVMRVideoEncoderFeedFrameRGB() will accept one frame while another
//      is still being encoded by the hardware, but will block if two
//      are still being encoded.  maxInputBuffering will be clamped between
//      1 and 16.  This field is ignored for YUV inputs which don't require
//      a pre-processing pipeline before the encode hardware.
//
//   maxOutputBuffering
//      This determines how many frames of encoded bitstream can be held
//      by the TVMRVideoEncoder before it must be retrieved using
//      TVMRVideoEncoderGetBits().  This number should be greater than
//      or equal to maxInputBuffering and is clamped to between
//      maxInputBuffering and 16.  If maxOutputBuffering frames worth
//      of encoded bitstream are yet unretrived by TVMRVideoEncoderGetBits()
//      TVMRVideoEncoderFeedFrameRGB/YUV() will return
//      TVMR_STATUS_INSUFFICIENT_BUFFERING.  One or more frames need to
//      be retrived with TVMRVideoEncoderGetBits() before frame feeding
//      can continue.
//
//   attributes
//      Specify encode parameters.  If NULL, TVMR will use default
//      parameters.
//
//   optionalDevice
//      Should be NULL under normal circumstances.  If not NULL, TVMR will
//      use the 3D engine (specified by the TVMRDevice) instead of the 2D
//      engine for RGB to YUV conversions during the
//      TVMRVideoEncoderFeedFrameRGB() function.  In this case,
//      TVMRVideoEncoderFeedFrameRGB() is bound to the TVMRDevice; this
//      has thread saftey consequences.  For example, if the same TVMRDevice
//      is used for both the Encoder and VideoMixer, the two should be
//      called from the same thread.  If the Encoder and VideoMixer are
//      to be used in separate threads they should use different TVMRDevices.


TVMRVideoEncoder *
TVMRVideoEncoderCreate(
   TVMRVideoCodec codec,
   NvU16 mbWidth,
   NvU16 mbHeight,
   TVMRSurfaceType inputFormat,
   NvU8 maxInputBuffering,
   NvU8 maxOutputBuffering,
   TVMRVideoEncoderAttributes *attributes,
   TVMRDevice *optionalDevice
);

//  TVMRVideoEncoderDestroy
//
//  Destroy the video encoder

void TVMRVideoEncoderDestroy(TVMRVideoEncoder *encoder);

// TVMRVideoEncoderFeedFrameRGB
//
//    Encode the specified "frame".  TVMRVideoEncoderFeedFrameRGB()
//  returns TVMR_STATUS_INSUFFICIENT_BUFFERING if TVMRVideoEncoderGetBits()
//  has not been called frequently enough and the maximum internal
//  bitstream buffering (determined by "maxOutputBuffering" passed to
//  TVMRVideoEncoderCreate()) has been exhausted.
//
//   frame
//      This must be of the same sourceType as the TVMREncoder.
//      There is no limit on the size of this surface.  The
//      source rectangle specified by "sourceRect" will be scaled
//      to the stream dimensions.
//
//   sourceRect
//       This rectangle on the source will be scaled to the stream
//       dimensions.  If NULL, a rectangle the full size of the source
//       surface is implied.  The source may be flipped horizontally
//       or vertically by swapping the left and right or top and bottom
//       coordinates.
//
//   fencesWait
//       If not NULL, fencesWait is expected to be a NULL terminated
//       list of TVMRFences.  TVMRVideoEncoderFeedFrameRGB() will
//       delay encoding of this frame until all fences in the list
//       have been reached.
//
//   fenceDone
//       If not NULL, the TVMRFence will be filled out that indicates
//       when the input frame is no longer being read.
//
//   flags
//       The following flags are supported:
//
//       TVMR_VIDEO_ENCODER_FLAG_FORCE_I_FRAME
//          Normally, I-Frames are inserted only at the begining of each
//          GOP as determined by the framesPerGOP attribute.  This flag
//          can be used to force an I-Frame at any time.


TVMRStatus
TVMRVideoEncoderFeedFrameRGB (
   TVMRVideoEncoder *encoder,
   TVMROutputSurface *frame,
   TVMRRect *sourceRect,
   TVMRFence *fencesWait,
   TVMRFence fenceDone,
   NvU32 flags
);

// TVMRVideoEncoderFeedFrameYUV
//
//   The same as TVMRVideoEncoderFeedFrameRGB() except that a source
//   rectangle is not supported.

TVMRStatus
TVMRVideoEncoderFeedFrameYUV (
   TVMRVideoEncoder *encoder,
   TVMRVideoSurface *frame,
   TVMRFence *fencesWait,
   TVMRFence fenceDone,
   NvU32 flags
);


//  TVMRVideoEncoderSetAttributes
//
//    These go into effect only at the start of the next GOP

void
TVMRVideoEncoderSetAttributes (
   TVMRVideoEncoder *encoder,
   TVMRVideoEncoderAttributes *attributes
);


//  TVMRVideoEncoderGetBits
//
//   TVMRVideoEncoderGetBits() returns a frame's worth of bitstream
//  into the provided "buffer".  "numBytes" returns the size of this
//  bitstream.  It is safe to call this function from a separate thread.
//  The return value and behavior is the same as that of
//  TVMRVideoEncoderBitsAvailable() when called with TVMR_BLOCKING_TYPE_NEVER
//  except that when TVMR_STATUS_OK is returned, the "buffer" will be
//  filled in addition to the "numBytes".

TVMRStatus
TVMRVideoEncoderGetBits (
   TVMRVideoEncoder *encoder,
   NvU32 *numBytes,
   void *buffer
);


//  TVMRVideoEncoderBitsAvailable
//
//    TVMRVideoEncoderBitsAvailable() returns the encode status
//  and number of bytes available for the next frame (if any).
//  The specific behavior depends on the specified TVMRBlockingType.
//  It is safe to call this function from a separate thread.
//  Possible return values are:
//
//   TVMR_STATUS_OK - bits are available and the size is returned
//                     in "numBytesAvailable".
//   TVMR_STATUS_PENDING - an encode is in progress but not yet completed.
//   TVMR_STATUS_NONE_PENDING - no encode is in progress.
//   TVMR_STATUS_TIMED_OUT - the operation timed out.
//
//  Arguments:
//
//   numBytesAvailable
//      The number of bytes available in the next encoded frame.  This
//      is valid only when the return value is TVMR_STATUS_OK.
//
//   blockingType
//      The following are supported blocking types:
//
//    TVMR_BLOCKING_TYPE_NEVER
//        This type never blocks so "millisecondTimeout" is ignored.
//        The following are possible return values:  TVMR_STATUS_OK
//        TVMR_STATUS_PENDING or TVMR_STATUS_NONE_PENDING.
//
//    TVMR_BLOCKING_TYPE_IF_PENDING
//        Same as TVMR_BLOCKING_TYPE_NEVER except that TVMR_STATUS_PENDING
//        will never be returned.  If an encode is pending this function
//        will block until the status changes to TVMR_STATUS_OK or until the
//        timeout has occurred.  Possible return values: TVMR_STATUS_OK,
//        TVMR_STATUS_NONE_PENDING, TVMR_STATUS_TIMED_OUT.
//
//    TVMR_BLOCKING_TYPE_ALWAYS
//         This function will return only TVMR_STATUS_OK or
//         TVMR_STATUS_TIMED_OUT.  It will block until those conditions.
//
//   millisecondTimeout
//       Timeout in milliseconds or TVMR_TIMEOUT_INFINITE if a timeout
//       is not desired.


TVMRStatus
TVMRVideoEncoderBitsAvailable (
   TVMRVideoEncoder *encoder,
   NvU32 *numBytesAvailable,
   TVMRBlockingType blockingType,
   TVMRTime millisecondTimeout
);


//  TVMRVideoDecoderCreate
//
//  Create a TVMRVideoDecoder object for the specified codec.  Each
//  decoder object may be accessed by a separate thread.  The object
//  is to be destroyed with TVMRVideoDecoderDestroy().  All surfaces
//  used with the TVMRVideoDecoder must be of type TVMRSurfaceType_YV12.
//
//  Arguments:
//
//    codec
//      The following are supported:
//        TVMR_VIDEO_CODEC_H264
//        TVMR_VIDEO_CODEC_H264_MVC
//        TVMR_VIDEO_CODEC_VC1
//        TVMR_VIDEO_CODEC_VC1_ADVANCED
//        TVMR_VIDEO_CODEC_MPEG1
//        TVMR_VIDEO_CODEC_MPEG2
//        TVMR_VIDEO_CODEC_MPEG4
//        TVMR_VIDEO_CODEC_MJPEG
//
//    width, height
//       In luminance pixels.  The maximum size is 2048 x 2048.
//
//    maxReferences
//       The maximum number of reference frames used.  This will
//       limit internal allocations.
//
//    inputBuffering
//       How many frames can be in flight at any given time.  If this
//       value is one, TVMRVideoDecoderRender() will block until the
//       previous frame has finished decoding.  If this is two,
//       TVMRVideoDecoderRender() will block if two frames are pending
//       but will not block if one is pending.  This value is clamped
//       internally to between 1 and 8.


typedef struct {
   TVMRVideoCodec codec;
   NvU16 width;
   NvU16 height;
   NvU16 maxReferences;
} TVMRVideoDecoder;

TVMRVideoDecoder *
TVMRVideoDecoderCreate (
    TVMRVideoCodec codec,
    NvU16 width,
    NvU16 height,
    NvU16 maxReferences,
    NvU8 inputBuffering
);

void
TVMRVideoDecoderDestroy(
   TVMRVideoDecoder *decoder
);

//   TVMRVideoDecoderRender
//
//   Convert a picture's worth of bitstream, possibly split over multiple
//   buffers, into a decoded picture (field or frame).
//
//   Arguments:
//
//     target
//        The picture to be rendered.
//
//     pictureInfo
//        A pointer to a structure specific to the codec.  For H.264 this
//        would be the TVMRPictureInfoH264.
//
//     numBitstreamBuffers, bitstreams
//        The number of bitstream buffers and pointer to array of bitstream
//        buffer structures.  TVMRVideoDecoderRender() copies the data out
//        of these buffers so the caller is free to reuse them as soon as
//        TVMRVideoDecoderRender() returns.
//
//     fencesWait
//        If not NULL, fencesWait is expected to be a NULL-terminated list
//        of TVMRFences.  TVMRVideoDecoderRender() will delay decode until
//        all fences in the list have been reached.  Typically, this will
//        hold one fence indicating when it's safe to overwrite the target
//        surface (when it is no longer being read).
//
//     fenceDone
//        It is always guaranteed that TVMRVideoDecoderRender() will complete
//        in a finite amount of time.  If not NULL, fenceDone will be filled
//        in with a fence indicating when the decode is finished and it's
//        safe to read from the target surface or overwrite any references.


typedef struct {
   NvU8 *bitstream;
   NvU32 bitstreamBytes;
} TVMRBitstreamBuffer;


TVMRStatus
TVMRVideoDecoderRender(
    TVMRVideoDecoder *decoder,
    TVMRVideoSurface *target,
    const void *pictureInfo,
    NvU32 numBitstreamBuffers,
    const TVMRBitstreamBuffer *bitstreams,
   TVMRFence *fencesWait,  /* NULL terminated list */
   TVMRFence fenceDone  /* optional */
);

//  TVMRVideoDecoderCopySliceData
//
//  This function is intended for use in low-latency decode mode.
//  It is implemented only for H264 decoder. Error will be returned if it is
//  called for any other codec.
//
//  Each set of buffers should contain exactly 1 slice data.
//  For first slice of every frame, TVMRVideoDecoderRender() function should be called.
//  TVMRVideoDecoderCopySliceData() function should be called for all subsequent
//  slices of the frame.
//
//  Note that the ucode expects next slice data to be available within certain
//  time (= 100msec). If data is not available within this time, it is assumed that
//  the data is lost and error-concealment may be performed on the remaining portion
//  of the frame.
//
//  Arguments:
//     decoder: Private context of the decoder
//
//     numBitstreamBuffers, bitstreams
//        The number of bitstream buffers and pointer to array of bitstream
//        buffer structures.  TVMRVideoDecoderCopySliceData() copies the data
//        out of these buffers so the caller is free to reuse them as soon as
//        TVMRVideoDecoderCopySliceData() returns.
//

TVMRStatus
TVMRVideoDecoderCopySliceData (
    TVMRVideoDecoder *decoder,
    NvU32 numBitstreamBuffers,
    const TVMRBitstreamBuffer *bitstreams
);


//  TVMRVideoDecrypterCreate
//
//  Create a TVMRVideoDecrypter object for the specificed codec.
//  TVMRVideoDecrypter is to extract/parse headers (i.e, slice header and/or SPS, PPS)
//  from encrypted bitstream. Clients should use TVMRVideoDecryptHeader()
//  to request underlying HW to decrypt and parse header info, and should use
//  TVMRVideoGetHeader() in pair to retrieve the parsed header info from
//  HW. TVMRVideoDecryptHeader() and TVMRVideoGetHeader() could be called
//  in separate threads.
//
//  Arguments:
//
//    codec
//      The following are supported:
//        TVMR_VIDEO_CODEC_H264
//
//    maxBitstreamSize
//      Maximum size of compressed bitstream buffer size. Clamped betweend 64 bytes
//      and 2 MB internally.
//
//    inputBuffering
//      This will determine how many parsed headers can be in flight at
//      any given time. TVMRVideoDecryptHeader will be blocked once the
//      number of pending buffers reached to the given 'inputBuffering'.
//      TVMRVideoGetHeader will open up slots. This value is clamped
//      between 1 and 8 internally.

typedef struct {
    TVMRVideoCodec codec;
    NvU8 inputBuffering;
} TVMRVideoDecrypter;

TVMRVideoDecrypter *
TVMRVideoDecrypterCreate (
    TVMRVideoCodec codec,
    NvU32 maxBitstreamSize,
    NvU8 inputBuffering
);

void
TVMRVideoDecrypterDestroy(
    TVMRVideoDecrypter *decrypter
);

//  TVMRVideoDecryptHeader
//
//  Request decrypting/parsing header (slice header and/or SPS and PPS) to HW. Results will
//  be stored internal buffers, and should be pulled off by TVMRVideoGetHeader()
//
//  Arguments:
//
//    pictureInfo
//      A pointer to the structure containing necessary info to parse headers
//
//    bitstream
//      A pointer to a bitstream buffer
//
//    fenceDone
//      It is always guaranteed that TVMRVideoDecryptHeader() will complete
//      in a finite amount of time.  If not NULL, fenceDone will be filled
//      in with a fence indicating when the decode is finished and it's
//      safe to read from the target surface or overwrite any references.

TVMRStatus
TVMRVideoDecryptHeader(
   TVMRVideoDecrypter *decrypter,
   const void *pictureInfo,
   const TVMRBitstreamBuffer *bitstream,
   TVMRFence fenceDone  /* optional */
);

//  TVMRVideoGetHeader
//
//  Retrieve parsed header info. This is a blocking call so that it's guaranteed
//  that outputs will contain complete content after this call returns.
//  The input encrypted bitstream packet should contain slice header, and may contain
//  SPS and PPS. 'NAL_present_flag' indicates which NAL units were contained in input packet.
//  This call should be used with TVMRVideoDecryptHeader() as a pair.
//
//  Arguments:
//
//    slhInfo
//      A pointer to the structure which will store decrypted & parsed SLH
//
//    seqParamInfo
//      A pointer to the structure which will store decyrpted & parsed sequence parameters
//
//    picParamInfo
//      A pointer to the structure which will store decrypted & parser picture parameters
//
//    NAL_present_flag
//      Bit field flag indicating which types of NAL is present in the decrypted packet
//

TVMRStatus
TVMRVideoGetHeader(
        TVMRVideoDecrypter *decrypter,
        void *slhInfo,
        void *seqParamInfo,
        void *picParamInfo,
        NvU8 *NAL_present_flag
);

/**************** H.264 SPS & PPS info *******************/
typedef struct {
    NvU8 scaling_matrix_present_flag;
    NvU8 scaling_list_type[8]; // scaling_list_type_e
    NvU8 ScalingList4x4[6][16];
    NvU8 ScalingList8x8[2][64];
}TVMRscalingH264_s;

typedef struct {
    NvU8 cpb_cnt_minus1;
    NvU32 bit_rate;
    NvU32 cbp_size;
}TVMRHRDparamH264_s;

typedef struct {
    NvU8 sar_width;
    NvU8 sar_height;
    NvU8 video_signal_type_present_flag;
    NvU8 video_format;
    NvU8 video_full_range_flag;
    NvU8 colour_description_present_flag;
    NvU8 colour_primaries;
    NvU8 transfer_characteristics;
    NvU8 matrix_coefficients;
    NvU8 timing_info_present_flag;
    NvU32 num_units_in_tick;
    NvU32 time_scale;
    NvU8 fixed_frame_rate_flag;
    NvU8 nal_hrd_parameters_present_flag;
    NvU8 vcl_hrd_parameters_present_flag;
    NvU8 pic_struct_present_flag;
    NvU8 initial_cpb_removal_delay_length;
    NvU8 cpb_removal_delay_length_minus1;
    NvU8 dpb_output_delay_length_minus1;
    NvU8 num_reorder_frames;
    NvU8 max_dec_frame_buffering;
    TVMRHRDparamH264_s nal_hrd;
    TVMRHRDparamH264_s vcl_hrd;
}TVMRVUIparamH264_s;

typedef struct {
    NvU8 seq_parameter_set_id;
    NvU8 profile_idc;
    NvU8 constraint_set_flags;
    NvU8 level_idc;
    NvU8 chroma_format_idc;
    NvU8 residual_colour_transform_flag;
    NvU8 bit_depth_luma_minus8;
    NvU8 bit_depth_chroma_minus8;
    NvU8 qpprime_y_zero_transform_bypass_flag;
    NvU8 log2_max_frame_num_minus4;
    NvU8 pic_order_cnt_type;
    NvU8 log2_max_pic_order_cnt_lsb_minus4;
    NvU8 delta_pic_order_always_zero_flag;
    NvS32 offset_for_non_ref_pic;
    NvS32 offset_for_top_to_bottom_field;
    NvU8 num_ref_frames_in_pic_order_cnt_cycle;
    NvU8 num_ref_frames;
    NvU8 gaps_in_frame_num_value_allowed_flag;
    NvU32 pic_width_in_mbs_minus1;
    NvU32 pic_height_in_map_units_minus1;
    NvU8 frame_mbs_only_flag;
    NvU8 mb_adaptive_frame_field_flag;
    NvU8 direct_8x8_inference_flag;
    NvU8 frame_cropping_flag;
    NvU32 frame_crop_left_offset;
    NvU32 frame_crop_right_offset;
    NvU32 frame_crop_top_offset;
    NvU32 frame_crop_bottom_offset;
    NvU8 vui_parameters_present_flag;
    NvS32 offset_for_ref_frame[255];
    TVMRscalingH264_s seq;
    TVMRVUIparamH264_s vui;
}TVMRSeqParamInfoH264;

typedef struct {
    NvU8 pic_parameter_set_id;
    NvU8 seq_parameter_set_id;
    NvU8 entropy_coding_mode_flag;
    NvU8 pic_order_present_flag;
    NvU8 num_slice_groups_minus1;
    NvU8 num_ref_idx_l0_active_minus1;
    NvU8 num_ref_idx_l1_active_minus1;
    NvU8 weighted_pred_flag;
    NvU8 weighted_bipred_idc;
    NvS8 pic_init_qp_minus26;
    NvS8 pic_init_qs_minus26;
    NvS8 chroma_qp_index_offset;
    NvS8 second_chroma_qp_index_offset;
    NvU8 deblocking_filter_control_present_flag;
    NvU8 constrained_intra_pred_flag;
    NvU8 redundant_pic_cnt_present_flag;
    NvU8 transform_8x8_mode_flag;
    TVMRscalingH264_s pic;
}TVMRPicParamInfoH264;

/**************** H.264 slice header info ****************/
typedef struct
{
    NvU32 memory_management_control_operation;
    union
    {
        NvU32 difference_of_pic_nums_minus1;
        NvU32 long_term_frame_idx;
    } mmco_ops;
}TVMRmmco_s;

typedef struct {
    NvU32 first_mb_in_slice;
    NvU32 slice_type;
    NvU32 frame_num;
    NvU32 pic_parameter_set_id;
    NvU32 idr_pic_id;
    NvU32 pic_order_cnt_lsb;
    NvS32 delta_pic_order_cnt_bottom;
    NvS32 delta_pic_order_cnt[2];
    NvU32 redundant_pic_cnt;

    NvU8 field_pic_flag;
    NvU8 bottom_field_flag;

    NvU8 direct_spatial_mv_pred_flag;
    NvU8 num_ref_idx_l0_active_minus1;
    NvU8 num_ref_idx_l1_active_minus1;

    NvU8 nal_ref_idc;
    NvU8 nal_unit_type;
    NvU8 IdrPicFlag;

    // access_unit_delimiter
    NvU32 primary_pic_type;
    // pic_timing
    NvU32 sei_pic_struct;
    NvU32 view_id;
    // FMO
    NvU32 slice_group_change_cycle;

    // dec_ref_pic_marking
    NvU8 no_output_of_prior_pics_flag;
    NvU8 long_term_reference_flag;
    NvU8 adaptive_ref_pic_marking_mode_flag;
    NvU8 mmco5; // derived value that indicates presence of an mmco equal to 5
    TVMRmmco_s mmco[72];  // MAX_MMCOS=72
}TVMRSliceHeaderInfoH264;

/**************** H.264 decoder picture info *************/

typedef struct {
  TVMRVideoSurface *surface;
  NvU8  is_long_term;        // used for long term reference
  NvU8  top_is_reference;    // top used as reference
  NvU8  bottom_is_reference; // bottom used as reference
  NvS32 FieldOrderCnt[2];    // [0]: top, [1]: bottom
  NvU16 FrameIdx;            // short or long term number or index
} TVMRReferenceFrameH264;

typedef struct {
  /* target surface parameters */
  NvS32 FieldOrderCnt[2];   // [0]: top, [1]: bottom
  NvU8 is_reference;        // target will be a reference later

  /* The following correspond to H.264 specification variables */
  NvU8 chroma_format_idc;
  NvU16 frame_num;
  NvU8 field_pic_flag;
  NvU8 bottom_field_flag;
  NvU8 num_ref_frames;
  NvU8 mb_adaptive_frame_field_flag;
  NvU8 constrained_intra_pred_flag;
  NvU8 weighted_pred_flag;
  NvU8 weighted_bipred_idc;
  NvU8 frame_mbs_only_flag;
  NvU8 transform_8x8_mode_flag;
  NvS8 chroma_qp_index_offset;
  NvS8 second_chroma_qp_index_offset;
  NvS8 pic_init_qp_minus26;
  NvU8 num_ref_idx_l0_active_minus1;
  NvU8 num_ref_idx_l1_active_minus1;
  NvU8 log2_max_frame_num_minus4;
  NvU8 pic_order_cnt_type;
  NvU8 log2_max_pic_order_cnt_lsb_minus4;
  NvU8 delta_pic_order_always_zero_flag;
  NvU8 direct_8x8_inference_flag;
  NvU8 entropy_coding_mode_flag;
  NvU8 pic_order_present_flag;
  NvU8 deblocking_filter_control_present_flag;
  NvU8 redundant_pic_cnt_present_flag;
  NvU8 num_slice_groups_minus1;
  NvU8 slice_group_map_type;
  NvU32 slice_group_change_rate_minus1;
  NvU8 *pMb2SliceGroupMap;
  NvU8 fmo_aso_enable;
  NvU8 scaling_matrix_present;

  /* H.264 4x4 and 8x8 scaling lists */
  NvU8 scaling_lists_4x4[6][16];  // in raster order
  NvU8 scaling_lists_8x8[2][64];  // in raster order

  TVMRReferenceFrameH264 referenceFrames[16];

  // MVC extension
  struct
  {
      NvU16 num_views_minus1;
      NvU16 view_id;
      NvU8 inter_view_flag;
      NvU8 num_inter_view_refs_l0;
      NvU8 num_inter_view_refs_l1;
      NvU8 MVCReserved8Bits;
      NvU16 InterViewRefsL0[16];
      NvU16 InterViewRefsL1[16];
  } mvcext;

  NvS32 last_sps_id;
  NvS32 last_pps_id;

  // AES encryption params
  NvBool         enableEncryption;
  TVMRDrmType    drmMode;
  NvU32          InitVector[4];
  NvU32          BytesOfEncryptedData;
  NvU32          NonAlignedOffset;

  //SXES mode enable
  NvBool sxes_enable;

  NvBool sxep_slice_mode;
  NvBool decode_next_slice;
} TVMRPictureInfoH264;


/**************** VC1 decoder picture info ******************/

typedef struct {
  TVMRVideoSurface *forwardReference;
  TVMRVideoSurface *backwardReference;
  NvU8 pictureType;     // I=0, P=1, B=3, BI=4  from 7.1.1.4
  NvU8 frameCodingMode; // Progressive=0,
                        // Frame-interlace = 2,
                        // Field-interlace = 3  from 7.1.1.15
  /* The following correspond to VC-1 specification variables */
  NvU8 postprocflag;    // 6.1.5
  NvU8 pulldown;        // 6.1.8
  NvU8 interlace;       // 6.1.9
  NvU8 tfcntrflag;      // 6.1.10
  NvU8 finterpflag;     // 6.1.11
  NvU8 psf;             // 6.1.3
  NvU8 dquant;          // 6.2.8
  NvU8 panscan_flag;    // 6.2.3
  NvU8 refdist_flag;    // 6.2.4
  NvU8 quantizer;       // 6.2.11
  NvU8 extended_mv;     // 6.2.7
  NvU8 extended_dmv;    // 6.2.14
  NvU8 overlap;         // 6.2.10
  NvU8 vstransform;     // 6.2.9
  NvU8 loopfilter;      // 6.2.5
  NvU8 fastuvmc;        // 6.2.6
  NvU8 range_mapy_flag;  // 6.2.15
  NvU8 range_mapy;
  NvU8 range_mapuv_flag; // 6.2.16
  NvU8 range_mapuv;
  /* The following are for simple and main profiles only */
  NvU8 multires;        // J.1.10
  NvU8 syncmarker;      // J.1.16
  NvU8 rangered;        // J.1.17
  NvU8 rangeredfrm;     // 7.1.13
  NvU8 maxbframes;      // J.1.17
} TVMRPictureInfoVC1;

/************ MPEG1/2 decoder picture info *****************/

typedef struct {
  TVMRVideoSurface *forwardReference;
  TVMRVideoSurface *backwardReference;
  /* The following correspond to MPEG1/2 specification variables */
  NvU8 picture_structure;
  NvU8 picture_coding_type;
  NvU8 intra_dc_precision;
  NvU8 frame_pred_frame_dct;
  NvU8 concealment_motion_vectors;
  NvU8 intra_vlc_format;
  NvU8 alternate_scan;
  NvU8 q_scale_type;
  NvU8 top_field_first;
  NvU8 full_pel_forward_vector;  // MPEG-1 only.  Set 0 for MPEG-2
  NvU8 full_pel_backward_vector; // MPEG-1 only.  Set 0 for MPEG-2
  NvU8 f_code[2][2];  // for MPEG-1 fill both horiz. & vert.
  NvU8 intra_quantizer_matrix[64];
  NvU8 non_intra_quantizer_matrix[64];
} TVMRPictureInfoMPEG;

/************ MPEG4 decoder picture info ************/

typedef struct {
   TVMRVideoSurface *forwardReference;
   TVMRVideoSurface *backwardReference;
   /* The following correspond to MPEG4 specification variables */
   NvS32 TRD[2];   // 7.7.2.2
   NvS32 TRB[2];   // 7.7.2.2
   NvU16 vop_time_increment_resolution;  // 6.3.3
   NvU8 vop_coding_type;        // 6.3.5
   NvU8 vop_fcode_forward;      // 6.3.5
   NvU8 vop_fcode_backward;     // 6.3.5
   NvU8 resync_marker_disable;  // 6.3.3
   NvU8 interlaced;             // 6.3.3
   NvU8 quant_type;             // 6.3.3
   NvU8 quarter_sample;         // 6.3.3
   NvU8 short_video_header;     // 6.3.3
   NvU8 rounding_control;      // derived from vop_rounding_type 6.3.5
   NvU8 alternate_vertical_scan_flag;  // 6.3.5
   NvU8 top_field_first;        // 6.3.5
   NvU8 intra_quant_mat[64];    // 6.3.3
   NvU8 nonintra_quant_mat[64]; // 6.3.3
   NvU8 data_partitioned;
   NvU8 reversible_vlc;
} TVMRPictureInfoMPEG4;

/*********** MJPEG decoder picture info ***************/

// None, just pass NULL
// TVMRVideoDecoderRender expects that the TVMRVideoSurface's TVMRSurfaceType
// matches the JPEG format with regard to YV12, YV16, YV24.

/****************** JPEG Decoder ***************************/

typedef struct {
  NvU16 width;    /* encoded width in pixels */
  NvU16 height;   /* encoded height in pixels */
  NvBool  partialAccel;  /* if NV_TRUE, partial acceleration is needed */
} TVMRJPEGInfo;

//  TVMRJPEGGetInfo
//
//  A helper routine which determines if TVMR can decode a particular
//  JPEG stream and, if it can decode it, whether or not the TVMRJPEGDecoder
//  needs to be allocated with supportPartialAccel in order to decode it.
//
//  If TVMR does not support decode of this image TVMR_STATUS_UNSUPPORTED
//  will be returned.  If TVMR does support decode of this JPEG image
//  TVMR_STATUS_OK will be returned and the TVMRJPEGInfo info will
//  be filed out.

TVMRStatus
TVMRJPEGGetInfo (
   TVMRJPEGInfo *info,
   NvU32 numBitstreamBuffers,
   const TVMRBitstreamBuffer *bitstreams
);

typedef struct {
  NvU16 maxWidth;
  NvU16 maxHeight;
  NvU32 maxBitstreamBytes;
  NvBool  supportPartialAccel;
} TVMRJPEGDecoder;

//  TVMRJPEGDecoderRender
//
//  Decode a JPEG image.  TVMRJPEGDecoderRender's pipeline produces a
//  result equivalent to the following sequence:
//
//    1) Decode the full JPEG image.
//    2) Downscale the 8x8 block padded image by the downscaleLog2 factor.
//       That is, a "width" by "height" JPEG is downscaled to
//       "((width + 7) & ~7) >> downscaleLog2" by
//       "((height + 7) & ~7) >> downscaleLog2"
//    3) The rectangle described by srcRect is removed from the downscaled
//       image, optionally mirrored horizontally and/or vertically and/or
//       rotated.
//    4) The transformed source rectangle is then scaled to the dstRect
//       on the output surface.
//
//   The decoder must have a maxWidth and maxHeight greater than or equal
//   to the post-downscale JPEG image and must have a maxBitstreamBytes
//   greater than or equal to the total number of bytes in the bitstream
//   buffers or TVMR_STATUS_INSUFFICIENT_BUFFERING is returned.
//   TVMRJPEGDecoderResize() can be used to increase the supported sizes
//   of existing decoders.
//
//   If the JPEG stream requires partialAccel, the decoder must have been
//   created with supportPartialAccel or TVMR_STATUS_BAD_PARAMETER will
//   be returned.  If the JPEG stream is not supported, TVMR_STATUS_UNSUPPORTED
//   will be returned.  TVMRJPEGGetInfo() can be used to determine if a
//   stream requires paritialAccel or is unsupported.
//
//  Arguments:
//
//   decoder
//      A TVMRJPEGDecoder with the values of maxWidth, maxHeight,
//      maxBitstreamBytes and supportPartialAccel that are required to
//      decode this stream.
//
//   output
//      The results of the decode will go to this surface.
//
//   srcRect
//      The rectangle from the post-downscaled image to be transformed
//      and scaled to the dstRect.  Horizontal and/or vertical mirroring
//      can be achieved by swapping the left-right and/or top-bottom
//      coordinates.  If NULL, the full post-downscaled surface is
//      implied.
//
//   dstRect
//      The destination rectangle on the output surface.  If NULL, a
//      rectangle the full size of the output surface is implied.
//
//   downscaleLog2
//      A value clamped between 0 and 3 inclusive, gives downscale factors
//      of 1 to 8.
//
//   numBitstreamBuffers, bitstreams
//     The number of bitstream buffers and pointer to array of bitstream
//     buffer structures.  TVMRJPEGDecoderRender() copies the data out
//     of these buffers so the caller is free to reuse them as soon as
//     TVMRJPEGDecoderRender() returns.
//
//   fencesWait
//     If not NULL, fencesWait is expected to be a NULL terminated list
//     of TVMRFences.  TVMRJPEGDecoderRender will delay touching the
//     output surface until after all fences in the list have been reached.
//
//   fenceDone
//     It is always guaranteed that TVMRJPEGDecoderRender will
//     complete in a finite amount of time (that is, it flushes).  If
//     not NULL, fenceDone will be filled with the fence indicating
//     when this TVMRJPEGDecoderRender operation will complete.
//
//    flags - Any one of the following:
//           TVMR_RENDER_FLAG_ROTATE_0
//           TVMR_RENDER_FLAG_ROTATE_90
//           TVMR_RENDER_FLAG_ROTATE_180
//           TVMR_RENDER_FLAG_ROTATE_270
//     and optionally OR in either or both of the following:
//           TVMR_RENDER_FLAG_FLIP_HORIZONTAL
//           TVMR_RENDER_FLAG_FLIP_VERTICAL
//
//       Specify a clockwise rotation of the source in degrees.
//       Horizontal and vertical flipping can be specified via these
//       flags as well as by swapping source coordinates.  Flipping is
//       performed before rotating.


TVMRStatus
TVMRJPEGDecoderRender (
   TVMRJPEGDecoder *decoder,
   TVMROutputSurface *output,
   const TVMRRect *srcRect,
   const TVMRRect *dstRect,
   NvU8 downscaleLog2,
   NvU32 numBitstreamBuffers,
   const TVMRBitstreamBuffer *bitstreams,
   TVMRFence *fencesWait,  /* NULL terminated list */
   TVMRFence fenceDone,  /* optional */
   NvU32 flags
);

#define TVMRJPEGDecoderRenderRGB TVMRJPEGDecoderRender

//  TVMRJPEGDecoderRenderYUV
//
//  The same as TVMRJPEGDecoderRender except that the destination surface
//  is a TVMRVideoSurface rather than a TVMROutputSurface.  Also, clipping
//  and scaling (other than downscaleLog2 scaling) are not supported so there
//  are no source or destination rectangles.
//
//  Furthermore, the following restictions apply on the video surface
//  destination:
//    1) The surface must be one of the planar YVXX formats and must have
//       the same chroma sampling as the stream after rotation (eg. 420, 422,
//       422R or 444).  Note that 422 changes to 422R and vice-versa after 90
//       or 270 degree rotation.
//    2) Must be large enough to hold an MCU-padded decoded stream that has
//       been downsampled with downscaleLog2 scaling and rotated.

TVMRStatus
TVMRJPEGDecoderRenderYUV (
   TVMRJPEGDecoder *decoder,
   TVMRVideoSurface *output,
   NvU8 downscaleLog2,
   NvU32 numBitstreamBuffers,
   const TVMRBitstreamBuffer *bitstreams,
   TVMRFence *fencesWait,  /* NULL terminated list */
   TVMRFence fenceDone,  /* optional */
   NvU32 flags
);


//  TVMRJPEGDecoderCreate
//
//  Create a TVMRJPEGDecoder.  See the TVMRJPEGDecoderRender discussion
//  for an overview of the decode process.  This function returns NULL
//  if there were insufficient resources.
//
//  Arguments:
//
//    maxWidth, maxHeight
//      This is typically set to the maximum output surface size you
//      wish to support.  Various size JPEG images can then be supported
//      by using TVMRJPEGDecoderRender's downscaleLog2 factor.
//      TVMRJPEGDecoderResize() can be used to enlarge this limit in
//      an existing decoder.  A decoder can be created with a zero size
//      to be enlarged later.
//
//   maxBitstreamBytes
//      This is set to the maximum JPEG file size you wish to support.
//      TVMRJPEGDecoderResize() can be used to enlarge this limit in
//      an existing decoder.  A decoder can be created with a zero size
//      to be enlarged later.
//
//   supportPartialAccel
//      Some JPEGs require partial acceleration.  Such JPEGs can be decoded
//      only if the decoder is created with this enabled.


TVMRJPEGDecoder *
TVMRJPEGDecoderCreate (
   NvU16 maxWidth,
   NvU16 maxHeight,
   NvU32 maxBitstreamBytes,
   NvBool supportPartialAccel
);

// TVMRJPEGDecoderDestroy
//
// Destroy and existing decoder.

void TVMRJPEGDecoderDestroy(TVMRJPEGDecoder *decoder);

//  TVMRJPEGDecoderResize
//
//  Resize an existing decoder.  See TVMRJPEGDecoderCreate discussion
//  for a description of the arguments.  It is valid for these sizes to
//  be zero.

TVMRStatus
TVMRJPEGDecoderResize (
   TVMRJPEGDecoder *decoder,
   NvU16 maxWidth,
   NvU16 maxHeight,
   NvU32 maxBitstreamBytes
);


/****************** JPEG Encoder ***************************/

typedef struct {
   NvU32 maxBitstreamBytes;
} TVMRJPEGEncoder;

//  TVMRJPEGEncoderCreate
//
//   Create a JPEG Encoder object.  Surfaces are fed to the encoder
//   through TVMRJPEGEncoderFeedFrame() and bitstreams are retrieved
//   with TVMRJPEGEncoderGetBits().
//
//    maxOutputBuffering
//     This determines how many frames of encoded bitstream can be held
//     by the TVMRJPEGEncoder before it must be retrieved using
//     TVMRJPEGEncoderGetBits().  This number is clamped to between 1 and 16.
//     If maxOutputBuffering frames worth of encoded bitstream are yet
//     unretrived by TVMRJPEGEncoderGetBits() then TVMRJPEGEncoderFeedFrame()
//     will return TVMR_STATUS_INSUFFICIENT_BUFFERING and one or more
//     frames need to be retrieved with TVMRJPEGEncoderGetBits() before
//     frame feeding can continue.
//
//   maxBitstreamBytes
//     This determines the maximum size bitstream that the TVMRJPEGEncoder
//     can generate.  If this size is not large enough to hold the resultant
//     bitstream, TVMRJPEGEncoderGetBits() will return
//     TVMR_STATUS_INSUFFICIENT_BUFFERING.

TVMRJPEGEncoder*
TVMRJPEGEncoderCreate (
   NvU8 maxOutputBuffering,
   NvU32 maxBitstreamBytes
);

void TVMRJPEGEncoderDestroy(TVMRJPEGEncoder *encoder);

//   TVMRJPEGEncoderFeedFrame
//
//     Encode the specified inputSurface.  TVMRJPEGEncoderFeedFrame()
//   returns TVMR_STATUS_INSUFFICIENT_BUFFERING if TVMRJPEGEncoderGetBits()
//   has not been called frequently enough and the maximum internal
//   bitstream buffering (determined byt "maxOutputBuffering" passed to
//   TVMRJPEGEncoderCreate()) has been exhausted.
//
//    inputSurface
//      Must be of the following types:
//       TVMRSurfaceType_YV12
//       TVMRSurfaceType_YV16
//       TVMRSurfaceType_YV16_Transposed
//       TVMRSurfaceType_YV24
//
//    fencesWait
///      If not NULL, fencesWait is expected to be a NULL terminated
//       list of TVMRFences.  TVMRVideoEncoderFeedFrame() will
//       delay encoding of this frame until all fences in the list
//       have been reached.
//
//    fenceDone
//       If not NULL, the TVMRFence will be filled out that indicates
//       when the inputSurface is no longer being read.
//
//    quality
//       A value clamped between 1 and 100, mimicking libjpeg's
//       quantization scaling.  Higher numbers are higher quality.
//       50 results in the sample table values from the JPEG specification.

TVMRStatus
TVMRJPEGEncoderFeedFrame (
   TVMRJPEGEncoder *encoder,
   TVMRVideoSurface *inputSurface,
   TVMRFence *fencesWait,
   TVMRFence fenceDone,
   NvU8 quality
);

//  TVMRJPEGEncoderFeedFrameQuant
//
//  Same as TVMRJPEGEncoderFeedFrame() except the application is required
//  to pass both luma and chroma quantization matrices rather than a quality
//  level which TVMRJPEGEncoderFeedFrame() would have used to generate the
//  matrices.  "lumaQuant" and "chromaQuant" are both 64 element 8-bit
//  unsigned quantization tables stored in raster scan order.

TVMRStatus
TVMRJPEGEncoderFeedFrameQuant (
   TVMRJPEGEncoder *encoder,
   TVMRVideoSurface *inputSurface,
   TVMRFence *fencesWait,
   TVMRFence fenceDone,
   NvU8 *lumaQuant,
   NvU8 *chromaQuant
);

//  TVMRJPEGEncoderFeedFrameRateControl
//
//  Same as TVMRJPEGEncoderFeedFrame() except the application is required
//  to pass both luma and chroma quantization matrices as well as target
//  image size in bytes rather than a quality level which
//  TVMRJPEGEncoderFeedFrame() would have used to generate the
//  matrices.  "lumaQuant" and "chromaQuant" are both 64 element 8-bit
//  unsigned quantization tables stored in raster scan order.
//  If there is no rate control needed, targetImageSize should be
//  set to 0xffffffff. The final image size would be close to the target
//  image size if quantization matrices are chosen properly. Most of the
//  time, the final image size would be smaller than the target image size.
//  But at times, the final image size may go beyond target image size by
//  little. The exception where final image size might exceed target
//  image size by a lot is when target image size is really small.

TVMRStatus
TVMRJPEGEncoderFeedFrameRateControl (
   TVMRJPEGEncoder *encoder,
   TVMRVideoSurface *inputSurface,
   TVMRFence *fencesWait,
   TVMRFence fenceDone,
   NvU8 *lumaQuant,
   NvU8 *chromaQuant,
   NvU32 targetImageSize
);

#define TVMR_JPEG_ENC_FLAG_NONE      (0 << 0)
#define TVMR_JPEG_ENC_FLAG_SKIP_SOI  (1 << 0)

//  TVMRJPEGEncoderGetBits
//
//   TVMRJPEGEncoderGetBits() copies the bitstream into the supplied
//   "buffer" and the size is returned in "numBytes".  It is safe to
//   call this function from a separate thread.  The return value and
//   behavior is the same as that of TVMRJPEGEncoderBitsAvailable() when
//   called with TVMR_BLOCKING_TYPE_NEVER except that when TVMR_STATUS_OK
//   is returned, the "buffer" will be filled in addition to the "numBytes".
//   One additional status value is possible:
//   TVMR_STATUS_INSUFFICIENT_BUFFERING, which indicates that the bitstream
//   is truncated due to maxBitstreamBytes not being large enough.
//   The following "flags" are supported:
//     TVMR_JPEG_ENC_FLAG_SKIP_SOI
//        Normally, app inserts EXIF data in jpeg image which occurs
//        after SOI marker. This flag be used to skip SOI marker insertion
//        by TVMR driver.


TVMRStatus
TVMRJPEGEncoderGetBits (
   TVMRJPEGEncoder *encoder,
   NvU32 *numBytes,
   void *buffer,
   NvU32 flags
);

//  TVMRJPEGEncoderBitsAvailable
//
//    TVMRJPEGEncoderBitsAvailable() returns the encode status and
//  number of bytes available for the next image (if any).
//  The specific behavior depends on the specified TVMRBlockingType.
//  It is safe to call this function from a separate thread.
//  Possible return values are:
//
//   TVMR_STATUS_OK - bits are available and the size is returned
//                     in "numBytesAvailable".
//   TVMR_STATUS_PENDING - an encode is in progress but not yet completed.
//   TVMR_STATUS_NONE_PENDING - no encode is in progress.
//   TVMR_STATUS_TIMED_OUT - the operation timed out.
//
//  Arguments:
//
//   numBytesAvailable
//      The number of bytes available in the next encoded frame.  This
//      is valid only when the return value is TVMR_STATUS_OK.
//
//   blockingType
//      The following are supported blocking types:
//
//    TVMR_BLOCKING_TYPE_NEVER
//        This type never blocks so "millisecondTimeout" is ignored.
//        The following are possible return values:  TVMR_STATUS_OK
//        TVMR_STATUS_PENDING or TVMR_STATUS_NONE_PENDING.
//
//    TVMR_BLOCKING_TYPE_IF_PENDING
//        Same as TVMR_BLOCKING_TYPE_NEVER except that TVMR_STATUS_PENDING
//        will never be returned.  If an encode is pending this function
//        will block until the status changes to TVMR_STATUS_OK or until the
//        timeout has occurred.  Possible return values: TVMR_STATUS_OK,
//        TVMR_STATUS_NONE_PENDING or TVMR_STATUS_TIMED_OUT.
//
//    TVMR_BLOCKING_TYPE_ALWAYS
//         This function will return only TVMR_STATUS_OK or
//         TVMR_STATUS_TIMED_OUT.  It will block until those conditions.
//
//   millisecondTimeout
//       Timeout in milliseconds or TVMR_TIMEOUT_INFINITE if a timeout
//       is not desired.


TVMRStatus
TVMRJPEGEncoderBitsAvailable (
   TVMRJPEGEncoder *encoder,
   NvU32 *numBytesIfAvailable,
   TVMRBlockingType blockingType,
   TVMRTime millisecondTimeout
);


/****************** Output-related *************************/

typedef enum {
   TMVR_OUTPUT_TYPE_UNKNOWN = 0,
   TVMR_OUTPUT_TYPE_TFTLCD = 0xb301,
   TVMR_OUTPUT_TYPE_CLILCD = 0xb302,
   TVMR_OUTPUT_TYPE_DSI = 0xb303,
   TVMR_OUTPUT_TYPE_CRT = 0xb304,
   TVMR_OUTPUT_TYPE_TV = 0xb305,
   TVMR_OUTPUT_TYPE_HDMI = 0xb306
} TVMROutputType;

typedef enum {
    TVMR_FLIP_QUEUE_COMPOSITE_TYPE_OPAQUE,
    TVMR_FLIP_QUEUE_COMPOSITE_TYPE_SOURCE_ALPHA,
    TVMR_FLIP_QUEUE_COMPOSITE_TYPE_PREMULTIPLIED_SOURCE_ALPHA
} TVMRFlipQueueCompositeType;

typedef struct {
   NvU16 width;
   NvU16 height;
   float refreshHz;
} TVMROutputMode;

//  TVMROutputInfo
//
//   displayId - The ID by which this output will be known
//   enabled - If NV_TRUE, this output has already been enabled
//      either as an existing TVMROutput or by another display manager.
//      When NV_TRUE, another TVMROutput cannot be created.
//      When NV_FALSE, a new TVMROutput may be created for this displayId
//   type - One of the types defined by the enum
//   mode - Valid only when enabled==NV_TRUE.

typedef struct {
   NvU8 displayId;
   NvBool enabled;
   TVMROutputType type;
   TVMROutputMode mode;
} TVMROutputInfo;

typedef struct {
  TVMROutputInfo outputInfo;
} TVMROutput;

typedef struct {
  NvU8 displayId;
  TVMRSurfaceType type;
} TVMRFlipQueue;

typedef struct {
   float brightness;
   float contrast;
   float saturation;
   float hue;
   TVMRColorStandard colorStandard;
} TVMRFlipQueueAttributes;


// TVMROutputQuery
//
//   Returns an array of available outputs.  The array should be freed
//   with the C library's free().  The returned array will be NULL if
//   numOutputs returns zero.

TVMROutputInfo * TVMROutputQuery (int *numOutputs);
// TVMROutputDestroy
//
//  Destroy an exisiting TVMROutput.

void TVMROutputDestroy (TVMROutput *output);

// TVMROutputCreate
//
//   Create a TVMROutput.  NULL is returned if an error occurred.
//
//  Arguments:
//     displayId - This must be one of the displayIds returned by
//                 TVMROutputQuery()
//     mode - The desired mode.  The desired mode is not guaranteed,
//            but an attempt will be made to come as close as possible.
//            If NULL, TVMR will pick a default mode (usually the panel's
//            native resolution).


TVMROutput *
TVMROutputCreate(
    NvU8 displayId,
    TVMROutputMode *mode
);


//  TVMRFlipQueueDestroy
//
//  Destroy an existing TVMRFlipQueue.

void TVMRFlipQueueDestroy(TVMRFlipQueue *flipQueue);


// TVMRFlipQueueCreate
//
//   Create a TVMRFlipQueue.  NULL is returned if an error occurred.
//   RGB types use TVMRFlipQueueDisplayRGB() to flip while YUV types
//   use TVMRFlipQueueDisplayYUV() to flip.  Only one of each type
//   (YUV and RGB) may be created.
//
//   The YUV and RGB surfaces are composited as follows:
//   1) The full output described by the TVMRFlipQueue's associated
//      TVMROutput (the one that shares the same displayId) is
//      filled with black.
//   2) The YUV surface, if it exists, replaces the black in the rectangle
//      specified by TVMRFlipQueueDisplayYUV().  The YUV surface can be
//      arbitrarily scaled.
//   3) The RGB surface, if it exists, is composited on top of the
//      black and YUV surface using the TVMRFlipQueueCompositeType and
//      rectangle specified by TVMRFlipQueueDisplayRGB().  The RGB surface
//      cannot be scaled but it can be clipped and positioned relative to
//      the output.
//
//  Arguments:
//     displayId - This must be one of the displayIds returned by
//                 TVMROutputQuery()
//     type - Must be one of the following types:
//            TVMRSurfaceType_R8G8B8A8
//            TVMRSurfaceType_YV12

TVMRFlipQueue * TVMRFlipQueueCreate(NvU8 displayId, TVMRSurfaceType type);

//  TVMRFlipQueueDisplayRGB
//
//    Display an RGB surface.
//
//  Arguments:
//     flipQueue - Must be of type TVMRSurfaceType_R8G8B8A8.
//     surf - Must be of type TVMRSurfaceType_R8G8B8A8 or NULL.  NULL
//            removes the RGB layer from the compositing.
//     srcRect - Rectangle of surface to be displayed.  If NULL, a rectangle
//               the full size of "surf" is implied.  Unlike some graphics
//               operations found in TVMR, the srcRect must be well-formed
//               and horizontal or vertical mirroring cannot be done by
//               swapping horizontal or vertical coordinates.  Poorly-
//               formed srcRects are treated as NULL surfaces.
//     dstx,dsty - The location relative to the output mode where the srcRect
//                 will be composited.
//     fenceIn - If not NULL, the flip will not occur until after the
//               fence has been reached.
//     fenceOut - If not NULL, a fence returned indicating when the
//                frame is displayed.
//     compositeType - Specifies how the RGB surface will be composited
//                on top of the lower levels (black and/or YUV).
//     timestamp - Specifies at what time the surface has to be flipped.
//                 NULL means immediately.

void
TVMRFlipQueueDisplayRGB(
   TVMRFlipQueue *flipQueue,
   TVMROutputSurface *surf,
   TVMRRect *srcRect,
   NvS16 dstx,
   NvS16 dsty,
   TVMRFence fenceIn,
   TVMRFence fenceOut,
   TVMRFlipQueueCompositeType compositeType,
   struct timespec *timestamp
);

//  TVMRFlipQueueDisplayYUV
//
//    Display a YUV surface.
//
//  Arguments:
//     flipQueue - Must be of type TVMRSurfaceType_YV12.
//     surf - Must be of type TVMRSurfaceType_YV12 or NULL.  NULL removes
//            the YUV surface from the compositing.
//     pictureStructure - If TVMR_PICTURE_STRUCTURE_TOP_FIELD or
//            TVMR_PICTURE_STRUCTURE_BOTTOM_FIELD the specified field will
//            be bobbed.  The whole frame will be displayed weaved otherwise.
//     srcRect - Rectangle of surface to be displayed.  If NULL, a rectangle
//               the full size of "surf" is implied.  Unlike some graphics
//               operations found in TVMR, the srcRect must be well-formed
//               and horizontal or vertical mirroring cannot be done by
//               swapping horizontal or vertical coordinates. Poorly-
//               formed srcRects are treated as NULL surfaces.
//     dstRect - Location relative to the output where the srcRect should
//               be scaled to and displayed.  If NULL, a rectangle the
//               full size of the output is implied.
//     fenceIn - If not NULL, the flip will not occur until after the
//               fence has been reached.
//     fenceOut - If not NULL, a fence returned indicating when the
//                frame is displayed.
//     timestamp - Specifies at what time the surface has to be flipped.
//                 NULL means immediately.

void
TVMRFlipQueueDisplayYUV(
   TVMRFlipQueue *flipQueue,
   TVMRPictureStructure pictureStructure,
   TVMRVideoSurface *surf,
   TVMRRect *srcRect,
   TVMRRect *dstRect,
   TVMRFence fenceIn,
   TVMRFence fenceOut,
   struct timespec *timestamp
);

// TVMRFlipQueueSetAttributes
//
// Change TVMRFlipQueue attribues.  A pointer to a TVMRFlipQueueAttributes
// is supplied but only the attributes specified in the attribute_mask will
// be changed.  The following attributes are supported for YUV flip queues
// only.  These attributes have no effect for RGB flip queues.
//
//  brightness - A value clamped to between -0.5 and 0.5, initialized to
//               0.0 at TVMRFlipQueue creation.  The corresponding
//               attribute mask is TVMR_FLIP_QUEUE_ATTRIBUTE_BRIGHTNESS.
//  contrast   - A value clamped to between 0.1 and 2.0, initialized to
//               1.0 at TVMRFlipQueue creation.  The corresponding
//               attribute mask is TVMR_FLIP_QUEUE_ATTRIBUTE_CONTRAST.
//  saturation - A value clamped to between 0.0 and 2.0, initialized to
//               1.0 at TVMRFlipQueue creation.  The corresponding
//               attribute mask is TVMR_FLIP_QUEUE_ATTRIBUTE_SATURATION.
//  hue        - A value clamped to between -PI and PI, initialized to
//               0.0 at TVMRFlipQueue creation.  The corresponding
//               attribute mask is TVMR_FLIP_QUEUE_ATTRIBUTE_HUE.
//  colorStandard - One of the following:
//                    TVMR_COLOR_STANDARD_ITUR_BT_601  (default)
//                    TVMR_COLOR_STANDARD_ITUR_BT_709
//                    TVMR_COLOR_STANDARD_SMPTE_240M
//                The corresponding attribute mask is
//                TVMR_FLIP_QUEUE_ATTRIBUTE_COLOR_STANDARD .

void
TVMRFlipQueueSetAttributes(
   TVMRFlipQueue *flipQueue,
   NvU32 attribute_mask,
   TVMRFlipQueueAttributes *attr
);


#ifdef __cplusplus
};     /* extern "C" */
#endif

#endif /* _TVMR_H */
