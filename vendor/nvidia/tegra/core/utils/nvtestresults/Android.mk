LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvtestresults
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvboot/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvboot/ap15/include
LOCAL_SRC_FILES += nvresults.c
LOCAL_SHARED_LIBRARIES += libnvtestio
LOCAL_SHARED_LIBRARIES += libnvos

LOCAL_CFLAGS += -Wno-error=sign-compare
LOCAL_CFLAGS += -Wno-missing-field-initializers
include $(NVIDIA_SHARED_LIBRARY)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvtestresults
LOCAL_MODULE_TAGS := eng
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvboot/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvboot/ap15/include
LOCAL_SRC_FILES += nvresults.c

include $(NVIDIA_HOST_STATIC_LIBRARY)
