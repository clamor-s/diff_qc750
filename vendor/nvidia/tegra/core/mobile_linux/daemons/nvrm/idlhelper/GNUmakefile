#
# Copyright (c) 2009-2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvdispatch_helper

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core/common

EXPORT_FILES := $(MODULE_NAME).export

C_FILES :=
C_FILES += nvrm_daemon_idlhelper.c

ifneq ($(TARGET_OS_SUBTYPE),android)
  LDFLAGS_DLL += -lpthread
endif

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(TARGETS): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
ifeq ($(NV_DEF_USE_DLL),1)
$(TARGETS): $(NVLIB_NVOS)
$(TARGETS): $(NVLIB_NVRM_KERNEL)
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules
