#
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#

TOPDIR := ../../../../../core
include $(TOPDIR)/../core-private/make/Makefile.defs

MODULE_SOURCES :=
MODULE_STATIC_LIBS :=
MODULE_DYNAMIC_LIBS :=
MODULE_INCLUDE_DIRS :=
SECURE_MODULE_SOURCES :=

LCINCS += -I$(TOPDIR)/../customers/nvidia-partner/template
LCINCS += -I$(TOPDIR)/../customers/nvidia-partner/template/odm_kit/adaptations

ifeq ($(TARGET_OS),rvds)
LCDEFS += -DDEFAULT_CONSOLE
endif

ifeq ($(TARGET_OS_SUBTYPE),aos)
  OPTIMIZER += -march=armv4t
endif

LCDEFS += -DFPGA_BOARD

MODULE_SOURCES += nvodm_query.c
MODULE_SOURCES += nvodm_query_discovery.c
MODULE_SOURCES += nvodm_query_nand.c
MODULE_SOURCES += nvodm_query_gpio.c
MODULE_SOURCES += nvodm_query_pinmux.c
MODULE_SOURCES += nvodm_query_kbc.c
MODULE_SOURCES += nvodm_platform_init.c
MODULE_SOURCES += secure/nvodm_query_secure.c
MODULE_SOURCES += nvodm_query_common.c
SECURE_MODULE_SOURCES += secure/nvodm_query_secure.c

include $(TOPDIR)/drivers/nvodm/makefiles/Makefile.odm_query
