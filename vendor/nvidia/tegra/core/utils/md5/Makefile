TOPDIR := ../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs


MODULE_NAME := libmd5

LIB := $(OUTDIR)/$(MODULE_NAME)$(LIB_SUFFIX)
LIB_HOST_STATIC := $(OUTDIR)/$(MODULE_NAME)_host_static$(LIB_SUFFIX)

ifeq ($(HOST_BUILD)-$(OUTDIR),0-$(HOST_OUTDIR))
  TO_BUILD := $(LIB) $(LIB_HOST_STATIC)
else
  # Build the host static lib only on the host
  ifeq ($(HOST_BUILD),0)
    TO_BUILD := $(LIB)
  else
    TO_BUILD := $(LIB_HOST_STATIC)
  endif
endif

LCINCS += $(NVLIB_COMMON_INCLUDES)

C_FILES := md5.c

EXPORT_FILES := $(MODULE_NAME).export

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))
OBJS_HOST_STATIC := $(OBJS)

default: $(TO_BUILD)
	$(DO_HOST_BUILD_TOO)

$(LIB): $(OBJS)
$(LIB_HOST_STATIC): $(OBJS_HOST_STATIC)

RELEASE_PACKAGE_TARGETS :=
RELEASE_PACKAGE_FILES :=


include $(TEGRA_TOP)/core-private/make/Makefile.rules
