/*
 * Copyright (c) 2011-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AutoRestoreGLState_h
#define AutoRestoreGLState_h

#include <GLES2/gl2.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/OwnPtr.h>

namespace WebCore {

class AutoRestoreScissorTest {
public:
    AutoRestoreScissorTest() { glGetBooleanv(GL_SCISSOR_TEST, &m_scissorTest); }
    ~AutoRestoreScissorTest()
    {
        if (m_scissorTest)
            glEnable(GL_SCISSOR_TEST);
        else
            glDisable(GL_SCISSOR_TEST);
    }
    bool scissorTest() const { return m_scissorTest; }
private:
    GLboolean m_scissorTest;
};

class AutoRestoreBlend {
public:
    AutoRestoreBlend() { glGetBooleanv(GL_BLEND, &m_blend); }
    ~AutoRestoreBlend()
    {
        if (m_blend)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
    }
    bool blend() const { return m_blend; }
private:
    GLboolean m_blend;
};

class AutoRestoreBlendFunc {
public:
    AutoRestoreBlendFunc()
    {
        glGetIntegerv(GL_BLEND_SRC_RGB, &m_srcRGB);
        glGetIntegerv(GL_BLEND_DST_RGB, &m_dstRGB);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &m_srcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &m_dstAlpha);
    }
    ~AutoRestoreBlendFunc()
    {
        glBlendFuncSeparate(m_srcRGB, m_dstRGB, m_srcAlpha, m_dstAlpha);
    }
    int srcRGB() const { return m_srcRGB; }
    int dstRGB() const { return m_dstRGB; }
    int srcAlpha() const { return m_srcAlpha; }
    int dstAlpha() const { return m_dstAlpha; }
private:
    int m_srcRGB;
    int m_dstRGB;
    int m_srcAlpha;
    int m_dstAlpha;
};

class AutoRestoreBlendEquation {
public:
    AutoRestoreBlendEquation()
    {
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &m_modeRGB);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &m_modeAlpha);
    }
    ~AutoRestoreBlendEquation()
    {
        glBlendEquationSeparate(m_modeRGB, m_modeAlpha);
    }
    int modeRGB() const { return m_modeRGB; }
    int modeAlpha() const { return m_modeAlpha; }
private:
    int m_modeRGB;
    int m_modeAlpha;
};

class AutoRestoreArrayBufferBinding {
public:
    AutoRestoreArrayBufferBinding() { glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &m_arrayBufferBinding); }
    ~AutoRestoreArrayBufferBinding() { glBindBuffer(GL_ARRAY_BUFFER, m_arrayBufferBinding); }
    int arrayBufferBinding() const { return m_arrayBufferBinding; }
private:
    int m_arrayBufferBinding;
};

class AutoRestoreTextureBinding2D {
public:
    AutoRestoreTextureBinding2D() { glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_textureBinding); }
    ~AutoRestoreTextureBinding2D() { glBindTexture(GL_TEXTURE_2D, m_textureBinding); }
    int textureBinding() const { return m_textureBinding; }
private:
    int m_textureBinding;
};

class AutoRestoreActiveTexture {
public:
    AutoRestoreActiveTexture() { glGetIntegerv(GL_ACTIVE_TEXTURE, &m_activeTexture); }
    ~AutoRestoreActiveTexture() { glActiveTexture(m_activeTexture); }
    int activeTexture() const { return m_activeTexture; }
private:
    int m_activeTexture;
};

class AutoRestoreEnabledVertexArrays {
public:
    AutoRestoreEnabledVertexArrays()
    {
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &m_vertexArrayCount);
        m_arraysEnabled = adoptArrayPtr(new int[m_vertexArrayCount]);
        for (int i = 0; i < m_vertexArrayCount; i++)
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &m_arraysEnabled[i]);
    }
    ~AutoRestoreEnabledVertexArrays()
    {
        for (int i = 0; i < m_vertexArrayCount; i++) {
            if (m_arraysEnabled[i])
                glEnableVertexAttribArray(i);
            else
                glDisableVertexAttribArray(i);
        }
    }
    int vertexArrayCount() const { return m_vertexArrayCount; }
    bool isArrayEnabled(int i) const { return m_arraysEnabled[i]; }
private:
    int m_vertexArrayCount;
    OwnArrayPtr<int> m_arraysEnabled;
};

class AutoRestoreVertexAttribPointer {
public:
    AutoRestoreVertexAttribPointer(int index)
        : m_index(index)
    {
        glGetVertexAttribiv(m_index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &m_arrayBufferBinding);
        glGetVertexAttribiv(m_index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &m_size);
        glGetVertexAttribiv(m_index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &m_type);
        glGetVertexAttribiv(m_index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &m_stride);
        glGetVertexAttribiv(m_index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &m_normalized);
        glGetVertexAttribPointerv(m_index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &m_pointer);
    }
    ~AutoRestoreVertexAttribPointer()
    {
        AutoRestoreArrayBufferBinding restoreArrayBuffer;
        glBindBuffer(GL_ARRAY_BUFFER, m_arrayBufferBinding);
        glVertexAttribPointer(m_index, m_size, m_type, m_normalized, m_stride, m_pointer);
    }
    int index() const { return m_index; }
    int arrayBufferBinding() const { return m_arrayBufferBinding; }
    int size() const { return m_size; }
    int type() const { return m_type; }
    int normalized() const { return m_normalized; }
    int stride() const { return m_stride; }
    void* pointer() const { return m_pointer; }
private:
    int m_index;
    int m_arrayBufferBinding;
    int m_size;
    int m_type;
    int m_normalized;
    int m_stride;
    void* m_pointer;
};

class AutoRestoreCurrentProgram {
public:
    AutoRestoreCurrentProgram() { glGetIntegerv(GL_CURRENT_PROGRAM, &m_program); }
    ~AutoRestoreCurrentProgram() { glUseProgram(m_program); }
    int program() const { return m_program; }
private:
    int m_program;
};

class AutoRestoreMultiTextureBindings2D {
public:
    AutoRestoreMultiTextureBindings2D(int count = -1)
        : m_count(count)
    {
        if (m_count == -1)
            glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &m_count);
        m_textureBindings = adoptArrayPtr(new int[m_count]);
        AutoRestoreActiveTexture restoreActiveTexture;
        for (int i = 0; i < m_count; i++) {
            glActiveTexture(i + GL_TEXTURE0);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_textureBindings[i]);
        }
    }
    ~AutoRestoreMultiTextureBindings2D()
    {
        AutoRestoreActiveTexture restoreActiveTexture;
        for (int i = 0; i < m_count; i++) {
            glActiveTexture(i + GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_textureBindings[i]);
        }
    }
    int count() const { return m_count; }
    int textureBinding(int index) { return m_textureBindings[index]; }
private:
    int m_count;
    OwnArrayPtr<int> m_textureBindings;
};

class AutoRestoreFramebufferBinding {
public:
    AutoRestoreFramebufferBinding() { glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_fbo); }
    ~AutoRestoreFramebufferBinding() { glBindFramebuffer(GL_FRAMEBUFFER, m_fbo); }

private:
    GLint m_fbo;
};

class AutoRestoreClearColor {
public:
    AutoRestoreClearColor() { glGetFloatv(GL_COLOR_CLEAR_VALUE, m_color); }
    ~AutoRestoreClearColor() { glClearColor(m_color[0], m_color[1], m_color[2], m_color[3]); }

private:
    GLfloat m_color[4];
};

class AutoRestoreClearStencil {
public:
    AutoRestoreClearStencil() { glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &m_stencil); }
    ~AutoRestoreClearStencil() { glClearStencil(m_stencil); }

private:
    GLint m_stencil;
};

} // namespace WebCore

#endif // AutoRestoreGLState_h
