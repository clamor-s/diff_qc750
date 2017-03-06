#
# Copyright (c)  2009 - 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := ../../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


LIB := $(OUTPUT_DIRNAME)/libnvrm_limits$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvrm
LCINCS += -I$(TEGRA_TOP)/core-private/drivers/nvrm
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core/common
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core/ap15
LCINCS += $(NVLIB_HWINC_INCLUDES)

LCDEFS += -DCONFIG_PLLP_BASE_AS_408MHZ=$(CONFIG_PLLP_BASE_AS_408MHZ)

# Tell the compiler to use the statically linked C-runtime libraries
# instead of DLLs.
WINCE_USE_STATIC_RUNTIME_LIBS := 1

C_FILES :=
C_FILES += ap20rm_clocks_limits.c
C_FILES += t30rm_clocks_limits.c
C_FILES += nvrm_clocks_limits_init.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX), $(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)

$(LIB): $(OBJS)

RELEASE_PACKAGE_TARGETS := $(LIB)
RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
