TOPDIR := $(TEGRA_TOP)/core

include $(TEGRA_TOP)/tests/make/Makefile.nvtestdefs
include $(TEGRA_TOP)/multimedia-partner/openmax/Makefile.omxdefs

TEST_NAME := omxplayer

LCINCS += -I../common

ifeq ($(TARGET_OS)-$(TARGET_CPU),linux-x86)
LCDEFS += -DUSE_ANDROID_WINSYS=0
endif

C_FILES :=
C_FILES += omxplayer.c 
C_FILES += main_omxplayer.c 
C_FILES += samplecomponent.c
C_FILES += sampleprotocol.c
C_FILES += nvhttppost.c
C_FILES += ../common/samplebase.c
C_FILES += ../common/omxtestlib.c

ifeq ($(TARGET_OS),wince)
  C_FILES += ../common/samplewindow_wince.c
else
  C_FILES += ../common/samplewindow_none.c
endif

TEST_LIBS += $(NVLIB_NVMM_UTILS)
TEST_LIBS += $(NVLIB_NVMM)
TEST_LIBS += $(NVLIB_NVRM)
TEST_LIBS += $(NVLIB_OMX_IL)
TEST_LIBS += $(NVLIB_NVDDK_2D)
TEST_LIBS += $(NVLIB_NVDDK_2D_V2)
TEST_LIBS += $(NVLIB_NVDDK_DISP)
TEST_LIBS += $(NVLIB_TESTMAIN)
TEST_LIBS += $(NVLIB_FXMATH)
TEST_LIBS += $(NVLIB_NVMM_CONTENTPIPE) 
ifeq ($(NV_DEF_USE_DLL),0)
  TEST_LIBS += $(NVLIB_MD5)
  TEST_LIBS += $(NVLIB_NVMM_SERVICE)
  TEST_LIBS += $(NVLIB_NVMM_MANAGER)
  TEST_LIBS += $(NVLIB_NVSM)
  TEST_LIBS += $(NVLIB_ARDRV)
  TEST_LIBS += $(NVLIB_NVODM_AUDIOCODEC)
  TEST_LIBS += $(NVLIB_NVODM_DTVTUNER)
  TEST_LIBS += $(NVLIB_NVODM_DISP)
  TEST_LIBS += $(NVLIB_NVODM_SENSOR)
  TEST_LIBS += $(NVLIB_NVODM_SERVICES)
  TEST_LIBS += $(NVLIB_NVDDK_MISC)
  TEST_LIBS += $(NVLIB_NVDDK_AUDIOMIXER)
  TEST_LIBS += $(NVLIB_DISPMGR)
  TEST_LIBS += $(NVLIB_NVAVP)
  TEST_LIBS += $(NVLIB_NVMMLITE)
  TEST_LIBS += $(NVLIB_NVMMLITE_UTILS)
endif

RELEASE_PACKAGE_TARGETS := $(TEST_NAME)$(EXE_SUFFIX)
RELEASE_PACKAGE_FILES := 

include $(TEGRA_TOP)/tests/make/Makefile.nvtestrules
