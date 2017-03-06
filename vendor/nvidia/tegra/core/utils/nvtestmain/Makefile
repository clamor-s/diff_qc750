TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


TESTMAIN_LIB := $(OUTDIR)/libnvtestmain$(LIB_SUFFIX)
TESTRUN_LIB := $(OUTDIR)/libnvtestrun$(LIB_SUFFIX)

TESTMAIN_C_FILES := nvmain.c
TESTRUN_C_FILES := nvrun.c

C_FILES := $(TESTMAIN_C_FILES) $(TESTRUN_C_FILES)

TESTMAIN_OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(TESTMAIN_C_FILES)))
TESTRUN_OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(TESTRUN_C_FILES)))

LCINCS += $(NVLIB_COMMON_INCLUDES)

default: $(TESTMAIN_LIB) $(TESTRUN_LIB)
	$(DO_HOST_BUILD_TOO)

$(TESTMAIN_LIB): $(TESTMAIN_OBJS)
$(TESTRUN_LIB): $(TESTRUN_OBJS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
