/* Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved. */
/*
** Copyright 2010, Icera Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** You may not use this file except in compliance with the License.
** You may obtain a copy of the License at:
**
** http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** imitations under the License.
*/
/*
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#ifndef ICERA_RIL_STK_H
#define ICERA_RIL_STK_H 1

void requestStkGetProfile(void *data, size_t datalen, RIL_Token t);
void requestSTKSetProfile(void * data, size_t datalen, RIL_Token t);
void requestSTKSendEnvelopeCommand(void * data, size_t datalen, RIL_Token t);
void requestSTKSendTerminalResponse(void * data, size_t datalen, RIL_Token t);
void requestStkHandleCallSetupRequestedFromSim(void * data, size_t datalen, RIL_Token t);
void requestSTKReportSTKServiceIsRunning(void * data, size_t datalen, RIL_Token t);
void requestSTKPROReceived(void *data, size_t datalen, RIL_Token t);
void unsolSimStatusChanged(const char *s);
void unsolStkEventNotify(const char *s);
void unsolicitedSTKPRO(const char *s);
int StkNotifyServiceIsRunning(int from_request);
void StkInitDefaultProfiles(void);

int PbkIsLoaded(void);
extern int stk_use_cusat_api;

/* STK profiles*/
#define RIL_STK_TE_PROFILE           "080017210101001CE900000000000000000000000000000000000000000000"
#define RIL_STK_MT_PROFILE_ADD "0000001E000000000000000000000000000000000000000000000000000000"
#define RIL_STK_CONF_PROFILE      "00000010000000000000000000000000000000000000000000000000000000"

#endif
