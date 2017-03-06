LOCAL_PATH := $(call my-dir)

#Static lib for blockdevice usage.

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvddk_aes

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/ap20
LOCAL_SRC_FILES += nvddk_aes_blockdev.c
LOCAL_SRC_FILES += nvddk_aes_core.c
LOCAL_SRC_FILES += ap20/nvddk_aes_hw_ap20.c
LOCAL_SRC_FILES += ap20/ap20_aes_hw_utils.c

#TODO: Remove following lines before include statement and fix the source giving warnings/errors
LOCAL_NVIDIA_RM_WARNING_FLAGS := -Wundef
LOCAL_CFLAGS += -Wno-error=cast-align
include $(NVIDIA_STATIC_LIBRARY)
