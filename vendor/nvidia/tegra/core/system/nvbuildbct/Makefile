###############################################################################
#
# Copyright (c) 2012, NVIDIA CORPORATION.  All Rights Reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#
###############################################################################
HOST_BUILD := 1

TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs

LIB := $(OUTDIR)/libnvbuildbct$(LIB_SUFFIX)
LIB_HOST_STATIC := $(OUTDIR)/libnvbuildbct_host_static$(LIB_SUFFIX)

TO_BUILD := $(LIB) $(LIB_HOST_STATIC)
TO_INSTALL := $(LIB) $(LIB_HOST_STATIC)

# Build the host static lib only on the host
ifeq ($(HOST_BUILD),0)
  ifneq ($(OUTDIR),$(HOST_OUTDIR))
    TO_BUILD := $(LIB)
  endif
endif

NVLIB_HWINC_INCLUDES += -I$(TEGRA_TOP)/core/drivers/hwinc

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)

C_FILES :=
C_FILES += ap20/ap20_parse.c
C_FILES += ap20/ap20_set.c
C_FILES += ap20/ap20_data_layout.c
C_FILES += ap20/ap20_buildbctinterface.c

C_FILES += t30/t30_parse.c
C_FILES += t30/t30_set.c
C_FILES += t30/t30_data_layout.c
C_FILES += t30/t30_buildbctinterface.c

C_FILES += nvbuildbct_util.c
C_FILES += nvbuildbct.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(TO_BUILD)
	$(DO_HOST_BUILD_TOO)
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)

$(LIB): $(OBJS)
$(LIB_HOST_STATIC): $(OBJS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
