#
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

LOCAL_PATH := $(call my-dir)
include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvodm_imager
LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS += -DBUILD_FOR_AOS=0

ifneq (,$(filter $(TARGET_PRODUCT),harmony ventana whistler aruba2 cardhu enterprise kai p1852 e1853 nabi2 cm9000 nabi2_3d nabi2_xd qc750 n710 itq700 nabi_2s n1010 n750 wikipad ns_14t004 birch))

ifeq ($(TARGET_PRODUCT),harmony)
LOCAL_CFLAGS += -DBUILD_FOR_HARMONY=1
endif

ifeq ($(TARGET_PRODUCT),nabi2)
LOCAL_CFLAGS += -DBUILD_FOR_GC0308=1
LOCAL_CFLAGS += -DBUILD_FOR_HM2057=1
endif

ifeq ($(TARGET_PRODUCT),qc750)
LOCAL_CFLAGS += -DBUILD_FOR_GC0308=1
LOCAL_CFLAGS += -DBUILD_FOR_HM2057=0
LOCAL_CFLAGS += -DBUILD_FOR_QC750=1
endif

ifeq ($(TARGET_PRODUCT),birch)
LOCAL_CFLAGS += -DBUILD_FOR_GC0308=1
LOCAL_CFLAGS += -DBUILD_FOR_HM2057=0
LOCAL_CFLAGS += -DBUILD_FOR_N750=1
endif

ifeq ($(TARGET_PRODUCT),wikipad)
LOCAL_CFLAGS += -DBUILD_FOR_GC0308=1
LOCAL_CFLAGS += -DBUILD_FOR_HM2057=1
LOCAL_CFLAGS += -DBUILD_FOR_QC710=1
endif

ifeq ($(TARGET_PRODUCT),n710)
LOCAL_CFLAGS += -DBUILD_FOR_GC0308=1
LOCAL_CFLAGS += -DBUILD_FOR_HM2057=1
endif

ifeq ($(TARGET_PRODUCT),itq700)
LOCAL_CFLAGS += -DBUILD_FOR_GC0308=1
LOCAL_CFLAGS += -DBUILD_FOR_HM2057=1
endif

ifeq ($(TARGET_PRODUCT),n1010)
LOCAL_CFLAGS += -DBUILD_FOR_GC0308=1
LOCAL_CFLAGS += -DBUILD_FOR_HM2057=1
LOCAL_CFLAGS += -DBUILD_FOR_N1010=1
endif

ifeq ($(TARGET_PRODUCT),ns_14t004)
LOCAL_CFLAGS += -DBUILD_FOR_GC0308=1
LOCAL_CFLAGS += -DBUILD_FOR_HM2057=1
LOCAL_CFLAGS += -DBUILD_FOR_NS_14T004=1
endif

ifeq ($(TARGET_PRODUCT),n750)
LOCAL_CFLAGS += -DBUILD_FOR_GC0308=1
LOCAL_CFLAGS += -DBUILD_FOR_HM2057=0
LOCAL_CFLAGS += -DBUILD_FOR_N750=1
endif

ifeq ($(TARGET_PRODUCT),nabi_2s)
LOCAL_CFLAGS += -DBUILD_FOR_GC0308=1
LOCAL_CFLAGS += -DBUILD_FOR_HM2057=1
endif

ifeq ($(TARGET_PRODUCT),nabi2_3d)
LOCAL_CFLAGS += -DBUILD_FOR_GC0308=1
LOCAL_CFLAGS += -DBUILD_FOR_HM2057=1
endif

ifeq ($(TARGET_PRODUCT),nabi2_xd)
LOCAL_CFLAGS += -DBUILD_FOR_GC0308=1
LOCAL_CFLAGS += -DBUILD_FOR_HM2057=1
LOCAL_CFLAGS += -DBUILD_FOR_NABI2_XD=1
endif

ifeq ($(TARGET_PRODUCT),leadtek_8218)
LOCAL_CFLAGS += -DBUILD_FOR_MT9P111=1
endif

ifeq ($(TARGET_PRODUCT),ventana)
LOCAL_CFLAGS += -DBUILD_FOR_VENTANA=1
endif
ifeq ($(TARGET_PRODUCT),whistler)
LOCAL_CFLAGS += -DBUILD_FOR_WHISTLER=1
endif
ifeq ($(TARGET_PRODUCT),cardhu)
LOCAL_CFLAGS += -DBUILD_FOR_CARDHU=1
endif
ifeq ($(TARGET_PRODUCT),kai)
LOCAL_CFLAGS += -DBUILD_FOR_KAI=1
endif
ifeq ($(TARGET_PRODUCT),p1852)
LOCAL_CFLAGS += -DBUILD_FOR_P1852=1
endif
ifeq ($(TARGET_PRODUCT),e1853)
LOCAL_CFLAGS += -DBUILD_FOR_E1853=1
endif

ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LOCAL_CFLAGS += -DSET_KERNEL_PINMUX
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/configs
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../customers/nvidia-partner/template/odm_kit/query/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../odm/template/odm_kit/query/include

LOCAL_SRC_FILES += imager_hal.c
LOCAL_SRC_FILES += imager_util.c
LOCAL_SRC_FILES += sensor_yuv_soc380.c
LOCAL_SRC_FILES += sensor_yuv_hm2057.c
LOCAL_SRC_FILES += sensor_yuv_gc0308.c
LOCAL_SRC_FILES += sensor_yuv_t8ev5.c
LOCAL_SRC_FILES += sensor_bayer_ar0832.c
LOCAL_SRC_FILES += sensor_bayer_ov5630.c
LOCAL_SRC_FILES += sensor_yuv_ov5640.c
LOCAL_SRC_FILES += sensor_bayer_ov5650.c
LOCAL_SRC_FILES += sensor_bayer_ov9726.c
LOCAL_SRC_FILES += sensor_bayer_ov14810.c
LOCAL_SRC_FILES += sensor_bayer_ov2710.c
LOCAL_SRC_FILES += sensor_null.c
LOCAL_SRC_FILES += sensor_host.c
LOCAL_SRC_FILES += focuser_t8ev5.c
LOCAL_SRC_FILES += focuser_sh532u.c
LOCAL_SRC_FILES += focuser_ad5820.c
LOCAL_SRC_FILES += focuser_ar0832.c
LOCAL_SRC_FILES += focuser_nvc.c
LOCAL_SRC_FILES += flash_ltc3216.c
LOCAL_SRC_FILES += flash_ssl3250a.c
LOCAL_SRC_FILES += flash_tps61050.c
LOCAL_SRC_FILES += torch_nvc.c

LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libnvrm
LOCAL_SHARED_LIBRARIES += libnvrm_graphics
LOCAL_SHARED_LIBRARIES += libnvos
LOCAL_SHARED_LIBRARIES += libnvodm_query
LOCAL_STATIC_LIBRARIES += libnvodm_services

#TODO: Remove following lines before include statement and fix the source giving warnings/errors
LOCAL_NVIDIA_NO_WARNINGS_AS_ERRORS := 1
LOCAL_CFLAGS += -Werror=unused-variable

include $(NVIDIA_SHARED_LIBRARY)
endif
