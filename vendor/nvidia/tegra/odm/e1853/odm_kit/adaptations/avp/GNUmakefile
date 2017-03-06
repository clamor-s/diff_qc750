#
# Copyright (c) 2012 NVIDIA CORPORATION.  All Rights Reserved.
#
TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/../core-private/make/Makefile.defs


ifeq ($(TARGET_OS),rvds)
nobuild:
	@echo This directory is not built for RVDS.
endif

MODULE_NAME := libnvodm_avp

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I..
LCINCS += -I.

C_FILES := nvodm_avp.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))
OBJS += $(NVLIB_NVODM_SERVICES_AVP)
LINKED_OBJ := $(OUTDIR)/$(MODULE_NAME)$(OBJ_SUFFIX)

default: $(LINKED_OBJ)
	$(NV_INSTALL) -l $(LINKED_OBJ) $(NVODMLIB_INSTALL_TARGET)
ifneq ($(NV_LIST_RELEASE_PACKAGE),1)
	-mkdir prebuilt
	-cp -f $(NVLIB_NVODM_SERVICES_AVP) prebuilt/
endif

$(LINKED_OBJ): $(OBJS)
ifneq ($(NV_LIST_RELEASE_PACKAGE),1)
	$(LD) -r -o $@ $^
	nm --extern-only $@ | awk '{print $$3}' | grep -v 'NvOdmAvp.*CpuInit' > $(OUTDIR)/symbols
	$(MAKEBIN) --localize-symbols=$(OUTDIR)/symbols $@
endif

$(NVLIB_NVODM_SERVICES_AVP):
	$(MAKE) -C $(TOPDIR)/drivers/nvodm/services/avp/

RELEASE_PACKAGE_TARGETS :=
RELEASE_PACKAGE_FILES :=
include $(TOPDIR)/../core-private/make/Makefile.rules
