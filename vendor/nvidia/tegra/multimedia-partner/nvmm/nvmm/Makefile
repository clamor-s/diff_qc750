TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

include $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/Makefile.nvmm_enable_defs
include $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/Makefile.nvmm_locale_defs

ifneq ($(IS_AVP),1)
  SUBDIRS += utils
  SUBDIRS += contentpipe
  SUBDIRS += manager
endif

SUBDIRS += transport
SUBDIRS += service

ifeq ($(IS_AVP),1)
  SUBDIRS += transport/avp
  SUBDIRS += manager/avp
endif

SUBDIRS += blocks

ifneq ($(NON_OS_BUILD),1)
SUBDIRS += $(TEGRA_TOP)/camera/core
endif
SUBDIRS += $(TEGRA_TOP)/multimedia/codecs
SUBDIRS += $(TEGRA_TOP)/multimedia-partner/codecs

MODULE_NAME := libnvmm

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

EXPORT_FILES := $(MODULE_NAME).export

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -Iinclude

ifeq ($(TARGET_OS),winxp)
  SYSTEM_LDLIBS_DLL += winmm.lib
endif

C_FILES :=
ifneq ($(IS_AVP),1)
  C_FILES += nvmm.c
endif

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default:
	$(MAKE) $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(TARGETS): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
$(TARGETS): $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/transport/$(OUTDIR)/libnvmmtransport$(LIB_SUFFIX)

# For AOS/Linux link all blocks. (Other OS uses dynamic loading of DLL's)
ifneq ($(IS_AVP),1)
  ifneq ($(NVMM_IS_DYNAMIC),1)
    $(TARGETS): $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/blocks/common/$(OUTPUT_DIRNAME)/libnvmmcommon$(LIB_SUFFIX)
    $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/audio_common/utils/$(OUTDIR)/libnvaudioutils$(LIB_SUFFIX)
    $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/audio_common/mem_utils/$(OUTDIR)/libnvaudio_memutils$(LIB_SUFFIX)
    $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/audio_common/ratecontrol/$(OUTDIR)/libnvaudio_ratecontrol$(LIB_SUFFIX)
    $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/audio_common/nvmm_audio_power/$(OUTDIR)/libnvaudio_power$(LIB_SUFFIX)
    $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/videodec_common/$(OUTDIR)/libnvvideodec$(LIB_SUFFIX)
    $(TARGETS): $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/blocks/writer_common/$(OUTDIR)/libnvbasewriter$(LIB_SUFFIX)

    ifeq ($(3GPWRITER_ENABLED),1)
      ifeq ($(3GPWRITER_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/blocks/3gp_writer/$(OUTDIR)/libnv3gpwriter$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECJPEG_ENABLED),1)
      ifeq ($(DECJPEG_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/jpeg_dec/$(OUTDIR)/libnvjpegdec$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECSUPERJPEG_ENABLED),1)
      ifneq ($(DECSUPERJPEG_LOCALE),$(LOCALE_AVP))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/super_jpeg_dec/$(OUTDIR)/libnvsuperjpegdec$(LIB_SUFFIX)
      endif
    endif    
    ifeq ($(DECJPEGPROGRESSIVE_ENABLED),1)
      ifneq ($(DECJPEGPROGRESSIVE_LOCALE),$(LOCALE_AVP))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/jpeg_progressive_dec/$(OUTDIR)/libnvjpegprogressivedec$(LIB_SUFFIX)
      endif
    endif    
    ifeq ($(DECMPEG4_ENABLED),1)
      ifneq ($(DECMPEG4_LOCALE),$(LOCALE_AVP))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/mpeg4_dec/$(OUTDIR)/libnvmpeg4dec$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECMP2_ENABLED),1)
      ifneq ($(DECMP2_LOCALE),$(LOCALE_AVP))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/mp2_dec/$(OUTDIR)/libnvmp2dec$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECMP3_ENABLED),1)
      ifneq ($(DECMP3_LOCALE),$(LOCALE_AVP))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/mp3_dec/$(OUTDIR)/libnvmp3dec$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECMP3SW_ENABLED),1)
      ifneq ($(DECMP3SW_LOCALE),$(LOCALE_AVP))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/mp3_dec_sw/$(OUTDIR)/libnvmp3decsw$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECH264_ENABLED),1)
      ifneq ($(DECH264_LOCALE),$(LOCALE_AVP))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/h264_dec/$(OUTDIR)/libnvh264dec$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECH264_2X_ENABLED),1)
      ifneq ($(DECH264_2X_LOCALE),$(LOCALE_AVP))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/h264_dec_2x/$(OUTDIR)/libnvh264dec2x$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECMPEG2_ENABLED),1)
      ifneq ($(DECMPEG2_LOCALE),$(LOCALE_AVP))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/mpeg2_dec/$(OUTDIR)/libnvmpeg2dec$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECVC1_ENABLED),1)
      ifeq ($(DECVC1_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/vc1_dec/$(OUTDIR)/libnvvc1dec$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECVC1_2X_ENABLED),1)
      ifeq ($(DECVC1_2X_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/vc1_dec_2x/$(OUTDIR)/libnvvc1dec2x$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECAAC_ENABLED),1)
      ifneq ($(DECAAC_LOCALE),$(LOCALE_AVP))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/aac_dec/$(OUTDIR)/libaacdec$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECAACPLUS_ENABLED),1)
      ifeq ($(DECAACPLUS_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/aac_dec/$(OUTDIR)/libaacdec$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECADTS_ENABLED),1)
      ifeq ($(DECADTS_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/adts_dec/$(OUTDIR)/libadtsdec$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECWMA_ENABLED),1)
      ifneq ($(DECWMA_LOCALE),$(LOCALE_AVP))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/wma9std_dec/$(OUTDIR)/libnvwma9std_dec$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECWMAPRO_ENABLED),1)
      ifneq ($(DECWMAPRO_LOCALE),$(LOCALE_AVP))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/wma/$(OUTDIR)/libnvwma$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECWMALSL_ENABLED),1)
      ifeq ($(DECWMALSL_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/wma9_lsl/$(OUTDIR)/libnvwmalsl$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECWMASUPER_ENABLED),1)
      ifeq ($(DECWMASUPER_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/wma_super_dec/$(OUTDIR)/libnvwmasuperdec$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECSUPERAUDIO_ENABLED),1)
      ifeq ($(DECSUPERAUDIO_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/super_audio_dec/$(OUTDIR)/libnvsuperaudiodec$(LIB_SUFFIX)
      endif
    endif    
    ifeq ($(DECWAV_ENABLED),1)
      ifneq ($(DECWAV_LOCALE),$(LOCALE_AVP))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/wav_dec/$(OUTDIR)/libnvwavdec$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECOGG_ENABLED),1)
      ifeq ($(DECOGG_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/vorbis_dec/$(OUTDIR)/libnvoggdec$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECBSAC_ENABLED),1)
      ifeq ($(DECBSAC_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/bsac_dec/$(OUTDIR)/libnvbsacdec$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECAMRWB_ENABLED),1)
      ifeq ($(DECAMRWB_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/amr_wb/decoder/$(OUTDIR)/libnvamrwbdec$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECILBC_ENABLED),1)
      ifeq ($(DECILBC_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/ilbc/decoder/$(OUTDIR)/libnvilbcdec$(LIB_SUFFIX)
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/ilbc/common/$(OUTDIR)/libnvilbccommon$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECAMRNB_ENABLED),1)
      ifeq ($(DECAMRNB_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/amrnb/decoder/$(OUTDIR)/libnvamrnbdec$(LIB_SUFFIX)
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/amrnb/common/$(OUTDIR)/libnvamrnbcommon$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECMPEG2VIDEOVLD_ENABLED),1)
      ifeq ($(DECMPEG2VIDEOVLD_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/mpeg2vdec_vld/$(OUTDIR)/libmpeg2vdec_vld$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(DECMPEG2VIDEORECON_ENABLED),1)
      ifeq ($(DECMPEG2VIDEORECON_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/mpeg2vdec_recon/$(OUTDIR)/libnvmpeg2vdec_recon$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(ENCAMRWB_ENABLED),1)
      ifeq ($(ENCAMRWB_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/amr_wb/encoder/$(OUTDIR)/libnvamrwbenc$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(ENCAMRNB_ENABLED),1)
      ifeq ($(ENCAMRNB_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/amrnb/encoder/$(OUTDIR)/libnvamrnbenc$(LIB_SUFFIX)
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/amrnb/common/$(OUTDIR)/libnvamrnbcommon$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(ENCAAC_ENABLED),1)
      ifeq ($(ENCAAC_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/aac_enc/$(OUTDIR)/libnvaacplusenc$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(ENCWAV_ENABLED),1)
          ifeq ($(ENCWAV_LOCALE),$(LOCALE_CPU))
            $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/wav_enc/$(OUTDIR)/libnvwavenc$(LIB_SUFFIX)
          endif
    endif
    ifeq ($(ENCILBC_ENABLED),1)
      ifeq ($(ENCILBC_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/ilbc/encoder/$(OUTDIR)/libnvilbcenc$(LIB_SUFFIX)
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/ilbc/common/$(OUTDIR)/libnvilbccommon$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(ENCAACPLUS_ENABLED),1)
      ifeq ($(ENCAACPLUS_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/aac_enc/$(OUTDIR)/libnvaacplusenc$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(ENCEAACPLUS_ENABLED),1)
      ifeq ($(ENCEAACPLUS_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/aac_enc/$(OUTDIR)/libnvaacplusenc$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(AUDIOMIXER_ENABLED),1)
      ifeq ($(AUDIOMIXER_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia/codecs/mixer/$(OUTDIR)/libnvmixer$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(SUPERPARSER_ENABLED),1)
      ifeq ($(SUPERPARSER_LOCALE),$(LOCALE_CPU))
        $(TARGETS): $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/blocks/super_parser/$(OUTDIR)/libnv_parser$(LIB_SUFFIX)
      endif
    endif
    ifeq ($(FLOATING_POINT_SUPPORTED),1)
      ifeq ($(SRCCAMERA_ENABLED),1)
        ifeq ($(SRCCAMERA_LOCALE),$(LOCALE_CPU))
          ifneq ($(NON_OS_BUILD),1)
          $(TARGETS): $(TEGRA_TOP)/camera/core/camera/$(OUTDIR)/libnvcamera$(LIB_SUFFIX)
          endif
        endif
      endif
      ifeq ($(DIGITALZOOM_ENABLED),1)
        ifeq ($(DIGITALZOOM_LOCALE),$(LOCALE_CPU))
          ifneq ($(NON_OS_BUILD),1)
          $(TARGETS): $(TEGRA_TOP)/camera/core/digitalzoom/$(OUTDIR)/libnvdigitalzoom$(LIB_SUFFIX)
          endif
        endif
      endif
      ifeq ($(NV_DEF_USE_DLL),1)
        $(TARGETS): $(NVLIB_NVODM_SENSOR)
        $(TARGETS): $(NVLIB_NVDDK_2D)
        $(TARGETS): $(NVLIB_NVDDK_2D_V2)
        $(TARGETS): $(NVLIB_FXMATH)
        $(TARGETS): $(NVLIB_NVSM)
      endif
    endif
    ifeq ($(NV_DEF_USE_DLL),1)
      $(TARGETS): $(NVLIB_NVODM_QUERY)
      $(TARGETS): $(NVLIB_NVODM_AUDIOCODEC)
      $(TARGETS): $(NVLIB_NVODM_SERVICES)
    endif
  endif
endif

ifeq ($(NV_DEF_USE_DLL),1)
  $(TARGETS): $(NVLIB_NVOS)
  $(TARGETS): $(NVLIB_NVRM)
  $(TARGETS): $(NVLIB_NVMM_SERVICE)
  $(TARGETS): $(NVLIB_NVMM_MANAGER)
  $(TARGETS): $(NVLIB_NVMM_UTILS)    
  $(TARGETS): $(NVLIB_NVMM_CONTENTPIPE)       
endif

RELEASE_PACKAGE_TARGETS := $(TARGETS)
RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
