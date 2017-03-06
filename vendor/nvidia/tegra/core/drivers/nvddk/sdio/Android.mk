#
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvddk_sdio

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/hwinc

LOCAL_NVIDIA_NO_COVERAGE := true
LOCAL_CFLAGS += -DNV_USE_FUSE_CLOCK_ENABLE=0
LOCAL_CFLAGS += -DNV_IS_AVP=0
ifneq ($(TARGET_TEGRA_VERSION),ap20)
LOCAL_CFLAGS += -DNV_IF_NOT_AP20
endif
ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LOCAL_CFLAGS += -DSET_KERNEL_PINMUX
endif

LOCAL_SRC_FILES += nvddk_sd_block_driver.c
LOCAL_SRC_FILES += nvddk_sdio.c
LOCAL_SRC_FILES += ap20_sdio.c
LOCAL_SRC_FILES += t30_sdio.c

include $(NVIDIA_STATIC_LIBRARY)

