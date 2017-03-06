##
# Copyright (c) 2005-2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
# Makefile for nvenhancedfilesystem
#
TOPDIR := ../../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


LIB := $(OUTDIR)/libnvenhancedfs$(LIB_SUFFIX)

LCINCS += -Iinclude
LCINCS += $(NVLIB_COMMON_INCLUDES)

C_FILES :=
C_FILES += nvenhancedfilesystem.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)

$(LIB): $(OBJS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
