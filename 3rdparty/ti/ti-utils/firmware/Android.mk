ifneq ( , $(findstring $(BOARD_WLAN_DEVICE), wl12xx_mac80211 wl18xx_mac80211))

local_target_dir := $(TARGET_OUT)/etc/firmware/ti-connectivity
LOCAL_PATH:= $(call my-dir)

ifeq ($(BOARD_WLAN_DEVICE),wl12xx_mac80211)
include $(CLEAR_VARS)
LOCAL_MODULE := wl1271-nvs_default.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := wl1271-nvs.bin
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := wl128x-fw-4-sr.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := wl128x-fw-4-mr.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := wl128x-fw-4-plt.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)
endif

ifeq ($(BOARD_WLAN_DEVICE),wl18xx_mac80211)
include $(CLEAR_VARS)
LOCAL_MODULE := wl18xx-fw-mc.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := wl18xx-fw.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)
endif

ifeq ($(BOARD_WLAN_DEVICE),wl12xx_mac80211)
LOCAL_MODULE := /data/misc/wifi/wl1271-nvs.bin
# Make a symlink from /etc/firmware/ti-connectivity/wl1271-nvs.bin to /data/misc/wifi/wl1271-nvs.bin
SYMLINKS := $(TARGET_OUT)/etc/firmware/ti-connectivity/wl1271-nvs.bin
$(SYMLINKS): NVS_BINARY := $(LOCAL_MODULE)
$(SYMLINKS): $(LOCAL_INSTALLED_MODULE) $(LOCAL_PATH)/Android.mk
	@echo "Symlink: $@ -> $(NVS_BINARY)"
	@mkdir -p $(dir $@)
	@rm -rf $@
	@ln -sf $(NVS_BINARY) $@

ALL_DEFAULT_INSTALLED_MODULES += $(SYMLINKS)

# We need this so that the installed files could be picked up based on the
# local module name
ALL_MODULES.$(LOCAL_MODULE).INSTALLED := \
    $(ALL_MODULES.$(LOCAL_MODULE).INSTALLED) $(SYMLINKS)
    

LOCAL_MODULE := /forever/misc/wifi/wl1271-nvs2.bin
# Make a symlink from /etc/firmware/ti-connectivity/wl1271-nvs2.bin to /forever/misc/wifi/wl1271-nvs2.bin
SYMLINKS := $(TARGET_OUT)/etc/firmware/ti-connectivity/wl1271-nvs2.bin
$(SYMLINKS): NVS_BINARY := $(LOCAL_MODULE)
$(SYMLINKS): $(LOCAL_INSTALLED_MODULE) $(LOCAL_PATH)/Android.mk
	@echo "Symlink: $@ -> $(NVS_BINARY)"
	@mkdir -p $(dir $@)
	@rm -rf $@
	@ln -sf $(NVS_BINARY) $@

ALL_DEFAULT_INSTALLED_MODULES += $(SYMLINKS)

# We need this so that the installed files could be picked up based on the
# local module name
ALL_MODULES.$(LOCAL_MODULE).INSTALLED := \
    $(ALL_MODULES.$(LOCAL_MODULE).INSTALLED) $(SYMLINKS)

endif
endif