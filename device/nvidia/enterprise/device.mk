# NVIDIA Tegra3 "Enterprise" development system

$(call inherit-product, $(SRC_TARGET_DIR)/product/generic.mk)

ifeq ($(wildcard 3rdparty/google/gms-apps/products/gms.mk),3rdparty/google/gms-apps/products/gms.mk)
$(call inherit-product, 3rdparty/google/gms-apps/products/gms.mk)
endif

$(call inherit-product-if-exists, vendor/nvidia/tegra/core/nvidia-tegra-vendor.mk)
$(call inherit-product-if-exists, frameworks/base/data/videos/VideoPackage2.mk)
$(call inherit-product-if-exists, frameworks/base/data/sounds/AudioPackage3.mk)
$(call inherit-product, build/target/product/languages_full.mk)

PRODUCT_LOCALES += hdpi

ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/customers/nvidia-partner/enterprise/nvflash/Enterprise_Samsung_1GB_KMKTS000VM-B604_533MHz_120307_sdmmc4_x8.bct:flash_a03.bct \
    vendor/nvidia/tegra/customers/nvidia-partner/enterprise/nvflash/Enterprise_Samsung_1GB_KMKTS000VM-B604_533MHz_120405_317_sdmmc4_x8.cfg:bct_a03.cfg \
    vendor/nvidia/tegra/customers/nvidia-partner/enterprise/nvflash/eks_nokey.dat:eks.dat \
    vendor/nvidia/tegra/customers/nvidia-partner/enterprise/partition_data/config/nvcamera.conf:system/etc/nvcamera.conf
else
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/odm/enterprise/nvflash/Enterprise_Samsung_1GB_KMKTS000VM-B604_533MHz_120307_sdmmc4_x8.bct:flash_a03.bct \
    vendor/nvidia/tegra/odm/enterprise/nvflash/Enterprise_Samsung_1GB_KMKTS000VM-B604_533MHz_120405_317_sdmmc4_x8.cfg:bct_a03.cfg \
    vendor/nvidia/tegra/odm/enterprise/nvflash/eks_nokey.dat:eks.dat \
    vendor/nvidia/tegra/odm/enterprise/partition_data/config/nvcamera.conf:system/etc/nvcamera.conf
endif

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.nfc.xml:system/etc/permissions/android.hardware.nfc.xml \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
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
  $(LOCAL_PATH)/tegra-kbc.kl:system/usr/keylayout/tegra-kbc.kl \
  $(LOCAL_PATH)/atmel-maxtouch.idc:system/usr/idc/atmel-maxtouch.idc \
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
	device/nvidia/enterprise/asound.conf:system/etc/asound.conf \
	device/nvidia/enterprise/nvaudio_conf.xml:system/etc/nvaudio_conf.xml \
        device/nvidia/enterprise/audioConfig_qvoice_icera_pc400.xml:system/etc/audioConfig_qvoice_icera_pc400.xml \
	hardware/broadcom/wlan/bcmdhd/firmware/bcm4330/fw_bcm4330_abg.bin:system/vendor/firmware/bcm4330/fw_bcmdhd.bin \
	hardware/broadcom/wlan/bcmdhd/firmware/bcm4330/fw_bcm4330_apsta_bg.bin:system/vendor/firmware/bcm4330/fw_bcmdhd_apsta.bin \
	hardware/broadcom/wlan/bcmdhd/firmware/bcm4329/fw_bcm4329_abg.bin:system/vendor/firmware/bcm4329/fw_bcmdhd.bin \
	hardware/broadcom/wlan/bcmdhd/firmware/bcm4329/fw_bcm4329_apsta.bin:system/vendor/firmware/bcm4329/fw_bcmdhd_apsta.bin \
	hardware/broadcom/wlan/bcm4329/firmware/fw_bcm4329.bin:system/vendor/firmware/fw_bcm4329.bin \
	hardware/broadcom/wlan/bcm4329/firmware/fw_bcm4329_apsta.bin:system/vendor/firmware/fw_bcm4329_apsta.bin

PRODUCT_COPY_FILES += \
	device/nvidia/enterprise/enctune.conf:system/etc/enctune.conf

ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
PRODUCT_COPY_FILES += \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4329/bluetooth/bcmpatchram.hcd:system/etc/firmware/bcm4329.hcd \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4330/bluetooth/BCM4330B1_002.001.003.0379.0390.hcd:system/etc/firmware/bcm4330.hcd \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4329/wlan/nh930_nvram.txt:system/etc/nvram_4329.txt \
   vendor/nvidia/tegra/3rdparty/bcmbinaries/bcm4330/wlan/NB099H.nvram_20110708.txt:system/etc/nvram_4330.txt
else
PRODUCT_COPY_FILES += \
   vendor/nvidia/tegra/prebuilt/enterprise/3rdparty/bcmbinaries/bcm4329/bluetooth/bcmpatchram.hcd:system/etc/firmware/bcm4329.hcd \
   vendor/nvidia/tegra/prebuilt/enterprise/3rdparty/bcmbinaries/bcm4330/bluetooth/BCM4330B1_002.001.003.0379.0390.hcd:system/etc/firmware/bcm4330.hcd \
   vendor/nvidia/tegra/prebuilt/enterprise/3rdparty/bcmbinaries/bcm4329/wlan/nh930_nvram.txt:system/etc/nvram_4329.txt \
   vendor/nvidia/tegra/prebuilt/enterprise/3rdparty/bcmbinaries/bcm4330/wlan/NB099H.nvram_20110708.txt:system/etc/nvram_4330.txt
endif

# Stereo API permissions file has different locations in private and customer builds
ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/stereo/api/com.nvidia.nvstereoutils.xml:system/etc/permissions/com.nvidia.nvstereoutils.xml
else
PRODUCT_COPY_FILES += \
    vendor/nvidia/tegra/prebuilt/enterprise/stereo/api/com.nvidia.nvstereoutils.xml:system/etc/permissions/com.nvidia.nvstereoutils.xml
endif

DEVICE_PACKAGE_OVERLAYS := $(LOCAL_PATH)/overlay

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
	setup_fs \
	audio.primary.tegra \
	audio.a2dp.default \
	audio.usb.default \
	audio_policy.tegra \
	Gallery2 \
	drmserver \
	libdrmframework_jni

# Nvidia baseband integration
PRODUCT_PACKAGES += nvidia_tegra_icera_common_modules \
                    nvidia_tegra_icera_phone_modules \
                    brcmgpslcsapi \
                    libagps-brcm4751

# NFC packages
PRODUCT_PACKAGES += \
                    libnfc \
                    libnfc_jni \
                    Nfc \
                    Tag

# Assuming Enterprise has 1024MB of memory
include frameworks/native/build/phone-hdpi-1024-dalvik-heap.mk

ifeq ($(wildcard vendor/nvidia/tegra/icera/firmware/binaries),vendor/nvidia/tegra/icera/firmware/binaries)
# Nvidia Icera Baseband firmware and default non device specific config files
NVIDIA_ICERA_VARIANT=e1205-eval
NVIDIA_ICERA_FW_SRC_PATH=vendor/nvidia/tegra/icera/firmware/binaries/binaries_$(NVIDIA_ICERA_VARIANT)

PRODUCT_COPY_FILES += $(NVIDIA_ICERA_FW_SRC_PATH)/secondary_boot.wrapped:system/vendor/firmware/app/secondary_boot.wrapped
PRODUCT_COPY_FILES += $(NVIDIA_ICERA_FW_SRC_PATH)/factory_tests.wrapped:system/vendor/firmware/app/factory_tests.wrapped
PRODUCT_COPY_FILES += $(NVIDIA_ICERA_FW_SRC_PATH)/tertiary_boot.wrapped:system/vendor/firmware/app/tertiary_boot.wrapped
PRODUCT_COPY_FILES += $(NVIDIA_ICERA_FW_SRC_PATH)/modem.wrapped:system/vendor/firmware/app/modem.wrapped
PRODUCT_COPY_FILES += $(NVIDIA_ICERA_FW_SRC_PATH)/audioConfig.bin:system/vendor/firmware/data/config/audioConfig.bin
PRODUCT_COPY_FILES += $(NVIDIA_ICERA_FW_SRC_PATH)/productConfig.bin:system/vendor/firmware/data/config/productConfig.bin
endif

ifeq ($(wildcard vendor/nvidia/tegra/icera/tools/data/etc/apns-conf.xml),vendor/nvidia/tegra/icera/tools/data/etc/apns-conf.xml)
PRODUCT_COPY_FILES += vendor/nvidia/tegra/icera/tools/data/etc/apns-conf.xml:system/etc/apns-conf.xml
endif

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp
