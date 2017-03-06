LOCAL_PATH:= $(call my-dir)
include $(NVIDIA_DEFAULTS)

# This is the target being built.
LOCAL_MODULE:= libnvhdmi3dplay_jni

# All of the source files that we will compile.
LOCAL_SRC_FILES:= \
    nvhdmi3dplay_jni.cpp

# All of the shared libraries we link against.
LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    liblog \
    libcutils \
    libnativehelper \
    libdl \

# No static libraries.
LOCAL_STATIC_LIBRARIES :=

# Also need the JNI headers.
LOCAL_C_INCLUDES += \
    $(JNI_H_INCLUDE) \

# CFLAGS
# Bug 933892 - Always have 3DV support
LOCAL_CFLAGS = -DHAS_3DV_SUPPORT=1

# Enable the USB host mode menu for boards with host
# mode disabled by default.
ifeq ($(BOARD_USB_HOST_MODE_DISABLED_BY_DEFAULT), true)
LOCAL_CFLAGS += -DENABLE_HOST_MODE_MENU=1
else
LOCAL_CFLAGS += -DENABLE_HOST_MODE_MENU=0
endif

# Don't prelink this library.  For more efficient code, you may want
# to add this library to the prelink map and set this to true.
LOCAL_PRELINK_MODULE := false

include $(NVIDIA_SHARED_LIBRARY)


