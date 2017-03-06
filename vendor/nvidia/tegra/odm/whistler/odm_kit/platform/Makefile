#
# Copyright (c) 2008 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/../core-private/make/Makefile.defs


ifneq ($(NO_SRC_MODULES),1)
SUBDIRS += touch
SUBDIRS += scrollwheel
SUBDIRS += nled
SUBDIRS += vibrate
endif

include $(TOPDIR)/../core-private/make/Makefile.rules


