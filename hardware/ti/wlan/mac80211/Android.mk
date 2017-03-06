ifeq ($(BOARD_WLAN_DEVICE),wl12xx_mac80211)
ifeq ($(WPA_SUPPLICANT_VERSION),VER_0_8_X)
    include $(call my-dir)/wpa_supplicant_lib/Android.mk
endif
endif
