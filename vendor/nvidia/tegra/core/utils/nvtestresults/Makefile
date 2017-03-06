TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvtestresults

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
LIB_HOST_STATIC := $(OUTDIR)/$(MODULE_NAME)_host_static$(LIB_SUFFIX)
TARGETS := $(LIB)

TO_BUILD := $(LIB) $(LIB_HOST_STATIC)

# Build the host static lib only on the host
ifeq ($(HOST_BUILD),0)
  ifneq ($(OUTDIR),$(HOST_OUTDIR))
    TO_BUILD := $(LIB)
  endif
endif

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvboot/include
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvboot/ap15/include

C_FILES :=
C_FILES += nvresults.c

EXPORT_FILES := $(MODULE_NAME).export

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(TO_BUILD)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)
	$(DO_HOST_BUILD_TOO)

DEPS :=
DEPS += $(OBJS)
DEPS += $(OUTDIR)/$(MODULE_NAME).def
ifeq ($(NV_DEF_USE_DLL),1)
  DEPS += $(NVLIB_NVOS)
  DEPS += $(NVLIB_TESTIO)
endif
$(LIB): $(DEPS)
$(LIB_HOST_STATIC): $(OBJS)

RELEASE_PACKAGE_TARGETS := $(TARGETS)
RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
