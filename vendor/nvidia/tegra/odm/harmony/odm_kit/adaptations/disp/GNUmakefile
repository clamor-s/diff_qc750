TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/../core-private/make/Makefile.defs


MODULE_SOURCES :=
MODULE_STATIC_LIBS :=
MODULE_DYNAMIC_LIBS :=

MODULE_SOURCES += display_hal.c
MODULE_SOURCES += panel_null.c
MODULE_SOURCES += amoled_amaf002.c
MODULE_SOURCES += panel_sharp_wvga.c
MODULE_SOURCES += bl_pcf50626.c
MODULE_SOURCES += bl_ap20.c
MODULE_SOURCES += panel_samsungsdi.c
MODULE_SOURCES += panel_tpo_wvga.c
MODULE_SOURCES += panel_sharp_dsi.c
MODULE_SOURCES += panel_lvds_wsga.c

include $(TOPDIR)/../core/drivers/nvodm/makefiles/Makefile.odm_disp
