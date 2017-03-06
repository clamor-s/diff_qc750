# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.

TOPDIR := ../../../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/include/nvposix
LCINCS += -I$(TEGRA_TOP)/core/utils/nvos/include
LCINCS += -I$(TEGRA_TOP)/core/utils/nvos
LCINCS += -I$(TEGRA_TOP)/core/utils/nvos/aos
LCINCS += -I$(TEGRA_TOP)/core/drivers/include
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvrm/nvrmkernel/core/
LCINCS += -I$(TEGRA_TOP)/core-private/include
LCINCS += $(NVLIB_HWINC_SOC_INCLUDES)

LIB := $(OUTDIR)/libavpmain$(LIB_SUFFIX)

C_FILES :=
S_FILES :=

# common stuff
C_FILES += $(TEGRA_TOP)/core/utils/nvos/nvos_trace.c
C_FILES += $(TEGRA_TOP)/core/utils/nvos/nvos_debug.c
C_FILES += $(TEGRA_TOP)/core/utils/nvos/nvos_alloc.c
C_FILES += $(TEGRA_TOP)/core/utils/nvos/nvos_pointer_hash.c
C_FILES += $(TEGRA_TOP)/core/utils/nvos/nvos_thread_no_coop.c
C_FILES += $(TEGRA_TOP)/core/utils/nvosutils/nvustring.c
C_FILES += $(TEGRA_TOP)/core/utils/nvosutils/nvuhash.c

# aos

LCDEFS += -DNVAOS_SHELL=1
LCDEFS += -DNV_IS_AVP=$(IS_AVP)
ifeq ($(TARGET_SOC), ap20)
  LCDEFS += -DAES_KEYSCHED_LOCK_WAR_BUG_598910=1
else
  LCDEFS += -DAES_KEYSCHED_LOCK_WAR_BUG_598910=0
endif
ifeq ($(TARGET_SOC), t30)
	LCDEFS += -DSE_AES_KEYSCHED_READ_LOCK=1
else
	LCDEFS += -DSE_AES_KEYSCHED_READ_LOCK=0
endif
LCDEFS += -DCONFIG_PLLP_BASE_AS_408MHZ=$(CONFIG_PLLP_BASE_AS_408MHZ)
ifeq ($(NV_TARGET_BOOTLOADER_PINMUX),kernel)
LCDEFS += -DSET_KERNEL_PINMUX
endif

C_FILES += ../nvos_aos_core.c
C_FILES += ../dlmalloc.c
C_FILES += ../aos_profile.c
C_FILES += ../nvos_aos.c
C_FILES += ../aos_avpdebugsemi_stub.c
C_FILES += ../aos_process_args.c
C_FILES += ../nvos_aos_semi.c
C_FILES += ../aos_semi_uart.c
C_FILES += ../aos_semi_rvice.c
C_FILES += ../aos_print.c
C_FILES += ../../nvos_file.c

ifeq ($(TARGET_CPU),armv4)
  C_FILES += ../nvap/aos_avp.c
  C_FILES += ../nvap/aos_avp_t30.c
  C_FILES += ../nvap/aos_avp_cache_t30.c
  C_FILES += ../nvap/init_avp.c
  S_FILES += ../nvap/avp_override.S
endif

ifeq ($(TARGET_CPU),armv4)
ifeq ($(TARGET_OS),eabi)
  C_FILES += ../nvap/nvos_aos_gcc_avp.c
  GCOPTS += -mno-thumb -U__thumb
endif
endif

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))
OBJS += $(patsubst %.S,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(S_FILES)))

default: $(LIB)
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)

$(LIB): $(OBJS)
ifeq ($(NV_DEF_USE_DLL),1)
$(LIB): $(NVOS_DEF)
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules
