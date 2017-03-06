#
# Copyright (c) 2009-2012, NVIDIA CORPORATION.  All rights reserved.
#

# This Makefile is used to build the NVIDIA drivers and the Linux kernel
# for GNU Linux or Chromium OS.  It also supports gbuild dependency
# generation for release packaging.
#
# $ make                        # build everything
# $ make dvs                    # build what gets built in DVS
# $ make gnu_linux_release_list # generate dependencies for Gnu Linux

# NV_WAR_BUG_816075
TEGRA_TOP ?= $(patsubst %/core,%,$(TEGRA_ROOT))
export TEGRA_TOP

TOPDIR := $(TEGRA_TOP)/core

include $(TEGRA_TOP)/core-private/make/Makefile.defs


#Why projetcs must be included separately here
#include $(TEGRA_TOP)/core-private/make/Makefile.projects

NV_MAKEFILE_LIST += $(TEGRA_TOP)/core/mobile_linux/make/Makefile

ifeq ($(EMPTY),$(filter $(TARGET_OS)-$(TARGET_CPU),linux-armv6))
  $(error $(TARGET_OS)-$(TARGET_CPU) is not supported by this Makefile)
endif

ifeq ($(TARGET_OS_SUBTYPE),android)
  $(error NVIDIA build flow is no longer supported for Android)
endif

# L4T is not supported
ifneq ($(NV_EMBEDDED_BUILD),1)
  ifeq ($(TARGET_MODS),)
    $(info This build flow is no longer supported for L4T)
    $(info L4T requires use of tmake build system: https://wiki.nvidia.com/wmpwiki/index.php/Tmake)
    $(info For details about building and flashing L4T please refer to:)
    $(info https://wiki.nvidia.com/wmpwiki/index.php/WMP_ap20/Gnu_Linux)
    $(error The build was unsuccessful)
  endif
endif

# For now, if the target board is an FPGA platform, build without a root
# device so that the root file system is initialized from the ramdisk.
ifneq ($(NVODM_BOARD_IS_FPGA),$(EMPTY))
  $(warning, NO_ROOT_DEVICE=1 assumed for FPGA platform)
  NO_ROOT_DEVICE ?= 1
endif

# map harmony, ventana and seaboard to common defconfig
ifneq ($(EMPTY),$(filter $(TARGET_BOARD),ventana seaboard harmony))
    kernel_config := tegra_defconfig
else
  ifeq ($(USE_MODS_DEFCONFIG),1)
    kernel_config := tegra_$(TARGET_BOARD)_mods_defconfig
  else ifneq ($(EMPTY),$(filter $(TARGET_BOARD),p1852 p852 e1853))
    kernel_config := tegra_$(TARGET_BOARD)_$(TARGET_OS_SUBTYPE)_defconfig
  else ifeq ($(USE_MODS_LDK_DEFCONFIG),1)
    kernel_config := tegra_$(TARGET_BOARD)_mods_ldk_defconfig
  else
      kernel_config := tegra3_defconfig
  endif
endif

KERN_DIR ?= $(TEGRA_TOP)/kernel
KERN_OUTDIR ?= $(TEGRA_TOP)/core/_out/$(BUILD_FLAVOR)_$(kernel_config)

export KERN_DIR
TEGRA_KERN_DIR := $(KERN_DIR)/arch/arm/mach-tegra

ifeq ($(BUILD_FLAVOR),release)
    OUTPUT_FLAVOR := Release
else
    OUTPUT_FLAVOR := Debug
endif

# This variable is defined in Makefile.defs.linux-arm.
CROSS_COMPILE := $(CROSSBIN)

KERN_ENV :=
KERN_ENV += ARCH=arm
KERN_ENV += CROSS_COMPILE=$(CROSSBIN)
KERN_ENV += O=$(KERN_OUTDIR)

DTC_PATH := $(TEGRA_TOP)/l4t/prebuilts/dtc
DTC_BIN := $(DTC_PATH)/dtc

UBOOT_ENV :=
UBOOT_ENV += ARCH=arm
UBOOT_ENV += CROSS_COMPILE=$(CROSSBIN)
UBOOT_ENV += CONFIG_L4T=1
UBOOT_ENV += USE_PRIVATE_LIBGCC=yes
UBOOT_ENV += DTC=$(DTC_BIN)

CP_FLAGS := -f -r -u -v

SUDO := sudo

ifneq ($(EMPTY),$(wildcard Makefile.defs.$(TARGET_OS_SUBTYPE)))
include Makefile.defs.$(TARGET_OS_SUBTYPE)
endif

# specify which loadable kernel modules should be built by the
# kernel_modules goal (included in the default goal)
kernel_module_goals :=
ifeq ($(NVODM_BOARD_IS_FPGA),$(EMPTY))
  ifneq ($(EMPTY),$(filter $(TARGET_BOARD),harmony))
    kernel_module_goals += ath_kernel
  endif
  ifeq ($(TARGET_RIL),emp)
    kernel_module_goals += emp_kernel
  else ifeq ($(TARGET_RIL),ifx)
    kernel_module_goals += ifx_kernel
  endif
endif

ifneq ($(TARGET_MODS),)
   kernel_module_goals += mods_kernel
else
   kernel_module_goals += tests_kernel
endif

kernel_module_clean_goals := $(addsuffix _clean,$(kernel_module_goals))

os_image_goal :=
ifneq ($(EMPTY),$(filter $(TARGET_OS_SUBTYPE),chromiumos gnu_linux))
  ifeq ($(TARGET_BOARD),harmony)
    os_image_goal += mkyaffs
  endif
  os_image_goal += mkbootimg
  os_image_goal += mkubootimg
  os_image_goal += nfs_targetfs
endif

# core-private install path
PRIVATE_INSTALL_TARGET := $(TEGRA_TOP)/core-private/$(OUTDIR)
PRIVATE_AVP_INSTALL_TARGET := $(TEGRA_TOP)/core-private/$(AVP_OUTDIR)

# microboot install path
MICROBOOT_OUTDIR := _out/$(BUILD_FLAVOR)_$(DVS_NEWBUILD_CHIP_FAMILY)_aos_armv4

# This division of targets is legacy from Android builds.
TARGETS := bootloader_eabi
TARGETS += drivers
TARGETS += kernel
TARGETS += kernel_modules
TARGETS += daemons
TARGETS += mobilemods
TARGETS += build_l4t_tests
ifeq ($(NV_EMBEDDED_BUILD),1)
  TARGETS += quickboot_production
  TARGETS += mkbootimg
else
  TARGETS += build_uboot
  TARGETS += $(os_image_goal)
endif

TEST_TARGETS := special_tests
TEST_TARGETS += sanity_test_plans
TEST_TARGETS += test_binaries
SRC_TARGETS :=
ifneq ($(NV_EMBEDDED_BUILD),1)
  SRC_TARGETS += source_package
endif

TARGETS_CLEAN := $(addsuffix _clean,$(TARGETS))
TEST_TARGETS_CLEAN := $(addsuffix _clean,$(TEST_TARGETS))

default: $(TARGETS)
	@echo "SUCCESS"

# Pattern used to filter out targets from tmake build
_SKIP_IN_TMAKE_PATTERN :=
_TMAKE_TARGETS := $(filter-out $(_SKIP_IN_TMAKE_PATTERN), $(TARGETS))

# Target used for building limited subest in Buildbrain tmake build
default-tmake: $(_TMAKE_TARGETS)
	@echo "$@ depends on targets: $+"
	@echo "Build of target $@ was a SUCCESS"

clean: $(TARGETS_CLEAN)

ifneq ($(EMPTY),$(kernel_module_goals))
include Makefile.rules.modules
endif

gstomx_src_package:
	$(MAKE) -C $(TEGRA_TOP)/multimedia-partner/openmax/gst-openmax gstomx_source_pkg

source_package: gstomx_src_package

drivers:
	$(MAKE) -C $(TEGRA_TOP)/core-private/make
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core-private/make TARGET_CPU=armv4 TARGET_OS=rvds)

drivers_clean:
	$(MAKE) -C $(TEGRA_TOP)/core-private/make clean
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core-private/make TARGET_CPU=armv4 TARGET_OS=rvds clean)

daemons:
	$(MAKE) -C $(TEGRA_TOP)/core/include
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvreftrack
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvfxmath
	$(MAKE) -C $(TEGRA_TOP)/core/drivers/nvodm
	$(MAKE) -C $(TEGRA_TOP)/camera-private/postprocessing/nvsm
	$(MAKE) -C $(TEGRA_TOP)/core/drivers/nvddk/dap
	$(MAKE) -C $(TEGRA_TOP)/core/drivers/nvddk/i2s
	$(MAKE) -C $(TEGRA_TOP)/core/drivers/nvddk/spdif
	$(MAKE) -C $(TEGRA_TOP)/core/mobile_linux/daemons

daemons_clean:
	$(MAKE) -C $(TEGRA_TOP)/core/mobile_linux/daemons clean


# Hack to work-around bug 559357 - dependency tracking is broken for the
# assembly files in NvMM.  The NOT_SUPPORTED_MULTIMEDIA flag just skips
# compilation of NvMM, which is fine for the bootloader.
define bootloader_goal
.PHONY: bootloader_$(1) bootloader_$(1)_clean bootloader_release_list_$(1)
bootloader_$(1):
	$$(MAKE) NOT_SUPPORTED_MULTIMEDIA=1 NVCAMERA_SUPPORTED=0 \
              TARGET_CPU=$$(TARGET_CPU) TARGET_OS=$(1) TARGET_OS_SUBTYPE=$(2) \
              -C $$(TEGRA_TOP)/core-private/make

bootloader_$(1)_clean:
	$$(MAKE) NOT_SUPPORTED_MULTIMEDIA=1 NVCAMERA_SUPPORTED=0 \
              TARGET_CPU=$$(TARGET_CPU) TARGET_OS=$(1) TARGET_OS_SUBTYPE=$(2) \
              -C $$(TEGRA_TOP)/core-private/make clean

bootloader_release_list_$(1):
	$$(MAKE) NOT_SUPPORTED_MULTIMEDIA=1 NV_LIST_RELEASE_PACKAGE=1 NVCAMERA_SUPPORTED=0 \
              TARGET_CPU=$$(TARGET_CPU) TARGET_OS=$(1) TARGET_OS_SUBTYPE=$(2) \
              -C $$(TEGRA_TOP)/core-private/make
endef

$(eval $(call bootloader_goal,eabi,aos))
$(eval $(call bootloader_goal,rvds,rvds))

# In order to produce x86 binaries, we must set TARGET_OS to rvds.
# Is this brain damaged choice just due to DVS packaging history??

FORCE_HOST_BUILD := TARGET_OS=rvds

nvflash_list:
	NV_LIST_RELEASE_PACKAGE=1 $(MAKE) _nvflash
	NV_LIST_RELEASE_PACKAGE=1 $(MAKE) _buildbct
	NV_LIST_RELEASE_PACKAGE=1 $(MAKE) _mobilemods
	NV_LIST_RELEASE_PACKAGE=1 $(MAKE) test_binaries

nvflash:
	$(MAKE) _nvflash

nvflash_clean:
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core-private/drivers/nvboot TARGET_CPU=armv4 TARGET_OS=rvds clean)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core/system/nv3p TARGET_CPU=armv4 TARGET_OS=rvds clean)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvos TARGET_CPU=armv4 TARGET_OS=rvds clean)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core/system/nvbct TARGET_CPU=armv4 TARGET_OS=rvds clean)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core-private/utils/aes_keysched_lock TARGET_CPU=armv4 TARGET_OS=rvds clean)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core/system/nvflash TARGET_CPU=armv4 TARGET_OS=rvds clean)
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/drivers/nvtestio clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core-private/drivers/nvboot clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/aes_ref clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/nvapputil clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/nvos clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/nvtestresults clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/nvusbhost/libnvusbhost clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/system/nv3p clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/system/nvdioconverter clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/system/nvflash/lib clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/system/nvflash/app clean

_nvflash:
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/drivers/nvtestio
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core-private/drivers/nvboot
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/aes_ref
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/nvapputil
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/nvos
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/nvtestresults
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/nvusbhost/libnvusbhost
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/system/nv3p
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/system/nvdioconverter
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/system/nvflash/lib
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/system/nvflash TARGET_CPU=armv4 TARGET_OS=rvds)
	#$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/system/ec/app

buildbct:
	$(MAKE) _buildbct

_buildbct:
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/aes_ref
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/nvapputil
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/nvos
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core-private/system/buildbct

mobilemods:
	$(MAKE) _mobilemods

mobilemods_clean:
	$(MAKE) _mobilemods_clean

_mobilemods:
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/drivers/nvtestio
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core-private/drivers/nvboot
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/aes_ref
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/nvapputil
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/nvos
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/nvusbhost/libnvusbhost
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core-private/drivers/nvblserver
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core-private/utils/nvtestserver
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core-private/utils/nvelfutil
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core-private/drivers/nvhost
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/tests/nvtest/basic TARGET_CPU=armv4 TARGET_OS=rvds)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/tests/diags/mats TARGET_CPU=armv4 TARGET_OS=rvds)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core-private/drivers/nvtestloader TARGET_CPU=armv4 TARGET_OS=rvds)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core-private/drivers/nvboot TARGET_CPU=armv4 TARGET_OS=rvds)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvos/aos/avp TARGET_CPU=armv4 TARGET_OS=rvds)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvapputil TARGET_CPU=armv4 TARGET_OS=rvds)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvappmain TARGET_CPU=armv4 TARGET_OS=rvds)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvtestmain TARGET_CPU=armv4 TARGET_OS=rvds)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvtestresults TARGET_CPU=armv4 TARGET_OS=rvds)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core/drivers/nvtestio TARGET_CPU=armv4 TARGET_OS=rvds)
ifeq ($(TARGET_SOC),ap20)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/tests/diags/nand_ap20 TARGET_CPU=armv4 TARGET_OS=rvds)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/tests/diags/sdmmc TARGET_CPU=armv4 TARGET_OS=rvds)
endif

_mobilemods_clean:
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/drivers/nvtestio clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core-private/drivers/nvboot clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/aes_ref clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/nvapputil clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/nvos clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core/utils/nvusbhost/libnvusbhost clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core-private/drivers/nvblserver clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core-private/utils/nvtestserver clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core-private/utils/nvelfutil clean
	$(MAKE) $(FORCE_HOST_BUILD) -C $(TEGRA_TOP)/core-private/drivers/nvhost clean
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/tests/nvtest/basic TARGET_CPU=armv4 TARGET_OS=rvds clean)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/tests/diags/mats TARGET_CPU=armv4 TARGET_OS=rvds clean)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core-private/drivers/nvtestloader TARGET_CPU=armv4 TARGET_OS=rvds clean)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core-private/drivers/nvboot TARGET_CPU=armv4 TARGET_OS=rvds clean)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvos/aos/avp TARGET_CPU=armv4 TARGET_OS=rvds clean)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvapputil TARGET_CPU=armv4 TARGET_OS=rvds clean)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvappmain TARGET_CPU=armv4 TARGET_OS=rvds clean)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvtestmain TARGET_CPU=armv4 TARGET_OS=rvds clean)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvtestresults TARGET_CPU=armv4 TARGET_OS=rvds clean)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/core/drivers/nvtestio TARGET_CPU=armv4 TARGET_OS=rvds clean)
ifeq ($(TARGET_SOC),ap20)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/tests/diags/nand_ap20 TARGET_CPU=armv4 TARGET_OS=rvds clean)
	(unset TARGET_OS_SUBTYPE; \
	$(MAKE) -C $(TEGRA_TOP)/tests/diags/sdmmc TARGET_CPU=armv4 TARGET_OS=rvds clean)
endif


test_binaries:
	$(MAKE) -C $(TEGRA_TOP)/core/drivers/nvtestio
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvapputil
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvtestresults
	$(MAKE) -C $(TEGRA_TOP)/core-private/utils/nvtestrun
	$(MAKE) -C $(TEGRA_TOP)/tests-partner/openmax/omxplayer
	$(MAKE) -C $(TEGRA_TOP)/tests-partner/openmax/omxplayer2
	$(MAKE) -C $(TEGRA_TOP)/tests/nvrm/power/dfs_cfg
	$(MAKE) -C $(TEGRA_TOP)/tests/nvrm/power/dfs_stress
	$(MAKE) -C $(TEGRA_TOP)/tests/nvrm/power/dfs_monitor
	$(MAKE) -C $(TEGRA_TOP)/tests/nvrm/power/dfs_log
	$(MAKE) -C $(TEGRA_TOP)/tests-camera/nvcamcorder
	$(MAKE) -C $(TEGRA_TOP)/tests-camera/nvodm/imager/focuser
	$(MAKE) -C $(TEGRA_TOP)/tests-camera/nvodm/imager/sensor
	$(MAKE) -C $(TEGRA_TOP)/tests-multimedia/nvmm/jpegenc
	$(MAKE) -C $(TEGRA_TOP)/tests-multimedia/nvmmlite/videoenc/hosttest
	$(MAKE) -C $(TEGRA_TOP)/tests-multimedia/nvmm/videodec
	$(MAKE) -C $(TEGRA_TOP)/tests-multimedia/nvmm/mpeg2vdec
	$(MAKE) -C $(TEGRA_TOP)/tests/linux/nvlowpower
ifeq ($(OGL1_SUPPORTED),1)
	$(MAKE) -C $(TEGRA_TOP)/3rdparty/khronos/conform/opengles1/conform
endif
ifeq ($(OGL2_SUPPORTED),1)
	$(MAKE) -C $(TEGRA_TOP)/3rdparty/khronos/conform/opengles2/conform
endif
	$(MAKE) -C $(TEGRA_TOP)/tests/linux/uart
	$(MAKE) -C $(TEGRA_TOP)/tests/linux/irda
	$(MAKE) -C $(TEGRA_TOP)/tests/linux/i2c/user/eeprom

test_binaries_clean:
	$(MAKE) -C $(TEGRA_TOP)/core/drivers/nvtestio clean
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvapputil clean
	$(MAKE) -C $(TEGRA_TOP)/core/utils/nvtestresults clean
	$(MAKE) -C $(TEGRA_TOP)/core-private/utils/nvtestrun clean
	$(MAKE) -C $(TEGRA_TOP)/tests-partner/openmax/omxplayer clean
	$(MAKE) -C $(TEGRA_TOP)/tests-partner/openmax/omxplayer2 clean
	$(MAKE) -C $(TEGRA_TOP)/tests/nvrm/power/dfs_cfg clean
	$(MAKE) -C $(TEGRA_TOP)/tests/nvrm/power/dfs_stress clean
	-$(MAKE) -C $(TEGRA_TOP)/tests/nvrm/power/dfs_monitor clean
	-$(MAKE) -C $(TEGRA_TOP)/tests/nvrm/power/dfs_log clean
	$(MAKE) -C $(TEGRA_TOP)/tests-camera/nvcamcorder clean
	$(MAKE) -C $(TEGRA_TOP)/tests/nvdispmgr/dmgles clean
	$(MAKE) -C $(TEGRA_TOP)/tests-camera/nvodm/imager/focuser clean
	$(MAKE) -C $(TEGRA_TOP)/tests-camera/nvodm/imager/sensor clean
	$(MAKE) -C $(TEGRA_TOP)/tests-multimedia/nvmm/jpegenc clean
	$(MAKE) -C $(TEGRA_TOP)/tests-multimedia/nvmmlite/videoenc/hosttest clean
	$(MAKE) -C $(TEGRA_TOP)/tests-multimedia/nvmm/videodec clean
	$(MAKE) -C $(TEGRA_TOP)/tests-multimedia/nvmm/mpeg2vdec clean
	$(MAKE) -C $(TEGRA_TOP)/tests/linux/nvlowpower clean
ifeq ($(OGL1_SUPPORTED),1)
	$(MAKE) -C $(TEGRA_TOP)/3rdparty/khronos/conform/opengles1/conform clean
endif
	$(MAKE) -C $(TEGRA_TOP)/3rdparty/khronos/conform/opengles2/conform clean
	$(MAKE) -C $(TEGRA_TOP)/tests/linux/uart clean
	$(MAKE) -C $(TEGRA_TOP)/tests/linux/i2c/user/eeprom clean



mods_ldk_test_binaries:
	$(MAKE) -C $(TEGRA_TOP)/tests-multimedia/nvmm/aacdec
	$(MAKE) -C $(TEGRA_TOP)/tests-multimedia/nvmm/mp3dec

kernel_deps := $(KERN_DIR)/arch/arm/configs/$(kernel_config)

kernel: $(kernel_deps)
	@mkdir -p $(KERN_OUTDIR)
	$(MAKE) -C $(KERN_DIR) $(KERN_ENV) $(kernel_config)
	$(MAKE) -C $(KERN_DIR) $(KERN_ENV)
	-$(NV_INSTALL) -l $(KERN_OUTDIR)/arch/arm/boot/zImage $(HOST_OUTDIR)
	-$(NV_INSTALL) -l $(KERN_OUTDIR)/arch/arm/boot/zImage $(OUTDIR)/$(TARGET_BOARD)

kernel_rebuild:
	$(MAKE) -C $(KERN_DIR) $(KERN_ENV)
	-$(NV_INSTALL) -l $(KERN_OUTDIR)/arch/arm/boot/zImage $(HOST_OUTDIR)
	-$(NV_INSTALL) -l $(KERN_OUTDIR)/arch/arm/boot/zImage $(OUTDIR)/$(TARGET_BOARD)

kernel_clean:
	-test -d $(KERN_OUTDIR) && $(MAKE) -C $(KERN_DIR) $(KERN_ENV) clean || echo Skipping $(MAKE) -C $(KERN_DIR) $(KERN_ENV) clean as $(KERN_OUTDIR) does not exist
	-test -d $(KERN_OUTDIR) && $(MAKE) -C $(KERN_DIR) $(KERN_ENV) mrproper || echo Skipping $(MAKE) -C $(KERN_DIR) $(KERN_ENV) mrproper as $(KERN_OUTDIR) does not exist
	-@rm -rf $(KERN_OUTDIR)

kernel_menuconfig:
	$(MAKE) -C $(KERN_DIR) $(KERN_ENV) $(kernel_config)
	$(MAKE) -C $(KERN_DIR) $(KERN_ENV) menuconfig

#
#   Updates all the tegra kernel configs interactively by using menuconfig
#   kernel target.
#
kernel_update_configs: $(SYS_STAMP) kernel_update_configs.sh
	bash ./kernel_update_configs.sh

# layer for quickboot:

quickboot quickboot_clean quickboot_production:
	$(MAKE) -C $(TEGRA_TOP)/core-private/embedded/quickboot $@

# generated by TARGETS_CLEAN:
quickboot_production_clean: quickboot_clean

#
# Build and copy in everything needed for the kernel, but DON'T build the
# kernel.
# To generate the release package, run
# official_build.py -p nvap_android_kernel -c <cl> -i_know_what_i_am_doing \
#    -local_dvs_files
#

kernel_modules: $(kernel_module_goals)

kernel_modules_clean: $(kernel_module_clean_goals)

mkyaffs:
	(unset TARGET_OS_SUBTYPE; unset TARGET_OS; unset TARGET_CPU; \
	$(MAKE) -C $(TEGRA_TOP)/3rdparty/mkyaffs2image/utils/ -f Makefile.nv YAFFS_TYPE=mkyaffs2)

mkyaffs_clean:
	(unset TARGET_OS_SUBTYPE; unset TARGET_OS; unset TARGET_CPU; \
	$(MAKE) -C $(TEGRA_TOP)/3rdparty/mkyaffs2image/utils/ -f Makefile.nv YAFFS_TYPE=mkyaffs2 clean)

mkbootimg:
	(unset TARGET_OS_SUBTYPE; unset TARGET_OS; unset TARGET_CPU; \
	$(MAKE) -C $(TEGRA_TOP)/3rdparty/mkbootimg/ -f Makefile.nv TOOL_TYPE=mkbootimg)

mkbootimg_clean:
	(unset TARGET_OS_SUBTYPE; unset TARGET_OS; unset TARGET_CPU; \
	$(MAKE) -C $(TEGRA_TOP)/3rdparty/mkbootimg/ -f Makefile.nv TOOL_TYPE=mkbootimg clean)

mkubootimg:
	(unset TARGET_OS_SUBTYPE; unset TARGET_OS; unset TARGET_CPU; \
	$(MAKE) -C $(TEGRA_TOP)/3rdparty/mkbootimg/ -f Makefile.nv TOOL_TYPE=mkubootscript)

mkubootimg_clean:
	(unset TARGET_OS_SUBTYPE; unset TARGET_OS; unset TARGET_CPU; \
	$(MAKE) -C $(TEGRA_TOP)/3rdparty/mkbootimg/ -f Makefile.nv TOOL_TYPE=mkubootscript clean)

#
# clean everything, including drivers.
#
clean: drivers_clean daemons_clean kernel_clean kernel_modules_clean quickboot_clean
clobber: clean
sweep: kernel_clean kernel_modules_clean quickboot_clean tsweep

# symlink all of the generated images and typically-used tools into
# $(INSTALL_TARGET) for convenience.  also perform a mkbootimg on the
# kernel and RAM disk, to generate a cold-bootable image. To add entire
# directories, append the directory path with a trailing '/*' to the
# list of TEST_FILES. To add specific system directory files, use the
# keyword '(SYSTEM)', just like that with no dollar sign, in place of
# the path to the source system directory.
#
# using ramdisk.img:
# $ sudo $ANDROID_HOST_BIN/fastboot -i 0x955 boot \
#        $KERN_DIR/arch/arm/boot/zImage ramdisk.img
#
# using flashboot.img:
# $ sudo $ANDROID_HOST_BIN/fastboot -i 0x955 flash boot flashboot.img
#

TEST_FILES :=
# Test Libs
TEST_FILES += libnvtestio.so
TEST_FILES += libnvtestresults.so
TEST_FILES += libnvapputil.so
# graphics 2D
TEST_FILES += nvddk_2d_test_brush.so
# graphics 3D
TEST_FILES += gles1_conform_cl
TEST_FILES += gles1_conform_cm
TEST_FIELS += gles1_covegl
TEST_FILES += gles1_covgl
TEST_FILES += gles1_primtest_cl
TEST_FIELS += gles1_primtest_cm
TEST_FILES += gles2_conform
TEST_FILES += gles2_simplest.so
# omx
TEST_FILES += omxplayer.so
TEST_FILES += omxplayer2
TEST_FILES += omxuseeglimage.so
# nvmm
TEST_FILES += nvmmlite_videoenc_test.so
TEST_FILES += nvmm_jpegenc_test.so
TEST_FILES += nvmm_videodec_test.so
TEST_FILES += nvmm_mpeg2vdec_test.so
# nvrm
TEST_FILES += nvrm_unit.so
TEST_FILES += nvrm_channel.so
TEST_FILES += nvrm_nvavp_smmu_sanity.so
TEST_FILES += nvrm_nvavp_smmu_test.so
# L4TMM apps
TEST_FILES += nvgstplayer
TEST_FILES += nvgstcapture
TEST_FILES += alplaybacktest
# Other
TEST_FILES += dfs_monitor.so
TEST_FILES += nvlowpower
TEST_FILES += uarttest
TEST_FILES += irda_test

TEST_FILES := $(addprefix $(INSTALL_TARGET)/,$(TEST_FILES))

# core-private test files
TEST_PRIV_FILES :=
TEST_PRIV_FILES += nvtest
TEST_PRIV_FILES := $(addprefix $(PRIVATE_INSTALL_TARGET)/,$(TEST_PRIV_FILES))
TEST_FILES += $(TEST_PRIV_FILES)

# If no root device is specified, put a selected subset of the system directory
# into the ramdisk. These paths do not get $(INSTALL_TARGET) prepended to them.
ifneq ($(NO_ROOT_DEVICE),$(EMPTY))

# Uncommenting the following line copies the entire system directory to the
# ramdisk. NOTE: that on FPGAs this will take a LONG TIME to decompress.
#TEST_FILES += $(TEGRA_TOP)/linux/distro/android/$(NV_ANDROID_BRANCH)/out/$(BUILD_FLAVOR)/target/product/$(TARGET_BOARD)/system/*

TEST_FILES += (SYSTEM)/bin/app_process
TEST_FILES += (SYSTEM)/bin/debuggerd
TEST_FILES += (SYSTEM)/bin/gdbserver
TEST_FILES += (SYSTEM)/bin/linker
TEST_FILES += (SYSTEM)/bin/logcat
TEST_FILES += (SYSTEM)/bin/logwrapper
TEST_FILES += (SYSTEM)/bin/mediaserver
TEST_FILES += (SYSTEM)/bin/nvrm_avp.bin
TEST_FILES += (SYSTEM)/bin/nvrm_daemon
TEST_FILES += (SYSTEM)/bin/nvrm_daemon_start.sh
TEST_FILES += (SYSTEM)/bin/service
TEST_FILES += (SYSTEM)/bin/servicemanager
TEST_FILES += (SYSTEM)/bin/sh
TEST_FILES += (SYSTEM)/bin/toolbox

# The following are symlinks to system/bin/toolbox
TEST_FILES += (SYSTEM)/bin/cat
TEST_FILES += (SYSTEM)/bin/chmod
TEST_FILES += (SYSTEM)/bin/chown
TEST_FILES += (SYSTEM)/bin/cmp
TEST_FILES += (SYSTEM)/bin/date
TEST_FILES += (SYSTEM)/bin/dd
TEST_FILES += (SYSTEM)/bin/df
TEST_FILES += (SYSTEM)/bin/dmesg
TEST_FILES += (SYSTEM)/bin/getevent
TEST_FILES += (SYSTEM)/bin/getprop
TEST_FILES += (SYSTEM)/bin/hd
TEST_FILES += (SYSTEM)/bin/id
TEST_FILES += (SYSTEM)/bin/ifconfig
TEST_FILES += (SYSTEM)/bin/iftop
TEST_FILES += (SYSTEM)/bin/insmod
TEST_FILES += (SYSTEM)/bin/ioctl
TEST_FILES += (SYSTEM)/bin/kill
TEST_FILES += (SYSTEM)/bin/ln
TEST_FILES += (SYSTEM)/bin/log
TEST_FILES += (SYSTEM)/bin/ls
TEST_FILES += (SYSTEM)/bin/lsmod
TEST_FILES += (SYSTEM)/bin/mkdir
TEST_FILES += (SYSTEM)/bin/mount
TEST_FILES += (SYSTEM)/bin/mv
TEST_FILES += (SYSTEM)/bin/netstat
TEST_FILES += (SYSTEM)/bin/newfs_msdos
TEST_FILES += (SYSTEM)/bin/notify
TEST_FILES += (SYSTEM)/bin/printenv
TEST_FILES += (SYSTEM)/bin/ps
TEST_FILES += (SYSTEM)/bin/reboot
TEST_FILES += (SYSTEM)/bin/renice
TEST_FILES += (SYSTEM)/bin/rm
TEST_FILES += (SYSTEM)/bin/rmdir
TEST_FILES += (SYSTEM)/bin/rmmod
TEST_FILES += (SYSTEM)/bin/route
TEST_FILES += (SYSTEM)/bin/schedtop
TEST_FILES += (SYSTEM)/bin/sendevent
TEST_FILES += (SYSTEM)/bin/setconsole
TEST_FILES += (SYSTEM)/bin/setprop
TEST_FILES += (SYSTEM)/bin/sleep
TEST_FILES += (SYSTEM)/bin/smd
TEST_FILES += (SYSTEM)/bin/start
TEST_FILES += (SYSTEM)/bin/stop
TEST_FILES += (SYSTEM)/bin/sync
TEST_FILES += (SYSTEM)/bin/top
TEST_FILES += (SYSTEM)/bin/umount
TEST_FILES += (SYSTEM)/bin/vmstat
TEST_FILES += (SYSTEM)/bin/watchprops
TEST_FILES += (SYSTEM)/bin/wipe

# Non-essential system files that may be moved to the active list
# as functionality is added.

#TEST_FILES += (SYSTEM)/bin/am
#TEST_FILES += (SYSTEM)/bin/applypatch
#TEST_FILES += (SYSTEM)/bin/applypatch_static
#TEST_FILES += (SYSTEM)/bin/bccmd
#TEST_FILES += (SYSTEM)/bin/bluetoothd
#TEST_FILES += (SYSTEM)/bin/bmgr
#TEST_FILES += (SYSTEM)/bin/bootanimation
#TEST_FILES += (SYSTEM)/bin/bugreport
#TEST_FILES += (SYSTEM)/bin/check_prereq
#TEST_FILES += (SYSTEM)/bin/dalvikvm
#TEST_FILES += (SYSTEM)/bin/dbus-daemon
#TEST_FILES += (SYSTEM)/bin/dexopt
#TEST_FILES += (SYSTEM)/bin/dhcpcd
#TEST_FILES += (SYSTEM)/bin/dumpcrash
#TEST_FILES += (SYSTEM)/bin/dumpstate
#TEST_FILES += (SYSTEM)/bin/dumpsys
#TEST_FILES += (SYSTEM)/bin/dvz
#TEST_FILES += (SYSTEM)/bin/flash_image
#TEST_FILES += (SYSTEM)/bin/fsck_msdos
#TEST_FILES += (SYSTEM)/bin/gzip
#TEST_FILES += (SYSTEM)/bin/hciattach
#TEST_FILES += (SYSTEM)/bin/ime
#TEST_FILES += (SYSTEM)/bin/input
#TEST_FILES += (SYSTEM)/bin/installd
#TEST_FILES += (SYSTEM)/bin/iptables
#TEST_FILES += (SYSTEM)/bin/keystore
#TEST_FILES += (SYSTEM)/bin/keystore_cli
#TEST_FILES += (SYSTEM)/bin/monkey
#TEST_FILES += (SYSTEM)/bin/mtpd
#TEST_FILES += (SYSTEM)/bin/netcfg
#TEST_FILES += (SYSTEM)/bin/nvddk_audiomixer_core.axf
#TEST_FILES += (SYSTEM)/bin/nv_hciattach
#TEST_FILES += (SYSTEM)/bin/nvmm_aacdec.axf
#TEST_FILES += (SYSTEM)/bin/nvmm_adtsdec.axf
#TEST_FILES += (SYSTEM)/bin/nvmm_audiomixer.axf
#TEST_FILES += (SYSTEM)/bin/nvmm_h264dec.axf
#TEST_FILES += (SYSTEM)/bin/nvmm_jpegdec.axf
#TEST_FILES += (SYSTEM)/bin/nvmm_jpegenc.axf
#TEST_FILES += (SYSTEM)/bin/nvmm_mp3dec.axf
#TEST_FILES += (SYSTEM)/bin/nvmm_mpeg4dec.axf
#TEST_FILES += (SYSTEM)/bin/nvmm_service.axf
#TEST_FILES += (SYSTEM)/bin/nvmm_manager.axf
#TEST_FILES += (SYSTEM)/bin/nvmm_sw_mp3dec.axf
#TEST_FILES += (SYSTEM)/bin/nvmm_vc1dec.axf
#TEST_FILES += (SYSTEM)/bin/nvmm_wavdec.axf
#TEST_FILES += (SYSTEM)/bin/nvmm_wmadec.axf
#TEST_FILES += (SYSTEM)/bin/nvmm_wmaprodec.axf
#TEST_FILES += (SYSTEM)/bin/ping
#TEST_FILES += (SYSTEM)/bin/pm
#TEST_FILES += (SYSTEM)/bin/pppd
#TEST_FILES += (SYSTEM)/bin/qemud
#TEST_FILES += (SYSTEM)/bin/qemu-props
#TEST_FILES += (SYSTEM)/bin/racoon
#TEST_FILES += (SYSTEM)/bin/radiooptions
#TEST_FILES += (SYSTEM)/bin/recovery
#TEST_FILES += (SYSTEM)/bin/rild
#TEST_FILES += (SYSTEM)/bin/schedtest
#TEST_FILES += (SYSTEM)/bin/sdptool
#TEST_FILES += (SYSTEM)/bin/sdutil
#TEST_FILES += (SYSTEM)/bin/showlease
#TEST_FILES += (SYSTEM)/bin/surfaceflinger
#TEST_FILES += (SYSTEM)/bin/svc
#TEST_FILES += (SYSTEM)/bin/system_server
#TEST_FILES += (SYSTEM)/bin/updater
#TEST_FILES += (SYSTEM)/bin/vold
#TEST_FILES += (SYSTEM)/bin/wpa_cli
#TEST_FILES += (SYSTEM)/bin/wpa_supplicant

TEST_FILES += (SYSTEM)/xbin/su
TEST_FILES += (SYSTEM)/xbin/ssh
TEST_FILES += (SYSTEM)/xbin/crasher
TEST_FILES += (SYSTEM)/xbin/scp
#TEST_FILES += (SYSTEM)/xbin/pppd
TEST_FILES += (SYSTEM)/xbin/showslab

TEST_FILES += (SYSTEM)/lib/libz.so
TEST_FILES += (SYSTEM)/lib/libdl.so
TEST_FILES += (SYSTEM)/lib/libc.so
TEST_FILES += (SYSTEM)/lib/libm.so
TEST_FILES += (SYSTEM)/lib/liblog.so
TEST_FILES += (SYSTEM)/lib/libcutils.so
TEST_FILES += (SYSTEM)/lib/libthread_db.so
TEST_FILES += (SYSTEM)/lib/libstdc++.so
TEST_FILES += (SYSTEM)/lib/libc_debug.so
TEST_FILES += (SYSTEM)/lib/libnv*.so

TEST_FILES += (SYSTEM)/etc/dbus.conf
TEST_FILES += (SYSTEM)/etc/init.goldfish.sh
TEST_FILES += (SYSTEM)/etc/security/otacerts.zip
TEST_FILES += (SYSTEM)/etc/apns-conf.xml
TEST_FILES += (SYSTEM)/etc/hosts
TEST_FILES += (SYSTEM)/build.prop

endif

SPECIAL_TESTS :=
#SPECIAL_TESTS += $(TEGRA_TOP)/tests/android/wlan
SPECIAL_TESTS += $(TEGRA_TOP)/tests-multimedia/nvmm/jpegenc
SPECIAL_TESTS += $(TEGRA_TOP)/tests-multimedia/nvmmlite/videoenc/hosttest
SPECIAL_TESTS += $(TEGRA_TOP)/tests-multimedia/nvmm/videodec
SPECIAL_TESTS += $(TEGRA_TOP)/tests-multimedia/nvmm/mpeg2vdec
SPECIAL_TESTS += $(TEGRA_TOP)/tests-multimedia/nvmmlite/videodec
SPECIAL_TESTS += $(TEGRA_TOP)/tests-camera/nvodm/imager/sensor
SPECIAL_TESTS += $(TEGRA_TOP)/tests-camera/nvodm/imager/focuser
#SPECIAL_TESTS += $(TEGRA_TOP)/tests/nvos # Bug 983298
SPECIAL_TESTS += $(TEGRA_TOP)/tests/nvrm/power/dfs_monitor
SPECIAL_TESTS += $(TEGRA_TOP)/tests/nvrm/unit
SPECIAL_TESTS += $(TEGRA_TOP)/tests/nvrm/channel
SPECIAL_TESTS += $(TEGRA_TOP)/tests/linux/nvlowpower
SPECIAL_TESTS += $(TEGRA_TOP)/tests/nvrm/smmu

special_tests:
	$(foreach _test,$(SPECIAL_TESTS),$(MAKE) -C $(_test) && ) true

special_tests_clean:
	$(foreach _test,$(SPECIAL_TESTS),$(MAKE) -C $(_test) clean && ) true

.PHONY: build_l4t_tests build_l4t_tests_clean

L4T_TESTS :=
L4T_TESTS += $(TEGRA_TOP)/tests-graphics/2d/nvddk/2d
L4T_TESTS += $(TEGRA_TOP)/tests-graphics/2d/nvddk/2d_v2
L4T_TESTS += $(TEGRA_TOP)/tests-graphics/nvogtest/nvogtest
L4T_TESTS += $(TEGRA_TOP)/tests-graphics/opengles/ardrv/ar20
ifeq ($(WIN_INTERFACE),x11) # These 3 are x11 only
ifneq ($(TARGET_FLOAT_ABI_HARD),1) # Bug 983386
L4T_TESTS += $(TEGRA_TOP)/tests-graphics/opengles/egl/eglpostsubbuffer
endif
L4T_TESTS += $(TEGRA_TOP)/tests-graphics/opengles/egl/eglwintest
L4T_TESTS += $(TEGRA_TOP)/tests-graphics/opengles/egl/eglx11test
endif
L4T_TESTS += $(TEGRA_TOP)/tests-graphics/opengles/egl/miscellaneous
L4T_TESTS += $(TEGRA_TOP)/tests-graphics/opengles/egl/threadsafety
ifeq ($(OGL1_SUPPORTED),1)
L4T_TESTS += $(TEGRA_TOP)/tests-graphics/opengles/opengles1
endif
ifeq ($(OGL2_SUPPORTED),1)
L4T_TESTS += $(TEGRA_TOP)/tests-graphics/opengles/opengles2
endif
ifeq ($(WIN_INTERFACE),x11) # These require x11
L4T_TESTS += $(TEGRA_TOP)/tests-multimedia/nvgstapps/nvgstplayer
ifneq ($(NV_EMBEDDED_BUILD),1) # Broken in embedded due to old gstreamer
L4T_TESTS += $(TEGRA_TOP)/tests-multimedia/nvgstapps/nvgstcapture
endif
endif
ifneq ($(NV_EMBEDDED_BUILD),1) # OMXAL is not build for embedded
L4T_TESTS += $(TEGRA_TOP)/tests-multimedia/openmax/al/alplaybacktest
endif
L4T_TESTS += $(TEGRA_TOP)/tests/linux/touchscreentest

build_l4t_tests:
	$(foreach _test,$(L4T_TESTS),$(MAKE) -C $(_test) && ) true

build_l4t_tests_clean:
	$(foreach _test,$(L4T_TESTS),$(MAKE) -C $(_test) clean && ) true

.PHONY: sanity_test_plans sanity_test_plans_clean

sanity_test_plans:
	$(MAKE) -C $(TEGRA_TOP)/tests/sanity/ats_tests default

sanity_test_plans_clean:
	$(MAKE) -C $(TEGRA_TOP)/tests/sanity/ats_tests sanity_test_plans_clean

.PHONY: build_tests build_tests_clean

build_tests: $(TEST_TARGETS)

build_tests_clean: $(TEST_TARGETS_CLEAN)

.PHONY: build_uboot build_uboot_clean

# NOTE: new boards need to have a ${TARGET_BOARD}_config for u-boot
build_uboot:
	$(MAKE) -C $(TEGRA_TOP)/3rdparty/u-boot $(UBOOT_ENV) distclean
	$(MAKE) -C $(TEGRA_TOP)/3rdparty/u-boot $(UBOOT_ENV) $(TARGET_BOARD)_config
	$(MAKE) -C $(TEGRA_TOP)/3rdparty/u-boot $(UBOOT_ENV)

build_uboot_clean:
	$(MAKE) -C $(TEGRA_TOP)/3rdparty/u-boot $(UBOOT_ENV) distclean


#
# sys_stamp.txt forces things to get rebuilt when needed
#
SYS_STAMP := $(TEGRA_TOP)/core/$(OUTDIR)/sys_stamp.txt
sys-dirty:
	rm -f $(SYS_STAMP)

default:            sys-dirty
default-tmake:      sys-dirty
drivers:            sys-dirty
daemons:            sys-dirty
kernel:             sys-dirty
special_tests:      sys-dirty


$(SYS_STAMP): Makefile
	-mkdir -p $(OUTDIR) $(@D)
	-mkdir -p $(OUTDIR)
	echo "stamp" > $@

NV_LOG_BUILD_TIME:=$(PYTHON) $(TEGRA_TOP)/core-private/make/platform_builder_utils.py --logfile $(TEGRA_TOP)/core-private/make/make_dvs_drivers.log --timer

ifneq ($(EMPTY),$(wildcard Makefile.rules.$(TARGET_OS_SUBTYPE)))
include Makefile.rules.$(TARGET_OS_SUBTYPE)
endif

# Common targets for both dvs and dvs-tmake
dvs-common: buildbct build_tests

#Buildbot uses dvs target
dvs: default nvflash dvs-common $(SRC_TARGETS)

#Tmake specific dvs target, used in tmake l4t builds
dvs-tmake: default-tmake dvs-common

#Scripts can get output directory or soc based on build env
echo-objdir:
	@echo "$(BUILD_FLAVOR)_$(DVS_NEWBUILD_CHIP_FAMILY)_$(TARGET_OS_SUBTYPE)_$(TARGET_CPU)"

echo-soc:
	@echo "$(TARGET_SOC)"

echo-kern-outdir:
	@echo "$(KERN_OUTDIR)"

include $(TEGRA_TOP)/core-private/make/Makefile.rules

# grep '^[-a-z0-9A-Z_]*:' Makefile | grep -v ':=' | sed -e 's|:.*||' | sort -u
.PHONY: buildbct _buildbct clean clobber daemons
.PHONY: daemons_clean default default-tmake drivers drivers_clean dvs-common dvs dvs-tmake
.PHONY: kernel kernel_clean kernel_menuconfig kernel_modules
.PHONY: quickboot quickboot_clean quickboot_production quickboot_production_clean
.PHONY: kernel_modules_clean kernel_rebuild kernel_update_configs
.PHONY: mobilemods _mobilemods nvflash _nvflash nvflash_list
.PHONY: special_tests special_tests_clean sweep sys-dirty test_binaries
.PHONY: echo-objdir echo-soc
