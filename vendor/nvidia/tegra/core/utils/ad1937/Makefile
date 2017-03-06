#
# Copyright 2009 by NVIDIA Corporation.  All rights reserved.
#
TOPDIR := ../..
include $(TOPDIR)/../core-private/make/Makefile.defs
include $(TOPDIR)/../core-private/make/Makefile.nvlibdefs

LCINCS += -I$(TOPDIR)/include
LCINCS += -I$(TOPDIR)/utils/nvos/linux

EXECUTABLES := $(OUTDIR)/ad1937$(EXE_SUFFIX)

C_FILES :=
C_FILES += main.c
C_FILES += ad1937.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(EXECUTABLES)
	$(NV_INSTALL) -l $(EXECUTABLES) $(INSTALL_TARGET)

$(EXECUTABLES): $(OBJS)
$(EXECUTABLES): $(NVLIB_NVOS)
$(EXECUTABLES): $(dynamic_deps)

include $(TOPDIR)/../core-private/make/Makefile.rules
