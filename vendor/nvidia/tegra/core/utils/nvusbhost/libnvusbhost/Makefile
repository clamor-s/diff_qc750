HOST_BUILD := 1

TOPDIR := ../../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvusbhost

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

ifeq ($(TARGET_OS),winxp)
  C_FILES := win32/nvusbhost.c

  LCINCS += -I$(NV_WIN32_DDK_DIR)/inc/crt
  LCINCS += -I$(NV_WIN32_DDK_DIR)/inc/ddk
  LCINCS += -I$(NV_WIN32_DDK_DIR)/inc/api

  LDFLAGS_DLL += -LIBPATH:$(NV_WIN32_DDK_LIB_DIR)

  SYSTEM_LDLIBS_DLL += setupapi.lib
  SYSTEM_LDLIBS_DLL += strsafe.lib
endif
ifeq ($(TARGET_OS),linux)
  C_FILES := linux/nvusbhost_linux.c
  LCINCS += -I/usr/include
endif
ifeq ($(TARGET_OS),darwin)
  C_FILES := darwin/nvusbhost_darwin.c
  LCINCS += -I/usr/include
endif

EXPORT_FILES := $(MODULE_NAME).export

# Determine object file names
OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(TO_BUILD)
	$(DO_HOST_BUILD_TOO)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(TO_BUILD): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
ifeq ($(NV_DEF_USE_DLL),1)
  $(TARGETS): $(NVLIB_APPUTIL)
  $(TARGETS): $(NVLIB_NVOS)
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules

