##
# Copyright (c) 2005 - 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
# Makefile for nv3pserver
#

TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs



LIB := $(OUTDIR)/libnv3pserver$(LIB_SUFFIX)
LIB_HOST_STATIC := $(OUTDIR)/libnv3pserver_host_static$(LIB_SUFFIX)

TO_BUILD := $(LIB) $(LIB_HOST_STATIC)
TO_INSTALL := $(LIB)

# Build the host static lib only on the host
ifeq ($(HOST_BUILD),0)
  ifneq ($(OUTDIR),$(HOST_OUTDIR))
    TO_BUILD := $(LIB)
  endif
endif

ifeq ($(NVODM_BOARD_IS_FPGA), 1)
CFLAGS += -DNVODM_BOARD_IS_FPGA=1
else
CFLAGS += -DNVODM_BOARD_IS_FPGA=0
endif

ifeq ($(NVODM_BOARD_IS_SIMULATION), 1)
CFLAGS += -DNVODM_BOARD_IS_SIMULATION=1
else
CFLAGS += -DNVODM_BOARD_IS_SIMULATION=0
endif

LCINCS += -Iinclude
LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/system/nvdiagnostics/sd
LCINCS += -I$(TEGRA_TOP)/core/system/nvdiagnostics/se
LCINCS += -I$(TEGRA_TOP)/core/system/nvdiagnostics/pwm
LCINCS += -I$(TEGRA_TOP)/core/system/nvdiagnostics/disp/dsi

ifeq ($(TARGET_OS),rvds)
  CFLAGS += -DNV_IS_AOS=1
else
  ifeq ($(TARGET_OS_SUBTYPE),aos)
    CFLAGS += -DNV_IS_AOS=1
  else
    CFLAGS += -DNV_IS_AOS=0
  endif
endif


# AP20 use the legacy address map with SDRAM at address zero.
# Everything else will use the newer address map.
ifneq ($(filter ap20, $(TARGET_SOC)),)
CFLAGS += -DNV_AOS_ENTRY_POINT=0x108000
CFLAGS += -DNV_CHARGE_ENTRY_POINT=0x40008000
else
CFLAGS += -DNV_AOS_ENTRY_POINT=0x80108000
CFLAGS += -DNV_CHARGE_ENTRY_POINT=0x4000a000
endif

# Common public C source files
C_FILES :=
C_FILES += nv3p_server.c
C_FILES += nv3p_server_utils_ap20.c
C_FILES += nv3p_server_utils_t30.c
C_FILES += nv3p_server_utils.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(TO_BUILD)
	$(DO_HOST_BUILD_TOO)
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)


$(LIB): $(OBJS)
$(TARGETS): $(NVLIB_NVRM) $(NVLIB_NVOS)


include $(TEGRA_TOP)/core-private/make/Makefile.rules


