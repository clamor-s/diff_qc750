LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        NvCryptoPlugin.cpp

ifeq ($(SECURE_OS_BUILD),y)
LOCAL_CFLAGS += -DSECUREOS
LOCAL_STATIC_LIBRARIES += libtee_client_api_driver
LOCAL_C_INCLUDES += \
        $(TOP)/3rdparty/trustedlogic/sdk/tegra3/tf_sdk/include \
        $(TOP)/3rdparty/trustedlogic/sdk/tegra3/tegra3_secure_world_integration_kit/sddk/drivers/sdrv_rsa/include
endif

LOCAL_SHARED_LIBRARIES := \
    libutils

LOCAL_MODULE:= libdrmdecrypt
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
