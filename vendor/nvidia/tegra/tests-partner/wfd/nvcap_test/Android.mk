LOCAL_PATH:= $(call my-dir)

#
# NvCap Test App
#
include $(NVIDIA_DEFAULTS)

LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libnvcap
LOCAL_SHARED_LIBRARIES += libutils

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/include

LOCAL_SRC_FILES:=        \
 nvcap_test.cpp

LOCAL_MODULE := nvcap_test
LOCAL_MODULE_TAGS := samples

LOCAL_CFLAGS += -DLOG_TAG=\"NvCapTest\"

include $(NVIDIA_EXECUTABLE)
