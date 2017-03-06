LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvreftrack

LOCAL_SRC_FILES += nvreftrack.c

include $(NVIDIA_STATIC_LIBRARY)

