LOCAL_CFLAGS += \
	-Wno-endif-labels \
	-Wno-import \
	-Wno-format \
	-fno-rtti \
	-DENABLE_DEBUGGER_SUPPORT \
	-O3 \
	-fomit-frame-pointer \
	-fstrict-aliasing \
	-fno-tree-vrp

ifeq ($(TARGET_ARCH),arm)
  LOCAL_CFLAGS += -DV8_TARGET_ARCH_ARM
endif

ifeq ($(TARGET_CPU_ABI),armeabi-v7a)
    ifeq ($(ARCH_ARM_HAVE_VFP),true)
        LOCAL_CFLAGS += -DCAN_USE_VFP_INSTRUCTIONS -DCAN_USE_ARMV7_INSTRUCTIONS
    endif
endif

ifeq ($(DEBUG_V8),true)
	LOCAL_CFLAGS += -DDEBUG -UNDEBUG -DOBJECT_PRINT -DV8_ENABLE_CHECKS -DENABLE_DISASSEMBLER -g -fno-omit-frame-pointer
endif
