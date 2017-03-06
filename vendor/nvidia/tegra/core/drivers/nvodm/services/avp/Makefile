TOPDIR := ../../../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvodm_services

LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core-private/drivers/nvboot/ap20/include
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvboot/ap20/include

C_FILES :=
C_FILES += ../nvodm_services_os.c
C_FILES += ../nvodm_services_os_common.c
ifeq ($(TARGET_SOC),ap20)
  C_FILES += nvodm_services_avp.c
else
  C_FILES += nvodm_services_avp_i2c_t30.c
endif

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)

$(LIB): $(OBJS)

RELEASE_PACKAGE_TARGETS := $(LIB)
RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)
include $(TEGRA_TOP)/core-private/make/Makefile.rules
