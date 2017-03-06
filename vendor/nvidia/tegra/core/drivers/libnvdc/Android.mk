LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvdc

LOCAL_CFLAGS += -Wno-missing-field-initializers

LOCAL_SRC_FILES += \
	caps.c \
	csc.c \
	lut.c \
	cursor.c \
	displays.c \
	events.c \
	flip.c \
	modes.c \
	openclose.c \
	util.c \

LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_SHARED_LIBRARIES += libnvrm

include $(NVIDIA_SHARED_LIBRARY)
