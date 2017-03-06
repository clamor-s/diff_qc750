#
# Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR ?= $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvdc

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGET_LIB := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))
TARGETS := $(TARGET_LIB)

EXPORT_FILES := ../$(MODULE_NAME).export

C_FILES := $(wildcard *.c)
OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

LCINCS += $(NVLIB_COMMON_INCLUDES)

default: submake.default
	$(MAKE) $(TARGETS)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(TARGETS): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def

RELEASE_PACKAGE_TARGETS := $(TARGETS)
RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
