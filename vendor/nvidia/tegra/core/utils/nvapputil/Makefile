TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvapputil

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
LIB_HOST_STATIC := $(OUTDIR)/$(MODULE_NAME)_host_static$(LIB_SUFFIX)

TO_BUILD := $(LIB) $(LIB_HOST_STATIC)
TO_INSTALL := $(LIB)

# Build the host static lib only on the host
ifeq ($(HOST_BUILD),0)
  ifneq ($(OUTDIR),$(HOST_OUTDIR))
    TO_BUILD := $(LIB)
  endif
endif

LCINCS += $(NVLIB_COMMON_INCLUDES)

ifeq ($(TARGET_CPU),armv4)
  LCDEFS += -DNV_DEF_USE_FLOAT=0
else
  LCDEFS += -DNV_DEF_USE_FLOAT=1
endif

PROC_IMPL :=
ifeq ($(TARGET_OS_WINDOWS),1)
  PROC_IMPL := nvauproc_win32.c
endif
ifeq ($(TARGET_OS),linux)
  PROC_IMPL := nvauproc_linux.c
endif
ifeq ($(PROC_IMPL),)
  PROC_IMPL := nvauproc_null.c
endif

C_FILES :=
C_FILES += nvaurand.c
C_FILES += nvaugetopt.c
C_FILES += nvaureadline.c
C_FILES += nvauerr.c
C_FILES += nvauprintf.c
C_FILES += nvau_hashtable.c
C_FILES += nvau_token.c
C_FILES += $(PROC_IMPL)

EXPORT_FILES := $(MODULE_NAME).export

# OBJECT files 
#######################################
OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

# Build Rules
#######################################
default: $(TO_BUILD)
	$(DO_HOST_BUILD_TOO)
	$(NV_INSTALL) -l $(TO_INSTALL) $(INSTALL_TARGET)

DEPS :=
DEPS += $(OBJS)

$(LIB_HOST_STATIC): $(DEPS)

ifeq ($(NV_DEF_USE_DLL),1)
  DEPS += $(OUTDIR)/$(MODULE_NAME).def
  DEPS += $(NVLIB_NVOS)
endif
$(LIB): $(DEPS)

RELEASE_PACKAGE_TARGETS := $(LIB)
RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
