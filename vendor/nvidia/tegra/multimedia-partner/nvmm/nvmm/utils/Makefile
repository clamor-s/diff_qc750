TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs


LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I../include
LCINCS += -I$(TEGRA_TOP)/graphics/2d/include
LCDEFS += -DNV_IS_AVP=$(IS_AVP)

MODULE_NAME := libnvmm_utils

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
C_FILES += nvmm_ulp_util.c
C_FILES += nvmm_ulp_kpi_logger.c
C_FILES += nvmm_bufmgr.c
C_FILES += nvmm_queue.c
C_FILES += nvmm_mediaclock.c
C_FILES += nvmm_sock_util.c
C_FILES += nvmm_file_util.c
C_FILES += nvmm_format_scan.c
C_FILES += nvmetafile_m3uparser.c
C_FILES += nvmetafile_asxparser.c
C_FILES += nvmetafile_parser.c
C_FILES += nvmm_logger.c
C_FILES += nvmm_logger_default.c

ifeq ($(TARGET_OS),linux)
  C_FILES += nvmm_sock_linux.c
  LCDEFS += -DNV_USE_NVAVP=1
else
  LCDEFS += -DNV_USE_NVAVP=0
  ifeq ($(TARGET_OS_WINDOWS),1)
    C_FILES += nvmm_sock_win.c
  else
    C_FILES += nvmm_sock_null.c
  endif
endif

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
	$(NV_INSTALL) -l metafileformatlibs.xml $(INSTALL_TARGET)
$(TARGETS): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
ifeq ($(NV_DEF_USE_DLL),1)
  $(TARGETS): $(NVLIB_NVMM_SERVICE)
  $(TARGETS): $(NVLIB_NVOS)
  $(TARGETS): $(NVLIB_NVRM)
  $(TARGETS): $(NVLIB_NVAVP)
endif

RELEASE_PACKAGE_TARGETS := $(TARGETS)
RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
