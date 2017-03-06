TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/make/Makefile.defs

SUBDIRS += disp
ifeq ($(NVCAMERA_SUPPORTED),1)
  SUBDIRS += imager
endif
SUBDIRS += misc

include $(TOPDIR)/make/Makefile.rules
