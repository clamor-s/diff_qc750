TOPDIR := ../../..
include $(TEGRA_TOP)/core/make/Makefile.defs


MODULE_NAME := libnvintr_secure

LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)

CFLAGS += -DENABLE_SECURITY=1
CFLAGS += -DTRUSTZONE_ENABLED=1

C_FILES :=
C_FILES += ../nvintrhandler.c
C_FILES += ../nvintrhandler_ap15.c
C_FILES += ../nvintrhandler_ap20.c
C_FILES += ../nvintrhandler_gpio.c
C_FILES += ../nvintrhandler_gpio_ap15.c
C_FILES += ../nvintrhandler_gpio_ap20.c

# OBJECT files 
#######################################
OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

# Build Rules
#######################################
default: $(LIB)

DEPS :=
DEPS += $(OBJS)

$(LIB): $(DEPS)

include $(TEGRA_TOP)/core/make/Makefile.rules
