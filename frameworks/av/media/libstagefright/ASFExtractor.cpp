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


#define LOG_NDEBUG 0
#define LOG_TAG "ASFExtractor"
#include <utils/Log.h>
#include "include/ASFExtractor.h"
#include <stdlib.h>
#include <string.h>
#include <media/stagefright/DataSource.h>
#include <OMX_Types.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>
#include "include/avc_utils.h"
#include <dlfcn.h>

namespace android {

class ASFSource : public MediaSource{
public:
    // Caller retains ownership of both "dataSource" and "Parser Handle".
    ASFSource(const sp<MetaData> &format,
        const sp<DataSource> &dataSource,
        uint32_t mTrackCount, size_t &index,
        ASFExtractorData *extractor, NvParseHalImplementation nvAsfParseHalImpl,
        void* NvAsfParserHalHandle);

    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();
    virtual sp<MetaData> getFormat();
    virtual void InitSource(size_t& index, ASFExtractorData *Extractor);
    virtual status_t read(
        MediaBuffer **buffer, const ReadOptions *options = NULL);

protected:
    virtual ~ASFSource();

private:
    sp<MetaData> mFormat;
    sp<DataSource> mDataSource;
    ASFExtractorData *m_hExtractor;
    bool mStarted;
    size_t mTrackIndex;
    MediaBufferGroup *mGroup;
    MediaBuffer *mBuffer;
    uint32_t mTrackCount;
    NvParseHalImplementation nvAsfParseHalImpl;
    void* NvAsfParserHalHandle;

    ASFSource(const ASFSource &);
    ASFSource &operator=(const ASFSource &);
};

/**********************************************************************************************************************/
ASFExtractor :: ASFExtractor(const sp<DataSource> &source)
        : mDataSource(source),
        Extractor(NULL),
        mHaveMetadata(false),
        mDuration(0),
        mFlags(0),
        NvAsfParserHalLibHandle(NULL),
        mFileMetaData(new MetaData) {

    const char* error;

    memset(&mTracks,0,sizeof(Track)*MAX_INPUT_STREAMS);

    LOGV("ASFExtractor::ASFExtractor");
    if(NvAsfParserHalLibHandle == NULL) {
        // Clear the old errors, no need to handle return for clearing
        dlerror();
        NvAsfParserHalLibHandle = dlopen("libnvasfparserhal.so", RTLD_NOW);
        if(NvAsfParserHalLibHandle == NULL) {
            LOGE("Failed to libnvasfarserhal.so with error %s\n",dlerror());
            return;
        }
    }

    typedef void *(*GetInstanceFunc)(NvParseHalImplementation *);
    // Clear the old errors, no need to handle return for clearing
    dlerror();
    GetInstanceFunc getInstanceFunc =
        (GetInstanceFunc) dlsym(NvAsfParserHalLibHandle, "NvAsfParserHal_GetInstanceFunc");

    error = dlerror();
    if(error != NULL) {
        LOGE("Failed to locate NvAsfParserHal_GetInstanceFunc in libnvasfarserhal.so ");
        return;
    } else {
        getInstanceFunc(&nvAsfParseHalImpl);
    }
}

/**********************************************************************************************************************/
ASFExtractor::~ASFExtractor() {
    status_t err = OK;
    LOGV("ASFExtractor::~ASFExtractor");

    if((mFlags & IS_INITIALIZED) && !(mFlags & HAS_SHARED_HAL)) {
        err = nvAsfParseHalImpl.nvParserHalDestroyFunc(NvAsfParserHalHandle);
        NvAsfParserHalHandle = NULL;
        CHECK_EQ(err,OK);
        delete Extractor;
        Extractor = NULL;
    }
}

/**********************************************************************************************************************/
bool ASFExtractor::initParser() {
    status_t err = OK;
    char pFilename[128];

    if(mFlags & IS_INITIALIZED)
        return true;

    if(NvAsfParserHalLibHandle == NULL) {
        LOGE("Error in ASFExtractor::initParser");
        return false;
    }

    LOGV("In ASFExtractor::initParser");
    Extractor = new ASFExtractorData;
    memset(Extractor,0,sizeof(ASFExtractorData));

    sprintf((char *)pFilename,"stagefright://%x",(mDataSource.get()));
    err = nvAsfParseHalImpl.nvParserHalCreateFunc(pFilename,&NvAsfParserHalHandle);
    if(err != OK) {
        LOGE("Error in ASFExtractor::initParser");
        return false;
    }

    // Get number track count
    mTrackCount = nvAsfParseHalImpl.nvParserHalGetStreamCountFunc(NvAsfParserHalHandle);
    mFlags |= HAS_TRACK_COUNT | IS_INITIALIZED;

    return true;
}

/**********************************************************************************************************************/
sp<MetaData> ASFExtractor::getMetaData() {
    status_t err = OK;

    LOGV("ASFExtractor::getMetaData ");

    if(!(mFlags & IS_INITIALIZED)) {
        if(!initParser()) {
            return mFileMetaData;
        }
    }
    if(mFlags & HAS_FILE_METADATA) {
        return mFileMetaData;
    }

    err = nvAsfParseHalImpl.nvParserHalGetMetaDataFunc(NvAsfParserHalHandle,mFileMetaData);
    if(err == OK) {
        mFlags |= HAS_FILE_METADATA;
    }

    return mFileMetaData;
}

/**********************************************************************************************************************/
size_t ASFExtractor::countTracks() {
    status_t err = OK;
    LOGV("ASFExtractor::countTracks ");

    if(!(mFlags & IS_INITIALIZED)) {
        if(!initParser()) {
            err = UNKNOWN_ERROR;
            return 0;
        }
    }
    if(!(mFlags & HAS_TRACK_COUNT)) {
        mTrackCount = nvAsfParseHalImpl.nvParserHalGetStreamCountFunc(NvAsfParserHalHandle);
        mFlags |= HAS_TRACK_COUNT;
    }

    return mTrackCount;
}

/**********************************************************************************************************************/
sp<MetaData> ASFExtractor::getTrackMetaData(
    size_t index, uint32_t flags) {
    status_t err = OK;

    if((mTracks[index].has_meta_data == true) && (!(flags & kIncludeExtensiveMetaData))) {
        return mTracks[index].meta;
    }

    LOGV("ASFExtractor::getTrackMetaData ");

    if(!(flags & kIncludeExtensiveMetaData)) {
        mTracks[index].meta = new MetaData;
        mTracks[index].timescale = 0;
        mTracks[index].includes_expensive_metadata = false;
    } else {
        mTracks[index].includes_expensive_metadata = true;
    }

    err = nvAsfParseHalImpl.nvParserHalGetTrackMetaDataFunc(NvAsfParserHalHandle, index, (flags & kIncludeExtensiveMetaData), mTracks[index].meta);
    if(err == OK) {
        err = nvAsfParseHalImpl.nvParserHalSetTrackHeaderFunc(NvAsfParserHalHandle,index,mTracks[index].meta);
        mTracks[index].has_meta_data = true;
        return  mTracks[index].meta;
    } else {
        return NULL;
    }
}

/**********************************************************************************************************************/
sp<MediaSource> ASFExtractor::getTrack(size_t index) {

    Track  track = mTracks[index];
    mFlags |= HAS_SHARED_HAL;
    LOGV("Returning Track index %d",index);

    return new ASFSource(
        track.meta, mDataSource, mTrackCount,index,Extractor,nvAsfParseHalImpl,NvAsfParserHalHandle);
}

/**********************************************************************************************************************
**********************************************************************************************************************/
ASFSource::ASFSource(
        const sp<MetaData> &format,
        const sp<DataSource> &dataSource,
        uint32_t mTrackCount,size_t &index,
        ASFExtractorData *extractor,
        NvParseHalImplementation nvAsfParseHalImpl,
        void* NvAsfParserHalHandle)
        : mFormat(format),
        mDataSource(dataSource),
        mStarted(false),
        mTrackIndex(index),
        mGroup(NULL),
        mBuffer(NULL),
        mTrackCount(mTrackCount),
        nvAsfParseHalImpl(nvAsfParseHalImpl),
        NvAsfParserHalHandle(NvAsfParserHalHandle) {

    InitSource(mTrackIndex,extractor);
    m_hExtractor->mSourceCreated++;
}

/**********************************************************************************************************************/
void ASFSource::InitSource(size_t &index, ASFExtractorData *Extractor) {
    status_t err = OK;
    char pFilename[128];

    LOGV("In ASFSource::InitSource");
    if(Extractor == NULL) {
        m_hExtractor = new ASFExtractorData;
        memset(m_hExtractor,0,sizeof(ASFExtractorData));
        sprintf((char *)pFilename,"stagefright://%x",(mDataSource.get()));
        err = nvAsfParseHalImpl.nvParserHalCreateFunc(pFilename,&NvAsfParserHalHandle);
        if(err != OK) {
            return;
        }
    } else {
        m_hExtractor = Extractor;
        LOGV("Reusing ASF source %p",Extractor);
    }

    sp<MetaData> meta = mFormat;
    err = nvAsfParseHalImpl.nvParserHalSetTrackHeaderFunc(NvAsfParserHalHandle,index,meta);
    if(err != OK) {
        return;
    }
}

/**********************************************************************************************************************/
ASFSource::~ASFSource() {
    LOGV("ASFSource::~ASFSource() ");
    if(mStarted) {
        stop();
    }
    if (m_hExtractor) {
        m_hExtractor->mSourceCreated--;
        if (m_hExtractor->mSourceCreated == 0 && NvAsfParserHalHandle) {
            LOGV("%s[%d], destrying parser HAL",__FUNCTION__,__LINE__);
            nvAsfParseHalImpl.nvParserHalDestroyFunc(NvAsfParserHalHandle);
            NvAsfParserHalHandle = NULL;

            //Free extractor memory
            delete m_hExtractor;
            m_hExtractor = NULL;
        }
    }
}

/**********************************************************************************************************************/
status_t ASFSource::start(MetaData *params) {
    Mutex::Autolock autoLock(m_hExtractor->mMutex);
    status_t err = OK;
    int32_t max_size;
    LOGV("enterd ASFSource start mTrackIndex %d ", mTrackIndex);
    CHECK(!mStarted);

    if(m_hExtractor == NULL) {
        LOGV("ASFSource Probably Restarted !!!!");
        InitSource(mTrackIndex, NULL);
    }
    // Check Once again
    if(m_hExtractor == NULL) {
        LOGE("Serious Allocation Error");
        return UNKNOWN_ERROR;
    }

    mGroup = new MediaBufferGroup;
    CHECK(mFormat->findInt32(kKeyMaxInputSize, &max_size));
    mGroup->add_buffer(new MediaBuffer(max_size));

    mStarted = true;
    return OK;
}

/**********************************************************************************************************************/
status_t ASFSource::stop() {
    Mutex::Autolock autoLock(m_hExtractor->mMutex);

    LOGV("ASFSource Stop--------");
    status_t err = OK;
    mStarted = false;

   if(mBuffer != NULL) {
        mBuffer->release();
        mBuffer = NULL;
    }

    delete mGroup;
    mGroup = NULL;
    return err;
}

/**********************************************************************************************************************/
sp<MetaData> ASFSource::getFormat() {
    return mFormat;
}

/**********************************************************************************************************************/
status_t ASFSource::read(
    MediaBuffer **out, const ReadOptions *options) {

    Mutex::Autolock autoLock(m_hExtractor->mMutex);
    status_t err = OK;
    int32_t decoderFlags = 0;
    CHECK(mStarted);

    *out = NULL;

    err = mGroup->acquire_buffer(&mBuffer);
    if (err != OK) {
        CHECK_EQ(mBuffer, NULL);
        return err;
    }

    err = nvAsfParseHalImpl.nvParserHalReadFunc(NvAsfParserHalHandle,mTrackIndex, mBuffer, options);
    if(err != OK) {
        goto cleanup;
    }

    // Check if buffer contains codec config data
    mBuffer->meta_data()->findInt32(kKeyDecoderFlags, &decoderFlags);
    if(decoderFlags & OMX_BUFFERFLAG_CODECCONFIG) {
        sp<MetaData> meta = mFormat;
        uint8_t * temp = (uint8_t *)mBuffer->data();
        meta->setData(kKeyHeader, kTypeHeader, temp, mBuffer->range_length());
        /*Do not pass options this time as seek is already done in previous function call */
        err = nvAsfParseHalImpl.nvParserHalReadFunc(NvAsfParserHalHandle,mTrackIndex, mBuffer, NULL);
        if(err != OK) {
            goto cleanup;
        }
    }
    if(mBuffer->range_length() == 0) {
        LOGV("Read is returing zero sized buffer ");
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
    return err;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

bool SniffASF(const sp<DataSource> &source, String8 *mimeType,
        float *confidence, sp<AMessage> *meta) {

    uint8_t header[16];

    ssize_t n = source->readAt(0, header, sizeof(header));
    if(n < (ssize_t)sizeof(header)) {
        return false;
    }

    if(!memcmp(header, ASF_HEADER_GUID, 16)) {
        *mimeType = MEDIA_MIMETYPE_CONTAINER_ASF;
        *confidence = 1.0;
        LOGV ("asf is identified ####");
        return true;
    }
    return false;
}

}  // namespace android

