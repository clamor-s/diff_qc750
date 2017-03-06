# Copyright (c) 2008-2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvdioconverter

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
LIB_HOST_STATIC := $(OUTDIR)/$(MODULE_NAME)_host_static$(LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

TO_BUILD := $(LIB) $(LIB_HOST_STATIC)

# Build the host static lib only on the host
ifeq ($(HOST_BUILD),0)
  ifneq ($(OUTDIR),$(HOST_OUTDIR))
    TO_BUILD := $(LIB)
  endif
endif

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/system/nvflash/app

EXPORT_FILES := $(MODULE_NAME).export

C_FILES :=
C_FILES += nvdioconverter.c
C_FILES += nvstorebinconverter.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(TO_BUILD)
	$(DO_HOST_BUILD_TOO)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(TO_BUILD): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
$(TARGETS): $(NVLIB_APPUTIL)
ifeq ($(NV_DEF_USE_DLL),1)
  $(TARGETS): $(NVLIB_NVOS)
endif

RELEASE_PACKAGE_TARGETS := $(TARGETS)
RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
