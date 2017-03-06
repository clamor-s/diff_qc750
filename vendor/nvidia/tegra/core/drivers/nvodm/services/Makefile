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


TARGET_SOC_LIST := ap20

ifneq (,$(filter $(TARGET_SOC),$(TARGET_SOC_LIST)))
  ifeq ($(TARGET_CPU),armv6)
    ifeq ($(ENABLE_SECURITY),1)
      SUBDIRS += secure
    endif
  endif
endif

MODULE_NAME := libnvodm_services

LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LCDEFS += -DSET_KERNEL_PINMUX
endif

ifeq ($(TARGET_OS_SUBTYPE),aos)
  OPTIMIZER += -march=armv4t
endif

ifeq ($(TARGET_DEVICE),cardhu)
LCDEFS += -DSKU_FUSEREAD=1
else
LCDEFS += -DSKU_FUSEREAD=0
endif

C_FILES :=
C_FILES += nvodm_services.c
C_FILES += nvodm_services_os.c
C_FILES += nvodm_services_ext.c
C_FILES += nvodm_services_common.c
C_FILES += nvodm_services_os_common.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)

$(LIB): $(OBJS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
