LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_query
# for some reason nvodm_query needs to be arm, as it is called
# very early from the bootloader
LOCAL_ARM_MODE := arm

ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
LOCAL_C_INCLUDES += $(TEGRA_TOP)/customers/nvidia-partner/template/odm_kit/adaptations
LOCAL_C_INCLUDES += $(TEGRA_TOP)/customers/nvidia-partner/template
LOCAL_C_INCLUDES += $(TEGRA_TOP)/customers/nvidia-partner/p1852/odm_kit/adaptations/pmu
else
LOCAL_C_INCLUDES += $(TEGRA_TOP)/odm/template/odm_kit/adaptations
LOCAL_C_INCLUDES += $(TEGRA_TOP)/odm/template
LOCAL_C_INCLUDES += $(TEGRA_TOP)/odm/p1852/odm_kit/adaptations/pmu
endif

LOCAL_CFLAGS += -DFPGA_BOARD
LOCAL_SRC_FILES += nvodm_query.c
LOCAL_SRC_FILES += nvodm_query_discovery.c
LOCAL_SRC_FILES += nvodm_query_nand.c
LOCAL_SRC_FILES += nvodm_query_gpio.c
LOCAL_SRC_FILES += nvodm_query_pinmux.c
LOCAL_SRC_FILES += nvodm_query_kbc.c
LOCAL_SRC_FILES += nvodm_platform_init.c
LOCAL_SRC_FILES += secure/nvodm_query_secure.c

LOCAL_SHARED_LIBRARIES += libnvrm
LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_STATIC_LIBRARIES += libnvodm_services

LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
include $(NVIDIA_STATIC_AND_SHARED_LIBRARY)

