#
# Copyright (c) 2010-2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
_avp_local_path := $(call my-dir)

# We need to generate ARMv4T code, not Thumb code and Thumb is the default.
# Some expressions and instructions we use are not compatible with Thumb.
_avp_local_arm_mode := arm

# Determine PLLP base freq to use. for t30+, use 408MHz.
ifneq ($(EMPTY),$(filter $(TARGET_SOC), ap20))
    CONFIG_PLLP_BASE_AS_408MHZ := 0
endif
CONFIG_PLLP_BASE_AS_408MHZ ?= 1

_avp_local_cflags := -DNV_IS_AVP=1
_avp_local_cflags += -DNVAOS_SHELL=1
_avp_local_cflags += -DCONFIG_PLLP_BASE_AS_408MHZ=$(CONFIG_PLLP_BASE_AS_408MHZ)

# Enabling the DisableAesKeyScheduleRead in microboot to lock all the Key
# Schedules for A03 chips in AP20
ifneq ($(filter ap20,$(TARGET_TEGRA_VERSION)),)
_avp_local_cflags += -DAES_KEYSCHED_LOCK_WAR_BUG_598910=1
else
_avp_local_cflags += -DAES_KEYSCHED_LOCK_WAR_BUG_598910=0
endif

ifneq ($(filter t30,$(TARGET_TEGRA_VERSION)),)
_avp_local_cflags += -DSE_AES_KEYSCHED_READ_LOCK=1
else
_avp_local_cflags += -DSE_AES_KEYSCHED_READ_LOCK=0
endif
ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
_avp_local_cflags += -DSET_KERNEL_PINMUX
endif
ifeq ($(HOST_OS),darwin)
_avp_local_cflags += -DNVML_BYPASS_PRINT=1
endif

_avp_local_c_includes := $(TEGRA_TOP)/core/utils/nvos/include
_avp_local_c_includes += $(TEGRA_TOP)/core/utils/nvos
_avp_local_c_includes += $(TEGRA_TOP)/core/utils/nvos/aos
_avp_local_c_includes += $(TEGRA_TOP)/core/drivers/include
_avp_local_c_includes += $(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core
_avp_local_c_includes += $(TEGRA_TOP)/core-private/include
_avp_local_c_includes += $(TEGRA_TOP)/core-private/drivers/hwinc
_avp_local_c_includes += $(TEGRA_TOP)/core-private/drivers/hwinc/$(TARGET_TEGRA_VERSION)
_avp_local_c_includes += $(TEGRA_TOP)/core/drivers/hwinc/$(TARGET_TEGRA_VERSION)
_avp_local_c_includes += $(TOP)/bionic/libc/arch-arm/include
_avp_local_c_includes += $(TOP)/bionic/libc/include
_avp_local_c_includes += $(TOP)/bionic/libc/kernel/common
_avp_local_c_includes += $(TOP)/bionic/libc/kernel/arch-arm

# We are not allowed to use ../ in LOCAL_SRC_FILES
LOCAL_PATH := $(_avp_local_path)/..

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libavpmain_avp_aos

LOCAL_ARM_MODE   := $(_avp_local_arm_mode)
LOCAL_C_INCLUDES += $(_avp_local_c_includes)
LOCAL_CFLAGS     := $(_avp_local_cflags)
LOCAL_CFLAGS     += $(TEGRA_CFLAGS)

LOCAL_SRC_FILES += aos_print.c
LOCAL_SRC_FILES += aos_process_args.c
LOCAL_SRC_FILES += aos_profile.c
LOCAL_SRC_FILES += aos_semi_rvice.c
LOCAL_SRC_FILES += aos_semi_uart.c
LOCAL_SRC_FILES += dlmalloc.c
LOCAL_SRC_FILES += nvos_aos.c
LOCAL_SRC_FILES += nvos_aos_core.c
LOCAL_SRC_FILES += nvos_aos_libgcc.c
LOCAL_SRC_FILES += nvos_aos_semi.c
LOCAL_SRC_FILES += nvap/aos_avp.c
LOCAL_SRC_FILES += nvap/aos_avp_cache_t30.c
LOCAL_SRC_FILES += nvap/aos_avp_t30.c
LOCAL_SRC_FILES += nvap/avp_override.S
LOCAL_SRC_FILES += nvap/init_avp.c
LOCAL_SRC_FILES += nvap/nvos_aos_gcc_avp.c
LOCAL_SRC_FILES += nvap/nvos_aos_libc.c

include $(NVIDIA_STATIC_AVP_LIBRARY)

# We are not allowed to use ../ in LOCAL_SRC_FILES
LOCAL_PATH := $(_avp_local_path)/../..

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libavpmain_avp_nvos

LOCAL_ARM_MODE   := $(_avp_local_arm_mode)
LOCAL_C_INCLUDES += $(_avp_local_c_includes)
LOCAL_CFLAGS     := $(_avp_local_cflags)
LOCAL_CFLAGS     += $(TEGRA_CFLAGS)

LOCAL_SRC_FILES += nvos_alloc.c
LOCAL_SRC_FILES += nvos_debug.c
LOCAL_SRC_FILES += nvos_file.c
LOCAL_SRC_FILES += nvos_pointer_hash.c
LOCAL_SRC_FILES += nvos_thread_no_coop.c
LOCAL_SRC_FILES += nvos_trace.c

include $(NVIDIA_STATIC_AVP_LIBRARY)

# We are not allowed to use ../ in LOCAL_SRC_FILES
LOCAL_PATH := $(_avp_local_path)/../../../nvosutils

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libavpmain_avp_nvosutils

LOCAL_ARM_MODE   := $(_avp_local_arm_mode)
LOCAL_C_INCLUDES += $(_avp_local_c_includes)
LOCAL_CFLAGS     := $(_avp_local_cflags)
LOCAL_CFLAGS     += $(TEGRA_CFLAGS)

LOCAL_SRC_FILES += nvustring.c
LOCAL_SRC_FILES += nvuhash.c

include $(NVIDIA_STATIC_AVP_LIBRARY)

LOCAL_PATH := $(_avp_local_path)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libavpmain_avp

LOCAL_ARM_MODE   := $(_avp_local_arm_mode)
LOCAL_C_INCLUDES += $(_avp_local_c_includes)
LOCAL_CFLAGS     := $(_avp_local_cflags)
LOCAL_CFLAGS     += $(TEGRA_CFLAGS)

LOCAL_SRC_FILES += aos_avpdebugsemi_stub.c
LOCAL_WHOLE_STATIC_LIBRARIES += libavpmain_avp_aos
LOCAL_WHOLE_STATIC_LIBRARIES += libavpmain_avp_nvos
LOCAL_WHOLE_STATIC_LIBRARIES += libavpmain_avp_nvosutils

include $(NVIDIA_STATIC_AVP_LIBRARY)

# variable cleanup
_avp_local_arm_mode   :=
_avp_local_c_includes :=
_avp_local_cflags     :=
_avp_local_path       :=
