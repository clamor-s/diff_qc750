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

LOCAL_MODULE := libnvddk_audiodap

LOCAL_CFLAGS += -DENABLE_AP20_DAPDEFS
ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LOCAL_CFLAGS += -DSET_KERNEL_PINMUX
endif

LOCAL_SRC_FILES += nvddk_ap20daptable.c
LOCAL_SRC_FILES += nvddk_audiodap.c

LOCAL_CFLAGS += -Wno-error=cast-align
include $(NVIDIA_STATIC_LIBRARY)

