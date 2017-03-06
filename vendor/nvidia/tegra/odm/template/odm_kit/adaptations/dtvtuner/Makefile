TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/../core-private/make/Makefile.defs


MODULE_NAME := libnvodm_dtvtuner

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)

EXPORT_FILES := $(MODULE_NAME).export

C_FILES :=
C_FILES += dtvtuner_hal.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(NVODMLIB_INSTALL_TARGET)

$(TARGETS): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
ifeq ($(NV_DEF_USE_DLL),1)
  $(TARGETS): $(NVLIB_NVRM)
  $(TARGETS): $(NVLIB_NVOS)
  $(TARGETS): $(NVLIB_NVODM_SERVICES)
  $(TARGETS): $(NVLIB_NVODM_QUERY)
endif

include $(TOPDIR)/../core-private/make/Makefile.rules
