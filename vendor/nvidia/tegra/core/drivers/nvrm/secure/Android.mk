LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvrm_secure
LOCAL_NVIDIA_NO_COVERAGE := true

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../nvrmkernel/core
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../nvrmkernel/core/common

ifeq ($(SECURE_OS_BUILD),y)
  ifeq ($(TARGET_TEGRA_VERSION),t30)
    TEGRA_CHIP=tegra3
  endif
  ifeq ($(TARGET_TEGRA_VERSION),ap20)
    TEGRA_CHIP=tegra2
  endif
  LOCAL_C_INCLUDES += $(TOP)/3rdparty/trustedlogic/sdk/$(TEGRA_CHIP)/tf_sdk/include
  LOCAL_STATIC_LIBRARIES += libtee_client_api_driver
endif

LOCAL_SRC_FILES += nvrm_secure.c

include $(NVIDIA_STATIC_LIBRARY)

ifeq ($(SECURE_OS_BUILD),y)
  TEGRA_CHIP :=
endif
