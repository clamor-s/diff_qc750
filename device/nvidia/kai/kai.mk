# NVIDIA Tegra3 "Kai" development system

$(call inherit-product, $(SRC_TARGET_DIR)/product/generic.mk)

ifeq ($(wildcard 3rdparty/google/gms-apps/products/gms.mk),3rdparty/google/gms-apps/products/gms.mk)
$(call inherit-product, 3rdparty/google/gms-apps/products/gms.mk)
endif

PRODUCT_NAME := kai
PRODUCT_DEVICE := kai
PRODUCT_MODEL := Kai
PRODUCT_MANUFACTURER := NVIDIA

PRODUCT_LOCALES += en_US

$(call inherit-product-if-exists, vendor/nvidia/tegra/core/nvidia-tegra-vendor.mk)
$(call inherit-product-if-exists, frameworks/base/data/videos/VideoPackage2.mk)
$(call inherit-product-if-exists, frameworks/base/data/sounds/AudioPackage3.mk)
$(call inherit-product, build/target/product/languages_full.mk)

PRODUCT_LOCALES += mdpi

ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/customers/nvidia-partner/kai/nvflash/Kai_Hynix_1GB_H5TC4G63MFR-H9A_667MHz_111231_debug_sdmmc4_x8.bct:flash.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/kai/nvflash/android_fastboot_microboot_emmc_full.cfg:flash.cfg \
    vendor/nvidia/tegra/customers/nvidia-partner/kai/nvflash/Kai_Hynix_1GB_H5TC4G63MFR-H9A_667MHz_111231_debug_sdmmc4_x8.cfg:bct.cfg \
    vendor/nvidia/tegra/customers/nvidia-partner/kai/nvflash/eks_nokey.dat:eks.dat \
    vendor/nvidia/tegra/customers/nvidia-partner/kai/partition_data/config/nvcamera.conf:system/etc/nvcamera.conf
else
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/odm/kai/nvflash/Kai_Hynix_1GB_H5TC4G63MFR-H9A_667MHz_111231_debug_sdmmc4_x8.bct:flash.bct \
    vendor/nvidia/tegra/odm/kai/nvflash/android_fastboot_microboot_emmc_full.cfg:flash.cfg \
    vendor/nvidia/tegra/odm/kai/nvflash/Kai_Hynix_1GB_H5TC4G63MFR-H9A_667MHz_111231_debug_sdmmc4_x8.cfg:bct.cfg \
    vendor/nvidia/tegra/odm/kai/nvflash/eks_nokey.dat:eks.dat \
    vendor/nvidia/tegra/odm/kai/partition_data/config/nvcamera.conf:system/etc/nvcamera.conf
endif

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
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
  $(LOCAL_PATH)/ueventd.kai.rc:root/ueventd.kai.rc \
  $(LOCAL_PATH)/tegra-kbc.kl:system/usr/keylayout/tegra-kbc.kl \
  $(LOCAL_PATH)/gpio-keys.kl:system/usr/keylayout/gpio-keys.kl \
  $(LOCAL_PATH)/dhcpcd.conf:system/etc/dhcpcd/dhcpcd.conf \
  $(LOCAL_PATH)/raydium_ts.idc:system/usr/idc/raydium_ts.idc \
  $(LOCAL_PATH)/sensor00fn11.idc:system/usr/idc/sensor00fn11.idc

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
    $(LOCAL_PATH)/init.kai.rc:root/init.kai.rc \
    $(LOCAL_PATH)/fstab.kai:root/fstab.kai \
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
	device/nvidia/kai/asound.conf:system/etc/asound.conf \
	device/nvidia/kai/nvaudio_conf.xml:system/etc/nvaudio_conf.xml \
	device/nvidia/enterprise/audioConfig_qvoice_icera_pc400.xml:system/etc/audioConfig_qvoice_icera_pc400.xml

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
	device/nvidia/kai/enctune.conf:system/etc/enctune.conf

# Stereo API permissions file has different locations in private and customer builds
ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/stereo/api/com.nvidia.nvstereoutils.xml:system/etc/permissions/com.nvidia.nvstereoutils.xml
else
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/prebuilt/kai/stereo/api/com.nvidia.nvstereoutils.xml:system/etc/permissions/com.nvidia.nvstereoutils.xml
endif

# Device-specific feature
# Balanced power mode
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/overlay/frameworks/base/data/etc/com.nvidia.balancedpower.xml:system/etc/permissions/com.nvidia.balancedpower.xml

# GPS configuration files have different location in customer build. In private build, these files are get coppied using Android.mk.
ifneq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
PRODUCT_COPY_FILES += \
	vendor/nvidia/tegra/prebuilt/kai/3rdparty/ti/gps/mcp/NaviLink/general/GPSCConfigFile.cfg:system/etc/gps/config/GPSCConfigFile.cfg \
	vendor/nvidia/tegra/prebuilt/kai/3rdparty/ti/gps/mcp/NaviLink/general/pathconfigfile.txt:system/etc/gps/config/pathconfigfile.txt \
	vendor/nvidia/tegra/prebuilt/kai/3rdparty/ti/gps/mcp/NaviLink/general/GpsConfigFile.txt:system/etc/gps/config/GpsConfigFile.txt \
	vendor/nvidia/tegra/prebuilt/kai/3rdparty/ti/gps/mcp/NaviLink/general/SuplConfig.spl:system/etc/gps/config/SuplConfig.spl \
	vendor/nvidia/tegra/prebuilt/kai/3rdparty/ti/gps/mcp/NaviLink/general/PeriodicConfFile.cfg:system/etc/gps/config/PeriodicConfFile.cfg \
	vendor/nvidia/tegra/prebuilt/kai/3rdparty/ti/gps/mcp/NaviLink/general/wpc.conf:system/etc/gps/config/wpc.conf \
	vendor/nvidia/tegra/prebuilt/kai/3rdparty/ti/gps/mcp/NaviLink/general/patch-X.0.ce:system/etc/gps/patch/patch-X.0.ce \
	vendor/nvidia/tegra/prebuilt/kai/3rdparty/ti/gps/mcp/NaviLink/general/inavconfigfile.txt:system/etc/gps/config/inavconfigfile.txt
endif

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
	sensors.kai \
	lights.kai \
	audio.primary.tegra \
	audio.a2dp.default \
	audio.usb.default \
	audio_policy.tegra \
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

# Nvidia baseband integration
PRODUCT_PACKAGES += nvidia_tegra_icera_common_modules \
					nvidia_tegra_icera_tablet_modules

#WiFi tools packages
PRODUCT_PACKAGES += \
		calibrator \
		TQS_S_2.6.ini \
		iw \
		crda \
		regulatory.bin \
		wpa_supplicant.conf \
		p2p_supplicant.conf \
		hostapd.conf \
		ibss_supplicant.conf \
		dhcpd.conf \
		dhcpcd.conf

#Wifi firmwares
PRODUCT_PACKAGES += \
		wl1271-nvs_default.bin \
		wl128x-fw-4-sr.bin \
		wl128x-fw-4-mr.bin \
		wl128x-fw-4-plt.bin

#BT & FM packages
PRODUCT_PACKAGES += \
		uim-sysfs \
		TIInit_10.6.15.bts \
		fmc_ch8_1283.2.bts \
		fm_rx_ch8_1283.2.bts \
		fm_tx_ch8_1283.2.bts

# NFC packages
PRODUCT_PACKAGES += \
		libnfc \
		libnfc_jni \
		Nfc \
		Tag
# GPS
PRODUCT_PACKAGES += \
		GPSCConfigFile.cfg \
		pathconfigfile.txt \
		GpsConfigFile.txt \
		SuplConfig.spl \
		PeriodicConfFile.cfg \
		wpc.conf \
		patch-X.0.ce \
		android.supl \
		gps.kai \
		navd \
		libgps \
		libgpsservices \
		libmcphalgps \
		libsuplhelperservicejni \
		libsupllocationprovider \
		SUPLClient

include frameworks/native/build/tablet-7in-hdpi-1024-dalvik-heap.mk
# we have enough storage space to hold precise GC data
PRODUCT_TAGS += dalvik.gc.type-precise

PRODUCT_CHARACTERISTICS := tablet

DEVICE_PACKAGE_OVERLAYS := $(LOCAL_PATH)/overlay

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp
