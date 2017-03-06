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


SUBDIRS +=
MODULE_NAME := libnvodm_kbc_keymapping

# generate static lib
LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)

EXPORT_FILES := $(MODULE_NAME).export

C_FILES :=
C_FILES += nvodm_kbc_keymapping.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(LIB) $(NVODMLIB_INSTALL_TARGET)

$(LIB): $(OBJS)


include $(TOPDIR)/../core-private/make/Makefile.rules

