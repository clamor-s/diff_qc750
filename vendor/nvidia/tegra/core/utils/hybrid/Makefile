TOPDIR := ../..
USE_VS2008 := 1
include $(TEGRA_TOP)/core-private/make/Makefile.defs


# INCLUDE / HEADER directories
#######################################
LCINCS += -Iinclude
LCINCS += $(NVLIB_COMMON_INCLUDES)

# PREPROCESSOR Macros / DEFINES
#######################################
LCDEFS += -DHG_NATIVE_FLOATS

# SOURCES (SRCS)
#######################################
include Makefile.src
C_FILES := $(HG_SRCS) $(STITCHGEN_SRCS) $(ARMEMULATOR_SRCS)

# OBJECT files from SRCS
#######################################
OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

# Libraries to be generated
#######################################
LIB := $(OUTDIR)/libhybrid$(LIB_SUFFIX)

######################################################################
# Build Rules
######################################################################
$(LIB): $(OBJS)

ifeq ($(NV_DEF_USE_DLL),0)
  RELEASE_PACKAGE_TARGETS := $(LIB)
  RELEASE_PACKAGE_FILES := $(RELEASE_PACKAGE_TARGETS)
else
  RELEASE_PACKAGE_TARGETS :=
  RELEASE_PACKAGE_FILES :=
endif

include $(TEGRA_TOP)/core-private/make/Makefile.rules
