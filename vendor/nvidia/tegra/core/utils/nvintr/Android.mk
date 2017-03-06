LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvintr
LOCAL_NVIDIA_NO_COVERAGE := true
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core/common

LOCAL_CFLAGS += -DTRUSTZONE_ENABLED=0
LOCAL_CFLAGS += -DENABLE_SECURITY=0

ifeq ($(TARGET_TEGRA_VERSION),ap20)
LOCAL_CFLAGS += -DTARGET_SOC_AP20
endif


LOCAL_SRC_FILES += nvintrhandler.c
LOCAL_SRC_FILES += nvintrhandler_common.c
LOCAL_SRC_FILES += nvintrhandler_sim.c
LOCAL_SRC_FILES += nvintrhandler_ap20.c
LOCAL_SRC_FILES += nvintrhandler_gpio.c
LOCAL_SRC_FILES += nvintrhandler_gpio_ap20.c
LOCAL_SRC_FILES += nvintrhandler_gpio_t30.c
LOCAL_SRC_FILES += nvintrhandler_apbdma.c
LOCAL_SRC_FILES += nvintrhandler_apbdma_ap20.c
LOCAL_SRC_FILES += nvintrhandler_apbdma_t30.c

include $(NVIDIA_STATIC_LIBRARY)
