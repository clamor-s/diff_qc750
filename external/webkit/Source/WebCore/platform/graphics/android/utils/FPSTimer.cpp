/*
 * Copyright (C) 2011, NVIDIA CORPORATION
 * All rights reserved.
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

#include "config.h"
#include "FPSTimer.h"

#include "AndroidProperties.h"
#include <cutils/log.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wtf/CurrentTime.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

FPSTimer::FPSTimer(const String& name, double interval)
    : m_name(name)
    , m_interval(interval)
    , m_lastTime(0)
    , m_frames(0)
    , m_bestFps(0)
    , m_totalFrames(0)
    , m_totalTimeInSeconds(0)
    , m_resetFileMTime(0)
{
    m_resetFileName = String("/data/fps-reset-") + m_name;
}

FPSTimer* FPSTimer::createIfNeeded(const String& name)
{
    String intervalProperty = String("webkit.log.") + name + ".fps-interval";
    double interval = AndroidProperties::getDoubleProperty(intervalProperty.utf8().data(), 0);
    if (interval <= 0)
        return 0;

    return new FPSTimer(name, interval);
}

void FPSTimer::frameComplete()
{
    double now = currentTime();

    if (!m_lastTime || now - m_lastTime > 1.0) {
        struct stat resetFileStat;
        stat(m_resetFileName.utf8().data(), &resetFileStat);
        if (resetFileStat.st_mtime > m_resetFileMTime) {
          m_bestFps = m_totalFrames = m_totalTimeInSeconds = 0;
          m_resetFileMTime = resetFileStat.st_mtime;
        }
    }

    if (!m_lastTime)
        m_lastTime = now;

    m_frames++;
    double elapsed = now - m_lastTime;
    if (elapsed > m_interval) {
        double fps = m_frames / elapsed;

        m_totalFrames += m_frames;
        m_totalTimeInSeconds += elapsed;
        m_bestFps = std::max(fps, m_bestFps);

        android_printLog(ANDROID_LOG_INFO, m_name.utf8().data(),
                "%p  FPS: %0.2f [now]   %0.2f [mean]   %0.2f [best]\n",
                this, fps, m_totalFrames / m_totalTimeInSeconds, m_bestFps);

        m_frames = 0;
        m_lastTime = now;
    }
}

} // namespace WebCore
