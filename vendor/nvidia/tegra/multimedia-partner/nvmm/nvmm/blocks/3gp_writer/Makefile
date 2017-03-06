TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

include $(TEGRA_TOP)/multimedia-partner/openmax/Makefile.omxdefs
LIB := $(OUTPUT_DIRNAME)/libnv3gpwriter$(LIB_SUFFIX)

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I../../../include
LCINCS += -I../../include
LCINCS += -I../include
LCINCS += -I../../../../include/openmax/il
LCDEFS += -DNV_IS_AVP=$(IS_AVP)
ifeq ($(NV_LOGGER_ENABLED),1)
  LCDEFS += -DNV_LOGGER_ENABLED=1
endif

C_FILES := 
C_FILES += nvmm_3gpwriterblock.c
C_FILES += nv_3gp_writer.c
C_FILES += nv3gpwritervideo.c
C_FILES += nv3gpwriteraudio.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

$(LIB): $(OBJS)

ifeq ($(NV_DEF_USE_DLL),0)
  RELEASE_PACKAGE_TARGETS := $(LIB)
  RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)
else
  RELEASE_PACKAGE_TARGETS :=
  RELEASE_PACKAGE_FILES :=
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules

