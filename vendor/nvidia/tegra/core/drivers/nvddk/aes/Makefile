#
# Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := ../../../

include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvddk_aes

EXPORT_FILES := $(MODULE_NAME).export

# Env variable used to enable multi-partition change
  SUBDIRS += nvddk
ifeq ($(ENABLE_SECURITY), 1)
  SUBDIRS += secure
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules
