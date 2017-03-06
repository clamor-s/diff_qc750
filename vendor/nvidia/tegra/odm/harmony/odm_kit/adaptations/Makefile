TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/../core-private/make/Makefile.defs

SUBDIRS += dtvtuner
ifneq ($(TARGET_OS),rvds)
  SUBDIRS += hdmi
endif

ifneq ($(NO_SRC_MODULES),1)
SUBDIRS += misc
SUBDIRS += $(TOPDIR)/$(TEMPLATE_DIR)/odm_kit/adaptations/gpio_ext
SUBDIRS += $(TOPDIR)/$(TEMPLATE_DIR)/odm_kit/adaptations/usb_mux
endif

include $(TOPDIR)/../core-private/make/Makefile.rules
