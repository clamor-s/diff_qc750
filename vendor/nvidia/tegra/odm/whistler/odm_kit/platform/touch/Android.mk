LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_touch

LOCAL_C_INCLUDES += $(TEGRA_TOP)/customers/nvidia-partner/whistler/odm_kit/platform/touch
LOCAL_SRC_FILES += nvodm_touch.c
LOCAL_SRC_FILES += nvodm_touch_tpk.c

#TODO: Remove following lines before include statement and fix the source giving warnings/errors
LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
include $(NVIDIA_STATIC_LIBRARY)

