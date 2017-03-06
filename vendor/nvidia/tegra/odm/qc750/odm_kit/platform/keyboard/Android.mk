LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_keyboard
LOCAL_NVIDIA_NO_COVERAGE := true
LOCAL_SRC_FILES += nvodm_keyboard.c

#TODO: Remove following lines before include statement and fix the source giving warnings/errors
LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
include $(NVIDIA_STATIC_LIBRARY)

