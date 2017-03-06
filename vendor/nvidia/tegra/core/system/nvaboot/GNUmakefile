TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs

MODULE_NAME := libnvaboot

LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/system/nv3p
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core/common

LCDEFS += -DCONFIG_PLLP_BASE_AS_408MHZ=$(CONFIG_PLLP_BASE_AS_408MHZ)

ifeq ($(NVODM_BOARD_IS_FPGA),)
ifeq ($(NVODM_BOARD_IS_SIMULATION),)
LCDEFS += -DLPM_BATTERY_CHARGING=1
endif
endif

ifeq ($(TARGET_SOC),t30)
LCDEFS += -DHAS_SE_CONTROLLER_INSTANCE=1
else
LCDEFS += -DHAS_SE_CONTROLLER_INSTANCE=0
endif

C_FILES :=
C_FILES += nvaboot.c
C_FILES += nvaboot_usbf.c
C_FILES += nvaboot_rawfs.c
C_FILES += nvaboot_bootfs.c
C_FILES += nvaboot_blockdev_nice.c
C_FILES += nvaboot_ap20.c
C_FILES += nvaboot_t30.c
C_FILES += nvaboot_warmboot_sign.c

ARM_FILES :=
ARM_FILES += nvaboot_sanitize.arm
ARM_FILES += nvaboot_warmboot_avp_$(TARGET_SOC).arm

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))
OBJS += $(patsubst %.arm,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(ARM_FILES)))

# FIXME: gcc (at least version 4.3.3) space-optimized code (default for EABI)
# doesn't link correctly against the assembly sanitize routines
ifeq ($(BUILD_FLAVOR)-$(USE_GCC_TOOLCHAIN),release-1)
CFLAGS += -O2
endif

$(LIB): $(OBJS)

ifeq ($(TARGET_OS),rvds)
  ifeq ($(TARGET_CPU),armv6)

      default: $(LIB)
	  $(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)

      DEPS :=
      DEPS += $(OBJS)
      DEPS += $(NVLIB_NVOS)
      DEPS += $(NVLIB_NVDDK_NAND)
      DEPS += $(NVLIB_NVOS)
  endif
endif

nosupport:
	@echo "ABoot is not supported for " $(TARGET_OS) " " $(TARGET_CPU)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
