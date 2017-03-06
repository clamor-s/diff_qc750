/*
* Copyright (c) 2012 NVIDIA Corporation.  All Rights Reserved.
*
* NVIDIA Corporation and its licensors retain all intellectual property and
* proprietary rights in and to this software and related documentation.  Any
* use, reproduction, disclosure or distribution of this software and related
* documentation without an express license agreement from NVIDIA Corporation
* is strictly prohibited.
*/


#ifndef NVCAP_VIDEO_H
#define NVCAP_VIDEO_H

struct nvcap_video;

struct nvcap_layer {
    NvNativeHandle *buffer;
    NvRmFence fence;
    int protect;
    int transform;
    NvRect src;
};

struct nvcap_mode {
    int xres;
    int yres;
    float refresh;
};

struct nvcap_callbacks {
    void *data;
    void (*enable)(void *data, int enable);
    void (*set_mode)(void *data, struct nvcap_mode *mode);
};

struct nvcap_interface {
    struct nvcap_video* (*open)(struct nvcap_callbacks*);
    void (*close)(struct nvcap_video*);
    int (*post)(struct nvcap_video*, struct nvcap_layer*);
};

#define NVCAP_VIDEO_INTERFACE_SYM     NVCAP_VIDEO_INTERFACE
#define NVCAP_VIDEO_INTERFACE_STRING "NVCAP_VIDEO_INTERFACE"
#define NVCAP_VIDEO_LIBRARY_STRING   "/system/lib/libnvcap_video.so"

#endif /* NVCAP_VIDEO_H */

