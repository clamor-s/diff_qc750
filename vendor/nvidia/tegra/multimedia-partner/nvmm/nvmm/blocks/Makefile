# XXX This component needs the whole-archive linker stuff.  Please fix and
# remove this -- this prevents the linker from doing a lot of optimizations.
NEEDS_WHOLE_ARCHIVE := 1

TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

include $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/Makefile.nvmm_enable_defs
include $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/Makefile.nvmm_locale_defs

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I../../../include
LCINCS += -I../../include
LCINCS += -I../include
LCINCS += -Iinclude

SUBDIRS += common

#Following code doesnt build for AVP, fix if any block needs this
ifneq ($(IS_AVP),1)
  SUBDIRS += writer_common
  SUBDIRS += super_parser
endif

#Find what blocks are built, based on enable/disable and locale (for  CPU)
3GPWRITER_BUILT := 0

ifeq ($(IS_AVP),1)
  ifeq ($(3GPWRITER_ENABLED),1)
      ifeq ($(3GPWRITER_LOCALE),$(LOCALE_CPU))
        SUBDIRS += 3gp_writer
        3GPWRITER_BUILT := 1
      endif
  endif
else
  ifeq ($(3GPWRITER_ENABLED),1)
      ifeq ($(3GPWRITER_LOCALE),$(LOCALE_CPU))
        SUBDIRS += 3gp_writer
        3GPWRITER_BUILT := 1
      endif
  endif
endif

ifeq ($(NVMM_BUILT_DYNAMIC),1)
  C_FILES :=
  C_FILES += common/nvmm_block.c

  OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

  LIB_PARSER := $(OUTPUT_DIRNAME)/libnvmm_parser$(DLL_OR_LIB_SUFFIX)
  TARGETS_PARSER := $(addprefix $(OUTDIR)/libnvmm_parser,$(DLL_OR_LIB_SUFFIXES))

  LIB_WRITER := $(OUTPUT_DIRNAME)/libnvmm_writer$(DLL_OR_LIB_SUFFIX)
  TARGETS_WRITER := $(addprefix $(OUTDIR)/libnvmm_writer,$(DLL_OR_LIB_SUFFIXES))

  EXPORT_FILES :=
  EXPORT_FILES += libnvmm_parser.export
  EXPORT_FILES += libnvmm_writer.export

  RELEASE_PACKAGE_TARGETS := $(TARGETS_PARSER) $(TARGETS_WRITER) $(TARGETS_MISC)
  RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)

  ifeq ($(TARGET_OS),winxp)
    SYSTEM_LDLIBS_DLL += winmm.lib
  endif

  default:
	$(MAKE) $(LIB_PARSER) $(LIB_WRITER)
	$(NV_INSTALL) -l $(TARGETS_PARSER) $(TARGETS_WRITER) $(INSTALL_TARGET)

  $(TARGETS_PARSER): $(OUTPUT_DIRNAME)/libnvmm_parser.def
  $(TARGETS_PARSER): $(OBJS)
  ifeq ($(NV_DEF_USE_DLL),1)
    $(TARGETS_PARSER): $(NVLIB_NVMM_UTILS)
    $(TARGETS_PARSER): $(NVLIB_NVOS)
    $(TARGETS_PARSER): $(NVLIB_NVRM)
    $(TARGETS_PARSER): $(NVLIB_NVMM_CONTENTPIPE)
  endif

  $(TARGETS_PARSER): $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/blocks/super_parser/$(OUTDIR)/libnv_parser$(LIB_SUFFIX)

  $(TARGETS_WRITER): $(OUTPUT_DIRNAME)/libnvmm_writer.def
  $(TARGETS_WRITER): $(OBJS)
  ifeq ($(NV_DEF_USE_DLL),1)
    $(TARGETS_WRITER): $(NVLIB_NVMM_UTILS)
    $(TARGETS_WRITER): $(NVLIB_NVOS)
    $(TARGETS_WRITER): $(NVLIB_NVRM)
    $(TARGETS_WRITER): $(NVLIB_NVMM_CONTENTPIPE)
  endif

  $(TARGETS_WRITER): $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/blocks/writer_common/$(OUTDIR)/libnvbasewriter$(LIB_SUFFIX)
  ifeq ($(3GPWRITER_BUILT),1)
    $(TARGETS_WRITER): $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/blocks/3gp_writer/$(OUTDIR)/libnv3gpwriter$(LIB_SUFFIX)
  endif

else   # other operating systems which statically link blocks directly
  RELEASE_PACKAGE_TARGETS :=
  RELEASE_PACKAGE_FILES :=
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules
