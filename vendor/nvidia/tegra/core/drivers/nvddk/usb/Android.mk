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

LOCAL_MODULE := libnvddk_usbphy

LOCAL_NVIDIA_NO_COVERAGE := true
LOCAL_CFLAGS += -DNV_USE_FUSE_CLOCK_ENABLE=0
LOCAL_CFLAGS += -DNV_IS_AVP=0
ifneq ($(TARGET_TEGRA_VERSION),ap20)
LOCAL_CFLAGS += -DNV_IF_NOT_AP20
endif
ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LOCAL_CFLAGS += -DSET_KERNEL_PINMUX
endif

LOCAL_SRC_FILES += usb/nvddk_usbphy.c
LOCAL_SRC_FILES += usb/nvddk_usbphy_ap20.c
LOCAL_SRC_FILES += usb/nvddk_usbphy_t30.c
LOCAL_SRC_FILES += usb/device/nvddk_usbf.c
LOCAL_SRC_FILES += usb/device/nvddk_usbf_priv.c
LOCAL_SRC_FILES += usb/otg/nvddk_usbotg.c
LOCAL_SRC_FILES += usb/host/nvddk_usbh.c
LOCAL_SRC_FILES += usb/host/nvddk_usbh_priv.c
LOCAL_SRC_FILES += usb/host/nvddk_usbh_priv_ap20.c
LOCAL_SRC_FILES += usb/host/nvddk_usbh_priv_t30.c

include $(NVIDIA_STATIC_LIBRARY)

