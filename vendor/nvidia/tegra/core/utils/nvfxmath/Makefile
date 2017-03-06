#
# Copyright (c) 2005 - 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


C_FILES := asincos.c sincos.c
LCINCS += $(NVLIB_COMMON_INCLUDES)

USE_ASM := 0
ifneq (,$(filter $(TARGET_OS),linux))
  ifneq (,$(filter $(TARGET_CPU),armv4 armv5 armv6))
    USE_ASM := 1
  endif
endif

ifeq ($(TARGET_CPU),armv4)
  LCDEFS += -DARMV4I
endif

ifneq ($(USE_ASM),1)
  C_FILES += rsqrt.c divmod.c explog.c atan.c
  ifeq ($(FLOATING_POINT_SUPPORTED),1)
    C_FILES += convert.c
  endif
else
  S_FILES :=
  S_FILES += fxconv_arm.S
  S_FILES += fxdiv_arm.S
  S_FILES += fxrsqrt_arm.S
  S_FILES += fxexplog_arm.S
  S_FILES += fxatan_arm.S

  ifeq ($(TARGET_FLOAT_ABI_HARD),1)
    LCDEFS += -DEABI_IS_HARD
  endif
endif

ifeq ($(TARGET_OS),linux)
  OPTIMIZER += -O3
endif

LIB := $(OUTPUT_DIRNAME)/libnvfxmath$(LIB_SUFFIX)

OBJS := $(patsubst %.c,$(OUTPUT_DIRNAME)/%$(OBJ_SUFFIX),$(C_FILES))
ifeq ($(USE_ASM),1)
  OBJS += $(patsubst %.S,$(OUTPUT_DIRNAME)/%$(OBJ_SUFFIX),$(S_FILES))
endif

$(LIB): $(OBJS)

ifeq ($(NV_DEF_USE_DLL),0)
  RELEASE_PACKAGE_TARGETS := $(LIB)
  RELEASE_PACKAGE_FILES := $(C_FILES) $(S_FILES)
else
  RELEASE_PACKAGE_TARGETS :=
  RELEASE_PACKAGE_FILES :=
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules
