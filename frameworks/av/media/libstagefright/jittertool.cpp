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
#define LOG_TAG "JitterTool"
#include <utils/Log.h>

#include <cassert>
#include <math.h>
#include <media/stagefright/foundation/ALooper.h>

#include "include/jittertool.h"

namespace android {

JitterTool::JitterTool(uint32_t nTicks)
        :mInitCheck(NO_INIT),
        pTicks(NULL),
        nLastTime(0),
        nTicksMax(nTicks),
        nTickCount(0),
        nHightestJitter(0),
        bShow(false)
{
    assert(nTicks > 0);

    pTicks = new uint64_t[nTicks];
    if (!pTicks)
    {
        return;
    }
    mInitCheck = OK;
    LOGV("%s[%d] out \n", __FUNCTION__, __LINE__);
}

JitterTool::~JitterTool()
{
    if (pTicks)
        delete[] pTicks;
    LOGV("%s[%d] out \n", __FUNCTION__, __LINE__);
}

status_t JitterTool::initCheck() const {
    LOGV("%s[%d] mInitCheck %d out \n",
        __FUNCTION__, __LINE__, mInitCheck);
    return mInitCheck;
}

void JitterTool::AddPoint(uint64_t ts)
{
    uint64_t now = ts;

    if (nLastTime == 0)
    {
        nLastTime = now;
        return;
    }
    pTicks[nTickCount] = now - nLastTime;
    nLastTime = now;
    nTickCount++;

    if (nTickCount < nTicksMax)
        return;

    AddEntry();
    nTickCount = 0;
}

void JitterTool::AddPoint()
{
    uint64_t now = 0;

    now = ALooper::GetNowUs();
    if (nLastTime == 0)
    {
        nLastTime = now;
        return;
    }
    pTicks[nTickCount] = now - nLastTime;
    nLastTime = now;
    nTickCount++;

    if (nTickCount < nTicksMax)
        return;

    AddEntry();
    nTickCount = 0;
}

void JitterTool::AddEntry()
{
    double localAvg = 0;
    double localStdDev = 0;
    uint32_t i;

    for (i = 0; i < nTicksMax; i++)
        localAvg += pTicks[i];
    localAvg /= nTicksMax;

    for (i = 0; i < nTicksMax; i++)
        localStdDev += (localAvg - pTicks[i]) * (localAvg - pTicks[i]);
    localStdDev = sqrt(localStdDev / nTicksMax);

    if (bShow)
        LOGV("Mean time between frames: %.2f  standard deviation: %.2f\n",
                        localAvg, localStdDev);

    fAvg.push(localAvg);
    fStdDev.push(localStdDev);

    if (nHightestJitter < localStdDev) {
        nHightestJitter = localStdDev;
    }
}

void JitterTool::SetShow(bool bShowTrace)
{
    bShow = bShowTrace;
}

void JitterTool::GetAvgs(double *pStdDev, double *pAvg,
                          double *pHighest)
{
    assert(pStdDev);
    assert(pAvg);
    assert(pHighest);

    uint32_t count = 0;
    *pStdDev = 0;
    *pAvg = 0;
    *pHighest = 0;

    *pHighest = nHightestJitter;

    count = fAvg.getItemSize() - 1;
    for (uint32_t i = 0; i < count; i++) {
        *pAvg += fAvg.top();
        fAvg.pop();
    }

    for (uint32_t i = 0; i < count; i++) {
        *pStdDev += fStdDev.top();
        fStdDev.pop();
    }

    *pAvg = *pAvg / count;
    *pStdDev = *pStdDev /count;
    if (!fAvg.isEmpty()) {
        fAvg.clear();
    }

    if (!fStdDev.isEmpty()) {
        fStdDev.clear();
    }

    LOGV("%s[%d] stddev %.2f avg %.2f highest %.2f \n",
        __FUNCTION__, __LINE__, *pStdDev, *pAvg, *pHighest);

}

}  // namespace android
