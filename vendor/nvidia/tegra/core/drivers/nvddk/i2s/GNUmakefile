#
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
TOPDIR := ../../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvddk_i2s

LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(LIB_SUFFIX))

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCDEFS += -DNV_IS_AVP=$(IS_AVP)
ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LCDEFS += -DSET_KERNEL_PINMUX
endif

C_FILES :=
C_FILES += nvddk/nvddk_i2s.c
C_FILES += ap15ddk_i2s_hw_private.c
C_FILES += t30ddk_i2s_hw_private.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(TARGETS): $(OBJS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
