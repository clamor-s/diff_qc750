ifneq ( , $(findstring $(BOARD_WLAN_DEVICE), wl12xx_mac80211 wl18xx_mac80211))
local_target_dir := $(TARGET_OUT)/etc/firmware
LOCAL_PATH := $(call my-dir)

# 128x PG-2.21 service pack 39
include $(CLEAR_VARS)
LOCAL_MODULE := TIInit_10.6.15.bts
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := TIInit_10.6.15-ble-0801.bts
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)

# FM
include $(CLEAR_VARS)
LOCAL_MODULE := fmc_ch8_1283.2.bts
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := fmc_ch8_1283.2.bts
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)

# FM
include $(CLEAR_VARS)
LOCAL_MODULE := fm_rx_ch8_1283.2.bts
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := fm_rx_ch8_1283.2.bts
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)

# FM
include $(CLEAR_VARS)
LOCAL_MODULE := fm_tx_ch8_1283.2.bts
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := fm_tx_ch8_1283.2.bts
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)

# 128x PG-2.21 service pack 39
include $(CLEAR_VARS)
LOCAL_MODULE := TIInit_11.8.32.bts
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := TIInit_11.8.32.bts
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)

# 128x PG-2.21 service pack 39
include $(CLEAR_VARS)
LOCAL_MODULE := TIInit_12.8.32.bts
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := TIInit_12.8.32.bts
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)
endif
