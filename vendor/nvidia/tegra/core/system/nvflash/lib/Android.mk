LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvflash

LOCAL_CFLAGS += -DNVODM_BOARD_IS_FPGA=0

LOCAL_SRC_FILES += nvflash_commands.c
LOCAL_SRC_FILES += nvflash_configfile.c
LOCAL_SRC_FILES += nvflash_configfile.tab.c
LOCAL_SRC_FILES += nvflash_verifylist.c
LOCAL_SRC_FILES += lex.yy.c

include $(NVIDIA_HOST_STATIC_LIBRARY)