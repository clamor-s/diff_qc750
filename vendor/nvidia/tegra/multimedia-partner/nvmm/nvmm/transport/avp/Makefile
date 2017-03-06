TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

LCINCS += $(NVLIB_COMMON_INCLUDES)

ifneq ($(IS_AVP),1)
  $(error This makefile is for AVP only)
endif

MODULE_NAME := libnvmm_avpentry
EXPORT_FILES := $(MODULE_NAME).export

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/transport
LCINCS += -I../../include

C_FILES :=
C_FILES += nvmm_block_avpentry.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(TARGETS): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def

include $(TEGRA_TOP)/core-private/make/Makefile.rules

