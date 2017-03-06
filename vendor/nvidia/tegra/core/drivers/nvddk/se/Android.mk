LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

ifneq ($(TARGET_TEGRA_VERSION),ap20)
LOCAL_MODULE := libnvddk_se

LOCAL_NVIDIA_NO_COVERAGE := true
LOCAL_CFLAGS += -DNV_USE_FUSE_CLOCK_ENABLE=0
LOCAL_CFLAGS += -DNV_IS_AVP=0

LOCAL_SRC_FILES += nvddk_se_blockdev.c
LOCAL_SRC_FILES += nvddk_se_common_core.c
LOCAL_SRC_FILES += nvddk_se_common_hw.c

LOCAL_CFLAGS += -Wno-error=cast-align

include $(NVIDIA_STATIC_LIBRARY)
endif
