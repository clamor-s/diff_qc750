#
# Copyright (c) 2008 - 2009 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/../core-private/make/Makefile.defs


SUBDIRS += keyboard
SUBDIRS += battery
SUBDIRS += mouse
SUBDIRS += powerkeys

SUBDIRS += $(TOPDIR)/customers/nvidia-partner/template/odm_kit/platform/scrollwheel
SUBDIRS += $(TOPDIR)/customers/nvidia-partner/template/odm_kit/platform/accelerometer
SUBDIRS += $(TOPDIR)/customers/nvidia-partner/template/odm_kit/platform/touch
SUBDIRS += $(TOPDIR)/customers/nvidia-partner/template/odm_kit/platform/vibrate
SUBDIRS += $(TOPDIR)/customers/nvidia-partner/template/odm_kit/platform/ota

include $(TOPDIR)/../core-private/make/Makefile.rules

