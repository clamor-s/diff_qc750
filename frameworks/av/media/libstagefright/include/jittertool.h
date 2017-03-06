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

#ifndef JITTER_TOOL_H_
#define JITTER_TOOL_H_

#include <sys/types.h>
#include <stdint.h>
#include <utils/RefBase.h>

namespace android {

class JitterTool
{
public:
    JitterTool(uint32_t nTicks);
    ~JitterTool();
    void AddPoint();
    void AddPoint(uint64_t ts);
    void SetShow(bool bShow);
    void GetAvgs(double *pStdDev, double *pAvg,
                              double *pHighest);
    status_t initCheck() const;
private:
    void  AddEntry();
    double nHightestJitter;

    uint64_t *pTicks;
    uint64_t nLastTime;

    uint32_t nTicksMax;
    uint32_t nTickCount;

    status_t mInitCheck;

    bool bShow;

    Vector<double> fAvg;
    Vector<double> fStdDev;

};
}
#endif

