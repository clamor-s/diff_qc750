LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := nvidl
LOCAL_SRC_FILES := nvidl.tab.c lex.yy.c nvidl.c
LOCAL_ACP_UNAVAILABLE := true

# This hack is here for preventing circular dependency
# from nvidl to generated headers. It is safe because we
# know that nvidl doesn't depend on anything generated

STORED_ALL_C_CPP_ETC_OBJECTS := $(ALL_C_CPP_ETC_OBJECTS)
include $(NVIDIA_HOST_EXECUTABLE)
ALL_C_CPP_ETC_OBJECTS := $(STORED_ALL_C_CPP_ETC_OBJECTS)
STORED_ALL_C_CPP_ETC_OBJECTS :=