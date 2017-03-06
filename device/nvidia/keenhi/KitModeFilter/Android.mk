LOCAL_PATH:= $(call my-dir)

# the library
# ============================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)
            
LOCAL_MODULE := nabi2_core
LOCAL_MODULE_TAGS := optional
LOCAL_PROGUARD_FLAG_FILES := proguard.flags

include $(BUILD_JAVA_LIBRARY)

# additionally, build unit tests in a separate .apk
#include $(call all-makefiles-under,$(LOCAL_PATH))
