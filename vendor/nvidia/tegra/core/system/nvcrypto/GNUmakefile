##
# Copyright (c) 2005-2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
# Makefile for nvcrypto
#
TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libnvcrypto
SWAES_MODULE_NAME := libnvswcrypto
ifneq ($(TARGET_OS),rvds)
 LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)
 SWAESLIB := $(OUTDIR)/$(SWAES_MODULE_NAME)$(LIB_SUFFIX)
else
 LIB := $(OUTDIR)/$(MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
 SWAESLIB := $(OUTDIR)/$(SWAES_MODULE_NAME)$(DLL_OR_LIB_SUFFIX)
 TARGETS := $(addprefix $(OUTDIR)/$(MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))
 SWAESTARGETS := $(addprefix $(OUTDIR)/$(SWAES_MODULE_NAME),$(DLL_OR_LIB_SUFFIXES))
endif

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I$(TOPDIR)/utils/aes_ref

C_FILES :=
C_FILES += nvcrypto_padding.c
C_FILES += nvcrypto_cipher.c
C_FILES += nvcrypto_cipher_aes.c
C_FILES += nvcrypto_hash.c
C_FILES += nvcrypto_hash_cmac.c
C_FILES += nvcrypto_random.c
C_FILES += nvcrypto_random_ansix931.c

SWAES_C_FILES :=
SWAES_C_FILES += nvcrypto_padding.c
SWAES_C_FILES += nvcrypto_cipher.c
SWAES_C_FILES += nvcrypto_cipher_sw_aes.c
SWAES_C_FILES += nvcrypto_hash.c
SWAES_C_FILES += nvcrypto_hash_cmac.c

EXPORT_FILES := $(MODULE_NAME).export
EXPORT_FILES += $(SWAES_MODULE_NAME).export

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))
SWAESOBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(SWAES_C_FILES)))

default: $(LIB) $(SWAESLIB)
ifeq ($(TARGET_OS),rvds)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)
	$(NV_INSTALL) -l $(SWAESTARGETS) $(INSTALL_TARGET)
else
	$(NV_INSTALL) -l $(LIB) $(INSTALL_TARGET)
	$(NV_INSTALL) -l $(SWAESLIB) $(INSTALL_TARGET)
endif

ifneq ($(TARGET_OS),rvds)
  $(LIB): $(OBJS)
  $(SWAESLIB): $(SWAESOBJS)
else
  $(TARGETS): $(OBJS)
  $(TARGETS): $(OUTDIR)/$(MODULE_NAME).def
  $(SWAESTARGETS): $(SWAESOBJS)
  $(SWAESTARGETS): $(OUTDIR)/$(SWAES_MODULE_NAME).def
endif

DEPS :=
DEPS += $(OBJS)
DEPS += $(SWAESOBJS)
DEPS += $(NVLIB_NVOS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
