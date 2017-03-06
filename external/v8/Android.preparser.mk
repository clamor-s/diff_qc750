LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := v8preparser
LOCAL_MODULE_TAGS := optional

LOCAL_CPP_EXTENSION := .cc

LOCAL_STATIC_LIBRARIES := libv8
LOCAL_SHARED_LIBRARIES := libcutils

LOCAL_SRC_FILES:= \
	src/allocation.cc \
	src/bignum.cc \
	src/bignum-dtoa.cc \
	src/cached-powers.cc \
	src/conversions.cc \
	src/diy-fp.cc \
	src/dtoa.cc \
	src/fast-dtoa.cc \
	src/fixed-dtoa.cc \
	src/once.cc \
	src/preparse-data.cc \
	src/preparser.cc \
	src/preparser-api.cc \
	src/scanner.cc \
	src/strtod.cc \
	src/token.cc \
	src/unicode.cc \
	src/utils.cc \
	preparser/preparser-process.cc

include $(LOCAL_PATH)/Android.cflags.mk

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src

include $(BUILD_EXECUTABLE)
