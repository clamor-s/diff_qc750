/* 
 ** Copyright 2007, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License"); 
 ** you may not use this file except in compliance with the License. 
 ** You may obtain a copy of the License at 
 **
 **     http://www.apache.org/licenses/LICENSE-2.0 
 **
 ** Unless required by applicable law or agreed to in writing, software 
 ** distributed under the License is distributed on an "AS IS" BASIS, 
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 ** See the License for the specific language governing permissions and 
 ** limitations under the License.
 */

#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <hardware/gralloc.h>
#include <system/window.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES/glext.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <cutils/compiler.h>
#include <cutils/properties.h>
#include <cutils/memory.h>

#include <utils/KeyedVector.h>
#include <utils/SortedVector.h>
#include <utils/String8.h>
#include <utils/Trace.h>

#include "egl_impl.h"
#include "egl_tls.h"
#include "glestrace.h"
#include "hooks.h"

#include "egl_display.h"
#include "egl_impl.h"
#include "egl_object.h"
#include "egl_tls.h"
#include "egldefs.h"

using namespace android;

// ----------------------------------------------------------------------------

#define EGL_VERSION_HW_ANDROID  0x3143

struct extention_map_t {
    const char* name;
    __eglMustCastToProperFunctionPointerType address;
};

static const extention_map_t sExtentionMap[] = {
    { "eglLockSurfaceKHR",
            (__eglMustCastToProperFunctionPointerType)&eglLockSurfaceKHR },
    { "eglUnlockSurfaceKHR",
            (__eglMustCastToProperFunctionPointerType)&eglUnlockSurfaceKHR },
    { "eglCreateImageKHR",
            (__eglMustCastToProperFunctionPointerType)&eglCreateImageKHR },
    { "eglDestroyImageKHR",
            (__eglMustCastToProperFunctionPointerType)&eglDestroyImageKHR },
    { "eglCreateStreamKHR",
            (__eglMustCastToProperFunctionPointerType)&eglCreateStreamKHR },
    { "eglDestroyStreamKHR",
            (__eglMustCastToProperFunctionPointerType)&eglDestroyStreamKHR },
    { "eglStreamAttribKHR",
            (__eglMustCastToProperFunctionPointerType)&eglStreamAttribKHR },
    { "eglQueryStreamKHR",
            (__eglMustCastToProperFunctionPointerType)&eglQueryStreamKHR },
    { "eglQueryStreamu64KHR",
            (__eglMustCastToProperFunctionPointerType)&eglQueryStreamu64KHR },
    { "eglQueryStreamTimeKHR",
            (__eglMustCastToProperFunctionPointerType)&eglQueryStreamTimeKHR },
    { "eglCreateStreamProducerSurfaceKHR",
            (__eglMustCastToProperFunctionPointerType)&eglCreateStreamProducerSurfaceKHR },
    { "eglStreamConsumerGLTextureExternalKHR",
            (__eglMustCastToProperFunctionPointerType)&eglStreamConsumerGLTextureExternalKHR },
    { "eglStreamConsumerAcquireKHR",
            (__eglMustCastToProperFunctionPointerType)&eglStreamConsumerAcquireKHR },
    { "eglStreamConsumerReleaseKHR",
            (__eglMustCastToProperFunctionPointerType)&eglStreamConsumerReleaseKHR },
    { "eglCreateSyncKHR",
            (__eglMustCastToProperFunctionPointerType)&eglCreateSyncKHR },
    { "eglDestroySyncKHR",
            (__eglMustCastToProperFunctionPointerType)&eglDestroySyncKHR },
    { "eglClientWaitSyncKHR",
            (__eglMustCastToProperFunctionPointerType)&eglClientWaitSyncKHR },
    { "eglSignalSyncKHR",
            (__eglMustCastToProperFunctionPointerType)&eglSignalSyncKHR },
    { "eglGetSyncAttribKHR",
            (__eglMustCastToProperFunctionPointerType)&eglGetSyncAttribKHR },
    { "eglGetStreamFileDescriptorKHR",
            (__eglMustCastToProperFunctionPointerType)&eglGetStreamFileDescriptorKHR },
    { "eglCreateStreamFromFileDescriptorKHR",
            (__eglMustCastToProperFunctionPointerType)&eglCreateStreamFromFileDescriptorKHR },
    { "eglGetSystemTimeFrequencyNV",
            (__eglMustCastToProperFunctionPointerType)&eglGetSystemTimeFrequencyNV },
    { "eglGetSystemTimeNV",
            (__eglMustCastToProperFunctionPointerType)&eglGetSystemTimeNV },
    { "eglQueryNativeDisplayNV",
            (__eglMustCastToProperFunctionPointerType)&eglQueryNativeDisplayNV },
    { "eglQueryNativeWindowNV",
            (__eglMustCastToProperFunctionPointerType)&eglQueryNativeWindowNV },
    { "eglQueryNativePixmapNV",
            (__eglMustCastToProperFunctionPointerType)&eglQueryNativePixmapNV },
    { "eglCreateFenceSyncNV",
            (__eglMustCastToProperFunctionPointerType)&eglCreateFenceSyncNV },
    { "eglDestroySyncNV",
            (__eglMustCastToProperFunctionPointerType)&eglDestroySyncNV },
    { "eglFenceNV",
            (__eglMustCastToProperFunctionPointerType)&eglFenceNV },
    { "eglClientWaitSyncNV",
            (__eglMustCastToProperFunctionPointerType)&eglClientWaitSyncNV },
    { "eglSignalSyncNV",
            (__eglMustCastToProperFunctionPointerType)&eglSignalSyncNV },
    { "eglGetSyncAttribNV",
            (__eglMustCastToProperFunctionPointerType)&eglGetSyncAttribNV },
    { "eglCreatePerfMonitorNV",
            (__eglMustCastToProperFunctionPointerType)&eglCreatePerfMonitorNV },
    { "eglDestroyPerfMonitorNV",
            (__eglMustCastToProperFunctionPointerType)&eglDestroyPerfMonitorNV },
    { "eglMakeCurrentPerfMonitorNV",
            (__eglMustCastToProperFunctionPointerType)&eglMakeCurrentPerfMonitorNV },
    { "eglGetCurrentPerfMonitorNV",
            (__eglMustCastToProperFunctionPointerType)&eglGetCurrentPerfMonitorNV },
    { "eglGetPerfCountersNV",
            (__eglMustCastToProperFunctionPointerType)&eglGetPerfCountersNV },
    { "eglGetPerfCounterAttribNV",
            (__eglMustCastToProperFunctionPointerType)&eglGetPerfCounterAttribNV },
    { "eglQueryPerfCounterStringNV",
            (__eglMustCastToProperFunctionPointerType)&eglQueryPerfCounterStringNV },
    { "eglPerfMonitorAddCountersNV",
            (__eglMustCastToProperFunctionPointerType)&eglPerfMonitorAddCountersNV },
    { "eglPerfMonitorRemoveCountersNV",
            (__eglMustCastToProperFunctionPointerType)&eglPerfMonitorRemoveCountersNV },
    { "eglPerfMonitorRemoveAllCountersNV",
            (__eglMustCastToProperFunctionPointerType)&eglPerfMonitorRemoveAllCountersNV },
    { "eglPerfMonitorBeginExperimentNV",
            (__eglMustCastToProperFunctionPointerType)&eglPerfMonitorBeginExperimentNV },
    { "eglPerfMonitorEndExperimentNV",
            (__eglMustCastToProperFunctionPointerType)&eglPerfMonitorEndExperimentNV },
    { "eglPerfMonitorBeginPassNV",
            (__eglMustCastToProperFunctionPointerType)&eglPerfMonitorBeginPassNV },
    { "eglPerfMonitorEndPassNV",
            (__eglMustCastToProperFunctionPointerType)&eglPerfMonitorEndPassNV },
    { "eglCreatePerfMarkerNV",
            (__eglMustCastToProperFunctionPointerType)&eglCreatePerfMarkerNV },
    { "eglDestroyPerfMarkerNV",
            (__eglMustCastToProperFunctionPointerType)&eglDestroyPerfMarkerNV },
    { "eglMakeCurrentPerfMarkerNV",
            (__eglMustCastToProperFunctionPointerType)&eglMakeCurrentPerfMarkerNV },
    { "eglGetCurrentPerfMarkerNV",
            (__eglMustCastToProperFunctionPointerType)&eglGetCurrentPerfMarkerNV },
    { "eglGetPerfMarkerCounterNV",
            (__eglMustCastToProperFunctionPointerType)&eglGetPerfMarkerCounterNV },
    { "eglValidatePerfMonitorNV",
            (__eglMustCastToProperFunctionPointerType)&eglValidatePerfMonitorNV },
    { "eglCreateStreamSyncNV",
            (__eglMustCastToProperFunctionPointerType)&eglCreateStreamSyncNV },
};

static DefaultKeyedVector<String8, __eglMustCastToProperFunctionPointerType> sGLExtentionMap;

static void(*findProcAddress(const char* name,
        const extention_map_t* map, size_t n))() {
    for (uint32_t i=0 ; i<n ; i++) {
        if (!strcmp(name, map[i].name)) {
            return map[i].address;
        }
    }
    return NULL;
}

// ----------------------------------------------------------------------------

namespace android {
extern void setGLHooksThreadSpecific(gl_hooks_t const *value);
extern EGLBoolean egl_init_drivers();
extern const __eglMustCastToProperFunctionPointerType gExtensionForwarders[MAX_NUMBER_OF_GL_EXTENSIONS];
extern int gEGLDebugLevel;
extern gl_hooks_t gHooksTrace;
} // namespace android;

// ----------------------------------------------------------------------------

static inline void clearError() { egl_tls_t::clearError(); }
static inline EGLContext getContext() { return egl_tls_t::getContext(); }

// ----------------------------------------------------------------------------

EGLDisplay eglGetDisplay(EGLNativeDisplayType display)
{
    clearError();

    uint32_t index = uint32_t(display);
    if (index >= NUM_DISPLAYS) {
        return setError(EGL_BAD_PARAMETER, EGL_NO_DISPLAY);
    }

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_NO_DISPLAY);
    }

    EGLDisplay dpy = egl_display_t::getFromNativeDisplay(display);
    return dpy;
}

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

EGLBoolean eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    clearError();

    egl_display_ptr dp = get_display(dpy);
    if (!dp) return setError(EGL_BAD_DISPLAY, EGL_FALSE);

    EGLBoolean res = dp->initialize(major, minor);

    return res;
}

EGLBoolean eglTerminate(EGLDisplay dpy)
{
    // NOTE: don't unload the drivers b/c some APIs can be called
    // after eglTerminate() has been called. eglTerminate() only
    // terminates an EGLDisplay, not a EGL itself.

    clearError();

    egl_display_ptr dp = get_display(dpy);
    if (!dp) return setError(EGL_BAD_DISPLAY, EGL_FALSE);

    EGLBoolean res = dp->terminate();
    
    return res;
}

// ----------------------------------------------------------------------------
// configuration
// ----------------------------------------------------------------------------

EGLBoolean eglGetConfigs(   EGLDisplay dpy,
                            EGLConfig *configs,
                            EGLint config_size, EGLint *num_config)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    if (num_config==0) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean res = EGL_FALSE;
    *num_config = 0;

    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso) {
        res = cnx->egl.eglGetConfigs(
                dp->disp.dpy, configs, config_size, num_config);
    }

    return res;
}

EGLBoolean eglChooseConfig( EGLDisplay dpy, const EGLint *attrib_list,
                            EGLConfig *configs, EGLint config_size,
                            EGLint *num_config)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    if (num_config==0) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean res = EGL_FALSE;
    *num_config = 0;

    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso) {
        if (attrib_list) {
            char value[PROPERTY_VALUE_MAX];
            property_get("debug.egl.force_msaa", value, "false");

            if (!strcmp(value, "true")) {
                size_t attribCount = 0;
                EGLint attrib = attrib_list[0];

                // Only enable MSAA if the context is OpenGL ES 2.0 and
                // if no caveat is requested
                const EGLint *attribRendererable = NULL;
                const EGLint *attribCaveat = NULL;

                // Count the number of attributes and look for
                // EGL_RENDERABLE_TYPE and EGL_CONFIG_CAVEAT
                while (attrib != EGL_NONE) {
                    attrib = attrib_list[attribCount];
                    switch (attrib) {
                        case EGL_RENDERABLE_TYPE:
                            attribRendererable = &attrib_list[attribCount];
                            break;
                        case EGL_CONFIG_CAVEAT:
                            attribCaveat = &attrib_list[attribCount];
                            break;
                    }
                    attribCount++;
                }

                if (attribRendererable && attribRendererable[1] == EGL_OPENGL_ES2_BIT &&
                        (!attribCaveat || attribCaveat[1] != EGL_NONE)) {
    
                    // Insert 2 extra attributes to force-enable MSAA 4x
                    EGLint aaAttribs[attribCount + 4];
                    aaAttribs[0] = EGL_SAMPLE_BUFFERS;
                    aaAttribs[1] = 1;
                    aaAttribs[2] = EGL_SAMPLES;
                    aaAttribs[3] = 4;

                    memcpy(&aaAttribs[4], attrib_list, attribCount * sizeof(EGLint));

                    EGLint numConfigAA;
                    EGLBoolean resAA = cnx->egl.eglChooseConfig(
                            dp->disp.dpy, aaAttribs, configs, config_size, &numConfigAA);

                    if (resAA == EGL_TRUE && numConfigAA > 0) {
                        ALOGD("Enabling MSAA 4x");
                        *num_config = numConfigAA;
                        return resAA;
                    }
                }
            }
        }

        res = cnx->egl.eglChooseConfig(
                dp->disp.dpy, attrib_list, configs, config_size, num_config);
    }
    return res;
}

EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config,
        EGLint attribute, EGLint *value)
{
    clearError();

    egl_connection_t* cnx = NULL;
    const egl_display_ptr dp = validate_display_connection(dpy, cnx);
    if (!dp) return EGL_FALSE;
    
    return cnx->egl.eglGetConfigAttrib(
            dp->disp.dpy, config, attribute, value);
}

// ----------------------------------------------------------------------------
// surfaces
// ----------------------------------------------------------------------------

EGLSurface eglCreateWindowSurface(  EGLDisplay dpy, EGLConfig config,
                                    NativeWindowType window,
                                    const EGLint *attrib_list)
{
    clearError();

    egl_connection_t* cnx = NULL;
    egl_display_ptr dp = validate_display_connection(dpy, cnx);
    if (dp) {
        EGLDisplay iDpy = dp->disp.dpy;
        EGLint format;

        if (native_window_api_connect(window, NATIVE_WINDOW_API_EGL) != OK) {
            ALOGE("EGLNativeWindowType %p already connected to another API",
                    window);
            return setError(EGL_BAD_NATIVE_WINDOW, EGL_NO_SURFACE);
        }

        // set the native window's buffers format to match this config
        if (cnx->egl.eglGetConfigAttrib(iDpy,
                config, EGL_NATIVE_VISUAL_ID, &format)) {
            if (format != 0) {
                int err = native_window_set_buffers_format(window, format);
                if (err != 0) {
                    ALOGE("error setting native window pixel format: %s (%d)",
                            strerror(-err), err);
                    native_window_api_disconnect(window, NATIVE_WINDOW_API_EGL);
                    return setError(EGL_BAD_NATIVE_WINDOW, EGL_NO_SURFACE);
                }
            }
        }

        // the EGL spec requires that a new EGLSurface default to swap interval
        // 1, so explicitly set that on the window here.
        ANativeWindow* anw = reinterpret_cast<ANativeWindow*>(window);
        anw->setSwapInterval(anw, 1);

        EGLSurface surface = cnx->egl.eglCreateWindowSurface(
                iDpy, config, window, attrib_list);
        if (surface != EGL_NO_SURFACE) {
            egl_surface_t* s = new egl_surface_t(dp.get(), config, window,
                    surface, cnx);
            return s;
        }

        // EGLSurface creation failed
        native_window_set_buffers_format(window, 0);
        native_window_api_disconnect(window, NATIVE_WINDOW_API_EGL);
    }
    return EGL_NO_SURFACE;
}

EGLSurface eglCreatePixmapSurface(  EGLDisplay dpy, EGLConfig config,
                                    NativePixmapType pixmap,
                                    const EGLint *attrib_list)
{
    clearError();

    egl_connection_t* cnx = NULL;
    egl_display_ptr dp = validate_display_connection(dpy, cnx);
    if (dp) {
        EGLSurface surface = cnx->egl.eglCreatePixmapSurface(
                dp->disp.dpy, config, pixmap, attrib_list);
        if (surface != EGL_NO_SURFACE) {
            egl_surface_t* s = new egl_surface_t(dp.get(), config, NULL,
                    surface, cnx);
            return s;
        }
    }
    return EGL_NO_SURFACE;
}

EGLSurface eglCreatePbufferSurface( EGLDisplay dpy, EGLConfig config,
                                    const EGLint *attrib_list)
{
    clearError();

    egl_connection_t* cnx = NULL;
    egl_display_ptr dp = validate_display_connection(dpy, cnx);
    if (dp) {
        EGLSurface surface = cnx->egl.eglCreatePbufferSurface(
                dp->disp.dpy, config, attrib_list);
        if (surface != EGL_NO_SURFACE) {
            egl_surface_t* s = new egl_surface_t(dp.get(), config, NULL,
                    surface, cnx);
            return s;
        }
    }
    return EGL_NO_SURFACE;
}
                                    
EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    SurfaceRef _s(dp.get(), surface);
    if (!_s.get())
        return setError(EGL_BAD_SURFACE, EGL_FALSE);

    egl_surface_t * const s = get_surface(surface);
    EGLBoolean result = s->cnx->egl.eglDestroySurface(dp->disp.dpy, s->surface);
    if (result == EGL_TRUE) {
        _s.terminate();
    }
    return result;
}

EGLBoolean eglQuerySurface( EGLDisplay dpy, EGLSurface surface,
                            EGLint attribute, EGLint *value)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    SurfaceRef _s(dp.get(), surface);
    if (!_s.get())
        return setError(EGL_BAD_SURFACE, EGL_FALSE);

    egl_surface_t const * const s = get_surface(surface);
    return s->cnx->egl.eglQuerySurface(
            dp->disp.dpy, s->surface, attribute, value);
}

void EGLAPI eglBeginFrame(EGLDisplay dpy, EGLSurface surface) {
    ATRACE_CALL();
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) {
        return;
    }

    SurfaceRef _s(dp.get(), surface);
    if (!_s.get()) {
        setError(EGL_BAD_SURFACE, EGL_FALSE);
        return;
    }

    int64_t timestamp = systemTime(SYSTEM_TIME_MONOTONIC);

    egl_surface_t const * const s = get_surface(surface);
    native_window_set_buffers_timestamp(s->win.get(), timestamp);
}

// ----------------------------------------------------------------------------
// Contexts
// ----------------------------------------------------------------------------

EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config,
                            EGLContext share_list, const EGLint *attrib_list)
{
    clearError();

    egl_connection_t* cnx = NULL;
    const egl_display_ptr dp = validate_display_connection(dpy, cnx);
    if (dp) {
        if (share_list != EGL_NO_CONTEXT) {
            if (!ContextRef(dp.get(), share_list).get()) {
                return setError(EGL_BAD_CONTEXT, EGL_NO_CONTEXT);
            }
            egl_context_t* const c = get_context(share_list);
            share_list = c->context;
        }
        EGLContext context = cnx->egl.eglCreateContext(
                dp->disp.dpy, config, share_list, attrib_list);
        if (context != EGL_NO_CONTEXT) {
            // figure out if it's a GLESv1 or GLESv2
            int version = 0;
            if (attrib_list) {
                while (*attrib_list != EGL_NONE) {
                    GLint attr = *attrib_list++;
                    GLint value = *attrib_list++;
                    if (attr == EGL_CONTEXT_CLIENT_VERSION) {
                        if (value == 1) {
                            version = egl_connection_t::GLESv1_INDEX;
                        } else if (value == 2) {
                            version = egl_connection_t::GLESv2_INDEX;
                        }
                    }
                };
            }
            egl_context_t* c = new egl_context_t(dpy, context, config, cnx,
                    version);
#if EGL_TRACE
            if (gEGLDebugLevel > 0)
                GLTrace_eglCreateContext(version, c);
#endif
            return c;
        } else {
            EGLint error = eglGetError();
            ALOGE_IF(error == EGL_SUCCESS,
                    "eglCreateContext(%p, %p, %p, %p) returned EGL_NO_CONTEXT "
                    "but no EGL error!",
                    dpy, config, share_list, attrib_list);
        }
    }
    return EGL_NO_CONTEXT;
}

EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp)
        return EGL_FALSE;

    ContextRef _c(dp.get(), ctx);
    if (!_c.get())
        return setError(EGL_BAD_CONTEXT, EGL_FALSE);
    
    egl_context_t * const c = get_context(ctx);
    EGLBoolean result = c->cnx->egl.eglDestroyContext(dp->disp.dpy, c->context);
    if (result == EGL_TRUE) {
        _c.terminate();
    }
    return result;
}

EGLBoolean eglMakeCurrent(  EGLDisplay dpy, EGLSurface draw,
                            EGLSurface read, EGLContext ctx)
{
    clearError();

    egl_display_ptr dp = validate_display(dpy);
    if (!dp) return setError(EGL_BAD_DISPLAY, EGL_FALSE);

    // If ctx is not EGL_NO_CONTEXT, read is not EGL_NO_SURFACE, or draw is not
    // EGL_NO_SURFACE, then an EGL_NOT_INITIALIZED error is generated if dpy is
    // a valid but uninitialized display.
    if ( (ctx != EGL_NO_CONTEXT) || (read != EGL_NO_SURFACE) ||
         (draw != EGL_NO_SURFACE) ) {
        if (!dp->isReady()) return setError(EGL_NOT_INITIALIZED, EGL_FALSE);
    }

    // get a reference to the object passed in
    ContextRef _c(dp.get(), ctx);
    SurfaceRef _d(dp.get(), draw);
    SurfaceRef _r(dp.get(), read);

    // validate the context (if not EGL_NO_CONTEXT)
    if ((ctx != EGL_NO_CONTEXT) && !_c.get()) {
        // EGL_NO_CONTEXT is valid
        return setError(EGL_BAD_CONTEXT, EGL_FALSE);
    }

    // these are the underlying implementation's object
    EGLContext impl_ctx  = EGL_NO_CONTEXT;
    EGLSurface impl_draw = EGL_NO_SURFACE;
    EGLSurface impl_read = EGL_NO_SURFACE;

    // these are our objects structs passed in
    egl_context_t       * c = NULL;
    egl_surface_t const * d = NULL;
    egl_surface_t const * r = NULL;

    // these are the current objects structs
    egl_context_t * cur_c = get_context(getContext());
    
    if (ctx != EGL_NO_CONTEXT) {
        c = get_context(ctx);
        impl_ctx = c->context;
    } else {
        // no context given, use the implementation of the current context
        if (draw != EGL_NO_SURFACE || read != EGL_NO_SURFACE) {
            // calling eglMakeCurrent( ..., !=0, !=0, EGL_NO_CONTEXT);
            return setError(EGL_BAD_MATCH, EGL_FALSE);
        }
        if (cur_c == NULL) {
            // no current context
            // not an error, there is just no current context.
            return EGL_TRUE;
        }
    }

    // retrieve the underlying implementation's draw EGLSurface
    if (draw != EGL_NO_SURFACE) {
        if (!_d.get()) return setError(EGL_BAD_SURFACE, EGL_FALSE);
        d = get_surface(draw);
        impl_draw = d->surface;
    }

    // retrieve the underlying implementation's read EGLSurface
    if (read != EGL_NO_SURFACE) {
        if (!_r.get()) return setError(EGL_BAD_SURFACE, EGL_FALSE);
        r = get_surface(read);
        impl_read = r->surface;
    }


    EGLBoolean result = dp->makeCurrent(c, cur_c,
            draw, read, ctx,
            impl_draw, impl_read, impl_ctx);

    if (result == EGL_TRUE) {
        if (c) {
            setGLHooksThreadSpecific(c->cnx->hooks[c->version]);
            egl_tls_t::setContext(ctx);
#if EGL_TRACE
            if (gEGLDebugLevel > 0)
                GLTrace_eglMakeCurrent(c->version, c->cnx->hooks[c->version], ctx);
#endif
            _c.acquire();
            _r.acquire();
            _d.acquire();
        } else {
            setGLHooksThreadSpecific(&gHooksNoContext);
            egl_tls_t::setContext(EGL_NO_CONTEXT);
        }
    } else {
        // this will ALOGE the error
        result = setError(c->cnx->egl.eglGetError(), EGL_FALSE);
    }
    return result;
}


EGLBoolean eglQueryContext( EGLDisplay dpy, EGLContext ctx,
                            EGLint attribute, EGLint *value)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    ContextRef _c(dp.get(), ctx);
    if (!_c.get()) return setError(EGL_BAD_CONTEXT, EGL_FALSE);

    egl_context_t * const c = get_context(ctx);
    return c->cnx->egl.eglQueryContext(
            dp->disp.dpy, c->context, attribute, value);

}

EGLContext eglGetCurrentContext(void)
{
    // could be called before eglInitialize(), but we wouldn't have a context
    // then, and this function would correctly return EGL_NO_CONTEXT.

    clearError();

    EGLContext ctx = getContext();
    return ctx;
}

EGLSurface eglGetCurrentSurface(EGLint readdraw)
{
    // could be called before eglInitialize(), but we wouldn't have a context
    // then, and this function would correctly return EGL_NO_SURFACE.

    clearError();

    EGLContext ctx = getContext();
    if (ctx) {
        egl_context_t const * const c = get_context(ctx);
        if (!c) return setError(EGL_BAD_CONTEXT, EGL_NO_SURFACE);
        switch (readdraw) {
            case EGL_READ: return c->read;
            case EGL_DRAW: return c->draw;            
            default: return setError(EGL_BAD_PARAMETER, EGL_NO_SURFACE);
        }
    }
    return EGL_NO_SURFACE;
}

EGLDisplay eglGetCurrentDisplay(void)
{
    // could be called before eglInitialize(), but we wouldn't have a context
    // then, and this function would correctly return EGL_NO_DISPLAY.

    clearError();

    EGLContext ctx = getContext();
    if (ctx) {
        egl_context_t const * const c = get_context(ctx);
        if (!c) return setError(EGL_BAD_CONTEXT, EGL_NO_SURFACE);
        return c->dpy;
    }
    return EGL_NO_DISPLAY;
}

EGLBoolean eglWaitGL(void)
{
    clearError();

    egl_connection_t* const cnx = &gEGLImpl;
    if (!cnx->dso)
        return setError(EGL_BAD_CONTEXT, EGL_FALSE);

    return cnx->egl.eglWaitGL();
}

EGLBoolean eglWaitNative(EGLint engine)
{
    clearError();

    egl_connection_t* const cnx = &gEGLImpl;
    if (!cnx->dso)
        return setError(EGL_BAD_CONTEXT, EGL_FALSE);

    return cnx->egl.eglWaitNative(engine);
}

EGLint eglGetError(void)
{
    EGLint err = EGL_SUCCESS;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso) {
        err = cnx->egl.eglGetError();
    }
    if (err == EGL_SUCCESS) {
        err = egl_tls_t::getError();
    }
    return err;
}

__eglMustCastToProperFunctionPointerType eglGetProcAddress(const char *procname)
{
    // eglGetProcAddress() could be the very first function called
    // in which case we must make sure we've initialized ourselves, this
    // happens the first time egl_get_display() is called.

    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        setError(EGL_BAD_PARAMETER, NULL);
        return  NULL;
    }

    // These extensions should not be exposed to applications. They're used
    // internally by the Android EGL layer.
    if (!strcmp(procname, "eglSetBlobCacheFuncsANDROID") ||
        !strcmp(procname, "eglDupNativeFenceFDANDROID") ||
        !strcmp(procname, "eglWaitSyncANDROID") ||
        !strcmp(procname, "eglHibernateProcessIMG") ||
        !strcmp(procname, "eglAwakenProcessIMG")) {
        return NULL;
    }

    // Some functions are preloaded in sExtentionMap, we query them from there
    // (though we won't use this table for other extensions)
    __eglMustCastToProperFunctionPointerType addr;
    addr = findProcAddress(procname, sExtentionMap, NELEM(sExtentionMap));
    if (addr) return addr;

    if (gEGLImpl.dso && gEGLImpl.egl.eglGetProcAddress) {
        addr = gEGLImpl.egl.eglGetProcAddress(procname);
    }

    return addr;
}

class FrameCompletionThread : public Thread {
public:

    static void queueSync(EGLSyncKHR sync) {
        static sp<FrameCompletionThread> thread(new FrameCompletionThread);
        static bool running = false;
        if (!running) {
            thread->run("GPUFrameCompletion");
            running = true;
        }
        {
            Mutex::Autolock lock(thread->mMutex);
            ScopedTrace st(ATRACE_TAG, String8::format("kicked off frame %d",
                    thread->mFramesQueued).string());
            thread->mQueue.push_back(sync);
            thread->mCondition.signal();
            thread->mFramesQueued++;
            ATRACE_INT("GPU Frames Outstanding", thread->mQueue.size());
        }
    }

private:
    FrameCompletionThread() : mFramesQueued(0), mFramesCompleted(0) {}

    virtual bool threadLoop() {
        EGLSyncKHR sync;
        uint32_t frameNum;
        {
            Mutex::Autolock lock(mMutex);
            while (mQueue.isEmpty()) {
                mCondition.wait(mMutex);
            }
            sync = mQueue[0];
            frameNum = mFramesCompleted;
        }
        EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        {
            ScopedTrace st(ATRACE_TAG, String8::format("waiting for frame %d",
                    frameNum).string());
            EGLint result = eglClientWaitSyncKHR(dpy, sync, 0, EGL_FOREVER_KHR);
            if (result == EGL_FALSE) {
                ALOGE("FrameCompletion: error waiting for fence: %#x", eglGetError());
            } else if (result == EGL_TIMEOUT_EXPIRED_KHR) {
                ALOGE("FrameCompletion: timeout waiting for fence");
            }
            eglDestroySyncKHR(dpy, sync);
        }
        {
            Mutex::Autolock lock(mMutex);
            mQueue.removeAt(0);
            mFramesCompleted++;
            ATRACE_INT("GPU Frames Outstanding", mQueue.size());
        }
        return true;
    }

    uint32_t mFramesQueued;
    uint32_t mFramesCompleted;
    Vector<EGLSyncKHR> mQueue;
    Condition mCondition;
    Mutex mMutex;
};

EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface draw)
{
    ATRACE_CALL();
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    SurfaceRef _s(dp.get(), draw);
    if (!_s.get())
        return setError(EGL_BAD_SURFACE, EGL_FALSE);

#if EGL_TRACE
    if (gEGLDebugLevel > 0)
        GLTrace_eglSwapBuffers(dpy, draw);
#endif

    egl_surface_t const * const s = get_surface(draw);

    if (CC_UNLIKELY(dp->finishOnSwap)) {
        uint32_t pixel;
        egl_context_t * const c = get_context( egl_tls_t::getContext() );
        if (c) {
            // glReadPixels() ensures that the frame is complete
            s->cnx->hooks[c->version]->gl.glReadPixels(0,0,1,1,
                    GL_RGBA,GL_UNSIGNED_BYTE,&pixel);
        }
    }

    EGLBoolean result = s->cnx->egl.eglSwapBuffers(dp->disp.dpy, s->surface);

    if (CC_UNLIKELY(dp->traceGpuCompletion)) {
        EGLSyncKHR sync = EGL_NO_SYNC_KHR;
        {
            sync = eglCreateSyncKHR(dpy, EGL_SYNC_FENCE_KHR, NULL);
        }
        if (sync != EGL_NO_SYNC_KHR) {
            FrameCompletionThread::queueSync(sync);
        }
    }

    return result;
}

EGLBoolean eglCopyBuffers(  EGLDisplay dpy, EGLSurface surface,
                            NativePixmapType target)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    SurfaceRef _s(dp.get(), surface);
    if (!_s.get())
        return setError(EGL_BAD_SURFACE, EGL_FALSE);

    egl_surface_t const * const s = get_surface(surface);
    return s->cnx->egl.eglCopyBuffers(dp->disp.dpy, s->surface, target);
}

const char* eglQueryString(EGLDisplay dpy, EGLint name)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return (const char *) NULL;

    switch (name) {
        case EGL_VENDOR:
            return dp->getVendorString();
        case EGL_VERSION:
            return dp->getVersionString();
        case EGL_EXTENSIONS:
            return dp->getExtensionString();
        case EGL_CLIENT_APIS:
            return dp->getClientApiString();
        case EGL_VERSION_HW_ANDROID:
            return dp->disp.queryString.version;
    }
    return setError(EGL_BAD_PARAMETER, (const char *)0);
}


// ----------------------------------------------------------------------------
// EGL 1.1
// ----------------------------------------------------------------------------

EGLBoolean eglSurfaceAttrib(
        EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    SurfaceRef _s(dp.get(), surface);
    if (!_s.get())
        return setError(EGL_BAD_SURFACE, EGL_FALSE);

    egl_surface_t const * const s = get_surface(surface);
    if (s->cnx->egl.eglSurfaceAttrib) {
        return s->cnx->egl.eglSurfaceAttrib(
                dp->disp.dpy, s->surface, attribute, value);
    }
    return setError(EGL_BAD_SURFACE, EGL_FALSE);
}

EGLBoolean eglBindTexImage(
        EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    SurfaceRef _s(dp.get(), surface);
    if (!_s.get())
        return setError(EGL_BAD_SURFACE, EGL_FALSE);

    egl_surface_t const * const s = get_surface(surface);
    if (s->cnx->egl.eglBindTexImage) {
        return s->cnx->egl.eglBindTexImage(
                dp->disp.dpy, s->surface, buffer);
    }
    return setError(EGL_BAD_SURFACE, EGL_FALSE);
}

EGLBoolean eglReleaseTexImage(
        EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    SurfaceRef _s(dp.get(), surface);
    if (!_s.get())
        return setError(EGL_BAD_SURFACE, EGL_FALSE);

    egl_surface_t const * const s = get_surface(surface);
    if (s->cnx->egl.eglReleaseTexImage) {
        return s->cnx->egl.eglReleaseTexImage(
                dp->disp.dpy, s->surface, buffer);
    }
    return setError(EGL_BAD_SURFACE, EGL_FALSE);
}

EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLBoolean res = EGL_TRUE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglSwapInterval) {
        res = cnx->egl.eglSwapInterval(dp->disp.dpy, interval);
    }

    return res;
}


// ----------------------------------------------------------------------------
// EGL 1.2
// ----------------------------------------------------------------------------

EGLBoolean eglWaitClient(void)
{
    clearError();

    egl_connection_t* const cnx = &gEGLImpl;
    if (!cnx->dso)
        return setError(EGL_BAD_CONTEXT, EGL_FALSE);

    EGLBoolean res;
    if (cnx->egl.eglWaitClient) {
        res = cnx->egl.eglWaitClient();
    } else {
        res = cnx->egl.eglWaitGL();
    }
    return res;
}

EGLBoolean eglBindAPI(EGLenum api)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    // bind this API on all EGLs
    EGLBoolean res = EGL_TRUE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglBindAPI) {
        res = cnx->egl.eglBindAPI(api);
    }
    return res;
}

EGLenum eglQueryAPI(void)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglQueryAPI) {
        return cnx->egl.eglQueryAPI();
    }

    // or, it can only be OpenGL ES
    return EGL_OPENGL_ES_API;
}

EGLBoolean eglReleaseThread(void)
{
    clearError();

    // If there is context bound to the thread, release it
    egl_display_t::loseCurrent(get_context(getContext()));

    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglReleaseThread) {
        cnx->egl.eglReleaseThread();
    }

    egl_tls_t::clearTLS();
#if EGL_TRACE
    if (gEGLDebugLevel > 0)
        GLTrace_eglReleaseThread();
#endif
    return EGL_TRUE;
}

EGLSurface eglCreatePbufferFromClientBuffer(
          EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer,
          EGLConfig config, const EGLint *attrib_list)
{
    clearError();

    egl_connection_t* cnx = NULL;
    const egl_display_ptr dp = validate_display_connection(dpy, cnx);
    if (!dp) return EGL_FALSE;
    if (cnx->egl.eglCreatePbufferFromClientBuffer) {
        return cnx->egl.eglCreatePbufferFromClientBuffer(
                dp->disp.dpy, buftype, buffer, config, attrib_list);
    }
    return setError(EGL_BAD_CONFIG, EGL_NO_SURFACE);
}

// ----------------------------------------------------------------------------
// EGL_EGLEXT_VERSION 3
// ----------------------------------------------------------------------------

EGLBoolean eglLockSurfaceKHR(EGLDisplay dpy, EGLSurface surface,
        const EGLint *attrib_list)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    SurfaceRef _s(dp.get(), surface);
    if (!_s.get())
        return setError(EGL_BAD_SURFACE, EGL_FALSE);

    egl_surface_t const * const s = get_surface(surface);
    if (s->cnx->egl.eglLockSurfaceKHR) {
        return s->cnx->egl.eglLockSurfaceKHR(
                dp->disp.dpy, s->surface, attrib_list);
    }
    return setError(EGL_BAD_DISPLAY, EGL_FALSE);
}

EGLBoolean eglUnlockSurfaceKHR(EGLDisplay dpy, EGLSurface surface)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    SurfaceRef _s(dp.get(), surface);
    if (!_s.get())
        return setError(EGL_BAD_SURFACE, EGL_FALSE);

    egl_surface_t const * const s = get_surface(surface);
    if (s->cnx->egl.eglUnlockSurfaceKHR) {
        return s->cnx->egl.eglUnlockSurfaceKHR(dp->disp.dpy, s->surface);
    }
    return setError(EGL_BAD_DISPLAY, EGL_FALSE);
}

EGLImageKHR eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target,
        EGLClientBuffer buffer, const EGLint *attrib_list)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_NO_IMAGE_KHR;

    ContextRef _c(dp.get(), ctx);
    egl_context_t * const c = _c.get();

    EGLImageKHR result = EGL_NO_IMAGE_KHR;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglCreateImageKHR) {
        result = cnx->egl.eglCreateImageKHR(
                dp->disp.dpy,
                c ? c->context : EGL_NO_CONTEXT,
                target, buffer, attrib_list);
    }
    return result;
}

EGLBoolean eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR img)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglDestroyImageKHR) {
        result = cnx->egl.eglDestroyImageKHR(dp->disp.dpy, img);
    }
    return result;
}

// ----------------------------------------------------------------------------
// EGL_EGLEXT_VERSION 5
// ----------------------------------------------------------------------------


EGLSyncKHR eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_NO_SYNC_KHR;

    EGLSyncKHR result = EGL_NO_SYNC_KHR;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglCreateSyncKHR) {
        result = cnx->egl.eglCreateSyncKHR(dp->disp.dpy, type, attrib_list);
    }
    return result;
}

EGLBoolean eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglDestroySyncKHR) {
        result = cnx->egl.eglDestroySyncKHR(dp->disp.dpy, sync);
    }
    return result;
}

EGLint eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync,
        EGLint flags, EGLTimeKHR timeout)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglClientWaitSyncKHR) {
        result = cnx->egl.eglClientWaitSyncKHR(
                dp->disp.dpy, sync, flags, timeout);
    }
    return result;
}

EGLBoolean eglSignalSyncKHR(EGLDisplay dpy, EGLSyncKHR sync,
        EGLenum mode)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglSignalSyncKHR) {
        result = cnx->egl.eglSignalSyncKHR(
                dp->disp.dpy, sync, mode);
    }
    return result;
}

EGLBoolean eglGetSyncAttribKHR(EGLDisplay dpy, EGLSyncKHR sync,
        EGLint attribute, EGLint *value)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglGetSyncAttribKHR) {
        result = cnx->egl.eglGetSyncAttribKHR(
                dp->disp.dpy, sync, attribute, value);
    }
    return result;
}

EGLStreamKHR eglCreateStreamKHR(EGLDisplay dpy, const EGLint *attrib_list)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_NO_STREAM_KHR;

    EGLStreamKHR result = EGL_NO_STREAM_KHR;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglCreateStreamKHR) {
        result = cnx->egl.eglCreateStreamKHR(
                dp->disp.dpy, attrib_list);
    }
    return result;
}

EGLBoolean eglDestroyStreamKHR(EGLDisplay dpy, EGLStreamKHR stream)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglDestroyStreamKHR) {
        result = cnx->egl.eglDestroyStreamKHR(
                dp->disp.dpy, stream);
    }
    return result;
}

EGLBoolean eglStreamAttribKHR(EGLDisplay dpy, EGLStreamKHR stream,
        EGLenum attribute, EGLint value)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglStreamAttribKHR) {
        result = cnx->egl.eglStreamAttribKHR(
                dp->disp.dpy, stream, attribute, value);
    }
    return result;
}

EGLBoolean eglQueryStreamKHR(EGLDisplay dpy, EGLStreamKHR stream,
        EGLenum attribute, EGLint *value)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglQueryStreamKHR) {
        result = cnx->egl.eglQueryStreamKHR(
                dp->disp.dpy, stream, attribute, value);
    }
    return result;
}

EGLBoolean eglQueryStreamu64KHR(EGLDisplay dpy, EGLStreamKHR stream,
        EGLenum attribute, EGLuint64KHR *value)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglQueryStreamu64KHR) {
        result = cnx->egl.eglQueryStreamu64KHR(
                dp->disp.dpy, stream, attribute, value);
    }
    return result;
}

EGLBoolean eglQueryStreamTimeKHR(EGLDisplay dpy, EGLStreamKHR stream,
        EGLenum attribute, EGLTimeKHR *value)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglQueryStreamTimeKHR) {
        result = cnx->egl.eglQueryStreamTimeKHR(
                dp->disp.dpy, stream, attribute, value);
    }
    return result;
}

EGLSurface eglCreateStreamProducerSurfaceKHR(EGLDisplay dpy, EGLConfig config,
        EGLStreamKHR stream, const EGLint *attrib_list)
{
    clearError();

    egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_NO_SURFACE;

    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglCreateStreamProducerSurfaceKHR) {
        EGLSurface surface = cnx->egl.eglCreateStreamProducerSurfaceKHR(
                dp->disp.dpy, config, stream, attrib_list);
        if (surface != EGL_NO_SURFACE) {
            egl_surface_t* s = new egl_surface_t(dp.get(), config, NULL,
                    surface, cnx);
            return s;
        }
    }
    return EGL_NO_SURFACE;
}

EGLBoolean eglStreamConsumerGLTextureExternalKHR(EGLDisplay dpy,
        EGLStreamKHR stream)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglStreamConsumerGLTextureExternalKHR) {
        result = cnx->egl.eglStreamConsumerGLTextureExternalKHR(
                dp->disp.dpy, stream);
    }
    return result;
}

EGLBoolean eglStreamConsumerAcquireKHR(EGLDisplay dpy,
        EGLStreamKHR stream)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglStreamConsumerAcquireKHR) {
        result = cnx->egl.eglStreamConsumerAcquireKHR(
                dp->disp.dpy, stream);
    }
    return result;
}

EGLBoolean eglStreamConsumerReleaseKHR(EGLDisplay dpy,
        EGLStreamKHR stream)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglStreamConsumerReleaseKHR) {
        result = cnx->egl.eglStreamConsumerReleaseKHR(
                dp->disp.dpy, stream);
    }
    return result;
}

EGLNativeFileDescriptorKHR eglGetStreamFileDescriptorKHR(
        EGLDisplay dpy, EGLStreamKHR stream)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_NO_FILE_DESCRIPTOR_KHR;

    EGLNativeFileDescriptorKHR result = EGL_NO_FILE_DESCRIPTOR_KHR;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglGetStreamFileDescriptorKHR) {
        result = cnx->egl.eglGetStreamFileDescriptorKHR(
                dp->disp.dpy, stream);
    }
    return result;
}

EGLStreamKHR eglCreateStreamFromFileDescriptorKHR(
        EGLDisplay dpy, EGLNativeFileDescriptorKHR file_descriptor)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_NO_STREAM_KHR;

    EGLStreamKHR result = EGL_NO_STREAM_KHR;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglCreateStreamFromFileDescriptorKHR) {
        result = cnx->egl.eglCreateStreamFromFileDescriptorKHR(
                dp->disp.dpy, file_descriptor);
    }
    return result;
}

// ----------------------------------------------------------------------------
// ANDROID extensions
// ----------------------------------------------------------------------------

EGLint eglDupNativeFenceFDANDROID(EGLDisplay dpy, EGLSyncKHR sync)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_NO_NATIVE_FENCE_FD_ANDROID;

    EGLint result = EGL_NO_NATIVE_FENCE_FD_ANDROID;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglDupNativeFenceFDANDROID) {
        result = cnx->egl.eglDupNativeFenceFDANDROID(dp->disp.dpy, sync);
    }
    return result;
}

EGLint eglWaitSyncANDROID(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_NO_NATIVE_FENCE_FD_ANDROID;

    EGLint result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglWaitSyncANDROID) {
        result = cnx->egl.eglWaitSyncANDROID(dp->disp.dpy, sync, flags);
    }
    return result;
}

// ----------------------------------------------------------------------------
// NVIDIA extensions
// ----------------------------------------------------------------------------
EGLuint64NV eglGetSystemTimeFrequencyNV()
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLuint64NV ret = 0;
    egl_connection_t* const cnx = &gEGLImpl;

    if (cnx->dso && cnx->egl.eglGetSystemTimeFrequencyNV) {
        return cnx->egl.eglGetSystemTimeFrequencyNV();
    }

    return setErrorQuiet(EGL_BAD_DISPLAY, 0);
}

EGLuint64NV eglGetSystemTimeNV()
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLuint64NV ret = 0;
    egl_connection_t* const cnx = &gEGLImpl;

    if (cnx->dso && cnx->egl.eglGetSystemTimeNV) {
        return cnx->egl.eglGetSystemTimeNV();
    }

    return setErrorQuiet(EGL_BAD_DISPLAY, 0);
}

EGLBoolean eglQueryNativeDisplayNV(EGLDisplay dpy,
        EGLNativeDisplayType *display_id)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;

    if (cnx->dso && cnx->egl.eglQueryNativeDisplayNV) {
        result = cnx->egl.eglQueryNativeDisplayNV(
                dp->disp.dpy, display_id);
    }

    return result;
}

EGLBoolean eglQueryNativeWindowNV(EGLDisplay dpy,
        EGLSurface surf, EGLNativeWindowType* window)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    SurfaceRef _s(dp.get(), surf);
    if (!_s.get())
        return setError(EGL_BAD_SURFACE, EGL_FALSE);

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;

    egl_surface_t * const s = get_surface(surf);
    if (cnx->dso && cnx->egl.eglQueryNativeWindowNV) {
        result = cnx->egl.eglQueryNativeWindowNV(
                dp->disp.dpy, s->surface, window);
    }

    return result;
}

EGLBoolean eglQueryNativePixmapNV(EGLDisplay dpy,
        EGLSurface surf, EGLNativePixmapType* pixmap)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    SurfaceRef _s(dp.get(), surf);
    if (!_s.get())
        return setError(EGL_BAD_SURFACE, EGL_FALSE);

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;

    egl_surface_t * const s = get_surface(surf);
    if (cnx->dso && cnx->egl.eglQueryNativePixmapNV) {
        result = cnx->egl.eglQueryNativePixmapNV(
                dp->disp.dpy, s->surface, pixmap);
    }

    return result;
}

EGLSyncNV eglCreateFenceSyncNV(EGLDisplay dpy,
        EGLenum condition, const EGLint* attrib_list)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLSyncNV result = EGL_NO_SYNC_NV;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglCreateFenceSyncNV) {
        result = cnx->egl.eglCreateFenceSyncNV(
                dp->disp.dpy, condition, attrib_list);
    }
    return result;
}

EGLBoolean eglDestroySyncNV(EGLSyncNV sync)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglDestroySyncNV) {
        result = cnx->egl.eglDestroySyncNV(sync);
    }
    return result;
}

EGLBoolean eglFenceNV(EGLSyncNV sync)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglFenceNV) {
        result = cnx->egl.eglFenceNV(sync);
    }
    return result;
}

EGLint eglClientWaitSyncNV(EGLSyncNV sync,
        EGLint flags, EGLTimeNV timeout)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLint result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglClientWaitSyncNV) {
        result = cnx->egl.eglClientWaitSyncNV(
                sync, flags, timeout);
    }
    return result;
}

EGLBoolean eglSignalSyncNV(EGLSyncNV sync,
        EGLenum mode)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglSignalSyncNV) {
        result = cnx->egl.eglSignalSyncNV(
                sync, mode);
    }
    return result;
}

EGLBoolean eglGetSyncAttribNV(EGLSyncNV sync,
        EGLint attribute, EGLint* value)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglGetSyncAttribNV) {
        result = cnx->egl.eglGetSyncAttribNV(
                sync, attribute, value);
    }
    return result;
}

EGLPerfMonitorNV eglCreatePerfMonitorNV(EGLDisplay dpy,
        EGLint mask)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_NO_PERFMONITOR_NV;

    EGLPerfMonitorNV result = EGL_NO_PERFMONITOR_NV;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglCreatePerfMonitorNV) {
        result = cnx->egl.eglCreatePerfMonitorNV(
                dp->disp.dpy, mask);
    }
    return result;
}

EGLBoolean eglDestroyPerfMonitorNV(EGLDisplay dpy,
        EGLPerfMonitorNV monitor)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_FALSE;

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglDestroyPerfMonitorNV) {
        result = cnx->egl.eglDestroyPerfMonitorNV(
                dp->disp.dpy, monitor);
    }
    return result;
}

EGLBoolean eglMakeCurrentPerfMonitorNV(EGLPerfMonitorNV monitor)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglMakeCurrentPerfMonitorNV) {
        result = cnx->egl.eglMakeCurrentPerfMonitorNV(
                monitor);
    }
    return result;
}

EGLPerfMonitorNV eglGetCurrentPerfMonitorNV(void)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_NO_PERFMONITOR_NV);
    }

    EGLPerfMonitorNV result = EGL_NO_PERFMONITOR_NV;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglGetCurrentPerfMonitorNV) {
        result = cnx->egl.eglGetCurrentPerfMonitorNV();
    }
    return result;
}

EGLBoolean eglGetPerfCountersNV(EGLPerfMonitorNV monitor,
        EGLPerfCounterNV* counters, EGLint counter_size, EGLint* num_counter)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglGetPerfCountersNV) {
        result = cnx->egl.eglGetPerfCountersNV(monitor,
                counters, counter_size, num_counter);
    }
    return result;
}

EGLBoolean eglGetPerfCounterAttribNV(EGLPerfMonitorNV monitor,
        EGLPerfCounterNV counter, EGLint pname, EGLint* value)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglGetPerfCounterAttribNV) {
        result = cnx->egl.eglGetPerfCounterAttribNV(monitor,
                counter, pname, value);
    }
    return result;
}

const char* eglQueryPerfCounterStringNV(EGLPerfMonitorNV monitor,
        EGLPerfCounterNV counter, EGLint pname)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, (const char*)NULL);
    }

    const char* result = NULL;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglQueryPerfCounterStringNV) {
        result = cnx->egl.eglQueryPerfCounterStringNV(monitor,
                counter, pname);
    }
    return result;
}

EGLBoolean eglPerfMonitorAddCountersNV(EGLint n,
        const EGLPerfCounterNV* counters)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglPerfMonitorAddCountersNV) {
        result = cnx->egl.eglPerfMonitorAddCountersNV(n,
                counters);
    }
    return result;
}

EGLBoolean eglPerfMonitorRemoveCountersNV(EGLint n,
        const EGLPerfCounterNV* counters)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglPerfMonitorRemoveCountersNV) {
        result = cnx->egl.eglPerfMonitorRemoveCountersNV(n,
                counters);
    }
    return result;
}

EGLBoolean eglPerfMonitorRemoveAllCountersNV(void)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglPerfMonitorRemoveAllCountersNV) {
        result = cnx->egl.eglPerfMonitorRemoveAllCountersNV();
    }
    return result;
}

EGLBoolean eglPerfMonitorBeginExperimentNV(void)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglPerfMonitorBeginExperimentNV) {
        result = cnx->egl.eglPerfMonitorBeginExperimentNV();
    }
    return result;
}

EGLBoolean eglPerfMonitorEndExperimentNV(void)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglPerfMonitorEndExperimentNV) {
        result = cnx->egl.eglPerfMonitorEndExperimentNV();
    }
    return result;
}

EGLBoolean eglPerfMonitorBeginPassNV(EGLint n)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglPerfMonitorBeginPassNV) {
        result = cnx->egl.eglPerfMonitorBeginPassNV(n);
    }
    return result;
}

EGLBoolean eglPerfMonitorEndPassNV(void)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglPerfMonitorEndPassNV) {
        result = cnx->egl.eglPerfMonitorEndPassNV();
    }
    return result;
}

EGLPerfMarkerNV eglCreatePerfMarkerNV(void)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_DEFAULT_PERFMARKER_NV);
    }

    EGLPerfMarkerNV result = EGL_DEFAULT_PERFMARKER_NV;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglCreatePerfMarkerNV) {
        result = cnx->egl.eglCreatePerfMarkerNV();
    }
    return result;
}

EGLBoolean eglDestroyPerfMarkerNV(EGLPerfMarkerNV marker)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglDestroyPerfMarkerNV) {
        result = cnx->egl.eglDestroyPerfMarkerNV(marker);
    }
    return result;
}

EGLBoolean eglMakeCurrentPerfMarkerNV(EGLPerfMarkerNV marker)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglMakeCurrentPerfMarkerNV) {
        result = cnx->egl.eglMakeCurrentPerfMarkerNV(marker);
    }
    return result;
}

EGLPerfMarkerNV eglGetCurrentPerfMarkerNV(void)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_DEFAULT_PERFMARKER_NV);
    }

    EGLPerfMarkerNV result = EGL_DEFAULT_PERFMARKER_NV;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglGetCurrentPerfMarkerNV) {
        result = cnx->egl.eglGetCurrentPerfMarkerNV();
    }
    return result;
}

EGLBoolean eglGetPerfMarkerCounterNV(EGLPerfMarkerNV marker,
        EGLPerfCounterNV counter, EGLuint64NV *value, EGLuint64NV *cycles)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglGetPerfMarkerCounterNV) {
        result = cnx->egl.eglGetPerfMarkerCounterNV(marker,
                counter, value, cycles);
    }
    return result;
}

EGLBoolean eglValidatePerfMonitorNV(EGLint* num_passes)
{
    clearError();

    if (egl_init_drivers() == EGL_FALSE) {
        return setError(EGL_BAD_PARAMETER, EGL_FALSE);
    }

    EGLBoolean result = EGL_FALSE;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglValidatePerfMonitorNV) {
        result = cnx->egl.eglValidatePerfMonitorNV(num_passes);
    }
    return result;
}

EGLSyncKHR eglCreateStreamSyncNV(EGLDisplay dpy, EGLStreamKHR stream,
                                 EGLenum type, const EGLint *attrib_list)
{
    clearError();

    const egl_display_ptr dp = validate_display(dpy);
    if (!dp) return EGL_NO_SYNC_KHR;

    EGLSyncKHR result = EGL_NO_SYNC_KHR;
    egl_connection_t* const cnx = &gEGLImpl;
    if (cnx->dso && cnx->egl.eglCreateStreamSyncNV) {
        result = cnx->egl.eglCreateStreamSyncNV(dp->disp.dpy, stream, type, attrib_list);
    }
    return result;
}