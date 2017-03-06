#
# Copyright (c) 2012, NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
TOPDIR := ../../../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs

MODULE_NAME := libnvddk_fuse_read

LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(LIB_SUFFIX))

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvddk/fuses/common
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvddk/fuses/read
LCINCS += -I$(TEGRA_TOP)/core/include

ifeq ($(TARGET_CPU),armv4)
    LCDEFS += -DADDITIONAL_FUNCTIONALITY=0
else
    LCDEFS += -DADDITIONAL_FUNCTIONALITY=1
endif

C_FILES :=
S_FILES :=

C_FILES += $(TEGRA_TOP)/core/drivers/nvddk/fuses/read/ap20/nvddk_ap20_fuse_priv.c
C_FILES += $(TEGRA_TOP)/core/drivers/nvddk/fuses/read/t30/nvddk_t30_fuse_priv.c
C_FILES += $(TEGRA_TOP)/core/drivers/nvddk/fuses/read/nvddk_fuse_read.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(TARGETS): $(OBJS)
include $(TEGRA_TOP)/core-private/make/Makefile.rules
