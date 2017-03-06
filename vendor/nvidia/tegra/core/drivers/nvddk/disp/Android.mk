# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.

LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvddk_disp

ifneq ($(filter  t30,$(TARGET_TEGRA_VERSION)),)
LOCAL_CFLAGS += -DNV_MIPI_PAD_CTRL_EXISTS
endif
ifneq ($(filter  ap20,$(TARGET_TEGRA_VERSION)),)
LOCAL_CFLAGS += -DNV_INPUT_CTRL_EXISTS
endif

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/hwinc/$(TARGET_TEGRA_VERSION)
LOCAL_C_INCLUDES += $(TEGRA_TOP)/camera/core/hwinc/$(TARGET_TEGRA_VERSION)
ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LOCAL_CFLAGS += -DSET_KERNEL_PINMUX
endif

LOCAL_SRC_FILES += nvddk_disp.c
LOCAL_SRC_FILES += nvddk_disp_hw.c
LOCAL_SRC_FILES += nvddk_disp_edid.c
LOCAL_SRC_FILES += dc_hal.c
LOCAL_SRC_FILES += dc_crt_hal.c
LOCAL_SRC_FILES += dc_dsi_hal.c
LOCAL_SRC_FILES += dc_hdmi_hal.c
LOCAL_SRC_FILES += dc_tvo2_hal.c
LOCAL_SRC_FILES += dc_tvo_common.c
LOCAL_SRC_FILES += dc_sd3.c

LOCAL_SHARED_LIBRARIES += libnvodm_query

LOCAL_CFLAGS += -Wno-error=missing-field-initializers -Wno-error=enum-compare
LOCAL_NVIDIA_NO_EXTRA_WARNINGS := 1
include $(NVIDIA_STATIC_LIBRARY)
