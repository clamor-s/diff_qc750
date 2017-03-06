/*
 * Copyright (C) 2011-2012 The Android Open Source Project
 * Copyright (C) 2012, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef ANDROID_I_NV_CPU_SERVICE_H
#define ANDROID_I_NV_CPU_SERVICE_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/RefBase.h>
#include <utils/Errors.h>

#include <binder/IInterface.h>

namespace android {

enum NvCpuBoostStrength {
    NVCPU_BOOST_STRENGTH_LOWEST = 0,
    NVCPU_BOOST_STRENGTH_LOW,
    NVCPU_BOOST_STRENGTH_MEDIUM_LOW,
    NVCPU_BOOST_STRENGTH_MEDIUM,
    NVCPU_BOOST_STRENGTH_MEDIUM_HIGH,
    NVCPU_BOOST_STRENGTH_HIGH,
    NVCPU_BOOST_STRENGTH_HIGHEST,
    NVCPU_BOOST_STRENGTH_COUNT
};

class NvCpuService;

#define NVCPU_FUNC_LIST(F) \
    F(requestCpuFreqMin) \
    F(requestCpuFreqMax) \
    F(requestMaxCbusFreq) \
    F(requestMinCbusFreq) \
    F(requestMinOnlineCpuCount) \
    F(requestMaxOnlineCpuCount)

#define NVCPU_AP_FUNC_LIST(F) \
    F(CpuCoreBias) \
    F(CpuHighFreqMinDelay) \
    F(CpuMaxNormalFreq) \
    F(CpuScalingMinFreq) \
    F(GpuCbusCapLevel) \
    F(GpuCbusCapState) \
    F(GpuScalingEnable) \
    F(EnableAppProfiles)

class INvCpuService : public IInterface
{
public:
DECLARE_META_INTERFACE(NvCpuService) ;

//Will time out on it's own
//In case you crash/forget
#define TIMEOUT_DECL(func) \
    virtual status_t func(int arg, nsecs_t timeoutNs) = 0;
NVCPU_FUNC_LIST(TIMEOUT_DECL)
#undef TIMEOUT_DECL

//Close the handle to stop this
#define HANDLE_DECL(func) \
    virtual int func##Handle(int arg) = 0;
NVCPU_FUNC_LIST(HANDLE_DECL)
#undef HANDLE_DECL

#define SET_DECL(func) \
    virtual void set##func(int arg) = 0;
NVCPU_AP_FUNC_LIST(SET_DECL)
#undef SET_DECL

#define GET_DECL(func) \
    virtual int get##func(void) = 0;
NVCPU_AP_FUNC_LIST(GET_DECL)
#undef GET_DECL

};


class BnNvCpuService : public BnInterface<INvCpuService>
{

#define TIMEOUT_ENUM(func) \
    func##TimeoutEnum,
#define HANDLE_ENUM(func) \
    func##HandleEnum,
#define SET_ENUM(func) \
    set##func##Enum,
#define GET_ENUM(func) \
    get##func##Enum,

    public:
    enum {
        NVCPU_FUNC_LIST(TIMEOUT_ENUM)
        NVCPU_FUNC_LIST(HANDLE_ENUM)
        NVCPU_AP_FUNC_LIST(SET_ENUM)
        NVCPU_AP_FUNC_LIST(GET_ENUM)
    };
#undef TIMEOUT_ENUM
#undef HANDLE_ENUM
#undef SET_ENUM
#undef GET_ENUM

    virtual status_t onTransact(uint32_t code,
            const Parcel& data,
            Parcel* reply,
            uint32_t flags = 0);
};

}; // namespace android

#endif //ANDROID_I_NV_CPU_SERVICE_H
