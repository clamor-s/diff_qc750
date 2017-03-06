#
# Copyright (c) 2008 -2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvbootupdate
ifneq ($(TARGET_OS),rvds)
 LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)
else
 LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
 TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))
endif

#LCDEFS += -DNV_ENABLE_DEBUG_PRINTS=1

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/drivers/hwinc

# Tell the compiler to use the statically linked C-runtime libraries
# instead of DLLs.
WINCE_USE_STATIC_RUNTIME_LIBS := 1

C_FILES :=
C_FILES += nvbu_private_ap20.c
C_FILES += nvbu_private_t30.c
C_FILES += nvbu.c

EXPORT_FILES := $(MODULE_NAME).export

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
ifeq ($(TARGET_OS),rvds)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)
else
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)
endif

ifneq ($(TARGET_OS),rvds)
  $(LIB): $(OBJS)
else
  $(TARGETS): $(OBJS)
  $(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
endif

DEPS :=
DEPS += $(OBJS)
DEPS += $(NVLIB_NVOS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
