#
# Copyright (c) 2012, NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
HOST_BUILD := 1
TOPDIR := ../../../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs

MODULE_NAME := libnvddk_fuse_read_host

LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvddk/fuses/read
LCINCS += -I$(TEGRA_TOP)/core/include

C_FILES :=
S_FILES :=

C_FILES += nvddk_fuse_read.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default:$(LIB)
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)

$(LIB): $(OBJS)
include $(TEGRA_TOP)/core-private/make/Makefile.rules
