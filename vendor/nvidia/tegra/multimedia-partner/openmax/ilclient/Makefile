# Copyright (c) 2009-2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an
# express license agreement from NVIDIA Corporation is strictly prohibited.

TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

include ../Makefile.omxdefs

MODULE_NAME := libnvomxilclient

LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))

#ifeq ($(BUILD_FLAVOR),debug)
#  LCDEFS += -DNV_ENABLE_DEBUG_PRINTS=1
#endif
LCDEFS += -DNV_IS_AVP=$(IS_AVP)
ifeq ($(NV_LOGGER_ENABLED),1)
  LCDEFS += -DNV_LOGGER_ENABLED=1
endif

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/include/openmax/ilclient

C_FILES :=
C_FILES += nvxrecordgraph.c
C_FILES += nvxtunneledrecordgraph.c
C_FILES += nvxframework.c
C_FILES += nvxgraph.c
C_FILES += nvxplayergraph.c

EXPORT_FILES := $(MODULE_NAME).export

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

$(TARGETS): $(OBJS)
$(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
ifeq ($(NV_DEF_USE_DLL),1)
  $(TARGETS): $(NVLIB_NVOS)
  $(TARGETS): $(NVLIB_FXMATH)
endif

RELEASE_PACKAGE_TARGETS := $(TARGETS)
RELEASE_PACKAGE_FILES := $(C_FILES)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
