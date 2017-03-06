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

LOCAL_MODULE := libnvodm_imager
LOCAL_PRELINK_MODULE := false

ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LOCAL_CFLAGS += -DSET_KERNEL_PINMUX
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/configs
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../query/include

LOCAL_SRC_FILES += nvodm_imager.c
LOCAL_SRC_FILES += nvodm_imager_i2c.c
LOCAL_SRC_FILES += nvodm_imager_util.c
LOCAL_SRC_FILES += sensors/nvodm_sensor_common.c
LOCAL_SRC_FILES += sensors/nvodm_sensor_host.c
LOCAL_SRC_FILES += sensors/nvodm_sensor_mi5130.c
LOCAL_SRC_FILES += sensors/nvodm_sensor_mi5130_i2c.c
LOCAL_SRC_FILES += sensors/nvodm_sensor_null.c

LOCAL_SHARED_LIBRARIES += libnvrm
LOCAL_SHARED_LIBRARIES += libnvrm_graphics
LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_SHARED_LIBRARIES += libnvodm_query
LOCAL_STATIC_LIBRARIES += libnvodm_services

#TODO: Remove following lines before include statement and fix the source giving warnings/errors
LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
include $(NVIDIA_SHARED_LIBRARY)
