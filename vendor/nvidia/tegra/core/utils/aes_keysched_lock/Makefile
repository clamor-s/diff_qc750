#
# Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs

MODULE_NAME := libnvseaes_keysched_lock_avp

C_FILES := se_aes_keyschedule_lock.c

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/utils/aes_keysched_lock

ifeq ($(TARGET_OS_SUBTYPE),aos)
  OPTIMIZER += -march=armv4t
endif

LIB := $(OUTPUT_DIRNAME)/$(MODULE_NAME)$(LIB_SUFFIX)

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX), $(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)

$(LIB): $(OBJS)

ifeq ($(NV_DEF_USE_DLL),0)
  RELEASE_PACKAGE_TARGETS := $(LIB)
  RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)
else
  RELEASE_PACKAGE_TARGETS :=
  RELEASE_PACKAGE_FILES :=
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules
