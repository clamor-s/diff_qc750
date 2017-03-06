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

#include "config.h"
#include "AndroidProperties.h"

#include <cutils/properties.h>

namespace WebCore {

bool AndroidProperties::getBoolProperty(const char* name, bool defaultValue)
{
    char stringValue[PROPERTY_VALUE_MAX];
    if (property_get(name, stringValue, "") == -1)
        return defaultValue;

    if (!strcmp(stringValue, "1"))
        return true;
    if (!strcmp(stringValue, "0"))
        return false;

    return defaultValue;
}

int AndroidProperties::getIntProperty(const char* name, int defaultValue)
{
    char stringValue[PROPERTY_VALUE_MAX];
    if (property_get(name, stringValue, "") == -1)
        return defaultValue;

    char* end;
    long value = strtol(stringValue, &end, 10);
    if (end != stringValue)
        return value;

    return defaultValue;
}

double AndroidProperties::getDoubleProperty(const char* name, double defaultValue)
{
    char stringValue[PROPERTY_VALUE_MAX];
    if (property_get(name, stringValue, "") == -1)
        return defaultValue;

    char* end;
    double value = strtod(stringValue, &end);
    if (end != stringValue)
        return value;

    return defaultValue;
}

String AndroidProperties::getStringProperty(const char* name, const char* defaultValue)
{
    char stringValue[PROPERTY_VALUE_MAX];
    if (property_get(name, stringValue, defaultValue) == -1)
        return String(defaultValue);

    return String(stringValue);
}

}
