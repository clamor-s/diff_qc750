##
# Copyright (c) 2008-2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvfsmgr
LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)
#TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

LCINCS += $(NVLIB_COMMON_INCLUDES)

EXPORT_FILES := $(MODULE_NAME).export

C_FILES :=
C_FILES += nvfsmgr.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)

#$(TARGETS): $(OBJS)
#$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
$(LIB): $(OBJS)
include $(TEGRA_TOP)/core-private/make/Makefile.rules
