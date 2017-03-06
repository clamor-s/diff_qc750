LOCAL_PATH := $(call my-dir)

# HAL module implemenation, not prelinked and stored in
# hw/<OVERLAY_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(NVIDIA_DEFAULTS)

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils \
	libhardware_legacy \
	libnvos \
	libnvrm \
	libnvrm_graphics \
	libnvddk_2d_v2
LOCAL_STATIC_LIBRARIES := \
	libnvfxmath
LOCAL_SRC_FILES := \
	nvgrmodule.c \
	nvgralloc.c \
	nvgrbuffer.c \
	nvgr_scratch.c \
	nvgr_2d.c
LOCAL_MODULE := gralloc.$(TARGET_BOARD_PLATFORM)

LOCAL_CFLAGS += -DLOG_TAG=\"gralloc\"

FB_IMPL :=
LOCAL_C_INCLUDES += $(TEGRA_TOP)/graphics/android/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/graphics-partner/android/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/graphics/2d/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/include

ifeq ($(BOARD_VENDOR_HDCP_ENABLED),true)
    LOCAL_CFLAGS += -DNV_HDCP=1
    LOCAL_C_INCLUDES += $(TOP)/$(BOARD_VENDOR_HDCP_PATH)/include
    ifeq ($(BOARD_ENABLE_SECURE_HDCP),1)
        LOCAL_STATIC_LIBRARIES += libsecure_hdcp_up
        LOCAL_STATIC_LIBRARIES += libtee_client_api_driver
    else
        LOCAL_STATIC_LIBRARIES += libhdcp_up
        LOCAL_SHARED_LIBRARIES += libpkip libcrypto
    endif
    ifneq ($(BOARD_VENDOR_HDCP_INTERVAL),)
        LOCAL_CFLAGS += -DNV_HDCP_POLL_INTERVAL=$(BOARD_VENDOR_HDCP_INTERVAL)
    endif
endif

# This config can be used to reduce memory usage at the cost of performance.
ifeq ($(BOARD_DISABLE_TRIPLE_BUFFERED_DISPLAY_SURFACES),true)
    LOCAL_CFLAGS += -DNVGR_USE_TRIPLE_BUFFERING=0
else
    LOCAL_CFLAGS += -DNVGR_USE_TRIPLE_BUFFERING=1
endif

ifneq ($(BOARD_HDMI_RESOLUTION_MODE),)
    LOCAL_CFLAGS += -DNV_PROPERTY_HDMI_RESOLUTION_DEF=$(BOARD_HDMI_RESOLUTION_MODE)
endif

ifeq ($(BOARD_SUPPORT_MHL_CTS), 1)
    LOCAL_CFLAGS += -DSUPPORT_MHL_CTS=1
else
    LOCAL_CFLAGS += -DSUPPORT_MHL_CTS=0
endif

include $(NVIDIA_SHARED_LIBRARY)
