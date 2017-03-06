#
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
TOPDIR := ../..
include $(TOPDIR)/../core-private/make/Makefile.defs
include $(TOPDIR)/../core-private/make/Makefile.nvlibdefs

LCINCS += -I$(TOPDIR)/include
LCINCS += -I$(TOPDIR)/utils/nvos/linux

EXECUTABLES := $(OUTDIR)/wm8731$(EXE_SUFFIX)

C_FILES :=
C_FILES += wm8731.c
C_FILES += main.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(EXECUTABLES)
	$(NV_INSTALL) -l $(EXECUTABLES) $(INSTALL_TARGET)

$(EXECUTABLES): $(OBJS)
$(EXECUTABLES): $(NVLIB_NVOS)
$(EXECUTABLES): $(dynamic_deps)

include $(TOPDIR)/../core-private/make/Makefile.rules
