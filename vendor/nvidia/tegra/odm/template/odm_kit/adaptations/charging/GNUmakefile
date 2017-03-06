TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/../core-private/make/Makefile.defs


MODULE_NAME := libnvodm_charging

LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I..
LCINCS += -I.
LCINCS += -I$(TEGRA_TOP)/core/system/nvaboot

# Tell the compiler to use the statically linked C-runtime libraries
# instead of DLLs.
WINCE_USE_STATIC_RUNTIME_LIBS := 1

LCDEFS += -DLPM_BATTERY_CHARGING=1
#Charging ODM adaption files

C_FILES :=
C_FILES += charging.c
C_FILES += charger-usb.c
C_FILES += battery-basic.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(LIB) $(NVODMLIB_INSTALL_TARGET)

$(LIB): $(OBJS)

include $(TOPDIR)/../core-private/make/Makefile.rules
