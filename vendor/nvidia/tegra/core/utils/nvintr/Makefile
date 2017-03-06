TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvintr

TARGET_SOC_LIST := ap20
ifeq ($(TARGET_SOC),ap20)
CFLAGS += -DTARGET_SOC_AP20
endif


LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core/common

ifeq ($(ENABLE_SECURITY),1)
  CFLAGS += -DTRUSTZONE_ENABLED=1
else
  CFLAGS += -DTRUSTZONE_ENABLED=0
endif

CFLAGS += -DENABLE_SECURITY=0

ifneq (,$(filter $(TARGET_SOC),$(TARGET_SOC_LIST)))
  ifeq ($(TARGET_CPU),armv6)
    ifeq ($(ENABLE_SECURITY),1)
      SUBDIRS += secure
    endif
  endif
endif

C_FILES :=
C_FILES += nvintrhandler.c
C_FILES += nvintrhandler_common.c
C_FILES += nvintrhandler_sim.c
C_FILES += nvintrhandler_ap20.c
C_FILES += nvintrhandler_gpio.c
C_FILES += nvintrhandler_gpio_ap20.c
C_FILES += nvintrhandler_gpio_t30.c
C_FILES += nvintrhandler_apbdma.c
C_FILES += nvintrhandler_apbdma_ap20.c
C_FILES += nvintrhandler_apbdma_t30.c

EXPORT_FILES := $(MODULE_NAME).export

# OBJECT files
#######################################
OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

# Build Rules
#######################################
default: $(LIB)
	$(DO_HOST_BUILD_TOO)
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)

DEPS :=
DEPS += $(OBJS)

$(LIB): $(DEPS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
