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

#ifndef ICERA_RIL_DATA_H
#define ICERA_RIL_DATA_H 1

void requestSetupDataCall(void *data, size_t datalen, RIL_Token t);
void requestDeactivateDataCall(void *data, size_t datalen, RIL_Token t);
void requestDataCallList(void *data, size_t datalen, RIL_Token t);
void onDataCallListChanged(void *param);
void requestLastDataFailCause(void *data, size_t datalen, RIL_Token t);
void setDataCallUsePPP(int use_ppp);
void ParseIpdpact(const char* s);
void ResetPppdState(void);
extern int s_use_ppp;
void ProcessDataCallEvent(int nwIf, int State);
int GetNmbConcurentProfile(void);
void AddNetworkInterface(char * NwIF);
void ConfigureExplicitRilRouting(int Set);
char* ConvertIPV6AddressToStandard(char * src);
#endif

