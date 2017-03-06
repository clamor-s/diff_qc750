#
# Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := ../../../..
include $(TOPDIR)/make/Makefile.defs


MODULE_NAME := libnvddk_secure_aes

LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TOPDIR)/drivers/nvddk/aes

C_FILES :=

C_FILES += ../ap20/ap20_aes_hw_utils.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)

$(LIB): $(OBJS)

include $(TOPDIR)/make/Makefile.rules
