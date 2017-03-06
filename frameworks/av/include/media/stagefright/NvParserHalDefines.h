/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NVPARSER_HAL_DEFINES_H_

#define NVPARSER_HAL_DEFINES_H_

#include <utils/Errors.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaSource.h>

#if defined(__cplusplus)
extern "C"
{
#endif

namespace android {

typedef status_t (*NvParserHalCreateFunc)(char*,void** pNvParserHalHandle);
typedef status_t (*NvParserHalDestroyFunc)(void* NvParserHalHandle);
typedef size_t (*NvParserHalGetStreamCountFunc)(void* NvParserHalHandle);
typedef status_t (*NvParserHalGetMetaDataFunc) (void* asfParserHalHandle, sp<MetaData> mFileMetaData);
typedef status_t (*NvParserHalGetTrackMetaDataFunc) (void* asfParserHalHandle, int32_t streamIndex, bool setThumbnailTime, sp<MetaData> mTrackMetaData);
typedef status_t (*NvParserHalSetTrackHeaderFunc) (void* asfParserHalHandle, int32_t streamIndex, sp<MetaData> meta);
typedef status_t (*NvParserHalReadFunc) (void* asfParserHalHandle, int32_t streamIndex, MediaBuffer* mBuffer, const MediaSource::ReadOptions* options);
typedef bool (*NvParserHalIsSeekableFunc) (void* NvParserHalHandle);


typedef struct {
    NvParserHalCreateFunc nvParserHalCreateFunc;
    NvParserHalDestroyFunc nvParserHalDestroyFunc;
    NvParserHalGetStreamCountFunc nvParserHalGetStreamCountFunc;
    NvParserHalGetMetaDataFunc nvParserHalGetMetaDataFunc;
    NvParserHalGetTrackMetaDataFunc nvParserHalGetTrackMetaDataFunc;
    NvParserHalSetTrackHeaderFunc nvParserHalSetTrackHeaderFunc;
    NvParserHalReadFunc nvParserHalReadFunc;
    NvParserHalIsSeekableFunc nvParserHalIsSeekableFunc;
}NvParseHalImplementation;

} // namespace android

#if defined(__cplusplus)
}
#endif

#endif //#ifndef  NVPARSER_HAL_DEFINES_H_
