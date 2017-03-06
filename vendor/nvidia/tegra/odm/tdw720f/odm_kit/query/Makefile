TOPDIR := $(TEGRA_TOP)/core
include $(TOPDIR)/../core-private/make/Makefile.defs


MODULE_STATIC_LIBS :=
MODULE_DYNAMIC_LIBS :=
MODULES_SOURCES :=
MODULE_INCLUDE_DIRS :=

LCDEFS += -DLPM_BATTERY_CHARGING=1

MODULE_SOURCES +=nvodm_query.c
MODULE_SOURCES +=nvodm_query_discovery.c
MODULE_SOURCES +=nvodm_query_nand.c
MODULE_SOURCES +=nvodm_query_gpio.c
MODULE_SOURCES +=nvodm_query_pinmux.c
MODULE_SOURCES +=nvodm_query_kbc.c
MODULE_SOURCES +=secure/nvodm_query_secure.c

#SECURE_MODULE_SOURCES :=
#SECURE_MODULE_SOURCES += secure/nvodm_query_secure.c

MODULE_INCLUDE_DIRS += -I$(TEGRA_TOP)/customers/nvidia-partner/template/odm_kit/adaptations


include $(TOPDIR)/../core/drivers/nvodm/makefiles/Makefile.odm_query
