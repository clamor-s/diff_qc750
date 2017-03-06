TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

SUBDIRS += dtvtuner
SUBDIRS += pmu
ifeq ($(NVCAMERA_SUPPORTED),1)
  SUBDIRS += $(TEGRA_TOP)/camera-partner/imager
endif
ifneq ($(TARGET_OS),rvds)
  SUBDIRS += hdmi
endif

SUBDIRS += audiocodec
ifneq ($(NO_SRC_MODULES),1)
SUBDIRS += misc
SUBDIRS += $(TEGRA_TOP)/$(TEMPLATE_DIR)/odm_kit/adaptations/gpio_ext
SUBDIRS += $(TEGRA_TOP)/$(TEMPLATE_DIR)/odm_kit/adaptations/usb_mux
endif

include $(TEGRA_TOP)/make/Makefile.rules
