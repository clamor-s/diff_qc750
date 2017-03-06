TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs
include $(TEGRA_TOP)/core-private/make/Makefile.osconfig

NV_IS_LDK := 0
ifneq (,$(filter $(TARGET_OS)-$(TARGET_OS_SUBTYPE), linux-chromiumos linux-gnu_linux))
  NV_IS_LDK := 1
endif
LCDEFS += -DNV_IS_LDK=$(NV_IS_LDK)

EXECUTABLES := $(OUTDIR)/tegrastats$(EXE_SUFFIX)
C_FILES := main.c
OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))
LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I$(ANDROID_ROOT)/frameworks/base/include/

ifeq (,$(filter $(TARGET_OS)-$(TARGET_OS_SUBTYPE), linux-chromiumos linux-gnu_linux))
  SYSTEM_LDLIBS_EXE += -lcutils
endif

default: $(EXECUTABLES)
	$(NV_INSTALL) -l $(EXECUTABLES) $(INSTALL_TARGET)

install: $(EXECUTABLES)
	adb remount && adb -d push $(EXECUTABLES) /system/bin/tegrastats

$(EXECUTABLES): $(OBJS)
$(EXECUTABLES): $(NVLIB_NVRM)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
