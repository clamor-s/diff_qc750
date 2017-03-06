LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvmm_service

LOCAL_C_INCLUDES += $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/include

LOCAL_SRC_FILES += nvmm_service.c

LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_SHARED_LIBRARIES += libnvrm
LOCAL_SHARED_LIBRARIES += libnvrm_graphics
LOCAL_SHARED_LIBRARIES += libnvmm_manager

LOCAL_CFLAGS += -Wno-error=cast-align
include $(NVIDIA_SHARED_LIBRARY)

