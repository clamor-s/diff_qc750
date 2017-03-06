TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs


LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I../include
LCDEFS += -DNV_IS_AVP=0

MODULE_NAME := libnvmmlite_utils

EXPORT_FILES := $(MODULE_NAME).export

LIB := $(OUTPUT_DIRNAME)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTPUT_DIRNAME)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

ifeq ($(TARGET_OS),winxp)
  SYSTEM_LDLIBS_DLL += winmm.lib
endif

ifeq ($(TARGET_OS),winxp)
  SYSTEM_LDLIBS_DLL += ws2_32.lib
  SYSTEM_LDLIBS_DLL += urlmon.lib
endif

C_FILES :=
C_FILES += nvmm_util.c
C_FILES += nvmm_block.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

ifeq ($(WIN_INTERFACE),x11)
  LCINCS += -I$(NVLIB_X11_ROOT)/include
  LCINCS += $(NVLIB_X11_TDR_INCLUDES)
  SYSTEM_LDLIBS_DLL += -L$(NVLIB_X11_ROOT)/lib -lX11 -lXext
  OBJS += $(NVLIB_X11_TDR)
  LCDEFS += -DHAVE_X11
endif

default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)
$(TARGETS): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
ifeq ($(NV_DEF_USE_DLL),1)
  $(TARGETS): $(NVLIB_NVOS)
  $(TARGETS): $(NVLIB_NVRM)
  $(TARGETS): $(NVLIB_NVMM_UTILS)
endif

RELEASE_PACKAGE_TARGETS := $(TARGETS)
RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
