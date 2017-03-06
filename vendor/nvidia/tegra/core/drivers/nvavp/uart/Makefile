#
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#

TOPDIR := ../../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs

MODULE_NAME := libnvavpuart

ifneq ($(TARGET_OS),rvds)
 LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)
else
 LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
 TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))
endif

ifeq ($(TARGET_SOC),ap20)
 LCDEFS += -DCONFIG_PLLP_BASE_AS_408MHZ=0
else
 LCDEFS += -DCONFIG_PLLP_BASE_AS_408MHZ=1
endif

ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LCDEFS += -DSET_KERNEL_PINMUX
endif

ifeq ($(TARGET_OS_SUBTYPE),aos)
  OPTIMIZER += -march=armv4t
endif

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/include
LCINCS += -I$(TEGRA_TOP)/core/drivers/hwinc
LCINCS += -I$(TEGRA_TOP)/core/drivers/hwinc/$(TARGET_SOC)
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvavp/uart
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvavp/uart/$(TARGET_SOC)
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvboot/$(TARGET_SOC)/include
LCINCS += -I$(TEGRA_TOP)/core/utils/nvos/aos

ifneq ($(NV_EMBEDDED_BUILD), 1)
C_FILES := avp_uart.c
C_FILES += avp_vsnprintf.c
ifneq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
C_FILES += $(TARGET_SOC)/avp_uart_soc.c
endif
else
C_FILES := avp_uart_stub.c
endif

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
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules
