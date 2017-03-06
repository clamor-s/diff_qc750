ifneq ($(USE_NEW_LIBAUDIO), 1)
LOCAL_PATH := $(call my-dir)
ifneq ($(BOARD_VENDOR_USE_NV_AUDIO),false)
ifeq ($(strip $(BOARD_USES_ALSA_AUDIO)),true)
        include $(LOCAL_PATH)/libaudioalsa/Android.mk
endif
endif
endif
