#
# Copyright (c) 2009-2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


LIB := $(OUTDIR)/libnvreftrack$(LIB_SUFFIX)

# INCLUDE directories (LCINC)
#######################################
LCINCS += $(NVLIB_COMMON_INCLUDES)

# Sources (C_FILES)
#######################################
C_FILES := nvreftrack.c

# OBJECT files from C_FILES
#######################################
OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

# Build rules
#######################################

TARGETS := $(LIB)

default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(LIB): $(OBJS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
