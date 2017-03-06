# Copyright (c) 2006-2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an
# express license agreement from NVIDIA Corporation is strictly prohibited.

TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

LIB_GSTOMX_DIR  := $(TEGRA_TOP)/external/gstreamer/gst-openmax
GSTOMX_SRCREL_DIRNAME := gstomx_srcrel
GSTOMX_SRCREL_DIR := $(TEGRA_TOP)/external/gstreamer/$(GSTOMX_SRCREL_DIRNAME)
TEGRA_PATCH_DIR := $(LIB_GSTOMX_DIR)/.nvtegra
LIB_GSTOMX    := $(LIB_GSTOMX_DIR)/omx/.libs/libgstomx.so
MAKE_GSTOMX   := $(LIB_GSTOMX_DIR)/Makefile
MAKEIN_GSTOMX := $(LIB_GSTOMX_DIR)/Makefile.in
CONF_GSTOMX   := $(LIB_GSTOMX_DIR)/configure
#OUT_DIR       :=_out/$(BUILD_FLAVOR)_$(DVS_NEWBUILD_CHIP_FAMILY)_$(TARGET_OS_SUBTYPE)_$(TARGET_CPU)
CONF_PARAMS   :=--host=arm-linux --enable-experimental --enable-tegra --enable-eglimage

ifeq ($(TARGET_FLOAT_ABI_HARD),1)
CONF_PARAMS  += --with-debdir=$(TEGRA_PATCH_DIR)/deb_ubuntu--12.04-armhf
CONF_PARAMS  += --with-pkg-config-path=$(LIB_GSTOMX_DIR)/.debs/usr/lib/arm-linux-gnueabihf/pkgconfig
else
 ifeq ($(NV_LATEST_GST),1)
 CONF_PARAMS  += --with-debdir=$(TEGRA_PATCH_DIR)/gstdeps_selfbuilt
 else
  ifeq ($(NV_EMBEDDED_BUILD),1)
  CONF_PARAMS  += --with-debdir=$(TEGRA_PATCH_DIR)/deb_ubuntu--10.10-armel
  else
  CONF_PARAMS  += --with-debdir=$(TEGRA_PATCH_DIR)/deb_ubuntu--10.04-armel
  endif
 endif
CONF_PARAMS  += --with-pkg-config-path=$(LIB_GSTOMX_DIR)/.debs/usr/lib/pkgconfig
endif

ifeq ($(NV_EMBEDDED_BUILD),1)
CONF_PARAMS  += --enable-automotive
endif


CFLAGS :=-march=armv7-a
ifeq ($(TARGET_FLOAT_ABI_HARD),1)
  CFLAGS += -mfloat-abi=hard
else
  CFLAGS += -mhard-float -mfloat-abi=softfp
endif
CFLAGS  += -DGST_KPI_MEASURE
CFLAGS  += -mfpu=vfpv3-d16 -mthumb-interwork -Os -O2
CFLAGS  += -finline-functions -finline-limit=300 -fomit-frame-pointer -fgcse-after-reload -Wmissing-prototypes
CFLAGS  += -Wno-pointer-sign -Wno-uninitialized -Wno-main -I$(P4ROOT)/sw/tools/unix/targets/common/xorg-server/include
CFLAGS  += $(NVLIB_KHRONOS_INCLUDES) -I$(TEGRA_TOP)/multimedia-partner/openmax/include/openmax/il
ifeq ($(TARGET_USE_NEW_TOOLCHAIN),1)
  # gcc-4.6.1 warns a lot more than previous compilers.
  # The following is the minimal set of warnings that need to not error out
  # the build for it to succeed. -Wno-error would also work, but this
  # explicit list allows them to be fixed in smaller chunks.
  CFLAGS += -Wno-error=switch
endif

LDFLAGS :=-L$(TEGRA_TOP)/core/include/$(OUTDIR) -lEGL -lGLESv2

# export needed environment chell scripts running autoconfig,
# configure and generated Makefile
export NOCONFIGURE=true
export CC
export CPP
export NM
export P4ROOT
export CFLAGS
export LDFLAGS

all: $(MAKE_GSTOMX) l4t-copy-libgstomx

.PHONY: l4t-copy-libgstomx
l4t-copy-libgstomx: $(LIB_GSTOMX)
	@mkdir -p $(OUTDIR)
	$(CP) $(LIB_GSTOMX) $(OUTDIR)
	$(CP) $(LIB_GSTOMX) $(TEGRA_TOP)/core/$(OUTDIR)

.PHONY: $(LIB_GSTOMX)
$(LIB_GSTOMX):
	@$(ECHO) $@ing libgstomx : $(MAKE) $(MFLAGS)
	$(MAKE) -C $(LIB_GSTOMX_DIR) -f Makefile

$(MAKE_GSTOMX): $(CONF_GSTOMX)
	set -e; cd $(LIB_GSTOMX_DIR); ./configure $(CONF_PARAMS)

$(CONF_GSTOMX):
	$(CP) -r $(TEGRA_PATCH_DIR)/pre-gen/* $(LIB_GSTOMX_DIR)

gstomx_source_pkg:
#Delete any generated files
	$(MAKE) clobber
	$(RM) -rf $(GSTOMX_SRCREL_DIR)
	@mkdir -p $(GSTOMX_SRCREL_DIR)/nv_headers $(GSTOMX_SRCREL_DIR)/gst-openmax/
	$(CP) -r $(LIB_GSTOMX_DIR)/* $(GSTOMX_SRCREL_DIR)/gst-openmax/
	$(CP) -r $(TEGRA_TOP)/multimedia-partner/openmax/include/openmax/il/* $(GSTOMX_SRCREL_DIR)/nv_headers/
	$(CP) $(TEGRA_TOP)/l4t/release/docs/GstOmx_ExtBuild_Readme.txt $(GSTOMX_SRCREL_DIR)/README.txt
	sed -i -e's/m4_esyscmd(\[build-aux\/git-version-gen\])/version-0.10/' $(GSTOMX_SRCREL_DIR)/gst-openmax/configure.ac
	tar -zcvf $(GSTOMX_SRCREL_DIR).tar.gz -C $(TEGRA_TOP)/external/gstreamer/ $(GSTOMX_SRCREL_DIRNAME)
#Mkdir, just in case this target runs before any build target, as it is feasible.
	@mkdir -p $(OUTDIR)
	$(CP) $(GSTOMX_SRCREL_DIR).tar.gz $(OUTDIR)
	$(CP) $(GSTOMX_SRCREL_DIR).tar.gz $(TEGRA_TOP)/core/$(OUTDIR)
#Clean up the staging directory and tar ball
	$(RM) -rf $(GSTOMX_SRCREL_DIR)
	$(RM) -rf $(GSTOMX_SRCREL_DIR).tar.gz

clean:
	@$(ECHO) $@ing libgstomx : $(MAKE) $(MFLAGS)
	-$(MAKE) -C $(LIB_GSTOMX_DIR) -f Makefile $@
	@-$(RM) $(OUTDIR)/$(notdir $(LIB_GSTOMX))
	@$(ECHO) returned from $@ libgstomx : : $(MAKE) $(MFLAGS)

distclean clobber sweep:
	@$(ECHO) $@ing libgstomx : $(MAKE) $(MFLAGS)
	-$(MAKE) -C $(LIB_GSTOMX_DIR) -f Makefile clean
	-$(MAKE) -C $(LIB_GSTOMX_DIR) -f Makefile distclean
	@-$(RM) $(OUTDIR)/$(notdir $(LIB_GSTOMX))
	@-$(RM) -r $(LIB_GSTOMX_DIR)/Makefile.in
	@-$(RM) -r $(LIB_GSTOMX_DIR)/omx/Makefile.in
	@-$(RM) $(LIB_GSTOMX_DIR)/aclocal.m4
	@-$(RM) $(LIB_GSTOMX_DIR)/autoregen.sh
	@-$(RM) $(LIB_GSTOMX_DIR)/configure
	@-$(RM) $(LIB_GSTOMX_DIR)/config.h.in
	@-$(RM) $(LIB_GSTOMX_DIR)/stamp-h.in
	@-$(RM) $(LIB_GSTOMX_DIR)/m4/l*.m4
	@-$(RM) $(LIB_GSTOMX_DIR)/build-aux/c*
	@-$(RM) $(LIB_GSTOMX_DIR)/build-aux/depcomp
	@-$(RM) $(LIB_GSTOMX_DIR)/build-aux/install-sh
	@-$(RM) $(LIB_GSTOMX_DIR)/build-aux/ltmain.sh
	@-$(RM) $(LIB_GSTOMX_DIR)/build-aux/missing
	@$(ECHO) returned from $@ libgstomx : : $(MAKE) $(MFLAGS)


