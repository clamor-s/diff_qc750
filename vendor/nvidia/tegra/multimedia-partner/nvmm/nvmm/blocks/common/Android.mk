LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)
include $(LOCAL_PATH)/../../Android.common.mk

LOCAL_MODULE := libnvmmcommon

LOCAL_SRC_FILES += nvmm_block.c

LOCAL_CFLAGS += -Wno-error=cast-align
include $(NVIDIA_STATIC_LIBRARY)

