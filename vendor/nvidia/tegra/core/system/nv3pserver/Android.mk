LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnv3pserver
LOCAL_NVIDIA_NO_COVERAGE := true

# AP20 use the legacy address map with SDRAM at address zero.
# Everything else will use the newer address map.
ifneq ($(filter ap20, $(TARGET_TEGRA_VERSION)),)
LOCAL_CFLAGS += -DNV_AOS_ENTRY_POINT=0x00108000
LOCAL_CFLAGS += -DNV_CHARGE_ENTRY_POINT=0x40008000
else
LOCAL_CFLAGS += -DNV_AOS_ENTRY_POINT=0x80108000
LOCAL_CFLAGS += -DNV_CHARGE_ENTRY_POINT=0x4000a000
endif

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/system/nvdiagnostics/sd
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/system/nvdiagnostics/se
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/system/nvdiagnostics/pwm
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/system/nvdiagnostics/disp/dsi

LOCAL_CFLAGS += -DNVODM_BOARD_IS_FPGA=0
#LOCAL_CFLAGS += -DNV_IS_AOS=1
LOCAL_CFLAGS += -DNVODM_BOARD_IS_SIMULATION=0

LOCAL_SRC_FILES += nv3p_server.c
LOCAL_SRC_FILES += nv3p_server_utils_ap20.c
LOCAL_SRC_FILES += nv3p_server_utils_t30.c
LOCAL_SRC_FILES += nv3p_server_utils.c

LOCAL_NVIDIA_NO_EXTRA_WARNINGS := 1
include $(NVIDIA_STATIC_LIBRARY)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnv3pserverhost

# AP20 use the legacy address map with SDRAM at address zero.
# Everything else will use the newer address map.
ifneq ($(filter ap20, $(TARGET_TEGRA_VERSION)),)
LOCAL_CFLAGS += -DNV_AOS_ENTRY_POINT=0x00108000
LOCAL_CFLAGS += -DNV_CHARGE_ENTRY_POINT=0x40008000
else
LOCAL_CFLAGS += -DNV_AOS_ENTRY_POINT=0x80108000
LOCAL_CFLAGS += -DNV_CHARGE_ENTRY_POINT=0x4000a000
endif
LOCAL_CFLAGS += -DNVODM_BOARD_IS_FPGA=0
LOCAL_CFLAGS += -DNVODM_BOARD_IS_SIMULATION=1

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/system/nvdiagnostics/sd
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/system/nvdiagnostics/se
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/system/nvdiagnostics/pwm
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/system/nvdiagnostics/disp/dsi
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)

LOCAL_SRC_FILES += nv3p_server.c
LOCAL_SRC_FILES += nv3p_server_utils_ap20.c
LOCAL_SRC_FILES += nv3p_server_utils_t30.c
LOCAL_SRC_FILES += nv3p_server_utils.c

include $(NVIDIA_HOST_STATIC_LIBRARY)
