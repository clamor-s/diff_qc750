#
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#

LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_services

ifeq ($(TARGET_DEVICE),cardhu)
LOCAL_CFLAGS += -DSKU_FUSEREAD=1
else
LOCAL_CFLAGS += -DSKU_FUSEREAD=0
endif

ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LOCAL_CFLAGS += -DSET_KERNEL_PINMUX
endif

LOCAL_SRC_FILES += nvodm_services.c
LOCAL_SRC_FILES += nvodm_services_os.c
LOCAL_SRC_FILES += nvodm_services_ext.c
LOCAL_SRC_FILES += nvodm_services_common.c
LOCAL_SRC_FILES += nvodm_services_os_common.c

LOCAL_CFLAGS += -Wno-error=enum-compare

include $(NVIDIA_STATIC_LIBRARY)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_services_os_avp

ifneq ($(filter cardhu, $(TARGET_DEVICE)),)
LOCAL_CFLAGS += -DSKU_FUSEREAD=1
else
LOCAL_CFLAGS += -DSKU_FUSEREAD=0
endif

ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LOCAL_CFLAGS += -DSET_KERNEL_PINMUX
endif

LOCAL_SRC_FILES += nvodm_services.c
LOCAL_SRC_FILES += nvodm_services_os.c
LOCAL_SRC_FILES += nvodm_services_ext.c
LOCAL_SRC_FILES += nvodm_services_common.c
LOCAL_SRC_FILES += nvodm_services_os_common.c

include $(NVIDIA_STATIC_AVP_LIBRARY)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_services_avp

LOCAL_CFLAGS += -Os -ggdb0
ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LOCAL_CFLAGS += -DSET_KERNEL_PINMUX
endif

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvboot/$(TARGET_TEGRA_VERSION)/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core-private/drivers/nvboot/$(TARGET_TEGRA_VERSION)/include
ifeq ($(TARGET_TEGRA_VERSION),t30)
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/hwinc/$(TARGET_TEGRA_VERSION)
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvboot/$(TARGET_TEGRA_VERSION)/include/$(TARGET_TEGRA_VERSION)
endif

LOCAL_SRC_FILES :=
ifneq ($(filter ap15 ap20, $(TARGET_TEGRA_VERSION)),)
LOCAL_SRC_FILES += avp/nvodm_services_avp.c
endif
ifeq ($(TARGET_TEGRA_VERSION),t30)
LOCAL_SRC_FILES += avp/nvodm_services_avp_spi_$(TARGET_TEGRA_VERSION).c
LOCAL_SRC_FILES += avp/nvodm_services_avp_i2c_$(TARGET_TEGRA_VERSION).c
LOCAL_SRC_FILES += avp/nvodm_services_avp_gpio_$(TARGET_TEGRA_VERSION).c
LOCAL_SRC_FILES += avp/nvodm_services_avp_pmu_$(TARGET_TEGRA_VERSION).c
LOCAL_SRC_FILES += avp/nvodm_services_avp_misc_$(TARGET_TEGRA_VERSION).c
endif
LOCAL_SRC_FILES += nvodm_services_os.c
LOCAL_SRC_FILES += nvodm_services_os_common.c

include $(NVIDIA_STATIC_AVP_LIBRARY)
