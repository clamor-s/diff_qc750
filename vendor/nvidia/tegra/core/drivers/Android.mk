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

# the build of nvboot/lib/host is temporarily here as the nvboot
# tree is p4 protected so that I can't submit the Android.mk file.

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE := libnvboothost

LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/drivers/nvboot/include
LOCAL_C_INCLUDES += $(TEGRA_TOP)/core/utils/aes_ref

LOCAL_SRC_FILES += nvboot/lib/host/nvbh_rcm.c
LOCAL_SRC_FILES += nvboot/lib/host/nvbh_crypto.c

include $(NVIDIA_HOST_STATIC_LIBRARY)

# end of libnvboothost

include $(addprefix $(LOCAL_PATH)/, $(addsuffix /Android.mk, \
	nvrm \
	nvodm \
	nvddk \
	nvtestio \
	libnvdc \
	nvavpgpio \
	nvavp \
	nvpinmux \
))
