#
# Copyright (c) 2008 - 2012 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs

#include $(TEGRA_TOP)/core-private/make/Makefile.projects

MODULE_NAME := libnv3p

ifeq ($(NVODM_BOARD_IS_FPGA), 1)
CFLAGS += -DNVODM_BOARD_IS_FPGA=1
else
CFLAGS += -DNVODM_BOARD_IS_FPGA=0
endif

ifneq ($(NO_SRC_MODULES),1)
 ifeq ($(TARGET_OS_SUBTYPE),android)
  LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)
  TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(LIB_SUFFIXES))
 else
  LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
  TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))
 endif
endif

LIB_HOST_STATIC := $(OUTDIR)/$(MODULE_NAME)_host_static$(LIB_SUFFIX)
TO_BUILD := $(LIB) $(LIB_HOST_STATIC)

# Build the host static lib only on the host
ifeq ($(HOST_BUILD),0)
  ifneq ($(OUTDIR),$(HOST_OUTDIR))
    TO_BUILD := $(LIB)
  endif
endif

#LCDEFS += -DNV_ENABLE_DEBUG_PRINTS=1

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/system/nv3p

ifneq (,$(filter $(TARGET_OS)-$(TARGET_OS_SUBTYPE),rvds-rvds eabi-aos linux-aos))
  LCINCS += -I$(TEGRA_TOP)/core-private/drivers/nvboot
  LCINCS += -I$(TEGRA_TOP)/core/drivers/nvboot
  LCINCS += -I$(TEGRA_TOP)/core/drivers/hwinc
endif

EXPORT_FILES := $(MODULE_NAME).export

C_FILES :=
C_FILES += nv3p.c

ifneq (,$(filter $(TARGET_OS)-$(TARGET_OS_SUBTYPE),rvds-rvds eabi-aos linux-aos))
  C_FILES += nvml_usbf_ap20.c
  C_FILES += nvml_usbf_t30.c
  C_FILES += nv3p_transport_device.c
  C_FILES += nvboot_misc_ap20.c
  C_FILES += nvboot_misc_t30.c
  C_FILES += nv3p_transport_usb_descriptors.c
endif
ifeq ($(TARGET_OS),winxp)
 C_FILES += nv3p_transport_usb_host.c
 ifeq ($(NVODM_BOARD_IS_FPGA),1)
  C_FILES += nv3p_transport_jtag_host.c
 endif
 C_FILES += nv3p_transport_host.c
 C_FILES += nv3p_transport_sema_host.c
 EXTRA_LIB := $(WIN32_PLATFORM_SDK_ROOT)/Lib/WSock32$(LIB_SUFFIX)
endif
ifeq ($(TARGET_OS),linux)
  ifeq ($(TARGET_CPU),armv6)
   ifneq ($(TARGET_OS_SUBTYPE),aos)
    C_FILES += nv3p_transport_stub.c
   endif
  endif
  ifeq ($(TARGET_CPU),x86)
    C_FILES += nv3p_transport_usb_host.c
    C_FILES += nv3p_transport_sema_host.c
    ifeq ($(NVODM_BOARD_IS_FPGA),1)
     C_FILES += nv3p_transport_jtag_host.c
    endif
    C_FILES += nv3p_transport_host.c
  endif
endif

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(TO_BUILD)
	$(DO_HOST_BUILD_TOO)
ifneq ($(NO_SRC_MODULES),1)
 ifeq ($(TARGET_OS_SUBTYPE),android)
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)
 else
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)
 endif
endif

ifeq ($(TARGET_OS_SUBTYPE),android)
 $(LIB): $(OBJS)
else
 $(TARGETS): $(OBJS)
 $(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
endif

ifneq (,$(filter $(TARGET_OS)-$(TARGET_OS_SUBTYPE),rvds-rvds eabi-aos linux-aos))
  ifeq ($(TARGET_CPU),armv4)
    $(TARGETS): $(NVLIB_NVBOOT_USBAPP)
  endif
endif

$(LIB_HOST_STATIC): $(OBJS)

DEPS :=
DEPS += $(OBJS)
DEPS += $(NVLIB_NVOS)

ifeq ($(NV_DEF_USE_DLL),1)
  $(TARGETS): $(NVLIB_NVOS)
endif

ifeq ($(TARGET_OS),winxp)
  $(TARGETS): $(EXTRA_LIB)
  $(TARGETS): $(NVLIB_USBHOST)
endif
ifeq ($(TARGET_OS),linux)
  ifeq ($(TARGET_CPU),x86)
    $(TARGETS): $(NVLIB_USBHOST)
  endif
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules
