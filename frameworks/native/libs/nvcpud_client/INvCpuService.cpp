/*
 * Copyright (C) 2011-2012 The Android Open Source Project
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
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

#define LOG_TAG "NvCpuService"

#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include <nvcpud/INvCpuService.h>

namespace android {

#define TIMEOUT_DEF(func) \
    virtual status_t func(int arg, nsecs_t timeoutNs) \
    { \
        Parcel data, reply; \
        data.writeInterfaceToken(INvCpuService::getInterfaceDescriptor()); \
        data.writeInt32(arg); \
        data.writeInt64(timeoutNs); \
        return remote()->transact(BnNvCpuService::func##TimeoutEnum, data, &reply, IBinder::FLAG_ONEWAY); \
    }

#define HANDLE_DEF(func) \
    virtual int func##Handle(int arg) \
    { \
        Parcel data, reply; \
        data.writeInterfaceToken(INvCpuService::getInterfaceDescriptor()); \
        data.writeInt32(arg); \
        status_t res = remote()->transact(BnNvCpuService::func##HandleEnum, data, &reply); \
        if (res != NO_ERROR) { \
            return -1; \
        } \
        res = reply.readInt32(); \
        if (res != NO_ERROR) { \
            return -1; \
        } \
    \
        return dup(reply.readFileDescriptor()); \
    }

#define SET_DEF(func) \
    virtual void set##func(int arg) \
    { \
        Parcel data, reply; \
        data.writeInterfaceToken(INvCpuService::getInterfaceDescriptor()); \
        data.writeInt32(arg); \
        remote()->transact(BnNvCpuService::set##func##Enum, data, &reply, IBinder::FLAG_ONEWAY); \
    }

#define GET_DEF(func) \
    virtual int get##func(void) \
    { \
        Parcel data, reply; \
        data.writeInterfaceToken(INvCpuService::getInterfaceDescriptor()); \
        status_t res = remote()->transact(BnNvCpuService::get##func##Enum, data, &reply); \
        if (res != NO_ERROR) { \
            return -1; \
        } \
        res = reply.readInt32(); \
        if (res != NO_ERROR) { \
            return -1; \
        } \
    \
        return reply.readInt32(); \
    }


class BpNvCpuService : public BpInterface<INvCpuService> {
public:
    BpNvCpuService(const sp<IBinder>& impl)
        : BpInterface<INvCpuService>(impl) { }

    NVCPU_FUNC_LIST(TIMEOUT_DEF)
    NVCPU_FUNC_LIST(HANDLE_DEF)
    NVCPU_AP_FUNC_LIST(GET_DEF)
    NVCPU_AP_FUNC_LIST(SET_DEF)

};

#undef TIMEOUT_DEF
#undef HANDLE_DEF
#undef GET_DEF
#undef SET_DEF

IMPLEMENT_META_INTERFACE(NvCpuService, "android.INvCpuService") ;

#define TIMEOUT_CASE(func) \
    case func##TimeoutEnum: { \
        CHECK_INTERFACE(INvCpuService, data, reply); \
        status_t res = func(data.readInt32(), (nsecs_t)data.readInt64()); \
        reply->writeInt32(res); \
    } break;
#define HANDLE_CASE(func) \
    case func##HandleEnum: { \
        CHECK_INTERFACE(INvCpuService, data, reply); \
        int fd = func##Handle( \
                data.readInt32()); \
        if (fd < 0) { \
            reply->writeInt32(UNKNOWN_ERROR); \
        } else { \
            reply->writeInt32(NO_ERROR); \
            reply->writeFileDescriptor(fd, true); \
        } \
    } break;

#define SET_CASE(func) \
    case set##func##Enum: { \
        CHECK_INTERFACE(INvCpuService, data, reply); \
        set##func(data.readInt32()); \
    } break;
#define GET_CASE(func) \
    case get##func##Enum: { \
        CHECK_INTERFACE(INvCpuService, data, reply); \
        int res = get##func(); \
        reply->writeInt32(NO_ERROR); \
        reply->writeInt32(res); \
    } break;


status_t BnNvCpuService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        NVCPU_FUNC_LIST(TIMEOUT_CASE)
        NVCPU_FUNC_LIST(HANDLE_CASE)
        NVCPU_AP_FUNC_LIST(GET_CASE)
        NVCPU_AP_FUNC_LIST(SET_CASE)
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
    return NO_ERROR;
}

#undef TIMEOUT_CASE
#undef HANDLE_CASE
#undef SET_CASE
#undef GET_CASE

}; //namespace android
