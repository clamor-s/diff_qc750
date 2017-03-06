LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvbootupdate
LOCAL_NVIDIA_NO_COVERAGE := true
LOCAL_SRC_FILES += nvbu_private_ap20.c
LOCAL_SRC_FILES += nvbu_private_t30.c
LOCAL_SRC_FILES += nvbu.c

LOCAL_NVIDIA_NO_EXTRA_WARNINGS := 1
include $(NVIDIA_STATIC_LIBRARY)
include $(NVIDIA_DEFAULTS)
LOCAL_MODULE := libnvbootupdatehost
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)
LOCAL_SRC_FILES += nvbu_private_ap20.c
LOCAL_SRC_FILES += nvbu_private_t30.c
LOCAL_SRC_FILES += nvbu.c

include $(NVIDIA_HOST_STATIC_LIBRARY)
