//
// Created by user on 2019/1/10.
//

#ifndef HTSOBOE_PLAYAUDIOENGINE_H
#define HTSOBOE_PLAYAUDIOENGINE_H

class PlayAudioEngine : oboe::AudioStreamCallback {

public:
    PlayAudioEngine();

    ~PlayAudioEngine();

    // oboe::StreamCallback methods
    oboe::DataCallbackResult
    onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames);

    void onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error);


private:
    oboe::AudioApi mAudioApi = oboe::AudioApi::Unspecified;
    int32_t mPlaybackDeviceId = oboe::kUnspecified;
    int32_t mSampleRate;
    int32_t mChannelCount;
    bool mIsToneOn = false;
    int32_t mFramesPerBurst;
    double mCurrentOutputLatencyMillis = 0;
    int32_t mBufferSizeSelection = kBufferSizeAutomatic;
    bool mIsLatencyDetectionSupported = false;
    oboe::AudioStream *mPlayStream;
    std::unique_ptr<oboe::LatencyTuner> mLatencyTuner;
    std::mutex mRestartingLock;

    // The SineGenerators generate audio data, feel free to replace with your own audio generators
    std::array<SineGenerator, kMaximumChannelCount> mOscillators;

    void createPlaybackStream();

    void closeOutputStream();

    void restartStream();

    void setupPlaybackStreamParameters(oboe::AudioStreamBuilder *builder);


};

#endif //HTSOBOE_PLAYAUDIOENGINE_H
