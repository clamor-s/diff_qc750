#ifndef __eglext_h_
#define __eglext_h_

#ifdef __cplusplus
extern "C" {
#endif

/*
** Copyright (c) 2007-2010 The Khronos Group Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
**
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#include <EGL/eglplatform.h>

/*************************************************************/

/* Header file version number */
/* Current version at http://www.khronos.org/registry/egl/ */
/* $Revision: 11249 $ on $Date: 2010-05-05 09:54:28 -0700 (Wed, 05 May 2010) $ */
#define EGL_EGLEXT_VERSION 5

#ifndef EGL_KHR_config_attribs
#define EGL_KHR_config_attribs 1
#define EGL_CONFORMANT_KHR			0x3042	/* EGLConfig attribute */
#define EGL_VG_COLORSPACE_LINEAR_BIT_KHR	0x0020	/* EGL_SURFACE_TYPE bitfield */
#define EGL_VG_ALPHA_FORMAT_PRE_BIT_KHR		0x0040	/* EGL_SURFACE_TYPE bitfield */
#endif

#ifndef EGL_KHR_lock_surface
#define EGL_KHR_lock_surface 1
#define EGL_READ_SURFACE_BIT_KHR		0x0001	/* EGL_LOCK_USAGE_HINT_KHR bitfield */
#define EGL_WRITE_SURFACE_BIT_KHR		0x0002	/* EGL_LOCK_USAGE_HINT_KHR bitfield */
#define EGL_LOCK_SURFACE_BIT_KHR		0x0080	/* EGL_SURFACE_TYPE bitfield */
#define EGL_OPTIMAL_FORMAT_BIT_KHR		0x0100	/* EGL_SURFACE_TYPE bitfield */
#define EGL_MATCH_FORMAT_KHR			0x3043	/* EGLConfig attribute */
#define EGL_FORMAT_RGB_565_EXACT_KHR		0x30C0	/* EGL_MATCH_FORMAT_KHR value */
#define EGL_FORMAT_RGB_565_KHR			0x30C1	/* EGL_MATCH_FORMAT_KHR value */
#define EGL_FORMAT_RGBA_8888_EXACT_KHR		0x30C2	/* EGL_MATCH_FORMAT_KHR value */
#define EGL_FORMAT_RGBA_8888_KHR		0x30C3	/* EGL_MATCH_FORMAT_KHR value */
#define EGL_MAP_PRESERVE_PIXELS_KHR		0x30C4	/* eglLockSurfaceKHR attribute */
#define EGL_LOCK_USAGE_HINT_KHR			0x30C5	/* eglLockSurfaceKHR attribute */
#define EGL_BITMAP_POINTER_KHR			0x30C6	/* eglQuerySurface attribute */
#define EGL_BITMAP_PITCH_KHR			0x30C7	/* eglQuerySurface attribute */
#define EGL_BITMAP_ORIGIN_KHR			0x30C8	/* eglQuerySurface attribute */
#define EGL_BITMAP_PIXEL_RED_OFFSET_KHR		0x30C9	/* eglQuerySurface attribute */
#define EGL_BITMAP_PIXEL_GREEN_OFFSET_KHR	0x30CA	/* eglQuerySurface attribute */
#define EGL_BITMAP_PIXEL_BLUE_OFFSET_KHR	0x30CB	/* eglQuerySurface attribute */
#define EGL_BITMAP_PIXEL_ALPHA_OFFSET_KHR	0x30CC	/* eglQuerySurface attribute */
#define EGL_BITMAP_PIXEL_LUMINANCE_OFFSET_KHR	0x30CD	/* eglQuerySurface attribute */
#define EGL_LOWER_LEFT_KHR			0x30CE	/* EGL_BITMAP_ORIGIN_KHR value */
#define EGL_UPPER_LEFT_KHR			0x30CF	/* EGL_BITMAP_ORIGIN_KHR value */
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY eglLockSurfaceKHR (EGLDisplay display, EGLSurface surface, const EGLint *attrib_list);
EGLAPI EGLBoolean EGLAPIENTRY eglUnlockSurfaceKHR (EGLDisplay display, EGLSurface surface);
#endif /* EGL_EGLEXT_PROTOTYPES */
typedef EGLBoolean (EGLAPIENTRYP PFNEGLLOCKSURFACEKHRPROC) (EGLDisplay display, EGLSurface surface, const EGLint *attrib_list);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLUNLOCKSURFACEKHRPROC) (EGLDisplay display, EGLSurface surface);
#endif

#ifndef EGL_KHR_image
#define EGL_KHR_image 1
#define EGL_NATIVE_PIXMAP_KHR			0x30B0	/* eglCreateImageKHR target */
typedef void *EGLImageKHR;
#define EGL_NO_IMAGE_KHR			((EGLImageKHR)0)
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLImageKHR EGLAPIENTRY eglCreateImageKHR (EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
EGLAPI EGLBoolean EGLAPIENTRY eglDestroyImageKHR (EGLDisplay dpy, EGLImageKHR image);
#endif /* EGL_EGLEXT_PROTOTYPES */
typedef EGLImageKHR (EGLAPIENTRYP PFNEGLCREATEIMAGEKHRPROC) (EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLDESTROYIMAGEKHRPROC) (EGLDisplay dpy, EGLImageKHR image);
#endif

#ifndef EGL_KHR_vg_parent_image
#define EGL_KHR_vg_parent_image 1
#define EGL_VG_PARENT_IMAGE_KHR			0x30BA	/* eglCreateImageKHR target */
#endif

#ifndef EGL_KHR_gl_texture_2D_image
#define EGL_KHR_gl_texture_2D_image 1
#define EGL_GL_TEXTURE_2D_KHR			0x30B1	/* eglCreateImageKHR target */
#define EGL_GL_TEXTURE_LEVEL_KHR		0x30BC	/* eglCreateImageKHR attribute */
#endif

#ifndef EGL_KHR_gl_texture_cubemap_image
#define EGL_KHR_gl_texture_cubemap_image 1
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR	0x30B3	/* eglCreateImageKHR target */
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR	0x30B4	/* eglCreateImageKHR target */
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR	0x30B5	/* eglCreateImageKHR target */
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR	0x30B6	/* eglCreateImageKHR target */
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR	0x30B7	/* eglCreateImageKHR target */
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR	0x30B8	/* eglCreateImageKHR target */
#endif

#ifndef EGL_KHR_gl_texture_3D_image
#define EGL_KHR_gl_texture_3D_image 1
#define EGL_GL_TEXTURE_3D_KHR			0x30B2	/* eglCreateImageKHR target */
#define EGL_GL_TEXTURE_ZOFFSET_KHR		0x30BD	/* eglCreateImageKHR attribute */
#endif

#ifndef EGL_KHR_gl_renderbuffer_image
#define EGL_KHR_gl_renderbuffer_image 1
#define EGL_GL_RENDERBUFFER_KHR			0x30B9	/* eglCreateImageKHR target */
#endif

#ifndef EGL_KHR_reusable_sync
#define EGL_KHR_reusable_sync 1

typedef void* EGLSyncKHR;
typedef khronos_utime_nanoseconds_t EGLTimeKHR;

#define EGL_SYNC_STATUS_KHR			0x30F1
#define EGL_SIGNALED_KHR			0x30F2
#define EGL_UNSIGNALED_KHR			0x30F3
#define EGL_TIMEOUT_EXPIRED_KHR			0x30F5
#define EGL_CONDITION_SATISFIED_KHR		0x30F6
#define EGL_SYNC_TYPE_KHR			0x30F7
#define EGL_SYNC_REUSABLE_KHR			0x30FA
#define EGL_SYNC_FLUSH_COMMANDS_BIT_KHR		0x0001	/* eglClientWaitSyncKHR <flags> bitfield */
#define EGL_FOREVER_KHR				0xFFFFFFFFFFFFFFFFull
#define EGL_NO_SYNC_KHR				((EGLSyncKHR)0)
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLSyncKHR EGLAPIENTRY eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list);
EGLAPI EGLBoolean EGLAPIENTRY eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync);
EGLAPI EGLint EGLAPIENTRY eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout);
EGLAPI EGLBoolean EGLAPIENTRY eglSignalSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode);
EGLAPI EGLBoolean EGLAPIENTRY eglGetSyncAttribKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint attribute, EGLint *value);
#endif /* EGL_EGLEXT_PROTOTYPES */
typedef EGLSyncKHR (EGLAPIENTRYP PFNEGLCREATESYNCKHRPROC) (EGLDisplay dpy, EGLenum type, const EGLint *attrib_list);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLDESTROYSYNCKHRPROC) (EGLDisplay dpy, EGLSyncKHR sync);
typedef EGLint (EGLAPIENTRYP PFNEGLCLIENTWAITSYNCKHRPROC) (EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLSIGNALSYNCKHRPROC) (EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLGETSYNCATTRIBKHRPROC) (EGLDisplay dpy, EGLSyncKHR sync, EGLint attribute, EGLint *value);
#endif

#ifndef EGL_KHR_image_base
#define EGL_KHR_image_base 1
/* Most interfaces defined by EGL_KHR_image_pixmap above */
#define EGL_IMAGE_PRESERVED_KHR			0x30D2	/* eglCreateImageKHR attribute */
#endif

#ifndef EGL_KHR_image_pixmap
#define EGL_KHR_image_pixmap 1
/* Interfaces defined by EGL_KHR_image above */
#endif

#ifndef EGL_IMG_context_priority
#define EGL_IMG_context_priority 1
#define EGL_CONTEXT_PRIORITY_LEVEL_IMG		0x3100
#define EGL_CONTEXT_PRIORITY_HIGH_IMG		0x3101
#define EGL_CONTEXT_PRIORITY_MEDIUM_IMG		0x3102
#define EGL_CONTEXT_PRIORITY_LOW_IMG		0x3103
#endif

#ifndef EGL_NV_coverage_sample
#define EGL_NV_coverage_sample 1
#define EGL_COVERAGE_BUFFERS_NV 0x30E0
#define EGL_COVERAGE_SAMPLES_NV 0x30E1
#endif

#ifndef EGL_NV_depth_nonlinear
#define EGL_NV_depth_nonlinear 1
#define EGL_DEPTH_ENCODING_NV 0x30E2
#define EGL_DEPTH_ENCODING_NONE_NV 0
#define EGL_DEPTH_ENCODING_NONLINEAR_NV 0x30E3
#endif

#ifndef EGL_NV_sync
#define EGL_NV_sync 1
#define EGL_SYNC_PRIOR_COMMANDS_COMPLETE_NV	0x30E6
#define EGL_SYNC_STATUS_NV			0x30E7
#define EGL_SIGNALED_NV				0x30E8
#define EGL_UNSIGNALED_NV			0x30E9
#define EGL_SYNC_FLUSH_COMMANDS_BIT_NV		0x0001
#define EGL_FOREVER_NV				0xFFFFFFFFFFFFFFFFull
#define EGL_ALREADY_SIGNALED_NV			0x30EA
#define EGL_TIMEOUT_EXPIRED_NV			0x30EB
#define EGL_CONDITION_SATISFIED_NV		0x30EC
#define EGL_SYNC_TYPE_NV			0x30ED
#define EGL_SYNC_CONDITION_NV			0x30EE
#define EGL_SYNC_FENCE_NV			0x30EF
#define EGL_NO_SYNC_NV				((EGLSyncNV)0)
typedef void* EGLSyncNV;
typedef unsigned long long EGLTimeNV;
#ifdef EGL_EGLEXT_PROTOTYPES
EGLSyncNV eglCreateFenceSyncNV (EGLDisplay dpy, EGLenum condition, const EGLint *attrib_list);
EGLBoolean eglDestroySyncNV (EGLSyncNV sync);
EGLBoolean eglFenceNV (EGLSyncNV sync);
EGLint eglClientWaitSyncNV (EGLSyncNV sync, EGLint flags, EGLTimeNV timeout);
EGLBoolean eglSignalSyncNV (EGLSyncNV sync, EGLenum mode);
EGLBoolean eglGetSyncAttribNV (EGLSyncNV sync, EGLint attribute, EGLint *value);
#endif /* EGL_EGLEXT_PROTOTYPES */
typedef EGLSyncNV (EGLAPIENTRYP PFNEGLCREATEFENCESYNCNVPROC) (EGLDisplay dpy, EGLenum condition, const EGLint *attrib_list);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLDESTROYSYNCNVPROC) (EGLSyncNV sync);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLFENCENVPROC) (EGLSyncNV sync);
typedef EGLint (EGLAPIENTRYP PFNEGLCLIENTWAITSYNCNVPROC) (EGLSyncNV sync, EGLint flags, EGLTimeNV timeout);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLSIGNALSYNCNVPROC) (EGLSyncNV sync, EGLenum mode);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLGETSYNCATTRIBNVPROC) (EGLSyncNV sync, EGLint attribute, EGLint *value);
#endif

#if KHRONOS_SUPPORT_INT64 /* EGLuint64KHR requires 64-bit uint support */
#ifndef EGL_KHR_stream
#define EGL_KHR_stream 1
typedef void* EGLStreamKHR;
typedef khronos_uint64_t EGLuint64KHR;
#define EGL_NO_STREAM_KHR               ((EGLStreamKHR)0)
#define EGL_CONSUMER_LATENCY_USEC_KHR           0x3210
#define EGL_PRODUCER_FRAME_KHR              0x3212
#define EGL_CONSUMER_FRAME_KHR              0x3213
#define EGL_STREAM_STATE_KHR                0x3214
#define EGL_STREAM_STATE_CREATED_KHR            0x3215
#define EGL_STREAM_STATE_CONNECTING_KHR         0x3216
#define EGL_STREAM_STATE_EMPTY_KHR          0x3217
#define EGL_STREAM_STATE_NEW_FRAME_AVAILABLE_KHR    0x3218
#define EGL_STREAM_STATE_OLD_FRAME_AVAILABLE_KHR    0x3219
#define EGL_STREAM_STATE_DISCONNECTED_KHR       0x321A
#define EGL_BAD_STREAM_KHR              0x321B
#define EGL_BAD_STATE_KHR               0x321C
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLStreamKHR EGLAPIENTRY eglCreateStreamKHR(EGLDisplay dpy, const EGLint *attrib_list);
EGLAPI EGLBoolean EGLAPIENTRY eglDestroyStreamKHR(EGLDisplay dpy, EGLStreamKHR stream);
EGLAPI EGLBoolean EGLAPIENTRY eglStreamAttribKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint value);
EGLAPI EGLBoolean EGLAPIENTRY eglQueryStreamKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint *value);
EGLAPI EGLBoolean EGLAPIENTRY eglQueryStreamu64KHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLuint64KHR *value);
#endif /* EGL_EGLEXT_PROTOTYPES */
typedef EGLStreamKHR (EGLAPIENTRYP PFNEGLCREATESTREAMKHRPROC)(EGLDisplay dpy, const EGLint *attrib_list);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLDESTROYSTREAMKHRPROC)(EGLDisplay dpy, EGLStreamKHR stream);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLSTREAMATTRIBKHRPROC)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint value);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYSTREAMKHRPROC)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint *value);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYSTREAMU64KHRPROC)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLuint64KHR *value);
#endif
#endif

#ifdef EGL_KHR_stream /* Requires KHR_stream extension */
#ifndef EGL_KHR_stream_consumer_gltexture
#define EGL_KHR_stream_consumer_gltexture 1
#define EGL_CONSUMER_ACQUIRE_TIMEOUT_USEC_KHR       0x321E
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY eglStreamConsumerGLTextureExternalKHR(EGLDisplay dpy, EGLStreamKHR stream);
EGLAPI EGLBoolean EGLAPIENTRY eglStreamConsumerAcquireKHR(EGLDisplay dpy, EGLStreamKHR stream);
EGLAPI EGLBoolean EGLAPIENTRY eglStreamConsumerReleaseKHR(EGLDisplay dpy, EGLStreamKHR stream);
#endif /* EGL_EGLEXT_PROTOTYPES */
typedef EGLBoolean (EGLAPIENTRYP PFNEGLSTREAMCONSUMERGLTEXTUREEXTERNALKHRPROC)(EGLDisplay dpy, EGLStreamKHR stream);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLSTREAMCONSUMERACQUIREKHRPROC)(EGLDisplay dpy, EGLStreamKHR stream);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLSTREAMCONSUMERRELEASEKHRPROC)(EGLDisplay dpy, EGLStreamKHR stream);
#endif
#endif

#ifdef EGL_KHR_stream /* Requires KHR_stream extension */
#ifndef EGL_KHR_stream_producer_eglsurface
#define EGL_KHR_stream_producer_eglsurface 1
#define EGL_STREAM_BIT_KHR              0x0800
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLSurface EGLAPIENTRY eglCreateStreamProducerSurfaceKHR(EGLDisplay dpy, EGLConfig config, EGLStreamKHR stream, const EGLint *attrib_list);
#endif /* EGL_EGLEXT_PROTOTYPES */
typedef EGLSurface (EGLAPIENTRYP PFNEGLCREATESTREAMPRODUCERSURFACEKHRPROC)(EGLDisplay dpy, EGLConfig config, EGLStreamKHR stream, const EGLint *attrib_list);
#endif
#endif

#ifdef EGL_KHR_stream /* Requires KHR_stream extension */
#ifndef EGL_KHR_stream_fifo
#define EGL_KHR_stream_fifo 1
/* reuse EGLTimeKHR */
#define EGL_STREAM_FIFO_LENGTH_KHR          0x31FC
#define EGL_STREAM_TIME_NOW_KHR             0x31FD
#define EGL_STREAM_TIME_CONSUMER_KHR            0x31FE
#define EGL_STREAM_TIME_PRODUCER_KHR            0x31FF
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY eglQueryStreamTimeKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLTimeKHR *value);
#endif /* EGL_EGLEXT_PROTOTYPES */
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYSTREAMTIMEKHRPROC)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLTimeKHR *value);
#endif
#endif

#ifndef EGL_KHR_fence_sync
#define EGL_KHR_fence_sync 1
/* Reuses most tokens and entry points from EGL_KHR_reusable_sync */
#define EGL_SYNC_PRIOR_COMMANDS_COMPLETE_KHR	0x30F0
#define EGL_SYNC_CONDITION_KHR			0x30F8
#define EGL_SYNC_FENCE_KHR			0x30F9
#endif

#ifndef EGL_EXT_create_context_robustness
#define EGL_EXT_create_context_robustness 1
#define EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT	    0x30BF
#define EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT 0x3138
#define EGL_NO_RESET_NOTIFICATION_EXT		    0x31BE
#define EGL_LOSE_CONTEXT_ON_RESET_EXT		    0x31BF
#endif

#ifndef EGL_KHR_create_context
#define EGL_KHR_create_context 1
#define EGL_CONTEXT_MAJOR_VERSION_KHR			    EGL_CONTEXT_CLIENT_VERSION
#define EGL_CONTEXT_MINOR_VERSION_KHR			    0x30FB
#define EGL_CONTEXT_FLAGS_KHR				    0x30FC
#define EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR		    0x30FD
#define EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR  0x31BD
#define EGL_NO_RESET_NOTIFICATION_KHR			    0x31BE
#define EGL_LOSE_CONTEXT_ON_RESET_KHR			    0x31BF
#define EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR		    0x00000001
#define EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR	    0x00000002
#define EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR	    0x00000004
#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR		    0x00000001
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR    0x00000002
#endif

/* EGL_KHR_stream_cross_process_fd
*/
#ifndef EGL_KHR_stream_cross_process_fd
#define EGL_KHR_stream_cross_process_fd 1
typedef int EGLNativeFileDescriptorKHR;
#define EGL_NO_FILE_DESCRIPTOR_KHR                   ((EGLNativeFileDescriptorKHR)(-1))
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLNativeFileDescriptorKHR EGLAPIENTRY eglGetStreamFileDescriptorKHR(EGLDisplay dpy, EGLStreamKHR stream);
EGLAPI EGLStreamKHR EGLAPIENTRY eglCreateStreamFromFileDescriptorKHR(EGLDisplay dpy, EGLNativeFileDescriptorKHR file_descriptor);
#endif
typedef EGLNativeFileDescriptorKHR (EGLAPIENTRYP PFNEGLGETSTREAMFILEDESCRIPTORKHRPROC)(EGLDisplay dpy, EGLStreamKHR stream);
typedef EGLStreamKHR (EGLAPIENTRYP PFNEGLCREATESTREAMFROMFILEDESCRIPTORKHRPROC)(EGLDisplay dpy, EGLNativeFileDescriptorKHR file_descriptor);
#endif

#ifndef EGL_ANDROID_image_native_buffer
#define EGL_ANDROID_image_native_buffer 1
struct ANativeWindowBuffer;
#define EGL_NATIVE_BUFFER_ANDROID               0x3140  /* eglCreateImageKHR target */
#endif

#ifndef EGL_ANDROID_recordable
#define EGL_ANDROID_recordable 1
#define EGL_RECORDABLE_ANDROID                  0x3142  /* EGLConfig attribute */
#endif

/* EGL_EXT_create_context_robustness
 */
#ifndef EGL_EXT_create_context_robustness
#define EGL_EXT_create_context_robustness 1
#define EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT    0x30BF
#define EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT 0x3138
#define EGL_NO_RESET_NOTIFICATION_EXT       0x31BE
#define EGL_LOSE_CONTEXT_ON_RESET_EXT       0x31BF
#endif

/* EGL_NV_system_time
 */
#ifndef EGL_NV_system_time
#define EGL_NV_system_time 1
typedef khronos_int64_t EGLint64NV;
typedef khronos_uint64_t EGLuint64NV;
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLuint64NV EGLAPIENTRY eglGetSystemTimeFrequencyNV(void);
EGLAPI EGLuint64NV EGLAPIENTRY eglGetSystemTimeNV(void);
#endif
typedef EGLuint64NV (EGLAPIENTRYP PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC)(void);
typedef EGLuint64NV (EGLAPIENTRYP PFNEGLGETSYSTEMTIMENVPROC)(void);
#endif

/* EGL_NV_native_query
 */
#ifndef EGL_NV_native_query
#define EGL_NV_native_query 1
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY eglQueryNativeDisplayNV(EGLDisplay dpy, EGLNativeDisplayType *display_id);
EGLAPI EGLBoolean EGLAPIENTRY eglQueryNativeWindowNV(EGLDisplay dpy, EGLSurface surf, EGLNativeWindowType *window);
EGLAPI EGLBoolean EGLAPIENTRY eglQueryNativePixmapNV(EGLDisplay dpy, EGLSurface surf, EGLNativePixmapType *pixmap);
#else
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYNATIVEDISPLAYNVPROC)(EGLDisplay dpy, EGLNativeDisplayType *display_id);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYNATIVEWINDOWNVPROC)(EGLDisplay dpy, EGLSurface surf, EGLNativeWindowType *window);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYNATIVEPIXMAPNVPROC)(EGLDisplay dpy, EGLSurface surf, EGLNativePixmapType *pixmap);
#endif
#endif

/* EGL_NV_3dvision_surface
 */
#ifndef EGL_NV_3dvision_surface
#define EGL_NV_3dvision_surface
#define EGL_AUTO_STEREO_NV              0x3136
#endif

/* EGL_NV_perfmon
 */
#ifndef EGL_NV_perfmon
#define EGL_NV_perfmon 1
#define EGL_PERFMONITOR_HARDWARE_COUNTERS_BIT_NV    0x00000001
#define EGL_PERFMONITOR_OPENGL_ES_API_BIT_NV        0x00000010
#define EGL_PERFMONITOR_OPENVG_API_BIT_NV           0x00000020
#define EGL_PERFMONITOR_OPENGL_ES2_API_BIT_NV       0x00000040
#define EGL_COUNTER_NAME_NV                         0x3220
#define EGL_COUNTER_DESCRIPTION_NV                  0x3221
#define EGL_IS_HARDWARE_COUNTER_NV                  0x3222
#define EGL_COUNTER_MAX_NV                          0x3223
#define EGL_COUNTER_VALUE_TYPE_NV                   0x3224
#define EGL_RAW_VALUE_NV                            0x3225
#define EGL_PERCENTAGE_VALUE_NV                     0x3226
#define EGL_BAD_CURRENT_PERFMONITOR_NV              0x3227
#define EGL_NO_PERFMONITOR_NV ((EGLPerfMonitorNV)0)
#define EGL_DEFAULT_PERFMARKER_NV ((EGLPerfMarkerNV)0)
typedef void *EGLPerfMonitorNV;
typedef void *EGLPerfCounterNV;
typedef void *EGLPerfMarkerNV;
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLPerfMonitorNV EGLAPIENTRY eglCreatePerfMonitorNV(EGLDisplay dpy, EGLint mask);
EGLAPI EGLBoolean EGLAPIENTRY eglDestroyPerfMonitorNV(EGLDisplay dpy, EGLPerfMonitorNV monitor);
EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrentPerfMonitorNV(EGLPerfMonitorNV monitor);
EGLAPI EGLPerfMonitorNV EGLAPIENTRY eglGetCurrentPerfMonitorNV(void);
EGLAPI EGLBoolean EGLAPIENTRY eglGetPerfCountersNV(EGLPerfMonitorNV monitor, EGLPerfCounterNV *counters, EGLint counter_size, EGLint *num_counter);
EGLAPI EGLBoolean EGLAPIENTRY eglGetPerfCounterAttribNV(EGLPerfMonitorNV monitor, EGLPerfCounterNV counter, EGLint pname, EGLint *value);
EGLAPI const char * EGLAPIENTRY eglQueryPerfCounterStringNV(EGLPerfMonitorNV monitor, EGLPerfCounterNV counter, EGLint pname);
EGLAPI EGLBoolean EGLAPIENTRY eglPerfMonitorAddCountersNV(EGLint n, const EGLPerfCounterNV *counters);
EGLAPI EGLBoolean EGLAPIENTRY eglPerfMonitorRemoveCountersNV(EGLint n, const EGLPerfCounterNV *counters);
EGLAPI EGLBoolean EGLAPIENTRY eglPerfMonitorRemoveAllCountersNV(void);
EGLAPI EGLBoolean EGLAPIENTRY eglPerfMonitorBeginExperimentNV(void);
EGLAPI EGLBoolean EGLAPIENTRY eglPerfMonitorEndExperimentNV(void);
EGLAPI EGLBoolean EGLAPIENTRY eglPerfMonitorBeginPassNV(EGLint n);
EGLAPI EGLBoolean EGLAPIENTRY eglPerfMonitorEndPassNV(void);
EGLAPI EGLPerfMarkerNV EGLAPIENTRY eglCreatePerfMarkerNV(void);
EGLAPI EGLBoolean EGLAPIENTRY eglDestroyPerfMarkerNV(EGLPerfMarkerNV marker);
EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrentPerfMarkerNV(EGLPerfMarkerNV marker);
EGLAPI EGLPerfMarkerNV EGLAPIENTRY eglGetCurrentPerfMarkerNV(void);
EGLAPI EGLBoolean EGLAPIENTRY eglGetPerfMarkerCounterNV(EGLPerfMarkerNV marker, EGLPerfCounterNV counter, EGLuint64NV *value, EGLuint64NV *cycles);
EGLAPI EGLBoolean EGLAPIENTRY eglValidatePerfMonitorNV(EGLint *num_passes);
#endif
typedef EGLPerfMonitorNV (EGLAPIENTRYP PFNEGLCREATEPERFMONITORNVPROC)(EGLDisplay dpy, EGLint mask);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLDESTROYPERFMONITORNVPROC)(EGLDisplay dpy, EGLPerfMonitorNV monitor);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLMAKECURRENTPERFMONITORNVPROC)(EGLPerfMonitorNV monitor);
typedef EGLPerfMonitorNV (EGLAPIENTRYP PFNEGLGETCURRENTPERFMONITORNVPROC)(void);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLGETPERFCOUNTERSNVPROC)(EGLPerfMonitorNV monitor, EGLPerfCounterNV *counters, EGLint counter_size, EGLint *num_counter);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLGETPERFCOUNTERATTRIBNVPROC)(EGLPerfMonitorNV monitor, EGLPerfCounterNV counter, EGLint pname, EGLint *value);
typedef const char * (EGLAPIENTRYP PFNEGLQUERYPERFCOUNTERSTRINGNVPROC)(EGLPerfMonitorNV monitor, EGLPerfCounterNV counter, EGLint pname);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLPERFMONITORADDCOUNTERSNVPROC)(EGLint n, const EGLPerfCounterNV *counters);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLPERFMONITORREMOVECOUNTERSNVPROC)(EGLint n, const EGLPerfCounterNV *counters);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLPERFMONITORREMOVEALLCOUNTERSNVPROC)(void);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLPERFMONITORBEGINEXPERIMENTNVPROC)(void);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLPERFMONITORENDEXPERIMENTNVPROC)(void);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLPERFMONITORBEGINPASSNVPROC)(EGLint n);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLPERFMONITORENDPASSNVPROC)(void);
typedef EGLPerfMarkerNV (EGLAPIENTRYP PFNEGLCREATEPERFMARKERNVPROC)(void);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLDESTROYPERFMARKERNVPROC)(EGLPerfMarkerNV marker);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLMAKECURRENTPERFMARKERNVPROC)(EGLPerfMarkerNV marker);
typedef EGLPerfMarkerNV (EGLAPIENTRYP PFNEGLGETCURRENTPERFMARKERNVPROC)(void);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLGETPERFMARKERCOUNTERNVPROC)(EGLPerfMarkerNV marker, EGLPerfCounterNV counter, EGLuint64NV *value, EGLuint64NV *cycles);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLVALIDATEPERFMONITORNVPROC)(EGLint *num_passes);
#endif

/* EGL_NV_coverage_sample
 */
#ifndef EGL_NV_coverage_sample
#define EGL_NV_coverage_sample 1
#define EGL_COVERAGE_BUFFERS_NV 0x30E0
#define EGL_COVERAGE_SAMPLES_NV 0x30E1
#endif

/* EGL_NV_coverage_sample_resolve
 */
#ifndef EGL_NV_coverage_sample_resolve
#define EGL_NV_coverage_sample_resolve 1
#define EGL_COVERAGE_SAMPLE_RESOLVE_NV          0x3131
#define EGL_COVERAGE_SAMPLE_RESOLVE_DEFAULT_NV  0x3132
#define EGL_COVERAGE_SAMPLE_RESOLVE_NONE_NV     0x3133
#endif

/* EGL_NV_multiview_window
*/
#ifndef EGL_NV_multiview_window
#define EGL_NV_multiview_window      1
#define EGL_MULTIVIEW_VIEW_COUNT_NV             0x3134
#endif

/* EGL_NV_stream_sync
*/
#ifndef EGL_NV_stream_sync
#define EGL_NV_stream_sync 1
#define EGL_SYNC_NEW_FRAME_NV 0x321F
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLSyncKHR EGLAPIENTRY eglCreateStreamSyncNV(EGLDisplay dpy, EGLStreamKHR stream, EGLenum type, const EGLint *attrib_list);
#endif
typedef EGLSyncKHR (EGLAPIENTRYP PFNEGLCREATESTREAMSYNCNVPROC)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum type, const EGLint *attrib_list);
#endif

/* EGL_ANDROID_blob_cache
 */
#ifndef EGL_ANDROID_blob_cache
#define EGL_ANDROID_blob_cache 1
typedef khronos_ssize_t EGLsizeiANDROID;
typedef void (*EGLSetBlobFuncANDROID) (const void* key, EGLsizeiANDROID keySize, const void* value, EGLsizeiANDROID valueSize);
typedef EGLsizeiANDROID (*EGLGetBlobFuncANDROID) (const void* key, EGLsizeiANDROID keySize, void* value, EGLsizeiANDROID valueSize);
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI void EGLAPIENTRY eglSetBlobCacheFuncsANDROID(EGLDisplay dpy, EGLSetBlobFuncANDROID set, EGLGetBlobFuncANDROID get);
#endif /* EGL_EGLEXT_PROTOTYPES */
typedef void (EGLAPIENTRYP PFNEGLSETBLOBCACHEFUNCSANDROIDPROC) (EGLDisplay dpy,
        EGLSetBlobFuncANDROID set, EGLGetBlobFuncANDROID get);
#endif

/* EGL_IMG_hibernate_process
 */
#ifndef EGL_IMG_hibernate_process
#define EGL_IMG_hibernate_process 1
typedef EGLBoolean (EGLAPIENTRYP PFEGLHIBERNATEPROCESSIMGPROC)(void);
typedef EGLBoolean (EGLAPIENTRYP PFEGLAWAKENPROCESSIMGPROC)(void);
#endif

/* EGL_ANDROID_native_fence_sync
 */
#ifndef EGL_ANDROID_native_fence_sync
#define EGL_ANDROID_native_fence_sync 1
#define EGL_SYNC_NATIVE_FENCE_ANDROID                 0x3144
#define EGL_SYNC_NATIVE_FENCE_FD_ANDROID              0x3145
#define EGL_NO_NATIVE_FENCE_FD_ANDROID                -1
#define EGL_SYNC_NATIVE_FENCE_SIGNALED_ANDROID        0x3146
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLint EGLAPIENTRY eglDupNativeFenceFDANDROID(EGLDisplay dpy, EGLSyncKHR sync);
#endif
typedef EGLint (EGLAPIENTRYP PFNEGLDUPNATIVEFENCEFDANDROID) (EGLDisplay dpy, EGLSyncKHR sync);
#endif

/* EGL_ANDROID_wait_sync
 */
#ifndef EGL_ANDROID_wait_sync
#define EGL_ANDROID_wait_sync
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLint EGLAPIENTRY eglWaitSyncANDROID(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags);
#endif
typedef EGLint (EGLAPIENTRYP PFNEGLWAITSYNCANDROID) (EGLDisplay dpy, EGLSyncKHR sync, EGLint flags);
#endif

/* EGL_ANDROID_framebuffer_target
 */
#ifndef EGL_ANDROID_framebuffer_target
#define EGL_ANDROID_framebuffer_target
#define EGL_FRAMEBUFFER_TARGET_ANDROID                0x3147
#endif

#ifdef __cplusplus
}
#endif

#endif
