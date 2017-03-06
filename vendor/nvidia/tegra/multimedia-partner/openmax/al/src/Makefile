# Copyright (c) 2011-2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an
# express license agreement from NVIDIA Corporation is strictly prohibited.

TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

MODULE_NAME := libnvomxal

WITHDEBSDIR := $(TEGRA_TOP)/external/gstreamer/gst-openmax/.nvtegra/gstdeps_selfbuilt
DEBSDIR := $(TEGRA_TOP)/multimedia-partner/openmax/al/.debs
LCINCS += -I$(DEBSDIR)/usr/include/glib-2.0
LCINCS += -I$(DEBSDIR)/usr/lib/glib-2.0/include
LCINCS += -I$(DEBSDIR)/usr/include/gstreamer-0.10

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

WARN := $(filter-out -Wmissing-declarations -Wpointer-sign -Wmissing-prototypes -Wstrict-prototypes -Wimplicit-function-declaration, $(WARN))
CFLAGS += -Wno-error=unused-variable

#al_trace can be on or off
al_trace := off
#al_debug can be on or off
al_debug := off
#al_log can be on or off
al_log := off
#al_loglevel can be Default, Verbose, Debug, Info, Warn, Error, Fatal, Silent
al_loglevel := Verbose

CFLAGS += -x c++
CFLAGS += -DLI_API=''
CFLAGS += -DUSE_PROFILES=0x18
CFLAGS += -DLINUX_OMXAL

ifeq ($(al_trace), on)
  CFLAGS += -DUSE_TRACE
  CFLAGS += -DXA_TRACE_DEFAULT=XA_TRACE_ALL
else
  CFLAGS += -UUSE_TRACE
endif

ifeq ($(al_debug), on)
  CFLAGS += -DUSE_DEBUG
else
  CFLAGS += -UUSE_DEBUG
endif

ifeq ($(al_log), on)
  ifeq ($(al_loglevel), Default)
     CFLAGS += -DUSE_LOG=SLAndroidLogLevel_Default
  endif
  ifeq ($(al_loglevel), Verbose)
     CFLAGS += -DUSE_LOG=SLAndroidLogLevel_Verbose
  endif
  ifeq ($(al_loglevel), Debug)
     CFLAGS += -DUSE_LOG=SLAndroidLogLevel_Debug
  endif
  ifeq ($(al_loglevel), Info)
     CFLAGS += -DUSE_LOG=SLAndroidLogLevel_Info
  endif
  ifeq ($(al_loglevel), Warn)
     CFLAGS += -DUSE_LOG=SLAndroidLogLevel_Warn
  endif
  ifeq ($(al_loglevel), Error)
     CFLAGS += -DUSE_LOG=SLAndroidLogLevel_Error
  endif
  ifeq ($(al_loglevel), Fatal)
     CFLAGS += -DUSE_LOG=SLAndroidLogLevel_Fatal
  endif
  ifeq ($(al_loglevel), Silent)
     CFLAGS += -DUSE_LOG=SLAndroidLogLevel_Silent
  endif
else
  CFLAGS += -DUSE_LOG=SLAndroidLogLevel_Silent
endif

CFLAGS += -DGST_USE_UNSTABLE_API
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/al/src
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/al/src/autogen
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/al/include
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/include/openmax/al
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/al/src/itf

C_FILES :=
C_FILES += OpenSLES_IID.c
C_FILES += assert.c
C_FILES += classes.c
C_FILES += MPH_to.c
C_FILES += data.c
C_FILES += devices.c
C_FILES += entry.c
C_FILES += handler_bodies.c
C_FILES += trace.c
C_FILES += locks.c
C_FILES += sles.c
C_FILES += sl_iid.c
C_FILES += sllog.c
C_FILES += ThreadPool.c
C_FILES += sync.c
C_FILES += xa_iid.c
C_FILES += xa_entry.c

C_FILES += autogen/IID_to_MPH.c
C_FILES += objects/C3DGroup.c
C_FILES += objects/CAudioPlayer.c
C_FILES += objects/CAudioRecorder.c
C_FILES += objects/CEngine.c
C_FILES += objects/COutputMix.c
C_FILES += objects/CMediaPlayer.c
C_FILES += objects/CCameraDevice.c
C_FILES += objects/CMediaRecorder.c
C_FILES += objects/CNvDataTap.c

C_FILES += itf/IEngine.c
C_FILES += itf/IEngineCapabilities.c
C_FILES += itf/IObject.c

C_FILES += itf/IDynamicInterfaceManagement.c
C_FILES += itf/IThreadSync.c

C_FILES += itf/ICamera.c
C_FILES += itf/IImageDecoderCapabilities.c
C_FILES += itf/IMetadataInsertion.c
C_FILES += itf/IVideoDecoderCapabilities.c
C_FILES += itf/IAudioDecoderCapabilities.c
C_FILES += itf/ICameraCapabilities.c
C_FILES += itf/IImageEffect.c
C_FILES += itf/IPlay.c
C_FILES += itf/ISeek.c
C_FILES += itf/IVideoEncoder.c
C_FILES += itf/IAudioEncoderCapabilities.c
C_FILES += itf/IImageEncoder.c
C_FILES += itf/IRecord.c
C_FILES += itf/IVideoEncoderCapabilities.c
C_FILES += itf/IAudioIODeviceCapabilities.c
C_FILES += itf/IImageControls.c
C_FILES += itf/IImageEncoderCapabilities.c
C_FILES += itf/ISnapshot.c
C_FILES += itf/IVideoPostProcessing.c
C_FILES += itf/linux_common.c
C_FILES += itf/linux_mediaplayer.c
C_FILES += itf/linux_postprocessing.c
C_FILES += itf/linux_streaminfo.c
C_FILES += itf/linux_outputmix.c
C_FILES += itf/IEqualizer.c
C_FILES += itf/IVolume.c
C_FILES += itf/IImageEncoder.c
C_FILES += itf/IMetadataExtraction.c
C_FILES += itf/IPlaybackRate.c
C_FILES += itf/IPrefetchStatus.c
C_FILES += itf/IStreamInformation.c
C_FILES += itf/IMetadataTraversal.c
C_FILES += itf/IDeviceVolume.c
C_FILES += itf/IDynamicSource.c
C_FILES += itf/IAudioEncoder.c
C_FILES += itf/linux_cameradevice.c
C_FILES += itf/linux_mediarecorder.c
C_FILES += itf/linux_audioiodeviceinfo.c
C_FILES += itf/IOutputMix.c

C_FILES += ut/slesutResult.c

SYSTEM_LDLIBS_DLL += -lpthread -L$(DEBSDIR)/usr/lib -lglib-2.0 -lgthread-2.0
SYSTEM_LDLIBS_DLL += -lstdc++ -lgobject-2.0 -lgstreamer-0.10 -lgstinterfaces-0.10 -lgstpbutils-0.10 -lgstphotography-0.10

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: GST_INSTALL $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(TARGETS): $(OBJS)

GST_INSTALL :
	dpkg --extract $(WITHDEBSDIR)/libglib2.0-dev_2.26.0-0ubuntu1_armel.deb $(DEBSDIR)
	dpkg --extract $(WITHDEBSDIR)/libglib2.0-0_2.26.0-0ubuntu1_armel.deb $(DEBSDIR)
	dpkg --extract $(WITHDEBSDIR)/libpcre3-dev_8.02-1_armel.deb $(DEBSDIR)
	dpkg --extract $(WITHDEBSDIR)/libpcre3_8.02-1_armel.deb $(DEBSDIR)
	dpkg --extract $(WITHDEBSDIR)/zlib1g-dev_1.2.3.4.dfsg-3ubuntu1_armel.deb $(DEBSDIR)
	dpkg --extract $(WITHDEBSDIR)/zlib1g_1.2.3.4.dfsg-3ubuntu1_armel.deb $(DEBSDIR)
	tar -xvzf $(WITHDEBSDIR)/gstv0.10.36.tar.gz -C $(DEBSDIR)

	rm $(DEBSDIR)/usr/lib/libglib-2.0.so
	ln -s $(DEBSDIR)/lib/libglib-2.0.so.0.2600.0 $(DEBSDIR)/usr/lib/libglib-2.0.so

	rm $(DEBSDIR)/usr/lib/libpcre.so
	ln -s $(DEBSDIR)/lib/libpcre.so.3 $(DEBSDIR)/usr/lib/libpcre.so

	rm $(DEBSDIR)/usr/lib/libz.so
	ln -s $(DEBSDIR)/lib/libz.so.1 $(DEBSDIR)/usr/lib/libz.so

	find $(DEBSDIR) -name "*.pc" | xargs sed -i -e "s@^prefix=\(.*\)@prefix=$(DEBSDIR)/usr@"
	find $(DEBSDIR) -name "*.la" | xargs sed -i -e "s@^libdir='\(.*\)@libdir='$(DEBSDIR)\1@"

include $(TEGRA_TOP)/core-private/make/Makefile.rules

