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


MODULE_SOURCES :=
MODULE_STATIC_LIBS :=
MODULE_DYNAMIC_LIBS :=

ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LCDEFS += -DSET_KERNEL_PINMUX
endif
MODULE_SOURCES += audiocodec_hal.c
MODULE_SOURCES += nvodm_codec_wolfson8753.c
MODULE_SOURCES += nvodm_codec_wolfson8903.c
MODULE_SOURCES += nvodm_codec_wolfson8961.c
MODULE_SOURCES += nvodm_codec_wolfson8994.c
MODULE_SOURCES += nvodm_codec_maxim9888.c
MODULE_SOURCES += nvodm_codec_null.c

include $(TOPDIR)/../core/drivers/nvodm/makefiles/Makefile.odm_audio
