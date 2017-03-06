###############################################################################
#
# Copyright (c) 2011-2012, NVIDIA CORPORATION.  All Rights Reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#
###############################################################################
HOST_BUILD := 1

TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs



ifeq ($(NVODM_BOARD_IS_FPGA),1)
CFLAGS += -DNVODM_BOARD_IS_FPGA=1
else
CFLAGS += -DNVODM_BOARD_IS_FPGA=0
endif
CFLAGS += -DNVODM_ENABLE_SIMULATION_CODE=0

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/system/nvflash/lib
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvboot/include
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvboot/ap20/include
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvboot/t30/include
LCINCS += -I$(TEGRA_TOP)/core/system/nvdioconverter
LCINCS += -I$(TEGRA_TOP)/core/system/nvflash/nvml/ap20/$(AVP_OUTDIR)
LCINCS += -I$(TEGRA_TOP)/core/system/nvflash/nvml/t30/$(AVP_OUTDIR)
LCINCS += -I$(TEGRA_TOP)/core/system/nvbuildbct

ifneq ($(HOST_OS),winxp)
  NVFLASH_STATIC := 1
  LDFLAGS_EXE += -static -lpthread -ldl
endif

EXECUTABLES := $(OUTDIR)/nvflash$(EXE_SUFFIX)
ifeq ($(TARGET_BOARD),whistler)
  NVFLASH_INPUT_FILE := $(TEGRA_TOP)/customers/nvidia-partner/whistler/nvflash/*.bct
  NVFLASH_INPUT_FILE += $(TEGRA_TOP)/customers/nvidia-partner/whistler/nvflash/*.cfg
endif

ifeq ($(TARGET_BOARD),aruba2)
  NVFLASH_INPUT_FILE += $(TEGRA_TOP)/customers/nvidia/aruba2/nvflash/*.bct
  NVFLASH_INPUT_FILE += $(TEGRA_TOP)/customers/nvidia/aruba2/nvflash/*.cfg
endif

ifneq ($(NVODM_PARTITION_DATA_BOOT_IMAGE),)
 NVFLASH_INPUT_FILE += $(NVODM_PARTITION_DATA_BOOT_IMAGE)
endif

ifneq ($(NVODM_PARTITION_DATA_CHARGE_IMAGE),)
 NVFLASH_INPUT_FILE += $(NVODM_PARTITION_DATA_CHARGE_IMAGE)
endif

ifneq ($(NVODM_PARTITION_DATA_BOOT_AUDIO),)
 NVFLASH_INPUT_FILE += $(NVODM_PARTITION_DATA_BOOT_AUDIO)
endif

.DEFAULT:
	$(RM) nvflash_miniloader_ap20.h
	$(RM) nvflash_miniloader_t30.h

C_FILES :=
C_FILES += nvflash_app.c

ifeq ($(NV_EMBEDDED_BUILD),1)
C_FILES += nvimager.c
endif

C_FILES += nvflash_util.c
C_FILES += nvflash_usage.c
C_FILES += nvflash_util_ap20.c
C_FILES += nvflash_util_t30.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

default: $(EXECUTABLES)
	$(NV_INSTALL) -l $(EXECUTABLES) $(INSTALL_TARGET)
ifneq ($(TARGET_BOARD),simulation)
ifneq ($(NVFLASH_INPUT_FILE),)
	$(NV_INSTALL) -l $(NVFLASH_INPUT_FILE) $(INSTALL_TARGET)
endif
endif

$(EXECUTABLES): $(OBJS)

ifneq ($(NVFLASH_STATIC),)
  $(EXECUTABLES): $(NVLIB_APPUTIL_HOST_STATIC)
  $(EXECUTABLES): $(NVLIB_NVOS_HOST_STATIC)
  $(EXECUTABLES): $(NVLIB_NV3P_HOST_STATIC)
  $(EXECUTABLES): $(NVLIB_NVFLASH_HOST_STATIC)
  $(EXECUTABLES): $(NVLIB_NVDIOCONVERTER_HOST_STATIC)
  $(EXECUTABLES): $(NVLIB_USBHOST_HOST_STATIC)
  $(EXECUTABLES): $(NVLIB_NVBOOTHOST_HOST_STATIC)
  $(EXECUTABLES): $(NVLIB_TESTRESULTS_HOST_STATIC)
  $(EXECUTABLES): $(NVLIB_NVAES_REF_HOST_STATIC)
  $(EXECUTABLES): $(NVLIB_TESTIO_HOST_STATIC)
  $(EXECUTABLES): $(NVLIB_NVBUILDBCT_HOST_STATIC)
else
  $(EXECUTABLES): $(NVLIB_APPUTIL)
  $(EXECUTABLES): $(NVLIB_NVOS)
  ifeq ($(HOST_OS),winxp)
    $(EXECUTABLES): $(TEGRA_TOP)/core/system/nv3p/$(OUTPUT_DIRNAME)/libnv3p$(LIB_SUFFIX)
    $(EXECUTABLES): $(TEGRA_TOP)/core/system/nvflash/lib/$(OUTPUT_DIRNAME)/libnvflash$(LIB_SUFFIX)
    $(EXECUTABLES): $(TEGRA_TOP)/core/system/nvdioconverter/$(OUTPUT_DIRNAME)/libnvdioconverter$(LIB_SUFFIX)
  else
    $(EXECUTABLES): $(TEGRA_TOP)/core/system/nv3p/$(OUTPUT_DIRNAME)/libnv3p$(DLL_SUFFIX)
    $(EXECUTABLES): $(TEGRA_TOP)/core/system/nvflash/lib/$(OUTPUT_DIRNAME)/libnvflash$(DLL_SUFFIX)
    $(EXECUTABLES): $(TEGRA_TOP)/core/system/nvdioconverter/$(OUTPUT_DIRNAME)/libnvdioconverter$(DLL_SUFFIX)
  endif
  $(EXECUTABLES): $(NVLIB_USBHOST)
  $(EXECUTABLES): $(NVLIB_NVBOOTHOST)
  $(EXECUTABLES): $(NVLIB_TESTRESULTS)
  $(EXECUTABLES): $(NVLIB_NVAES_REF)
  $(EXECUTABLES): $(NVLIB_TESTIO)
  $(EXECUTABLES): $(NVLIB_NVBUILDBCT)
endif


# Use vcbuild.exe upgrade a VS 2005 .vcproj file to the latest file format (current VS 2008)
.PHONY: vcbuild_upgrade
vcbuild_upgrade:
	@$(ECHO) running vcbuild_upgrade
	$(CHMOD) -R a-r,+w $(VCPROJ_FILE)
ifdef COPY_DEF_FILE_FOR_VCPROJ
	$(CP) $(VCPROJ_FILE)/../$(OUTDIR)/*.def $(VCPROJ_FILE)/..
endif
	$(VCBUILDPATH)/vcbuild.exe /nologo /nocolor /upgrade "$(VCPROJ_FILE)"


ifeq ($(BUILD_FLAVOR),release)
  VCBUILD_DEFAULT_CFG:=Release|Win32
else
  VCBUILD_DEFAULT_CFG:=Debug|Win32
endif

# Build Visual C++ solution file using MSBUILD and VCBUILD
.PHONY: vc_sln_build
vc_sln_build:
	$(CP) ../nvml/_out/debug_nvap15_rvds_armv4/nvflash_miniloader.h .
	#$(VCBUILDPATH)/vcbuild.exe /clean "$(SLN_FILE)" "$(VCBUILD_DEFAULT_CFG)"
	$(VCBUILDPATH)/vcbuild.exe "$(SLN_FILE)"  "$(VCBUILD_DEFAULT_CFG)"

.PHONY: prepare_sln
prepare_sln:
	$(MAKE) vcbuild_upgrade VCPROJ_FILE=$(TEGRA_TOP)/core/system/nvflash/app/nvflash.vcproj
	$(MAKE) vcbuild_upgrade VCPROJ_FILE=$(TEGRA_TOP)/core/system/nvflash/lib/libnvflash.vcproj COPY_DEF_FILE_FOR_VCPROJ=1
	$(MAKE) vcbuild_upgrade VCPROJ_FILE=$(TEGRA_TOP)/core/drivers/nvboot/lib/host/libnvboothost.vcproj  COPY_DEF_FILE_FOR_VCPROJ=1
	$(MAKE) vcbuild_upgrade VCPROJ_FILE=$(TEGRA_TOP)/core/utils/nvusbhost/libnvusbhost/libnvusbhost.vcproj
	$(MAKE) vcbuild_upgrade VCPROJ_FILE=$(TEGRA_TOP)/core/utils/nvos/libnvos.vcproj
	$(MAKE) vcbuild_upgrade VCPROJ_FILE=$(TEGRA_TOP)/core/system/nv3p/libnv3p.vcproj COPY_DEF_FILE_FOR_VCPROJ=1
	$(MAKE) vcbuild_upgrade VCPROJ_FILE=$(TEGRA_TOP)/core/utils/aes_ref/libnvaes_ref.vcproj
	$(MKDIR) -p $(TEGRA_TOP)/prebuilt/lib/win32/$(BUILD_FLAVOR)
	$(MKDIR) -p $(TEGRA_TOP)/prebuilt/bin/win32/$(BUILD_FLAVOR)
	$(CP) $(TEGRA_TOP)/core/utils/nvapputil/$(OUTDIR)/libnvapputil.lib $(TEGRA_TOP)/prebuilt/lib/win32/$(BUILD_FLAVOR)
	$(CP) $(TEGRA_TOP)/core/utils/nvapputil/$(OUTDIR)/libnvapputil.dll $(TEGRA_TOP)/prebuilt/bin/win32/$(BUILD_FLAVOR)
	$(CP) $(TEGRA_TOP)/core/system/nvdioconverter/$(OUTDIR)/libnvdioconverter.lib $(TEGRA_TOP)/prebuilt/lib/win32/$(BUILD_FLAVOR)
	$(CP) $(TEGRA_TOP)/core/system/nvdioconverter/$(OUTDIR)/libnvdioconverter.dll $(TEGRA_TOP)/prebuilt/bin/win32/$(BUILD_FLAVOR)

#Set the WDK_ROOT_DIR environment variable, which is used in some of our vcproj files.
export WDK_ROOT_DIR:=$(NV_WIN32_DDK_DIR)

.PHONY: build_sln
build_sln:
	$(MAKE) prepare_sln
	$(ECHO) WDK_ROOT_DIR=$(WDK_ROOT_DIR)
	$(MAKE) vc_sln_build SLN_FILE=$(TEGRA_TOP)/core/system/nvflash/app/nvflash.sln

include $(TEGRA_TOP)/core-private/make/Makefile.rules
