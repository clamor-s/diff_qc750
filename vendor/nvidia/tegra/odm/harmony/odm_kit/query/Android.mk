LOCAL_PATH := $(call my-dir)
ifneq (,$(filter $(TARGET_PRODUCT),harmony ventana))
LOCAL_COMMON_C_INCLUDES := $(LOCAL_PATH)/../../../template/odm_kit/adaptations

LOCAL_COMMON_SRC_FILES := nvodm_query.c
LOCAL_COMMON_SRC_FILES += nvodm_query_discovery.c
LOCAL_COMMON_SRC_FILES += nvodm_query_nand.c
LOCAL_COMMON_SRC_FILES += nvodm_query_gpio.c
LOCAL_COMMON_SRC_FILES += nvodm_query_pinmux.c
LOCAL_COMMON_SRC_FILES += nvodm_query_kbc.c
LOCAL_COMMON_SRC_FILES += secure/nvodm_query_secure.c
LOCAL_COMMON_CFLAGS := -DLPM_BATTERY_CHARGING=1

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_query
# For some reason nvodm_query needs to be arm, as it is called
# very early from the bootloader.
LOCAL_ARM_MODE := arm
LOCAL_CFLAGS += $(LOCAL_COMMON_CFLAGS)
LOCAL_C_INCLUDES += $(LOCAL_COMMON_C_INCLUDES)
LOCAL_SRC_FILES := $(LOCAL_COMMON_SRC_FILES)

LOCAL_SHARED_LIBRARIES += libnvrm
LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_STATIC_LIBRARIES += libnvodm_services

include $(NVIDIA_STATIC_AND_SHARED_LIBRARY)


include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_query_static_avp
# for some reason nvodm_query needs to be arm, as it is called
# very early from the bootloader
LOCAL_ARM_MODE := arm

LOCAL_CFLAGS += -Os -ggdb0
LOCAL_CFLAGS += -DFPGA_BOARD
LOCAL_CFLAGS += -DAVP_PINMUX=0
LOCAL_CFLAGS += $(LOCAL_COMMON_CFLAGS)

LOCAL_C_INCLUDES += $(LOCAL_COMMON_C_INCLUDES)
LOCAL_SRC_FILES += $(LOCAL_COMMON_SRC_FILES)

include $(NVIDIA_STATIC_AVP_LIBRARY)


include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_query_avp
# For some reason nvodm_query needs to be arm, as it is called
# very early from the bootloader.
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES += $(LOCAL_COMMON_C_INCLUDES)
LOCAL_SRC_FILES := $(LOCAL_COMMON_SRC_FILES)
LOCAL_CFLAGS += $(LOCAL_COMMON_CFLAGS)
LOCAL_CFLAGS += -Os -ggdb0

include $(NVIDIA_STATIC_AVP_LIBRARY)
endif
