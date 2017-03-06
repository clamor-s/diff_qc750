#
# Makefile for BRCM aGPS GPAL
#
# This shared library is loaded by the aGPS daemon and implements
# wrappers to abstract the BRCM GPS from aGPS
#
#
LOCAL_PATH:= $(call my-dir)


### rules to copy prebuilt brcmgpslcsapi.a

include $(NVIDIA_DEFAULTS)

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := brcmgpslcsapi
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := brcmgpslcsapi.a

include $(NVIDIA_PREBUILT)

### rules to generate shared library

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE_TAGS := optional

# source files
LOCAL_SRC_FILES:= \
    agps_gpal_brcm4751.c \

# link against Android libc
LOCAL_SHARED_LIBRARIES := \
    libcutils

LOCAL_STATIC_LIBRARIES := brcmgpslcsapi

# Extend include path
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/icera/ril/agpsd
LOCAL_REQUIRED_MODULES := brcmgpslcsapi
LOCAL_MODULE:= libagps-brcm4751

include $(NVIDIA_SHARED_LIBRARY)

