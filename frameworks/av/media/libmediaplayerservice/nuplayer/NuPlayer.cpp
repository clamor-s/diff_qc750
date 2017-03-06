/*
 * Copyright (C) 2010 The Android Open Source Project
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
#define LOG_TAG "NuPlayer"
#include <utils/Log.h>

#include "NuPlayer.h"

#include "HTTPLiveSource.h"
#include "NuPlayerDecoder.h"
#include "NuPlayerDriver.h"
#include "NuPlayerRenderer.h"
#include "NuPlayerSource.h"
#include "RTSPSource.h"
#include "StreamingSource.h"
#include "GenericSource.h"
#include "mp4/MP4Source.h"

#include "ATSParser.h"

#include <cutils/properties.h> // for property_get
#include <media/stagefright/foundation/hexdump.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/ACodec.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <gui/ISurfaceTexture.h>

#include "avc_utils.h"
#ifdef PROFILING
#include <cutils/properties.h>
#endif

#include "ESDS.h"
#include <media/stagefright/Utils.h>

namespace android {

////////////////////////////////////////////////////////////////////////////////

NuPlayer::NuPlayer()
    : mUIDValid(false),
      mVideoIsAVC(false),
      mAudioEOS(false),
      mVideoEOS(false),
      mIsRtspStreaming(false),
      mScanSourcesPending(false),
      mScanSourcesGeneration(0),
      mPollDurationGeneration(0),
      mTimeDiscontinuityPending(false),
      mFlushingAudio(NONE),
      mFlushingVideo(NONE),
      mResetInProgress(false),
      mResetPostponed(false),
      mSkipRenderingAudioUntilMediaTimeUs(-1ll),
      mSkipRenderingVideoUntilMediaTimeUs(-1ll),
      mVideoLateByUs(0ll),
      mNumFramesTotal(0ll),
      mVideoScalingMode(NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW),
#ifdef PROFILING
      mAudioSamplesTotal(0ll),
      mEndTimeUs(0ll),
      mVideoDamagedAccessUnits(0ll),
      mAudioDamagedAccessUnits(0ll),
      mProfFlags(0ll),
      mFirstFrame(true),
#endif
      mNumFramesDropped(0ll),
      mAudioFormatChange(false),
      mVideoFormatChange(false) {
}

NuPlayer::~NuPlayer() {
}

void NuPlayer::setUID(uid_t uid) {
    mUIDValid = true;
    mUID = uid;
}

void NuPlayer::setDriver(const wp<NuPlayerDriver> &driver) {
    mDriver = driver;
}

void NuPlayer::setDataSource(const sp<IStreamSource> &source) {
    sp<AMessage> msg = new AMessage(kWhatSetDataSource, id());

#ifdef PROFILING
    setProfileFlags();
#endif

    char prop[PROPERTY_VALUE_MAX];
    if (property_get("media.stagefright.use-mp4source", prop, NULL)
            && (!strcmp(prop, "1") || !strcasecmp(prop, "true"))) {
        msg->setObject("source", new MP4Source(source));
    } else {
        msg->setObject("source", new StreamingSource(source));
    }


    msg->post();
}

static bool IsHTTPLiveURL(const char *url) {
    if (!strncasecmp("http://", url, 7)
            || !strncasecmp("https://", url, 8)) {
        size_t len = strlen(url);
        if (len >= 5 && !strcasecmp(".m3u8", &url[len - 5])) {
            return true;
        }

        if (strstr(url,"m3u8")) {
            return true;
        }
    }

    return false;
}

void NuPlayer::setDataSource(
        const char *url, const KeyedVector<String8, String8> *headers) {
    sp<AMessage> msg = new AMessage(kWhatSetDataSource, id());
#ifdef PROFILING
    setProfileFlags();
    mSetDataTimeUs = ALooper::GetNowUs();
#endif
    sp<Source> source;
    if (IsHTTPLiveURL(url)) {
        source = new HTTPLiveSource(url, headers, mUIDValid, mUID);
    } else if (!strncasecmp(url, "rtsp://", 7)) {
        mIsRtspStreaming = true;
        source = new RTSPSource(url, headers, mUIDValid, mUID);
    } else {
        source = new GenericSource(url, headers, mUIDValid, mUID);
    }

    msg->setObject("source", source);
    msg->post();
}

void NuPlayer::setDataSource(int fd, int64_t offset, int64_t length) {
    sp<AMessage> msg = new AMessage(kWhatSetDataSource, id());

#ifdef PROFILING
    setProfileFlags();
#endif

    sp<Source> source = new GenericSource(fd, offset, length);
    msg->setObject("source", source);
    msg->post();
}

void NuPlayer::setVideoSurfaceTexture(const sp<ISurfaceTexture> &surfaceTexture) {
    sp<AMessage> msg = new AMessage(kWhatSetVideoNativeWindow, id());
    sp<SurfaceTextureClient> surfaceTextureClient(surfaceTexture != NULL ?
                new SurfaceTextureClient(surfaceTexture) : NULL);
    msg->setObject("native-window", new NativeWindowWrapper(surfaceTextureClient));
    msg->post();
}

void NuPlayer::setAudioSink(const sp<MediaPlayerBase::AudioSink> &sink) {
    sp<AMessage> msg = new AMessage(kWhatSetAudioSink, id());
    msg->setObject("sink", sink);
    msg->post();
}

void NuPlayer::start() {
    (new AMessage(kWhatStart, id()))->post();
}

void NuPlayer::pause() {
    (new AMessage(kWhatPause, id()))->post();
}

void NuPlayer::resume() {
    (new AMessage(kWhatResume, id()))->post();
}

void NuPlayer::resetAsync() {
    (new AMessage(kWhatReset, id()))->post();
}

void NuPlayer::seekToAsync(int64_t seekTimeUs) {
    sp<AMessage> msg = new AMessage(kWhatSeek, id());
    msg->setInt64("seekTimeUs", seekTimeUs);
    msg->post();
}

#ifdef PROFILING
void NuPlayer::setProfileFlags() {
    char value[PROPERTY_VALUE_MAX];

    size_t kNumProfileFlags =
        sizeof(kProfileProps) / sizeof(kProfileProps[0]);

    for (size_t i = 0; i < kNumProfileFlags; ++i) {
        property_get(kProfileProps[i], value, NULL);
        if (atoi(value)) {
            mProfFlags |= 1 << i;
        }
    }
    ALOGI("Profile flag is set to %lld", mProfFlags);
}
#endif

// static
bool NuPlayer::IsFlushingState(FlushStatus state, bool *needShutdown) {
    switch (state) {
        case FLUSHING_DECODER:
            if (needShutdown != NULL) {
                *needShutdown = false;
            }
            return true;

        case FLUSHING_DECODER_SHUTDOWN:
            if (needShutdown != NULL) {
                *needShutdown = true;
            }
            return true;

        default:
            return false;
    }
}

void NuPlayer::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatSetDataSource:
        {
            ALOGV("kWhatSetDataSource");

            CHECK(mSource == NULL);

            sp<RefBase> obj;
            CHECK(msg->findObject("source", &obj));

            mSource = static_cast<Source *>(obj.get());
            break;
        }

        case kWhatPollDuration:
        {
            int32_t generation;
            CHECK(msg->findInt32("generation", &generation));

            if (generation != mPollDurationGeneration) {
                // stale
                break;
            }

            int64_t durationUs;
            if (mDriver != NULL && mSource->getDuration(&durationUs) == OK) {
                sp<NuPlayerDriver> driver = mDriver.promote();
                if (driver != NULL) {
                    driver->notifyDuration(durationUs);
                }
            }

            msg->post(1000000ll);  // poll again in a second.
            break;
        }

        case kWhatSetVideoNativeWindow:
        {
            ALOGV("kWhatSetVideoNativeWindow");

            sp<RefBase> obj;
            CHECK(msg->findObject("native-window", &obj));

            mNativeWindow = static_cast<NativeWindowWrapper *>(obj.get());

            // XXX - ignore error from setVideoScalingMode for now
            setVideoScalingMode(mVideoScalingMode);
            break;
        }

        case kWhatSetAudioSink:
        {
            ALOGV("kWhatSetAudioSink");

            sp<RefBase> obj;
            CHECK(msg->findObject("sink", &obj));

            mAudioSink = static_cast<MediaPlayerBase::AudioSink *>(obj.get());
            break;
        }

        case kWhatStart:
        {
            ALOGV("kWhatStart");

            mVideoIsAVC = false;
            mAudioEOS = false;
            mVideoEOS = false;
            mSkipRenderingAudioUntilMediaTimeUs = -1;
            mSkipRenderingVideoUntilMediaTimeUs = -1;
            mVideoLateByUs = 0;
            mNumFramesTotal = 0;
            mNumFramesDropped = 0;
#ifdef PROFILING
            mAudioSamplesTotal = 0;
            mVideoDamagedAccessUnits = 0;
            mAudioDamagedAccessUnits = 0;
#endif

            mSource->start();

            mRenderer = new Renderer(
                    mAudioSink,
                    new AMessage(kWhatRendererNotify, id()));

            looper()->registerHandler(mRenderer);

            postScanSources();
            break;
        }

        case kWhatScanSources:
        {
            int32_t generation;
            CHECK(msg->findInt32("generation", &generation));
            if (generation != mScanSourcesGeneration) {
                // Drop obsolete msg.
                break;
            }

            mScanSourcesPending = false;

            ALOGV("scanning sources haveAudio=%d, haveVideo=%d",
                 mAudioDecoder != NULL, mVideoDecoder != NULL);

            bool mHadAnySourcesBefore =
                (mAudioDecoder != NULL) || (mVideoDecoder != NULL);

            if (mNativeWindow != NULL) {
                instantiateDecoder(false, &mVideoDecoder);
            }

#ifdef PROFILING
            if (mAudioSink != NULL && !(mProfFlags & kNoAudio)) {
#else
            if (mAudioSink != NULL) {
#endif
                instantiateDecoder(true, &mAudioDecoder);
            }

            if (!mHadAnySourcesBefore
                    && (mAudioDecoder != NULL || mVideoDecoder != NULL)) {
                // This is the first time we've found anything playable.

                uint32_t flags = mSource->flags();

                if (flags & Source::FLAG_DYNAMIC_DURATION) {
                    schedulePollDuration();
                }
            }

            status_t err;
            if ((err = mSource->feedMoreTSData()) != OK) {
                if (mAudioDecoder == NULL && mVideoDecoder == NULL) {
                    // We're not currently decoding anything (no audio or
                    // video tracks found) and we just ran out of input data.

                    if (err == ERROR_END_OF_STREAM) {
                        notifyListener(MEDIA_PLAYBACK_COMPLETE, 0, 0);
                    } else {
                        notifyListener(MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, err);
                    }
                }
                break;
            }

#ifdef PROFILING
            if (((mAudioDecoder == NULL && mAudioSink != NULL)
                  && !(mProfFlags & kNoAudio) ) 
                  || (mVideoDecoder == NULL && mNativeWindow != NULL)) {
#else
            if ((mAudioDecoder == NULL && mAudioSink != NULL)
                    || (mVideoDecoder == NULL && mNativeWindow != NULL)) {
#endif
                msg->post(100000ll);
                mScanSourcesPending = true;
            }
            break;
        }

        case kWhatVideoNotify:
        case kWhatAudioNotify:
        {
            bool audio = msg->what() == kWhatAudioNotify;

            sp<AMessage> codecRequest;
            CHECK(msg->findMessage("codec-request", &codecRequest));

            int32_t what;
            CHECK(codecRequest->findInt32("what", &what));

            if (what == ACodec::kWhatFillThisBuffer) {
                status_t err = feedDecoderInputData(
                        audio, codecRequest);

                if (err == -EWOULDBLOCK) {
                    if (mSource->feedMoreTSData() == OK) {
                        msg->post(10000ll);
                    }
                }
            } else if (what == ACodec::kWhatEOS) {
                int32_t err;
                CHECK(codecRequest->findInt32("err", &err));

                if (err == ERROR_END_OF_STREAM) {
                    ALOGV("got %s decoder EOS", audio ? "audio" : "video");
#ifdef PROFILING
                    if (mIsRtspStreaming) {
                        if (!audio) {
                            mSource->getDamagedAccessUnits(&mVideoDamagedAccessUnits, audio);
                        } else {
                            mSource->getDamagedAccessUnits(&mAudioDamagedAccessUnits, audio);
                        }
                    }
#endif
                } else {
                    ALOGE("got %s decoder EOS w/ error %d",
                         audio ? "audio" : "video",
                         err);
                }

                mRenderer->queueEOS(audio, err);
            } else if (what == ACodec::kWhatFlushCompleted) {
                bool needShutdown;

                if (audio) {
                    CHECK(IsFlushingState(mFlushingAudio, &needShutdown));
                    mFlushingAudio = FLUSHED;
                } else {
                    CHECK(IsFlushingState(mFlushingVideo, &needShutdown));
                    mFlushingVideo = FLUSHED;

                    mVideoLateByUs = 0;
                }

                ALOGD("decoder %s flush completed", audio ? "audio" : "video");

                if (needShutdown) {
                    ALOGV("initiating %s decoder shutdown",
                         audio ? "audio" : "video");

                    (audio ? mAudioDecoder : mVideoDecoder)->initiateShutdown();

                    if (audio) {
                        mFlushingAudio = SHUTTING_DOWN_DECODER;
                    } else {
                        mFlushingVideo = SHUTTING_DOWN_DECODER;
                    }
                }

                finishFlushIfPossible();
            } else if (what == ACodec::kWhatOutputFormatChanged) {
                if (audio) {
                    int32_t numChannels;
                    CHECK(codecRequest->findInt32("channel-count", &numChannels));

                    int32_t sampleRate;
                    CHECK(codecRequest->findInt32("sample-rate", &sampleRate));

                    ALOGV("Audio output format changed to %d Hz, %d channels",
                         sampleRate, numChannels);

                    mAudioSink->close();

                    audio_output_flags_t flags;
                    int64_t durationUs;
                    // FIXME: we should handle the case where the video decoder is created after
                    // we receive the format change indication. Current code will just make that
                    // we select deep buffer with video which should not be a problem as it should
                    // not prevent from keeping A/V sync.
                    if (mVideoDecoder == NULL &&
                            mSource->getDuration(&durationUs) == OK &&
                            durationUs > AUDIO_SINK_MIN_DEEP_BUFFER_DURATION_US) {
                        flags = AUDIO_OUTPUT_FLAG_DEEP_BUFFER;
                    } else {
                        flags = AUDIO_OUTPUT_FLAG_NONE;
                    }

                    int32_t channelMask;
                    if (!codecRequest->findInt32("channel-mask", &channelMask)) {
                        channelMask = CHANNEL_MASK_USE_CHANNEL_ORDER;
                    }

                    CHECK_EQ(mAudioSink->open(
                                sampleRate,
                                numChannels,
                                (audio_channel_mask_t)channelMask,
                                AUDIO_FORMAT_PCM_16_BIT,
                                8 /* bufferCount */,
                                NULL,
                                NULL,
                                flags),
                             (status_t)OK);
                    mAudioSink->start();

                    mRenderer->signalAudioSinkChanged();
                } else {
                    // video

                    int32_t width, height;
                    CHECK(codecRequest->findInt32("width", &width));
                    CHECK(codecRequest->findInt32("height", &height));

                    int32_t cropLeft, cropTop, cropRight, cropBottom;
                    CHECK(codecRequest->findRect(
                                "crop",
                                &cropLeft, &cropTop, &cropRight, &cropBottom));

                    ALOGV("Video output format changed to %d x %d "
                         "(crop: %d x %d @ (%d, %d))",
                         width, height,
                         (cropRight - cropLeft + 1),
                         (cropBottom - cropTop + 1),
                         cropLeft, cropTop);

                    notifyListener(
                            MEDIA_SET_VIDEO_SIZE,
                            cropRight - cropLeft + 1,
                            cropBottom - cropTop + 1);
                }
            } else if (what == ACodec::kWhatShutdownCompleted) {
                ALOGV("%s shutdown completed", audio ? "audio" : "video");
#ifdef PROFILING
                if (mProfFlags & kShowStats) {
                    if (!audio && mVideoEOS) {
                        int32_t renderDrop;
                        CHECK(codecRequest->findInt32("fdrop", &renderDrop));
                        int64_t latencyTimeUs = mStartTimeUs - mSetDataTimeUs;
                        ALOGI("---------Video Profiling Statistics---------");
                        int fps = (mNumFramesTotal / ((mEndTimeUs - mStartTimeUs)/1E6)) ;
                        ALOGI("---------FPS : %d", fps);
                        ALOGI("---------Frames Dropped By Parser : %lld",mNumFramesDropped);
                        ALOGI("---------Frames Dropped By Renderer : %d",renderDrop);
                        ALOGI("---------Total Frames Dropped : %lld", mNumFramesDropped + renderDrop);
                        ALOGI("---------Video Damaged Access units : %d", mVideoDamagedAccessUnits);
                        ALOGI("---------Total Video Frames : %lld", mNumFramesTotal + mVideoDamagedAccessUnits);
                        ALOGI("---------Buffering Latency: = %.3f secs", latencyTimeUs/1E6);
                        ALOGI("---------Duration : %.3f sec", (mEndTimeUs - mStartTimeUs)/1E6);
                    } else if (audio && mAudioEOS) {
                        ALOGI("---------Audio Profiling Statistics---------");
                        ALOGI("---------Audio Damaged Access units: %d", mAudioDamagedAccessUnits);
                        ALOGI("---------Total Audio Samples : %lld", mAudioSamplesTotal + mAudioDamagedAccessUnits);
                    }
                }
#endif
                if (audio) {
                    mAudioDecoder.clear();

                    CHECK_EQ((int)mFlushingAudio, (int)SHUTTING_DOWN_DECODER);
                    mFlushingAudio = SHUT_DOWN;
                } else {
                    mVideoDecoder.clear();

                    CHECK_EQ((int)mFlushingVideo, (int)SHUTTING_DOWN_DECODER);
                    mFlushingVideo = SHUT_DOWN;
                }

                finishFlushIfPossible();
            } else if (what == ACodec::kWhatError) {
                ALOGE("Received error from %s decoder, aborting playback.",
                     audio ? "audio" : "video");

                mRenderer->queueEOS(audio, UNKNOWN_ERROR);
            } else if (what == ACodec::kWhatDrainThisBuffer) {
#ifdef PROFILING
                if (mFirstFrame) {
                    mFirstFrame = false;
                    mStartTimeUs = ALooper::GetNowUs();
                    if (mProfFlags & kNoAvsync) {
                        mRenderer->enableNoAvSync();
                    }
                }
#endif
                renderBuffer(audio, codecRequest);
            } else {
                ALOGV("Unhandled codec notification %d.", what);
            }

            break;
        }

        case kWhatRendererNotify:
        {
            int32_t what;
            CHECK(msg->findInt32("what", &what));

            if (what == Renderer::kWhatEOS) {
                int32_t audio;
                CHECK(msg->findInt32("audio", &audio));

                int32_t finalResult;
                CHECK(msg->findInt32("finalResult", &finalResult));

                if (audio) {
                    mAudioEOS = true;
                } else {
                    mVideoEOS = true;
#ifdef PROFILING
                    if (mProfFlags & kShowStats) {
                        mEndTimeUs = ALooper::GetNowUs();
                    }
#endif
                }

                if (finalResult == ERROR_END_OF_STREAM) {
                    ALOGV("reached %s EOS", audio ? "audio" : "video");
                } else {
                    ALOGE("%s track encountered an error (%d)",
                         audio ? "audio" : "video", finalResult);

                    notifyListener(
                            MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, finalResult);
                }

                if ((mAudioEOS || mAudioDecoder == NULL)
                        && (mVideoEOS || mVideoDecoder == NULL)) {
                    notifyListener(MEDIA_PLAYBACK_COMPLETE, 0, 0);
                }
            } else if (what == Renderer::kWhatPosition) {
                int64_t positionUs;
                CHECK(msg->findInt64("positionUs", &positionUs));

                CHECK(msg->findInt64("videoLateByUs", &mVideoLateByUs));

                if (mDriver != NULL) {
                    sp<NuPlayerDriver> driver = mDriver.promote();
                    if (driver != NULL) {
                        driver->notifyPosition(positionUs);

                        driver->notifyFrameStats(
                                mNumFramesTotal, mNumFramesDropped);
                    }
                }
            } else if (what == Renderer::kWhatFlushComplete) {
                CHECK_EQ(what, (int32_t)Renderer::kWhatFlushComplete);

                int32_t audio;
                CHECK(msg->findInt32("audio", &audio));

                ALOGV("renderer %s flush completed.", audio ? "audio" : "video");
            } else if (what == Renderer::kWhatVideoRenderingStart) {
                notifyListener(MEDIA_INFO, MEDIA_INFO_RENDERING_START, 0);
            }
            break;
        }

        case kWhatMoreDataQueued:
        {
            break;
        }

        case kWhatReset:
        {
            ALOGV("kWhatReset");

            cancelPollDuration();

            if (mRenderer != NULL) {
                // There's an edge case where the renderer owns all output
                // buffers and is paused, therefore the decoder will not read
                // more input data and will never encounter the matching
                // discontinuity. To avoid this, we resume the renderer.

                if (mFlushingAudio == AWAITING_DISCONTINUITY
                        || mFlushingVideo == AWAITING_DISCONTINUITY) {
                    mRenderer->resume();
                }
            }

            if (mFlushingAudio != NONE || mFlushingVideo != NONE) {
                // We're currently flushing, postpone the reset until that's
                // completed.

                ALOGV("postponing reset mFlushingAudio=%d, mFlushingVideo=%d",
                      mFlushingAudio, mFlushingVideo);

                mResetPostponed = true;
                break;
            }

            if (mAudioDecoder == NULL && mVideoDecoder == NULL) {
                finishReset();
                break;
            }

            mTimeDiscontinuityPending = true;

            if (mAudioDecoder != NULL) {
                flushDecoder(true /* audio */, true /* needShutdown */);
            }

            if (mVideoDecoder != NULL) {
                flushDecoder(false /* audio */, true /* needShutdown */);
            }

            mResetInProgress = true;
            break;
        }

        case kWhatSeek:
        {
            int64_t seekTimeUs;
            CHECK(msg->findInt64("seekTimeUs", &seekTimeUs));

            ALOGV("kWhatSeek seekTimeUs=%lld us (%.2f secs)",
                 seekTimeUs, seekTimeUs / 1E6);

            mSource->seekTo(seekTimeUs);

            if (mDriver != NULL) {
                sp<NuPlayerDriver> driver = mDriver.promote();
                if (driver != NULL) {
                    driver->notifySeekComplete();
                }
            }

            break;
        }

        case kWhatPause:
        {
            if (mIsRtspStreaming) {
                mSource->pause();
            }

            CHECK(mRenderer != NULL);
            mRenderer->pause();
            break;
        }

        case kWhatResume:
        {
            CHECK(mRenderer != NULL);
            mRenderer->resume();

            if (mIsRtspStreaming) {
                mSource->resume();
            }
            break;
        }

        default:
            TRESPASS();
            break;
    }
}

void NuPlayer::finishFlushIfPossible() {
    if (mFlushingAudio != FLUSHED && mFlushingAudio != SHUT_DOWN) {
        ALOGE("mFlushingAudio != FLUSHED && mFlushingAudio != SHUT_DOWN.");
        return;
    }

    if (mFlushingVideo != FLUSHED && mFlushingVideo != SHUT_DOWN) {
        ALOGE("mFlushingVideo != FLUSHED && mFlushingVideo != SHUT_DOWN.");
        return;
    }

    ALOGV("both audio and video are flushed now.");

    if (mTimeDiscontinuityPending) {
        mRenderer->signalTimeDiscontinuity();
        mTimeDiscontinuityPending = false;
    }

    if (mAudioDecoder != NULL) {
        mAudioDecoder->signalResume();
    }

    if (mVideoDecoder != NULL) {
        mVideoDecoder->signalResume();
    }

    mFlushingAudio = NONE;
    mFlushingVideo = NONE;

    if (mResetInProgress) {
        ALOGV("reset completed");

        mResetInProgress = false;
        finishReset();
    } else if (mResetPostponed) {
        (new AMessage(kWhatReset, id()))->post();
        mResetPostponed = false;
    } else if (mAudioDecoder == NULL || mVideoDecoder == NULL) {
        postScanSources();
    }
}

void NuPlayer::finishReset() {
    CHECK(mAudioDecoder == NULL);
    CHECK(mVideoDecoder == NULL);

    ++mScanSourcesGeneration;
    mScanSourcesPending = false;

    mRenderer.clear();

    if (mSource != NULL) {
        mSource->stop();
        mSource.clear();
    }

    if (mDriver != NULL) {
        sp<NuPlayerDriver> driver = mDriver.promote();
        if (driver != NULL) {
            driver->notifyResetComplete();
        }
    }
}

void NuPlayer::postScanSources() {
    if (mScanSourcesPending) {
        return;
    }

    sp<AMessage> msg = new AMessage(kWhatScanSources, id());
    msg->setInt32("generation", mScanSourcesGeneration);
    msg->post();

    mScanSourcesPending = true;
}

status_t NuPlayer::instantiateDecoder(bool audio, sp<Decoder> *decoder) {
    if (*decoder != NULL) {
        return OK;
    }

    sp<AMessage> format = mSource->getFormat(audio);

    if (format == NULL) {
        return -EWOULDBLOCK;
    }

//    const char *mime;
//    CHECK(meta->findCString(kKeyMIMEType, &mime));
    AString mime;
    CHECK(format->findString("mime", &mime));

    ALOGV("%s: %s -> mime %s \n",
        __FUNCTION__, audio ? "audio" : "video", mime.c_str());
    if (!audio) {
        mInitialMimeVideo = mime;
        mVideoIsAVC = !strcasecmp(MEDIA_MIMETYPE_VIDEO_AVC, mime.c_str());
    }
    else {
        mInitialMimeAudio = mime;
    }

    sp<AMessage> notify =
        new AMessage(audio ? kWhatAudioNotify : kWhatVideoNotify,
                     id());

    *decoder = audio ? new Decoder(notify) :
                       new Decoder(notify, mNativeWindow);
    looper()->registerHandler(*decoder);

    (*decoder)->configure(format);

    int64_t durationUs;
    if (mDriver != NULL && mSource->getDuration(&durationUs) == OK) {
        sp<NuPlayerDriver> driver = mDriver.promote();
        if (driver != NULL) {
            driver->notifyDuration(durationUs);
        }
    }

#ifdef PROFILING
    if (mProfFlags & kShowStats) {
        (*decoder)->enableProfileFlags();
    }
#endif

    return OK;
}

status_t NuPlayer::feedDecoderInputData(bool audio, const sp<AMessage> &msg) {
    sp<AMessage> reply;
    CHECK(msg->findMessage("reply", &reply));

    if ((audio && IsFlushingState(mFlushingAudio))
            || (!audio && IsFlushingState(mFlushingVideo))) {
        reply->setInt32("err", INFO_DISCONTINUITY);
        reply->post();
        return OK;
    }

    sp<ABuffer> accessUnit;

    bool dropAccessUnit;
    do {
        status_t err = mSource->dequeueAccessUnit(audio, &accessUnit);

        if (err == -EWOULDBLOCK) {
            return err;
        } else if (err != OK) {
            bool &mediaFormatChange = audio ? mAudioFormatChange : mVideoFormatChange;
            bool bSkipThisBuffer = false;
            mediaFormatChange = false;
            if (err == INFO_DISCONTINUITY) {
                int32_t type;
                int32_t keyFormatChange = 0;
                sp<AMessage> extra = NULL;

                CHECK(accessUnit->meta()->findInt32("discontinuity", &type));
                accessUnit->meta()->findMessage("extra", &extra);
                mediaFormatChange =
                    (audio &&
                     (type & ATSParser::DISCONTINUITY_AUDIO_FORMAT))
                    || (!audio &&
                            (type & ATSParser::DISCONTINUITY_VIDEO_FORMAT));

                bool timeChange = (type & ATSParser::DISCONTINUITY_TIME) != 0;

                ALOGI("%s discontinuity (formatChange=%d, time=%d) type 0x%x",
                     audio ? "audio" : "video", mediaFormatChange, timeChange, type);

                if (mediaFormatChange && (extra != NULL)) {
                    int32_t formatChange = 0;
                    extra->findInt32(
                        IStreamListener::kKeyDiscontinuityMask,
                        (int32_t *)&formatChange);
                    formatChange = formatChange & ATSParser::DISCONTINUITY_FORMATCHANGE;
                    bSkipThisBuffer = ((formatChange == ATSParser::DISCONTINUITY_FORMATCHANGE) ||
                                   (formatChange == ATSParser::DISCONTINUITY_VIDEO_FORMAT));
                    ALOGV("NU: IStreamListener::kKeyDiscontinuityMask %d skip %d time change %d \n",
                        formatChange, (int)bSkipThisBuffer, (int)timeChange);
                }

                if (audio) {
                    mSkipRenderingAudioUntilMediaTimeUs = -1;
                } else {
                    mSkipRenderingVideoUntilMediaTimeUs = -1;
                }
                if (timeChange) {
                    sp<AMessage> extra;
                    if (accessUnit->meta()->findMessage("extra", &extra)
                            && extra != NULL) {
                        int64_t resumeAtMediaTimeUs;
                        if (extra->findInt64(
                                    "resume-at-mediatimeUs", &resumeAtMediaTimeUs)) {
                            ALOGI("suppressing rendering of %s until %lld us",
                                    audio ? "audio" : "video", resumeAtMediaTimeUs);

                            if (audio) {
                                mSkipRenderingAudioUntilMediaTimeUs =
                                    resumeAtMediaTimeUs;
                            } else {
                                mSkipRenderingVideoUntilMediaTimeUs =
                                    resumeAtMediaTimeUs;
                            }
                        }
                    }
                }

                mTimeDiscontinuityPending =
                    mTimeDiscontinuityPending || timeChange;

                if (!bSkipThisBuffer && (mediaFormatChange || timeChange)) {
                    flushDecoder(audio, mediaFormatChange);
                    mediaFormatChange = false;
                }
                else if (bSkipThisBuffer) {
                    mFlushingAudio = NONE;
                    mFlushingVideo = NONE;
                    continue;
                }
                else {
                    // This stream is unaffected by the discontinuity
                    if (audio) {
                        mFlushingAudio = FLUSHED;
                    } else {
                        mFlushingVideo = FLUSHED;
                    }

                    finishFlushIfPossible();
                    ALOGI("%s: %s discontinuity -EWOULDBLOCK ", __FUNCTION__,
                     audio ? "audio" : "video");
                    return -EWOULDBLOCK;
                }
                if ((type & ATSParser::DISCONTINUITY_TIME) && (mIsRtspStreaming)) {
                    if (!audio && mVideoIsAVC) {
                        ALOGV("Time Discontinuity detected. Sending CSD again to the decoder...");
                        size_t csdIndex, size;
                        size = mVideoDecoder->getCsdSize();

                        for (csdIndex = 0; csdIndex < size; csdIndex++) {
                            sp<ABuffer> outBuffer;
                            outBuffer = mVideoDecoder->getCsdAtIndex(csdIndex);
                            outBuffer->meta()->setInt64("timeUs", 0);

                            CHECK(mIsRtspStreaming);
                            sp<RTSPSource> source;
                            source = static_cast<RTSPSource *>(mSource.get());
                            source->queueCsd(audio, outBuffer);
                        }
                    }
                }
            }
            reply->setInt32("err", err);
            reply->post();
            ALOGI("%s: %s discontinuity err 0x%x ", __FUNCTION__,
                     audio ? "audio" : "video", err);
            return OK;
        }

        if (!audio) {
            ++mNumFramesTotal;
#ifdef PROFILING
        } else {
            ++mAudioSamplesTotal;
        }
#endif

        dropAccessUnit = false;
        if (!audio
                && mVideoLateByUs > 100000ll
                && mVideoIsAVC
                && !IsAVCReferenceFrame(accessUnit)) {
            dropAccessUnit = true;
            ++mNumFramesDropped;
        }
    } while (dropAccessUnit);

    // ALOGV("returned a valid buffer of %s data", audio ? "audio" : "video");

#if 0
    int64_t mediaTimeUs;
    CHECK(accessUnit->meta()->findInt64("timeUs", &mediaTimeUs));
    ALOGV("feeding %s input buffer at media time %.2f secs",
         audio ? "audio" : "video",
         mediaTimeUs / 1E6);
#endif
    bool &formatChange = audio ? mAudioFormatChange : mVideoFormatChange;
    if (formatChange)
    {
        bool shutDownDecoder = false;
        sp<AMessage> meta = mSource->getFormat(audio);

        if (meta != NULL) {
            AString newMime;
            CHECK(meta->findString("mime", &newMime));
            if (!audio)
            {
                if (strcmp(mInitialMimeVideo.c_str(),newMime.c_str()))
                {
                    mInitialMimeVideo = newMime;
                    shutDownDecoder = true;
                }
            }
            else
            {
                if (strcmp(mInitialMimeAudio.c_str(),newMime.c_str()))
                {
                    mInitialMimeAudio = newMime;
                    shutDownDecoder = true;
                }
            }
            if (shutDownDecoder)
            {
                ALOGI("####FormatChange:%s valid buffer of %s data",
                    newMime.c_str(), audio ? "audio" : "video");
                flushDecoder(audio, shutDownDecoder);
            }
            formatChange = false;
        }
        else
        {
            ALOGV("%s: %s -> check format next time \n",
                __FUNCTION__, audio ? "audio" : "video");
        }
    }

    reply->setBuffer("buffer", accessUnit);
    reply->post();

    return OK;
}

void NuPlayer::renderBuffer(bool audio, const sp<AMessage> &msg) {
    // ALOGV("renderBuffer %s", audio ? "audio" : "video");

    sp<AMessage> reply;
    CHECK(msg->findMessage("reply", &reply));

    if (IsFlushingState(audio ? mFlushingAudio : mFlushingVideo)) {
        // We're currently attempting to flush the decoder, in order
        // to complete this, the decoder wants all its buffers back,
        // so we don't want any output buffers it sent us (from before
        // we initiated the flush) to be stuck in the renderer's queue.

        ALOGV("we're still flushing the %s decoder, sending its output buffer"
             " right back.", audio ? "audio" : "video");

        reply->post();
        return;
    }

    sp<ABuffer> buffer;
    CHECK(msg->findBuffer("buffer", &buffer));

    int64_t &skipUntilMediaTimeUs =
        audio
            ? mSkipRenderingAudioUntilMediaTimeUs
            : mSkipRenderingVideoUntilMediaTimeUs;

    if (skipUntilMediaTimeUs >= 0) {
        int64_t mediaTimeUs;
        CHECK(buffer->meta()->findInt64("timeUs", &mediaTimeUs));

        if (mediaTimeUs < skipUntilMediaTimeUs) {
            ALOGV("dropping %s buffer at time %lld as requested.",
                 audio ? "audio" : "video",
                 mediaTimeUs);

            reply->post();
            return;
        }

        skipUntilMediaTimeUs = -1;
    }

    mRenderer->queueBuffer(audio, buffer, reply);
}

void NuPlayer::notifyListener(int msg, int ext1, int ext2) {
    if (mDriver == NULL) {
        return;
    }

    sp<NuPlayerDriver> driver = mDriver.promote();

    if (driver == NULL) {
        return;
    }

    driver->notifyListener(msg, ext1, ext2);
}

void NuPlayer::flushDecoder(bool audio, bool needShutdown) {
    if ((audio && mAudioDecoder == NULL) || (!audio && mVideoDecoder == NULL)) {
        ALOGI("flushDecoder %s without decoder present",
             audio ? "audio" : "video");
    }

    // Make sure we don't continue to scan sources until we finish flushing.
    ++mScanSourcesGeneration;
    mScanSourcesPending = false;

    mRenderer->flush(audio);
    (audio ? mAudioDecoder : mVideoDecoder)->signalFlush();

    FlushStatus newStatus =
        needShutdown ? FLUSHING_DECODER_SHUTDOWN : FLUSHING_DECODER;

    if (audio) {
        ALOGI("flushDecoder %s decoder present mFlushingAudio 0x%x ",
            audio ? "audio" : "video", mFlushingAudio);
        CHECK(mFlushingAudio == NONE
                || mFlushingAudio == AWAITING_DISCONTINUITY);

        mFlushingAudio = newStatus;

        if (mFlushingVideo == NONE) {
            mFlushingVideo = (mVideoDecoder != NULL)
                ? AWAITING_DISCONTINUITY
                : FLUSHED;
        }
    } else {
        CHECK(mFlushingVideo == NONE
                || mFlushingVideo == AWAITING_DISCONTINUITY);

        mFlushingVideo = newStatus;

        if (mFlushingAudio == NONE) {
            mFlushingAudio = (mAudioDecoder != NULL)
                ? AWAITING_DISCONTINUITY
                : FLUSHED;
        }
    }
}

sp<AMessage> NuPlayer::Source::getFormat(bool audio) {
    sp<MetaData> meta = getFormatMeta(audio);

    if (meta == NULL) {
        return NULL;
    }

    sp<AMessage> msg = new AMessage;

    if(convertMetaDataToMessage(meta, &msg) == OK) {
        return msg;
    }
    return NULL;
}

status_t NuPlayer::setVideoScalingMode(int32_t mode) {
    mVideoScalingMode = mode;
    if (mNativeWindow != NULL
            && mNativeWindow->getNativeWindow() != NULL) {
        status_t ret = native_window_set_scaling_mode(
                mNativeWindow->getNativeWindow().get(), mVideoScalingMode);
        if (ret != OK) {
            ALOGE("Failed to set scaling mode (%d): %s",
                -ret, strerror(-ret));
            return ret;
        }
    }
    return OK;
}

void NuPlayer::schedulePollDuration() {
    sp<AMessage> msg = new AMessage(kWhatPollDuration, id());
    msg->setInt32("generation", mPollDurationGeneration);
    msg->post();
}

void NuPlayer::cancelPollDuration() {
    ++mPollDurationGeneration;
}

}  // namespace android
