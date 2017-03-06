#wpan utilties and TI ST user space manager
ifneq ( , $(findstring $(BOARD_WLAN_DEVICE), wl12xx_mac80211 wl18xx_mac80211))
include $(call first-makefiles-under,$(call my-dir))
endif
