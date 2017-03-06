TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


AVP_MODULELOADER_TEST ?= 0

LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_KHRONOS_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/graphics/openkode/include

C_FILES := nvappmain_standard.c

ifeq ($(TARGET_OS),linux)
  C_FILES := nvappmain_standard.c
endif
ifeq ($(TARGET_OS),winxp)
  C_FILES := nvappmain_standard.c
endif
ifneq (,$(filter $(TARGET_OS)-$(TARGET_OS_SUBTYPE),rvds-rvds linux-aos eabi-aos))
  LCINCS += -I$(TEGRA_TOP)/core/drivers/hwinc
  ifeq ($(TARGET_CPU),armv6)
    C_FILES := nvappmain_aos.c
    C_FILES += nvappmain_jtag.c
  endif
  ifeq ($(TARGET_CPU),armv4)
    ifneq ($(AVP_MODULELOADER_TEST),1)
      C_FILES := nvappmain_avp.c
      C_FILES += nvappmain_jtag.c
    endif
  endif
endif

# Primary OS-dependent version
OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))
LIB := $(OUTDIR)/libnvappmain$(LIB_SUFFIX)

# For KD testing, version using kdMain
C_FILES_KDMAIN := nvappmain_kdmain.c
OBJS_KDMAIN := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES_KDMAIN)))
LIB_KDMAIN := $(OUTDIR)/libnvappmain_kdmain$(LIB_SUFFIX)

# For KD testing, version using main + kdInitialize (currently only linux/qnx)
C_FILES_KDINIT := nvappmain_kdinit.c
OBJS_KDINIT := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES_KDINIT)))
LIB_KDINIT := $(OUTDIR)/libnvappmain_kdinit$(LIB_SUFFIX)

LIBS := $(LIB)
ifeq ($(OKD_SUPPORTED),1)
  LIBS += $(LIB_KDMAIN)
  ifneq ($(EMPTY),$(filter $(TARGET_OS),linux qnx))
    LIBS += $(LIB_KDINIT)
  endif
endif

default: $(LIBS)
	$(DO_HOST_BUILD_TOO)

$(LIB): $(OBJS)

$(LIB_KDMAIN): $(OBJS_KDMAIN)

$(LIB_KDINIT): $(OBJS_KDINIT)

include $(TEGRA_TOP)/core-private/make/Makefile.rules
