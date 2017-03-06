/*
 * Copyright (c) 2011-2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _NVHWCDEBUG_H
#define _NVHWCDEBUG_H

#include "nvhwc.h"


#ifndef DUMP_LAYERLIST
#define DUMP_LAYERLIST 0
#endif

#ifndef DUMP_CONFIG
#define DUMP_CONFIG 0
#endif

#define DEBUG_FB_CACHE 0

#if DEBUG_FB_CACHE
#define FBCLOG(...) ALOGD("FB Cache: "__VA_ARGS__)
#else
#define FBCLOG(...)
#endif

#define DEBUG_IDLE 0

#if DEBUG_IDLE
#define IDLELOG(...) ALOGD("Idle Detection: "__VA_ARGS__)
#else
#define IDLELOG(...)
#endif

#define DEBUG_MIRROR 0

#if DEBUG_MIRROR
#define MIRRORLOG(...) ALOGD("Mirror: "__VA_ARGS__)
#else
#define MIRRORLOG(...)
#endif

void hwc_dump_display_contents(hwc_display_contents_t *list);
void hwc_dump(hwc_composer_device_t *dev, char *buff, int buff_len);

#endif /* ifndef _NVHWCDEBUG_H */
