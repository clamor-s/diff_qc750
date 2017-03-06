# Copyright (C) 2009 The Android Open Source Project
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
#

# This lists the packages that are necessary to build a device using
# Icera modem


PRODUCT_COPY_FILES += \
$(LOCAL_PATH)/init.modem_icera.rc:root/init.modem_icera.rc \
$(LOCAL_PATH)/init.icera_tegra_$(TARGET_PRODUCT).rc:root/init.icera_tegra_$(TARGET_PRODUCT).rc

# Nvidia baseband integration
PRODUCT_PACKAGES += nvidia_tegra_icera_common_modules \
                    nvidia_tegra_icera_phone_modules \
                    DatacallWhitelister
