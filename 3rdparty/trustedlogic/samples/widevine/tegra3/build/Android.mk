LOCAL_PATH:= $(call my-dir)
ifeq ($(TARGET_ARCH),arm)
#####################################################################
# liboemcrypto.a, lib1
#include $(CLEAR_VARS)
#LOCAL_PREBUILT_LIBS := liboemcrypto.a
#LOCAL_MODULE_TAGS := optional
#include $(BUILD_MULTI_PREBUILT)
-include $(TOP)/3rdparty/trustedlogic/samples/widevine/tegra3/build/Android.liboemcrypto.mk

# Only build the widevine plugins if we have widevine
ifneq ($(wildcard vendor/widevine),)
#####################################################################
# libdrmwvmplugin.so
include $(CLEAR_VARS)
-include $(TOP)/vendor/widevine/proprietary/drmwvmplugin/plugin-core.mk
LOCAL_MODULE := libdrmwvmplugin
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)/drm
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES += liboemcrypto
LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_SHARED_LIBRARIES += libnvavp
LOCAL_LDFLAGS += $(PRODUCT_ROOT)/tf_sdk/lib/arm_gcc_android/libtee_client_api_driver.a
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)
#####################################################################
# libwvm.so
include $(CLEAR_VARS)
-include $(TOP)/vendor/widevine/proprietary/wvm/wvm-core.mk
LOCAL_MODULE := libwvm
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES += liboemcrypto
LOCAL_LDFLAGS += $(PRODUCT_ROOT)/tf_sdk/lib/arm_gcc_android/libtee_client_api_driver.a
LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_SHARED_LIBRARIES += libnvavp
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
#-include $(TOP)/3rdparty/trustedlogic/samples/widevine/tegra3/build/Android_tester.mk
################################################################
#libdrmdecrypt.so
include $(CLEAR_VARS)
include $(TOP)/vendor/widevine/proprietary/cryptoPlugin/decrypt-core.mk
LOCAL_LDFLAGS += $(PRODUCT_ROOT)/tf_sdk/lib/arm_gcc_android/libtee_client_api_driver.a
LOCAL_C_INCLUDES := \
	$(TOP)/frameworks/native/include/media/hardware \
	$(TOP)/vendor/widevine/proprietary/cryptoPlugin

LOCAL_SHARED_LIBRARIES := \
	libstagefright_foundation \
	liblog
LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_SHARED_LIBRARIES += libnvavp
LOCAL_MODULE := libdrmdecrypt
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES += libcrypto libcutils
LOCAL_STATIC_LIBRARIES += liboemcrypto
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)
endif
endif