ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    ../src/tester.cpp

LOCAL_C_INCLUDES+= \
    bionic \
    external/stlport/stlport


LOCAL_STATIC_LIBRARIES := liboemcrypto
LOCAL_LDFLAGS += $(PRODUCT_ROOT)/tf_sdk/lib/arm_gcc_android/libtee_client_api_driver.a

LOCAL_SHARED_LIBRARIES := \
    libstlport            \
    liblog                \
    libutils              \
    libz                  \
    libdl
    
LOCAL_STATIC_LIBRARIES += liboemcrypto
LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_SHARED_LIBRARIES += libnvavp

LOCAL_MODULE:=oemcrypto_api_test

LOCAL_MODULE_TAGS := tests

include $(BUILD_EXECUTABLE)

endif
