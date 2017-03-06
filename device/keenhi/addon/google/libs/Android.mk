LOCAL_PATH := $(my-dir)

#########################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := libAppDataSearch.so

LOCAL_MODULE_TAGS := optional

include $(BUILD_MULTI_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := libfacelock_jni.so

LOCAL_MODULE_TAGS := optional

include $(BUILD_MULTI_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := libfilterpack_facedetect.so

LOCAL_MODULE_TAGS := optional

include $(BUILD_MULTI_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := libfrsdk.so

LOCAL_MODULE_TAGS := optional

include $(BUILD_MULTI_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := libgcomm_jni.so

LOCAL_MODULE_TAGS := optional

include $(BUILD_MULTI_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := libgoggles_clientvision.so

LOCAL_MODULE_TAGS := optional

include $(BUILD_MULTI_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := libgoogle_recognizer_jni_l.so

LOCAL_MODULE_TAGS := optional

include $(BUILD_MULTI_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := libgtalk_jni.so

LOCAL_MODULE_TAGS := optional

include $(BUILD_MULTI_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := libgtalk_stabilize.so

LOCAL_MODULE_TAGS := optional

include $(BUILD_MULTI_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := libjni_latinimegoogle.so

LOCAL_MODULE_TAGS := optional

include $(BUILD_MULTI_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := liblightcycle.so

LOCAL_MODULE_TAGS := optional

include $(BUILD_MULTI_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := libpatts_engine_jni_api.so

LOCAL_MODULE_TAGS := optional

include $(BUILD_MULTI_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := libspeexwrapper.so

LOCAL_MODULE_TAGS := optional

include $(BUILD_MULTI_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := libWVphoneAPI.so

LOCAL_MODULE_TAGS := optional

include $(BUILD_MULTI_PREBUILT)

#########################################
include $(CLEAR_VARS)

LOCAL_MODULE := libchromeview

ifeq ($(TARGET_ARCH),arm)
    LOCAL_SRC_FILES := $(LOCAL_MODULE).so
else ifeq ($(TARGET_ARCH),x86)
    LOCAL_SRC_FILES := x86/$(LOCAL_MODULE).so
endif

LOCAL_MODULE_STEM := $(LOCAL_MODULE)
LOCAL_MODULE_SUFFIX := $(suffix $(LOCAL_SRC_FILES))
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional

include $(BUILD_PREBUILT)
