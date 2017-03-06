LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvfs
LOCAL_NVIDIA_NO_COVERAGE := true

LOCAL_C_INCLUDES += $(LOCAL_PATH)/basic
LOCAL_C_INCLUDES += $(LOCAL_PATH)/enhanced
LOCAL_C_INCLUDES += $(LOCAL_PATH)/ext2
LOCAL_C_INCLUDES += $(LOCAL_PATH)/yaffs2

LOCAL_SRC_FILES += basic/nvbasicfilesystem.c
LOCAL_SRC_FILES += enhanced/nvenhancedfilesystem.c
LOCAL_SRC_FILES += ext2/nvext2filesystem.c
LOCAL_SRC_FILES += ext2/nvext2operations.c
LOCAL_SRC_FILES += yaffs2/nvyaffs2filesystem.c
LOCAL_SRC_FILES += yaffs2/nvyaffs2nandfs.c

LOCAL_CFLAGS += -Wno-error=sign-compare
LOCAL_NVIDIA_NO_EXTRA_WARNINGS := 1
include $(NVIDIA_STATIC_LIBRARY)
