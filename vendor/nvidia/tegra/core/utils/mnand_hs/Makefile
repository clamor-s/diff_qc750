TOPDIR := ../..
include $(TOPDIR)/../core-private/make/Makefile.defs
include $(TOPDIR)/../core-private/make/Makefile.nvlibdefs

EXECUTABLES := $(OUTDIR)/mnand_hs$(EXE_SUFFIX)
C_FILES := mnand_hs.c

LCINCS += -I$(TOPDIR)/include
LCINCS += -I$(KERN_DIR)/include

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(EXECUTABLES)
	$(NV_INSTALL) -l $(EXECUTABLES) $(INSTALL_TARGET)

$(EXECUTABLES): $(OBJS)

include $(TOPDIR)/../core-private/make/Makefile.rules
