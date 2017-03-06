# NVIDIA Tegra2 "Ventana" development system

$(call inherit-product, $(SRC_TARGET_DIR)/product/generic_no_telephony.mk)

ifeq ($(wildcard 3rdparty/google/gms-apps/products/gms.mk),3rdparty/google/gms-apps/products/gms.mk)
$(call inherit-product, 3rdparty/google/gms-apps/products/gms.mk)
endif

PRODUCT_NAME := ventana
PRODUCT_DEVICE := ventana
PRODUCT_MODEL := Ventana
PRODUCT_MANUFACTURER := NVIDIA

PRODUCT_LOCALES += en_US

$(call inherit-product-if-exists, vendor/nvidia/tegra/core/nvidia-tegra-vendor.mk)
$(call inherit-product-if-exists, frameworks/base/data/videos/VideoPackage1.mk)
$(call inherit-product-if-exists, frameworks/base/data/sounds/AudioPackage3.mk)
$(call inherit-product, build/target/product/languages_full.mk)

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
    frameworks/native/data/etc/android.software.sip.xml:system/etc/permissions/android.software.sip.xml

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
  $(LOCAL_PATH)/init.ventana.rc:root/init.ventana.rc \
  $(LOCAL_PATH)/fstab.ventana:root/fstab.ventana \
  $(LOCAL_PATH)/../common/init.nv_dev_board.usb.rc:root/init.nv_dev_board.usb.rc \
  $(LOCAL_PATH)/ueventd.ventana.rc:root/ueventd.ventana.rc \
  $(LOCAL_PATH)/gpio-keys.kl:system/usr/keylayout/gpio-keys.kl \
  $(LOCAL_PATH)/media_profiles.xml:system/etc/media_profiles.xml \
  $(LOCAL_PATH)/atmel-maxtouch.idc:system/usr/idc/atmel-maxtouch.idc \
  $(LOCAL_PATH)/panjit_touch.idc:system/usr/idc/panjit_touch.idc \
  $(LOCAL_PATH)/dhcpcd.conf:system/etc/dhcpcd/dhcpcd.conf

PRODUCT_COPY_FILES += \
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
	device/nvidia/ventana/asound.conf:system/etc/asound.conf \
	device/nvidia/ventana/nvaudio_conf.xml:system/etc/nvaudio_conf.xml \
	device/nvidia/common/bdaddr:system/etc/bluetooth/bdaddr \
	hardware/broadcom/wlan/bcmdhd/firmware/bcm4330/fw_bcm4330_abg.bin:system/vendor/firmware/bcm4330/fw_bcmdhd.bin \
	hardware/broadcom/wlan/bcmdhd/firmware/bcm4330/fw_bcm4330_apsta_bg.bin:system/vendor/firmware/bcm4330/fw_bcmdhd_apsta.bin \
	hardware/broadcom/wlan/bcmdhd/firmware/bcm4329/fw_bcm4329_abg.bin:system/vendor/firmware/bcm4329/fw_bcmdhd.bin \
	hardware/broadcom/wlan/bcmdhd/firmware/bcm4329/fw_bcm4329_apsta.bin:system/vendor/firmware/bcm4329/fw_bcmdhd_apsta.bin

PRODUCT_COPY_FILES += \
    device/nvidia/ventana/enctune.conf:system/etc/enctune.conf

PRODUCT_COPY_FILES += \
   vendor/nvidia/tegra/3rdparty/broadcom/gps/bin/glgps_nvidiaTegra2android:system/bin/glgps_nvidiaTegra2android \
   vendor/nvidia/tegra/3rdparty/broadcom/gps/bin/gpsconfig.xml:system/etc/gps/gpsconfig.xml \
   vendor/nvidia/tegra/3rdparty/broadcom/gps/bin/gps.tegra.so:system/lib/hw/gps.tegra.so

ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
PRODUCT_COPY_FILES += \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4329/bluetooth/bcmpatchram.hcd:system/etc/firmware/bcm4329.hcd \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4330/bluetooth/BCM4330B1_002.001.003.0379.0390.hcd:system/etc/firmware/bcm4330.hcd \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4329/wlan/nh930_nvram.txt:system/etc/nvram_4329.txt \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4330/wlan/NB099H.nvram_20110708.txt:system/etc/nvram_4330.txt \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4329/wlan/nh930_nvram.txt:system/etc/nvram.txt \
   vendor/nvidia/tegra/customers/nvidia-partner/ventana/partition_data/config/nvcamera.conf:system/etc/nvcamera.conf
else
PRODUCT_COPY_FILES += \
   vendor/nvidia/tegra/prebuilt/ventana/3rdparty/bcmbinaries/bcm4329/bluetooth/bcmpatchram.hcd:system/etc/firmware/bcm4329.hcd \
   vendor/nvidia/tegra/prebuilt/ventana/3rdparty/bcmbinaries/bcm4330/bluetooth/BCM4330B1_002.001.003.0379.0390.hcd:system/etc/firmware/bcm4330.hcd \
   vendor/nvidia/tegra/prebuilt/ventana/3rdparty/bcmbinaries/bcm4329/wlan/nh930_nvram.txt:system/etc/nvram_4329.txt \
   vendor/nvidia/tegra/prebuilt/ventana/3rdparty/bcmbinaries/bcm4330/wlan/NB099H.nvram_20110708.txt:system/etc/nvram_4330.txt \
   vendor/nvidia/tegra/prebuilt/ventana/3rdparty/bcmbinaries/bcm4329/wlan/nh930_nvram.txt:system/etc/nvram.txt \
   vendor/nvidia/tegra/odm/ventana/partition_data/config/nvcamera.conf:system/etc/nvcamera.conf
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
    sensors.ventana \
    lights.ventana \
    tegra_alsa.tegra \
    audio.primary.tegra \
    audio.a2dp.default \
    audio_policy.tegra \
    setup_fs \
    drmserver \
    Gallery2 \
    librs_jni \
    libdrmframework_jni

#WiFi Direct Application
PRODUCT_PACKAGES += \
	WiFiDirectDemo

include frameworks/native/build/tablet-dalvik-heap.mk

# we have enough storage space to hold precise GC data
PRODUCT_TAGS += dalvik.gc.type-precise

PRODUCT_CHARACTERISTICS := tablet

# Default NVFlash boot config files.
ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/customers/nvidia-partner/ventana/nvflash/ventana_A03_12MHz_EDB8132B1PB6DF_300Mhz_1GB_emmc_THGBM1G6D4EBAI4.bct:flash.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/ventana/nvflash/ventana_A03_12MHz_EDB8132B1PB6DF_300Mhz_1GB_emmc_THGBM1G6D4EBAI4.cfg:bct.cfg \
    vendor/nvidia/tegra/customers/nvidia-partner/ventana/nvflash/eks_nokey.dat:eks.dat
ifeq ($(APPEND_DTB_TO_KERNEL), true)
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/customers/nvidia-partner/ventana/nvflash/android_fastboot_microboot_emmc_full.cfg:flash.cfg
else
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/customers/nvidia-partner/ventana/nvflash/android_fastboot_microboot_dtb_emmc_full.cfg:flash.cfg
endif
else
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/odm/ventana/nvflash/ventana_A03_12MHz_EDB8132B1PB6DF_300Mhz_1GB_emmc_THGBM1G6D4EBAI4.bct:flash.bct \
    vendor/nvidia/tegra/odm/ventana/nvflash/android_fastboot_microboot_emmc_full.cfg:flash.cfg \
    vendor/nvidia/tegra/odm/ventana/nvflash/ventana_A03_12MHz_EDB8132B1PB6DF_300Mhz_1GB_emmc_THGBM1G6D4EBAI4.cfg:bct.cfg \
    vendor/nvidia/tegra/odm/ventana/nvflash/eks_nokey.dat:eks.dat
endif

# Face detection model
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/core/include/ft/model_frontalface.xml:system/etc/model_frontal.xml

DEVICE_PACKAGE_OVERLAYS := $(LOCAL_PATH)/overlay

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp
