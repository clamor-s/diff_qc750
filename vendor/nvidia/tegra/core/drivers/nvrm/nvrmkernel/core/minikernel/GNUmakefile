#
# Copyright (c) 2009-2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := ../../../../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvrm_minikernel

LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core/common
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core/ap15

LCDEFS += -DNV_IS_AVP=0
LCDEFS += -DNV_USE_FUSE_CLOCK_ENABLE=0
LCDEFS += -DCONFIG_PLLP_BASE_AS_408MHZ=$(CONFIG_PLLP_BASE_AS_408MHZ)

C_FILES :=
C_FILES += rm_minikernel_init.c
C_FILES += ../common/nvrm_configuration.c
C_FILES += ../common/nvrm_keylist.c
C_FILES += ../common/nvrm_module.c
C_FILES += ../common/nvrm_relocation_table.c
C_FILES += ../ap15/ap15rm_hwmap.c
C_FILES += ../ap15/ap15rm_init_common.c
C_FILES += ../ap15/ap15rm_interrupt.c
C_FILES += ../ap20/ap20rm_reloctable.c
C_FILES += ../t30/t30rm_reloctable.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)

$(LIB): $(OBJS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
