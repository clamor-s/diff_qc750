TOPDIR := ../..
export FORCE_RVDS := 1

include $(TEGRA_TOP)/core-private/make/Makefile.defs
include $(TEGRA_TOP)/core-private/make/Makefile.nvlibdefs

MODULE_NAME := libnvsku
ifneq ($(TARGET_OS),rvds)
 LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)
else
 LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
 TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))
endif

LIB_HOST_STATIC := $(OUTDIR)/$(MODULE_NAME)_host_static$(LIB_SUFFIX)
TO_BUILD := $(LIB) $(LIB_HOST_STATIC)

# Build the host static lib only on the host
ifeq ($(HOST_BUILD),0)
  ifneq ($(OUTDIR),$(HOST_OUTDIR))
    TO_BUILD := $(LIB)
  endif
else
  LCDEFS += -DNVODM_BOARD_IS_SIMULATION=1
endif
#LCDEFS += -DNV_ENABLE_DEBUG_PRINTS=1

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/system/nvbct

# Tell the compiler to use the statically linked C-runtime libraries
# instead of DLLs.
ifeq ($(TARGET_OS),wince)
  WINCE_USE_STATIC_RUNTIME_LIBS := 1
  LCDEFS += -DNV_IS_AOS=0
else
  LCDEFS += -DNV_IS_AOS=1
endif
ifneq ($(TARGET_OS),rvds)
  LCDEFS += -DNV_USE_FUSE_CLOCK_ENABLE=0
endif

LCDEFS += -DNV_IS_AVP=$(IS_AVP)

C_FILES :=
ifeq ($(TARGET_SOC),ap20)
C_FILES += nvsku_boot_ap20.c
C_FILES += nvsku_burn_ap20.c
else
C_FILES += nvsku_boot_t30.c
C_FILES += nvsku_burn_t30.c
endif

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

# FIXME: gcc (at least version 4.3.3) space-optimized code (default for EABI)
# doesn't link correctly against the assembly sanitize routines
ifeq ($(BUILD_FLAVOR)-$(USE_GCC_TOOLCHAIN),release-1)
CFLAGS += -O2
endif

default: $(TO_BUILD)
	$(DO_HOST_BUILD_TOO)
ifeq ($(TARGET_OS),rvds)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)
else
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)
endif

ifneq ($(TARGET_OS),rvds)
  $(LIB): $(OBJS)
endif

$(LIB_HOST_STATIC): $(OBJS)

DEPS :=
DEPS += $(OBJS)
DEPS += $(NVLIB_NVOS)

nosupport:
	@echo "Sku is not supported for " $(TARGET_OS) " " $(TARGET_CPU)

RELEASE_PACKAGE_TARGETS := $(LIB)
RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
