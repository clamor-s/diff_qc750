HOST_BUILD := 1

TOPDIR := ../../../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvboothost

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
LIB_HOST_STATIC := $(OUTDIR)/$(MODULE_NAME)_host_static$(LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

TO_BUILD := $(LIB) $(LIB_HOST_STATIC)

# Build the host static lib only on the host
ifeq ($(HOST_BUILD),0)
  ifneq ($(OUTDIR),$(HOST_OUTDIR))
    TO_BUILD := $(LIB)
  endif
endif

NVBOOT_COMMON_INCLUDES :=
NVBOOT_COMMON_INCLUDES += -I$(TEGRA_TOP)/core/drivers/nvboot/ap15/include
NVBOOT_COMMON_INCLUDES += -I$(TEGRA_TOP)/core/drivers/nvboot/include

LCINCS += $(NVBOOT_COMMON_INCLUDES)
LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/utils/aes_ref

C_FILES :=
C_FILES += nvbh_rcm.c
C_FILES += nvbh_crypto.c

EXPORT_FILES := $(MODULE_NAME).export

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(TO_BUILD)
	$(DO_HOST_BUILD_TOO)
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)

$(TO_BUILD): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def

ifeq ($(NV_DEF_USE_DLL),1)
$(TARGETS): $(NVLIB_NVOS)
$(TARGETS): $(NVLIB_APPUTIL)
$(TARGETS): $(NVLIB_NVAES_REF)
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules
