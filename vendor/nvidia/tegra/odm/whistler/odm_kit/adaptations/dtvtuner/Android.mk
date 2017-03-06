# Generated Android.mk
LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_dtvtuner

LOCAL_C_INCLUDES += $(TEGRA_TOP)/customers/nvidia-partner/whistler/odm_kit/adaptations/dtvtuner
LOCAL_SRC_FILES += nvodm_dtvtuner.c
LOCAL_SRC_FILES += nvodm_dtvtuner_murata.c
LOCAL_STATIC_LIBRARIES += libnvodm_services
LOCAL_SHARED_LIBRARIES += libnvrm
LOCAL_SHARED_LIBRARIES += libnvrm_graphics
LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_SHARED_LIBRARIES += libnvodm_query

LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
include $(NVIDIA_STATIC_AND_SHARED_LIBRARY)
