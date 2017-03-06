LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := tegrastats

LOCAL_SRC_FILES += main.c
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libnvrm_graphics

# Due to bug in bionic lib, https://review.source.android.com/#change,21736
LOCAL_NVIDIA_RM_WARNING_FLAGS := -Wundef
include $(NVIDIA_EXECUTABLE)
