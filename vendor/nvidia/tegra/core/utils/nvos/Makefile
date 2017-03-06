# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.

TOPDIR := ../..

include $(TEGRA_TOP)/core-private/make/Makefile.defs

LCINCS += $(NVLIB_COMMON_INCLUDES)
ifneq (,$(filter $(TARGET_CPU),armv4 armv6))
  ifeq ($(TARGET_OS),rvds)
    #AOS build with RVDS
    LCINCS += -I$(TEGRA_TOP)/core/include/nvposix
  endif
  ifeq ($(TARGET_OS_SUBTYPE),aos)
    #AOS build with CodeSourcery
    LCINCS += -I$(TEGRA_TOP)/core/include/nvposix
  endif
endif
LCINCS += -I$(TEGRA_TOP)/core/utils/nvos/aos/nvap
LCINCS += -I.
LCINCS += -I$(TEGRA_TOP)/core/drivers/hwinc/$(TARGET_TEGRA_FAMILY)

MODULE_NAME := libnvos

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
LIB_HOST_STATIC := $(OUTDIR)/$(MODULE_NAME)_host_static$(LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

TO_BUILD := $(LIB) $(LIB_HOST_STATIC)

# Advanced debug feature disabled until Bug 950465 fixed
ifeq ($(BUILD_FLAVOR),debug)
  ifneq ($(TARGET_OS)-$(TARGET_OS_SUBTYPE),rvds-rvds)
    NVOS_ADVANCED_DEBUG=0
  endif
endif

ifeq ($(TARGET_OS),qnx)
  NVOS_ADVANCED_DEBUG := 0
endif

ifeq ($(NVOS_ADVANCED_DEBUG),1)
  LCDEFS := -DNVOS_ADVANCED_DEBUG=1
else
  LCDEFS := -DNVOS_ADVANCED_DEBUG=0
endif
LCDEFS += -DCONFIG_PLLP_BASE_AS_408MHZ=$(CONFIG_PLLP_BASE_AS_408MHZ)

# Build the host static lib only on the host
ifeq ($(HOST_BUILD),0)
  ifneq ($(OUTDIR),$(HOST_OUTDIR))
    TO_BUILD := $(LIB)
  endif
endif

ifeq ($(TARGET_CPU),armv6)
  # AP20 uses the legacy address map with SDRAM at address zero.
  # Everything else will use the newer address map.
  ifneq ($(filter ap20,$(TARGET_SOC)),)
    CFLAGS += -DNV_AOS_ENTRY_POINT=0x108000
    CFLAGS += -DAES_KEYSCHED_LOCK_WAR_BUG_598910=1
  else
    CFLAGS += -DNV_AOS_ENTRY_POINT=0x80108000
    CFLAGS += -DAES_KEYSCHED_LOCK_WAR_BUG_598910=0
  endif
  ifneq ($(filter t30,$(TARGET_SOC)),)
    CFLAGS += -DSE_AES_KEYSCHED_READ_LOCK=1
  else
    CFLAGS += -DSE_AES_KEYSCHED_READ_LOCK=0
  endif

  # Don't enable NEON on AOS -- there are too many versions of the compiler
  # floating around with different levels of VFP support. So disable them all.
  ifneq ($(TARGET_OS)-$(TARGET_OS_SUBTYPE),rvds-rvds)
    ifneq ($(ARCH_ARM_HAVE_NEON),)
      CFLAGS += -DARCH_ARM_HAVE_NEON=1
    endif
  endif
endif

ifeq ($(TARGET_DEVICE),cardhu)
LCDEFS  += -DSET_I2C_EXPANDER
endif

#use kernel pinmux implementation
ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LCDEFS += -DSET_KERNEL_PINMUX
endif

ifeq ($(TARGET_CPU),armv4)
  LCINCS += -I$(OUTDIR)
endif

ifeq ($(AOS_MON_MODE_ENABLE),1)
    CFLAGS += -DAOS_MON_MODE_ENABLE=1
else
    CFLAGS += -DAOS_MON_MODE_ENABLE=0
endif

C_FILES :=
S_FILES :=
ifneq ($(TARGET_CPU),armv4)
  C_FILES += nvos_debug.c
  C_FILES += nvos_internal.c
  C_FILES += nvos_alloc.c
  C_FILES += nvos_pointer_hash.c
  C_FILES += ../nvosutils/nvustring.c
  C_FILES += ../nvosutils/nvuhash.c
  C_FILES += nvos_callstack.c
endif
ifeq ($(TARGET_OS),darwin)
  C_FILES += linux/nvos_linux.c
  C_FILES += linux/nvos_linux_user.c
  C_FILES += linux/nvos_linux_debugcomm.c
  C_FILES += linux/nvos_linux_host_stub.c
  C_FILES += nvos_thread.c
  C_FILES += nvos_coop_thread.c
  C_FILES += nvos_trace.c
  C_FILES += nvos_file.c
  C_FILES += nvos_config.c
  ifeq ($(BUILD_FLAVOR),profile)
    LCDEFS += -DBUILD_NO_DL_PROFILE
  endif
  LDFLAGS_DLL += -lm
  LDFLAGS_DLL += -ldl
  LDFLAGS_DLL += -lpthread
  # cross-compile for leopard
  CC := $(GCCBASE)/bin/gcc-4.0
  CXX := $(GCCBASE)/bin/g++-4.0
  CPP := $(GCCBASE)/bin/cpp-4.0 -E
  LCOPTS += -iwithsysroot /Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5
  LDFLAGS_DLL += -macosx_version_min 10.5 -lgcc_s.10.5
endif
ifneq (,$(filter $(TARGET_OS_SUBTYPE),android rh9 chromiumos gnu_linux))
  ifeq ($(TARGET_OS_SUBTYPE),android)
    LCDEFS += -DANDROID
    # Enable next three lines to send debug prints to the ADB log.
    LCINCS += -I$(ANDROID_ROOT)/system/core/include
    LDFLAGS_DLL += -L$(ANDROID_ROOT)/out/target/product/tegra/obj/lib -llog -lcutils
  endif

  C_FILES += linux/nvos_linux.c
  C_FILES += linux/nvos_linux_stub.c
  C_FILES += linux/nvos_linux_user.c
  C_FILES += linux/nvos_linux_debugcomm.c
  C_FILES += linux/nvos_linux_settings.c
  C_FILES += nvos_thread.c
  C_FILES += nvos_coop_thread.c
  C_FILES += nvos_trace.c
  C_FILES += nvos_file.c
  C_FILES += nvos_config.c
  C_FILES += ../nvosutils/nvustring.c
  C_FILES += ../nvosutils/nvuhash.c
  ifeq ($(BUILD_FLAVOR),profile)
    LCDEFS += -DBUILD_NO_DL_PROFILE
  endif
  LDFLAGS_DLL += -lm
  LDFLAGS_DLL += -ldl
  ifneq ($(TARGET_OS_SUBTYPE),android)
    LDFLAGS_DLL += -lpthread
  endif
endif
ifeq ($(TARGET_OS),qnx)
  C_FILES += qnx/nvos_qnx.c
  C_FILES += nvos_thread.c
  C_FILES += nvos_coop_thread.c
  C_FILES += nvos_trace.c
  C_FILES += nvos_file.c
  C_FILES += nvos_config.c
  ifeq ($(BUILD_FLAVOR),profile)
    LCDEFS += -DBUILD_NO_DL_PROFILE
  endif
  LDFLAGS_DLL += -lm
endif
ifeq ($(TARGET_OS_WINDOWS),1)
  C_FILES += nvos_thread.c
  C_FILES += nvos_coop_thread.c
  C_FILES += win32/nvos_win32.c
  C_FILES += nvos_file.c
  C_FILES += nvos_trace.c
  C_FILES += nvos_config.c
  ifeq ($(TARGET_OS),winxp)
    C_FILES += win32/nvos_winnt.c
  endif
endif
ifneq (,$(filter $(TARGET_OS)-$(TARGET_OS_SUBTYPE),eabi-aos rvds-rvds linux-aos))
  LCINCS += $(NVLIB_HWINC_INCLUDES)
  LCINCS += -I$(TEGRA_TOP)/core/include
  LCINCS += -Iaos
  LCINCS += -I$(TEGRA_TOP)/core/utils/aes_keysched_lock
  LCINCS += -I$(TEGRA_TOP)/core/drivers/hwinc/$(TARGET_SOC_FAMILY)
  LCINCS += -I$(TEGRA_TOP)/core/drivers/nvavpgpio

  ifeq ($(TARGET_CPU),armv4)
    C_FILES += aos/nvos_aos_swi.c
    # FIXME: this should be handled without a submake?
    SUBDIRS +=  $(TEGRA_TOP)/core/utils/nvos/aos/avp
  else
    ifeq ($(USE_GCC_TOOLCHAIN),1)
      LCDEFS += -DHAVE_USR_INCLUDE_MALLOC_H
      # remove this conditional as soon as linux-aos is deprecated
      ifneq ($(TARGET_OS),linux)
        C_FILES += aos/nvap/aos_posix.c
      endif
    endif
    ifeq ($(TARGET_BOARD),p1852)
      LCDEFS += -DDISABLE_AHB_SPLIT_TRANSACTIONS
    endif
    LCDEFS += -DNVAOS_SHELL=0
    C_FILES += nvos_file.c
    C_FILES += aos/nvos_aos_core.c
    C_FILES += aos/nvos_aos.c
    C_FILES += nvos_thread.c
    C_FILES += nvos_coop_thread.c
    C_FILES += nvos_trace.c
    C_FILES += aos/dlmalloc.c
    C_FILES += aos/aos_process_args.c
    C_FILES += aos/aos_profile.c
    C_FILES += aos/aos_semi_rvice.c
    C_FILES += aos/aos_print.c
    C_FILES += aos/aos_semi_uart.c
    C_FILES += aos/nvos_aos_semi.c
    C_FILES += aos/nvap/init_fpu.c
    C_FILES += aos/nvap/bootloader.c
    C_FILES += aos/nvap/bootloader_ap20.c
    C_FILES += aos/nvap/bootloader_t30.c
    C_FILES += aos/nvap/aos_cpu.c
    C_FILES += aos/nvap/aos_cpu_t30.c
    C_FILES += aos/nvap/aos_pl310.c
    C_FILES += aos/nvap/aos_scu.c
    C_FILES += aos/nvap/init_cpu.c
    C_FILES += aos/nvap/aos_odmoption_ap20.c
    C_FILES += aos/nvap/aos_odmoption_t30.c
    C_FILES += aos/nvap/nvos_aos_gcc.c
    C_FILES += aos/nvap/aos_uart.c
    ifneq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
      ifneq ($(filter ap20,$(TARGET_SOC)),)
        C_FILES += aos/nvap/aos_uart_ap20.c
      else
        C_FILES += aos/nvap/aos_uart_t30.c
      endif
    endif
    C_FILES += aos/nvap/aos_cpu_odmstub.c
    C_FILES += aos/nvap/aos_mon_mode.c
    C_FILES += aos/nvap/aos_ns_mode.c
  endif
endif

EXPORT_FILES := $(MODULE_NAME).export

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))
OBJS += $(patsubst %.S,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(S_FILES)))

OBJS_HOST_STATIC := $(OBJS)

ifneq (,$(filter $(TARGET_OS),linux darwin))
  ifneq ($(TARGET_OS_SUBTYPE),aos)
    SO_MAIN := linux/nvos_main.c
    C_FILES += $(SO_MAIN)
    OBJS += $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(SO_MAIN)))
  endif
endif

default: $(TO_BUILD)
	$(DO_HOST_BUILD_TOO)
ifneq ($(TARGETS),)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)
endif

$(LIB): $(OBJS)
$(LIB_HOST_STATIC): $(OBJS_HOST_STATIC)

ifneq ($(HOST_OS),darwin)
ifneq ($(NON_OS_BUILD),1) # We get link errors when we try to use this for AOS...
  $(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
endif
endif

ifeq ($(TARGET_CPU),armv4)
  $(OUTDIR)/nvos_swi_client.h: $(TEGRA_TOP)/core-private/make/swigen.py $(OUTDIR)/dummy.txt
	$(PYTHON) $< -client NvOs >$@

  $(OUTDIR)/nvmm_swi_client.h: $(TEGRA_TOP)/core-private/make/swigen.py $(OUTDIR)/dummy.txt
	$(PYTHON) $< -client NvMM >$@

  $(OUTDIR)/nvos_aos_swi$(OBJ_SUFFIX): $(OUTDIR)/nvos_swi_client.h
  $(OUTDIR)/nvos_aos_swi$(OBJ_SUFFIX): $(OUTDIR)/nvmm_swi_client.h
endif

ifneq (,$(filter $(TARGET_OS),linux darwin))
ifneq ($(TARGET_OS_SUBTYPE),aos)

GLOBAL_NVOS_EXPORTS_C_FILE := $(OUTDIR)/nvos_exports.c
default: $(GLOBAL_NVOS_EXPORTS_C_FILE)
$(GLOBAL_NVOS_EXPORTS_C_FILE): libnvos.export Makefile
	$(RM) $@ $@.1
	$(SMAKE) exports-c-file-gen > $@.1
	$(MV) $@.1 $@

.PHONY: exports-c-file-gen
exports-c-file-gen: libnvos.export
	@echo "/* This file is autogenerated by the Makefile $@ rule from $< */"
	@echo " * This file should be hand copied to */"
	@echo " *    <LINUX_KERNEL>/common/arch/arm/mach-tegra/utils/nvos */"
	@echo " * "
	@echo " */"
	@echo "/*"
	@echo " * Copyright (c) 2007-2012, NVIDIA CORPORATION.  All rights reserved."
	@echo " *"
	@echo " * NVIDIA Corporation and its licensors retain all intellectual"
	@echo " * property and proprietary rights in and to this software,"
	@echo " * related documentation and any modifications thereto.  Any use,"
	@echo " * reproduction, disclosure or distribution of this software and"
	@echo " * related documentation without an express license agreement"
	@echo " * from NVIDIA Corporation is strictly prohibited."
	@echo " */"
	@echo ""
	@echo '#include "nvos.h"'
	@echo '#include "nvutil.h"'
	@echo '#include "nvassert.h"'
	@echo '#if NVOS_IS_LINUX_KERNEL'
	@echo "#include <linux/module.h>"
	@$(SED) -n -e 's|^\(NvOs[^@]*\).*|EXPORT_SYMBOL(\1);|p' $< | \
	    grep -v -E 'NvOsDump|NvOsCallstack|NvOsCheckpoint|NvOsVerifyHeap|NvOsGetResTrackerStat|NvOsSetAllocHook|NvOsGetProcessInfo|NvOsCallstack|NvOsResourceInfo|NvOsListResources|NvOsShowError|NvOsDataCacheWritebackInvalidate|NvOsDataCacheWriteback|NvOsExecFree|NvOsFremove'
	@echo '#endif /* NVOS_IS_LINUX_KERNEL */'

.PHONY: export-copy export-copy2
export-copy: $(GLOBAL_NVOS_EXPORTS_C_FILE)
	@$(SMAKE) export-copy2 \
	    SRC=$< \
	    DST=$(P4ROOT)/sw/mobile/linux/kernel/android-2.6.29/common/arch/arm/mach-tegra/utils/nvos/$(notdir $<)
export-copy2: $(SRC) $(DST)
	echo ""
	echo ""
	echo "Run these commands to copy the exports file:"
	echo "   p4 edit $(DST)"
	echo "   $(SED) -n -e '/^\/\*$$/,\$$p' $(SRC) > $(DST)"

endif # ($(TARGET_OS_SUBTYPE),aos)
endif # ($(TARGET_OS),linux)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
