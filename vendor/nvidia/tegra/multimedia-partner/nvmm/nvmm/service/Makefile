TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs


LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I../include

LCDEFS += -DNV_IS_AVP=$(IS_AVP)

ifeq ($(IS_AVP),1)
  C_FILES :=
  C_FILES += nvmm_service_avp.c
  SUBDIRS += nvmm_service_swi

  APP_NAME := nvmm_service
  OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

  MY_LIBS :=
  MY_LIBS += $(NVLIB_NVRM)
  MY_LIBS += $(NVLIB_NVOS)
  #LCINCS += -I../../include
  LCINCS += -I../include

  LINKOPTS := --entry AxfModuleMain --map --list=$(OUTPUT_DIRNAME)/$(APP_NAME).map --info sizes,totals,common,unused --symbols
  EXECUTABLES := $(OUTPUT_DIRNAME)/$(APP_NAME)$(EXE_SUFFIX)

  default: $(EXECUTABLES)
	$(NV_INSTALL) -l $(EXECUTABLES) $(INSTALL_TARGET)

  $(EXECUTABLES): $(OBJS)
  $(EXECUTABLES): $(MY_LIBS)
  $(EXECUTABLES): nvmm_service.ldscript
else
  MODULE_NAME := libnvmm_service
  EXPORT_FILES := $(MODULE_NAME).export

  LIB := $(OUTPUT_DIRNAME)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
  TARGETS := $(addprefix $(OUTPUT_DIRNAME)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

  ifeq ($(TARGET_OS),winxp)
    SYSTEM_LDLIBS_DLL += winmm.lib
  endif

  C_FILES :=
  C_FILES += nvmm_service.c

  OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

  default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

  $(TARGETS): $(OBJS)
  $(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
  ifeq ($(NV_DEF_USE_DLL),1)
    $(TARGETS): $(NVLIB_NVOS)
    $(TARGETS): $(NVLIB_NVRM_KERNEL)
    $(TARGETS): $(NVLIB_NVRM_GRAPHICS_IMPL)
    $(TARGETS): $(NVLIB_NVMM_MANAGER)
  endif

  RELEASE_PACKAGE_TARGETS := $(TARGETS)
  RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules

