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
LOCAL_NVIDIA_NO_COVERAGE := true

LOCAL_MODULE := libnvodm_misc

ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LOCAL_CFLAGS += -DSET_KERNEL_PINMUX
endif

LOCAL_SRC_FILES += nvodm_sdio.c
LOCAL_SRC_FILES += nvodm_uart.c
LOCAL_SRC_FILES += nvodm_kbc.c
LOCAL_SRC_FILES += nvodm_usbulpi.c

LOCAL_SHARED_LIBRARIES += libnvrm
LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_SHARED_LIBRARIES += libnvodm_query
LOCAL_STATIC_LIBRARIES += libnvodm_services

LOCAL_NVIDIA_RM_WARNING_FLAGS := -Wundef
LOCAL_NVIDIA_RM_WARNING_FLAGS := -Wundef
include $(NVIDIA_STATIC_AND_SHARED_LIBRARY)
