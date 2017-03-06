# Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an
# express license agreement from NVIDIA Corporation is strictly prohibited.

TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

MODULE_NAME := libnvavp
EXPORT_FILES := $(MODULE_NAME).export
LIB := $(OUTPUT_DIRNAME)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTPUT_DIRNAME)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I$(OUTDIR)

ifneq (,$(filter $(TARGET_OS)-$(TARGET_OS_SUBTYPE),linux-gnu_linux))
  C_FILES := nvavp.c
else
  ifeq ($(TARGET_OS),qnx)
    C_FILES := nvavp_qnx.c
  else
    C_FILES := nvavp_stub.c
  endif
endif


OBJ_FILES := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)

$(TARGETS): $(OBJ_FILES)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
$(TARGETS): $(NVLIB_NVRM)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
