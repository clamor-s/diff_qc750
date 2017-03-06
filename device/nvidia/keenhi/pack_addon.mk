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

INTERNAL_ADDON_OUT := $(PRODUCT_OUT)/addon.img
INTERNAL_ADDON_TEST_FILE := device/keenhi/addon/FuhuKidsApps/test/

.PHONY: pack_addon create_sct

LOCAL_ADDON_INSTALL_FILES :=

unique_product_copy_files_destinations1 :=
$(foreach cf,$(PRODUCT_COPY_FILES_ADDON), \
    $(eval _src := $(call word-colon,1,$(cf))) \
    $(eval _dest := $(call word-colon,2,$(cf))) \
    $(if $(filter $(unique_product_copy_files_destinations1),$(_dest)),, \
        $(eval _fulldest := $(call append-path,$(PRODUCT_OUT),$(_dest))) \
        $(eval $(call copy-one-file,$(_src),$(_fulldest))) \
        $(eval LOCAL_ADDON_INSTALL_FILES += $(_fulldest)) \
        $(eval unique_product_copy_files_destinations1 += $(_dest))))
unique_product_copy_files_destinations1 :=

pack_addon: FORCE $(LOCAL_ADDON_INSTALL_FILES)
	@echo "Package..." $(PRODUCT_OUT)
	rm -f $(INTERNAL_ADDON_OUT)
	rm -rf $(PRODUCT_OUT)/addon/test/
	$(call build-custom_image-target,$(INTERNAL_ADDON_OUT),$(PRODUCT_OUT)/addon)	