##
# Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
# Makefile for nvdiagnostics
#
TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


LIB := $(OUTDIR)/libnvdiagnostics$(LIB_SUFFIX)

LCINCS += -Iinclude
LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/system/nvdiagnostics/pwm/$(TARGET_SOC)

C_FILES :=
ifneq ($(filter t30,$(TARGET_SOC)),)
C_FILES += sd/nvddk_sd_block_driver_test_t30.c
C_FILES += se/se_test_t30.c
C_FILES += pwm/pwm_test.c
C_FILES += disp/dsi/nvddk_dsi_block_driver_test.c
else
C_FILES += sd/nvddk_sd_block_driver_test_stub.c
C_FILES += se/se_test_stub.c
C_FILES += pwm/pwm_test_stub.c
C_FILES += disp/dsi/nvddk_dsi_block_driver_test_stub.c
endif

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)

$(LIB): $(OBJS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
