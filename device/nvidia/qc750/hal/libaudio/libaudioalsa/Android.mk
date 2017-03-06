# hardware/libaudio-alsa/Android.mk
ifeq ($(TARGET_DEVICE),qc750)

ifneq ($(strip $(BOARD_USES_GENERIC_AUDIO)),true)
ifeq ($(strip $(BOARD_USES_ALSA_AUDIO)),true)

LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)

LOCAL_CFLAGS := -D_POSIX_SOURCE -Wno-multichar

LOCAL_SRC_FILES :=
LOCAL_SRC_FILES += nvaudioalsa.cpp

LOCAL_SHARED_LIBRARIES := \
        libasound \
        libcutils \
        libutils

LOCAL_C_INCLUDES += external/alsa-lib/include
LOCAL_C_INCLUDES += vendor/nvidia/tegra/multimedia-partner/android/libaudio/libaudioalsa
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/include

LOCAL_MODULE := tegra_alsa.tegra

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := eng
LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1

include $(NVIDIA_STATIC_LIBRARY)

endif
endif
endif
