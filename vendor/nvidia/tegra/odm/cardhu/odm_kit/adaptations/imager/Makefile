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

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

#LCDEFS += -DNV_ENABLE_DEBUG_PRINTS=1

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I../common
LCINCS += -I../../query/include
LCINCS += -I.
LCINCS += -I$(OUTPUT_DIRNAME)

ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LCDEFS += -DSET_KERNEL_PINMUX
endif
EXPORT_FILES := $(MODULE_NAME).export

C_FILES :=
C_FILES += nvodm_imager.c
C_FILES += nvodm_imager_i2c.c
C_FILES += sensors/nvodm_imager_devicelist.c
C_FILES += sensors/nvodm_imager_util.c
# sensors
C_FILES += sensors/nvodm_sensor_mi5130.c
# for testing
C_FILES += sensors/nvodm_sensor_null.c
C_FILES += sensors/nvodm_sensor_host.c

NVODM_IMAGER_CONFIGS :=
NVODM_IMAGER_CONFIGS += configs/camera_host.isp
NVODM_IMAGER_CONFIGS += configs/camera_micron_5130.isp

NVODM_IMAGER_CONFIG_TARGETS := $(patsubst %.isp,$(OUTPUT_DIRNAME)/%_calibration.h,$(notdir $(NVODM_IMAGER_CONFIGS)))


OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(NVODMLIB_INSTALL_TARGET)

$(TARGETS): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
ifeq ($(NV_DEF_USE_DLL),1)
  $(TARGETS): $(NVLIB_NVOS)
  $(TARGETS): $(NVLIB_NVODM_SERVICES)
  $(TARGETS): $(NVLIB_NVODM_QUERY)
  $(TARGETS): $(NVLIB_NVRM)
  $(TARGETS): $(NVLIB_DDK_FUSE)
endif

$(OUTPUT_DIRNAME)/nvodm_sensor_host$(OBJ_SUFFIX): $(OUTPUT_DIRNAME)/camera_host_calibration.h
$(OUTPUT_DIRNAME)/nvodm_sensor_null$(OBJ_SUFFIX): $(OUTPUT_DIRNAME)/camera_host_calibration.h

$(OUTPUT_DIRNAME)/nvodm_sensor_mi5130$(OBJ_SUFFIX): $(OUTPUT_DIRNAME)/camera_micron_5130_calibration.h

$(OUTPUT_DIRNAME)/%_calibration.h: configs/%.isp $(OUTPUT_DIRNAME)/dummy.txt
	$(RM) $@ $@.tmp
	echo "// GENERATED FILE - DO NOT MODIFY!" > $@.tmp
	echo "static const char *sensorCalibrationData = " >> $@.tmp
	$(SED) -e 's#"#\\"#g' -e 's#\(.*\)#"\1\\n"#' < $< >> $@.tmp
	echo ";" >> $@.tmp
	$(MV) $@.tmp $@

# For Arch supplied calibration cfg files, and for nvodm adaptations
# in which a header file is needed, you can use this make rule.
# Example use:
#   make convert ISP_CFG_FILE=name_of_file ISP_H_FILE=path/name
# The ISP_H_FILE is optional, if unspecified, a default name is used.
ISP_CFG_FILE ?= "error"
ISP_H_FILE ?= sensor_bayer_camera_config.h
CFG_SIGN = $(PERL) ${TOPDIR}/utils/isp/scripts/cfgsign.pl

sign:
	${CFG_SIGN} ${ISP_CFG_FILE}

verify:
	${CFG_SIGN} -v ${ISP_CFG_FILE}

convert: ${ISP_CFG_FILE}
	${CFG_SIGN} ${ISP_CFG_FILE} -o ${ISP_H_FILE}

include $(TOPDIR)/../core-private/make/Makefile.rules
