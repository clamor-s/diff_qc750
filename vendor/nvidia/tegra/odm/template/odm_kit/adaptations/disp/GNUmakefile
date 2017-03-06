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
MODULE_SOURCES += display_hal.c
MODULE_SOURCES += panel_null.c
MODULE_SOURCES += amoled_amaf002.c
MODULE_SOURCES += panel_sharp_wvga.c
MODULE_SOURCES += bl_pcf50626.c
MODULE_SOURCES += bl_ap20.c
MODULE_SOURCES += panel_samsungsdi.c
MODULE_SOURCES += panel_tpo_wvga.c
MODULE_SOURCES += panel_sharp_dsi.c
MODULE_SOURCES += panel_lvds_wsga.c
MODULE_SOURCES += panel_sharp_wvgap.c
MODULE_SOURCES += panel_hdmi.c

include $(TOPDIR)/../core/drivers/nvodm/makefiles/Makefile.odm_disp

