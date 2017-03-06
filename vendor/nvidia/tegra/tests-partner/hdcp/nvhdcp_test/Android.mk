LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := hdcp_test
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
LOCAL_SRC_FILES += nvhdcp_test.c
LOCAL_STATIC_LIBRARIES += libhdcp_up

include $(NVIDIA_EXECUTABLE)

ifeq ($(BOARD_ENABLE_SECURE_HDCP),1)
    include $(NVIDIA_DEFAULTS)

    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
    LOCAL_MODULE := secure_hdcp_test
    LOCAL_MODULE_TAGS := optional

    LOCAL_SRC_FILES += nvhdcp_test.c
    LOCAL_STATIC_LIBRARIES += libsecure_hdcp_up
    LOCAL_STATIC_LIBRARIES += libtee_client_api_driver

    include $(NVIDIA_EXECUTABLE)
endif
