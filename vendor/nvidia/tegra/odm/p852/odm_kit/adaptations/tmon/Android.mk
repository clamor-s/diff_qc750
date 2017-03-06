LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_tmon

LOCAL_SRC_FILES += tmon_hal.c
LOCAL_SRC_FILES += adt7461/nvodm_tmon_adt7461.c

include $(NVIDIA_STATIC_LIBRARY)