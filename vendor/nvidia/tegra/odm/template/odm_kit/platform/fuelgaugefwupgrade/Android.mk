LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_fuelgaugefwupgrade
LOCAL_NVIDIA_NO_COVERAGE := true

LOCAL_C_INCLUDES += $(TEGRA_TOP)/customers/nvidia-partner/harmony/odm_kit/platform/fuelgaugefwupgrade

LOCAL_SRC_FILES += nvodm_fuelgaugefwupgrade.c

include $(NVIDIA_STATIC_LIBRARY)
