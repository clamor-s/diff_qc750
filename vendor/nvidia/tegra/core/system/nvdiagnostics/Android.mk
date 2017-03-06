LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvdiagnostics
LOCAL_NVIDIA_NO_COVERAGE := true

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/system/nvdiagnostics/pwm/$(TARGET_TEGRA_VERSION)

ifneq ($(filter t30,$(TARGET_TEGRA_VERSION)),)
LOCAL_SRC_FILES += sd/nvddk_sd_block_driver_test_t30.c
LOCAL_SRC_FILES += se/se_test_t30.c
LOCAL_SRC_FILES += pwm/pwm_test.c
LOCAL_SRC_FILES += disp/dsi/nvddk_dsi_block_driver_test.c
else
LOCAL_SRC_FILES += sd/nvddk_sd_block_driver_test_stub.c
LOCAL_SRC_FILES += se/se_test_stub.c
LOCAL_SRC_FILES += pwm/pwm_test_stub.c
LOCAL_SRC_FILES += disp/dsi/nvddk_dsi_block_driver_test_stub.c
endif

LOCAL_NVIDIA_NO_EXTRA_WARNINGS := 1
include $(NVIDIA_STATIC_LIBRARY)

