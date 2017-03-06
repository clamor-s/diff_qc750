LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

# display manager loads this dynamically,
# therefore need shared library as well

LOCAL_MODULE := libnvodm_hdmi
LOCAL_SRC_FILES += nvodm_hdmi.c

LOCAL_SHARED_LIBRARIES += libnvrm
LOCAL_SHARED_LIBRARIES += libnvrm_graphics
LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_SHARED_LIBRARIES += libnvodm_query
LOCAL_STATIC_LIBRARIES += libnvodm_services

include $(NVIDIA_STATIC_AND_SHARED_LIBRARY)
