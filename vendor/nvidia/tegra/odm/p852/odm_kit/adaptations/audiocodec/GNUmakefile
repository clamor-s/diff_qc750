TOPDIR := $(TEGRA_TOP)/core
include $(TEGRA_TOP)/core-private/make/Makefile.defs
include $(TEGRA_TOP)/core-private/make/Makefile.nvlibdefs

MODULE_SOURCES :=
MODULE_STATIC_LIBS :=
MODULE_DYNAMIC_LIBS :=

MODULE_SOURCES += audiocodec_hal.c
MODULE_SOURCES += nvodm_codec_ad1937.c
MODULE_SOURCES += nvodm_codec_dirana.c

include $(TEGRA_TOP)/core/drivers/nvodm/makefiles/Makefile.odm_audio
