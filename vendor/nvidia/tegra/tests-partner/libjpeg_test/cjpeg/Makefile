TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs

NVLIB_LIBJPEG := $(TOPDIR)/$(OUTDIR)/libjpeg.so
LCINCS += -I$(TEGRA_TOP)/external/jpeg/
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_KHRONOS_INCLUDES)
LCINCS += -I$(OUTPUT_DIRNAME)

EXECUTABLES := $(OUTDIR)/cjpeg

CJPEG_C_FILES := cjpeg.c rdppm.c rdgif.c rdtarga.c rdrle.c rdbmp.c \
        rdswitch.c rdy4m.c cdjpeg.c

LDFLAGS_EXE += -Wl,--rpath -Wl,$(NVLIB_X11_ROOT)/lib -L$(NVLIB_X11_ROOT)/lib -L$(TEGRA_TOP)/external/jpeg

LIBS += $(NVLIB_LIBJPEG)
LIBS += $(NVLIB_NVRM)
LIBS += $(NVLIB_NVOS)
LIBS += $(NVLIB_NVAVP)
LIBS += $(NVLIB_NVDC)
LIBS += $(NVLIB_NVDDK_2D_V2)
LIBS += $(NVLIB_TVMR)

CJPEG_OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(CJPEG_C_FILES)))

default: $(EXECUTABLES)
	$(NV_INSTALL) -l $(EXECUTABLES) $(INSTALL_TARGET)

$(EXECUTABLES): $(CJPEG_OBJS) $(LIBS)

include $(TEGRA_TOP)/core-private/make/Makefile.rules

