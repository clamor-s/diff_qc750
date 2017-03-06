#
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
LOCAL_PATH := $(call my-dir)

#
# libcplconnectclient.so
#
include $(NVIDIA_DEFAULTS)
LOCAL_ARM_MODE := arm
LOCAL_MODULE := libcplconnectclient
LOCAL_SHARED_LIBRARIES := libcutils libutils libbinder
LOCAL_SRC_FILES := cplconnectclient/cplconnectclient.cpp
LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
LOCAL_PRELINK_MODULE := false
include $(NVIDIA_SHARED_LIBRARY)

include $(NVIDIA_DEFAULTS)
COMMON_SRC_FILES :=
COMMON_SRC_FILES += nvos_debug.c
COMMON_SRC_FILES += nvos_internal.c
COMMON_SRC_FILES += nvos_alloc.c
COMMON_SRC_FILES += nvos_pointer_hash.c
COMMON_SRC_FILES += nvos_thread.c
COMMON_SRC_FILES += nvos_coop_thread.c
COMMON_SRC_FILES += nvos_trace.c
COMMON_SRC_FILES += nvos_file.c

# linux usermode implementation

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvos
LOCAL_ARM_MODE := arm

# AP20 uses the legacy address map with SDRAM at address zero.
# Everything else will use the newer address map.
ifneq ($(filter ap20, $(TARGET_TEGRA_VERSION)),)
LOCAL_CFLAGS += -DNV_AOS_ENTRY_POINT=0x00108000
else
LOCAL_CFLAGS += -DNV_AOS_ENTRY_POINT=0x80108000
endif
LOCAL_CFLAGS += -DANDROID_LOG_ADB=1
# Advanced debug feature disabled until Bug 950465 fixed
LOCAL_CFLAGS += -DNVOS_ADVANCED_DEBUG=0

ifneq ($(filter dolak_sim curacao_sim bonaire_sim ,$(TARGET_PRODUCT)),)
LOCAL_CFLAGS += -DBUILD_FOR_COSIM
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/cplconnectclient

LOCAL_SRC_FILES += $(COMMON_SRC_FILES)
LOCAL_SRC_FILES += nvos_config.c
LOCAL_SRC_FILES += linux/nvos_linux.c
LOCAL_SRC_FILES += linux/nvos_linux_stub.c
LOCAL_SRC_FILES += linux/nvos_linux_user.c
LOCAL_SRC_FILES += linux/nvos_linux_debugcomm.c
LOCAL_SRC_FILES += linux/nvos_linux_settings.c
LOCAL_SRC_FILES += linux/nvos_main.c

LOCAL_SHARED_LIBRARIES += libdl
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libcplconnectclient
LOCAL_SHARED_LIBRARIES += libcutils libutils libbinder
LOCAL_WHOLE_STATIC_LIBRARIES += libnvosutils

# Turning off warnings as errors as getting following:
# vendor/nvidia/tegra/core/utils/nvos/nvos_file.c: In function 'NvOsSetFileHooks':
# vendor/nvidia/tegra/core/utils/nvos/nvos_file.c:80: error: dereferencing type-punned pointer will break strict-aliasing rules
# Should be removed after fixing the source

LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
LOCAL_NVIDIA_NO_EXTRA_WARNINGS := 1
include $(NVIDIA_SHARED_LIBRARY)

# host static library

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvos
# Advanced debug feature disabled until Bug 950465 fixed
LOCAL_CFLAGS += -DNVOS_ADVANCED_DEBUG=0
LOCAL_CFLAGS += -UANDROID

LOCAL_SRC_FILES += $(COMMON_SRC_FILES)
LOCAL_SRC_FILES += nvos_config.c
LOCAL_SRC_FILES += linux/nvos_linux.c
LOCAL_SRC_FILES += linux/nvos_linux_host_stub.c
LOCAL_SRC_FILES += linux/nvos_linux_user.c
LOCAL_SRC_FILES += linux/nvos_linux_debugcomm.c
LOCAL_SRC_FILES += linux/nvos_linux_settings.c

LOCAL_WHOLE_STATIC_LIBRARIES += libnvosutils

include $(NVIDIA_HOST_STATIC_LIBRARY)

# bootloader libgcc implementation

include $(NVIDIA_DEFAULTS)
LOCAL_NVIDIA_NO_COVERAGE := true

LOCAL_MODULE := libnvos_aos_libgcc_avp
LOCAL_ARM_MODE := arm
LOCAL_NO_DEFAULT_COMPILER_FLAGS := true

LOCAL_CFLAGS :=
LOCAL_CFLAGS += -DANDROID

LOCAL_SRC_FILES := aos/nvos_aos_libgcc.c

include $(NVIDIA_STATIC_AVP_LIBRARY)

# bootloader aos implementation

include $(NVIDIA_DEFAULTS)
LOCAL_NVIDIA_NO_COVERAGE := true

LOCAL_MODULE := libnvos_aos
LOCAL_ARM_MODE := arm
LOCAL_NO_DEFAULT_COMPILER_FLAGS := true

ifneq ($(filter ap20,$(TARGET_TEGRA_VERSION)),)
LOCAL_CFLAGS += -DNV_AOS_ENTRY_POINT=0x00108000
LOCAL_CFLAGS += -DAES_KEYSCHED_LOCK_WAR_BUG_598910=1
else
LOCAL_CFLAGS += -DNV_AOS_ENTRY_POINT=0x80108000
LOCAL_CFLAGS += -DAES_KEYSCHED_LOCK_WAR_BUG_598910=0
endif

ifeq ($(AOS_MON_MODE_ENABLE),1)
LOCAL_CFLAGS += -DAOS_MON_MODE_ENABLE=1
else
LOCAL_CFLAGS += -DAOS_MON_MODE_ENABLE=0
endif

LOCAL_CFLAGS += -DNVAOS_SHELL=0
LOCAL_CFLAGS += -DNO_MALLINFO
LOCAL_CFLAGS += -DNVOS_IS_LINUX=0

LOCAL_CFLAGS += -march=armv7-a
LOCAL_CFLAGS += -DCOMPILE_ARCH_V7=1
LOCAL_CFLAGS += -mthumb-interwork
LOCAL_CFLAGS += -mfloat-abi=softfp

ifeq ($(ARCH_ARM_HAVE_NEON),true)
LOCAL_CFLAGS += -mfpu=neon
LOCAL_CFLAGS += -DARCH_ARM_HAVE_NEON=1
endif
LOCAL_CFLAGS += -ffunction-sections
LOCAL_CFLAGS += -fno-exceptions
LOCAL_CFLAGS += -fpic
LOCAL_CFLAGS += -funwind-tables
LOCAL_CFLAGS += -fstack-protector
LOCAL_CFLAGS += -fno-short-enums
LOCAL_CFLAGS += -g
ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LOCAL_CFLAGS += -DSET_KERNEL_PINMUX
endif

LOCAL_C_INCLUDES += $(TARGET_PROJECT_INCLUDES)
LOCAL_C_INCLUDES += $(TARGET_C_INCLUDES)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/aos
LOCAL_C_INCLUDES += $(LOCAL_PATH)/aos/nvap
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/hwinc
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/hwinc/$(TARGET_TEGRA_VERSION)
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/utils/aes_keysched_lock

LOCAL_SRC_FILES += $(COMMON_SRC_FILES)
LOCAL_SRC_FILES += aos/nvos_aos_core.c
LOCAL_SRC_FILES += aos/nvos_aos.c
LOCAL_SRC_FILES += aos/dlmalloc.c
LOCAL_SRC_FILES += aos/aos_process_args.c
LOCAL_SRC_FILES += aos/aos_profile.c
LOCAL_SRC_FILES += aos/aos_semi_rvice.c
LOCAL_SRC_FILES += aos/aos_semi_uart.c
LOCAL_SRC_FILES += aos/aos_print.c
LOCAL_SRC_FILES += aos/nvos_aos_semi.c
LOCAL_SRC_FILES += aos/nvap/init_fpu.c
LOCAL_SRC_FILES += aos/nvap/aos_cpu.c
LOCAL_SRC_FILES += aos/nvap/aos_cpu_t30.c
LOCAL_SRC_FILES += aos/nvap/aos_pl310.c
LOCAL_SRC_FILES += aos/nvap/aos_scu.c
LOCAL_SRC_FILES += aos/nvap/init_cpu.c
LOCAL_SRC_FILES += aos/nvap/aos_odmoption_ap20.c
LOCAL_SRC_FILES += aos/nvap/aos_odmoption_t30.c
LOCAL_SRC_FILES += aos/nvap/nvos_aos_gcc.c
LOCAL_SRC_FILES += aos/nvap/nvos_aos_libc.c
LOCAL_SRC_FILES += aos/nvap/aos_uart.c
ifneq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
ifneq ($(filter ap20,$(TARGET_TEGRA_VERSION)),)
LOCAL_SRC_FILES += aos/nvap/aos_uart_ap20.c
else
LOCAL_SRC_FILES += aos/nvap/aos_uart_t30.c
endif
endif
LOCAL_SRC_FILES += aos/nvap/aos_cpu_odmstub.c
LOCAL_SRC_FILES += aos/nvap/aos_mon_mode.c
LOCAL_SRC_FILES += aos/nvap/aos_ns_mode.c

LOCAL_WHOLE_STATIC_LIBRARIES += libnvosutils
LOCAL_WHOLE_STATIC_LIBRARIES += libnvos_aos_libgcc_avp

# Turning of warnings as errors as getting:
# vendor/nvidia/tegra/core/utils/nvos/aos/nvap/aos_uart.c:145: error: assignment from incompatible pointer type
# vendor/nvidia/tegra/core/utils/nvos/aos/nvap/aos_uart.c:150: error: assignment from incompatible pointer type
# Source should be fixed and following flags should be removed

LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
LOCAL_NVIDIA_NO_EXTRA_WARNINGS := 1

include $(NVIDIA_STATIC_LIBRARY)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvos_aos_avp
LOCAL_ARM_MODE := arm
LOCAL_NO_DEFAULT_COMPILER_FLAGS := true

ifneq ($(filter ap20,$(TARGET_TEGRA_VERSION)),)
LOCAL_CFLAGS += -DAVP_CACHE_2X=0
LOCAL_CFLAGS += -DNV_AOS_ENTRY_POINT=0x00108000
LOCAL_CFLAGS += -DAES_KEYSCHED_LOCK_WAR_BUG_598910=1
else
LOCAL_CFLAGS += -DAVP_CACHE_2X=1
LOCAL_CFLAGS += -DNV_AOS_ENTRY_POINT=0x80108000
LOCAL_CFLAGS += -DAES_KEYSCHED_LOCK_WAR_BUG_598910=0
endif

ifneq ($(filter t30,$(TARGET_TEGRA_VERSION)),)
LOCAL_CFLAGS += -DSE_AES_KEYSCHED_READ_LOCK=1
else
LOCAL_CFLAGS += -DSE_AES_KEYSCHED_READ_LOCK=0
endif

ifeq ($(TARGET_DEVICE),cardhu)
LOCAL_CFLAGS += -DSET_I2C_EXPANDER
endif

ifeq ($(TARGET_DEVICE),kai)
LOCAL_CFLAGS += -DBOARD_IS_KAI
endif
ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LOCAL_CFLAGS += -DSET_KERNEL_PINMUX
endif

ifeq ($(TARGET_DEVICE),nabi2_xd)
LOCAL_CFLAGS += -DBOARD_IS_NABI2
endif

ifeq ($(TARGET_DEVICE),nabi_2s)
LOCAL_CFLAGS += -DBOARD_IS_NABI2
endif

ifeq ($(TARGET_DEVICE),mt799)
LOCAL_CFLAGS += -DBOARD_IS_NABI2
endif

ifeq ($(TARGET_DEVICE),nabi2_3d)
LOCAL_CFLAGS += -DBOARD_IS_NABI2
endif

ifeq ($(TARGET_DEVICE),cm9000)
LOCAL_CFLAGS += -DBOARD_IS_NABI2
endif

ifeq ($(TARGET_DEVICE),qc750)
LOCAL_CFLAGS += -DBOARD_IS_NABI2
endif

ifeq ($(TARGET_DEVICE),n710)
LOCAL_CFLAGS += -DBOARD_IS_NABI2
endif

ifeq ($(TARGET_DEVICE),mm3201)
LOCAL_CFLAGS += -DBOARD_IS_NABI2
endif
ifeq ($(TARGET_DEVICE),itq700)
LOCAL_CFLAGS += -DBOARD_IS_NABI2
endif

ifeq ($(TARGET_DEVICE),n1010)
LOCAL_CFLAGS += -DBOARD_IS_NABI2
endif

ifeq ($(TARGET_DEVICE),birch)
LOCAL_CFLAGS += -DBOARD_IS_NABI2
endif
ifeq ($(TARGET_DEVICE),n750)
LOCAL_CFLAGS += -DBOARD_IS_NABI2
endif

ifeq ($(TARGET_DEVICE),wikipad)
LOCAL_CFLAGS += -DBOARD_IS_NABI2
endif

ifeq ($(TARGET_DEVICE),ns_14t004)
LOCAL_CFLAGS += -DBOARD_IS_NABI2
endif
LOCAL_C_INCLUDES += $(TARGET_PROJECT_INCLUDES)
LOCAL_C_INCLUDES += $(TARGET_C_INCLUDES)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/aos
LOCAL_C_INCLUDES += $(LOCAL_PATH)/aos/nvap
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/hwinc
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/utils/aes_keysched_lock
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvavpgpio
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/hwinc/$(TARGET_TEGRA_VERSION)

LOCAL_SRC_FILES :=
LOCAL_SRC_FILES += aos/nvap/bootloader.c
LOCAL_SRC_FILES += aos/nvap/bootloader_ap20.c
LOCAL_SRC_FILES += aos/nvap/bootloader_t30.c

LOCAL_WHOLE_STATIC_LIBRARIES += libnvosutils

include $(NVIDIA_STATIC_AVP_LIBRARY)
