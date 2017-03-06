#
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/../core-private/make/Makefile.defs


MODULE_NAME := libnvodm_imager

ifeq ($(TARGET_BOARD),aruba2)
LCDEFS += -DBUILD_FOR_ARUBA=1
else
LCDEFS += -DBUILD_FOR_ARUBA=0
endif


ifeq (,$(filter-out rvds eabi, $(TARGET_OS)))
LCDEFS += -DBUILD_FOR_AOS=1
else
LCDEFS += -DBUILD_FOR_AOS=0
endif

ifeq ($(TARGET_BOARD),harmony)
LCDEFS += -DBUILD_FOR_HARMONY=1
endif
ifeq ($(TARGET_BOARD),ventana)
LCDEFS += -DBUILD_FOR_VENTANA=1
endif
ifeq ($(TARGET_BOARD),cardhu)
LCDEFS += -DBUILD_FOR_CARDHU=1
endif
ifeq ($(TARGET_BOARD),kai)
LCDEFS += -DBUILD_FOR_KAI=1
endif
ifeq ($(TARGET_BOARD),p1852)
LCDEFS += -DBUILD_FOR_P1852=1
endif
ifeq ($(TARGET_BOARD),e1853)
LCDEFS += -DBUILD_FOR_E1853=1
endif

ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LCDEFS += -DSET_KERNEL_PINMUX

endif

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I./
LCINCS += -I./configs/
LCINCS += -I../../customers/nvidia-partner/template/odm_kit/query/include/
LCINCS += -I../../odm/template/odm_kit/query/include/
LCINCS += -I$(KERN_DIR)/include
LCINCS += -I$(KERN_DIR)/arch/arm/include

EXPORT_FILES := $(MODULE_NAME).export

C_FILES :=
C_FILES += imager_hal.c
C_FILES += imager_util.c
C_FILES += sensor_yuv_soc380.c
C_FILES += sensor_yuv_ov5640.c
C_FILES += sensor_bayer_ov5630.c
C_FILES += sensor_bayer_ov5650.c
C_FILES += sensor_bayer_ov9726.c
C_FILES += sensor_bayer_ov14810.c
C_FILES += sensor_bayer_ov2710.c
C_FILES += sensor_bayer_ar0832.c
C_FILES += sensor_null.c
C_FILES += sensor_host.c
C_FILES += focuser_ad5820.c
C_FILES += focuser_sh532u.c
C_FILES += focuser_ar0832.c
C_FILES += focuser_nvc.c
C_FILES += flash_ltc3216.c
C_FILES += flash_ssl3250a.c
C_FILES += flash_tps61050.c
C_FILES += torch_nvc.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(NVODMLIB_INSTALL_TARGET)

$(TARGETS): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
ifeq ($(NV_DEF_USE_DLL),1)
  $(TARGETS): $(NVLIB_NVRM)
  $(TARGETS): $(NVLIB_NVOS)
  $(TARGETS): $(NVLIB_NVODM_SERVICES)
  $(TARGETS): $(NVLIB_NVODM_QUERY)
endif


include $(TOPDIR)/../core-private/make/Makefile.rules
