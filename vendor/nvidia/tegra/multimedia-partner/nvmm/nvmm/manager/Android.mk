LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)
include $(LOCAL_PATH)/../Android.common.mk

LOCAL_MODULE := libnvmm_manager

LOCAL_SRC_FILES += nvmm_manager.c
LOCAL_SRC_FILES += nvmm_manager_tables.c
LOCAL_SRC_FILES += list.c

LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_SHARED_LIBRARIES += libnvrm
LOCAL_SHARED_LIBRARIES += libnvavp

LOCAL_CFLAGS += -Wno-missing-field-initializers
include $(NVIDIA_SHARED_LIBRARY)

