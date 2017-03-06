#
# Copyright (c) 2011-2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#

LOCAL_PATH := $(call my-dir)

ifeq ($(BOARD_BUILD_BOOTLOADER),true)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := microboot

# Place microboot bootloader at root of output hierarchy.
LOCAL_MODULE_PATH := $(PRODUCT_OUT)

# AP15 and AP20 use the legacy address map with SDRAM at address zero.
# Everything else will use the newer address map.
ifneq ($(filter ap15 ap20, $(TARGET_TEGRA_VERSION)),)
LOCAL_CFLAGS += -DNV_AOS_ENTRY_POINT=0x00108000
else
LOCAL_CFLAGS += -DNV_AOS_ENTRY_POINT=0x80108000
endif
ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LOCAL_CFLAGS += -DSET_KERNEL_PINMUX
endif

LOCAL_NO_DEFAULT_COMPILER_FLAGS := true

LOCAL_NVIDIA_OBJCOPY_FLAGS += -R USB_BUFFER1 -R HEAP_START -R USB_BUFFER2 -R .stack

LOCAL_CFLAGS := -march=armv4t
LOCAL_CFLAGS += -Os
LOCAL_CFLAGS += -fno-unwind-tables
LOCAL_CFLAGS += -fomit-frame-pointer
LOCAL_CFLAGS += -ffunction-sections
LOCAL_CFLAGS += -fdata-sections
ifeq ($(AVP_EXTERNAL_TOOLCHAIN),)
  LOCAL_CFLAGS += -mno-thumb
else
  LOCAL_CFLAGS += -mthumb-interwork
  LOCAL_CFLAGS += -mthumb
endif
LOCAL_CFLAGS += -DNO_MALLINFO
LOCAL_CFLAGS += $(TEGRA_CFLAGS)

LOCAL_SRC_FILES := microboot.c

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvboot/$(TARGET_TEGRA_VERSION)/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvboot/$(TARGET_TEGRA_VERSION)/include/$(TARGET_TEGRA_VERSION)
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/utils/nvos/aos
LOCAL_C_INCLUDES += $(TOP)/bionic/libc/arch-arm/include
LOCAL_C_INCLUDES += $(TOP)/bionic/libc/include
LOCAL_C_INCLUDES += $(TOP)/bionic/libc/kernel/common
LOCAL_C_INCLUDES += $(TOP)/bionic/libc/kernel/arch-arm


LOCAL_STATIC_LIBRARIES := libnvodm_pmu_avp
ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LOCAL_STATIC_LIBRARIES += libnvpinmux_avp
endif
LOCAL_STATIC_LIBRARIES += libnvodm_tmon_avp
LOCAL_STATIC_LIBRARIES += libnvodm_query_avp
LOCAL_STATIC_LIBRARIES += libnvbct_avp
LOCAL_STATIC_LIBRARIES += libnvodm_services_avp
LOCAL_STATIC_LIBRARIES += libavpmain_avp
LOCAL_STATIC_LIBRARIES += libnvboot_sdram_avp
LOCAL_STATIC_LIBRARIES += libnvboot_clocks_avp
LOCAL_STATIC_LIBRARIES += libnvboot_fuse_avp
LOCAL_STATIC_LIBRARIES += libnvboot_reset_avp
LOCAL_STATIC_LIBRARIES += libnvboot_nand_avp
LOCAL_STATIC_LIBRARIES += libnvboot_util_avp
LOCAL_STATIC_LIBRARIES += libnvboot_pads_avp
LOCAL_STATIC_LIBRARIES += libnvboot_spi_flash_avp
LOCAL_STATIC_LIBRARIES += libnvboot_pmc_avp
LOCAL_STATIC_LIBRARIES += libnvaes_keysched_lock_avp
LOCAL_STATIC_LIBRARIES += libnvmicroboot_avp
LOCAL_STATIC_LIBRARIES += libnvavpuart
ifneq ($(filter t30, $(TARGET_TEGRA_VERSION)),)
LOCAL_STATIC_LIBRARIES += libnvboot_ahb
LOCAL_STATIC_LIBRARIES += libnvboot_irom_patch
LOCAL_STATIC_LIBRARIES += libnvseaes_keysched_lock_avp
endif

# Potentially override the default compiler.  Froyo arm-eabi-4.4.0 toolchain
# does not produce valid code for ARMv4T ARM/Thumb interworking.
ifneq ($(AVP_EXTERNAL_TOOLCHAIN),)
  LOCAL_CC := $(AVP_EXTERNAL_TOOLCHAIN)gcc
endif

LOCAL_NVIDIA_LINK_SCRIPT_PATH := $(LOCAL_PATH)/$(TARGET_TEGRA_VERSION)

include $(NVIDIA_STATIC_AVP_EXECUTABLE)
endif
