#
# Copyright (c) 2008-2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


SUBDIRS :=

ifeq ($(TARGET_OS),rvds)
ifeq ($(TARGET_CPU),armv4)
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/sdram
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/ahb
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/aes
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/clocks
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/fuse
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/reset
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/util
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/usbf
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/pads
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/nand
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/snor
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/spi_flash
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/sdmmc
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/irom_patch
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/hash
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/ap20/sdram
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/ap20/aes
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/ap20/clocks
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/ap20/fuse
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/ap20/reset
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/ap20/util
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/ap20/usbf
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/ap20/pads
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/ap20/nand
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/ap20/snor
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/ap20/spi_flash
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/ap20/sdmmc
SUBDIRS += $(TEGRA_TOP)/core-private/drivers/nvboot/ap20/hash
SUBDIRS += $(TEGRA_TOP)/core-private/utils/aes_keysched_lock
SUBDIRS += $(TEGRA_TOP)/core/utils/aes_keysched_lock
SUBDIRS += $(TEGRA_TOP)/core/include
SUBDIRS += $(TEGRA_TOP)/core/system/nvbct
SCATTER_SCRIPT := miniloader.ldscript

SUBDIRS += $(TEGRA_TOP)/core/utils/nvos
SUBDIRS += $(TEGRA_TOP)/core/utils/aes_ref
SUBDIRS += $(TEGRA_TOP)/core/system/nv3p
SUBDIRS += $(TEGRA_TOP)/core/utils/nvusbhost/libnvusbhost
SUBDIRS += $(TEGRA_TOP)/core/drivers/nvboot/lib/host
SUBDIRS += $(TEGRA_TOP)/core/utils/nvapputil
SUBDIRS += $(TEGRA_TOP)/core/system/nvdioconverter
SUBDIRS += $(TEGRA_TOP)/core/utils/nvtestresults
SUBDIRS += $(TEGRA_TOP)/core/drivers/nvtestio
SUBDIRS += $(TEGRA_TOP)/core/system/nvbuildbct
SUBDIRS += $(TEGRA_TOP)/core/drivers/nvddk/fuses/read
SUBDIRS += $(TEGRA_TOP)/core/drivers/nvddk/fuses/read/host
endif
endif

SUBDIRS += lib

ifeq ($(TARGET_OS),rvds)
ifeq ($(TARGET_CPU),armv4)
SUBDIRS += nvml
SUBDIRS += app
endif
endif

$(warning export HOST_OS = $(HOST_OS))
$(warning export BUILD_OS = $(BUILD_OS))
$(warning export TARGET_OS = $(TARGET_OS))
$(warning export TARGET_CPU = $(TARGET_CPU))
$(warning export TARGET_GPU = $(TARGET_GPU))

include $(TEGRA_TOP)/core-private/make/Makefile.rules
