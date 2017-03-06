# Note: This code should be used on the host only.
#       On-chip code should use the HW AES engines.
#       Management approval is required for exceptions.

HOST_BUILD := 1

TOPDIR := ../..

include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvaes_ref

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

LCINCS += $(NVLIB_COMMON_INCLUDES)

C_FILES :=
C_FILES += aes_ref.c


EXPORT_FILES := $(MODULE_NAME).export

# Determine object file names
OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(TO_BUILD)
	$(DO_HOST_BUILD_TOO)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(TO_BUILD): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
ifeq ($(NV_DEF_USE_DLL),1)
$(TARGETS): $(NVLIB_NVOS)
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules
