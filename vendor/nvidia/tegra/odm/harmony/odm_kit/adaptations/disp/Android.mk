LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_disp

LOCAL_SRC_FILES += display_hal.c
LOCAL_SRC_FILES += panel_null.c
LOCAL_SRC_FILES += amoled_amaf002.c
LOCAL_SRC_FILES += panel_sharp_wvga.c
LOCAL_SRC_FILES += bl_pcf50626.c
LOCAL_SRC_FILES += bl_ap20.c
LOCAL_SRC_FILES += panel_samsungsdi.c
LOCAL_SRC_FILES += panel_tpo_wvga.c
LOCAL_SRC_FILES += panel_sharp_dsi.c
LOCAL_SRC_FILES += panel_lvds_wsga.c

include $(NVIDIA_STATIC_LIBRARY)

