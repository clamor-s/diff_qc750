#ifndef __eglext_nv_h_
#define __eglext_nv_h_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright (c) 2008-2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include <EGL/eglplatform.h>

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

/* EGL_NV_perfmon
 */
#ifndef EGL_NV_perfmon
#define EGL_NV_perfmon 1
#define EGL_PERFMONITOR_HARDWARE_COUNTERS_BIT_NV    0x00000001
#define EGL_PERFMONITOR_OPENGL_ES_API_BIT_NV        0x00000010
#define EGL_PERFMONITOR_OPENVG_API_BIT_NV           0x00000020
#define EGL_PERFMONITOR_OPENGL_ES2_API_BIT_NV       0x00000040
#define EGL_COUNTER_NAME_NV            0x1234
#define EGL_COUNTER_DESCRIPTION_NV     0x1235
#define EGL_IS_HARDWARE_COUNTER_NV     0x1236
#define EGL_COUNTER_MAX_NV             0x1237
#define EGL_COUNTER_VALUE_TYPE_NV      0x1238
#define EGL_RAW_VALUE_NV               0x1239
#define EGL_PERCENTAGE_VALUE_NV        0x1240
#define EGL_BAD_CURRENT_PERFMONITOR_NV 0x1241
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

/* EGL_NV_depth_nonlinear
 */
#ifndef EGL_NV_depth_nonlinear
#define EGL_NV_depth_nonlinear 1
#define EGL_DEPTH_ENCODING_NV 0x30E2
#define EGL_DEPTH_ENCODING_NONE_NV 0
#define EGL_DEPTH_ENCODING_NONLINEAR_NV 0x30E3
#endif

/* EGL_NV_sync
 */
#ifndef EGL_NV_sync
#define EGL_NV_sync 1
typedef void* EGLSyncNV;
typedef unsigned long long EGLTimeNV;
#ifdef EGL_EGLEXT_PROTOTYPES
EGLSyncNV eglCreateFenceSyncNV( EGLDisplay dpy, EGLenum condition, const EGLint *attrib_list );
EGLBoolean eglDestroySyncNV( EGLSyncNV sync );
EGLBoolean eglFenceNV( EGLSyncNV sync );
EGLint eglClientWaitSyncNV( EGLSyncNV sync, EGLint flags, EGLTimeNV timeout );
EGLBoolean eglSignalSyncNV( EGLSyncNV sync, EGLenum mode );
EGLBoolean eglGetSyncAttribNV( EGLSyncNV sync, EGLint attribute, EGLint *value );
#else
typedef EGLSyncNV (EGLAPIENTRYP PFNEGLCREATEFENCESYNCNVPROC)( EGLDisplay dpy, EGLenum condition, const EGLint *attrib_list );
typedef EGLBoolean (EGLAPIENTRYP PFNEGLDESTROYSYNCNVPROC)( EGLSyncNV sync );
typedef EGLBoolean (EGLAPIENTRYP PFNEGLFENCENVPROC)( EGLSyncNV sync );
typedef EGLint (EGLAPIENTRYP PFNEGLCLIENTWAITSYNCNVPROC)( EGLSyncNV sync, EGLint flags, EGLTimeNV timeout );
typedef EGLBoolean (EGLAPIENTRYP PFNEGLSIGNALSYNCNVPROC)( EGLSyncNV sync, EGLenum mode );
typedef EGLBoolean (EGLAPIENTRYP PFNEGLGETSYNCATTRIBNVPROC)( EGLSyncNV sync, EGLint attribute, EGLint *value );
#endif

#define EGL_SYNC_PRIOR_COMMANDS_COMPLETE_NV     0x30E6
#define EGL_SYNC_STATUS_NV                      0x30E7
#define EGL_SIGNALED_NV                         0x30E8
#define EGL_UNSIGNALED_NV                       0x30E9
#define EGL_SYNC_FLUSH_COMMANDS_BIT_NV          0x0001
#define EGL_FOREVER_NV                          0xFFFFFFFFFFFFFFFFull
#define EGL_ALREADY_SIGNALED_NV                 0x30EA
#define EGL_TIMEOUT_EXPIRED_NV                  0x30EB
#define EGL_CONDITION_SATISFIED_NV              0x30EC
#define EGL_SYNC_TYPE_NV                        0x30ED
#define EGL_SYNC_CONDITION_NV                   0x30EE
#define EGL_SYNC_FENCE_NV                       0x30EF
#define EGL_NO_SYNC_NV                          ((EGLSyncNV)0)
#endif

/* EGL_NV_post_sub_buffer
 */
#ifndef EGL_NV_post_sub_buffer
#define EGL_NV_post_sub_buffer 1
#ifdef EGL_EGLEXT_PROTOTYPES
EGLBoolean eglPostSubBufferNV( EGLDisplay dpy, EGLSurface draw, EGLint x, EGLint y, EGLint width, EGLint height );
#endif
typedef EGLBoolean (EGLAPIENTRYP PFNEGLPOSTSUBBUFFERNVPROC)( EGLDisplay dpy, EGLSurface draw, EGLint x, EGLint y, EGLint width, EGLint height);
#define EGL_POST_SUB_BUFFER_SUPPORTED_NV        0x30BE
#endif

/* EGL_NV_coverage_sample
 */
#ifndef EGL_NV_coverage_sample_resolve
#define EGL_NV_coverage_sample_resolve 1
#define EGL_COVERAGE_SAMPLE_RESOLVE_NV          0x3131
#define EGL_COVERAGE_SAMPLE_RESOLVE_DEFAULT_NV  0x3132
#define EGL_COVERAGE_SAMPLE_RESOLVE_NONE_NV     0x3133
#endif

/* EGL_NV_multiview
*/
#ifndef EGL_NV_multiview
#define EGL_NV_multiview      1
#define EGL_MULTIVIEW_VIEW_COUNT_NV              0x3134
#endif

/* EGL_EXT_create_context_robustness
*/
#ifndef EGL_EXT_create_context_robustness
#define EGL_EXT_create_context_robustness   1
#define EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT        0x30BF
#define EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT 0x3138
#define EGL_NO_RESET_NOTIFICATION_EXT               0x31BE
#define EGL_LOSE_CONTEXT_ON_RESET_EXT               0x31BF
#endif

/* EGL_KHR_stream
*/
#ifndef EGL_KHR_stream
#define EGL_KHR_stream
typedef void* EGLStreamKHR;
typedef khronos_uint64_t EGLuint64KHR;
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLStreamKHR EGLAPIENTRY eglCreateStreamKHR(EGLDisplay dpy, const EGLint *attrib_list);
EGLAPI EGLBoolean EGLAPIENTRY eglDestroyStreamKHR(EGLDisplay dpy, EGLStreamKHR stream);
EGLAPI EGLBoolean EGLAPIENTRY eglStreamAttribKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint value);
EGLAPI EGLBoolean EGLAPIENTRY eglQueryStreamKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint *value);
EGLAPI EGLBoolean EGLAPIENTRY eglQueryStream64KHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLuint64KHR *value);
#endif
typedef EGLStreamKHR (EGLAPIENTRYP PFNEGLCREATESTREAMKHRPROC)(EGLDisplay dpy, const EGLint *attrib_list);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLDESTROYSTREAMKHRPROC)(EGLDisplay dpy, EGLStreamKHR stream);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLSTREAMATTRIBKHRPROC)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint value);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYSTREAMKHRPROC)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint *value);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYSTREAM64KHRPROC)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLuint64KHR *value);
#define EGL_NO_STREAM_KHR                           ((EGLStreamKHR)0)
#define EGL_CONSUMER_LATENCY_USEC_KHR               0x3210
#define EGL_PRODUCER_FRAME_KHR                      0x3212
#define EGL_CONSUMER_FRAME_KHR                      0x3213
#define EGL_STREAM_STATE_KHR                        0x3214
#define EGL_STREAM_STATE_CREATED_KHR                0x3215
#define EGL_STREAM_STATE_CONNECTING_KHR             0x3216
#define EGL_STREAM_STATE_EMPTY_KHR                  0x3217
#define EGL_STREAM_STATE_NEW_FRAME_AVAILABLE_KHR    0x3218
#define EGL_STREAM_STATE_OLD_FRAME_AVAILABLE_KHR    0x3219
#define EGL_STREAM_STATE_DISCONNECTED_KHR           0x321A
#define EGL_BAD_STREAM_KHR                          0x321B
#define EGL_BAD_STATE_KHR                           0x321C
#endif

/* EGL_KHR_stream_fifo
*/
#ifndef EGL_KHR_stream_fifo
#define EGL_KHR_stream_fifo
// EGLTimeKHR also defined by EGL_KHR_reusable_sync
#ifndef EGL_KHR_reusable_sync
typedef khronos_utime_nanoseconds_t EGLTimeKHR;
#endif
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY eglQueryStreamTimeKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLTimeKHR *value);
#endif
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYSTREAMTIMEKHRPROC)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLTimeKHR *value);
#define EGL_STREAM_FIFO_LENGTH_KHR                  0x31FC
#define EGL_STREAM_TIME_NOW_KHR                     0x31FD
#define EGL_STREAM_TIME_CONSUMER_KHR                0x31FE
#define EGL_STREAM_TIME_PRODUCER_KHR                0x31FF
#endif

/* EGL_NV_stream_producer_eglsurface
*/
#ifndef EGL_NV_stream_producer_eglsurface
#define EGL_NV_stream_producer_eglsurface
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLSurface EGLAPIENTRY eglCreateStreamProducerSurfaceNV(EGLDisplay dpy, EGLConfig config, EGLStreamKHR stream, const EGLint *attrib_list);
#endif
typedef EGLSurface (EGLAPIENTRYP PFNEGLCREATESTREAMPRODUCERSURFACENVPROC)(EGLDisplay dpy, EGLConfig config, EGLStreamKHR stream, const EGLint *attrib_list);
#define EGL_BUFFER_COUNT_NV                         0x321D
#define EGL_STREAM_BIT_NV                           0x0800
#endif

/* EGL_NV_stream_consumer_gltexture
*/
#ifndef EGL_NV_stream_consumer_gltexture
#define EGL_NV_stream_consumer_gltexture
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY eglStreamConsumerGLTextureExternalNV(EGLDisplay dpy, EGLStreamKHR stream);
EGLAPI EGLBoolean EGLAPIENTRY eglStreamConsumerAcquireNV(EGLDisplay dpy, EGLStreamKHR stream);
EGLAPI EGLBoolean EGLAPIENTRY eglStreamConsumerReleaseNV(EGLDisplay dpy, EGLStreamKHR stream);
#endif
typedef EGLBoolean (EGLAPIENTRYP PFNEGLSTREAMCONSUMERGLTEXTUREEXTERNALNVPROC)(EGLDisplay dpy, EGLStreamKHR stream);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLSTREAMCONSUMERACQUIRENVPROC)(EGLDisplay dpy, EGLStreamKHR stream);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLSTREAMCONSUMERRELEASENVPROC)(EGLDisplay dpy, EGLStreamKHR stream);
#define EGL_CONSUMER_ACQUIRE_TIMEOUT_USEC_NV        0x321E
#endif

//
// Supported ANDROID Extensions
//

#ifndef EGL_ANDROID_image_native_buffer
#define EGL_ANDROID_image_native_buffer 1
struct android_native_buffer_t;
#define EGL_NATIVE_BUFFER_ANDROID       0x3140  /* eglCreateImageKHR target */
#endif

//
// Deprecated NV extensions. To be removed, do not use.
//

/* EGL_NV_omx_il_sink
 */
#ifndef EGL_NV_omx_il_sink
#define EGL_NV_omx_il_sink
#define EGL_OPENMAX_IL_BIT_NV  0x0010 /* EGL_RENDERABLE_TYPE bit */
#define EGL_SURFACE_OMX_IL_EVENT_CALLBACK_NV 0x309A /* eglCreate*Surface attribute */
#define EGL_SURFACE_OMX_IL_EMPTY_BUFFER_DONE_CALLBACK_NV 0x309B /* eglCreate*Surface attribute */
#define EGL_SURFACE_OMX_IL_CALLBACK_DATA_NV 0x309C /* eglCreate*Surface attribute */
#define EGL_SURFACE_COMPONENT_HANDLE_NV 0x309D /* eglQuerySurface attribute */
#endif

/* EGL_NV_swap_hint
 */
#ifndef EGL_NV_swap_hint
#define EGL_NV_swap_hint
#define EGL_SWAP_HINT_NV                0x30E4 /* eglCreateWindowSurface attribute name */
#define EGL_FASTEST_NV                  0x30E5 /* eglCreateWindowSurface attribute value */
#endif

/* EGL_CONTEXT_PRIORITY_LEVEL
 */
#ifndef EGL_CONTEXT_PRIORITY_LEVEL_IMG
#define EGL_CONTEXT_PRIORITY_LEVEL_IMG  0X3100
// values
#define EGL_CONTEXT_PRIORITY_HIGH_IMG   0x3101 /* EGL_CONTEXT_PRIORITY_LEVEL_IMG value */
#define EGL_CONTEXT_PRIORITY_MEDIUM_IMG 0x3102 /* EGL_CONTEXT_PRIORITY_LEVEL_IMG value */
#define EGL_CONTEXT_PRIORITY_LOW_IMG    0x3103 /* EGL_CONTEXT_PRIORITY_LEVEL_IMG value */
#endif




//
// Disabled NV extensions. To be removed, do not use.
//

/* EGL_NV_texture_rectangle (reuse analagous WGL enum)
*/
#ifndef EGL_NV_texture_rectangle
#define EGL_NV_texture_rectangle 1
#define EGL_GL_TEXTURE_RECTANGLE_NV_KHR           0x30BB
#define EGL_TEXTURE_RECTANGLE_NV       0x20A2
#endif

#ifdef __cplusplus
}
#endif

#endif
