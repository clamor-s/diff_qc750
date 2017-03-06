# Copyright (c) 2009-2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.

# The purpose of this makefile is to build dummy .so's or import libraries for
# each of our major interfaces.  You can link against these even if the library
# itself has not been built.

TOPDIR := ..
include $(TEGRA_TOP)/core-private/make/Makefile.defs

SUBDIRS += ../utils/nvidl

LCINCS += $(NVLIB_COMMON_INCLUDES)

BUILD_STUBS := 0
ifeq ($(TARGET_OS),linux)
  ifeq ($(TARGET_CPU),armv6)
     BUILD_STUBS := 1
  endif
endif
ifeq ($(TARGET_OS),qnx)
  BUILD_STUBS := 1
endif

# stubs by definition should not have dependencies on other libs.
ifeq ($(BUILD_STUBS),1)
  SYSTEM_LDLIBS_DLL =
endif

MAKE_EXPORT_FILE_FLAGS := -imp-def
EXPORT_FILES :=
TARGETS :=

ifeq ($(NV_DEF_USE_DLL),1)
  EXPORT_FILES += $(TEGRA_TOP)/core/utils/aes_ref/libnvaes_ref.export
  EXPORT_FILES += $(TEGRA_TOP)/core/utils/nvapputil/libnvapputil.export
  EXPORT_FILES += $(TEGRA_TOP)/core/utils/nvos/libnvos.export
  EXPORT_FILES += $(TEGRA_TOP)/core/utils/nvtestresults/libnvtestresults.export
  EXPORT_FILES += $(TEGRA_TOP)/core-private/utils/nvtiff/libnvtiff.export
  EXPORT_FILES += $(TEGRA_TOP)/core/utils/nvusbhost/libnvusbhost/libnvusbhost.export
  #EXPORT_FILES += $(NVODMDIR_ODM_KIT)/nvidia/template/odm_kit/adaptations/hdmi/libnvodm_hdmi.export
  EXPORT_FILES += $(TEGRA_TOP)/customers/nvidia-partner/template/odm_kit/adaptations/hdmi/libnvodm_hdmi.export

  EXPORT_FILES += $(TEGRA_TOP)/cg/cg_compiler/cgdrv/libcgdrv.export
  EXPORT_FILES += $(TEGRA_TOP)/graphics/2d/2d/libnvddk_2d.export
  EXPORT_FILES += $(TEGRA_TOP)/graphics/2d/2d_v2/libnvddk_2d_v2.export
  EXPORT_FILES += $(TEGRA_TOP)/graphics/cwm/source/libnvcwm.export
  EXPORT_FILES += $(TEGRA_TOP)/graphics/egl/egl/libEGL.export
  EXPORT_FILES += $(TEGRA_TOP)/graphics/egl/nvwsi/libnvwsi.export
  EXPORT_FILES += $(TEGRA_TOP)/graphics/opengles/opengles1/libGLESv1_CM.export
  EXPORT_FILES += $(TEGRA_TOP)/graphics/opengles/opengles2/libGLESv2.export
  EXPORT_FILES += $(TEGRA_TOP)/graphics/openkode/source/libKD.export
  EXPORT_FILES += $(TEGRA_TOP)/graphics/openvg/source/libOpenVG.export
  EXPORT_FILES += $(TEGRA_TOP)/graphics/openvg/source/libOpenVGU.export
  EXPORT_FILES += $(TEGRA_TOP)/graphics/winutil/nvwinsys/libnvwinsys.export
  EXPORT_FILES += $(TEGRA_TOP)/graphics/winutil/nvwinsys/libnvwinsys_kd.export

  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/nvboot/lib/host/libnvboothost.export
  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/libnvdc/libnvdc.export
  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/nvddk/disp/libnvddk_disp.export
  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/nvddk/blockdev/libnvddk_blockdevmgr.export
  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/nvddk/nand/libnvddk_nand.export
  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/nvddk/sdio/libnvddk_sdio.export
  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/nvddk/snor/libnvddk_snor.export
  ifneq ($(TARGET_SOC),ap20)
  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/nvddk/se/libnvddk_se.export
  endif
  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/nvddk/aes/libnvddk_aes.export
  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/nvddk/spi_flash/libnvddk_spif.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/libnvmm.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/utils/libnvmm_utils.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia-partner/nvmm/nvmmlite/libnvmmlite.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia-partner/nvmm/nvmmlite/utils/libnvmmlite_utils.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia/codecs/libnvmm_audio.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia/codecs/libnvmm_video.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia/codecs/libnvmm_image.export
  EXPORT_FILES += $(TEGRA_TOP)/camera/core/libnvmm_camera.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/blocks/libnvmm_parser.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/blocks/libnvmm_writer.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/service/libnvmm_service.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/manager/libnvmm_manager.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/contentpipe/libnvmm_contentpipe.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia-partner/nvmm/nvmm/transport/avp/libnvmm_avpentry.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia-partner/utils/nvavp/libnvavp.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia/tvmr/tvmr/libnvtvmr.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia/tvmr/nvparser/libnvparser.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia/tvmr/nvmedia/libnvmedia.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia/tvmr/nvmedia/libnvmedia_audio.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia/utils/nvvsapi/nvvsdec/libnvvsdec.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia/utils/nvvsapi/nvvsenc/libnvvsenc.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia/wfd/nvcap/clock/libnvtsclk.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia/wfd/nvcap/tsmux/libnvtsmux.export
  EXPORT_FILES += $(TEGRA_TOP)/camera-private/postprocessing/nvsm/libnvsm.export
  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/nvodm/libnvodm_misc.export
  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/nvodm/libnvodm_audiocodec.export
  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/nvodm/libnvodm_disp.export
  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/nvodm/libnvodm_dtvtuner.export
  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/nvodm/libnvodm_imager.export
  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/nvodm/libnvodm_query.export

  EXPORT_FILES += $(TEGRA_TOP)/core/drivers/nvtestio/libnvtestio.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia-partner/openmax/il/libnvomx.export
  EXPORT_FILES += $(TEGRA_TOP)/multimedia-partner/openmax/ilclient/libnvomxilclient.export
  EXPORT_FILES += $(TEGRA_TOP)/core-private/drivers/opensles/libnvosles.export


  ifeq ($(TARGET_OS)-$(TARGET_CPU),linux-armv6)
    EXPORT_FILES += $(TEGRA_TOP)/core/mobile_linux/daemons/nvrm/idlhelper/libnvdispatch_helper.export
  endif
  ifeq ($(TARGET_OS),qnx)
    EXPORT_FILES += $(TEGRA_TOP)/qnx/src/resmgrs/nvrm/idlhelper/libnvdispatch_helper.export
  endif

  INTERFACES := $(patsubst %.export,%,$(notdir $(EXPORT_FILES)))
  TARGETS := $(patsubst %,$(OUTDIR)/%$(IMPLIB_SUFFIX),$(INTERFACES))
  TARGETS += $(OUTDIR)/libnvrm$(IMPLIB_SUFFIX)
  TARGETS += $(OUTDIR)/libnvrm_graphics$(IMPLIB_SUFFIX)

  ifeq ($(BUILD_STUBS),1)
    TARGETS += $(OUTDIR)/libnvodm_kernel_query$(IMPLIB_SUFFIX)
    TARGETS += $(OUTDIR)/libnvrm_impl$(IMPLIB_SUFFIX)
    TARGETS += $(OUTDIR)/libnvrm_graphics_impl$(IMPLIB_SUFFIX)
    TARGETS += $(OUTDIR)/libnvstormgr_stub$(IMPLIB_SUFFIX)
  endif

  ifneq ($(EMPTY),$(filter $(TARGET_OS),linux qnx))
    C_FILES := $(patsubst %$(IMPLIB_SUFFIX),%.c,$(TARGETS))
  endif
endif

IDL_TARGETS :=
ifneq ($(HOST_BUILD),1)
  IDL_TARGETS += $(HOST_OUTDIR)/nvddk_aes_common.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvddk_aes.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_analog.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_diag.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_dma.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_gpio.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_init.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_memmgr.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_module.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_spi.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_channel.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_interrupt.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_pinmux.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_power.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_transport.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_xpc.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_i2c.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_owr.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_moduleloader.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvvibrate.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvaccelerometer.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_pmu.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvmultitouch.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_keylist.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_pwm.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_pcie.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvrm_memctrl.h
  IDL_TARGETS += $(HOST_OUTDIR)/nvstormgr.h
endif

default:
ifneq ($(EMPTY),$(filter $(TARGET_OS),linux qnx)) # Workaround for vpath bugs in make
default: $(C_FILES)
endif
ifneq ($(HOST_BUILD),1)
	$(MAKE) $(IDL_TARGETS)
endif
ifdef TARGETS
	$(MAKE) $(TARGETS)
endif
	$(DO_HOST_BUILD_TOO)

ifeq ($(NV_LIST_RELEASE_PACKAGE),1)
  include Makefile.localrules.list_package
else
  include Makefile.localrules.build
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules
