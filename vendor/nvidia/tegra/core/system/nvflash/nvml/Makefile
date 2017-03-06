#
# Copyright (c) 2008 -2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := ../../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


SUBDIRS :=
ifeq ($(TARGET_OS),rvds)
ifeq ($(TARGET_CPU),armv4)
SUBDIRS += ap20
SUBDIRS += t30
endif
endif

$(warning export HOST_OS = $(HOST_OS))
$(warning export BUILD_OS = $(BUILD_OS))
$(warning export TARGET_OS = $(TARGET_OS))
$(warning export TARGET_CPU = $(TARGET_CPU))
$(warning export TARGET_GPU = $(TARGET_GPU))

include $(TEGRA_TOP)/core-private/make/Makefile.rules
