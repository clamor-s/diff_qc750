LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvrm_limits
LOCAL_NVIDIA_NO_COVERAGE := true

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../nvrmkernel/core
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../nvrmkernel/core/common
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../nvrmkernel/core/ap15

LOCAL_SRC_FILES += ap20rm_clocks_limits.c
LOCAL_SRC_FILES += t30rm_clocks_limits.c
LOCAL_SRC_FILES += nvrm_clocks_limits_init.c

LOCAL_CFLAGS += -Wno-missing-field-initializers
include $(NVIDIA_STATIC_LIBRARY)
