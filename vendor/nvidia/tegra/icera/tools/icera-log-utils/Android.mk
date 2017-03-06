# icera_log_serial_arm/Android.mk
#
# Copyright 2011 Icera Inc.
#

# This is the Icera file system server application

LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE:= icera_log_serial_arm
LOCAL_SRC_FILES := icera_log_serial_arm
LOCAL_MODULE_CLASS := EXECUTABLES

include $(NVIDIA_PREBUILT)
