LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_dtvtuner

LOCAL_SRC_FILES += nvodm_dtvtuner.c
LOCAL_SRC_FILES += nvodm_dtvtuner_murata.c

LOCAL_SHARED_LIBRARIES += libnvrm
LOCAL_SHARED_LIBRARIES += libnvrm_graphics
LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_SHARED_LIBRARIES += libnvodm_query
LOCAL_STATIC_LIBRARIES += libnvodm_services

include $(NVIDIA_SHARED_LIBRARY)

