TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/../core-private/make/Makefile.defs


MODULE_NAME := libnvodm_usb_mux

LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I..
LCINCS += -I.

EXPORT_FILES := $(MODULE_NAME).export

C_FILES :=
C_FILES += usb_mux_hal.c
C_FILES += usb_mux_null.c
C_FILES += usb_mux_j7aj6.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(LIB) $(NVODMLIB_INSTALL_TARGET)

$(LIB): $(OBJS)

include $(TOPDIR)/../core-private/make/Makefile.rules
