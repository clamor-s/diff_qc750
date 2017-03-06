ifeq ($(WPA_SUPPLICANT_VERSION),VER_TI_0_8_X)
    ifeq ($(BOARD_WLAN_DEVICE),wl18xx_mac80211)
        include $(call my-dir)/hostap_wl8/Android.mk
    else
        include $(call all-subdir-makefiles)
    endif
endif
