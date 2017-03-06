LOCAL_PATH := $(call my-dir)

#LOCAL_MODULE_PATH := $(TARGET_OUT_EXE)/hw
# akmd2
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := \
	$(KERNEL_HEADERS) \
	$(LOCAL_PATH)/libAK8975 

LOCAL_SRC_FILES:= \
	AK8975Driver.c \
	DispMessage.c \
	FileIO.c \
	Measure.c \
	main.c \
	misc.c

LOCAL_CFLAGS += \
	-Wall \
	-DENABLE_AKMDEBUG=1 \
	-DOUTPUT_STDOUT=1 \
	-DDBG_LEVEL=3

LOCAL_LDFLAGS += -L$(LOCAL_PATH)/libAK8975 -lAK8975
LOCAL_MODULE := akmd8975_service
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := false
LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := libc libm libz libutils libcutils
include $(BUILD_EXECUTABLE)


