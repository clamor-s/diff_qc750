TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


SUBDIRS += services

default:
	$(MAKE) -f Makefile.one_board || exit 3

BOARDS_COMPATIBLE_WITH_SOC :=
ifeq ($(TARGET_SOC),ap20)
  BOARDS_COMPATIBLE_WITH_SOC := $(SUPPORTED_BOARDS_AP20)
endif
ifeq ($(TARGET_SOC),t30)
  BOARDS_COMPATIBLE_WITH_SOC := $(SUPPORTED_BOARDS_T30)
endif
ifeq ($(EMPTY),$(filter $(TARGET_SOC),$(TARGET_SOC_LIST)))
  $(error Unknown TARGET_SOC of "$(TARGET_SOC)")
endif

BUILD_BOARDS=$(foreach b,$(1),$(MAKE) -f Makefile.one_board TARGET_BOARD=$(b) $(2) &&) true

odm_kits_for_all_boards: submake.default
	$(call BUILD_BOARDS,$(BOARDS_COMPATIBLE_WITH_SOC),)

clean: local.clean
clobber: local.clobber
sweep: local.sweep

.PHONY: local.clean local.clobber local.sweep

local.clean:
	$(call BUILD_BOARDS,$(BOARDS_COMPATIBLE_WITH_SOC),clean)

local.clobber:
	$(call BUILD_BOARDS,$(BOARDS_COMPATIBLE_WITH_SOC),clobber)

local.sweep:
	$(call BUILD_BOARDS,$(BOARDS_COMPATIBLE_WITH_SOC),sweep)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
