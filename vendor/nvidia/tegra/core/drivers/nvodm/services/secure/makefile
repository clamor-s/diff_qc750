TOPDIR := ../../../..
include $(TEGRA_TOP)/core/make/Makefile.defs


MODULE_NAME := libnvodm_services_secure

LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)

C_FILES :=
C_FILES += nvodm_services_secure.c
C_FILES += ../nvodm_services_common.c
C_FILES += ../nvodm_services_os_common.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)

$(LIB): $(OBJS)

include $(TEGRA_TOP)/core/make/Makefile.rules
