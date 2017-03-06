##
# Copyright (c) 2005 - 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
# Makefile for nvstormgr

TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvstormgr
EXPORT_FILES := $(MODULE_NAME).export
LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)
LCINCS += -Iinclude
LCINCS += $(NVLIB_COMMON_INCLUDES)

C_FILES :=
C_FILES += nvstormgr.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)

$(LIB): $(OBJS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
