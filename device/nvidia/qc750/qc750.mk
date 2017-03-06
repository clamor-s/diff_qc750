# NVIDIA Tegra3 "Kai" development system

$(call inherit-product, $(SRC_TARGET_DIR)/product/generic_no_telephony_for_n710.mk)

ifeq ($(wildcard 3rdparty/google/gms-apps/products/gms.mk),3rdparty/google/gms-apps/products/gms.mk)
$(call inherit-product, 3rdparty/google/gms-apps/products/gms.mk)
endif

PRODUCT_NAME := qc750
PRODUCT_DEVICE := qc750
PRODUCT_MODEL := WEXLER-TAB-7T
PRODUCT_MANUFACTURER := WEXLER
PRODUCT_BRAND := WEXLER

PRODUCT_LOCALES += ru_RU

$(call inherit-product-if-exists, vendor/nvidia/tegra/core/nvidia-tegra-vendor.mk)
$(call inherit-product-if-exists, frameworks/base/data/videos/VideoPackage2.mk)
$(call inherit-product-if-exists, frameworks/base/data/sounds/AudioPackage3.mk)
$(call inherit-product, build/target/product/locales_full.mk)
$(call inherit-product-if-exists, external/svox/pico/lang/all_pico_languages.mk)

PRODUCT_LOCALES += hdpi en_US

ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/customers/nvidia-partner/qc750/nvflash/Kai_Hynix_1GB_H5TC4G63MFR-H9A_667MHz_111231_debug_sdmmc4_x8.bct:flash.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/qc750/nvflash/android_fastboot_emmc_full.cfg:flash.cfg \
    vendor/nvidia/tegra/customers/nvidia-partner/qc750/nvflash/eks_nokey.dat:eks.dat \
    vendor/nvidia/tegra/customers/nvidia-partner/qc750/partition_data/config/nvcamera.conf:system/etc/nvcamera.conf
else
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/odm/qc750/nvflash/Kai_Hynix_1GB_H5TC4G63MFR-H9A_667MHz_111231_debug_sdmmc4_x8.bct:flash.bct \
    vendor/nvidia/tegra/odm/qc750/nvflash/android_fastboot_emmc_full.cfg:flash.cfg \
    vendor/nvidia/tegra/odm/qc750/nvflash/eks_nokey.dat:eks.dat \
    vendor/nvidia/tegra/odm/qc750/partition_data/config/nvcamera.conf:system/etc/nvcamera.conf
endif

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/qc750_feature.xml:system/etc/permissions/tablet_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
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
  $(LOCAL_PATH)/ueventd.qc750.rc:root/ueventd.qc750.rc \
  $(LOCAL_PATH)/tegra-kbc.kl:system/usr/keylayout/tegra-kbc.kl \
  $(LOCAL_PATH)/gpio-keys.kl:system/usr/keylayout/gpio-keys.kl \
  $(LOCAL_PATH)/dhcpcd.conf:system/etc/dhcpcd/dhcpcd.conf \
  $(LOCAL_PATH)/raydium_ts.idc:system/usr/idc/raydium_ts.idc \
  $(LOCAL_PATH)/sensor00fn11.idc:system/usr/idc/sensor00fn11.idc \
  $(LOCAL_PATH)/sis_touch.idc:system/usr/idc/sis_touch.idc  \
  $(LOCAL_PATH)/ft5x0x_ts.idc:system/usr/idc/ft5x0x_ts.idc \
  $(LOCAL_PATH)/sd8787_uapsta.bin:system/etc/firmware/mrvl/sd8787_uapsta.bin \
  $(LOCAL_PATH)/nt1103-ts.idc:system/usr/idc/nt1103-ts.idc

ifeq ($(NV_ANDROID_FRAMEWORK_ENHANCEMENTS),TRUE)
PRODUCT_COPY_FILES += \
  $(LOCAL_PATH)/media_profiles.xml:system/etc/media_profiles.xml
else
PRODUCT_COPY_FILES += \
  $(LOCAL_PATH)/media_profiles_noenhance.xml:system/etc/media_profiles.xml
endif

PRODUCT_COPY_FILES += \
    device/nvidia/qc750/media_codecs.xml:system/etc/media_codecs.xml

ifeq ($(NO_ROOT_DEVICE),1)
  PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/init_no_root_device.rc:root/init.rc
else
  PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/init.qc750.rc:root/init.qc750.rc \
    $(LOCAL_PATH)/fstab.qc750:root/fstab.qc750 \
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
    
#add by vin for usi_3g_module
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/usiuna-ril/usiuna-ril.so:system/lib/usiuna-ril.so \
    $(LOCAL_PATH)/usiuna-ril/ppp/chat:system/bin/chat \
    $(LOCAL_PATH)/usiuna-ril/ppp/call-pppd:system/etc/ppp/call-pppd \
    $(LOCAL_PATH)/usiuna-ril/ppp/ip-down:system/etc/ppp/ip-down \
    $(LOCAL_PATH)/usiuna-ril/ppp/ip-up:system/etc/ppp/ip-up \
    $(LOCAL_PATH)/usiuna-ril/ppp/ip-up-vpn:system/etc/ppp/ip-up-vpn \
    $(LOCAL_PATH)/usiuna-ril/apns-conf_sdk.xml:system/etc/apns-conf.xml

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
	device/nvidia/qc750/asound.conf:system/etc/asound.conf \
	device/nvidia/qc750/nvaudio_conf.xml:system/etc/nvaudio_conf.xml \
	device/nvidia/qc750/audio_policy.conf:system/etc/audio_policy.conf

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
	device/nvidia/qc750/enctune.conf:system/etc/enctune.conf	
	
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/ti/wifi_cal_conv_mac.sh:system/bin/wifi_cal_conv_mac.sh \
	$(LOCAL_PATH)/ti/convert_mac.sh:system/bin/convert_mac.sh \
	$(LOCAL_PATH)/ti/busybox:system/bin/busybox \
	$(LOCAL_PATH)/ti/hciconfig:system/bin/hciconfig \
	$(LOCAL_PATH)/ti/wifi_address.sh:system/bin/wifi_address.sh \
	$(LOCAL_PATH)/ti/wifi_channel.sh:system/bin/wifi_channel.sh \
	$(LOCAL_PATH)/ti/udp_port_any.sh:system/bin/udp_port_any.sh

# Stereo API permissions file has different locations in private and customer builds
ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/stereo/api/com.nvidia.nvstereoutils.xml:system/etc/permissions/com.nvidia.nvstereoutils.xml
else
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/prebuilt/qc750/stereo/api/com.nvidia.nvstereoutils.xml:system/etc/permissions/com.nvidia.nvstereoutils.xml
endif

# Device-specific feature
# Balanced power mode
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/overlay/frameworks/base/data/etc/com.nvidia.balancedpower.xml:system/etc/permissions/com.nvidia.balancedpower.xml

PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/general/auto_hslp_keystore.bks:system/etc/gps/cert/auto_hslp_keystore.bks \
	$(LOCAL_PATH)/general/com.google.android.maps.jar:system/framework/com.google.android.maps.jar \
	$(LOCAL_PATH)/general/com.google.android.maps.xml:system/framework/com.google.android.maps.xml \
	$(LOCAL_PATH)/general/com.google.android.maps.xml:system/etc/permissions/com.google.android.maps.xml \
	$(LOCAL_PATH)/general/GPSCConfigFile.cfg:system/etc/gps/config/GPSCConfigFile.cfg \
	$(LOCAL_PATH)/general/GpsConfigFile.txt:system/etc/gps/config/GpsConfigFile.txt \
	$(LOCAL_PATH)/general/inavconfigfile.txt:system/etc/gps/config/inavconfigfile.txt \
	$(LOCAL_PATH)/general/patch-X.0.ce:system/etc/gps/patch/patch-X.0.ce \
	$(LOCAL_PATH)/general/pathconfigfile.txt:system/etc/gps/config/pathconfigfile.txt \
	$(LOCAL_PATH)/general/PeriodicConfFile.cfg:system/etc/gps/config/PeriodicConfFile.cfg \
	$(LOCAL_PATH)/general/SuplConfig.spl:system/etc/gps/config/SuplConfig.spl \
	$(LOCAL_PATH)/general/navd:system/bin/navd \
	$(LOCAL_PATH)/general/gps.kai.so:system/lib/hw/gps.tegra.so
	
#enable Widevine drm
PRODUCT_PROPERTY_OVERRIDES += drm.service.enabled=true
PRODUCT_PACKAGES += \
    com.google.widevine.software.drm.xml \
    com.google.widevine.software.drm \
    libdrmwvmplugin \
    libwvm \
    libdrmdecrypt \
    libWVStreamControlAPI_L3 \
    libwvdrm_L3
    
    #libWVStreamControlAPI_L1 \
    #libwvdrm_L1

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
	sensors.qc750 \
	lights.qc750 \
	audio.primary.tegra \
	audio.a2dp.default \
	audio.usb.default \
	audio_policy.tegra \
	make_ext4fs \
	setup_fs \
	drmserver \
	Gallery2 \
	libdrmframework_jni	\
	akmd8975_service 

# Application to connect WiiMote with Tegra device
#PRODUCT_PACKAGES += \
#	WiiMote

# Nvidia baseband integration
#PRODUCT_PACKAGES += nvidia_tegra_icera_common_modules \
#                    DatacallWhitelister

#WiFi Direct Application
#PRODUCT_PACKAGES += \
#	WiFiDirectDemo

PRODUCT_PACKAGES += \
	nabi2_core \
	libpagemap \
	procrank \
	procmem \
	bootUpdate


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
		dhcpcd.conf \
		bdt

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
		
# GPS
PRODUCT_PACKAGES += \
		libgps \
		libgpsservices \
		libmcphalgps \
		libsuplhelperservicejni \
		libsupllocationprovider

# add by vin for usi_3g_module
PRODUCT_PACKAGES += \
	rild

include frameworks/native/build/tablet-dalvik-heap.mk
# we have enough storage space to hold precise GC data
PRODUCT_TAGS += dalvik.gc.type-precise

PRODUCT_CHARACTERISTICS := tablet

DEVICE_PACKAGE_OVERLAYS := $(LOCAL_PATH)/overlay

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp
#nabi2_demo only
KH_ADD_NVIDIA_KAI_DEMO := false

#nabi2 only
include device/keenhi/addon/addon-nvidia.mk
include device/keenhi/addon/addon-qc750.mk

local_KEENHI_OTA_KEY := device/nvidia/keenhi/qc750_keys/releasekey
#local_KEENHI_OTA_KEY := build/target/product/security/testkey
PRODUCT_OTA_PUBLIC_KEYS := build/target/product/security/testkey.x509.pem
