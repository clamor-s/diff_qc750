LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvseaes_keysched_lock_avp

LOCAL_NO_DEFAULT_COMPILER_FLAGS := true

LOCAL_CFLAGS += -Os
LOCAL_SRC_FILES := se_aes_keyschedule_lock.c
LOCAL_C_INCLUDES := \
	$(TEGRA_TOP)/core/utils/aes_keysched_lock \
	$(TARGET_PROJECT_INCLUDES) \
	$(TARGET_C_INCLUDES) \
	$(TEGRA_TOP)/core/include \
	$(TEGRA_TOP)/core/drivers/hwinc

include $(NVIDIA_STATIC_AVP_LIBRARY)
