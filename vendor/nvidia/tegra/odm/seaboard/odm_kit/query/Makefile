#
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#

TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/../core-private/make/Makefile.defs


MODULE_STATIC_LIBS :=
MODULE_DYNAMIC_LIBS :=
MODULES_SOURCES :=
MODULE_INCLUDE_DIRS :=

ifeq ($(TARGET_OS_SUBTYPE),aos)
  OPTIMIZER += -march=armv4t
endif

LCDEFS += -DLPM_BATTERY_CHARGING=1
MODULE_SOURCES +=nvodm_query.c
MODULE_SOURCES +=nvodm_query_discovery.c
MODULE_SOURCES +=nvodm_query_nand.c
MODULE_SOURCES +=nvodm_query_gpio.c
MODULE_SOURCES +=nvodm_query_pinmux.c
MODULE_SOURCES +=nvodm_query_kbc.c
MODULE_SOURCES +=secure/nvodm_query_secure.c

#SECURE_MODULE_SOURCES :=
#SECURE_MODULE_SOURCES += secure/nvodm_query_secure.c

MODULE_INCLUDE_DIRS += -I$(TEGRA_TOP)/customers/nvidia-partner/template/odm_kit/adaptations


include $(TOPDIR)/../core/drivers/nvodm/makefiles/Makefile.odm_query
