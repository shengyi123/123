//
// Created by user on 2019/1/10.
//

#ifndef HTSOBOE_HTS_XS_H
#define HTSOBOE_HTS_XS_H

#include <math.h>
#include <cstdint>
#include "Hts_engine.h"

class HTS_xs
{
public:


    void render(int16_t *buffer, int32_t channelStride, int32_t numFrames);



private:
    double mAmplitude;
    double mPhase = 0.0;
    int32_t mFrameRate;
    double mPhaseIncrement;
    double mPhaseIncrementLow;
    double mPhaseIncrementHigh;
    double mUpScaler = 1.0;
    double mDownScaler = 1.0;
    bool   mGoingUp = false;
    bool   mSweeping = false;
};

#endif //HTSOBOE_HTS_XS_H
