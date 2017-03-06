LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
ifeq ($(SECURE_OS_BUILD),y)
LOCAL_SRC_FILES := init.tf_enabled.rc
else
LOCAL_SRC_FILES := init.tf_disabled.rc
endif
LOCAL_BUILT_MODULE_STEM := init.tf.rc
LOCAL_MODULE_SUFFIX := .rc
LOCAL_MODULE := init.tf
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

include $(call all-makefiles-under,$(LOCAL_PATH))
