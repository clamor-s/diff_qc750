TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/../core-private/make/Makefile.defs


MODULE_NAME := libnvodm_hdmi

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

LCINCS += $(NVLIB_COMMON_INCLUDES)

EXPORT_FILES := $(MODULE_NAME).export

C_PUB_FILES :=
C_PUB_FILES += nvodm_hdmi.c
C_FILES := $(sort $(C_PUB_FILES))

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_PUB_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(PRIV_LIB) $(NVODMLIB_INSTALL_TARGET)

$(TARGETS): $(OBJS)

$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
ifeq ($(NV_DEF_USE_DLL),1)
  $(TARGETS): $(NVLIB_NVOS)
  $(TARGETS): $(NVLIB_NVODM_SERVICES)
  $(TARGETS): $(NVLIB_NVRM_DAEMON)
  $(TARGETS): $(NVLIB_NVODM_QUERY)
endif

include $(TOPDIR)/../core-private/make/Makefile.rules
