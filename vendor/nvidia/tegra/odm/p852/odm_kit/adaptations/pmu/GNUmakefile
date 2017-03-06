TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs
include $(TEGRA_TOP)/core-private/make/Makefile.nvlibdefs

MODULE_NAME := libnvodm_pmu

ifeq ($(TARGET_OS),eabi)
GCOPTS += -DNV_SKU_SUPPORT
endif

LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I..
LCINCS += -I.

C_FILES :=
C_FILES += pmu_hal.c
C_FILES += nvodm_pmu_tps6586x_i2c.c
C_FILES += nvodm_pmu_tps6586x_interrupt.c
C_FILES += nvodm_pmu_tps6586x_batterycharger.c
C_FILES += nvodm_pmu_tps6586x_adc.c
C_FILES += nvodm_pmu_tps6586x.c
C_FILES += nvodm_pmu_tps6586x_rtc.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(LIB) $(NVODMLIB_INSTALL_TARGET)

$(LIB): $(OBJS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
