#
# Copyright (c) 2009 - 2012 NVIDIA Corporation.  All Rights Reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property and
# proprietary rights in and to this software and related documentation. Any
# use, reproduction, disclosure or distribution of this software and related
# documentation without an express license agreement from NVIDIA Corporation
# is strictly prohibited.

TOPDIR := ../..

include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvbct
ifneq ($(TARGET_OS),rvds)
 LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)
else
 LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
 TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))
endif

LIB_HOST_STATIC := $(OUTDIR)/$(MODULE_NAME)_host_static$(LIB_SUFFIX)
TO_BUILD := $(LIB) $(LIB_HOST_STATIC)

# Build the host static lib only on the host
ifeq ($(HOST_BUILD),0)
  ifneq ($(OUTDIR),$(HOST_OUTDIR))
    TO_BUILD := $(LIB)
  endif
else
  LCDEFS += -DNVODM_BOARD_IS_SIMULATION=1
endif
#LCDEFS += -DNV_ENABLE_DEBUG_PRINTS=1

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/system/nvbct
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core/common

# Tell the compiler to use the statically linked C-runtime libraries
# instead of DLLs.
LCDEFS += -DNV_IS_AOS=1

ifneq ($(TARGET_OS),rvds)
  LCDEFS += -DNV_USE_FUSE_CLOCK_ENABLE=0
endif

LCDEFS += -DNV_IS_AVP=$(IS_AVP)

C_FILES :=
C_FILES += nvbct_private_ap20.c
C_FILES += nvbit_private_ap20.c
C_FILES += nvbct_private_t30.c
C_FILES += nvbit_private_t30.c
C_FILES += nvbct.c
C_FILES += nvbit.c
C_FILES += nvfuse.c
C_FILES += nvfuse_ap20.c
C_FILES += nvfuse_t30.c

EXPORT_FILES := $(MODULE_NAME).export

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(TO_BUILD)
	$(DO_HOST_BUILD_TOO)
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

$(LIB_HOST_STATIC): $(OBJS)

DEPS :=
DEPS += $(OBJS)
DEPS += $(NVLIB_NVOS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
