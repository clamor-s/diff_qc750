# NVIDIA Tegra2 "Whistler" development system

$(call inherit-product, $(SRC_TARGET_DIR)/product/generic.mk)

ifeq ($(wildcard 3rdparty/google/gms-apps/products/gms.mk),3rdparty/google/gms-apps/products/gms.mk)
$(call inherit-product, 3rdparty/google/gms-apps/products/gms.mk)
endif

PRODUCT_NAME := whistler
PRODUCT_DEVICE := whistler
PRODUCT_MODEL := Whistler
PRODUCT_MANUFACTURER := NVIDIA

PRODUCT_LOCALES += en_US

$(call inherit-product-if-exists, vendor/nvidia/tegra/core/nvidia-tegra-vendor.mk)
$(call inherit-product-if-exists, frameworks/base/data/videos/VideoPackage1.mk)
$(call inherit-product-if-exists, frameworks/base/data/sounds/AudioPackage3.mk)
$(call inherit-product, build/target/product/languages_full.mk)

PRODUCT_LOCALES += hdpi

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.camera.autofocus.xml:system/etc/permissions/android.hardware.camera.autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.xml

ifneq (,$(filter $(BOARD_INCLUDES_TEGRA_JNI),display))
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/hal/frameworks/Display/com.nvidia.display.xml:system/etc/permissions/com.nvidia.display.xml
endif

ifneq (,$(filter $(BOARD_INCLUDES_TEGRA_JNI),cursor))
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/hal/frameworks/Graphics/com.nvidia.graphics.xml:system/etc/permissions/com.nvidia.graphics.xml
endif

PRODUCT_COPY_FILES += \
  $(LOCAL_PATH)/vold.fstab:system/etc/vold.fstab \
  $(LOCAL_PATH)/init.whistler.rc:root/init.whistler.rc \
  $(LOCAL_PATH)/fstab.whistler:root/fstab.whistler \
  $(LOCAL_PATH)/../common/init.nv_dev_board.usb.rc:root/init.nv_dev_board.usb.rc \
  $(LOCAL_PATH)/ueventd.whistler.rc:root/ueventd.whistler.rc \
  $(LOCAL_PATH)/tegra-kbc.kl:system/usr/keylayout/tegra-kbc.kl \
  $(LOCAL_PATH)/media_profiles.xml:system/etc/media_profiles.xml \
  $(LOCAL_PATH)/synaptics-rmi-touchscreen.idc:system/usr/idc/synaptics-rmi-touchscreen.idc \
  $(LOCAL_PATH)/dhcpcd.conf:system/etc/dhcpcd/dhcpcd.conf

PRODUCT_COPY_FILES += \
    device/nvidia/common/bdaddr:system/etc/bluetooth/bdaddr \
    hardware/broadcom/wlan/bcmdhd/firmware/bcm4330/fw_bcm4330_abg.bin:system/vendor/firmware/bcm4330/fw_bcmdhd.bin \
    hardware/broadcom/wlan/bcmdhd/firmware/bcm4330/fw_bcm4330_apsta_bg.bin:system/vendor/firmware/bcm4330/fw_bcmdhd_apsta.bin \
    hardware/broadcom/wlan/bcmdhd/firmware/bcm4329/fw_bcm4329_abg.bin:system/vendor/firmware/bcm4329/fw_bcmdhd.bin \
    hardware/broadcom/wlan/bcmdhd/firmware/bcm4329/fw_bcm4329_apsta.bin:system/vendor/firmware/bcm4329/fw_bcmdhd_apsta.bin \
    vendor/nvidia/tegra/graphics-partner/android/build/egl.cfg:system/lib/egl/egl.cfg

PRODUCT_COPY_FILES += \
    external/alsa-lib/src/conf/alsa.conf:system/usr/share/alsa/alsa.conf \
    external/alsa-lib/src/conf/pcm/dsnoop.conf:system/usr/share/alsa/pcm/dsnoop.conf \
    external/alsa-lib/src/conf/pcm/modem.conf:system/usr/share/alsa/pcm/modem.conf \
    external/alsa-lib/src/conf/pcm/dpl.conf:system/usr/share/alsa/pcm/dpl.conf \
    external/alsa-lib/src/conf/pcm/default.conf:system/usr/share/alsa/pcm/default.conf \
    external/alsa-lib/src/conf/pcm/surround51.conf:system/usr/share/alsa/pcm/surround51.conf \
    external/alsa-lib/src/conf/pcm/surround41.conf:system/usr/share/alsa/pcm/surround41.conf \
    external/alsa-lib/src/conf/pcm/surround50.conf:system/usr/share/alsa/pcm/surround50.conf \
    external/alsa-lib/src/conf/pcm/dmix.conf:system/usr/share/alsa/pcm/dmix.conf \
    external/alsa-lib/src/conf/pcm/center_lfe.conf:system/usr/share/alsa/pcm/center_lfe.conf \
    external/alsa-lib/src/conf/pcm/surround40.conf:system/usr/share/alsa/pcm/surround40.conf \
    external/alsa-lib/src/conf/pcm/side.conf:system/usr/share/alsa/pcm/side.conf \
    external/alsa-lib/src/conf/pcm/iec958.conf:system/usr/share/alsa/pcm/iec958.conf \
    external/alsa-lib/src/conf/pcm/rear.conf:system/usr/share/alsa/pcm/rear.conf \
    external/alsa-lib/src/conf/pcm/surround71.conf:system/usr/share/alsa/pcm/surround71.conf \
    external/alsa-lib/src/conf/pcm/front.conf:system/usr/share/alsa/pcm/front.conf \
    external/alsa-lib/src/conf/cards/aliases.conf:system/usr/share/alsa/cards/aliases.conf \
    device/nvidia/whistler/asound.conf:system/etc/asound.conf \
    device/nvidia/whistler/nvaudio_conf.xml:system/etc/nvaudio_conf.xml

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    device/nvidia/whistler/enctune.conf:system/etc/enctune.conf

PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/3rdparty/broadcom/gps/bin/glgps_nvidiaTegra2android:system/bin/glgps_nvidiaTegra2android \
    vendor/nvidia/tegra/3rdparty/broadcom/gps/bin/gpsconfig-whistler.xml:system/etc/gps/gpsconfig.xml \
    vendor/nvidia/tegra/3rdparty/broadcom/gps/bin/gps.tegra.so:system/lib/hw/gps.tegra.so


# Default NVFlash boot config files.
ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4329/bluetooth/bcmpatchram.hcd:system/etc/firmware/bcm4329.hcd \
    vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4330/bluetooth/BCM4330B1_002.001.003.0379.0390.hcd:system/etc/firmware/bcm4330.hcd \
    vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4329/wlan/nh930_nvram.txt:system/etc/nvram_4329.txt \
    vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4330/wlan/NB099H.nvram_20110708.txt:system/etc/nvram_4330.txt \
    vendor/nvidia/tegra/customers/nvidia-partner/whistler/nvflash/whistler_a02p_a03_pop_12MHz_EDB4032B2PB6DF_300MHz_512MB_emmc_THGBM1G6D4EBAI4_x8.bct:flash.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/whistler/nvflash/whistler_a02p_a03_pop_12MHz_EDB4032B2PB6DF_300MHz_512MB_emmc_THGBM1G6D4EBAI4_x8.bct:flash_512MB.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/whistler/nvflash/whistler_a02p_pop_12MHz_EDB8132B1PB6DF_300Mhz_1GB_emmc_THGBM1G6D4EBAI4_x8.bct:flash_1GB.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/whistler/nvflash/whistler_a04_pop_12MHz_EDB8132B2PB8DF_380MHz_1GB_emmc_THGBM1G6D4EBAI4_x8.bct:flash_AP25_1GB.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/whistler/nvflash/whistler_a02p_a03_pop_12MHz_EDB4032B2PB6DF_300MHz_512MB_emmc_THGBM1G6D4EBAI4_x8.cfg:bct.cfg \
    vendor/nvidia/tegra/customers/nvidia-partner/whistler/nvflash/whistler_a02p_a03_pop_12MHz_EDB4032B2PB6DF_300MHz_512MB_emmc_THGBM1G6D4EBAI4_x8.cfg:bct_512MB.cfg \
    vendor/nvidia/tegra/customers/nvidia-partner/whistler/nvflash/whistler_a02p_pop_12MHz_EDB8132B1PB6DF_300Mhz_1GB_emmc_THGBM1G6D4EBAI4_x8.cfg:bct_1GB.cfg \
    vendor/nvidia/tegra/customers/nvidia-partner/whistler/nvflash/whistler_a04_pop_12MHz_EDB8132B2PB8DF_380MHz_1GB_emmc_THGBM1G6D4EBAI4_x8.cfg:bct_AP25_1GB.cfg \
    vendor/nvidia/tegra/customers/nvidia-partner/whistler/nvflash/eks_nokey.dat:eks.dat \
    vendor/nvidia/tegra/customers/nvidia-partner/whistler/partition_data/config/nvcamera.conf:system/etc/nvcamera.conf
ifeq ($(APPEND_DTB_TO_KERNEL), true)
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/customers/nvidia-partner/whistler/nvflash/android_fastboot_microboot_emmc_full.cfg:flash.cfg
else
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/customers/nvidia-partner/whistler/nvflash/android_fastboot_microboot_dtb_emmc_full.cfg:flash.cfg
endif
else
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/prebuilt/whistler/3rdparty/bcmbinaries/bcm4329/bluetooth/bcmpatchram.hcd:system/etc/firmware/bcm4329.hcd \
    vendor/nvidia/tegra/prebuilt/whistler/3rdparty/bcmbinaries/bcm4330/bluetooth/BCM4330B1_002.001.003.0379.0390.hcd:system/etc/firmware/bcm4330.hcd \
    vendor/nvidia/tegra/prebuilt/whistler/3rdparty/bcmbinaries/bcm4329/wlan/nh930_nvram.txt:system/etc/nvram_4329.txt \
    vendor/nvidia/tegra/prebuilt/whistler/3rdparty/bcmbinaries/bcm4330/wlan/NB099H.nvram_20110708.txt:system/etc/nvram_4330.txt \
    vendor/nvidia/tegra/odm/whistler/nvflash/whistler_a02p_a03_pop_12MHz_EDB4032B2PB6DF_300MHz_512MB_emmc_THGBM1G6D4EBAI4_x8.bct:flash.bct \
    vendor/nvidia/tegra/odm/whistler/nvflash/whistler_a02p_a03_pop_12MHz_EDB4032B2PB6DF_300MHz_512MB_emmc_THGBM1G6D4EBAI4_x8.bct:flash_512MB.bct \
    vendor/nvidia/tegra/odm/whistler/nvflash/whistler_a02p_pop_12MHz_EDB8132B1PB6DF_300Mhz_1GB_emmc_THGBM1G6D4EBAI4_x8.bct:flash_1GB.bct \
    vendor/nvidia/tegra/odm/whistler/nvflash/whistler_a04_pop_12MHz_EDB8132B2PB8DF_380MHz_1GB_emmc_THGBM1G6D4EBAI4_x8.bct:flash_AP25_1GB.bct \
    vendor/nvidia/tegra/odm/whistler/nvflash/whistler_a04_pop_12MHz_EDB8132B2PB8DF_380MHz_1GB_emmc_THGBM1G6D4EBAI4_x8.bct:flash_1GB.bct \
    vendor/nvidia/tegra/odm/whistler/nvflash/android_fastboot_microboot_emmc_full.cfg:flash.cfg \
    vendor/nvidia/tegra/odm/whistler/nvflash/whistler_a02p_a03_pop_12MHz_EDB4032B2PB6DF_300MHz_512MB_emmc_THGBM1G6D4EBAI4_x8.cfg:bct.cfg \
    vendor/nvidia/tegra/odm/whistler/nvflash/whistler_a02p_a03_pop_12MHz_EDB4032B2PB6DF_300MHz_512MB_emmc_THGBM1G6D4EBAI4_x8.cfg:bct_512MB.cfg \
    vendor/nvidia/tegra/odm/whistler/nvflash/whistler_a02p_pop_12MHz_EDB8132B1PB6DF_300Mhz_1GB_emmc_THGBM1G6D4EBAI4_x8.cfg:bct_1GB.cfg \
    vendor/nvidia/tegra/odm/whistler/nvflash/whistler_a04_pop_12MHz_EDB8132B2PB8DF_380MHz_1GB_emmc_THGBM1G6D4EBAI4_x8.cfg:bct_AP25_1GB.cfg \
    vendor/nvidia/tegra/odm/whistler/nvflash/eks_nokey.dat:eks.dat \
    vendor/nvidia/tegra/odm/whistler/partition_data/config/nvcamera.conf:system/etc/nvcamera.conf
endif

ifeq ($(wildcard vendor/nvidia/tegra/icera/tools/data/etc/apns-conf.xml),vendor/nvidia/tegra/icera/tools/data/etc/apns-conf.xml)
PRODUCT_COPY_FILES += \
   vendor/nvidia/tegra/icera/tools/data/etc/apns-conf.xml:system/etc/apns-conf.xml
endif

# Live Wallpapers
PRODUCT_PACKAGES += \
    LiveWallpapers \
    LiveWallpapersPicker \
    HoloSpiralWallpaper \
    MagicSmokeWallpapers \
    NoiseField \
    Galaxy4 \
    VisualizationWallpapers \
    PhaseBeam \
    librs_jni

PRODUCT_PACKAGES += \
    init.tf \
    setup_fs \
    sensors.whistler \
    Gallery2 \
    lights.whistler \
    tegra_alsa.tegra \
    audio.primary.tegra \
    audio.a2dp.default \
    audio_policy.tegra \
    drmserver \
    libdrmframework_jni

# Nvidia baseband integration
PRODUCT_PACKAGES += nvidia_tegra_icera_common_modules \
                    nvidia_tegra_icera_phone_modules

ifeq ($(wildcard vendor/nvidia/tegra/icera/firmware/binaries),vendor/nvidia/tegra/icera/firmware/binaries)
# Nvidia Icera Baseband firmware and default non device specific config files
NVIDIA_ICERA_VARIANT=e1219-eval
NVIDIA_ICERA_FW_SRC_PATH=vendor/nvidia/tegra/icera/firmware/binaries/binaries_$(NVIDIA_ICERA_VARIANT)

PRODUCT_COPY_FILES += $(NVIDIA_ICERA_FW_SRC_PATH)/secondary_boot.wrapped:system/vendor/firmware/app/secondary_boot.wrapped
PRODUCT_COPY_FILES += $(NVIDIA_ICERA_FW_SRC_PATH)/factory_tests.wrapped:system/vendor/firmware/app/factory_tests.wrapped
PRODUCT_COPY_FILES += $(NVIDIA_ICERA_FW_SRC_PATH)/tertiary_boot.wrapped:system/vendor/firmware/app/tertiary_boot.wrapped
PRODUCT_COPY_FILES += $(NVIDIA_ICERA_FW_SRC_PATH)/modem.wrapped:system/vendor/firmware/app/modem.wrapped

PRODUCT_COPY_FILES += $(NVIDIA_ICERA_FW_SRC_PATH)/audioConfig.bin:system/vendor/firmware/data/config/audioConfig.bin
PRODUCT_COPY_FILES += $(NVIDIA_ICERA_FW_SRC_PATH)/productConfig.bin:system/vendor/firmware/data/config/productConfig.bin
endif

# Face detection model
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/core/include/ft/model_frontalface.xml:system/etc/model_frontal.xml

#WiFi Direct Application
PRODUCT_PACKAGES += \
    WiFiDirectDemo

DEVICE_PACKAGE_OVERLAYS := $(LOCAL_PATH)/overlay

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp

# set memory configuration for 512 MB RAM based phone
include frameworks/native/build/phone-hdpi-512-dalvik-heap.mk
