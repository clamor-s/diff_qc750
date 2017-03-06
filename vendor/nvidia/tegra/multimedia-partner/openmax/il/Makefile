# Copyright (c) 2006-2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an
# express license agreement from NVIDIA Corporation is strictly prohibited.

TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

include ../Makefile.omxdefs

ifeq ($(NV_EMBEDDED_BUILD),1)
 LCDEFS  += -DNV_RECEIVE_HEADER_AFTER_ABORT
 LCDEFS  += -DNV_DEF_USE_PITCH_MODE
endif
LCDEFS  += -DUSE_FULL_FRAME_MODE=0

LCINCS += -I$(TEGRA_TOP)/core-private/drivers
LCINCS += -I$(TEGRA_TOP)/core/drivers
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/il
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/il/common
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/il/components
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/il/components/common
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/il/components/common
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/il/nvmm
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/il/nvmm/common
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/il/nvmm/components
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/il/nvmm/components/common
LCINCS += -I$(TEGRA_TOP)/graphics/egl/include/interface
LCINCS += -I$(TEGRA_TOP)/multimedia/tvmr/include

MODULE_NAME := libnvomx

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

LCDEFS += -DUSE_NVOS_AND_NVTYPES
ifeq ($(TARGET_BOARD),simulation)
LCDEFS += -DUSE_ANDROID_NATIVE_WINDOW=0
LCDEFS += -DNV_IS_AVP=$(IS_AVP)
endif

USE_TVMR :=0
USE_DC_DRIVER :=0
USE_ALSA :=0
# Enable DC, ALSA and TVMR components under all Linux (host side) only builds.
# Android builds are unaffected and do not use these components.
ifneq (,$(filter $(TARGET_CPU),armv6 armv7))
    ifeq ($(TARGET_OS),linux)
        USE_DC_DRIVER :=1
        USE_ALSA :=1
        USE_TVMR :=1
        LCDEFS += -DUSE_TVMR_OMX
    endif
endif

ifeq ($(USE_DC_DRIVER),1)
    LCDEFS += -DUSE_DC_DRIVER
endif

ifeq ($(USE_ALSA),1)
    LCINCS += -I$(TEGRA_TOP)/3rdparty/alsa/alsa-lib-1.0.18/include
endif

#ifeq ($(BUILD_FLAVOR),debug)
#  LCDEFS += -DNV_ENABLE_DEBUG_PRINTS=1
#endif

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_KHRONOS_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/camera/utils/camerautil
LCINCS += -I$(TEGRA_TOP)/graphics/egl/include
LCINCS += -I$(TEGRA_TOP)/graphics/2d/include

C_FILES :=
C_FILES += core/NvxCore.c
C_FILES += components/common/NvxComponent.c
C_FILES += components/common/NvxResourceManager.c
C_FILES += components/common/NvxPort.c
C_FILES += components/common/simplecomponentbase.c
C_FILES += components/NvxClockComponent.c
C_FILES += components/NvxMp4FileWriter.c
C_FILES += components/NvxVideoScheduler.c
C_FILES += components/NvxRawFileReader.c
C_FILES += components/NvxRawFileWriter.c
C_FILES += common/NvxTrace.c
C_FILES += common/NvxMutex.c
C_FILES += common/nvxjitter.c
C_FILES += common/nvxlist.c
C_FILES += components/common/NvxScheduler.c
C_FILES += nvmm/components/common/NvxCompReg.c
C_FILES += nvmm/components/common/nvmmtransformbase.c
C_FILES += nvmm/components/common/NvxCompMisc.c
C_FILES += nvmm/common/NvxHelpers.c
C_FILES += nvmm/components/nvxaudiodecoder.c
C_FILES += nvmm/components/nvxbypassdecoder.c
C_FILES += nvmm/components/nvxvideodecoder.c
ifeq ($(USE_TVMR),1)
   C_FILES += tvmr/components/nvxtvmrmixer.c
   C_FILES += tvmr/components/nvxtvmrcapture.c
   C_FILES += tvmr/components/nvxtvmrrender.c
endif
C_FILES += nvmm/components/nvxvideorenderer.c
C_FILES += nvmm/components/nvxloopbackrenderer.c
ifeq ($(USE_ALSA),1)
    C_FILES += components/nvxalsarenderer.c
    C_FILES += components/nvxalsacapture.c
else
    C_FILES += nvmm/components/nvxaudiorenderer.c
    C_FILES += nvmm/components/nvxaudiocapturer.c
endif

C_FILES += nvmm/components/nvxcamera.c
C_FILES += nvmm/components/nvximageencoder.c
C_FILES += nvmm/components/nvximagedecoder.c
C_FILES += nvmm/components/nvxaudioencoder.c
C_FILES += nvmm/components/nvxvideoextractor.c
C_FILES += nvmm/common/nvxcrccheck.c
C_FILES += nvmm/components/nvxparser.c
C_FILES += nvmm/common/nvxeglimage.c
C_FILES += nvmm/common/nvxegl.c
C_FILES += nvmm/components/nvxliteaudioencoder.c
C_FILES += nvmm/components/nvxliteimageencoder.c
C_FILES += nvmm/components/nvxliteaudiodecoder.c
C_FILES += nvmm/components/nvxlitevideodecoder.c
C_FILES += nvmm/components/nvxlitevideoencoder.c
C_FILES += nvmm/components/common/nvxlitechooser.c
C_FILES += nvmm/components/common/nvmmlitetransformbase.c
C_FILES += nvmm/components/common/nvmmlitevideo_profile_level.c
C_FILES += nvmm/components/common/nvxaudiocapability.c
C_FILES += nvmm/components/nvxliteimagedecoder.c

ifeq ($(USE_DC_DRIVER),1)
    C_FILES += nvmm/common/nvxhelpers_dc.c
endif

C_FILES += nvmm/common/nvxwinmanagerstub.c

ifeq ($(NV_DEF_USE_DLL),1)
  C_FILES += libomxil.c
endif

EXPORT_FILES := $(MODULE_NAME).export

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(TARGETS): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
ifeq ($(NV_DEF_USE_DLL),1)
  $(TARGETS): $(NVLIB_NVOS)
  $(TARGETS): $(NVLIB_NVRM)
  $(TARGETS): $(NVLIB_NVMM)
  $(TARGETS): $(NVLIB_NVMM_UTILS)
  $(TARGETS): $(NVLIB_NVDDK_2D_V2)
  $(TARGETS): $(NVLIB_NVODM_QUERY)
  $(TARGETS): $(NVLIB_FXMATH)
  $(TARGETS): $(NVLIB_MD5)
  $(TARGETS): $(NVLIB_NVODM_SENSOR)
  $(TARGETS): $(NVLIB_NVMM_UTILS)
  $(TARGETS): $(NVLIB_NVMM_CONTENTPIPE)
  $(TARGETS): $(NVLIB_WINSYS)
  $(TARGETS): $(NVLIB_NVMMLITE)
  $(TARGETS): $(NVLIB_NVMMLITE_UTILS)
ifeq ($(USE_DC_DRIVER),1)
  $(TARGETS): $(TEGRA_TOP)/core/drivers/libnvdc/$(OUTDIR)/libnvdc$(DLL_SUFFIX)
endif
endif
$(TARGETS): $(TEGRA_TOP)/camera/utils/camerautil/$(OUTDIR)/libnvcamerautil$(LIB_SUFFIX)
$(TARGETS): $(NVLIB_TVMR)
ifeq ($(USE_ALSA),1)
    $(TARGETS): $(TEGRA_TOP)/apps/nvidia/alsa_usr_libs_utils/usr/lib/libasound.so
endif

RELEASE_PACKAGE_TARGETS := $(TARGETS)
RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
