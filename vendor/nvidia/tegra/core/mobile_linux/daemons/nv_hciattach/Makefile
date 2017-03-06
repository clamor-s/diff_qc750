#
# Copyright (c) 2009-2011 NVIDIA Corporation.  All rights reserved.
#

TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs



LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/utils/nvos/linux
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core/common
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/include

EXECUTABLES := $(OUTDIR)/nv_hciattach$(EXE_SUFFIX)

C_FILES :=
C_FILES += nv_hciattach.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(EXECUTABLES)
	$(NV_INSTALL) -l $(EXECUTABLES) $(INSTALL_TARGET)

$(EXECUTABLES): $(OBJS)
$(EXECUTABLES): $(NVLIB_NVOS)
$(EXECUTABLES): $(NVLIB_NVREFTRACK)
$(EXECUTABLES): $(NVLIB_NVDDK_AUDIOMIXER_IMPL)
$(EXECUTABLES): $(NVLIB_NVRM_KERNEL)
$(EXECUTABLES): $(NVLIB_DISPMGR_IMPL)
$(EXECUTABLES): $(NVLIB_NVDDK_DISP)
$(EXECUTABLES): $(NVLIB_NVDDK_2D)
$(EXECUTABLES): $(NVLIB_NVODM_HDMI)
$(EXECUTABLES): $(NVLIB_NVODM_DISP)
$(EXECUTABLES): $(NVLIB_DISPMGR_IMPL)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
