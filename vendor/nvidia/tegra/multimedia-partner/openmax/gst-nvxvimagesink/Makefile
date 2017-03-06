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
LIB_NVXV_DIR := $(TEGRA_TOP)/external/gstreamer/gst-nvxvimagesink
TEGRA_PATCH_DIR := $(LIB_NVXV_DIR)/.nvtegra
LIB_NVXVSINK  := $(LIB_NVXV_DIR)/src/.libs/libgstnvxvimagesink.so
MAKE_NVXVSINK  := $(LIB_NVXV_DIR)/Makefile
CONF_NVXVSINK  := $(LIB_NVXV_DIR)/configure
CONF_PARAMS   :=--host=arm-linux --disable-silent-rules
ifeq ($(TARGET_FLOAT_ABI_HARD),1)
CONF_PARAMS   += --with-pkg-config-path=$(LIB_GSTOMX_DIR)/.debs/usr/lib/arm-linux-gnueabihf/pkgconfig
else
CONF_PARAMS   += --with-pkg-config-path=$(LIB_GSTOMX_DIR)/.debs/usr/lib/pkgconfig
endif
CONF_PARAMS   += --x-includes=$(P4ROOT)/sw/tools/unix/targets/common/xorg-server/include

CFLAGS :=-march=armv7-a

LDFLAGS := -L$(P4ROOT)/sw/mobile/tools/linux/xorg/arm-none-linux-gnueabi/lib/ -lXv -lX11 -lXrender -lXext -lXau -lXdmcp

ifeq ($(TARGET_FLOAT_ABI_HARD),1)
  CFLAGS += -mfloat-abi=hard
else
  CFLAGS += -mhard-float -mfloat-abi=softfp
endif
CFLAGS  += -mfpu=vfpv3-d16 -mthumb-interwork -Os -O2
CFLAGS  += -finline-functions -finline-limit=300 -fomit-frame-pointer -fgcse-after-reload -Wmissing-prototypes
CFLAGS  += -Wno-pointer-sign -Wno-uninitialized -Wno-main -I$(P4ROOT)/sw/tools/unix/targets/common/xorg-server/include
CFLAGS  += $(NVLIB_KHRONOS_INCLUDES)
CFLAGS  += -I$(TEGRA_TOP)/graphics/X11/drivers/tegra/src/include -I$(TEGRA_TOP)/kernel/drivers/video/tegra/nvmap
ifeq ($(TARGET_USE_NEW_TOOLCHAIN),1)
  # gcc-4.6.1 warns a lot more than previous compilers.
  # The following is the minimal set of warnings that need to not error out
  # the build for it to succeed. -Wno-error would also work, but this
  # explicit list allows them to be fixed in smaller chunks.
  CFLAGS += -Wno-error=switch
endif

# export needed environment shell scripts running autoconfig,
# configure and generated Makefile
export NOCONFIGURE=true
export CC
export CPP
export NM
export P4ROOT
export CFLAGS
export LDFLAGS

all: $(MAKE_NVXVSINK) l4t-copy-libnvxvimagesink

.PHONY: l4t-copy-libnvxvimagesink
l4t-copy-libnvxvimagesink: $(LIB_NVXVSINK)
	@mkdir -p $(OUTDIR)
	$(CP) $(LIB_NVXVSINK) $(OUTDIR)
	$(CP) $(LIB_NVXVSINK) $(TEGRA_TOP)/core/$(OUTDIR)

.PHONY: $(LIB_NVXVSINK)
$(LIB_NVXVSINK):
	$(MAKE) -C $(LIB_NVXV_DIR) -f Makefile

$(MAKE_NVXVSINK): $(CONF_NVXVSINK)
	set -e; cd $(LIB_NVXV_DIR); ./configure $(CONF_PARAMS)

$(CONF_NVXVSINK):
	$(CP) -r $(TEGRA_PATCH_DIR)/pre-gen/* $(LIB_NVXV_DIR)

clean:
	@$(ECHO) $@ing libgstomx : $(MAKE) $(MFLAGS)
	-$(MAKE) -C $(LIB_NVXV_DIR) -f Makefile $@
	@-$(RM) $(OUTDIR)/$(notdir $(LIB_NVXVSINK))
	@$(ECHO) returned from $@ libgstnvxvsink : : $(MAKE) $(MFLAGS)

distclean clobber sweep:
	@$(ECHO) $@ing libgstnvxvimagesink : $(MAKE) $(MFLAGS)
	-$(MAKE) -C $(LIB_NVXV_DIR) -f Makefile clean
	-$(MAKE) -C $(LIB_NVXV_DIR) -f Makefile distclean
	@-$(RM) $(OUTDIR)/$(notdir $(LIB_NVXVSINK))
	@-$(FIND) $(TEGRA_PATCH_DIR)/pre-gen/* -type f | $(SED) 's/.*\/pre-gen/./' | xargs $(RM)
	@$(ECHO) returned from $@ libgstnvxvimagesink : : $(MAKE) $(MFLAGS)
