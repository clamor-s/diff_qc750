LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libhybrid

LOCAL_CFLAGS += -DHG_NATIVE_FLOATS
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/utils/hybrid/include

LOCAL_SRC_FILES += hg/hgdefs.c
LOCAL_SRC_FILES += hg/hgfill.c
LOCAL_SRC_FILES += hg/hgfloat.c
LOCAL_SRC_FILES += hg/hgint32.c
LOCAL_SRC_FILES += hg/hgint64.c
LOCAL_SRC_FILES += hg/hgmalloc.c
LOCAL_SRC_FILES += hg/hgdebugmalloc.c
LOCAL_SRC_FILES += stitchgen/sgcodecache.c
LOCAL_SRC_FILES += stitchgen/sgstitchinterp.c
LOCAL_SRC_FILES += armemulator/armdis.c
LOCAL_SRC_FILES += armemulator/armdismemory.c
LOCAL_SRC_FILES += armemulator/armemuhelp.c
LOCAL_SRC_FILES += armemulator/armdisexecute.c
LOCAL_SRC_FILES += armemulator/armemu.c
LOCAL_SRC_FILES += armemulator/armemumemory.c
LOCAL_SRC_FILES += armemulator/armdishelp.c
LOCAL_SRC_FILES += armemulator/armemuexecute.c
LOCAL_SRC_FILES += armemulator/armemuvfpexecute.c

LOCAL_NVIDIA_RM_WARNING_FLAGS := -Wundef
LOCAL_CFLAGS += -Wno-error=cast-align
include $(NVIDIA_STATIC_LIBRARY)

