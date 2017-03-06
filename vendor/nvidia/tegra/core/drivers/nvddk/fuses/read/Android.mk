LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvddk_fuse_read

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvddk/fuses/read
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvddk/fuses/common
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/hwinc

LOCAL_SRC_FILES += nvddk_fuse_read.c
LOCAL_SRC_FILES += ap20/nvddk_ap20_fuse_priv.c
LOCAL_SRC_FILES += t30/nvddk_t30_fuse_priv.c

LOCAL_CFLAGS += -DNVODM_BOARD_IS_SIMULATION=0
LOCAL_CFLAGS += -Wno-error=cast-align
LOCAL_CFLAGS += -Wno-missing-field-initializers
LOCAL_CFLAGS += -fno-exceptions
LOCAL_CFLAGS += -DADDITIONAL_FUNCTIONALITY=0
LOCAL_CFLAGS += -Os -ggdb0
include $(NVIDIA_STATIC_AVP_LIBRARY)

include $(NVIDIA_DEFAULTS)
LOCAL_MODULE := libnvddk_fuse_read_cpu

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvddk/fuses/read
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvddk/fuses/common
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/hwinc

LOCAL_SRC_FILES += nvddk_fuse_read.c
LOCAL_SRC_FILES += ap20/nvddk_ap20_fuse_priv.c
LOCAL_SRC_FILES += t30/nvddk_t30_fuse_priv.c

LOCAL_CFLAGS += -DNVODM_BOARD_IS_SIMULATION=0
LOCAL_CFLAGS += -Wno-error=cast-align
LOCAL_CFLAGS += -Wno-missing-field-initializers
LOCAL_CFLAGS += -fno-exceptions
LOCAL_CFLAGS += -DADDITIONAL_FUNCTIONALITY=1
LOCAL_CFLAGS += -Os -ggdb0
include $(NVIDIA_STATIC_LIBRARY)


# Include host specific library make file
include $(addprefix $(LOCAL_PATH)/, $(addsuffix /Android.mk, \
     host \
))
