TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

include $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/Makefile.nvmm_enable_defs

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I../include

MODULE_NAME := libnvmm_contentpipe
EXPORT_FILES := $(MODULE_NAME).export

LIB := $(OUTPUT_DIRNAME)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTPUT_DIRNAME)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

C_FILES :=
C_FILES += nvmm_contentpipe.c
C_FILES += nvlocalfilecontentpipe.c
C_FILES += nvmm_customprotocol.c
C_FILES += nvmm_protocol_file.c
C_FILES += nvmm_protocol_http.c
C_FILES += nvmm_protocol_rtsp.c
C_FILES += rtsp.c
C_FILES += rtp.c
C_FILES += rtp_audio.c
C_FILES += rtp_video.c
C_FILES += rtp_video_h264.c
C_FILES += rtp_latm.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

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


