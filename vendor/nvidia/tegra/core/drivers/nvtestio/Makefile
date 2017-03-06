#
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvtestio

LIB     := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
LIB_HOST_STATIC := $(OUTDIR)/$(MODULE_NAME)_host_static$(LIB_SUFFIX)
TARGETS := $(LIB)

TO_BUILD := $(LIB) $(LIB_HOST_STATIC)

# Build the host static lib only on the host
ifeq ($(HOST_BUILD),0)
  ifneq ($(OUTDIR),$(HOST_OUTDIR))
    TO_BUILD := $(LIB)
  endif
endif

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -Iinclude
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += $(NVLIB_HWINC_SOC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core-private/drivers/nvboot
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvboot
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvboot/ap15/include
LCINCS += -I$(TEGRA_TOP)/core/utils/nvos/aos
LCINCS += -I$(TEGRA_TOP)/core/utils/nvos/aos/nvap

ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LCDEFS += -DSET_KERNEL_PINMUX
endif

#
# Do not change these here.
# Some of these are set to 1 below depending on build.
#
TIO_HOST_SUPPORTED      := 0
TIO_POLL_SUPPORTED       := 0
TIO_LISTEN_SUPPORTED    := 0
TIO_TARGET_SUPPORTED    := 0
TIO_CACHE_SUPPORTED     ?= 0
TIO_RELI_SUPPORTED      := 0
TIO_GDBT_SUPPORTED      := 0
TIO_NVOS_SUPPORTED      := 0
TIO_RMSHMOO_SUPPORTED   := 0
TIO_RM_REQUIRED         := 0
#
# Things that are supported in all builds
#
TIO_RELI_SUPPORTED      := 1
TIO_GDBT_SUPPORTED      := 1
TIO_NVOS_SUPPORTED      := 1

DEPS :=
C_FILES :=
C_FILES += tio_vfs.c
C_FILES += tio_remote.c

ifeq ($(TARGET_OS),linux)
  C_FILES += tio_linux.c
  C_FILES += tio_linux_poll.c
  C_FILES += tio_tcp.c
  TIO_HOST_SUPPORTED   := 1
  TIO_POLL_SUPPORTED    := 1
  TIO_LISTEN_SUPPORTED    := 1
  TIO_TARGET_SUPPORTED := 1
  ifeq ($(TARGET_OS_SUBTYPE), rh9)
     override TIO_CACHE_SUPPORTED := 0
     DEPS += $(NVLIB_USBHOST)
     C_FILES += tio_linux_usb.c
     SYSTEM_LDLIBS_DLL += -lpthread
  else
     C_FILES += tio_null_usb.c
  endif
endif

ifeq ($(TARGET_OS),winxp)
  C_FILES += tio_winnt.c
  C_FILES += tio_winnt_poll.c
  C_FILES += tio_winnt_usb.c
  C_FILES += tio_tcp.c
  TIO_HOST_SUPPORTED   := 1
  TIO_POLL_SUPPORTED    := 1
  TIO_LISTEN_SUPPORTED    := 1
  override TIO_CACHE_SUPPORTED  := 0
  DEPS += $(NVLIB_USBHOST)
  LDFLAGS_DLL += Ws2_32.lib
endif
ifeq ($(TARGET_OS),qnx)
  C_FILES += tio_null_tcp.c
  C_FILES += tio_null_usb.c
  C_FILES += tio_null_uart.c
  C_FILES += tio_null_stdio.c
endif

ifneq (,$(filter $(strip $(TARGET_OS)),rvds eabi))
  LCINCS += $(NVLIB_HWINC_INCLUDES)

  #
  # just use one of the following 2
  #    tio_uart_ap15hw.c      - basic driver, r/w hardcoded registers
  #    tio_uart_nvddk.c       - use nvddk uart driver (broken)
  #
  C_FILES += tio_uart_ap15hw.c
  #C_FILES += tio_uart_nvddk.c

  ifeq ($(IS_AVP),1)
    C_FILES += tio_usb_ap15hw.c
    DEPS += $(TEGRA_TOP)/core-private/drivers/nvboot/$(TARGET_SOC)/usbf/$(OUTPUT_DIRNAME)/libnvboot_usbf$(LIB_SUFFIX)
  endif
  ifeq ($(IS_AVP),0)
    C_FILES += tio_usb_nvddk.c
    TIO_RMSHMOO_SUPPORTED := 1
  endif

  TIO_TARGET_SUPPORTED := 1

  C_FILES += tio_null_tcp.c
  C_FILES += tio_null_stdio.c
endif

ifeq ($(TIO_HOST_SUPPORTED),1)
  C_FILES += tio_host.c
  C_FILES += tio_host_stdio.c
else
  C_FILES += tio_null_host.c
endif

ifeq ($(TIO_LISTEN_SUPPORTED),1)
  C_FILES += tio_listen.c
else
  C_FILES += tio_null_listen.c
endif

ifeq ($(TIO_POLL_SUPPORTED),0)
  C_FILES += tio_null_poll.c
endif

ifeq ($(TIO_TARGET_SUPPORTED),1)
  C_FILES += tio_target.c
else
  C_FILES += tio_null_target.c
endif

ifeq ($(TIO_CACHE_SUPPORTED),1)
  C_FILES += tio_file_cache.c
  LCDEFS += -DTIO_CACHE_SUPPORTED
else
  C_FILES += tio_null_file_cache.c
endif

ifeq ($(TIO_RELI_SUPPORTED),1)
  C_FILES += tio_reliable.c
else
  C_FILES += tio_null_reliable.c
endif

ifeq ($(TIO_GDBT_SUPPORTED),1)
  C_FILES += tio_gdbt.c
  ifeq ($(TIO_HOST_SUPPORTED),1)
    C_FILES += tio_gdbt_host.c
  endif
  ifeq ($(TIO_TARGET_SUPPORTED),1)
    C_FILES += tio_gdbt_target.c
    C_FILES += tio_shmoo.c

    ifeq ($(TIO_RMSHMOO_SUPPORTED),1)
      C_FILES += tio_shmoo_rm.c
      TIO_RM_REQUIRED := 1
    else
      C_FILES += tio_shmoo_norm.c
    endif
  endif
else
  C_FILES += tio_null_gdbt.c
endif

ifeq ($(TIO_NVOS_SUPPORTED),1)
  C_FILES += tio_nvos.c
  ifeq ($(NV_DEF_USE_DLL),1)
    DEPS += $(NVLIB_NVOS)
    DEPS += $(NVLIB_APPUTIL)
    ifeq ($(TIO_RM_REQUIRED),1)
      DEPS += $(NVLIB_NVRM)
    endif
  endif
else
  C_FILES += tio_null_nvos.c
endif

EXPORT_FILES := $(MODULE_NAME).export

# OBJECT files
#######################################
OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

# Build Rules
#######################################
default: $(TO_BUILD)
	$(DO_HOST_BUILD_TOO)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

DEPS += $(OBJS)
DEPS += $(OUTDIR)/libnvtestio.def
$(LIB): $(DEPS)
$(LIB_HOST_STATIC): $(OBJS)

RELEASE_PACKAGE_TARGETS := $(TARGETS)
RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
