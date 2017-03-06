TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

include $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/Makefile.nvmm_enable_defs
include $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/Makefile.nvmm_locale_defs

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I../include
LCINCS += -I$(TEGRA_TOP)/multimedia/codecs/include


LCDEFS += -DNV_IS_AVP=$(IS_AVP)

C_FILES :=
C_FILES += nvmm_manager.c
C_FILES += list.c
C_FILES += nvmm_manager_tables.c

MODULE_NAME := libnvmm_manager
EXPORT_FILES := $(MODULE_NAME).export

LIB := $(OUTPUT_DIRNAME)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTPUT_DIRNAME)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -Iinclude

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(TARGETS): $(OBJS)

ifeq ($(NV_DEF_USE_DLL),1)
  $(TARGETS): $(NVLIB_NVOS)
  $(TARGETS): $(NVLIB_NVRM_KERNEL)
  $(TARGETS): $(NVLIB_NVAVP)
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules

