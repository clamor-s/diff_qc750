ifneq ( , $(findstring $(BOARD_WLAN_DEVICE), wl12xx_mac80211 wl18xx_mac80211))
LOCAL_PATH:= $(call my-dir)

ifeq ($(BOARD_WLAN_DEVICE),wl12xx_mac80211)
#
# Calibrator
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
		nvs.c \
		misc_cmds.c \
		calibrator.c \
		plt.c \
		ini.c

LOCAL_CFLAGS := -Wall -Wno-unused-parameter
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	3rdparty/ti/libnl/include

LOCAL_SHARED_LIBRARIES := libnl
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := calibrator
include $(BUILD_EXECUTABLE)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := ProjectLogDef.ili
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := fw_logger/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := iliparser.py
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := fw_logger/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := message.py
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := fw_logger/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := parser.py
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := fw_logger/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#
# UIM Application
#
#include $(CLEAR_VARS)

#LOCAL_C_INCLUDES:= \
#	$(LOCAL_PATH)/uim_rfkill/ \
#	external/bluetooth/bluez/

#LOCAL_SRC_FILES:= \
#	uim_rfkill/uim.c
#LOCAL_CFLAGS:= -g -c -W -Wall -O2 -D_POSIX_SOURCE
#LOCAL_SHARED_LIBRARIES:= libnetutils
#LOCAL_MODULE_TAGS := eng
#LOCAL_MODULE:=uim-util

#include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := TQS_S_2.6.ini
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/firmware/ti-connectivity
LOCAL_SRC_FILES := ini_files/128x/TQS_S_2.6.ini
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := iw
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := iw
LOCAL_MODULE_CLASS := EXECUTABLES
include $(BUILD_PREBUILT)

include $(call all-makefiles-under,$(LOCAL_PATH))
endif
