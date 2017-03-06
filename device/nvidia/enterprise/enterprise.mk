# NVIDIA Tegra3 "Enterprise" development system

$(call inherit-product, device/nvidia/enterprise/device.mk)

PRODUCT_NAME := enterprise
PRODUCT_DEVICE := enterprise
PRODUCT_MODEL := Enterprise
PRODUCT_MANUFACTURER := NVIDIA

ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/customers/nvidia-partner/enterprise/nvflash/Enterprise_Samsung_768MB_KMMLL000QM-B503_400MHz_111114_sdmmc4_x8.bct:flash_a02.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/enterprise/nvflash/Enterprise_Samsung_768MB_KMMLL000QM-B503_400MHz_111114_sdmmc4_x8.bct:flash.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/enterprise/nvflash/E1205_Hynix_512MB_KMMLL0000QM-B503_300MHz_20110408.bct:flash_a01.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/enterprise/nvflash/Enterprise_Samsung_768MB_KMMLL000QM-B503_400MHz_111114_sdmmc4_x8.cfg:bct_a02.cfg \
    vendor/nvidia/tegra/customers/nvidia-partner/enterprise/nvflash/Enterprise_Samsung_768MB_KMMLL000QM-B503_400MHz_111114_sdmmc4_x8.cfg:bct.cfg \
    vendor/nvidia/tegra/customers/nvidia-partner/enterprise/nvflash/E1205_Hynix_512MB_KMMLL0000QM-B503_300MHz_20110408.cfg:bct_a01.cfg
ifeq ($(APPEND_DTB_TO_KERNEL), true)
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/customers/nvidia-partner/enterprise/nvflash/android_fastboot_microboot_emmc_full.cfg:flash.cfg
else
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/customers/nvidia-partner/enterprise/nvflash/android_fastboot_microboot_dtb_emmc_full.cfg:flash.cfg
endif
else
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/odm/enterprise/nvflash/Enterprise_Samsung_768MB_KMMLL000QM-B503_400MHz_111114_sdmmc4_x8.bct:flash_a02.bct \
    vendor/nvidia/tegra/odm/enterprise/nvflash/Enterprise_Samsung_768MB_KMMLL000QM-B503_400MHz_111114_sdmmc4_x8.bct:flash.bct \
    vendor/nvidia/tegra/odm/enterprise/nvflash/E1205_Hynix_512MB_KMMLL0000QM-B503_300MHz_20110408.bct:flash_a01.bct \
    vendor/nvidia/tegra/odm/enterprise/nvflash/Enterprise_Samsung_768MB_KMMLL000QM-B503_400MHz_111114_sdmmc4_x8.cfg:bct_a02.cfg \
    vendor/nvidia/tegra/odm/enterprise/nvflash/Enterprise_Samsung_768MB_KMMLL000QM-B503_400MHz_111114_sdmmc4_x8.cfg:bct.cfg \
    vendor/nvidia/tegra/odm/enterprise/nvflash/E1205_Hynix_512MB_KMMLL0000QM-B503_300MHz_20110408.cfg:bct_a01.cfg \
    vendor/nvidia/tegra/odm/enterprise/nvflash/android_fastboot_microboot_emmc_full.cfg:flash.cfg
endif

PRODUCT_COPY_FILES += \
  $(LOCAL_PATH)/ueventd.tegra_enterprise.rc:root/ueventd.tegra_enterprise.rc


ifneq ($(NO_ROOT_DEVICE),1)
 PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/init.tegra_enterprise.rc:root/init.tegra_enterprise.rc \
    $(LOCAL_PATH)/fstab.tegra_enterprise:root/fstab.tegra_enterprise
endif

PRODUCT_PACKAGES += \
        sensors.tegra_enterprise \
        lights.tegra_enterprise

PRODUCT_COPY_FILES += \
   vendor/nvidia/tegra/3rdparty/broadcom/gps/bin/glgps_nvidiaTegra2android:system/bin/glgps_nvidiaTegra2android \
   vendor/nvidia/tegra/3rdparty/broadcom/gps/bin/gpsconfig-enterprise.xml:system/etc/gps/gpsconfig.xml \
   vendor/nvidia/tegra/3rdparty/broadcom/gps/bin/gps.tegra.so:system/lib/hw/gps.tegra.so
