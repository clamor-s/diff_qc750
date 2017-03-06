#
# Copyright (c) 2011-2012, NVIDIA Corporation.  All rights reserved.
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

MODULE_SOURCES += panels.c
MODULE_SOURCES += panel_sony_vga.c
MODULE_SOURCES += panel_samsung_test_dsi.c
MODULE_SOURCES += panel_sharp_test_dsi.c
MODULE_SOURCES += panel_lvds_wsga.c

include $(TOPDIR)/drivers/nvodm/makefiles/Makefile.odm_disp

