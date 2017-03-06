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

#ifndef ICERA_RIL_UTILS_H
#define ICERA_RIL_UTILS_H 1

int bcdByteToInt(char a);
char charToHex(char a);
char hexToChar(char a);
void asciiToHex(const char *input, char *output, int len);
void hexToAscii(const char *input, char *output, int len);
void UCS2ToUTF8(const char *input, char *output, int len);
void GSMToUTF8(const char *input, char *output, int len);
void numericToBCD(const char *input, char *output, int len);

#endif
