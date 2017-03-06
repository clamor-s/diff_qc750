/*
 * Copyright (C) 2009 The Android Open Source Project
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
#define LOG_TAG "AudioPlayer"
#include <utils/Log.h>

#include <binder/IPCThreadState.h>
#include <media/AudioTrack.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/AudioPlayer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>

#include "include/AwesomePlayer.h"

#include <cutils/properties.h>

namespace android {

struct AudioPlayerEvent : public TimedEventQueue::Event {
    AudioPlayerEvent(
            AudioPlayer *player,
            void (AudioPlayer::*method)())
        : mAudioPlayer(player),
          mMethod(method) {
    }

protected:
    virtual ~AudioPlayerEvent() {}

    virtual void fire(TimedEventQueue *queue, int64_t /* now_us */) {
       (mAudioPlayer->*mMethod)();
    }

private:
    AudioPlayer *mAudioPlayer;
    void (AudioPlayer::*mMethod)();

    AudioPlayerEvent(const AudioPlayerEvent &);
    AudioPlayerEvent &operator=(const AudioPlayerEvent &);
};


AudioPlayer::AudioPlayer(
        const sp<MediaPlayerBase::AudioSink> &audioSink,
        bool allowDeepBuffering,
        AwesomePlayer *observer)
    : mAudioTrack(NULL),
      mInputBuffer(NULL),
      mSampleRate(0),
      mLatencyUs(0),
      mFrameSize(0),
      mAudioFormat(AUDIO_FORMAT_PCM_16_BIT),
      mNumFramesPlayed(0),
      mNumFramesPlayedSysTimeUs(ALooper::GetNowUs()),
      mPositionTimeMediaUs(-1),
      mPositionTimeRealUs(-1),
      mSeeking(false),
      mReachedEOS(false),
      mFinalStatus(OK),
      mStarted(false),
      mEnabledAudioULP(false),
      mIsFirstBuffer(false),
      mFirstBufferResult(OK),
      mFirstBuffer(NULL),
      mAudioSink(audioSink),
      mAllowDeepBuffering(allowDeepBuffering),
      mObserver(observer),
      mPinnedTimeUs(-1ll),
      mULP_ResumeOnEOS(false),
      mFramecount(0) {

    mPortSettingsChangedEvent = new AudioPlayerEvent(this, &AudioPlayer::onPortSettingsChangedEvent);
    mPortSettingsChangedEventPending = false;
    mQueueStarted = false;

}

AudioPlayer::~AudioPlayer() {
    if (mStarted) {
        reset();
    }
    if (mQueueStarted) {
        mQueue.stop();
    }
}

void AudioPlayer::setSource(const sp<MediaSource> &source) {
    CHECK(mSource == NULL);
    mSource = source;
}

status_t AudioPlayer::start(bool sourceAlreadyStarted) {
    CHECK(!mStarted);
    CHECK(mSource != NULL);

    status_t err;
    if (!sourceAlreadyStarted) {
        err = mSource->start();

        if (err != OK) {
            return err;
        }
    }

    if (!mQueueStarted) {
        mQueue.start();
        mQueueStarted = true;
    }

    // We allow an optional INFO_FORMAT_CHANGED at the very beginning
    // of playback, if there is one, getFormat below will retrieve the
    // updated format, if there isn't, we'll stash away the valid buffer
    // of data to be used on the first audio callback.

    CHECK(mFirstBuffer == NULL);

    MediaSource::ReadOptions options;
    if (mSeeking) {
        options.setSeekTo(mSeekTimeUs);
        mSeeking = false;
    }

    mFirstBufferResult = mSource->read(&mFirstBuffer, &options);
    if (mFirstBufferResult == INFO_FORMAT_CHANGED) {
        ALOGV("INFO_FORMAT_CHANGED!!!");

        CHECK(mFirstBuffer == NULL);
        mFirstBufferResult = OK;
        mIsFirstBuffer = false;
    } else {
        mIsFirstBuffer = true;
    }

    sp<MetaData> format = mSource->getFormat();
    const char *mime;
    bool success = format->findCString(kKeyMIMEType, &mime);
    CHECK(success);

    success = format->findInt32(kKeySampleRate, &mSampleRate);
    CHECK(success);

    int32_t numChannels, channelMask;
    success = format->findInt32(kKeyChannelCount, &numChannels);
    CHECK(success);

    uint32_t type;
    const void *value = NULL;
    size_t size;
    int32_t status = 0;

    success = format->findData(kKeyChannelType, &type, &value, &size);
    if (success) {
        status = property_set(kTegraAudioChannelMap, (char *)value);
    } else {
        status = property_set(kTegraAudioChannelMap, NULL);
    }

    if (status)
        ALOGE("set property to media.tegra.out.channel.map fails with error - %d", status);

    if(!format->findInt32(kKeyChannelMask, &channelMask)) {
        // log only when there's a risk of ambiguity of channel mask selection
        ALOGI_IF(numChannels > 2,
                "source format didn't specify channel mask, using (%d) channel order", numChannels);
        channelMask = CHANNEL_MASK_USE_CHANNEL_ORDER;
    }

    if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MPEG) ||
        !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MPEG_LAYER_II)) {
        // sending compressed audio as it is
        CHECK(numChannels == 2);
        mAudioFormat = AUDIO_FORMAT_MP3;
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC)) {
        // sending compressed audio as it is
        CHECK(numChannels == 2);
        mAudioFormat = AUDIO_FORMAT_AAC;
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AC3) ||
        !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_DTS)) {
         // sending compressed audio in IEC61937 format
         CHECK(numChannels == 2);
         mAudioFormat = AUDIO_FORMAT_IEC61937;
    } else {
        int32_t bitsPerSample;
        if (format->findInt32(kKeyBitsPerSample, &bitsPerSample)) {
            switch (bitsPerSample) {
                case 8:
                    mAudioFormat = AUDIO_FORMAT_PCM_8_BIT;
                    break;
                case 24:
                    mAudioFormat = AUDIO_FORMAT_PCM_8_24_BIT;
                    break;
                case 16:
                default:
                    mAudioFormat = AUDIO_FORMAT_PCM_16_BIT;
            }
        }
    }

    if (mAudioSink.get() != NULL) {

        status_t err = mAudioSink->open(
                mSampleRate, numChannels, channelMask, mAudioFormat,
                DEFAULT_AUDIOSINK_BUFFERCOUNT,
                &AudioPlayer::AudioSinkCallback,
                this,
                (mAllowDeepBuffering ?
                            AUDIO_OUTPUT_FLAG_DEEP_BUFFER :
                            AUDIO_OUTPUT_FLAG_NONE));
        if (err != OK) {
            if (mFirstBuffer != NULL) {
                mFirstBuffer->release();
                mFirstBuffer = NULL;
            }

            if (!sourceAlreadyStarted) {
                mSource->stop();
            }

            return err;
        }

        mLatencyUs = (int64_t)mAudioSink->latency() * 1000;
        mFrameSize = mAudioSink->frameSize();

        mAudioSink->start();
    } else {
        // playing to an AudioTrack, set up mask if necessary
        audio_channel_mask_t audioMask = channelMask == CHANNEL_MASK_USE_CHANNEL_ORDER ?
                audio_channel_out_mask_from_count(numChannels) : channelMask;
        if (0 == audioMask) {
            return BAD_VALUE;
        }

        mAudioTrack = new AudioTrack(
                AUDIO_STREAM_MUSIC, mSampleRate, mAudioFormat, audioMask,
                0, AUDIO_OUTPUT_FLAG_NONE, &AudioCallback, this, 0);

        if ((err = mAudioTrack->initCheck()) != OK) {
            delete mAudioTrack;
            mAudioTrack = NULL;

            if (mFirstBuffer != NULL) {
                mFirstBuffer->release();
                mFirstBuffer = NULL;
            }

            if (!sourceAlreadyStarted) {
                mSource->stop();
            }

            return err;
        }

        mLatencyUs = (int64_t)mAudioTrack->latency() * 1000;
        mFrameSize = mAudioTrack->frameSize();

        mAudioTrack->start();
    }

    mStarted = true;
    mPinnedTimeUs = -1ll;

    return OK;
}

void AudioPlayer::pause(bool playPendingSamples) {
    CHECK(mStarted);

    // If HDMI receiver is removed in pause state while sending compressed
    // data, flush pending data to avoid playback of buffered compressed
    // frames as PCM
    if ((playPendingSamples || (mAudioFormat != AUDIO_FORMAT_PCM_16_BIT)) &&
        (mEnabledAudioULP == false)) {
        if (mAudioSink.get() != NULL) {
            mAudioSink->stop();
        } else {
            mAudioTrack->stop();
        }

        mNumFramesPlayed = 0;
        mNumFramesPlayedSysTimeUs = ALooper::GetNowUs();
    } else {
        if(mEnabledAudioULP) {
            String8 key = String8("nv_param_avp_decode_pause=1");
            AudioSystem::setParameters(0,key);
            mFramecount_ULP = 0;
            mAudioTrack->getMinFrameCount((int *)&mFramecount, AUDIO_STREAM_MUSIC, mSampleRate);
            mULP_ResumeOnEOS = false;
        }
        if (mAudioSink.get() != NULL) {
            mAudioSink->pause();
        } else {
            mAudioTrack->pause();
        }

        mPinnedTimeUs = ALooper::GetNowUs();
    }
}

void AudioPlayer::resume() {
    CHECK(mStarted);

    if ((audio_is_linear_pcm(mAudioFormat)) || mEnabledAudioULP) {
        if(mEnabledAudioULP) {
            String8 key = String8("nv_param_avp_decode_pause=0");
            AudioSystem::setParameters(0,key);
            if (mReachedEOS)
                mULP_ResumeOnEOS = true;
        }

        if (mAudioSink.get() != NULL) {
            mAudioSink->start();
        } else {
            mAudioTrack->start();
        }
    } else {
        // compressed audio case
        // HDMI receiver could have been removed (or plugged out and plugged in).
        // So the AUX stream might not be in proper state required for
        // compressed.audio pass-thru.
        // Reopen AudioTrack to handle this scenario
        mPortSettingsChangedEventPending = true;
        mQueue.postEvent(mPortSettingsChangedEvent);
    }
}

void AudioPlayer::reset() {
    CHECK(mStarted);

    if (mAudioSink.get() != NULL) {
        mAudioSink->stop();
        mAudioSink->close();
    } else {
        mAudioTrack->stop();

        delete mAudioTrack;
        mAudioTrack = NULL;
    }

    // Make sure to release any buffer we hold onto so that the
    // source is able to stop().

    if (mFirstBuffer != NULL) {
        mFirstBuffer->release();
        mFirstBuffer = NULL;
    }

    if (mInputBuffer != NULL) {
        ALOGV("AudioPlayer releasing input buffer.");

        mInputBuffer->release();
        mInputBuffer = NULL;
    }

    mSource->stop();

    // The following hack is necessary to ensure that the OMX
    // component is completely released by the time we may try
    // to instantiate it again.
    wp<MediaSource> tmp = mSource;
    mSource.clear();
    if(!mEnabledAudioULP)
    while (tmp.promote() != NULL) {
        usleep(1000);
    }
    IPCThreadState::self()->flushCommands();

    mNumFramesPlayed = 0;
    mNumFramesPlayedSysTimeUs = ALooper::GetNowUs();
    mPositionTimeMediaUs = -1;
    mPositionTimeRealUs = -1;
    mSeeking = false;
    mReachedEOS = false;
    mFinalStatus = OK;
    mStarted = false;
}

// static
void AudioPlayer::AudioCallback(int event, void *user, void *info) {
    static_cast<AudioPlayer *>(user)->AudioCallback(event, info);
}

bool AudioPlayer::isSeeking() {
    Mutex::Autolock autoLock(mLock);
    return mSeeking;
}

bool AudioPlayer::reachedEOS(status_t *finalStatus) {
    *finalStatus = OK;

    Mutex::Autolock autoLock(mLock);
    *finalStatus = mFinalStatus;
    return mReachedEOS;
}

status_t AudioPlayer::setPlaybackRatePermille(int32_t ratePermille) {
    if (mAudioSink.get() != NULL) {
        return mAudioSink->setPlaybackRatePermille(ratePermille);
    } else if (mAudioTrack != NULL){
        return mAudioTrack->setSampleRate(ratePermille * mSampleRate / 1000);
    } else {
        return NO_INIT;
    }
}

// static
size_t AudioPlayer::AudioSinkCallback(
        MediaPlayerBase::AudioSink *audioSink,
        void *buffer, size_t size, void *cookie) {
    AudioPlayer *me = (AudioPlayer *)cookie;

    return me->fillBuffer(buffer, size);
}

void AudioPlayer::AudioCallback(int event, void *info) {
    if (event != AudioTrack::EVENT_MORE_DATA) {
        return;
    }

    AudioTrack::Buffer *buffer = (AudioTrack::Buffer *)info;
    size_t numBytesWritten = fillBuffer(buffer->raw, buffer->size);

    buffer->size = numBytesWritten;
}

uint32_t AudioPlayer::getNumFramesPendingPlayout() const {
    uint32_t numFramesPlayedOut;
    status_t err;

    if (mAudioSink != NULL) {
        err = mAudioSink->getPosition(&numFramesPlayedOut);
    } else {
        err = mAudioTrack->getPosition(&numFramesPlayedOut);
    }

    if (err != OK || mNumFramesPlayed < numFramesPlayedOut) {
        return 0;
    }

    // mNumFramesPlayed is the number of frames submitted
    // to the audio sink for playback, but not all of them
    // may have played out by now.
    return mNumFramesPlayed - numFramesPlayedOut;
}

void AudioPlayer::onPortSettingsChangedEvent() {
    status_t err = OK;
    sp<MetaData> format;
    bool success;
    uint32_t type;
    const void *value = NULL;
    size_t size;
    int32_t status = 0;

   ALOGV("onPortSettingsChangedEvent");

    Mutex::Autolock autoLock(mLock);

    if (!mPortSettingsChangedEventPending) {
        goto onPortSettingsChangedEvent_exit;
    }

    // close exisiting playback
    if (mAudioSink.get() != NULL) {
        mAudioSink->stop();
        mAudioSink->close();
    } else {
        mAudioTrack->stop();
        delete mAudioTrack;
        mAudioTrack = NULL;
    }

    // open new
    format = mSource->getFormat();
    const char *mime;
    success = format->findCString(kKeyMIMEType, &mime);
    CHECK(success);

    success = format->findInt32(kKeySampleRate, &mSampleRate);
    CHECK(success);

    mNumFramesPlayed = 0;
    int32_t numChannels, channelMask;
    success = format->findInt32(kKeyChannelCount, &numChannels);
    CHECK(success);

    success = format->findData(kKeyChannelType, &type, &value, &size);
    if (success) {
        status = property_set(kTegraAudioChannelMap, (char *)value);
    } else {
        status = property_set(kTegraAudioChannelMap, NULL);
    }

    if (status)
        ALOGE("set property to media.tegra.out.channel.map fails with error - %d", status);

    if(!format->findInt32(kKeyChannelMask, &channelMask)) {
        // log only when there's a risk of ambiguity of channel mask selection
        ALOGI_IF(numChannels > 2,
             "source format didn't specify channel mask, using (%d) channel order", numChannels);
        channelMask = CHANNEL_MASK_USE_CHANNEL_ORDER;
    }

    if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MPEG) ||
        !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MPEG_LAYER_II)) {
        // sending compressed audio as it is
        CHECK(numChannels == 2);
        mAudioFormat = AUDIO_FORMAT_MP3;
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC)) {
        // sending compressed audio as it is
        CHECK(numChannels == 2);
        mAudioFormat = AUDIO_FORMAT_AAC;
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AC3) ||
               !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_DTS)) {
        // sending compressed audio in IEC61937 format
        CHECK(numChannels == 2);
        mAudioFormat = AUDIO_FORMAT_IEC61937;
    } else {
        int32_t bitsPerSample;
        if (format->findInt32(kKeyBitsPerSample, &bitsPerSample)) {
            switch (bitsPerSample) {
                case 8:
                    mAudioFormat = AUDIO_FORMAT_PCM_8_BIT;
                    break;
                case 24:
                    mAudioFormat = AUDIO_FORMAT_PCM_8_24_BIT;
                    break;
                case 16:
                default:
                    mAudioFormat = AUDIO_FORMAT_PCM_16_BIT;
            }
        }
    }

    ALOGV("New sample rate %d channels %d format: 0x%x", mSampleRate, numChannels, mAudioFormat);

    if (mAudioSink.get() != NULL) {
        err = mAudioSink->open(
                 mSampleRate, numChannels, channelMask, mAudioFormat,
                 DEFAULT_AUDIOSINK_BUFFERCOUNT,
                 &AudioPlayer::AudioSinkCallback,
                 this,
                 (mAllowDeepBuffering ?
                          AUDIO_OUTPUT_FLAG_DEEP_BUFFER :
                          AUDIO_OUTPUT_FLAG_NONE));

        if (err != OK) {
            ALOGE("mAudioSink->open error : %d", err);
            goto onPortSettingsChangedEvent_exit;
        }

        mLatencyUs = (int64_t)mAudioSink->latency() * 1000;
        mFrameSize = mAudioSink->frameSize();

        mAudioSink->start();
    } else {
        // playing to an AudioTrack, set up mask if necessary
        audio_channel_mask_t audioMask = channelMask == CHANNEL_MASK_USE_CHANNEL_ORDER ?
                          audio_channel_out_mask_from_count(numChannels) : channelMask;
        if (0 == audioMask) {
            goto onPortSettingsChangedEvent_exit;
        }

        mAudioTrack = new AudioTrack(
                   AUDIO_STREAM_MUSIC, mSampleRate, mAudioFormat, audioMask,
                   0, AUDIO_OUTPUT_FLAG_NONE, &AudioCallback, this, 0);

        if ((err = mAudioTrack->initCheck()) != OK) {
            ALOGE("AudioTrack error : %d", err);
            delete mAudioTrack;
            mAudioTrack = NULL;
            goto onPortSettingsChangedEvent_exit;
        }

        mLatencyUs = (int64_t)mAudioTrack->latency() * 1000;
        mFrameSize = mAudioTrack->frameSize();

        mAudioTrack->start();
    }

    mStarted = true;
    mPinnedTimeUs = -1ll;

    mPortSettingsChangedEventPending = false;

onPortSettingsChangedEvent_exit:

    if (err != OK) {
        if (mObserver && !mReachedEOS) {
            mObserver->postAudioEOS();
        }

        mReachedEOS = true;
        mFinalStatus = err;
    }

    mPortSettingsChangedEventPending = false;
    return;
}

size_t AudioPlayer::fillBuffer(void *data, size_t size) {
    if (mNumFramesPlayed == 0) {
        ALOGV("AudioCallback");
    }

    if (mReachedEOS && !mULP_ResumeOnEOS) {
        return 0;
    }

    bool postSeekComplete = false;
    bool postEOS = false;
    int64_t postEOSDelayUs = 0;
    bool postPortSettingsChanged = false;

    if (true == mPortSettingsChangedEventPending) {
        LOGV("Waiting for reconfig to finish... filling zeros");
        memset(data, 0, size);

        return size;
    }

    if (true == mULP_ResumeOnEOS) {
        LOGV("Trigger resume... filling zeros %d",size);
        memset(data, 0, size);
        mFramecount_ULP += size;
        if(mFramecount <= mFramecount_ULP)
            mULP_ResumeOnEOS = false;

        return size;
    }

    size_t size_done = 0;
    size_t size_remaining = size;
    while (size_remaining > 0) {
        MediaSource::ReadOptions options;

        {
            Mutex::Autolock autoLock(mLock);

            if (mSeeking) {
                if (mIsFirstBuffer) {
                    if (mFirstBuffer != NULL) {
                        mFirstBuffer->release();
                        mFirstBuffer = NULL;
                    }
                    mIsFirstBuffer = false;
                }

                options.setSeekTo(mSeekTimeUs);

                if (mInputBuffer != NULL) {
                    mInputBuffer->release();
                    mInputBuffer = NULL;
                }

                mSeeking = false;
                if (mObserver) {
                    postSeekComplete = true;
                }
            }
        }

        if (mInputBuffer == NULL) {
            status_t err;

            if (mIsFirstBuffer) {
                mInputBuffer = mFirstBuffer;
                mFirstBuffer = NULL;
                err = mFirstBufferResult;

                mIsFirstBuffer = false;
            } else {
                err = mSource->read(&mInputBuffer, &options);
            }

            if (err == INFO_FORMAT_CHANGED) {
                LOGV("INFO_FORMAT_CHANGED");
                postPortSettingsChanged = true;
                break;
            }

            // Technically INFO_DICONTINUITY is not an error
            if (err == INFO_DISCONTINUITY) {
               // Ignore this error
               continue;
            }

            CHECK((err == OK && mInputBuffer != NULL)
                   || (err != OK && mInputBuffer == NULL));

            Mutex::Autolock autoLock(mLock);

            if (err != OK) {
                if (mObserver && !mReachedEOS) {
                    // We don't want to post EOS right away but only
                    // after all frames have actually been played out.

                    // These are the number of frames submitted to the
                    // AudioTrack that you haven't heard yet.
                    uint32_t numFramesPendingPlayout =
                        getNumFramesPendingPlayout();

                    // These are the number of frames we're going to
                    // submit to the AudioTrack by returning from this
                    // callback.
                    uint32_t numAdditionalFrames = size_done / mFrameSize;

                    numFramesPendingPlayout += numAdditionalFrames;

                    int64_t timeToCompletionUs =
                        (1000000ll * numFramesPendingPlayout) / mSampleRate;

                    ALOGV("total number of frames played: %lld (%lld us)",
                            (mNumFramesPlayed + numAdditionalFrames),
                            1000000ll * (mNumFramesPlayed + numAdditionalFrames)
                                / mSampleRate);

                    ALOGV("%d frames left to play, %lld us (%.2f secs)",
                         numFramesPendingPlayout,
                         timeToCompletionUs, timeToCompletionUs / 1E6);

                    if (mEnabledAudioULP) {
                        String8 key = String8("nv_param_avp_decode_eos=1");
                        AudioSystem::setParameters(0,key);
                    } else {
                        postEOS = true;
                        if (mAudioSink->needsTrailingPadding()) {
                            postEOSDelayUs = timeToCompletionUs + mLatencyUs;
                        } else {
                            postEOSDelayUs = 0;
                        }
                    }
                }

                mReachedEOS = true;
                mFinalStatus = err;
                break;
            }

            if (mAudioSink != NULL) {
                mLatencyUs = (int64_t)mAudioSink->latency() * 1000;
            } else {
                mLatencyUs = (int64_t)mAudioTrack->latency() * 1000;
            }

            CHECK(mInputBuffer->meta_data()->findInt64(
                        kKeyTime, &mPositionTimeMediaUs));

            mPositionTimeRealUs =
                ((mNumFramesPlayed + size_done / mFrameSize) * 1000000)
                    / mSampleRate;

            ALOGV("buffer->size() = %d, "
                 "mPositionTimeMediaUs=%.2f mPositionTimeRealUs=%.2f",
                 mInputBuffer->range_length(),
                 mPositionTimeMediaUs / 1E6, mPositionTimeRealUs / 1E6);
        }

        if (mInputBuffer->range_length() == 0) {
            mInputBuffer->release();
            mInputBuffer = NULL;

            continue;
        }

        size_t copy = size_remaining;
        if (copy > mInputBuffer->range_length()) {
            copy = mInputBuffer->range_length();
        }

        memcpy((char *)data + size_done,
               (const char *)mInputBuffer->data() + mInputBuffer->range_offset(),
               copy);

        mInputBuffer->set_range(mInputBuffer->range_offset() + copy,
                                mInputBuffer->range_length() - copy);

        size_done += copy;
        size_remaining -= copy;
    }

    {
        Mutex::Autolock autoLock(mLock);
        mNumFramesPlayed += size_done / mFrameSize;
        mNumFramesPlayedSysTimeUs = ALooper::GetNowUs();

        if (mReachedEOS) {
            mPinnedTimeUs = mNumFramesPlayedSysTimeUs;
        } else {
            mPinnedTimeUs = -1ll;
        }
    }

   if (postPortSettingsChanged) {
        mPortSettingsChangedEventPending = true;
        // These are the number of frames submitted to the
        // AudioTrack that you haven't heard yet.
        uint32_t numFramesPendingPlayout =
                        getNumFramesPendingPlayout();
        int64_t timeToCompletionUs =
                        (1000000ll * numFramesPendingPlayout) / mSampleRate;

        //Delaying the event to complete the playback of the remaining frames by track
        mQueue.postEventWithDelay(mPortSettingsChangedEvent,timeToCompletionUs);
    }

    if (postEOS) {
        mObserver->postAudioEOS(postEOSDelayUs);
    }

    if (postSeekComplete) {
        mObserver->postAudioSeekComplete();
    }

    return size_done;
}

int64_t AudioPlayer::getRealTimeUs() {
    Mutex::Autolock autoLock(mLock);
    return getRealTimeUsLocked();
}

int64_t AudioPlayer::getRealTimeUsLocked() const {
    CHECK(mStarted);
    CHECK_NE(mSampleRate, 0);
    int64_t result = -mLatencyUs + (mNumFramesPlayed * 1000000) / mSampleRate;

    // Compensate for large audio buffers, updates of mNumFramesPlayed
    // are less frequent, therefore to get a "smoother" notion of time we
    // compensate using system time.
    int64_t diffUs;
    if (mPinnedTimeUs >= 0ll) {
        diffUs = mPinnedTimeUs;
    } else {
        diffUs = ALooper::GetNowUs();
    }

    diffUs -= mNumFramesPlayedSysTimeUs;

    return result + diffUs;
}

int64_t AudioPlayer::getMediaTimeUs() {
    Mutex::Autolock autoLock(mLock);

    if (mPositionTimeMediaUs < 0 || mPositionTimeRealUs < 0) {
        if (mSeeking) {
            return mSeekTimeUs;
        }

        return 0;
    }

    int64_t realTimeOffset = getRealTimeUsLocked() - mPositionTimeRealUs;
    if (realTimeOffset < 0) {
        realTimeOffset = 0;
    }

    return mPositionTimeMediaUs + realTimeOffset;
}

bool AudioPlayer::getMediaTimeMapping(
        int64_t *realtime_us, int64_t *mediatime_us) {
    Mutex::Autolock autoLock(mLock);

    *realtime_us = mPositionTimeRealUs;
    *mediatime_us = mPositionTimeMediaUs;

    return mPositionTimeRealUs != -1 && mPositionTimeMediaUs != -1;
}

status_t AudioPlayer::seekTo(int64_t time_us) {
    Mutex::Autolock autoLock(mLock);

    mSeeking = true;
    mPositionTimeRealUs = mPositionTimeMediaUs = -1;
    mReachedEOS = false;
    mSeekTimeUs = time_us;

    // Flush resets the number of played frames
    mNumFramesPlayed = 0;
    mNumFramesPlayedSysTimeUs = ALooper::GetNowUs();

    if (mAudioSink != NULL) {
        mAudioSink->flush();
    } else {
        mAudioTrack->flush();
    }

    return OK;
}

void AudioPlayer::enableAudioULP(bool enabledAudioULP) {
    mEnabledAudioULP = enabledAudioULP;
}

}
