##
##
## Copyright 2009, The Android Open Source Project
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions
## are met:
##  * Redistributions of source code must retain the above copyright
##    notice, this list of conditions and the following disclaimer.
##  * Redistributions in binary form must reproduce the above copyright
##    notice, this list of conditions and the following disclaimer in the
##    documentation and/or other materials provided with the distribution.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
## EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
## PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
## CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
## EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
## PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
## PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
## OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    ../../TestNetscapePlugIn/main.cpp \
    ../../TestNetscapePlugIn/PluginObject.cpp \
    ../../TestNetscapePlugIn/PluginTest.cpp \
    ../../TestNetscapePlugIn/TestObject.cpp \
    ../../TestNetscapePlugIn/android/AndroidHelpers.cpp \
    ../../TestNetscapePlugIn/Tests/DocumentOpenInDestroyStream.cpp \
    ../../TestNetscapePlugIn/Tests/EvaluateJSAfterRemovingPluginElement.cpp \
    ../../TestNetscapePlugIn/Tests/GetURLWithJavaScriptURLDestroyingPlugin.cpp \
    ../../TestNetscapePlugIn/Tests/GetUserAgentWithNullNPPFromNPPNew.cpp \
    ../../TestNetscapePlugIn/Tests/NPDeallocateCalledBeforeNPShutdown.cpp \
    ../../TestNetscapePlugIn/Tests/NPPSetWindowCalledDuringDestruction.cpp \
    ../../TestNetscapePlugIn/Tests/NPRuntimeObjectFromDestroyedPlugin.cpp \
    ../../TestNetscapePlugIn/Tests/NPRuntimeRemoveProperty.cpp \
    ../../TestNetscapePlugIn/Tests/NullNPPGetValuePointer.cpp \
    ../../TestNetscapePlugIn/Tests/PassDifferentNPPStruct.cpp \
    ../../TestNetscapePlugIn/Tests/PluginScriptableNPObjectInvokeDefault.cpp \
    ../../TestNetscapePlugIn/Tests/android/SurfaceTextureHang.cpp \
    ../../TestNetscapePlugIn/Tests/android/InvalidWindowDimensions.cpp

WEBCORE_PATH := external/webkit/Source/WebCore

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(WEBCORE_PATH) \
    $(WEBCORE_PATH)/bridge \
    $(WEBCORE_PATH)/plugins \
    $(WEBCORE_PATH)/platform/android/JavaVM \
    external/webkit/Source/WebKit/android/plugins \
    $(LOCAL_PATH)/../../TestNetscapePlugIn \
    $(LOCAL_PATH)/../TestNetscapePlugIn/ForwardingHeaders

LOCAL_CFLAGS += -DXP_ANDROID
LOCAL_CFLAGS += -fvisibility=hidden
LOCAL_CFLAGS += -DDEBUG=1 -UNDEBUG -DSK_RELEASE

LOCAL_MODULE := libtestnetscapeplugin
LOCAL_MODULE_TAGS := tests
LOCAL_SHARED_LIBRARIES += libstlport
LOCAL_SHARED_LIBRARIES += libcutils libandroid libEGL libGLESv2

include external/stlport/libstlport.mk

include $(BUILD_SHARED_LIBRARY)

