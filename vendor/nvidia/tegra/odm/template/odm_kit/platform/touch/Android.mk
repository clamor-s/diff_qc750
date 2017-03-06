LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_touch

LOCAL_C_INCLUDES += $(TEGRA_TOP)/customers/nvidia-partner/whistler/odm_kit/platform/touch
LOCAL_SRC_FILES += nvodm_touch_stub.c

#TODO: Remove following lines before include statement and fix the source giving warnings/errors
LOCAL_NVIDIA_RM_WARNING_FLAGS := -Wundef
LOCAL_CFLAGS += -Wno-missing-field-initializers
include $(NVIDIA_STATIC_LIBRARY)
