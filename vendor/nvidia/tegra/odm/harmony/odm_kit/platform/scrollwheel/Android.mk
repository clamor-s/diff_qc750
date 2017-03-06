LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_scrollwheel
LOCAL_NVIDIA_NO_COVERAGE := true
LOCAL_SRC_FILES += nvodm_scrollwheel.c

include $(NVIDIA_STATIC_LIBRARY)

