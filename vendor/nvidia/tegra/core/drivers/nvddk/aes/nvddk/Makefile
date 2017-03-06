#
# Copyright (c) 2008-2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := ../../../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvddk_aes

LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(LIB_SUFFIX))

#LCDEFS += -DNV_ENABLE_DEBUG_PRINTS=1

ifeq ($(ENABLE_SECURITY), 1)
CFLAGS += -DENABLE_SECURITY=1
else
CFLAGS += -DENABLE_SECURITY=0
endif

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TOPDIR)/drivers/nvddk/aes

EXPORT_FILES := $(MODULE_NAME).export

C_FILES :=
S_FILES :=

C_FILES += ../nvddk_aes_blockdev.c
C_FILES += ../nvddk_aes_core.c
C_FILES += ../ap20/nvddk_aes_hw_ap20.c


ifeq ($(ENABLE_SECURITY), 1)
  C_FILES += smc_aes_stub_c.c
  S_FILES += smc_aes_stub_asm.S
else
  C_FILES += ../ap20/ap20_aes_hw_utils.c
endif

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))
OBJS += $(patsubst %.S,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(S_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(TARGETS): $(OBJS)
ifeq ($(NV_DEF_USE_DLL),1)
  $(TARGETS): $(NVLIB_NVRM)
endif

RELEASE_PACKAGE_TARGETS := $(TARGETS)
RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)

ifeq ($(ENABLE_SECURITY), 1)
smc_aes_stub_c.c: $(TEGRA_TOP)/core-private/make/smcgen.py $(OUTDIR)/dummy.txt
	$(PYTHON) $< -client -c NvSecureServiceAes >$@

smc_aes_stub_asm.S: $(TEGRA_TOP)/core-private/make/smcgen.py $(OUTDIR)/dummy.txt
	$(PYTHON) $< -client -arm NvSecureServiceAes >$@
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules
