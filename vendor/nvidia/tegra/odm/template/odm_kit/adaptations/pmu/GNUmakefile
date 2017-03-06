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


MODULE_NAME := libnvodm_pmu

ifeq ($(TARGET_BOARD),p1852)
LCDEFS  += -DNV_TARGET_P1852
endif

ifeq ($(TARGET_BOARD),cardhu)
LCDEFS  += -DNV_TARGET_CARDHU
endif

LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I..
LCINCS += -I.

C_FILES :=
C_FILES += pmu_hal.c
C_FILES += pmu_i2c/nvodm_pmu_i2c.c
C_FILES += tps6586x/nvodm_pmu_tps6586x_i2c.c
C_FILES += tps6586x/nvodm_pmu_tps6586x_interrupt.c
C_FILES += tps6586x/nvodm_pmu_tps6586x_batterycharger.c
C_FILES += tps6586x/nvodm_pmu_tps6586x_adc.c
C_FILES += tps6586x/nvodm_pmu_tps6586x.c
C_FILES += tps6586x/nvodm_pmu_tps6586x_rtc.c
C_FILES += tps6591x/nvodm_pmu_tps6591x_pmu_driver.c
C_FILES += tps6236x/nvodm_pmu_tps6236x_pmu_driver.c
C_FILES += tps6238x/nvodm_pmu_tps6238x_pmu_driver.c
C_FILES += max897x/nvodm_pmu_max897x_pmu_driver.c
C_FILES += max8907b/max8907b.c
C_FILES += max8907b/max8907b_adc.c
C_FILES += max8907b/max8907b_batterycharger.c
C_FILES += max8907b/max8907b_i2c.c
C_FILES += max8907b/max8907b_interrupt.c
C_FILES += max8907b/max8907b_rtc.c
C_FILES += max8907b/fan5355_buck_i2c.c
C_FILES += max8907b/tca6416_expander_i2c.c
C_FILES += max8907b/mic2826_i2c.c
C_FILES += max8907b/ad5258_dpm.c
C_FILES += tps8003x/tps8003x_core_driver.c
C_FILES += tps8003x/nvodm_pmu_tps8003x_pmu_driver.c
C_FILES += tps8003x/nvodm_pmu_tps8003x_adc_driver.c
C_FILES += tps8003x/nvodm_pmu_tps8003x_battery_charging_driver.c
C_FILES += ricoh583/nvodm_pmu_ricoh583_pmu_driver.c
C_FILES += tca6416/nvodm_pmu_tca6416.c
C_FILES += apgpio/nvodm_pmu_apgpio.c
C_FILES += max77663/nvodm_pmu_max77663_pmu_driver.c
C_FILES += boards/cardhu/nvodm_pmu_cardhu_rail.c
C_FILES += boards/cardhu/nvodm_pmu_cardhu_dummy.c
C_FILES += boards/cardhu/odm_pmu_cardhu_tps6591x_rail.c
C_FILES += boards/cardhu/odm_pmu_cardhu_tps6236x_rail.c
C_FILES += boards/cardhu/odm_pmu_cardhu_apgpio_rail.c
C_FILES += boards/cardhu/odm_pmu_cardhu_ricoh583_rail.c
C_FILES += boards/cardhu/odm_pmu_cardhu_max77663_rail.c
C_FILES += boards/kai/nvodm_pmu_kai_rail.c
C_FILES += boards/kai/nvodm_pmu_kai_dummy.c
C_FILES += boards/kai/odm_pmu_kai_apgpio_rail.c
C_FILES += boards/kai/odm_pmu_kai_max77663_rail.c
C_FILES += boards/enterprise/nvodm_pmu_enterprise_rail.c
C_FILES += boards/enterprise/nvodm_pmu_enterprise_dummy.c
C_FILES += boards/enterprise/odm_pmu_enterprise_tps8003x_rail.c
C_FILES += boards/enterprise/odm_pmu_enterprise_apgpio_rail.c
C_FILES += boards/enterprise/nvodm_pmu_enterprise_battery.c
C_FILES += boards/p1852/nvodm_pmu_p1852_rail.c
C_FILES += boards/p1852/odm_pmu_p1852_tps6591x_rail.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(LIB) $(NVODMLIB_INSTALL_TARGET)

$(LIB): $(OBJS)

include $(TOPDIR)/../core-private/make/Makefile.rules
