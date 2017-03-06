/*
 * Copyright (C) 2003, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2012, NVIDIA Corporation, All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Logging.h"

#include <cutils/log.h>
#include <cutils/properties.h>

namespace WebCore {

static inline void initializeWithUserDefault(WTFLogChannel& channel)
{

    const char* channelPropertyName = "webkit.log.channelsEnabled";
    char value[PROPERTY_VALUE_MAX];
    char* endptr = value;
    property_get(channelPropertyName, value, "0");
    long logLevel = strtol(value, &endptr, 0);
    if (*endptr) {
        android_printLog(ANDROID_LOG_DEBUG, "WebKit", "Invalid value for %s", channelPropertyName);
        return;
    }

    if ((logLevel & channel.mask) == channel.mask)
        channel.state = WTFLogChannelOn;
    else
        channel.state = WTFLogChannelOff;
}

void InitializeLoggingChannelsIfNecessary()
{
    static bool haveInitializedLoggingChannels = false;
    if (haveInitializedLoggingChannels)
        return;
    haveInitializedLoggingChannels = true;

    initializeWithUserDefault(LogNotYetImplemented);
    initializeWithUserDefault(LogFrames);
    initializeWithUserDefault(LogLoading);
    initializeWithUserDefault(LogPopupBlocking);
    initializeWithUserDefault(LogEvents);
    initializeWithUserDefault(LogEditing);
    initializeWithUserDefault(LogLiveConnect);
    initializeWithUserDefault(LogIconDatabase);
    initializeWithUserDefault(LogSQLDatabase);
    initializeWithUserDefault(LogSpellingAndGrammar);
    initializeWithUserDefault(LogBackForward);
    initializeWithUserDefault(LogHistory);
    initializeWithUserDefault(LogPageCache);
    initializeWithUserDefault(LogPlatformLeaks);
    initializeWithUserDefault(LogResourceLoading);
    initializeWithUserDefault(LogNetwork);
    initializeWithUserDefault(LogFTP);
    initializeWithUserDefault(LogThreading);
    initializeWithUserDefault(LogStorageAPI);
    initializeWithUserDefault(LogMedia);
    initializeWithUserDefault(LogPlugins);
    initializeWithUserDefault(LogArchives);
}

}
