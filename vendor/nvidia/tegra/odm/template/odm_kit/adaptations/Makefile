TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/../core-private/make/Makefile.defs

SUBDIRS += audiocodec
SUBDIRS += disp
ifeq ($(NVCAMERA_SUPPORTED),1)
  SUBDIRS += imager
endif
SUBDIRS += misc # fails to compile
SUBDIRS += pmu
SUBDIRS += hdmi
SUBDIRS += tmon
SUBDIRS += charging

include $(TOPDIR)/../core-private/make/Makefile.rules
