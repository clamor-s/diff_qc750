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

#ifndef ANDROID_NV_CPU_CLIENT_H
#define ANDROID_NV_CPU_CLIENT_H

#include <utils/threads.h>
#include <utils/Errors.h>

#include <nvcpud/INvCpuService.h>

namespace android {

#define TIMEOUT_DECL(func) \
    void func(int arg, nsecs_t timeoutNs, nsecs_t now);

#define HANDLE_DECL(func) \
    int func##Handle(int arg, nsecs_t now);

#define SET_DECL(func) \
    void set##func(int arg);

#define GET_DECL(func) \
    int get##func(void);

#define RATE_LIMIT_DECL(func) \
    nsecs_t last##func;

class NvCpuClient {
public:
    NvCpuClient();

    /**
     * Timeout methods.
     *
     * Each of these set a floor/ceiling on the specified resource for timeoutNs.
     * The now argument should be the current time, and is used for API rate limiting.
     * You may call these functions multiple times with overlapping timeouts.
     *
     * requestCpuFreqMin sets a floor on CPU freq
     * requestCpuFreqMax sets a ceiling on CPU freq
     * requestMinOnlineCpu sets a minimum # of CPUS
     * requestMaxOnlineCpu sets a maximum # of CPUS
     *
     */
    NVCPU_FUNC_LIST(TIMEOUT_DECL)

    /**
     * Handle versions of the above calls.
     *
     * These return a file descriptor.  Closing the file descriptor will release the requested
     * perf setting
     * now is the current time, used for API rate-limiting
     */
    NVCPU_FUNC_LIST(HANDLE_DECL)

    /**
     * Set and Get functions used by the App Profile system
     */
    NVCPU_AP_FUNC_LIST(GET_DECL)
    NVCPU_AP_FUNC_LIST(SET_DECL)

private:

    int failedConnectionAttempts;
    void checkAndTryReconnect(nsecs_t now);
    status_t tryBindToService();
    status_t bindToService();
    void breakConnection();
    bool rateLimitAndCheckConnection(nsecs_t* last, nsecs_t now);

    NVCPU_FUNC_LIST(RATE_LIMIT_DECL);

    //IPC ain't cheap.  Rate limit these calls
    nsecs_t lastConnectionAttempt;

    static const nsecs_t MINIMUM_POKE_WAIT_NS;
    static const nsecs_t MINIMUM_CONNECTION_DELAY_NS;
    static const int MAXIMUM_CONNECTION_ATTEMPTS;
    sp<INvCpuService> mInterface;
};

#undef RATE_LIMIT_DECL
#undef TIMEOUT_DECL
#undef HANDLE_DECL
#undef GET_DECL
#undef SET_DECL
};

#endif //ANDROID_NV_CPU_CLIENT_H
