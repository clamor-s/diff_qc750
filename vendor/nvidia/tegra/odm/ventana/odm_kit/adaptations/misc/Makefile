#
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/../core-private/make/Makefile.defs


MODULE_NAME := libnvodm_misc

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)

EXPORT_FILES := $(MODULE_NAME).export

ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LCDEFS += -DSET_KERNEL_PINMUX
endif

C_FILES :=
C_FILES += nvodm_sdio.c
C_FILES += nvodm_uart.c
C_FILES += nvodm_kbc.c
C_FILES += nvodm_usbulpi.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(NVODMLIB_INSTALL_TARGET)

$(TARGETS): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
ifeq ($(NV_DEF_USE_DLL),1)
  $(TARGETS): $(NVLIB_NVRM)
  $(TARGETS): $(NVLIB_NVOS)
  $(TARGETS): $(NVLIB_NVODM_SERVICES)
  $(TARGETS): $(NVLIB_NVODM_QUERY)
endif

include $(TOPDIR)/../core-private/make/Makefile.rules
