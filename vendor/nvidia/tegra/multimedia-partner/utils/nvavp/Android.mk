LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvavp

LOCAL_SRC_FILES += nvavp.c

LOCAL_SHARED_LIBRARIES := \
	libnvos \
	libnvrm \

include $(NVIDIA_SHARED_LIBRARY)
