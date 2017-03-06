/*
 * Copyright (c) 2009-2012, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#define LOG_TAG "jni_HwCursor"

#include "JNIHelp.h"
#include <jni.h>
#include "utils/Log.h"
#include "utils/misc.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "nvdispmgr.h"


static NvDispMgrCursorShapeSpec g_Cursor16x16 = {
    {0,},
    {   16,16,8,8,10,10,
        {   
            0x00, 0x00,
            0x40, 0x02,
            0x20, 0x04,
            0x10, 0x08,
            0x08, 0x10,
            0x04, 0x20,
            0x02, 0x40,
            0x01, 0x80,

            0x01, 0x80,
            0x02, 0x40,
            0x04, 0x20,
            0x08, 0x10,
            0x10, 0x08,
            0x20, 0x04,
            0x40, 0x02,
            0x00, 0x00,

            0xbf, 0xfd,
            0x1f, 0xf8,
            0x8f, 0xf1,
            0xc7, 0xe3,
            0xe3, 0xc7,
            0xf1, 0x8f,
            0xf8, 0x1f,
            0xfc, 0x3f,

            0xfc, 0x3f,
            0xf8, 0x1f,
            0xf1, 0x8f,
            0xe3, 0xc7,
            0xc7, 0xe3,
            0x8f, 0xf1,
            0x1f, 0xf8,
            0xbf, 0xfd, 
        },
    }
};

NvDispMgrClientHandle g_DispMgrClient = NULL;
static NvU32 gSeqNum = 0;

static void com_nvidia_graphics_HwShowCursor(JNIEnv* env, jobject obj,
    jboolean Show) {

    NvError e;

    e = NvDispMgrDisplaySetAttributes(g_DispMgrClient,
            NV_DISP_MGR_DISPLAY_MASK_ALL, NvDispMgrAttrFlags_None,
            NULL, NvDispMgrDisplayAttr_CursorVisible,
            (Show) ? NV_TRUE : NV_FALSE, 0);

    if (e != NvSuccess) {
        ALOGE("Error occurred changing HW cursor visibility");
    }
}

static void com_nvidia_graphics_HwMoveCursor(JNIEnv* env,
    jobject obj, jint X, jint Y, jint W, jint H) {

    NvError e;
    static NvDispMgrDisplayMask DisplayMask = 0;
    NvDispMgrDisplayMask TempDisplayMask = 0;
    NvU32 IDToUse=0, MouseX = 0, MouseY = 0;
    NvU32 PrimaryDesktopWidth = W == 0 ? 1024 : W;
    NvU32 PrimaryDesktopHeight = W == 0 ? 600 : H;
    static NvU32 OldSeqNum=0;
    
    {
     static NvDispMgrClientAttr attrs[] = {
                NvDispMgrClientAttr_StateSequenceNumber,
        };
        NvDispMgrGetAttrsIn InAttr;
        NvDispMgrGetAttrsOut OutAttr;

        InAttr.NAttributes = 1;
        memcpy(InAttr.Attributes, attrs, sizeof(NvDispMgrClientAttr));

        e = NvDispMgrClientGetAttrs(
                g_DispMgrClient,
                NvDispMgrAttrFlags_None,
                &InAttr,
                &OutAttr,
                &gSeqNum);
    }

    if(gSeqNum != OldSeqNum)
    {
        static NvDispMgrDisplayAttr attrs[] = {
            NvDispMgrDisplayAttr_IsConnected,
        };
        static NvU32 vals[] = {
            NV_TRUE,
        };
        NvDispMgrFindDisplays(
                g_DispMgrClient,
                NvDispMgrFindDisplaysFlags_None,
                NV_DISP_MGR_DISPLAY_MASK_ALL,
                0, NULL,  // no GUIDs
                NV_ARRAY_SIZE(attrs),
                attrs,
                vals,
                &DisplayMask,
                NULL);

        e = NvDispMgrDisplaySetCursorShape(g_DispMgrClient,
            NV_DISP_MGR_DISPLAY_MASK_ALL, NvDispMgrAttrFlags_None,
            &g_Cursor16x16, NULL);
        if (e != NvSuccess) {
            ALOGE("Error occurred changing HW cursor shape");
        }

        e = NvDispMgrDisplaySetAttributes(g_DispMgrClient,
            NV_DISP_MGR_DISPLAY_MASK_ALL, NvDispMgrAttrFlags_None,
            &gSeqNum, NvDispMgrDisplayAttr_CursorVisible,
            NV_TRUE, 0);

        if (e != NvSuccess) {
            ALOGE("Error occurred changing HW cursor visibility");
        }
        OldSeqNum = gSeqNum;
    }
    TempDisplayMask = DisplayMask;
    while (TempDisplayMask > 0)
    {
        static NvDispMgrDisplayAttr attrs[] = {
                NvDispMgrDisplayAttr_ModeWidth,
                NvDispMgrDisplayAttr_ModeHeight,
        };
        NvDispMgrGetAttrsIn InAttr;
        NvDispMgrGetAttrsOut OutAttr;

        InAttr.NAttributes = 2;
        memcpy(InAttr.Attributes, attrs, 2*sizeof(NvDispMgrDisplayAttr));

        while (!(TempDisplayMask & 0x1))
        {
            IDToUse++;
            TempDisplayMask = TempDisplayMask >> 1;
        }
        TempDisplayMask = TempDisplayMask >> 1;
        
        e = NvDispMgrDisplayGetAttrs(
                g_DispMgrClient,
                IDToUse,
                NvDispMgrAttrFlags_None,
                &InAttr,
                &OutAttr,
                NULL);

        if (e != NvSuccess) {
            ALOGE("Error occurred setting HW cursor position");
        }

        MouseX = ceil(((float)(OutAttr.Values[0]*X)/(float)(PrimaryDesktopWidth)));
        MouseY = ceil(((float)(OutAttr.Values[1]*Y)/(float)(PrimaryDesktopHeight)));

        e = NvDispMgrDisplaySetAttributes(g_DispMgrClient,
            0x1<<IDToUse, NvDispMgrAttrFlags_None,
            NULL, NvDispMgrDisplayAttr_CursorX, MouseX,
            NvDispMgrDisplayAttr_CursorY, MouseY, 0);

        if (e != NvSuccess) {
            ALOGE("Error occurred setting HW cursor position");
        }
        IDToUse++;
    }
}


static JNINativeMethod sMethods[] = {
     /* name, signature, funcPtr */
    { "HwShowCursor", "(Z)V",  (void*)com_nvidia_graphics_HwShowCursor
    },
    { "HwMoveCursor", "(IIII)V", (void*)com_nvidia_graphics_HwMoveCursor
    },
};

int register_com_nvidia_graphics_HwCursor(JNIEnv* env)
{
    static const char* const kClassName = "com/nvidia/graphics/Cursor";
    jclass clazz = env->FindClass(kClassName);

    if (clazz == NULL) {
        ALOGE("Can't find %s", kClassName);
        return -1;
    }

    if (env->RegisterNatives(clazz, sMethods,
            sizeof(sMethods)/sizeof(sMethods[0])) != JNI_OK) {
        ALOGE("Failed registering methods for %s\n", kClassName);
        return -1;
    }

    return 0;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;
    NvError e;
    NvDispMgrSetAttrsIn  setIn;

    e = NvDispMgrClientInitialize(&g_DispMgrClient);

    if (e != NvSuccess) {
        ALOGE("failed to initialize NvDispMgr client");
        return -1;
    }
    
    
    setIn.AttrVal[0] = NvDispMgrClientAttr_EventMask;
    setIn.AttrVal[1] = NvDispMgrEventMask_All;
    setIn.NAttributes = 1;

    e = NvDispMgrClientSetAttrs( g_DispMgrClient, NvDispMgrAttrFlags_Immediate,
                &setIn, &gSeqNum);
                
    if (e != NvSuccess) {
        ALOGE("failed to set the cursor dispmgr events");
        return -1;
    }

    e = NvDispMgrDisplaySetCursorShape(g_DispMgrClient,
            NV_DISP_MGR_DISPLAY_MASK_ALL, NvDispMgrAttrFlags_None,
            &g_Cursor16x16, NULL);

    if (e != NvSuccess) {
        ALOGE("failed to set the cursor shape");
        return -1;
    }

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return result;
    }
    LOG_ASSERT(env, "Could not retrieve the env!");

    register_com_nvidia_graphics_HwCursor(env);

    return JNI_VERSION_1_4;
}
