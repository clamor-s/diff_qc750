#
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#
LOCAL_PATH := $(call my-dir)

LOCAL_COMMON_C_INCLUDES := $(TEGRA_TOP)/customers/nvidia-partner/template/odm_kit/adaptations/pmu
LOCAL_COMMON_C_INCLUDES += $(TEGRA_TOP)/customers/nvidia-partner/template/odm_kit/adaptations/pmu/tps6586x
LOCAL_COMMON_C_INCLUDES += $(TEGRA_TOP)/customers/nvidia-partner/template/odm_kit/adaptations/pmu/max8907b

LOCAL_COMMON_SRC_FILES :=  \
        pmu_hal.c \
        pmu_i2c/nvodm_pmu_i2c.c \
        tps6586x/nvodm_pmu_tps6586x_i2c.c \
        tps6586x/nvodm_pmu_tps6586x_interrupt.c \
        tps6586x/nvodm_pmu_tps6586x_batterycharger.c \
        tps6586x/nvodm_pmu_tps6586x_adc.c \
        tps6586x/nvodm_pmu_tps6586x.c \
        tps6586x/nvodm_pmu_tps6586x_rtc.c \
        tps6591x/nvodm_pmu_tps6591x_pmu_driver.c \
        tps6236x/nvodm_pmu_tps6236x_pmu_driver.c \
        tps6238x/nvodm_pmu_tps6238x_pmu_driver.c \
        max897x/nvodm_pmu_max897x_pmu_driver.c \
        tps8003x/tps8003x_core_driver.c \
        tps8003x/nvodm_pmu_tps8003x_pmu_driver.c \
        tps8003x/nvodm_pmu_tps8003x_adc_driver.c \
        tps8003x/nvodm_pmu_tps8003x_battery_charging_driver.c \
        ricoh583/nvodm_pmu_ricoh583_pmu_driver.c \
        max8907b/max8907b.c \
        max8907b/max8907b_adc.c \
        max8907b/max8907b_batterycharger.c \
        max8907b/max8907b_i2c.c \
        max8907b/max8907b_interrupt.c \
        max8907b/max8907b_rtc.c \
        max8907b/fan5355_buck_i2c.c \
        max8907b/tca6416_expander_i2c.c \
        max8907b/mic2826_i2c.c \
        max8907b/ad5258_dpm.c \
        tca6416/nvodm_pmu_tca6416.c \
        apgpio/nvodm_pmu_apgpio.c \
        max77663/nvodm_pmu_max77663_pmu_driver.c \
        max17048/nvodm_pmu_max17048_fuel_gauge_driver.c \
        smb349/nvodm_pmu_smb349_battery_charger_driver.c \
        boards/cardhu/nvodm_pmu_cardhu_rail.c \
        boards/cardhu/nvodm_pmu_cardhu_dummy.c \
        boards/cardhu/odm_pmu_cardhu_tps6591x_rail.c \
        boards/cardhu/odm_pmu_cardhu_tps6236x_rail.c \
        boards/cardhu/odm_pmu_cardhu_apgpio_rail.c \
        boards/cardhu/odm_pmu_cardhu_ricoh583_rail.c \
        boards/cardhu/odm_pmu_cardhu_max77663_rail.c \
        boards/kai/nvodm_pmu_kai_rail.c \
        boards/kai/nvodm_pmu_kai_dummy.c \
        boards/kai/nvodm_pmu_kai_battery.c \
        boards/kai/odm_pmu_kai_apgpio_rail.c \
        boards/kai/odm_pmu_kai_max77663_rail.c \
        boards/enterprise/nvodm_pmu_enterprise_rail.c \
        boards/enterprise/nvodm_pmu_enterprise_dummy.c \
        boards/enterprise/odm_pmu_enterprise_tps8003x_rail.c \
        boards/enterprise/odm_pmu_enterprise_apgpio_rail.c \
        boards/enterprise/nvodm_pmu_enterprise_battery.c \

include $(NVIDIA_DEFAULTS)
LOCAL_MODULE := libnvodm_pmu
LOCAL_NVIDIA_NO_COVERAGE := true
LOCAL_C_INCLUDES += $(LOCAL_C_COMMON_INCLUDES)
LOCAL_SRC_FILES +=  $(LOCAL_COMMON_SRC_FILES)
ifeq ($(TARGET_DEVICE),ventana)
LOCAL_CFLAGS += -DNV_TARGET_VENTANA
endif
ifeq ($(TARGET_DEVICE),whistler)
LOCAL_CFLAGS += -DNV_TARGET_WHISTLER
endif
ifeq ($(TARGET_DEVICE),cardhu)
LOCAL_CFLAGS += -DNV_TARGET_CARDHU
endif
ifeq ($(TARGET_DEVICE),kai)
LOCAL_CFLAGS += -DNV_TARGET_KAI
endif
ifeq ($(TARGET_DEVICE),enterprise)
LOCAL_CFLAGS += -DNV_TARGET_ENTERPRISE
endif

ifeq ($(TARGET_DEVICE),nabi2_xd)
LOCAL_CFLAGS += -DNV_TARGET_KAI
endif

ifeq ($(TARGET_DEVICE),nabi_2s)
LOCAL_CFLAGS += -DNV_TARGET_KAI
endif

ifeq ($(TARGET_DEVICE),mt799)
LOCAL_CFLAGS += -DNV_TARGET_KAI
endif
ifeq ($(TARGET_DEVICE),nabi2_3d)
LOCAL_CFLAGS += -DNV_TARGET_KAI
endif
ifeq ($(TARGET_DEVICE),cm9000)
LOCAL_CFLAGS += -DNV_TARGET_KAI
endif

ifeq ($(TARGET_DEVICE),qc750)
LOCAL_CFLAGS += -DNV_TARGET_KAI
endif

ifeq ($(TARGET_DEVICE),n710)
LOCAL_CFLAGS += -DNV_TARGET_KAI
endif

ifeq ($(TARGET_DEVICE),mm3201)
LOCAL_CFLAGS += -DNV_TARGET_KAI
endif
ifeq ($(TARGET_DEVICE),itq700)
LOCAL_CFLAGS += -DNV_TARGET_KAI
endif

ifeq ($(TARGET_DEVICE),n1010)
LOCAL_CFLAGS += -DNV_TARGET_KAI
endif

ifeq ($(TARGET_DEVICE),birch)
LOCAL_CFLAGS += -DNV_TARGET_KAI
endif
ifeq ($(TARGET_DEVICE),n750)
LOCAL_CFLAGS += -DNV_TARGET_KAI
endif

ifeq ($(TARGET_DEVICE),wikipad)
LOCAL_CFLAGS += -DNV_TARGET_KAI
endif

ifeq ($(TARGET_DEVICE),ns_14t004)
LOCAL_CFLAGS += -DNV_TARGET_KAI
endif
#TODO: Remove following lines before include statement and fix the source giving warnings/errors
LOCAL_NVIDIA_RM_WARNING_FLAGS := -Wundef
LOCAL_CFLAGS += -Wno-missing-field-initializers
LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
include $(NVIDIA_STATIC_LIBRARY)

include $(NVIDIA_DEFAULTS)
LOCAL_MODULE := libnvodm_pmu_avp
LOCAL_CFLAGS += -Os -ggdb0
LOCAL_C_INCLUDES += $(LOCAL_C_COMMON_INCLUDES)

LOCAL_SRC_FILES := pmu_hal.c
LOCAL_SRC_FILES += pmu_i2c/nvodm_pmu_i2c.c

ifeq ($(TARGET_DEVICE),ventana)
LOCAL_SRC_FILES += tps6586x/nvodm_pmu_tps6586x_i2c.c
LOCAL_SRC_FILES += tps6586x/nvodm_pmu_tps6586x_interrupt.c
LOCAL_SRC_FILES += tps6586x/nvodm_pmu_tps6586x_batterycharger.c
LOCAL_SRC_FILES += tps6586x/nvodm_pmu_tps6586x_adc.c
LOCAL_SRC_FILES += tps6586x/nvodm_pmu_tps6586x.c
LOCAL_SRC_FILES += tps6586x/nvodm_pmu_tps6586x_rtc.c
LOCAL_CFLAGS += -DNV_TARGET_VENTANA
endif
ifeq ($(TARGET_DEVICE),whistler)
LOCAL_SRC_FILES += max8907b/max8907b.c
LOCAL_SRC_FILES += max8907b/max8907b_adc.c
LOCAL_SRC_FILES += max8907b/max8907b_batterycharger.c
LOCAL_SRC_FILES += max8907b/max8907b_i2c.c
LOCAL_SRC_FILES += max8907b/max8907b_interrupt.c
LOCAL_SRC_FILES += max8907b/max8907b_rtc.c
LOCAL_SRC_FILES += max8907b/fan5355_buck_i2c.c
LOCAL_SRC_FILES += max8907b/tca6416_expander_i2c.c
LOCAL_SRC_FILES += max8907b/mic2826_i2c.c
LOCAL_SRC_FILES += max8907b/ad5258_dpm.c
LOCAL_CFLAGS += -DNV_TARGET_WHISTLER
endif
ifeq ($(TARGET_DEVICE),cardhu)
LOCAL_SRC_FILES += boards/cardhu/nvodm_pmu_cardhu_rail.c
LOCAL_SRC_FILES += boards/cardhu/nvodm_pmu_cardhu_dummy.c
LOCAL_SRC_FILES += boards/cardhu/odm_pmu_cardhu_tps6591x_rail.c
LOCAL_SRC_FILES += boards/cardhu/odm_pmu_cardhu_tps6236x_rail.c
LOCAL_SRC_FILES += boards/cardhu/odm_pmu_cardhu_apgpio_rail.c
LOCAL_SRC_FILES += boards/cardhu/odm_pmu_cardhu_ricoh583_rail.c
LOCAL_SRC_FILES += boards/cardhu/odm_pmu_cardhu_max77663_rail.c
LOCAL_SRC_FILES += max897x/nvodm_pmu_max897x_pmu_driver.c
LOCAL_SRC_FILES += tps6591x/nvodm_pmu_tps6591x_pmu_driver.c
LOCAL_SRC_FILES += tps6236x/nvodm_pmu_tps6236x_pmu_driver.c
LOCAL_SRC_FILES += ricoh583/nvodm_pmu_ricoh583_pmu_driver.c
LOCAL_SRC_FILES += max77663/nvodm_pmu_max77663_pmu_driver.c
LOCAL_SRC_FILES += apgpio/nvodm_pmu_apgpio.c
LOCAL_CFLAGS += -DNV_TARGET_CARDHU
endif


ifneq (,$(filter $(TARGET_PRODUCT),nabi2 kai cm9000 nabi2_3d nabi2_xd nabi_2s qc750 n710 itq700 n1010 n750 wikipad birch itq701 mm3201 ns_14t004))
LOCAL_SRC_FILES += boards/kai/nvodm_pmu_kai_rail.c
LOCAL_SRC_FILES += boards/kai/nvodm_pmu_kai_dummy.c
LOCAL_SRC_FILES += boards/kai/nvodm_pmu_kai_battery.c
LOCAL_SRC_FILES += boards/kai/odm_pmu_kai_apgpio_rail.c
LOCAL_SRC_FILES += boards/kai/odm_pmu_kai_max77663_rail.c
LOCAL_SRC_FILES += max77663/nvodm_pmu_max77663_pmu_driver.c
LOCAL_SRC_FILES += max17048/nvodm_pmu_max17048_fuel_gauge_driver.c
LOCAL_SRC_FILES += smb349/nvodm_pmu_smb349_battery_charger_driver.c
LOCAL_SRC_FILES += apgpio/nvodm_pmu_apgpio.c
LOCAL_CFLAGS += -DNV_TARGET_KAI
endif
ifeq ($(TARGET_DEVICE),enterprise)
LOCAL_SRC_FILES += boards/enterprise/nvodm_pmu_enterprise_rail.c
LOCAL_SRC_FILES += boards/enterprise/nvodm_pmu_enterprise_dummy.c
LOCAL_SRC_FILES += boards/enterprise/odm_pmu_enterprise_tps8003x_rail.c
LOCAL_SRC_FILES += boards/enterprise/odm_pmu_enterprise_apgpio_rail.c
LOCAL_SRC_FILES += boards/enterprise/nvodm_pmu_enterprise_battery.c
LOCAL_SRC_FILES += apgpio/nvodm_pmu_apgpio.c
LOCAL_SRC_FILES += tps8003x/tps8003x_core_driver.c
LOCAL_SRC_FILES += tps8003x/nvodm_pmu_tps8003x_pmu_driver.c
LOCAL_SRC_FILES += tps8003x/nvodm_pmu_tps8003x_adc_driver.c
LOCAL_SRC_FILES += tps8003x/nvodm_pmu_tps8003x_battery_charging_driver.c
LOCAL_CFLAGS += -DNV_TARGET_ENTERPRISE
endif

include $(NVIDIA_STATIC_AVP_LIBRARY)
