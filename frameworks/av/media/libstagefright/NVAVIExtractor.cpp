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



//#define LOG_NDEBUG 0
#define LOG_TAG "NVAVIExtractor"
#include <utils/Log.h>
#include "include/NVAVIExtractor.h"
#include <stdlib.h>
#include <string.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>
//#include "include/avc_utils.h"
#include <dlfcn.h>

namespace android {

class NVAVISource : public MediaSource{
public:
    // Caller retains ownership of both "dataSource" and "Parser Handle".
    NVAVISource(const sp<MetaData> &format,
        const sp<DataSource> &dataSource,
        uint32_t mTrackCount, size_t &index,
        NVAVIExtractorData *extractor,
        NvParseHalImplementation nvAviParseHalImpl,
        void* NvAviParserHalHandle);

    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();
    virtual sp<MetaData> getFormat();
    virtual void InitSource(size_t& index, NVAVIExtractorData *Extractor);
    virtual status_t read(
        MediaBuffer **buffer, const ReadOptions *options = NULL);

protected:
    virtual ~NVAVISource();

private:
    sp<MetaData> mFormat;
    sp<DataSource> mDataSource;
    NVAVIExtractorData *m_hExtractor;
    bool mStarted;
    size_t mTrackIndex;
    MediaBufferGroup *mGroup;
    MediaBuffer *mBuffer;
    uint32_t mTrackCount;
    NvParseHalImplementation nvAviParseHalImpl;
    void* NvAviParserHalHandle;

    NVAVISource(const NVAVISource &);
    NVAVISource &operator=(const NVAVISource &);
};

/*****************************************************************************/
NVAVIExtractor :: NVAVIExtractor(const sp<DataSource> &source)
        : mDataSource(source),
        Extractor(NULL),
        mHaveMetadata(false),
        mDuration(0),
        mFlags(0),
        NvAviParserHalLibHandle(NULL),
        mFileMetaData(new MetaData) {

    const char* error;

    memset(&mTracks,0,sizeof(Track)*MAX_INPUT_STREAMS);

    LOGV("+++ %s +++",__FUNCTION__);
    if (NvAviParserHalLibHandle == NULL) {
        // Clear the old errors, no need to handle return for clearing
        dlerror();
        NvAviParserHalLibHandle = dlopen("libnvaviparserhal.so", RTLD_NOW);
        if (NvAviParserHalLibHandle == NULL) {
            LOGE("--- %s[%d],Failed to open libnvaviparserhal.so, error %s",
                 __FUNCTION__,__LINE__,dlerror());
            return;
        }
    }

    typedef void *(*GetInstanceFunc)(NvParseHalImplementation *);
    // Clear the old errors, no need to handle return for clearing
    dlerror();
    GetInstanceFunc getInstanceFunc =
        (GetInstanceFunc) dlsym(NvAviParserHalLibHandle,
                                "NvAviParserHal_GetInstanceFunc");

    error = dlerror();
    if (error != NULL) {
        LOGE("--- %s[%d],Failed to locate getInstanceFunc in libnvaviparserhal.so",
             __FUNCTION__,__LINE__);
        return;
    } else {
        getInstanceFunc(&nvAviParseHalImpl);
    }
    LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
}

/*****************************************************************************/
NVAVIExtractor::~NVAVIExtractor() {
    status_t err = OK;
    LOGV("+++ %s +++",__FUNCTION__);

    if ((mFlags & IS_INITIALIZED) && !(mFlags & HAS_SHARED_HAL)) {
        LOGV("%s[%d], destrying parser HAL",__FUNCTION__,__LINE__);
        err = nvAviParseHalImpl.nvParserHalDestroyFunc(NvAviParserHalHandle);
        NvAviParserHalHandle = NULL;
        CHECK_EQ(err,OK);
        delete Extractor;
        Extractor = NULL;
    }
    LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
}

/*****************************************************************************/
bool NVAVIExtractor::initParser() {
    status_t err = OK;
    char pFilename[128];

    LOGV("+++ %s +++",__FUNCTION__);

    if (mFlags & IS_INITIALIZED) {
        LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
        return true;
    }

    if (NvAviParserHalLibHandle == NULL) {
        LOGE("ERROR :: --- %s[%d] ---",__FUNCTION__,__LINE__);
        return false;
    }

    Extractor = new NVAVIExtractorData;
    memset(Extractor,0,sizeof(NVAVIExtractorData));

    sprintf((char *)pFilename,"stagefright://%x",(mDataSource.get()));
    err =
        nvAviParseHalImpl.nvParserHalCreateFunc(pFilename,
                                                &NvAviParserHalHandle);
    if (err != OK) {
        LOGE("ERROR :: --- %s[%d] ---",__FUNCTION__,__LINE__);
        return false;
    }
    // Get number track count
    mTrackCount =
        nvAviParseHalImpl.nvParserHalGetStreamCountFunc(NvAviParserHalHandle);

    mFlags |= HAS_TRACK_COUNT | IS_INITIALIZED;

    LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
    return true;
}

/*****************************************************************************/
sp<MetaData> NVAVIExtractor::getMetaData() {
    LOGV("+++ %s +++",__FUNCTION__);
    status_t err = OK;

    if (!(mFlags & IS_INITIALIZED)) {
        if (!initParser()) {
            err = UNKNOWN_ERROR;
            LOGE("ERROR :: --- %s[%d] ---",__FUNCTION__,__LINE__);
            return mFileMetaData;
        }
    }
    if (mFlags & HAS_FILE_METADATA) {
        LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
        return mFileMetaData;
    }

    err = nvAviParseHalImpl.nvParserHalGetMetaDataFunc(NvAviParserHalHandle,
                                                       mFileMetaData);
    if (err == OK) {
        mFlags |= HAS_FILE_METADATA;
    }
    LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
    return mFileMetaData;
}

/*****************************************************************************/
size_t NVAVIExtractor::countTracks() {
    status_t err = OK;
    LOGV("+++ %s +++",__FUNCTION__);

    if (!(mFlags & IS_INITIALIZED)) {
        if (!initParser()) {
            err = UNKNOWN_ERROR;
            LOGE("ERROR :: --- %s[%d] ---",__FUNCTION__,__LINE__);
            return 0;
        }
    }

    if (!(mFlags & HAS_TRACK_COUNT)) {
        mTrackCount = nvAviParseHalImpl.nvParserHalGetStreamCountFunc(NvAviParserHalHandle);
        mFlags |= HAS_TRACK_COUNT;
    }

    LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
    return mTrackCount;
}

/*****************************************************************************/
sp<MetaData> NVAVIExtractor::getTrackMetaData(
    size_t index, uint32_t flags) {
    status_t err = OK;
    LOGV("+++ %s +++",__FUNCTION__);

    if ((mTracks[index].has_meta_data == true) &&
        (!(flags & kIncludeExtensiveMetaData))) {
        LOGV("NVAVIExtractor::getTrackMetaData ---");
        return mTracks[index].meta;
    }

    if (!(flags & kIncludeExtensiveMetaData)) {
        mTracks[index].meta = new MetaData;
        mTracks[index].timescale = 0;
        mTracks[index].includes_expensive_metadata = false;
    } else {
        mTracks[index].includes_expensive_metadata = true;
    }

    err =
    nvAviParseHalImpl.nvParserHalGetTrackMetaDataFunc(NvAviParserHalHandle,
                                                      index,
                                                      (flags & kIncludeExtensiveMetaData),
                                                      mTracks[index].meta);
    if (err == OK) {
        mTracks[index].has_meta_data = true;
        LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
        return  mTracks[index].meta;
    } else {
        LOGE("ERROR :: --- %s[%d] ---",__FUNCTION__,__LINE__);
        return NULL;
    }
}

/*****************************************************************************/
sp<MediaSource> NVAVIExtractor::getTrack(size_t index) {
    LOGV("+++ %s +++",__FUNCTION__);

    Track  track = mTracks[index];

    mFlags |= HAS_SHARED_HAL;
    LOGV("--- %s[%d] ---, index = %d",__FUNCTION__,__LINE__,index);

    return new NVAVISource(
        track.meta, mDataSource, mTrackCount,index,
        Extractor,nvAviParseHalImpl,NvAviParserHalHandle);
}

/*****************************************************************************/
uint32_t NVAVIExtractor::flags() const {
    LOGV("+++ %s +++",__FUNCTION__);

    uint32_t flags = CAN_PAUSE;

    if (nvAviParseHalImpl.nvParserHalIsSeekableFunc(NvAviParserHalHandle)) {
        flags |= CAN_SEEK_FORWARD | CAN_SEEK_BACKWARD | CAN_SEEK;
    } else {
        LOGW("Non-Seekable Stream.");
    }
    LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
    return flags;
}

/******************************************************************************
******************************************************************************/
NVAVISource::NVAVISource(
        const sp<MetaData> &format,
        const sp<DataSource> &dataSource,
        uint32_t mTrackCount,size_t &index,
        NVAVIExtractorData *extractor,
        NvParseHalImplementation nvAviParseHalImpl,
        void* NvAviParserHalHandle)
        : mFormat(format),
        mDataSource(dataSource),
        mStarted(false),
        mTrackIndex(index),
        mBuffer(NULL),
        mTrackCount(mTrackCount),
        nvAviParseHalImpl(nvAviParseHalImpl),
        NvAviParserHalHandle(NvAviParserHalHandle) {

    LOGV("+++ %s +++",__FUNCTION__);
    InitSource(mTrackIndex,extractor);
    m_hExtractor->mSourceCreated++;
    LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
}

/*****************************************************************************/
void NVAVISource::InitSource(size_t &index, NVAVIExtractorData *Extractor) {
    status_t err = OK;
    uint32_t i;

    LOGV("+++ %s +++",__FUNCTION__);

    if (Extractor == NULL) {
        LOGV("NVAVISource::InitSource creating new NVAVIExtractorData");
        m_hExtractor = new NVAVIExtractorData;
        memset(m_hExtractor,0,sizeof(NVAVIExtractorData));
    } else {
        m_hExtractor = Extractor;
        LOGV("Reusing AVI source %p",Extractor);
    }

    sp<MetaData> meta = mFormat;
    err = nvAviParseHalImpl.nvParserHalSetTrackHeaderFunc(NvAviParserHalHandle,
                                                          index,meta);
    if (err != OK) {
        LOGE("ERROR :: --- %s[%d] ---",__FUNCTION__,__LINE__);
        return;
    }
    LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
}

/*****************************************************************************/
NVAVISource::~NVAVISource() {
    LOGV("+++ %s +++",__FUNCTION__);
    if (mStarted) {
        stop();
    }
    if (m_hExtractor) {
        m_hExtractor->mSourceCreated--;
        if (m_hExtractor->mSourceCreated == 0 && NvAviParserHalHandle) {
            LOGV("%s[%d], destrying parser HAL",__FUNCTION__,__LINE__);
            nvAviParseHalImpl.nvParserHalDestroyFunc(NvAviParserHalHandle);
            NvAviParserHalHandle = NULL;

            //Free extractor memory
            delete m_hExtractor;
            m_hExtractor = NULL;
        }
    }
    LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
}

/*****************************************************************************/
status_t NVAVISource::start(MetaData *params) {
    status_t err = OK;
    int32_t max_size;
    LOGV("+++ %s +++, mTrackIndex = %d",__FUNCTION__,mTrackIndex);

    Mutex::Autolock autoLock(m_hExtractor->mMutex);

    CHECK(!mStarted);

    if (m_hExtractor == NULL) {
        LOGV("NVAVISource Probably Restarted !!!!");
        InitSource(mTrackIndex, NULL);
    }
    // Check Once again
    if (m_hExtractor == NULL) {
        err = UNKNOWN_ERROR;
        LOGE("ERROR :: --- %s[%d] ---",__FUNCTION__,__LINE__);
        return UNKNOWN_ERROR;
    }

    mGroup = new MediaBufferGroup;
    CHECK(mFormat->findInt32(kKeyMaxInputSize, &max_size));
    mGroup->add_buffer(new MediaBuffer(max_size));

    mStarted = true;
    LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
    return OK;
}

/*****************************************************************************/
status_t NVAVISource::stop() {
    LOGV("+++ %s +++",__FUNCTION__);
    Mutex::Autolock autoLock(m_hExtractor->mMutex);
    status_t err = OK;
    mStarted = false;

    if (mBuffer != NULL) {
        mBuffer->release();
        mBuffer = NULL;
    }
    delete mGroup;
    mGroup = NULL;
    LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
    return err;
}

/*****************************************************************************/
sp<MetaData> NVAVISource::getFormat() {
    LOGV("+++ %s +++",__FUNCTION__);
    Mutex::Autolock autoLock(m_hExtractor->mMutex);
    LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
    return mFormat;
}

/*****************************************************************************/
status_t NVAVISource::read(
    MediaBuffer **out, const ReadOptions *options) {
    LOGV("+++ %s +++",__FUNCTION__);

    Mutex::Autolock autoLock(m_hExtractor->mMutex);

    status_t err = OK;
    int32_t decoderFlags = 0;
    CHECK(mStarted);

    *out = NULL;

    err = mGroup->acquire_buffer(&mBuffer);
    if (err != OK) {
        LOGE("ERROR :: --- %s[%d] ---",__FUNCTION__,__LINE__);
        CHECK_EQ(mBuffer, NULL);
        goto cleanup;
    }

    err = nvAviParseHalImpl.nvParserHalReadFunc(NvAviParserHalHandle,
                                                mTrackIndex, mBuffer, options);
    if (err != OK) {
        LOGV("--- %s[%d] --- EOS",__FUNCTION__,__LINE__);
        goto cleanup;
    }

    // Check if buffer contains codec config data
    mBuffer->meta_data()->findInt32(kKeyDecoderFlags, &decoderFlags);
    if (decoderFlags & OMX_BUFFERFLAG_CODECCONFIG) {
        sp<MetaData> meta = mFormat;
        uint8_t * temp = (uint8_t *)mBuffer->data();
        meta->setData(kKeyHeader, kTypeHeader, temp, mBuffer->range_length());
        err = nvAviParseHalImpl.nvParserHalReadFunc(NvAviParserHalHandle,
                                                    mTrackIndex,
                                                    mBuffer, NULL);
        if (err != OK) {
            LOGV("--- %s[%d] --- EOS",__FUNCTION__,__LINE__);
            goto cleanup;
        }
    } else if (mBuffer->range_length() == 0) {
            LOGV("--- %s[%d] ---, returning zero size buffer",
                 __FUNCTION__,__LINE__);
    }
cleanup:
    if(err == OK) {
        *out = mBuffer;
    } else {
        *out = NULL;
        if(mBuffer) {
            mBuffer->release();
        }
    }
    mBuffer = NULL;
    LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
    return err;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool SniffNVAVI(const sp<DataSource> &source, String8 *mimeType,
        float *confidence, sp<AMessage> *meta) {
    char tmp[12];
    LOGV("+++ %s +++",__FUNCTION__);
    if (source->readAt(0, tmp, 12) < 12) {
        return false;
    }

    if (!memcmp(tmp, "RIFF", 4) && !memcmp(&tmp[8], "AVI ", 4)) {
        mimeType->setTo(MEDIA_MIMETYPE_CONTAINER_AVI);
        *confidence = 1.1;
        LOGV("--- %s[%d] ---",__FUNCTION__,__LINE__);
        return true;
    }
    LOGV("ERROR :: --- %s[%d] ---",__FUNCTION__,__LINE__);
    return false;
}
}  // namespace android
