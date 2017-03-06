LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvaboot
LOCAL_NVIDIA_NO_COVERAGE := true

ifeq ($(SECURE_OS_BUILD),y)
ifeq ($(TARGET_TEGRA_VERSION),t30)
LOCAL_C_INCLUDES += $(TOP)/3rdparty/trustedlogic/sdk/include/tegra3
endif
ifeq ($(TARGET_TEGRA_VERSION),ap20)
LOCAL_C_INCLUDES += $(TOP)/3rdparty/trustedlogic/sdk/include/tegra2
endif
endif

ifeq ($(TF_INCLUDE_WITH_SERVICES),y)
LOCAL_C_INCLUDES += $(TOP)/3rdparty/trustedlogic/donotdistribute/include/$(TARGET_PRODUCT)
endif

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/system/nv3p
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core/common

ifeq ($(filter $(TARGET_BOARD_PLATFORM_TYPE),fpga simulation),)
LOCAL_CFLAGS += -DLPM_BATTERY_CHARGING=1
endif

ifneq ($(filter t30,$(TARGET_TEGRA_VERSION)),)
LOCAL_CFLAGS += -DHAS_SE_CONTROLLER_INSTANCE=1
else
LOCAL_CFLAGS += -DHAS_SE_CONTROLLER_INSTANCE=0
endif

LOCAL_SRC_FILES += nvaboot.c
LOCAL_SRC_FILES += nvaboot_rawfs.c
LOCAL_SRC_FILES += nvaboot_bootfs.c
LOCAL_SRC_FILES += nvaboot_blockdev_nice.c
LOCAL_SRC_FILES += nvaboot_ap20.c
LOCAL_SRC_FILES += nvaboot_t30.c
LOCAL_SRC_FILES += nvaboot_usbf.c
ifeq ($(SECURE_OS_BUILD),y)
LOCAL_SRC_FILES += nvaboot_tf.c
endif
LOCAL_SRC_FILES += nvaboot_warmboot_sign.c
LOCAL_SRC_FILES += nvaboot_sanitize_keys.c
LOCAL_SRC_FILES += nvaboot_sanitize.S
LOCAL_SRC_FILES += nvaboot_warmboot_avp_$(TARGET_TEGRA_VERSION).S

#TODO: Remove following lines before include statement and fix the source giving warnings/errors
LOCAL_NVIDIA_NO_EXTRA_WARNINGS := 1
LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
include $(NVIDIA_STATIC_LIBRARY)
