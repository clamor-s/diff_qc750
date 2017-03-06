/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2012 NVIDIA. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "PluginTest.h"

#include "PluginObject.h"
#include "android_npapi.h"
#include "android/AndroidHelpers.h"
#include <android/native_window.h>
#include <gui/SurfaceTextureClient.h>
#include <utils/StrongPointer.h>

using namespace std;

// Call multiple times to draw(). The plugin should be able to draw to the window multiple times
// with GL while the browser GUI painting is disabled. Tests whether the SurfaceTexture that backs
// the ANativeWindow causes the eglSwapBuffers hang if the previous frames are not consumed.

class SurfaceTextureHang : public PluginTest {
public:
    SurfaceTextureHang(NPP npp, const string& identifier)
        : PluginTest(npp, identifier)
        , m_eglSurface(EGL_NO_SURFACE)
        , m_eglContext(EGL_NO_CONTEXT)
    {
    }

    ~SurfaceTextureHang();

    virtual NPError NPP_New(NPMIMEType, uint16_t, int16_t, char*[], char*[], NPSavedData*);
    virtual NPError NPP_GetValue(NPPVariable, void*);
    virtual NPError NPP_HandleEvent(void *event);

    void draw();
private:
    struct ScriptObject : Object<ScriptObject> {
        bool hasMethod(NPIdentifier);
        bool invoke(NPIdentifier, const NPVariant*, uint32_t, NPVariant*);
    };

    void initNativeWindowIfNeeded(int32_t width, int32_t height);

    android::sp<ANativeWindow> m_nativeWindow;
    EGLSurface  m_eglSurface;
    EGLContext m_eglContext;
};

static PluginTest::Register<SurfaceTextureHang> surfaceTextureHang("surface-texture-hang");

SurfaceTextureHang::~SurfaceTextureHang()
{
    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (m_eglContext != EGL_NO_CONTEXT)
        eglDestroyContext(eglDisplay, m_eglContext);

    if (m_eglSurface != EGL_NO_SURFACE)
        eglDestroySurface(eglDisplay, m_eglSurface);
}

NPError SurfaceTextureHang::NPP_New(NPMIMEType, uint16_t, int16_t, char*[], char*[], NPSavedData*)
{
    browser->setvalue(m_npp, kRequestDrawingModel_ANPSetValue, (void*)kOpenGL_ANPDrawingModel);
    return NPERR_NO_ERROR;
}

NPError SurfaceTextureHang::NPP_GetValue(NPPVariable variable, void* value)
{
    if (variable == NPPVpluginScriptableNPObject) {
        *static_cast<NPObject**>(value) = ScriptObject::create(this);
        return NPERR_NO_ERROR;
    }

    return NPERR_GENERIC_ERROR;
}

NPError SurfaceTextureHang::NPP_HandleEvent(void *event)
{
    ANPEvent* aevent = static_cast<ANPEvent*>(event);
    switch (aevent->eventType) {
        case kDraw_ANPEventType: {
            int32_t width = aevent->data.draw.data.surface.width;
            int32_t height = aevent->data.draw.data.surface.height;
            initNativeWindowIfNeeded(width, height);
            break;
        }
    }

    return 0;
}

void SurfaceTextureHang::draw()
{
    if (!m_nativeWindow.get()) {
        pluginLog(m_npp, "Can not draw, no native window.");
        return;
    }
    if (m_eglSurface == EGL_NO_SURFACE) {
        pluginLog(m_npp, "Can not draw, no surface.");
        return;
    }

    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglMakeCurrent(eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
    assert(EGL_SUCCESS == eglGetError());

    eglSwapBuffers(eglDisplay, m_eglSurface);
    assert(EGL_SUCCESS == eglGetError());

    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void SurfaceTextureHang::initNativeWindowIfNeeded(int32_t width, int32_t height)
{
    if (m_nativeWindow.get())
        return;

    m_nativeWindow = gNativeWindowInterface.acquireNativeWindow(m_npp);
    assert(m_nativeWindow.get());
    assert(width > 0);
    assert(height > 0);

    ANativeWindow_setBuffersGeometry(m_nativeWindow.get(), width, height, WINDOW_FORMAT_RGBA_8888);

    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(eglDisplay, NULL, NULL);
    assert(EGL_SUCCESS == eglGetError());

    static const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(eglDisplay, configAttribs, &config, 1, &numConfigs);
    assert(EGL_SUCCESS == eglGetError());

    m_eglSurface = eglCreateWindowSurface(eglDisplay, config, m_nativeWindow.get(), NULL);

    EGLint contextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    m_eglContext = eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT, contextAttributes);
    assert(EGL_SUCCESS == eglGetError());

    eglMakeCurrent(eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
    assert(EGL_SUCCESS == eglGetError());
}

bool SurfaceTextureHang::ScriptObject::hasMethod(NPIdentifier methodName)
{
    return methodName == pluginTest()->NPN_GetStringIdentifier("draw");
}

bool SurfaceTextureHang::ScriptObject::invoke(NPIdentifier identifier, const NPVariant*, uint32_t, NPVariant*)
{
    if (identifier == pluginTest()->NPN_GetStringIdentifier("draw"))
        static_cast<SurfaceTextureHang*>(pluginTest())->draw();
    else
        assert(0);

    return true;
}

