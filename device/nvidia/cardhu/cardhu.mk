# NVIDIA Tegra3 "Cardhu" development system

$(call inherit-product, $(SRC_TARGET_DIR)/product/generic.mk)

ifeq ($(wildcard 3rdparty/google/gms-apps/products/gms.mk),3rdparty/google/gms-apps/products/gms.mk)
$(call inherit-product, 3rdparty/google/gms-apps/products/gms.mk)
endif

PRODUCT_NAME := cardhu
PRODUCT_DEVICE := cardhu
PRODUCT_MODEL := Cardhu
PRODUCT_MANUFACTURER := NVIDIA

PRODUCT_LOCALES += en_US

$(call inherit-product-if-exists, vendor/nvidia/tegra/core/nvidia-tegra-vendor.mk)
$(call inherit-product-if-exists, frameworks/base/data/videos/VideoPackage2.mk)
$(call inherit-product-if-exists, frameworks/base/data/sounds/AudioPackage3.mk)
$(call inherit-product, build/target/product/languages_full.mk)

PRODUCT_LOCALES += mdpi hdpi xhdpi

ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/nvflash/E1186_Hynix_1GB_H5TC2G83BFR-PBA_375MHz_110622_sdmmc4_x8.bct:flash.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/nvflash/E1186_Hynix_1GB_H5TC2G83BFR-PBA_375MHz_110622_sdmmc4_x8.bct:flash_cardhu.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/nvflash/E1186_Hynix_1GB_H5TC2G83BFR-PBA_667MHz_111116_sdmmc4_x8.bct:flash_cardhu_t30l_hynix.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/nvflash/E1186_Samsung_2GB_K4B4G0846B-HYK0_375MHz_111122_sdmmc4_x8.bct:flash_cardhu_a05_hyk0.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/nvflash/E1198_Hynix_2GB_H5TC4G83MFR-PBA_375MHz_111122_sdmmc4_x8.bct:flash_cardhu_a05_hynix.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/nvflash/PM305_Samsung_1GB_KMKYL000VM-B603_400MHz_110809_sdmmc4_x8.bct:flash_pm305.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/nvflash/PM311_Hynix_1GB_H5TC2G83BFR-H9A_ddr3_667MHz_111121_sdmmc4_x8.bct:flash_pm311.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/nvflash/PM269_Elpida_1GB_EDB8132B2MA-1D-F_533MHz_110725_sdmmc4_x8.bct:flash_pm269.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/nvflash/PM269_Samsung_1GB_K4P8G304EB-FGC2_533MHz_110727_sdmmc4_x8.bct:flash_pm269_samsung.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/nvflash/E1186_Hynix_1GB_H5TC2G83BFR-PBA_375MHz_110622_sdmmc4_x8.cfg:bct.cfg \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/nvflash/E1186_Hynix_1GB_H5TC2G83BFR-PBA_375MHz_110622_sdmmc4_x8.cfg:bct_cardhu.cfg \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/nvflash/PM305_Samsung_1GB_KMKYL000VM-B603_400MHz_110809_sdmmc4_x8.cfg:bct_pm305.cfg \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/nvflash/PM269_Elpida_1GB_EDB8132B2MA-1D-F_533MHz_110725_sdmmc4_x8.cfg:bct_pm269.cfg \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/nvflash/eks_nokey.dat:eks.dat \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/partition_data/config/nvcamera.conf:system/etc/nvcamera.conf

ifeq ($(APPEND_DTB_TO_KERNEL), true)
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/nvflash/android_fastboot_microboot_emmc_full.cfg:flash.cfg
else
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/customers/nvidia-partner/cardhu/nvflash/android_fastboot_microboot_dtb_emmc_full.cfg:flash.cfg
endif
else
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/odm/cardhu/nvflash/E1186_Hynix_1GB_H5TC2G83BFR-PBA_375MHz_110622_sdmmc4_x8.bct:flash.bct \
    vendor/nvidia/tegra/odm/cardhu/nvflash/E1186_Hynix_1GB_H5TC2G83BFR-PBA_375MHz_110622_sdmmc4_x8.bct:flash_cardhu.bct \
    vendor/nvidia/tegra/odm/cardhu/nvflash/E1186_Hynix_1GB_H5TC2G83BFR-PBA_667MHz_111116_sdmmc4_x8.bct:flash_cardhu_t30l_hynix.bct \
    vendor/nvidia/tegra/odm/cardhu/nvflash/E1186_Samsung_2GB_K4B4G0846B-HYK0_375MHz_111122_sdmmc4_x8.bct:flash_cardhu_a05_hyk0.bct \
    vendor/nvidia/tegra/odm/cardhu/nvflash/E1198_Hynix_2GB_H5TC4G83MFR-PBA_375MHz_111122_sdmmc4_x8.bct:flash_cardhu_a05_hynix.bct \
    vendor/nvidia/tegra/odm/cardhu/nvflash/PM305_Samsung_1GB_KMKYL000VM-B603_400MHz_110809_sdmmc4_x8.bct:flash_pm305.bct \
    vendor/nvidia/tegra/odm/cardhu/nvflash/PM311_Hynix_1GB_H5TC2G83BFR-H9A_ddr3_667MHz_111121_sdmmc4_x8.bct:flash_pm311.bct \
    vendor/nvidia/tegra/odm/cardhu/nvflash/PM269_Elpida_1GB_EDB8132B2MA-1D-F_533MHz_110725_sdmmc4_x8.bct:flash_pm269.bct \
    vendor/nvidia/tegra/odm/cardhu/nvflash/PM269_Samsung_1GB_K4P8G304EB-FGC2_533MHz_110727_sdmmc4_x8.bct:flash_pm269_samsung.bct \
    vendor/nvidia/tegra/odm/cardhu/nvflash/android_fastboot_microboot_emmc_full.cfg:flash.cfg \
    vendor/nvidia/tegra/odm/cardhu/nvflash/E1186_Hynix_1GB_H5TC2G83BFR-PBA_375MHz_110622_sdmmc4_x8.cfg:bct.cfg \
    vendor/nvidia/tegra/odm/cardhu/nvflash/E1186_Hynix_1GB_H5TC2G83BFR-PBA_375MHz_110622_sdmmc4_x8.cfg:bct_cardhu.cfg \
    vendor/nvidia/tegra/odm/cardhu/nvflash/PM305_Samsung_1GB_KMKYL000VM-B603_400MHz_110809_sdmmc4_x8.cfg:bct_pm305.cfg \
    vendor/nvidia/tegra/odm/cardhu/nvflash/PM269_Elpida_1GB_EDB8132B2MA-1D-F_533MHz_110725_sdmmc4_x8.cfg:bct_pm269.cfg \
    vendor/nvidia/tegra/odm/cardhu/nvflash/eks_nokey.dat:eks.dat \
    vendor/nvidia/tegra/odm/cardhu/partition_data/config/nvcamera.conf:system/etc/nvcamera.conf
endif

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.nfc.xml:system/etc/permissions/android.hardware.nfc.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml

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
  $(LOCAL_PATH)/ueventd.cardhu.rc:root/ueventd.cardhu.rc \
  $(LOCAL_PATH)/tegra-kbc.kl:system/usr/keylayout/tegra-kbc.kl \
  $(LOCAL_PATH)/gpio-keys.kl:system/usr/keylayout/gpio-keys.kl \
  $(LOCAL_PATH)/atmel-maxtouch.idc:system/usr/idc/atmel-maxtouch.idc \
  $(LOCAL_PATH)/panjit_touch.idc:system/usr/idc/panjit_touch.idc \
  $(LOCAL_PATH)/raydium_ts.idc:system/usr/idc/raydium_ts.idc \
  $(LOCAL_PATH)/dhcpcd.conf:system/etc/dhcpcd/dhcpcd.conf \
  $(LOCAL_PATH)/../common/wifi_loader.sh:system/bin/wifi_loader.sh

ifeq ($(NV_ANDROID_FRAMEWORK_ENHANCEMENTS),TRUE)
PRODUCT_COPY_FILES += \
  $(LOCAL_PATH)/media_profiles.xml:system/etc/media_profiles.xml \
  $(LOCAL_PATH)/media_codecs.xml:system/etc/media_codecs.xml \
  $(LOCAL_PATH)/audio_policy.conf:system/etc/audio_policy.conf
else
PRODUCT_COPY_FILES += \
  $(LOCAL_PATH)/media_profiles_noenhance.xml:system/etc/media_profiles.xml \
  $(LOCAL_PATH)/media_codecs_noenhance.xml:system/etc/media_codecs.xml \
  $(LOCAL_PATH)/audio_policy_noenhance.conf:system/etc/audio_policy.conf
endif

ifeq ($(NO_ROOT_DEVICE),1)
  PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/init_no_root_device.rc:root/init.rc
else
  PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/init.cardhu.rc:root/init.cardhu.rc \
    $(LOCAL_PATH)/fstab.cardhu:root/fstab.cardhu \
    $(LOCAL_PATH)/../common/init.nv_dev_board.usb.rc:root/init.nv_dev_board.usb.rc
endif

# Face detection model
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/core/include/ft/model_frontalface.xml:system/etc/model_frontal.xml

# Test files
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/../common/cluster:system/bin/cluster \
    $(LOCAL_PATH)/../common/cluster_get.sh:system/bin/cluster_get.sh \
    $(LOCAL_PATH)/../common/cluster_set.sh:system/bin/cluster_set.sh \
    $(LOCAL_PATH)/../common/dcc:system/bin/dcc \
    $(LOCAL_PATH)/../common/hotplug:system/bin/hotplug \
    $(LOCAL_PATH)/../common/mount_debugfs.sh:system/bin/mount_debugfs.sh

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
	device/nvidia/common/bdaddr:system/etc/bluetooth/bdaddr \
	device/nvidia/cardhu/asound.conf:system/etc/asound.conf \
	device/nvidia/cardhu/nvaudio_conf.xml:system/etc/nvaudio_conf.xml \
	hardware/broadcom/wlan/bcmdhd/firmware/bcm4330/fw_bcm4330_abg.bin:system/vendor/firmware/bcm4330/fw_bcmdhd.bin \
	hardware/broadcom/wlan/bcmdhd/firmware/bcm4330/fw_bcm4330_apsta_bg.bin:system/vendor/firmware/bcm4330/fw_bcmdhd_apsta.bin \
	hardware/broadcom/wlan/bcmdhd/firmware/bcm4329/fw_bcm4329_abg.bin:system/vendor/firmware/bcm4329/fw_bcmdhd.bin \
	hardware/broadcom/wlan/bcmdhd/firmware/bcm4329/fw_bcm4329_apsta.bin:system/vendor/firmware/bcm4329/fw_bcmdhd_apsta.bin \
	hardware/broadcom/wlan/bcm4329/firmware/fw_bcm4329.bin:system/vendor/firmware/fw_bcm4329.bin \
	hardware/broadcom/wlan/bcm4329/firmware/fw_bcm4329_apsta.bin:system/vendor/firmware/fw_bcm4329_apsta.bin
#	hardware/broadcom/wlan/bcmdhd/firmware/bcm4329/fw_bcm4329_p2p.bin:system/vendor/firmware/bcm4329/fw_bcmdhd_p2p.bin \

# Configuration files for WiiMote support
PRODUCT_COPY_FILES += \
	vendor/nvidia/tegra/3rdparty/cwiid/wminput/configs/acc_ptr:system/etc/acc_ptr \
	vendor/nvidia/tegra/3rdparty/cwiid/wminput/configs/nunchuk_acc_ptr:system/etc/nunchuk_acc_ptr \
	vendor/nvidia/tegra/3rdparty/cwiid/wminput/configs/acc_led:system/etc/acc_led \
	vendor/nvidia/tegra/3rdparty/cwiid/wminput/configs/neverball:system/etc/neverball \
	vendor/nvidia/tegra/3rdparty/cwiid/wminput/configs/ir_ptr:system/etc/ir_ptr \
	vendor/nvidia/tegra/3rdparty/cwiid/wminput/configs/gamepad:system/etc/gamepad \
	vendor/nvidia/tegra/3rdparty/cwiid/wminput/configs/buttons:system/etc/buttons \
	vendor/nvidia/tegra/3rdparty/cwiid/wminput/configs/nunchuk_stick2btn:system/etc/nunchuk_stick2btn

PRODUCT_COPY_FILES += \
	device/nvidia/cardhu/enctune.conf:system/etc/enctune.conf

PRODUCT_COPY_FILES += \
   vendor/nvidia/tegra/3rdparty/broadcom/gps/bin/glgps_nvidiaTegra2android:system/bin/glgps_nvidiaTegra2android \
   vendor/nvidia/tegra/3rdparty/broadcom/gps/bin/gpsconfig-cardhu.xml:system/etc/gps/gpsconfig.xml \
   vendor/nvidia/tegra/3rdparty/broadcom/gps/bin/gps.tegra.so:system/lib/hw/gps.tegra.so

ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
PRODUCT_COPY_FILES += \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4329/bluetooth/bcmpatchram.hcd:system/etc/firmware/bcm4329.hcd \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4330/bluetooth/BCM4330B1_002.001.003.0379.0390.hcd:system/etc/firmware/bcm4330.hcd \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4329/wlan/nh930_nvram.txt:system/etc/nvram_4329.txt \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4330/wlan/NB099H.nvram_20110708.txt:system/etc/nvram_4330.txt \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4334/wlan/bcm94334wlagb_kk-steven.txt:system/etc/nvram_4334.txt \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4334/wlan/sdio-ag-pno-p2p-ccx-extsup-proptxstatus-dmatxrc-rxov-pktfilter-keepalive-aoe-vsdb-sr-wapi-wl11d.bin:system/vendor/firmware/bcm4334/fw_bcmdhd.bin \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm43241/bluetooth/AB113_BCM43241B0_0012_Azurewave_AW-AH691_TEST.HCD:system/etc/firmware/bcm43241.hcd \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm43241/wlan/AH691.NVRAM_20120914.txt:system/etc/nvram_43241.txt \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm43241/wlan/sdio-ag-pno-p2p-proptxstatus-dmatxrc-rxov-pktfilter-keepalive-aoe-vsdb-wapi-wl11d-sr-srvsdb-opt3-autoabn.bin:system/vendor/firmware/bcm43241/fw_bcmdhd.bin
else
PRODUCT_COPY_FILES += \
   vendor/nvidia/tegra/prebuilt/cardhu/3rdparty/bcmbinaries/bcm4329/bluetooth/bcmpatchram.hcd:system/etc/firmware/bcm4329.hcd \
   vendor/nvidia/tegra/prebuilt/cardhu/3rdparty/bcmbinaries/bcm4330/bluetooth/BCM4330B1_002.001.003.0379.0390.hcd:system/etc/firmware/bcm4330.hcd \
   vendor/nvidia/tegra/prebuilt/cardhu/3rdparty/bcmbinaries/bcm4329/wlan/nh930_nvram.txt:system/etc/nvram_4329.txt \
   vendor/nvidia/tegra/prebuilt/cardhu/3rdparty/bcmbinaries/bcm4330/wlan/NB099H.nvram_20110708.txt:system/etc/nvram_4330.txt \
   vendor/nvidia/tegra/prebuilt/cardhu/3rdparty/bcmbinaries/bcm4334/wlan/bcm94334wlagb_kk-steven.txt:system/etc/nvram_4334.txt \
   vendor/nvidia/tegra/prebuilt/cardhu/3rdparty/bcmbinaries/bcm4334/wlan/sdio-ag-pno-p2p-ccx-extsup-proptxstatus-dmatxrc-rxov-pktfilter-keepalive-aoe-vsdb-sr-wapi-wl11d.bin:system/vendor/firmware/bcm4334/fw_bcmdhd.bin \
   vendor/nvidia/tegra/prebuilt/cardhu/3rdparty/bcmbinaries/bcm43241/bluetooth/AB113_BCM43241B0_0012_Azurewave_AW-AH691_TEST.HCD:system/etc/firmware/bcm43241.hcd \
   vendor/nvidia/tegra/prebuilt/cardhu/3rdparty/bcmbinaries/bcm43241/wlan/AH691.NVRAM_20120914.txt:system/etc/nvram_43241.txt \
   vendor/nvidia/tegra/prebuilt/cardhu/3rdparty/bcmbinaries/bcm43241/wlan/sdio-ag-pno-p2p-proptxstatus-dmatxrc-rxov-pktfilter-keepalive-aoe-vsdb-wapi-wl11d-sr-srvsdb-opt3-autoabn.bin:system/vendor/firmware/bcm43241/fw_bcmdhd.bin
endif

# Stereo API permissions file has different locations in private and customer builds
ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/stereo/api/com.nvidia.nvstereoutils.xml:system/etc/permissions/com.nvidia.nvstereoutils.xml
else
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/prebuilt/cardhu/stereo/api/com.nvidia.nvstereoutils.xml:system/etc/permissions/com.nvidia.nvstereoutils.xml
endif

# Device-specific feature
# Balanced power mode
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/overlay/frameworks/base/data/etc/com.nvidia.balancedpower.xml:system/etc/permissions/com.nvidia.balancedpower.xml

#enable Widevine drm
PRODUCT_PROPERTY_OVERRIDES += drm.service.enabled=true
PRODUCT_PACKAGES += \
    com.google.widevine.software.drm.xml \
    com.google.widevine.software.drm \
    libdrmwvmplugin \
    libwvm \
    libWVStreamControlAPI_L1 \
    libwvdrm_L1

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
	sensors.cardhu \
	audio.primary.tegra \
	audio.a2dp.default \
	audio.usb.default \
	audio_policy.tegra \
	lights.cardhu \
	setup_fs \
	drmserver \
	Gallery2 \
	libdrmframework_jni

# WiiMote support
PRODUCT_PACKAGES += \
	libcwiid \
	wminput \
	acc \
	ir_ptr \
	led \
	nunchuk_acc \
	nunchuk_stick2btn

# Application to connect WiiMote with Tegra device
PRODUCT_PACKAGES += \
	WiiMote

# PlayStation 3 controller support
PRODUCT_PACKAGES += \
	libusb \
	sixpair

# Nvidia baseband integration
PRODUCT_PACKAGES += nvidia_tegra_icera_common_modules \
                    nvidia_tegra_icera_tablet_modules

# NFC packages
PRODUCT_PACKAGES += \
		libnfc \
		libnfc_jni \
		Nfc \
		Tag

include frameworks/native/build/tablet-10in-mdpi-dalvik-heap.mk

# we have enough storage space to hold precise GC data
PRODUCT_TAGS += dalvik.gc.type-precise

PRODUCT_CHARACTERISTICS := tablet

DEVICE_PACKAGE_OVERLAYS := $(LOCAL_PATH)/overlay

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp
