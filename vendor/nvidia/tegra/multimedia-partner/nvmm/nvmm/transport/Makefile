TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

include $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/Makefile.nvmm_enable_defs
include $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/Makefile.nvmm_locale_defs

LIB := $(OUTPUT_DIRNAME)/libnvmmtransport$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I../include

ifneq (,$(filter -Wstrict-aliasing,$(WARN)))
  WARN += -Wno-error=strict-aliasing
endif

C_FILES :=
ifeq ($(IS_AVP),1)
  C_FILES += nvmm_transport_block.c
  C_FILES += nvmm_stats.c
else
  C_FILES += nvmm_transport_block.c
  C_FILES += nvmm_transport_client.c
  C_FILES += nvmm_stats.c
  C_FILES += nvmm_contextmanager.c
endif

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

$(LIB): $(OBJS)

ifeq ($(NV_DEF_USE_DLL),0)
  RELEASE_PACKAGE_TARGETS := $(LIB)
  RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)
else
  RELEASE_PACKAGE_TARGETS :=
  RELEASE_PACKAGE_FILES :=
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules
