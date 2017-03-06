#
# Copyright (c) 2008 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

#
# Makefile - make omx_dfs_log program for wince armv6 device
#HOST_BUILD := 1
TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

include $(TEGRA_TOP)/multimedia-partner/openmax/Makefile.omxdefs
include $(TEGRA_TOP)/tests/make/Makefile.nvtestdefs

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/include/openmax/ilclient
LCINCS += -I$(TEGRA_TOP)/multimedia-partner/openmax/include/openmax/il
C_FILES := 
C_FILES += omxplayer2.c
ONE_FILE_EXECUTABLES := $(OUTDIR)/omxplayer2$(EXE_SUFFIX)

default: $(ONE_FILE_EXECUTABLES)
	$(NV_INSTALL) -l $(ONE_FILE_EXECUTABLES) $(INSTALL_TARGET)

OS_LIBS :=
OS_LIBS += $(NVLIB_NVOS)
OS_LIBS += $(NVLIB_WINSYS)
OS_LIBS += $(NVLIB_OGLES2)
OS_LIBS += $(NVLIB_EGL)

$(ONE_FILE_EXECUTABLES): $(OS_LIBS)
$(ONE_FILE_EXECUTABLES): $(TEGRA_TOP)/multimedia-partner/openmax/ilclient/$(OUTDIR)/libnvomxilclient$(DLL_SUFFIX)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
