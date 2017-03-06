LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_touch
LOCAL_SRC_FILES += nvodm_touch.c

include $(NVIDIA_STATIC_LIBRARY)

