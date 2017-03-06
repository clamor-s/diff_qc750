LOCAL_PATH := $(call my-dir)

# libnvrm_graphics (channel & moduleloader stubs)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvrm_graphics
LOCAL_C_INCLUDES += $(LOCAL_PATH)/common
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../nvrmkernel/core/common

LOCAL_SRC_FILES += common/nvrm_disasm.c
LOCAL_SRC_FILES += common/nvrm_stream.c
LOCAL_SRC_FILES += common/nvsched.c
LOCAL_SRC_FILES += common/nvrm_channel_linux.c
LOCAL_SRC_FILES += ap20/ap20sched.c
LOCAL_SRC_FILES += ap20/ap20rm_stream_parse.c

LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_SHARED_LIBRARIES += libnvrm
LOCAL_SHARED_LIBRARIES += liblog

include $(NVIDIA_SHARED_LIBRARY)

# libnvrm_channel_impl for bootloader

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvrm_channel_impl
LOCAL_NVIDIA_NO_COVERAGE := true
LOCAL_C_INCLUDES += $(LOCAL_PATH)/common
LOCAL_CFLAGS += -DNVRM_TRANSPORT_IN_KERNEL=1

LOCAL_SRC_FILES += common/nvrm_disasm.c
LOCAL_SRC_FILES += common/nvrm_stream.c
LOCAL_SRC_FILES += common/nvsched.c
LOCAL_SRC_FILES += common/nvrm_graphics_init.c
LOCAL_SRC_FILES += ap20/ap20sched.c
LOCAL_SRC_FILES += ap20/ap20rm_stream_parse.c
LOCAL_SRC_FILES += ap20/ap20rm_hostintr.c
LOCAL_SRC_FILES += ap20/ap20rm_hwcontext.c
LOCAL_SRC_FILES += ap20/ap20rm_hwcontext_3d.c
LOCAL_SRC_FILES += ap20/ap20rm_hwcontext_mpe.c
LOCAL_SRC_FILES += ap20/ap20rm_channel.c

include $(NVIDIA_STATIC_LIBRARY)

