#
# Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
LOCAL_PATH := $(call my-dir)

_nvml_local_path := $(call my-dir)

#ap20
include $(_nvml_local_path)/Android.ap20.mk

#t30
include $(_nvml_local_path)/Android.t30.mk
