ifeq ($(TARGET_DEVICE),p852)
LOCAL_PATH := $(call my-dir)
TEMPLATE_PATH := $(LOCAL_PATH)/../template

include $(addsuffix /Android.mk, \
	$(LOCAL_PATH)/odm_kit/adaptations/dtvtuner \
	$(LOCAL_PATH)/odm_kit/adaptations/misc \
	$(LOCAL_PATH)/odm_kit/query \
	$(LOCAL_PATH)/os/drivers/linux/keymapping \
	$(TEMPLATE_PATH)/odm_kit/adaptations/disp \
	$(TEMPLATE_PATH)/odm_kit/adaptations/pmu \
	$(TEMPLATE_PATH)/odm_kit/adaptations/hdmi \
	$(TEMPLATE_PATH)/odm_kit/adaptations/audiocodec \
	$(TEMPLATE_PATH)/odm_kit/adaptations/tmon \
	$(TEMPLATE_PATH)/odm_kit/adaptations/gpio_ext \
	$(TEMPLATE_PATH)/odm_kit/platform/keyboard \
	$(TEMPLATE_PATH)/odm_kit/platform/ota \
	$(TEMPLATE_PATH)/os/linux/system/extfs \
	$(TEMPLATE_PATH)/os/linux/system/nvextfsmgr \
)
endif

