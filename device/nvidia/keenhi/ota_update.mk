# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

name := $(TARGET_PRODUCT)
ifeq ($(TARGET_BUILD_TYPE),debug)
  name := $(name)_debug
endif
name := $(name)-ota-release-$(FILE_NAME_TAG)

intermediates := $(call intermediates-dir-for,PACKAGING,target_files_sig)
KEENHI_BUILT_TARGET_FILES_PACKAGE_SIG := $(intermediates)/$(name).zip
INTERNAL_SIGN_SYSTEM := $(PRODUCT_OUT)/system_sig
INTERNAL_SIGN_RECOVERY_FILE := $(PRODUCT_OUT)/recovery_sig.img
INTERNAL_SIGN_SYSTEM_FILE := $(INTERNAL_SIGN_SYSTEM).img
INTERNAL_SIGN_ADDON := addon_nabi2
ifneq (,$(filter $(KEENHI_ADDON_NAME),addon))
	INTERNAL_SIGN_ADDON := $(KEENHI_ADDON_NAME)
endif

kh_recovery_kernel := $(INSTALLED_KERNEL_TARGET) # same as a non-recovery system
kh_recovery_ramdisk := $(PRODUCT_OUT)/ramdisk-recovery.img


KH_INTERNAL_RECOVERYIMAGE_ARGS := \
	$(addprefix --second ,$(INSTALLED_2NDBOOTLOADER_TARGET)) \
	--kernel $(kh_recovery_kernel) \
	--ramdisk $(kh_recovery_ramdisk)
	
# Assumes this has already been stripped
ifdef BOARD_KERNEL_CMDLINE
  KH_INTERNAL_RECOVERYIMAGE_ARGS += --cmdline "$(BOARD_KERNEL_CMDLINE)"
endif
ifdef BOARD_KERNEL_BASE
  KH_INTERNAL_RECOVERYIMAGE_ARGS += --base $(BOARD_KERNEL_BASE)
endif
BOARD_KERNEL_PAGESIZE := $(strip $(BOARD_KERNEL_PAGESIZE))
ifdef BOARD_KERNEL_PAGESIZE
  KH_INTERNAL_RECOVERYIMAGE_ARGS += --pagesize $(BOARD_KERNEL_PAGESIZE)
endif	
	

$(KEENHI_BUILT_TARGET_FILES_PACKAGE_SIG): $(BUILT_TARGET_FILES_PACKAGE) $(OTATOOLS) pack_addon
		@echo "Package OTA: $@ BUILT_TARGET_FILES_PACKAGE=$(BUILT_TARGET_FILES_PACKAGE)"
	$(hide) rm -rf $@ 
	$(hide) mkdir -p $(dir $@)
	$(hide) ./build/tools/releasetools/sign_target_files_apks \
		-k build/target/product/security/testkey=$(RELEASE_KEY_RELEASE) \
		-k build/target/product/security/platform=$(RELEASE_KEY_PLATFORM) \
		-k build/target/product/security/shared=$(RELEASE_KEY_SHARED) \
		-k build/target/product/security/media=$(RELEASE_KEY_MEDIA) \
		-o \
		$(BUILT_TARGET_FILES_PACKAGE) $@
		unzip -o $@ -d $(INTERNAL_SIGN_SYSTEM)
		$(call build-custom_image-target,$(INTERNAL_SIGN_SYSTEM_FILE),$(INTERNAL_SIGN_SYSTEM)/SYSTEM)
		$(MKBOOTFS) $(INTERNAL_SIGN_SYSTEM)/RECOVERY/RAMDISK | $(MINIGZIP) > $(kh_recovery_ramdisk)
		$(MKBOOTIMG) $(KH_INTERNAL_RECOVERYIMAGE_ARGS) --output $(INTERNAL_SIGN_RECOVERY_FILE)
		rm -rf $(INTERNAL_SIGN_SYSTEM)
		-mkdir $(PRODUCT_OUT)/$(INTERNAL_SIGN_ADDON)
		$(eval tmp_ota_path := $(shell pwd)/$@)
		$(if $(KEENHI_DISABLE_OTA_RES),,\
		cd $(shell pwd)/$(PRODUCT_OUT) && echo $(shell pwd) && zip -r $(tmp_ota_path)  ./$(INTERNAL_SIGN_ADDON);)


# -----------------------------------------------------------------
# OTA update package, this will depend on release package.

name := $(TARGET_PRODUCT)
ifeq ($(TARGET_BUILD_TYPE),debug)
  name := $(name)_debug
endif
name := $(name)-ota-update-$(FILE_NAME_TAG)

KEENHI_INTERNAL_OTA_PACKAGE_TARGET := $(PRODUCT_OUT)/$(name).zip

KEENHI_OTA_UPDATE_MAIN_PARA	:= -v 
LOCAL_KEENHI_OTA_DEP := $(KEENHI_BUILT_TARGET_FILES_PACKAGE_SIG) $(OTATOOLS)

ifneq ($(filter ota_update_nodep ota_update_inc_nodep,$(MAKECMDGOALS)),)
	LOCAL_KEENHI_OTA_DEP := FORCE
endif

ifeq ($(KEENHI_DISABLE_OTA_RES),true)
	#don't add the addon_res to ota
	KEENHI_OTA_UPDATE_ADDON :=
else
	KEENHI_OTA_UPDATE_ADDON := --res
endif	

ifneq ($(filter ota_update_inc ota_update_inc_nodep,$(MAKECMDGOALS)),)
	name := $(TARGET_PRODUCT)
	ifeq ($(TARGET_BUILD_TYPE),debug)
  	name := $(name)_debug
	endif
	name := $(name)-ota-update-$(KEENHI_OTA_UPDATE_INCREMENTAL_FROM_VERSION)-to-$(FILE_NAME_TAG)
	KEENHI_INTERNAL_OTA_PACKAGE_TARGET := $(PRODUCT_OUT)/$(name).zip
	
	KEENHI_OTA_UPDATE_MAIN_PARA += --mount_cache -i $(KEENHI_OTA_INCREMENTAL_FROM_FILE) $(KEENHI_OTA_UPDATE_ADDON)
endif


ifdef KEENHI_OTA_KEY
$(KEENHI_INTERNAL_OTA_PACKAGE_TARGET) ota_update_inc_all ota_update_inc_spec: KEY_CERT_PAIR := $(KEENHI_OTA_KEY)
else
$(KEENHI_INTERNAL_OTA_PACKAGE_TARGET) ota_update_inc_all ota_update_inc_spec: KEY_CERT_PAIR := $(DEFAULT_KEY_CERT_PAIR)
endif

$(KEENHI_INTERNAL_OTA_PACKAGE_TARGET): $(LOCAL_KEENHI_OTA_DEP)
	@echo "Package OTA: $@ "
	@echo "Package OTA: KEENHI_OTA_UPDATE_MAIN_PARA=$(KEENHI_OTA_UPDATE_MAIN_PARA) "
	$(hide) ./build/tools/releasetools/ota_from_target_files \
		$(KEENHI_OTA_UPDATE_MAIN_PARA) \
	  -p $(HOST_OUT) \
    -k $(KEY_CERT_PAIR) \
    $(KEENHI_BUILT_TARGET_FILES_PACKAGE_SIG) $@


.PHONY: ota_update ota_update_nodep ota_update_inc ota_update_inc_nodep ota_update_inc_all
ota_update: $(KEENHI_INTERNAL_OTA_PACKAGE_TARGET)

ota_update_nodep: $(KEENHI_INTERNAL_OTA_PACKAGE_TARGET)

ota_update_inc: $(KEENHI_INTERNAL_OTA_PACKAGE_TARGET)

ota_update_inc_nodep: $(KEENHI_INTERNAL_OTA_PACKAGE_TARGET)

ota_update_inc_all: LOCAL_ALL_KH_UPDATE_ZIP_SOURCE := $(strip $(wildcard $(KEENHI_OTA_UPDATE_REPO_CID)$(TARGET_PRODUCT)-ota-release-*.zip))

define make-incremental-update-zip
	$(info "1=$(1) 2=$(2) 3=$(3)")
	$(hide) ./build/tools/releasetools/ota_from_target_files \
	$(2) \
	-p $(HOST_OUT) \
	-k $(KEY_CERT_PAIR) \
	$(3) $(1)
endef

INTERNAL_ALL_INC_UPDATE := $(PRODUCT_OUT)/all-incremental-updates-$(FILE_NAME_TAG).zip


$(INTERNAL_ALL_INC_UPDATE): FORCE
	rm -f $@
	$(foreach m, $(LOCAL_ALL_KH_UPDATE_ZIP_SOURCE), \
		$(info "m=$(m)") \
		$(eval local_version := $(shell echo $(m) | awk 'BEGIN {FS="-release-|.zip"} {print $$2}')) \
		$(info "local_version=$(local_version)") \
		$(eval name := $(TARGET_PRODUCT)-ota-update-$(local_version)-to-$(KEENHI_OTA_SPEC_TARGET_VERSION)) \
		$(eval name := $(PRODUCT_OUT)/$(name).zip) \
		$(rm $(name)) \
		$(eval local_KEENHI_OTA_UPDATE_MAIN_PARA := $(KEENHI_OTA_UPDATE_MAIN_PARA) $(KEENHI_OTA_UPDATE_ADDON) --mount_cache -i $(m)) \
		$(info local_KEENHI_OTA_UPDATE_MAIN_PARA=$(local_KEENHI_OTA_UPDATE_MAIN_PARA)) \
		$(call make-incremental-update-zip, $(name), $(local_KEENHI_OTA_UPDATE_MAIN_PARA), $(KEENHI_OTA_UPDATE_REPO)$(TARGET_PRODUCT)-ota-release-$(KEENHI_OTA_SPEC_TARGET_VERSION).zip); \
		cd $(PRODUCT_OUT); \
		zip ./$(notdir $(INTERNAL_ALL_INC_UPDATE)) ./$(notdir $(name)); \
		cd - ; \
	)

ota_update_inc_all: $(INTERNAL_ALL_INC_UPDATE)


ifneq ($(filter ota_update_inc_spec,$(MAKECMDGOALS)),)

LOCAL_KEENHI_SPEC_OUT_FILE := $(PRODUCT_OUT)/$(TARGET_PRODUCT)-ota-update-$(KEENHI_OTA_SPEC_SOURCE_VERSION)-to-$(KEENHI_OTA_SPEC_TARGET_VERSION).zip

LOCAL_KEENHI_SPEC_SOURCE_FILE := $(KEENHI_OTA_UPDATE_REPO_CID)$(TARGET_PRODUCT)-ota-release-$(KEENHI_OTA_SPEC_SOURCE_VERSION).zip
LOCAL_KEENHI_SPEC_TARGET_FILE := $(KEENHI_OTA_UPDATE_REPO_CID)$(TARGET_PRODUCT)-ota-release-$(KEENHI_OTA_SPEC_TARGET_VERSION).zip


$(LOCAL_KEENHI_SPEC_OUT_FILE):
	$(eval local_KEENHI_OTA_UPDATE_MAIN_PARA := $(KEENHI_OTA_UPDATE_MAIN_PARA) $(KEENHI_OTA_UPDATE_ADDON) --mount_cache -i $(LOCAL_KEENHI_SPEC_SOURCE_FILE))
	$(info local_KEENHI_OTA_UPDATE_MAIN_PARA=$(local_KEENHI_OTA_UPDATE_MAIN_PARA))
	$(call make-incremental-update-zip, $(LOCAL_KEENHI_SPEC_OUT_FILE), $(local_KEENHI_OTA_UPDATE_MAIN_PARA), $(LOCAL_KEENHI_SPEC_TARGET_FILE))


ota_update_inc_spec: $(LOCAL_KEENHI_SPEC_OUT_FILE)


endif

$(call dist-for-goals, dist_files, \
	$(KEENHI_INTERNAL_OTA_PACKAGE_TARGET) \
	$(KEENHI_BUILT_TARGET_FILES_PACKAGE_SIG) \
  $(INTERNAL_SIGN_RECOVERY_FILE) \
  $(INTERNAL_SIGN_SYSTEM_FILE) \
  $(PRODUCT_OUT)/bootloader.bin \
  $(PRODUCT_OUT)/boot.img \
  $(PRODUCT_OUT)/eks.dat \
 )
