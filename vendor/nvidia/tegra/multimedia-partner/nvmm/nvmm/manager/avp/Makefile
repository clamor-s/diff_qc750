#
# Copyright (c) 2008-2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

LCINCS += $(NVLIB_COMMON_INCLUDES)

ifneq ($(IS_AVP),1)
  $(error This makefile is for AVP only)
endif

APP_NAME := nvmm_manager

GCOPTS := $(subst --fpu None,--fpu vfpv2,$(GCOPTS))

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I../../include

LCDEFS += -DNV_IS_AVP=$(IS_AVP)

C_FILES :=
C_FILES += nvmm_manager_avp.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

LINKOPTS := --entry AxfModuleMain --map --list=$(OUTPUT_DIRNAME)/$(APP_NAME).map --info sizes,totals,common,unused --symbols
EXECUTABLES := $(OUTPUT_DIRNAME)/$(APP_NAME)$(EXE_SUFFIX)

default: submake.default 
	$(MAKE) $(EXECUTABLES)
	$(NV_INSTALL) -l $(EXECUTABLES) $(INSTALL_TARGET)

$(EXECUTABLES): $(OBJS)
$(EXECUTABLES): $(NVLIB_NVRM)
$(EXECUTABLES): $(NVLIB_NVOS)
$(EXECUTABLES): nvmm_manager.ldscript

include $(TEGRA_TOP)/core-private/make/Makefile.rules
