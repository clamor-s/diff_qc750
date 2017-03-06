#
# Copyright (c) 2008-2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := ../../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvflash

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
LCINCS += $(NVLIB_HWINC_INCLUDES)

EXPORT_FILES := $(MODULE_NAME).export

ifeq ($(TARGET_OS),winxp)
WARN := $(WARN) -WX-
else
WARN :=
endif

ifeq ($(NVODM_BOARD_IS_FPGA),1)
CFLAGS += -DNVODM_BOARD_IS_FPGA=1
else
CFLAGS += -DNVODM_BOARD_IS_FPGA=0
endif

C_FILES :=
C_FILES += nvflash_commands.c
C_FILES += nvflash_configfile.c
C_FILES += nvflash_configfile.tab.c
C_FILES += nvflash_verifylist.c
C_FILES += lex.yy.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(TO_BUILD)
	$(DO_HOST_BUILD_TOO)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(TO_BUILD): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
$(TARGETS): $(NVLIB_NVOS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
