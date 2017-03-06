ifneq ($(HAVE_NVIDIA_PROP_SRC),false)
ifeq ($(TARGET_BOARD_PLATFORM),tegra)
include $(all-subdir-makefiles)
endif
endif
