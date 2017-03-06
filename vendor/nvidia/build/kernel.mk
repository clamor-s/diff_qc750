#
# Linux kernel and loadable kernel modules
#
ifneq ($(TARGET_NO_KERNEL),true)

ifneq ($(TOP),.)
$(error Kernel build assumes TOP == . i.e Android build has been started from TOP/Makefile )
endif

# Android build is started from the $TOP/Makefile, therefore $(CURDIR)
# gives the absolute path to the TOP.
KERNEL_PATH ?= $(CURDIR)/kernel

#kernel_version := $(strip $(shell head $(KERNEL_PATH)/Makefile | \
#	grep "SUBLEVEL =" | cut -d= -f2))

TARGET_USE_DTB ?= false
BOOTLOADER_SUPPORTS_DTB ?= false
KERNEL_SUPPORTS_DTB := false
APPEND_DTB_TO_KERNEL ?= false
EXTRA_KERNEL_TARGETS :=

# Always use absolute path for NV_KERNEL_INTERMEDIATES_DIR
ifneq ($(filter /%, $(TARGET_OUT_INTERMEDIATES)),)
NV_KERNEL_INTERMEDIATES_DIR := $(TARGET_OUT_INTERMEDIATES)/KERNEL
else
NV_KERNEL_INTERMEDIATES_DIR := $(CURDIR)/$(TARGET_OUT_INTERMEDIATES)/KERNEL
endif

dotconfig := $(NV_KERNEL_INTERMEDIATES_DIR)/.config
BUILT_KERNEL_TARGET := $(NV_KERNEL_INTERMEDIATES_DIR)/arch/$(TARGET_ARCH)/boot/zImage

ifeq ($(TARGET_TEGRA_VERSION),ap20)
    TARGET_KERNEL_CONFIG ?= tegra_android_defconfig
else
ifeq ($(TARGET_TEGRA_VERSION),t30)
    TARGET_KERNEL_CONFIG ?= tegra3_android_defconfig
endif
endif


ifneq (,$(filter $(TARGET_PRODUCT),nabi2 nabi2_3d nabi2_xd nabi_2s))
	TMP_CUSTOM_KERNEL_CONFIG := tegra3_nabi2_android_defconfig
endif
ifneq (,$(filter $(TARGET_PRODUCT),qc750 n710 n1010))
	TMP_CUSTOM_KERNEL_CONFIG := tegra3_qc750_android_defconfig
endif


ifneq (,$(filter $(TARGET_PRODUCT),ns_14t004))
	TMP_CUSTOM_KERNEL_CONFIG := tegra3_ns_14t004_android_defconfig
endif

ifneq (,$(filter $(TARGET_PRODUCT),wikipad))
	TMP_CUSTOM_KERNEL_CONFIG := tegra3_wikipad_android_defconfig
endif

ifneq (,$(filter $(TARGET_PRODUCT),itq700 mm3201))
	TMP_CUSTOM_KERNEL_CONFIG := tegra3_itq700_android_defconfig
endif
ifneq (,$(filter $(TARGET_PRODUCT),n750))
	TMP_CUSTOM_KERNEL_CONFIG := tegra3_n750_android_defconfig
endif
ifneq (,$(filter $(TARGET_PRODUCT),birch))
	TMP_CUSTOM_KERNEL_CONFIG := tegra3_birch_android_defconfig
endif
ifneq ($(wildcard $(KERNEL_PATH)/arch/arm/configs/$(TMP_CUSTOM_KERNEL_CONFIG)),)
    TARGET_KERNEL_CONFIG := $(TMP_CUSTOM_KERNEL_CONFIG)
endif


ifeq ($(wildcard $(KERNEL_PATH)/arch/arm/configs/$(TARGET_KERNEL_CONFIG)),)
    $(error Could not find kernel defconfig for board)
endif


# Always use absolute path for NV_KERNEL_MODULES_TARGET_DIR and
# NV_KERNEL_BIN_TARGET_DIR
ifneq ($(filter /%, $(TARGET_OUT)),)
NV_KERNEL_MODULES_TARGET_DIR := $(TARGET_OUT)/lib/modules
NV_KERNEL_BIN_TARGET_DIR     := $(TARGET_OUT)/bin
else
NV_KERNEL_MODULES_TARGET_DIR := $(CURDIR)/$(TARGET_OUT)/lib/modules
NV_KERNEL_BIN_TARGET_DIR     := $(CURDIR)/$(TARGET_OUT)/bin
endif

ifeq ($(BOARD_WLAN_DEVICE),wl12xx_mac80211)
    NV_COMPAT_KERNEL_DIR := $(CURDIR)/3rdparty/ti/compat-wireless
    NV_COMPAT_KERNEL_MODULES_TARGET_DIR := $(NV_KERNEL_MODULES_TARGET_DIR)/compat
endif

ifeq ($(BOARD_WLAN_DEVICE),wl18xx_mac80211)
    NV_COMPAT_KERNEL_DIR := $(CURDIR)/3rdparty/ti/compat-wireless/compat-wireless-wl8
    NV_COMPAT_KERNEL_MODULES_TARGET_DIR := $(NV_KERNEL_MODULES_TARGET_DIR)/compat
endif

KERNEL_DEFCONFIG_PATH := $(KERNEL_PATH)/arch/$(TARGET_ARCH)/configs/$(TARGET_KERNEL_CONFIG)

# If target claims to support Device Tree, check for the device tree configs in the defconfig
ifeq ($(shell grep CONFIG_USE_OF=y $(KERNEL_DEFCONFIG_PATH)),CONFIG_USE_OF=y)
    KERNEL_SUPPORTS_DTB := true
endif

# If we don't have kernel support for DTB, we won't be using it
ifeq ($(KERNEL_SUPPORTS_DTB),false)
ifeq ($(TARGET_USE_DTB),true)
    $(warning Kernel doesn\'t support Device Tree, disabling DT)
    TARGET_USE_DTB := false
endif
endif

# If we don't have kernel or bootloader support for DTB loading, we won't be using it
ifeq ($(BOOTLOADER_SUPPORTS_DTB),false)
ifeq ($(APPEND_DTB_TO_KERNEL),false)
ifeq ($(TARGET_USE_DTB),true)
    $(warning No support to pass Device Tree to kernel, disabling DT)
    TARGET_USE_DTB := false
endif
endif
endif

# If we are not using DTB, don't append DTB to kernel
ifeq ($(TARGET_USE_DTB),false)
    APPEND_DTB_TO_KERNEL := false
endif

# The target must provide a name for the DT file (sources located in arch/arm/boot/dts/*)
ifeq ($(TARGET_USE_DTB),true)
    ifeq ($(TARGET_KERNEL_DT_NAME),)
        $(error Must provide a DT file name in TARGET_KERNEL_DT_NAME -- <kernel>/arch/arm/boot/dts/*)
    else
        KERNEL_DTS_PATH := $(KERNEL_PATH)/arch/$(TARGET_ARCH)/boot/dts/$(TARGET_KERNEL_DT_NAME).dts
        BUILT_KERNEL_DTB := $(NV_KERNEL_INTERMEDIATES_DIR)/arch/$(TARGET_ARCH)/boot/$(TARGET_KERNEL_DT_NAME).dtb
        INSTALLED_DTB_TARGET := $(OUT)/$(TARGET_KERNEL_DT_NAME).dtb
        ifneq ($(wildcard $(KERNEL_DTS_PATH)), $(KERNEL_DTS_PATH))
            $(error DTS file not found -- $(KERNEL_DTS_PATH))
        endif
    endif

    ifeq ($(APPEND_DTB_TO_KERNEL),false)
        EXTRA_KERNEL_TARGETS := $(INSTALLED_DTB_TARGET)
    endif
endif

$(warning TARGET_USE_DTB = $(TARGET_USE_DTB))
$(warning KERNEL_DTS_PATH = $(KERNEL_DTS_PATH))
$(warning BUILT_KERNEL_DTB = $(BUILT_KERNEL_DTB))
$(warning INSTALLED_DTB_TARGET = $(INSTALLED_DTB_TARGET))
$(warning EXTRA_KERNEL_TARGETS = $(EXTRA_KERNEL_TARGETS))
$(warning APPEND_DTB_TO_KERNEL = $(APPEND_DTB_TO_KERNEL))

KERNEL_EXTRA_ARGS=
OS=$(shell uname)
ifeq ($(OS),Darwin)
  # check prerequisites
  ifeq ($(GNU_COREUTILS),)
    $(error GNU_COREUTILS is not set)
  endif
  ifeq ($(wildcard $(GNU_COREUTILS)/stat),)
    $(error $(GNU_COREUTILS)/stat not found. Please install GNU coreutils.)
  endif

  # add GNU stat to the path
  KERNEL_EXTRA_ENV=env PATH=$(GNU_COREUTILS):$(PATH)
  # bring in our elf.h
  KERNEL_EXTRA_ARGS=HOST_EXTRACFLAGS=-I$(TOP)/../vendor/nvidia/tegra/core-private/include\ -DKBUILD_NO_NLS
  HOSTTYPE=darwin-x86
endif

ifeq ($(OS),Linux)
  KERNEL_EXTRA_ENV=
  HOSTTYPE=linux-x86
endif

ifdef PLATFORM_IS_JELLYBEAN
KERNEL_TOOLCHAIN := prebuilts/gcc/$(HOSTTYPE)/arm/arm-eabi-4.6/bin/arm-eabi-
else
KERNEL_TOOLCHAIN := prebuilt/$(HOSTTYPE)/toolchain/arm-eabi-4.4.3/bin/arm-eabi-
endif

# We should rather use CROSS_COMPILE=$(PRIVATE_TOPDIR)/$(TARGET_TOOLS_PREFIX).
# Absolute paths used in all path variables.
# ALWAYS prefix these macros with "+" to correctly enable parallel building!
define kernel-make
$(KERNEL_EXTRA_ENV) $(MAKE) -C $(PRIVATE_SRC_PATH) \
		KBUILD_BUILD_USER=MID \
    ARCH=$(TARGET_ARCH) \
    CROSS_COMPILE=$(PRIVATE_KERNEL_TOOLCHAIN) \
    O=$(NV_KERNEL_INTERMEDIATES_DIR) $(KERNEL_EXTRA_ARGS) \
    $(if $(SHOW_COMMANDS),V=1)
endef

ifneq ( , $(findstring $(BOARD_WLAN_DEVICE), wl12xx_mac80211 wl18xx_mac80211))
define compat-kernel-make
$(KERNEL_EXTRA_ENV) $(MAKE) -C $(NV_COMPAT_KERNEL_DIR) \
    ARCH=$(TARGET_ARCH) \
    CROSS_COMPILE=$(PRIVATE_KERNEL_TOOLCHAIN) \
    KLIB=$(NV_KERNEL_INTERMEDIATES_DIR) \
    KLIB_BUILD=$(NV_KERNEL_INTERMEDIATES_DIR) \
    $(if $(SHOW_COMMANDS),V=1)
endef
endif

$(dotconfig): $(KERNEL_PATH)/arch/$(TARGET_ARCH)/configs/$(TARGET_KERNEL_CONFIG) | $(NV_KERNEL_INTERMEDIATES_DIR)
	@echo "Kernel config " $(TARGET_KERNEL_CONFIG)
	+$(hide) $(kernel-make) $(TARGET_KERNEL_CONFIG)
ifeq ($(SECURE_OS_BUILD),y)
	@echo "SecureOS enabled kernel"
	$(hide) $(KERNEL_PATH)/scripts/config --file $@ --enable TRUSTED_FOUNDATIONS
endif
ifeq ($(NVIDIA_KERNEL_COVERAGE_ENABLED),1)
	@echo "Explicitly enabling coverage support in kernel config on user request"
	$(hide) $(KERNEL_PATH)/scripts/config --file $@ \
		--enable DEBUG_FS \
		--enable GCOV_KERNEL \
		--disable GCOV_PROFILE_ALL
endif

ifeq ($(APPEND_DTB_TO_KERNEL),true)
	@echo "Enable configs to handle DTB appended kernel image (zImage)"
	$(hide) $(KERNEL_PATH)/scripts/config --file $@ \
		--enable ARM_APPENDED_DTB \
		--enable ARM_ATAG_DTB_COMPAT
endif

# TODO: figure out a way of not forcing kernel & module builds.
$(BUILT_KERNEL_TARGET): $(dotconfig) FORCE | $(NV_KERNEL_INTERMEDIATES_DIR)
	@echo "Kernel build"
	+$(hide) $(kernel-make) zImage

$(BUILT_KERNEL_DTB): $(BUILT_KERNEL_TARGET) FORCE
	@echo "Device tree build"
	+$(hide) $(kernel-make) $(TARGET_KERNEL_DT_NAME).dtb

kmodules-build_only: $(BUILT_KERNEL_TARGET) FORCE | $(NV_KERNEL_INTERMEDIATES_DIR)
	@echo "Kernel modules build"
	+$(hide) $(kernel-make) modules
ifneq ( , $(findstring $(BOARD_WLAN_DEVICE), wl12xx_mac80211 wl18xx_mac80211))
	+$(hide) $(compat-kernel-make)
endif

# This will add all kernel modules we build for inclusion the system
# image - no blessing takes place.
kmodules: kmodules-build_only FORCE | $(NV_KERNEL_MODULES_TARGET_DIR) $(NV_COMPAT_KERNEL_MODULES_TARGET_DIR)
	@echo "Kernel modules install"
	for f in `find $(NV_KERNEL_INTERMEDIATES_DIR) -name "*.ko"` ; do cp -v "$$f" $(NV_KERNEL_MODULES_TARGET_DIR) ; done
ifneq ( , $(findstring $(BOARD_WLAN_DEVICE), wl12xx_mac80211 wl18xx_mac80211))
	for f in `find $(NV_COMPAT_KERNEL_DIR) -name "*.ko"` ; do cp -v "$$f" $(NV_COMPAT_KERNEL_MODULES_TARGET_DIR) ; done
endif

# At this stage, BUILT_SYSTEMIMAGE in $TOP/build/core/Makefile has not
# yet been defined, so we cannot rely on it.
_systemimage_intermediates_kmodules := \
    $(call intermediates-dir-for,PACKAGING,systemimage)
BUILT_SYSTEMIMAGE_KMODULES := $(_systemimage_intermediates_kmodules)/system.img
NV_INSTALLED_SYSTEMIMAGE := $(PRODUCT_OUT)/system.img

# When kernel tests are built, we also want to update the system
# image, but in general case we do not want to build kernel tests
# always.
ifneq ($(findstring kernel-tests,$(MAKECMDGOALS)),)
kernel-tests: build_kernel_tests $(NV_INSTALLED_SYSTEMIMAGE) FORCE

# In order to prevent kernel-tests rule from matching pattern rule
# kernel-%
kernel-tests:
	@echo "Kernel space tests built and system image updated!"

# For parallel builds. Systemimage can only be built after kernel
# tests have been built.
$(BUILT_SYSTEMIMAGE_KMODULES): build_kernel_tests
endif

build_kernel_tests: kmodules FORCE | $(NV_KERNEL_MODULES_TARGET_DIR) $(NV_KERNEL_BIN_TARGET_DIR)
	@echo "Kernel space tests build"
	@echo "Tests at $(PRIVATE_TOPDIR)/vendor/nvidia/tegra/tests/linux/kernel_space_tests"
	+$(hide) $(kernel-make) M=$(PRIVATE_TOPDIR)/vendor/nvidia/tegra/tests/linux/kernel_space_tests
	for f in `find $(PRIVATE_TOPDIR)/vendor/nvidia/tegra/tests/linux/kernel_space_tests -name "*.ko"` ; do cp -v "$$f" $(NV_KERNEL_MODULES_TARGET_DIR) ; done
	for f in `find $(PRIVATE_TOPDIR)/vendor/nvidia/tegra/tests/linux/kernel_space_tests -name "*.sh"` ; do cp -v "$$f" $(NV_KERNEL_BIN_TARGET_DIR) ; done
	+$(hide) $(kernel-make) M=$(PRIVATE_TOPDIR)/vendor/nvidia/tegra/tests/linux/kernel_space_tests clean
	find $(PRIVATE_TOPDIR)/vendor/nvidia/tegra/tests/linux/kernel_space_tests -name "modules.order" -print0 | xargs -0 rm -rf

# Unless we hardcode the list of kernel modules, we cannot create
# a proper dependency from systemimage to the kernel modules.
# If we decide to hardcode later on, BUILD_PREBUILT (or maybe
# PRODUCT_COPY_FILES) can be used for including the modules in the image.
# For now, let's rely on an explicit dependency.
$(BUILT_SYSTEMIMAGE_KMODULES): kmodules

# Following dependency is already defined in $TOP/build/core/Makefile,
# but for the sake of clarity let's re-state it here. This dependency
# causes following dependencies to be indirectly defined:
#   $(NV_INSTALLED_SYSTEMIMAGE): kmodules $(BUILT_KERNEL_TARGET)
# which will prevent too early creation of systemimage.
$(NV_INSTALLED_SYSTEMIMAGE): $(BUILT_SYSTEMIMAGE_KMODULES)

# $(INSTALLED_KERNEL_TARGET) is defined in
# $(TOP)/build/target/board/Android.mk
$(INSTALLED_DTB_TARGET): $(BUILT_KERNEL_DTB) | $(ACP)
ifeq ($(APPEND_DTB_TO_KERNEL),false)
	@echo "Copying DTB file"
	$(copy-file-to-target)
endif

$(INSTALLED_KERNEL_TARGET): $(BUILT_KERNEL_TARGET) $(BUILT_KERNEL_DTB) $(EXTRA_KERNEL_TARGETS) FORCE | $(ACP)
ifeq ($(APPEND_DTB_TO_KERNEL),true)
	@echo "Appending DTB file to kernel image"
	+$(hide) cat $(BUILT_KERNEL_DTB) >>$(BUILT_KERNEL_TARGET)
endif
	$(copy-file-to-target)

# Kernel build also includes some drivers as kernel modules which are
# packaged inside system image. Therefore, for incremental builds,
# dependency from kernel to installed system image must be introduced,
# so that recompilation of kernel automatically updates also the
# drivers in system image to be flashed to the device.
kernel: $(INSTALLED_KERNEL_TARGET) kmodules $(NV_INSTALLED_SYSTEMIMAGE)

# 'kernel-build_only' is an isolated target meant to be used if _only_
# the build of the kernel and kernel modules is needed. This can be
# useful for example when measuring the build time of these
# components, but in most cases 'kernel-build_only' is probably not
# the target you want to use!
#
# Please use 'kernel'-target instead, it will also update the system
# image after compiling kernel and modules, and copy both the kernel
# and system images to correct locations for flashing.
kernel-build_only: $(BUILT_KERNEL_TARGET) kmodules-build_only
	@echo "kernel + modules built successfully! (Note, just build, no install done!)"

kernel-%: | $(NV_KERNEL_INTERMEDIATES_DIR)
	+$(hide) $(kernel-make) $*
ifneq ( , $(findstring $(BOARD_WLAN_DEVICE), wl12xx_mac80211 wl18xx_mac80211))
	+$(hide) $(compat-kernel-make) $*
endif

NV_KERNEL_BUILD_DIRECTORY_LIST := \
	$(NV_KERNEL_INTERMEDIATES_DIR) \
	$(NV_KERNEL_MODULES_TARGET_DIR) \
	$(NV_COMPAT_KERNEL_MODULES_TARGET_DIR) \
	$(NV_KERNEL_BIN_TARGET_DIR)

$(NV_KERNEL_BUILD_DIRECTORY_LIST):
	$(hide) mkdir -p $@

.PHONY: kernel kernel-% build_kernel_tests kmodules

# Set private variables for all builds. TODO: Why?
kernel kernel-% build_kernel_tests kmodules $(dotconfig) $(BUILT_KERNEL_TARGET) $(BUILT_KERNEL_DTB): PRIVATE_SRC_PATH := $(KERNEL_PATH)
kernel kernel-% build_kernel_tests kmodules $(dotconfig) $(BUILT_KERNEL_TARGET) $(BUILT_KERNEL_DTB): PRIVATE_TOPDIR := $(CURDIR)
kernel kernel-% build_kernel_tests kmodules $(dotconfig) $(BUILT_KERNEL_TARGET) $(BUILT_KERNEL_DTB): PRIVATE_KERNEL_TOOLCHAIN := $(CURDIR)/$(KERNEL_TOOLCHAIN)

endif
