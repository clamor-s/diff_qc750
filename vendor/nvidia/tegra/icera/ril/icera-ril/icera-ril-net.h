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


#ifndef ICERA_RIL_NET_H
#define ICERA_RIL_NET_H 1

typedef enum {
    ANY = 0,
    EURO,
    US,
    JPN,
    AUS,
    AUS2,
    CELLULAR,
    PCS,
    BAND_CLASS_3,
    BAND_CLASS_4,
    BAND_CLASS_5,
    BAND_CLASS_6,
    BAND_CLASS_7,
    BAND_CLASS_8,
    BAND_CLASS_9,
    BAND_CLASS_10,
    BAND_CLASS_11,
    BAND_CLASS_15,
    BAND_CLASS_16,
    MAX_NUMBER_OF_BANDS
} AT_BandModes;


// Icera specific -- NITZ time
void unsolNetworkTimeReceived(const char *data);
void unsolSignalStrength(const char *s);
void unsolRestrictedStateChanged(void);
void requestSignalStrength(void *data, size_t datalen,
                                      RIL_Token t);
void requestSetBandMode(void *data, size_t datalen,
                                      RIL_Token t);
void requestQueryNetworkSelectionMode(void *data, size_t datalen,
                                      RIL_Token t);
void requestSetNetworkSelectionAutomatic(void *data, size_t datalen,
                                      RIL_Token t);
void requestSetNetworkSelectionManual(void *data, size_t datalen,
                                      RIL_Token t);
void requestQueryAvailableNetworks(void *data, size_t datalen,
                                   RIL_Token t);
void requestSetPreferredNetworkType(void *data, size_t datalen,
                                    RIL_Token t);
void requestGetPreferredNetworkType(void *data, size_t datalen,
                                    RIL_Token t);
void requestEnterNetworkDepersonalization(void *data, size_t datalen,
                                          RIL_Token t);
void requestRegistrationState(int request, void *data,
                              size_t datalen, RIL_Token t);
void requestNeighbouringCellIDs(void *data, size_t datalen, RIL_Token t);
void requestQueryAvailableBandMode(void *data, size_t datalen, RIL_Token t);
void requestSetLocationUpdates(void *data, size_t datalen, RIL_Token t);
void requestGetNeighboringCellIds(void *data, size_t datalen, RIL_Token t);
void requestOperator(void *data, size_t datalen, RIL_Token t);
void InitializeNetworkUnsols(int defaultLocUp);

void GetCurrentNetworkMode(int* mode);
int GetCurrentTechno(void);

void parseCreg(const char *s);
void parseCgreg(const char *s);
void parseCereg(const char *s);
void parseNwstate(const char *s);
void parseIcti(const char *s);
void InitNetworkCacheState(void);

int IsRegistered(void);

int GetLocupdateState(void);
int GetLteSupported(void);

#endif
