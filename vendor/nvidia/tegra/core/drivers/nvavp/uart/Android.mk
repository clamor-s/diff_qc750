#
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#

LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvavpuart
LOCAL_ARM_MODE := arm
LOCAL_CFLAGS += -Os -ggdb0
ifeq ($(AVP_EXTERNAL_TOOLCHAIN),)
  LOCAL_C_INCLUDES += $(TOP)/bionic/libc/include
  LOCAL_CFLAGS += -D_MALLOC_H_
endif

ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LOCAL_CFLAGS += -DSET_KERNEL_PINMUX
endif

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/hwinc/$(TARGET_TEGRA_VERSION)
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvavp/uart
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvavp/uart/$(TARGET_TEGRA_VERSION)
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvboot/$(TARGET_TEGRA_VERSION)/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/utils/nvos/aos

ifneq ($(NV_EMBEDDED_BUILD), 1)
LOCAL_SRC_FILES := avp_uart.c
LOCAL_SRC_FILES += avp_vsnprintf.c
ifneq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LOCAL_SRC_FILES += $(TARGET_TEGRA_VERSION)/avp_uart_soc.c
endif
else
LOCAL_SRC_FILES := avp_uart_stub.c
endif

include $(NVIDIA_STATIC_AVP_LIBRARY)
