LOCAL_PATH := $(call my-dir)

ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
LOCAL_COMMON_C_INCLUDES += $(TEGRA_TOP)/customers/nvidia-partner/template/odm_kit/adaptations
else
LOCAL_COMMON_C_INCLUDES += $(TEGRA_TOP)/odm/template/odm_kit/adaptations
endif

LOCAL_COMMON_SRC_FILES := nvodm_query.c
LOCAL_COMMON_SRC_FILES += nvodm_query_discovery.c
LOCAL_COMMON_SRC_FILES += nvodm_query_nand.c
LOCAL_COMMON_SRC_FILES += nvodm_query_gpio.c
LOCAL_COMMON_SRC_FILES += nvodm_query_pinmux.c
LOCAL_COMMON_SRC_FILES += nvodm_query_kbc.c
LOCAL_COMMON_SRC_FILES += nvodm_query_secure.c
LOCAL_COMMON_CFLAGS := -DLPM_BATTERY_CHARGING=1

include $(NVIDIA_DEFAULTS)
LOCAL_NVIDIA_NO_COVERAGE := true

LOCAL_MODULE := libnvodm_query
# for some reason nvodm_query needs to be arm, as it is called
# very early from the bootloader
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES += $(LOCAL_COMMON_C_INCLUDES)
LOCAL_SRC_FILES += $(LOCAL_COMMON_SRC_FILES)
LOCAL_CFLAGS += $(LOCAL_COMMON_CFLAGS)
LOCAL_SHARED_LIBRARIES += libnvrm
LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_STATIC_LIBRARIES += libnvodm_services

LOCAL_CFLAGS += -Wno-missing-field-initializers
LOCAL_CFLAGS += -Wno-missing-field-initializers
LOCAL_NVIDIA_RM_WARNING_FLAGS := -Wundef
LOCAL_NVIDIA_RM_WARNING_FLAGS := -Wundef
include $(NVIDIA_STATIC_AND_SHARED_LIBRARY)


include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_query_avp
# For some reason nvodm_query needs to be arm, as it is called
# very early from the bootloader.
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES += $(LOCAL_COMMON_C_INCLUDES)
LOCAL_SRC_FILES += $(LOCAL_COMMON_SRC_FILES)
LOCAL_CFLAGS += $(LOCAL_COMMON_CFLAGS)
LOCAL_CFLAGS += -Os -ggdb0

include $(NVIDIA_STATIC_AVP_LIBRARY)

