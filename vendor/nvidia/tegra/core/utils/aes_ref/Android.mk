LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvaes_ref
LOCAL_SRC_FILES += aes_ref.c

include $(NVIDIA_HOST_STATIC_LIBRARY)