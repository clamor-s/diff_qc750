# Copyright (c) 2008-2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.

TOPDIR := ../../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvddk_disp

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

#LCDEFS += -DNV_ENABLE_DEBUG_PRINTS=1

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/drivers/hwinc/$(TARGET_SOC_FAMILY)
LCINCS += -I$(TEGRA_TOP)/camera/core/hwinc/$(TARGET_SOC_FAMILY)

ifneq ($(filter  t30,$(TARGET_SOC)),)
CFLAGS += -DNV_MIPI_PAD_CTRL_EXISTS
endif
ifneq ($(filter  ap20,$(TARGET_SOC)),)
CFLAGS += -DNV_INPUT_CTRL_EXISTS
endif

ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LCDEFS += -DSET_KERNEL_PINMUX
endif

EXPORT_FILES := $(MODULE_NAME).export

C_FILES :=
C_FILES += nvddk_disp.c
C_FILES += nvddk_disp_hw.c
C_FILES += nvddk_disp_edid.c
C_FILES += dc_hal.c
C_FILES += dc_crt_hal.c
C_FILES += dc_dsi_hal.c
C_FILES += dc_hdmi_hal.c
C_FILES += dc_tvo2_hal.c
C_FILES += dc_tvo_common.c
C_FILES += dc_sd3.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(TARGETS): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
ifeq ($(NV_DEF_USE_DLL),1)
  $(TARGETS): $(NVLIB_NVRM_DAEMON)
  $(TARGETS): $(NVLIB_NVOS)
  $(TARGETS): $(NVLIB_NVODM_DISP)
  $(TARGETS): $(NVLIB_FXMATH)
  $(TARGETS): $(NVLIB_NVODM_QUERY)
  $(TARGETS): $(NVLIB_NVODM_SERVICES)
endif

RELEASE_PACKAGE_TARGETS := $(TARGETS)
RELEASE_PACKAGE_FILES := $(C_FILES)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
