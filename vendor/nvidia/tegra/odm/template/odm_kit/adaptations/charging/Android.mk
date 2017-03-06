LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)
LOCAL_MODULE := libnvodm_charging

LOCAL_C_INCLUDES += $(TEGRA_TOP)/customers/nvidia-partner/template/odm_kit/adaptations/charging
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/system/nvaboot

LOCAL_CFLAGS += -DLPM_BATTERY_CHARGING=1
LOCAL_SRC_FILES := battery-basic.c
LOCAL_SRC_FILES += charging.c
LOCAL_SRC_FILES += charger-usb.c

include $(NVIDIA_STATIC_LIBRARY)
