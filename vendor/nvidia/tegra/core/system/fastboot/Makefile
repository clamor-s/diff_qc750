#
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#

TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


TEST_NAME := fastboot
ifneq ($(filter ap20,$(TARGET_SOC)),)
CFLAGS += -DNV_AOS_ENTRY_POINT=0x108000
CFLAGS += -DNV_AOS_LOAD_ADDRESS=0x108000
else
CFLAGS += -DNV_AOS_ENTRY_POINT=0x80108000
CFLAGS += -DNV_AOS_LOAD_ADDRESS=0x80108000
endif

C_FILES :=
C_FILES += libfdt/fdt.c
C_FILES += libfdt/fdt_ro.c
C_FILES += libfdt/fdt_rw.c
C_FILES += libfdt/fdt_strerror.c
C_FILES += libfdt/fdt_sw.c
C_FILES += libfdt/fdt_wip.c
C_FILES += main.c
C_FILES += prettyprint.c
C_FILES += gui.c
C_FILES += linux_cmdline.c
C_FILES += usb.c
C_FILES += jingle.c
C_FILES += recovery_utils.c


OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

EXECUTABLES := $(OUTPUT_DIRNAME)/$(TARGET_BOARD)/$(TEST_NAME)$(EXE_SUFFIX)
EXECUTABLES += $(OUTPUT_DIRNAME)/$(TARGET_BOARD)/$(TEST_NAME)_$(TARGET_BOARD)$(EXE_SUFFIX)

POST_EXECUTABLES := $(patsubst %$(EXE_SUFFIX),%.bin,$(EXECUTABLES))

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/system/nv3p
LCINCS += -I$(TEGRA_TOP)/core/system/nv3pserver
LCINCS += -I$(TEGRA_TOP)/core/system/nvbootupdate
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvddk/disp
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core/common
LCINCS += -I$(TEGRA_TOP)/core/system/nvaboot
LCINCS += -I$(TEGRA_TOP)/core/utils/nvos/aos
LCINCS += -I$(TEGRA_TOP)/core/system/fastboot/libfdt
LCDEFS += -DNV_LDK_BUILD

TEST_LIBS :=
TEST_LIBS += $(NVLIB_NVOS_STATIC)
TEST_LIBS += $(NVLIB_NVRM)
TEST_LIBS += $(NVLIB_AVP_GPIO)
TEST_LIBS += $(NVLIB_AVP_UART)
TEST_LIBS += $(NVLIB_APPMAIN)
TEST_LIBS += $(NVLIB_NVDDK_BLOCKDEVMGR)
TEST_LIBS += $(NVLIB_NVDDK_DISP)
TEST_LIBS += $(NVLIB_NVDDK_KBC)
TEST_LIBS += $(NVLIB_NVDDK_NAND)
TEST_LIBS += $(NVLIB_NVDDK_SDIO)
TEST_LIBS += $(NVLIB_NVDDK_SNOR)
ifneq ($(TARGET_SOC),ap20)
  TEST_LIBS += $(NVLIB_NVDDK_SE)
endif
TEST_LIBS += $(NVLIB_NVDDK_SPIF)
TEST_LIBS += $(NVLIB_NV3P)
TEST_LIBS += $(TEGRA_TOP)/core-private/utils/aes_keysched_lock/$(OUTPUT_DIRNAME)/libnvaes_keysched_lock$(LIB_SUFFIX)
ifeq ($(TARGET_SOC),t30)
  TEST_LIBS += $(TEGRA_TOP)/core/utils/aes_keysched_lock/$(OUTPUT_DIRNAME)/libnvseaes_keysched_lock_avp$(LIB_SUFFIX)
endif
TEST_LIBS += $(TEGRA_TOP)/core/system/nv3pserver/$(OUTPUT_DIRNAME)/libnv3pserver$(LIB_SUFFIX)
TEST_LIBS += $(NVLIB_NVBCT_SOC)
TEST_LIBS += $(TEGRA_TOP)/core/system/nvpartmgr/$(OUTPUT_DIRNAME)/libnvpartmgr$(LIB_SUFFIX)
TEST_LIBS += $(TEGRA_TOP)/core/system/nvstormgr/$(OUTPUT_DIRNAME)/libnvstormgr$(LIB_SUFFIX)
TEST_LIBS += $(TEGRA_TOP)/core/system/nvbootupdate/$(OUTPUT_DIRNAME)/libnvbootupdate$(LIB_SUFFIX)
TEST_LIBS += $(NVLIB_NVCRYPTO)
TEST_LIBS += $(TEGRA_TOP)/core/system/utils/$(OUTPUT_DIRNAME)/libnvsystem_utils$(LIB_SUFFIX)
TEST_LIBS += $(TEGRA_TOP)/core/system/nvfsmgr/$(OUTPUT_DIRNAME)/libnvfsmgr$(LIB_SUFFIX)
TEST_LIBS += $(TEGRA_TOP)/core/system/nvfs/basic/$(OUTPUT_DIRNAME)/libnvbasicfs$(LIB_SUFFIX)
TEST_LIBS += $(TEGRA_TOP)/core/system/nvfs/enhanced/$(OUTPUT_DIRNAME)/libnvenhancedfs$(LIB_SUFFIX)
TEST_LIBS += $(TEGRA_TOP)/core/system/nvfs/ext2/$(OUTPUT_DIRNAME)/libnvext2fs$(LIB_SUFFIX)
TEST_LIBS += $(TEGRA_TOP)/core/system/nvfs/yaffs2/$(OUTPUT_DIRNAME)/libnvyaffs2fs$(LIB_SUFFIX)
TEST_LIBS += $(TEGRA_TOP)/core/system/nvaboot/$(OUTPUT_DIRNAME)/libnvaboot$(LIB_SUFFIX)
ifeq ($(NV_EMBEDDED_BUILD),1)
TEST_LIBS += $(TEGRA_TOP)/core/system/nvsku/$(OUTPUT_DIRNAME)/libnvsku$(LIB_SUFFIX)
endif
TEST_LIBS += $(TEGRA_TOP)/core/system/nvdiagnostics/$(OUTPUT_DIRNAME)/libnvdiagnostics$(LIB_SUFFIX)
ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
TEST_LIBS += $(TEGRA_TOP)/core/drivers/nvpinmux/$(OUTPUT_DIRNAME)/libnvpinmux$(LIB_SUFFIX)
endif
TEST_LIBS += $(NVLIB_NVDDK_AES)
TEST_LIBS += $(NVLIB_NVODM_SCROLLWHEEL)
TEST_LIBS += $(NVLIB_NVODM_KEYBOARD)
TEST_LIBS += $(NVLIB_NVODM_DISP)
TEST_LIBS += $(NVLIB_NVODM_MISC)
TEST_LIBS += $(NVLIB_NVODM_QUERY)
TEST_LIBS += $(NVLIB_NVODM_AUDIOCODEC)
TEST_LIBS += $(NVLIB_NVDDK_I2S)
TEST_LIBS += $(NVLIB_NVODM_PMU)
TEST_LIBS += $(NVLIB_NVODM_FUELGAUGEFWUPGRADE)
ifneq ($(NV_EMBEDDED_BUILD),1)
ifeq ($(NVODM_BOARD_IS_FPGA),)
ifeq ($(NVODM_BOARD_IS_SIMULATION),)
LCDEFS += -DLPM_BATTERY_CHARGING=1
TEST_LIBS += $(NVLIB_NVODM_CHARGING)
endif
endif
endif
ifneq ($(TARGET_OS),rvds)
TEST_LIBS += $(NVLIB_NVODM_AVP)
endif
TEST_LIBS += $(NVLIB_FXMATH)
TEST_LIBS += $(NVLIB_NVODM_EXTFSMGR_LINUX)
TEST_LIBS += $(NVLIB_NVODM_EXTFS_LINUX)
TEST_LIBS += $(NVLIB_NVODM_OTA)
TEST_LIBS += $(NVLIB_NVDIAGNOSTICS)

#TEST_LIBS += $(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core/minikernel/$(OUTPUT_DIRNAME)/libnvrm_minikernel$(LIB_SUFFIX)

TEST_LIBS += $(NVLIB_NVODM_SERVICES)

ifeq ($(TARGET_OS)-$(TARGET_OS_SUBTYPE),linux-aos)
  TEST_LIBS += $(ANDROID_ROOT)/prebuilt/linux-x86/toolchain/arm-eabi-$(GCC_TOOLCHAIN_VERSION)/lib/gcc/arm-eabi/$(GCC_TOOLCHAIN_VERSION)/libgcc.a
 ANDROID_LIBC := obj/STATIC_LIBRARIES/libc_intermediates/libc.a

  ifeq ($(NV_ANDROID_BRANCH),donut)
    ANDROID_LIBC := tegra/$(ANDROID_LIBC)
  else
    ANDROID_LIBC := $(TARGET_BOARD)/$(ANDROID_LIBC)
  endif

  ifeq ($(BUILD_FLAVOR),debug)
    ANDROID_LIBC := debug/$(ANDROID_LIBC)
  endif

  TEST_LIBS += $(ANDROID_ROOT)/out/target/product/$(ANDROID_LIBC)
endif
TEST_LIBS += $(NVLIB_NVDDK_FUSE)

ifneq (,$(filter $(TARGET_OS)-$(TARGET_OS_SUBTYPE)-$(TARGET_CPU),linux-aos-armv6 rvds-rvds-armv6 eabi-aos-armv6))

ifeq ($(TARGET_OS)-$(TARGET_CPU),rvds-armv6)
  LINKOPTS += --keep nvaosGetCommandLine
  LINKOPTS += --keep nvaosRegisterSemiRvice
  TEST_LIBS += $(TEGRA_TOP)/core/utils/nvos/$(OUTDIR)/aos_process_args.o
  TEST_LIBS += $(TEGRA_TOP)/core/utils/nvos/$(OUTDIR)/aos_semi_rvice.o
endif

default: avp $(POST_EXECUTABLES)
	$(NV_INSTALL) -l $(EXECUTABLES) $(OUTDIR)
	$(NV_INSTALL) -l -f $(EXECUTABLES) $(INSTALL_TARGET)
	$(NV_INSTALL) -l -f $(patsubst %$(EXE_SUFFIX),%.bin,$(EXECUTABLES)) $(INSTALL_TARGET)

avp:
ifneq ($(NVODMDIR_ODM_AVP),)
ifeq ($(TARGET_CPU),armv6)
	$(MAKE) -C $(NVODMDIR_ODM_AVP) TARGET_CPU=armv4
endif
endif

$(EXECUTABLES): $(OBJS) $(OUTDIR)/$(TARGET_BOARD)/dummy.txt
$(EXECUTABLES): $(TEST_LIBS)
$(EXECUTABLES): $(SCATTER_FILE)

$(POST_EXECUTABLES): $(EXECUTABLES)
endif

nosupport:
	@echo "fastboot not supported on this platform"

RELEASE_PACKAGE_TARGETS := $(EXECUTABLES)
RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS) $(C_FILES) $(SCATTER_FILE)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
