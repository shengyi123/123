//
// Created by user on 2019/1/10.
//

#include <trace.h>
#include <inttypes.h>

#include "PlayAudioEngine.h"
#include "logging_macros.h"

constexpr int64_t kNanosPerMillisecond = 1000000; // Use int64_t to avoid overflows in calculations
constexpr int32_t kDefaultChannelCount = 2; // Stereo


PlayAudioEngine::PlayAudioEngine() {

    // Initialize the trace functions, this enables you to output trace statements without
    // blocking. See https://developer.android.com/studio/profile/systrace-commandline.html
    Trace::initialize();

    mChannelCount = kDefaultChannelCount;
    createPlaybackStream();
}

PlayAudioEngine::~PlayAudioEngine() {

    closeOutputStream();
}

void PlayAudioEngine::createPlaybackStream() {

    oboe::AudioStreamBuilder builder;
    setupPlaybackStreamParameters(&builder);

    oboe::Result result = builder.openStream(&mPlayStream);

    if (result == oboe::Result::OK && mPlayStream != nullptr) {

        mSampleRate = mPlayStream->getSampleRate();
        mFramesPerBurst = mPlayStream->getFramesPerBurst();

        int channelCount = mPlayStream->getChannelCount();
        if (channelCount != mChannelCount){
            LOGW("Requested %d channels but received %d", mChannelCount, channelCount);
        }

        // Set the buffer size to the burst size - this will give us the minimum possible latency
        mPlayStream->setBufferSizeInFrames(mFramesPerBurst);

        // TODO: Implement Oboe_convertStreamToText
        // PrintAudioStreamInfo(mPlayStream);
        prepareOscillators();

        // Create a latency tuner which will automatically tune our buffer size.
        mLatencyTuner = std::unique_ptr<oboe::LatencyTuner>(new oboe::LatencyTuner(*mPlayStream));
        // Start the stream - the dataCallback function will start being called
        result = mPlayStream->requestStart();
        if (result != oboe::Result::OK) {
            LOGE("Error starting stream. %s", oboe::convertToText(result));
        }

        mIsLatencyDetectionSupported = (mPlayStream->getTimestamp(CLOCK_MONOTONIC, 0, 0) !=
                                        oboe::Result::ErrorUnimplemented);

    } else {
        LOGE("Failed to create stream. Error: %s", oboe::convertToText(result));
    }
}

void PlayAudioEngine::setupPlaybackStreamParameters(oboe::AudioStreamBuilder *builder) {
    builder->setAudioApi(mAudioApi);
    builder->setDeviceId(mPlaybackDeviceId);
    builder->setChannelCount(mChannelCount);

    // We request EXCLUSIVE mode since this will give us the lowest possible latency.
    // If EXCLUSIVE mode isn't available the builder will fall back to SHARED mode.
    builder->setSharingMode(oboe::SharingMode::Exclusive);
    builder->setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder->setCallback(this);
}

void PlayAudioEngine::closeOutputStream() {

    if (mPlayStream != nullptr) {
        oboe::Result result = mPlayStream->requestStop();
        if (result != oboe::Result::OK) {
            LOGE("Error stopping output stream. %s", oboe::convertToText(result));
        }

        result = mPlayStream->close();
        if (result != oboe::Result::OK) {
            LOGE("Error closing output stream. %s", oboe::convertToText(result));
        }
    }
}

oboe::DataCallbackResult
PlayAudioEngine::onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) {

    int32_t bufferSize = audioStream->getBufferSizeInFrames();

    if (mBufferSizeSelection == kBufferSizeAutomatic) {
        mLatencyTuner->tune();
    } else if (bufferSize != (mBufferSizeSelection * mFramesPerBurst)) {
        auto setBufferResult = audioStream->setBufferSizeInFrames(mBufferSizeSelection * mFramesPerBurst);
        if (setBufferResult == oboe::Result::OK) bufferSize = setBufferResult.value();
    }

    /**
     * The following output can be seen by running a systrace. Tracing is preferable to logging
     * inside the callback since tracing does not block.
     *
     * See https://developer.android.com/studio/profile/systrace-commandline.html
     */
    auto underrunCountResult = audioStream->getXRunCount();

    Trace::beginSection("numFrames %d, Underruns %d, buffer size %d",
                        numFrames, underrunCountResult.value(), bufferSize);

    int32_t channelCount = audioStream->getChannelCount();

    // If the tone is on we need to use our synthesizer to render the audio data for the sine waves
    /*if (audioStream->getFormat() == oboe::AudioFormat::Float) {
        if (mIsToneOn) {
            for (int i = 0; i < channelCount; ++i) {
                mOscillators[i].render(static_cast<float *>(audioData) + i, channelCount, numFrames);
            }
        } else {
            memset(static_cast<uint8_t *>(audioData), 0,
                   sizeof(float) * channelCount * numFrames);
        }
    } else {
        if (mIsToneOn) {
            for (int i = 0; i < channelCount; ++i) {
                mOscillators[i].render(static_cast<int16_t *>(audioData) + i, channelCount, numFrames);
            }
        } else {
            memset(static_cast<uint8_t *>(audioData), 0,
                   sizeof(int16_t) * channelCount * numFrames);
        }
    }*/

    HTS_xs.render( static_cast<int16_t *>(audioData), channelCount , numFrames);

    /*if (mIsLatencyDetectionSupported) {
        calculateCurrentOutputLatencyMillis(audioStream, &mCurrentOutputLatencyMillis);
    }*/

    Trace::endSection();
    return oboe::DataCallbackResult::Continue;
}

void PlayAudioEngine::onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) {
    if (error == oboe::Result::ErrorDisconnected) restartStream();
}

void PlayAudioEngine::restartStream() {

    LOGI("Restarting stream");

    if (mRestartingLock.try_lock()) {
        closeOutputStream();
        createPlaybackStream();
        mRestartingLock.unlock();
    } else {
        LOGW("Restart stream operation already in progress - ignoring this request");
        // We were unable to obtain the restarting lock which means the restart operation is currently
        // active. This is probably because we received successive "stream disconnected" events.
        // Internal issue b/63087953
    }
}