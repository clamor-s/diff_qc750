LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvbuildbct

LOCAL_MODULE_TAGS := eng

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/hwinc

LOCAL_SRC_FILES += ap20/ap20_parse.c
LOCAL_SRC_FILES += ap20/ap20_set.c
LOCAL_SRC_FILES += ap20/ap20_data_layout.c
LOCAL_SRC_FILES += ap20/ap20_buildbctinterface.c

LOCAL_SRC_FILES += t30/t30_parse.c
LOCAL_SRC_FILES += t30/t30_set.c
LOCAL_SRC_FILES += t30/t30_data_layout.c
LOCAL_SRC_FILES += t30/t30_buildbctinterface.c

LOCAL_SRC_FILES += nvbuildbct_util.c
LOCAL_SRC_FILES += nvbuildbct.c

LOCAL_LDLIBS += -lpthread -ldl

include $(NVIDIA_HOST_STATIC_LIBRARY)
