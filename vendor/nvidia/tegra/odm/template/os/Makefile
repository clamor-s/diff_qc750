TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/../core-private/make/Makefile.defs

SUBDIRS += drivers
ifneq (,$(filter $(TARGET_OS)-$(TARGET_OS_SUBTYPE),linux-aos rvds-rvds))
    ifeq ($(TARGET_CPU),armv6)
        SUBDIRS += linux
    endif
endif

include $(TOPDIR)/../core-private/make/Makefile.rules
