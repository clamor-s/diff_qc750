#
# Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
#

# Build a static library that will be run on AVP (ARMv4 only).

LOCAL_MODULE_CLASS := STATIC_LIBRARIES

LOCAL_NO_DEFAULT_COMPILER_FLAGS := true

LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)
LOCAL_C_INCLUDES += $(TOP)/bionic/libc/arch-arm/include
LOCAL_C_INCLUDES += $(TOP)/bionic/libc/include
LOCAL_C_INCLUDES += $(TOP)/bionic/libc/kernel/common
LOCAL_C_INCLUDES += $(TOP)/bionic/libc/kernel/arch-arm


LOCAL_AVP_CFLAGS := -march=armv4t
LOCAL_AVP_CFLAGS += -Os
LOCAL_AVP_CFLAGS += -fno-unwind-tables
LOCAL_AVP_CFLAGS += -fomit-frame-pointer
LOCAL_AVP_CFLAGS += -ffunction-sections
LOCAL_AVP_CFLAGS += -fdata-sections
LOCAL_AVP_CFLAGS += -DWIN_INTERFACE_CUSTOM
LOCAL_AVP_CFLAGS += -DNO_MALLINFO
LOCAL_AVP_CFLAGS += -UDEBUG -U_DEBUG -DNDEBUG -DNV_DEBUG=0

# Let the user override flags above.
LOCAL_MODULE_CFLAGS := $(LOCAL_CFLAGS)
LOCAL_CFLAGS := $(LOCAL_AVP_CFLAGS)
LOCAL_CFLAGS += $(LOCAL_MODULE_CFLAGS)

# Froyo arm-eabi-4.4.0 toolchain does not produce valid code for
# ARMv4T ARM/Thumb interworking.
ifeq ($(AVP_EXTERNAL_TOOLCHAIN),)
  LOCAL_CFLAGS += -mno-thumb-interwork
  ifneq ($(LOCAL_ARM_MODE),thumb)
    LOCAL_CFLAGS += -mno-thumb
    LOCAL_CFLAGS += -U__thumb
  endif
else
  LOCAL_CFLAGS += -mthumb-interwork
  ifneq ($(LOCAL_ARM_MODE),arm)
    LOCAL_CFLAGS += -mthumb
    LOCAL_CFLAGS += -D__thumb
  endif
  LOCAL_CC := $(AVP_EXTERNAL_TOOLCHAIN)gcc
endif

# Override the ARM vs. Thumb default above.
ifeq ($(LOCAL_ARM_MODE),arm)
  LOCAL_CFLAGS += -mno-thumb
  LOCAL_CFLAGS += -U__thumb
endif
ifeq ($(LOCAL_ARM_MODE),thumb)
  LOCAL_CFLAGS += -mthumb
  LOCAL_CFLAGS += -D__thumb
endif

include $(NVIDIA_BASE)
include $(BUILD_STATIC_LIBRARY)
