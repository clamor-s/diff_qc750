#
# Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := ../../..

include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvddk_blockdevmgr

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)

LCDEFS += -DNV_IS_AVP=$(IS_AVP)
LCDEFS += -DNV_USE_FUSE_CLOCK_ENABLE=0

ifneq ($(TARGET_SOC),ap20)
LCDEFS += -DNV_IF_NOT_AP20
endif
ifeq ($(NVODM_BOARD_IS_FPGA),1)
LCDEFS += -DNVODM_BOARD_IS_FPGA
endif

EXPORT_FILES := $(MODULE_NAME).export

C_FILES := nvddk_blockdevmgr.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	@echo TARGET_SOC: $(TARGET_SOC)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

LIB_DEPS :=
ifeq ($(NV_DEF_USE_DLL),1)
LIB_DEPS += $(NVLIB_NVRM)
LIB_DEPS += $(NVLIB_NVOS)
LIB_DEPS += $(NVLIB_NVODM_QUERY)
LIB_DEPS += $(NVLIB_NVDDK_NAND)
LIB_DEPS += $(NVLIB_NVDDK_SDIO)
LIB_DEPS += $(NVLIB_NVDDK_AES)
LIB_DEPS += $(NVLIB_NVDDK_SPIF)
LIB_DEPS += $(NVLIB_NVDDK_SNOR)
ifneq ($(TARGET_SOC),ap20)
LIB_DEPS += $(NVLIB_NVDDK_SE)
endif
endif

$(TARGETS): $(OBJS) $(LIB_DEPS) $(OUTDIR)/$(MODULE_NAME).def

include $(TEGRA_TOP)/core-private/make/Makefile.rules
