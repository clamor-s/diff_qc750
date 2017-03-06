TOPDIR := ../../../..
include $(TEGRA_TOP)/core-private/make/Makefile.defs

TEST_NAME := nvml_server

ifeq ($(NVODM_BOARD_IS_FPGA),1)
CFLAGS += -DNVODM_BOARD_IS_FPGA=1
else
CFLAGS += -DNVODM_BOARD_IS_FPGA=0
endif

LCDEFS += -DSE_AES_KEYSCHED_READ_LOCK=1
AES_KEYSCHED_LOCK_WAR_BUG_598910 := 0
NAND_SUPPORT := 1
NO_BOOTROM := 0

LCDEFS += -DAES_KEYSCHED_LOCK_WAR_BUG_598910=$(AES_KEYSCHED_LOCK_WAR_BUG_598910)
LCDEFS += -DNAND_SUPPORT=$(NAND_SUPPORT)
LCDEFS += -DNO_BOOTROM=$(NO_BOOTROM)

C_FILES := ../nvml_server.c
C_FILES += ../nvml_nvboot.c
C_FILES += ../nvml_device.c
C_FILES += t30ml_utils.c
C_FILES += nvml_aes.c
C_FILES += nvml_hash.c

OBJS := $(patsubst %.c,$(OUTDIR)/%$(OBJ_SUFFIX),$(notdir $(C_FILES)))

EXECUTABLES := $(OUTPUT_DIRNAME)/$(TEST_NAME)$(EXE_SUFFIX)

POST_EXECUTABLES := $(OUTPUT_DIRNAME)/StartRegion

TARGETS := $(OUTPUT_DIRNAME)/nvflash_miniloader_t30.h


LCINCS += $(NVLIB_COMMON_INCLUDES)
LCINCS += $(NVLIB_HWINC_INCLUDES)
LCINCS += -I$(TEGRA_TOP)/core/drivers/nvboot/t30/include
LCINCS += -I$(TEGRA_TOP)/core/drivers/hwinc/t30
LCINCS += -I$(TEGRA_TOP)/core/system/nvbct/t30
LCINCS += -I$(TEGRA_TOP)/core/system/nvflash/nvml
LCINCS += -I$(TEGRA_TOP)/core/system/nvflash/nvml/t30
LCINCS += -I$(TEGRA_TOP)/core/utils/aes_keysched_lock

TEST_LIBS :=
TEST_LIBS += $(NVLIB_NVOS_STATIC)
TEST_LIBS += $(TEGRA_TOP)/core/system/nv3p/$(OUTPUT_DIRNAME)/libnv3p$(LIB_SUFFIX)

ifeq ($(TARGET_CPU),armv4)
 TEST_LIBS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/sdram/$(OUTPUT_DIRNAME)/libnvboot_sdram$(LIB_SUFFIX)
 TEST_LIBS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/ahb/$(OUTPUT_DIRNAME)/libnvboot_ahb$(LIB_SUFFIX)
 TEST_LIBS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/clocks/$(OUTPUT_DIRNAME)/libnvboot_clocks$(LIB_SUFFIX)
 TEST_LIBS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/fuse/$(OUTPUT_DIRNAME)/libnvboot_fuse$(LIB_SUFFIX)
 TEST_LIBS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/reset/$(OUTPUT_DIRNAME)/libnvboot_reset$(LIB_SUFFIX)
 TEST_LIBS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/util/$(OUTPUT_DIRNAME)/libnvboot_util$(LIB_SUFFIX)
 TEST_LIBS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/pads/$(OUTPUT_DIRNAME)/libnvboot_pads$(LIB_SUFFIX)
 TEST_LIBS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/nand/$(OUTPUT_DIRNAME)/libnvboot_nand$(LIB_SUFFIX)
 TEST_LIBS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/spi_flash/$(OUTPUT_DIRNAME)/libnvboot_spi_flash$(LIB_SUFFIX)
 TEST_LIBS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/sdmmc/$(OUTPUT_DIRNAME)/libnvboot_sdmmc$(LIB_SUFFIX)
 TEST_LIBS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/snor/$(OUTPUT_DIRNAME)/libnvboot_snor$(LIB_SUFFIX)
 TEST_LIBS += $(TEGRA_TOP)/core-private/drivers/nvboot/t30/irom_patch/$(OUTPUT_DIRNAME)/libnvboot_irom_patch$(LIB_SUFFIX)
 TEST_LIBS += $(TEGRA_TOP)/core/system/nvbct/$(OUTPUT_DIRNAME)/libnvbct$(LIB_SUFFIX)
 TEST_LIBS += $(TEGRA_TOP)/core/utils/aes_keysched_lock/$(OUTPUT_DIRNAME)/libnvseaes_keysched_lock_avp$(LIB_SUFFIX)
SCATTER_SCRIPT := miniloader.ldscript
endif

default: $(TARGETS)
	$(NV_INSTALL) -l $(EXECUTABLES) $(INSTALL_TARGET)
	$(NV_INSTALL) -l $(TARGETS) $(INSTALL_TARGET)

LINKOPTS += --callgraph --info sizes --map --list miniloader.map

$(EXECUTABLES): $(OBJS)
$(EXECUTABLES): $(TEST_LIBS)
$(EXECUTABLES): $(SCATTER_SCRIPT)

# HACKHACKHACK -- The ldscript causes fromelf to always treat --output as an
# output directory, with a single file "StartRegion" in it.  So the standard
# %.bin rule will not work, and a custom rule (and dummy rule for release package
# generation) need to be provided here.
ifeq ($(NV_LIST_RELEASE_PACKAGE),1)
$(POST_EXECUTABLES): $(EXECUTABLES)
	@$(ECHO) "Dummy rule $@"
else
$(POST_EXECUTABLES): $(EXECUTABLES)
	$(ARM_RETRY) $(RVDS_PROGRAMS_DIR)/fromelf --output $(@D) --bin $<
endif

$(TARGETS): $(POST_EXECUTABLES)
	$(HEXIFY) $< $@


include $(TEGRA_TOP)/core-private/make/Makefile.rules
