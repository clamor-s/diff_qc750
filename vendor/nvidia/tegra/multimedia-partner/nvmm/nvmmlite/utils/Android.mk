LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvmmlite_utils

LOCAL_C_INCLUDES += $(TEGRA_TOP)/multimedia-partner/nvmm/nvmmlite/include

LOCAL_SRC_FILES += nvmm_util.c
LOCAL_SRC_FILES += nvmm_block.c

LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_SHARED_LIBRARIES += libnvrm
LOCAL_SHARED_LIBRARIES += libnvrm_graphics
LOCAL_SHARED_LIBRARIES += libnvmm_utils

LOCAL_CFLAGS += -Wno-error=cast-align

include $(NVIDIA_SHARED_LIBRARY)

