/*
 * Copyright (c) 2012, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "cplconnectclient.h"
#include <binder/IServiceManager.h>
#include <binder/IInterface.h>
#include <binder/IMemory.h>
#include <binder/Parcel.h>
#include <utils/String8.h>
#include <utils/TextOutput.h>

namespace android {


    int NvCplGetAppProfileSettingInt(const char* exeName, const char* setting)
    {
        sp<IServiceManager> sm = defaultServiceManager();
        if (sm == 0)
            return -1;

        sp<IBinder> binder = sm->checkService(String16("nvcpl"));
        if (binder == 0)
            return -1;

        Parcel data;
        Parcel reply;

        status_t err = binder->transact(IBinder::INTERFACE_TRANSACTION, data, &reply);
        if (err == NO_ERROR) {
            data.writeInterfaceToken(reply.readString16());

            data.writeString16(String16(exeName));
            data.writeString16(String16(setting));

            // getAppProfileSettingInt is 2nd function declared in INvCPLRemoteService.aidl
            err = binder->transact(IBinder::FIRST_CALL_TRANSACTION+1, data, &reply);

            if (err == NO_ERROR) {
                int retCode = reply.readExceptionCode();
                int result = reply.readInt32();
                if (!retCode)
                    return result;
            }
        }
        return -1;
    }

    char* NvCplGetAppProfileSettingString(const char* exeName, const char* setting)
    {
        sp<IServiceManager> sm = defaultServiceManager();
        if (sm == 0)
            return NULL;

        sp<IBinder> binder = sm->checkService(String16("nvcpl"));
        if (binder == 0)
            return NULL;

        Parcel data;
        Parcel reply;

        status_t err = binder->transact(IBinder::INTERFACE_TRANSACTION, data, &reply);
        if (err == NO_ERROR) {
            data.writeInterfaceToken(reply.readString16());

            data.writeString16(String16(exeName));
            data.writeString16(String16(setting));

            // getAppProfileSettingInt is first function declared in INvCPLRemoteService.aidl
            err = binder->transact(IBinder::FIRST_CALL_TRANSACTION+0, data, &reply);

            if (err == NO_ERROR) {
                int retCode = reply.readExceptionCode();
                String16 result = reply.readString16();
                return (char *)result.string();
            }
        }
        return NULL;
    }

    int NvCplGetAppProfileSetting3DVStruct(const char* exeName, unsigned char* stereoProfs, int len)
    {
        sp<IServiceManager> sm = defaultServiceManager();
        if (sm == 0)
            return -1;

        sp<IBinder> binder = sm->checkService(String16("nvcpl"));
        if (binder == 0)
            return -1;

        Parcel data;
        Parcel reply;

        status_t err = binder->transact(IBinder::INTERFACE_TRANSACTION, data, &reply);
        if (err == NO_ERROR) {
            data.writeInterfaceToken(reply.readString16());
            data.writeString16(String16(exeName));

            // getAppProfileSetting3DVStruct is 4th function declared in INvCPLRemoteService.aidl
            err = binder->transact(IBinder::FIRST_CALL_TRANSACTION+3, data, &reply);

            if (err == NO_ERROR) {
                // read first 8 bytes of return data; structure follows that
                int retCode = reply.readExceptionCode();
                retCode = reply.readExceptionCode();

                // now read NvStereoProfs structure
                err = reply.read(stereoProfs, len);
            }
        }
        return err;
    }
}


extern "C" {

int NvCplGetAppProfileSettingInt(const char* exeName, const char* setting)
{
    return (android::NvCplGetAppProfileSettingInt(exeName, setting));
}

char* NvCplGetAppProfileSettingString(const char* exeName, const char* setting)
{
    return (android::NvCplGetAppProfileSettingString(exeName, setting));
}

int NvCplGetAppProfileSetting3DVStruct(const char* exeName, unsigned char* stereoProfs, int len)
{
    return (android::NvCplGetAppProfileSetting3DVStruct(exeName, stereoProfs, len));
}

}

