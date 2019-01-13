//
// Created by user on 2019/1/10.
//

#include "HTS_xs.h"
#include "HTS_engine.h"

constexpr int kDefaultFrameRate = 48000;
constexpr float kDefaultAmplitude = 0.01;
constexpr float kDefaultFrequency = 440.0;
constexpr double kTwoPi = M_PI * 2;
constexpr int32_t offset = 0;

void HTS_xs::render( int16_t *buffer, int32_t channelStride, int32_t numFrames) {
    int sampleIndex = 0;
    int i;
    for (i = 0; i < numFrames && (offset+i) < buffersize; i++) {
        buffer[sampleIndex] = HTS_wavebuffer  [ offset+i ];
        sampleIndex += channelStride;
    }
    for(;i<numFrames;i++){
        buffer[sampleIndex] = 0;
        sampleIndex += channelStride;
    }

    offset += numFrames;
}