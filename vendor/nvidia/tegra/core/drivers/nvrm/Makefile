#
# Copyright (c) 2007 - 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


ifeq ($(USE_AOS),1)
  LCDEFS += -DNV_USE_AOS=1
endif

ifeq ($(IS_AVP),1)
  SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvrm/avp
  SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvrm/avp/stub
  SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvrm/avp/kernel
else
  SUBDIRS += nvlimits
  SUBDIRS += nvrmkernel
  SUBDIRS += nvrmkernel/core/minikernel
  SUBDIRS += graphics
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules

.PHONY: sweep sweep.local
sweep: sweep.local
sweep.local:
	$(RM_RECURSIVE) avp/stub/_out avp/_out avp/kernel/_out
	$(RM_RECURSIVE) graphics/_out
