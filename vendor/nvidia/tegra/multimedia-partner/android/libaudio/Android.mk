#
# Copyright (c) 2011-2012, NVIDIA CORPORATION.  All rights reserved.
#

USE_NEW_LIBAUDIO = 1
ifneq ($(USE_NEW_LIBAUDIO), 1)
LOCAL_PATH := $(call my-dir)
ifneq ($(BOARD_VENDOR_USE_NV_AUDIO),false)
ifeq ($(strip $(BOARD_USES_ALSA_AUDIO)),true)
    include $(LOCAL_PATH)/libaudioalsa/Android.mk
endif
endif

else

ifneq ($(strip $(BOARD_USES_GENERIC_AUDIO)),true)
ifeq ($(strip $(BOARD_USES_ALSA_AUDIO)),true)

LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

#LOCAL_CFLAGS += -D_POSIX_SOURCE -Wno-multichar

# Now build the libnvaudioservice
include $(NVIDIA_DEFAULTS)

LOCAL_ARM_MODE := arm
LOCAL_CFLAGS += -D_POSIX_SOURCE

LOCAL_MODULE := libnvaudioservice

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libbinder \
    libc

LOCAL_C_INCLUDES += $(TEGRA_INCLUDE)

LOCAL_SRC_FILES := \
    invaudioservice.cpp

include $(NVIDIA_SHARED_LIBRARY)

# Now build the libaudio
include $(NVIDIA_DEFAULTS)

LOCAL_ARM_MODE := arm
#LOCAL_CFLAGS += -D_POSIX_SOURCE

LOCAL_MODULE := audio.primary.tegra

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libtinyalsa \
    libaudioutils \
    libdl \
    libexpat \
    libbinder \
    libmedia \
    libnvaudioservice \
    libaudioavp

ifeq ($(BOARD_SUPPORT_NVOICE),true)
    LOCAL_SHARED_LIBRARIES += libnvoice
    LOCAL_CFLAGS += -DNVOICE
endif

LOCAL_C_INCLUDES += \
    $(TEGRA_INCLUDE) \
    external/tinyalsa/include \
    external/expat/lib \
    system/media/audio_utils/include \
    system/media/audio_effects/include

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_CFLAGS += -DTEGRA_TEST_LIBAUDIO

LOCAL_SRC_FILES := \
    nvaudio_list.c \
    nvaudio_hw.c \
    nvaudio_service.cpp

include $(NVIDIA_SHARED_LIBRARY)

#
# Build libaudiopolicy
#
include $(NVIDIA_DEFAULTS)

#LOCAL_CFLAGS += -D_POSIX_SOURCE

ifeq ($(BOARD_HAVE_BLUETOOTH),true)
  LOCAL_CFLAGS += -DWITH_A2DP
endif

LOCAL_SRC_FILES := nvaudiopolicymanager.cpp

LOCAL_MODULE := audio_policy.tegra

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_WHOLE_STATIC_LIBRARIES += libaudiopolicy_legacy \
    libmedia_helper

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libmedia \
    libbinder \
    libnvaudioservice

#TODO: Remove following lines before include statement and fix the source giving warnings/errors
LOCAL_NVIDIA_NO_EXTRA_WARNINGS := 1
LOCAL_CFLAGS += -Wno-error=sign-compare
include $(NVIDIA_SHARED_LIBRARY)

endif
endif
endif
