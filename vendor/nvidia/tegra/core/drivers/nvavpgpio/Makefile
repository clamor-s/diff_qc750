TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/drivers/hwinc/$(TARGET_SOC)
LCINCS += -I$(TEGRA_TOP)/core/utils/nvos/aos

LIB := $(OUTDIR)/libnvgpio_avp$(LIB_SUFFIX)


C_FILES += avp_gpio.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)

$(LIB): $(OBJS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
