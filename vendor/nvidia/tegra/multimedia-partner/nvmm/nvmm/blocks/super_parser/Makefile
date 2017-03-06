TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

include $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/Makefile.nvmm_enable_defs

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I../../../include
LCINCS += -I../../include
LCINCS += -I../include
LCINCS += -I../../../../include/openmax/il
LCINCS += -Iinclude

LCINCS += -I../../../../../avi
LCINCS += -I../../../../../mp3
LCINCS += -I../../../../../mp4
LCINCS += -I../mp2_dec/include

LIB := $(OUTPUT_DIRNAME)/libnv_parser$(LIB_SUFFIX)

C_FILES := nvmm_super_parserblock.c

C_FILES += parser_core/mp3/nvmm_mp3parser_core.c
C_FILES += parser_core/mp3/nvmm_mp3parser.c

C_FILES += parser_core/mp4/nvmm_mp4parser_core.c
C_FILES += parser_core/mp4/nv_mp4parser.c

C_FILES += parser_core/ogg/nvmm_oggparser_core.c
C_FILES += parser_core/ogg/nvmm_oggparser.c
C_FILES += parser_core/ogg/nvmm_oggbookunpack.c
C_FILES += parser_core/ogg/nvmm_oggframing.c
C_FILES += parser_core/ogg/nvmm_ogginfo.c
C_FILES += parser_core/ogg/nvmm_oggwrapper.c
C_FILES += parser_core/ogg/nvmm_oggvorbisfile.c
C_FILES += parser_core/ogg/nvmm_oggseek.c

C_FILES += parser_core/aac/nvmm_aacparser_core.c
C_FILES += parser_core/aac/nvmm_aacparser.c

C_FILES += parser_core/amr/nvmm_amrparser_core.c
C_FILES += parser_core/amr/nvmm_amrparser.c

C_FILES += parser_core/m2v/nvmm_m2vparser_core.c
C_FILES += parser_core/m2v/nvmm_m2vparser.c

C_FILES += parser_core/mps/nvmm_mpsparser_core.c
C_FILES += parser_core/mps/nvmm_mps_parser.c
C_FILES += parser_core/mps/nvmm_mps_reader.c

C_FILES += parser_core/nem/nvmm_nemparser_core.c

C_FILES += parser_core/wav/nvmm_wavparser_core.c
C_FILES += parser_core/wav/nvmm_wavparser.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

$(LIB): $(OBJS)

ifeq ($(NV_DEF_USE_DLL),0)
  RELEASE_PACKAGE_TARGETS := $(LIB)
  RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)
else
  RELEASE_PACKAGE_TARGETS :=
  RELEASE_PACKAGE_FILES :=
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules

