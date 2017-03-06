# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.

TOPDIR := ../..
include $(TOPDIR)/../core-private/make/Makefile.defs
include $(TOPDIR)/../core-private/make/Makefile.nvlibdefs

EXECUTABLES := $(OUTDIR)/mnand_rfsh$(EXE_SUFFIX)
C_FILES := mnand_rfsh.c

LCINCS += -I$(TOPDIR)/include
LCINCS += -I$(KERN_DIR)/include

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(EXECUTABLES)
	$(NV_INSTALL) -l $(EXECUTABLES) $(INSTALL_TARGET)

$(EXECUTABLES): $(OBJS)

include $(TOPDIR)/../core-private/make/Makefile.rules
