LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := nvflash

LOCAL_CFLAGS += -DNVODM_BOARD_IS_FPGA=0
LOCAL_CFLAGS += -DNVODM_ENABLE_SIMULATION_CODE=1

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/system/nvflash/lib
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvboot/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvboot/ap20/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvboot/t30/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/system/nvdioconverter
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/system/nvbuildbct
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)

LOCAL_SRC_FILES += nvflash_app.c
LOCAL_SRC_FILES += nvflash_util.c
LOCAL_SRC_FILES += nvflash_usage.c
LOCAL_SRC_FILES += nvflash_util_ap20.c
LOCAL_SRC_FILES += nvflash_util_t30.c
LOCAL_SRC_FILES += nvflash_hostblockdev.c

LOCAL_STATIC_LIBRARIES += libnvapputil
LOCAL_STATIC_LIBRARIES += libnv3p
LOCAL_STATIC_LIBRARIES += libnv3pserverhost
LOCAL_STATIC_LIBRARIES += libnvbcthost
LOCAL_STATIC_LIBRARIES += libnvpartmgrhost
LOCAL_STATIC_LIBRARIES += libnvbootupdatehost
LOCAL_STATIC_LIBRARIES += libnvsystem_utilshost
LOCAL_STATIC_LIBRARIES += libnvflash
LOCAL_STATIC_LIBRARIES += libnvdioconverter
LOCAL_STATIC_LIBRARIES += libnvtestresults
LOCAL_STATIC_LIBRARIES += libnvboothost
LOCAL_STATIC_LIBRARIES += libnvusbhost
LOCAL_STATIC_LIBRARIES += libnvos
LOCAL_STATIC_LIBRARIES += libnvaes_ref
LOCAL_STATIC_LIBRARIES += libnvswcrypto
LOCAL_STATIC_LIBRARIES += libnvbuildbct
ifeq ($(NV_EMBEDDED_BUILD),1)
LOCAL_STATIC_LIBRARIES += libnvskuhost
endif

LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_HEADERS)/nvflash_miniloader_ap20.h
LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_HEADERS)/nvflash_miniloader_t30.h

LOCAL_LDLIBS += -lpthread -ldl

ifeq ($(HOST_OS),darwin)
LOCAL_LDLIBS += -lpthread -framework CoreFoundation -framework IOKit \
	-framework Carbon
endif

include $(NVIDIA_HOST_EXECUTABLE)

