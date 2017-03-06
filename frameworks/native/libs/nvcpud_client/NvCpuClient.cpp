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

#include <nvcpud/NvCpuClient.h>

#include <binder/IServiceManager.h>

#undef LOG_TAG
#define LOG_TAG "NvCpuClient"

namespace android {

#define RATE_LIMIT_INIT(func) \
    last##func(0),

NvCpuClient::NvCpuClient() :
    failedConnectionAttempts(0),
    NVCPU_FUNC_LIST(RATE_LIMIT_INIT)
    lastConnectionAttempt(0) {}

#undef RATE_LIMIT_INIT

const nsecs_t NvCpuClient::MINIMUM_POKE_WAIT_NS = 40 * 1000000LL;
const nsecs_t NvCpuClient::MINIMUM_CONNECTION_DELAY_NS = 5000 * 1000000LL;
const int NvCpuClient::MAXIMUM_CONNECTION_ATTEMPTS = 100;

status_t NvCpuClient::tryBindToService() {
    sp<IServiceManager> sm = defaultServiceManager();
    if (sm == 0)
        return FAILED_TRANSACTION;
    sp<IBinder> binder = sm->checkService(String16("NvCpuService"));
    if (binder == 0)
        return NAME_NOT_FOUND;
    mInterface = INvCpuService::asInterface(binder);
    if (mInterface == 0)
        return BAD_VALUE;
    LOGI("Successfully bound to service");
    return NO_ERROR;
}

status_t NvCpuClient::bindToService() {
    sp<IServiceManager> sm = defaultServiceManager();
    if (sm == 0)
        return FAILED_TRANSACTION;
    sp<IBinder> binder = sm->getService(String16("NvCpuService"));
    if (binder == 0)
        return NAME_NOT_FOUND;
    mInterface = INvCpuService::asInterface(binder);
    if (mInterface == 0)
        return BAD_VALUE;
    LOGI("Successfully bound to service");
    return NO_ERROR;
}

void NvCpuClient::checkAndTryReconnect(nsecs_t now) {
    if (mInterface != 0)
        return;
    if (failedConnectionAttempts > MAXIMUM_CONNECTION_ATTEMPTS)
        return;
    if (now < lastConnectionAttempt + MINIMUM_CONNECTION_DELAY_NS)
        return;
    lastConnectionAttempt = now;
    status_t res = tryBindToService();
    if (res != NO_ERROR) {
        LOGW("Failed to bind to service");
        failedConnectionAttempts++;
    }
}

bool NvCpuClient::rateLimitAndCheckConnection(nsecs_t* last, nsecs_t now) {
    if (now - *last < MINIMUM_POKE_WAIT_NS)
        return false;
    *last = now;
    checkAndTryReconnect(now);
    if (mInterface == 0)
        return false;
    return true;
}

#define TIMEOUT_DEF(func) \
void NvCpuClient::func(int arg, nsecs_t timeoutNs, nsecs_t now) { \
    if (!rateLimitAndCheckConnection(&last##func, now)) \
        return; \
    status_t res = mInterface->func(arg, timeoutNs); \
    if (res != NO_ERROR) \
        breakConnection(); \
}

#define HANDLE_DEF(func) \
int NvCpuClient::func##Handle(int arg, nsecs_t now) { \
    checkAndTryReconnect(now); \
    if (mInterface == 0) \
        return -1; \
    return mInterface->func##Handle(arg); \
}

NVCPU_FUNC_LIST(TIMEOUT_DEF)
NVCPU_FUNC_LIST(HANDLE_DEF)

#define SET_DEF(func) \
void NvCpuClient::set##func(int arg) { \
    checkAndTryReconnect(systemTime(SYSTEM_TIME_MONOTONIC)); \
    if (mInterface == 0) { \
        return; \
    } \
    mInterface->set##func(arg); \
}

#define GET_DEF(func) \
int NvCpuClient::get##func(void) { \
    checkAndTryReconnect(systemTime(SYSTEM_TIME_MONOTONIC)); \
    if (mInterface == 0) { \
        return -1; \
    } \
  return mInterface->get##func(); \
}

NVCPU_AP_FUNC_LIST(SET_DEF)
NVCPU_AP_FUNC_LIST(GET_DEF)

#undef TIMEOUT_DEF
#undef HANDLE_DEF
#undef SET_DEF
#undef GET_DEF

void NvCpuClient::breakConnection() {
    LOGW("Lost connection to service");
    mInterface = NULL;
}

};
