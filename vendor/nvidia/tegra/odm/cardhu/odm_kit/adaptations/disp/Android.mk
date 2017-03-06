LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)
LOCAL_NVIDIA_NO_COVERAGE := true

LOCAL_MODULE := libnvodm_disp

LOCAL_SRC_FILES += panels.c
LOCAL_SRC_FILES += panel_sony_vga.c
LOCAL_SRC_FILES += panel_samsung_test_dsi.c
LOCAL_SRC_FILES += panel_sharp_test_dsi.c
LOCAL_SRC_FILES += panel_lvds_wsga.c


#TODO: Remove following lines before include statement and fix the source giving warnings/errors
LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
include $(NVIDIA_STATIC_LIBRARY)
