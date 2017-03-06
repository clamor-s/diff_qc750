TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/../core-private/make/Makefile.defs


SUBDIRS += nvextfsmgr
SUBDIRS += extfs

ifneq (,$(filter $(TARGET_OS)-$(TARGET_OS_SUBTYPE),linux-aos rvds-rvds))
ifeq ($(TARGET_CPU),armv6)

default: $(SUBDIRS)

endif
endif

nosupport:
	@echo "This version of nvextfsmgr is exclusive to linux"
    
include $(TOPDIR)/../core-private/make/Makefile.rules
