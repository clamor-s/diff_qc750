# NVIDIA Tegra3 "Tai" development system

$(call inherit-product, device/nvidia/enterprise/device.mk)

PRODUCT_NAME := tai
PRODUCT_DEVICE := enterprise
PRODUCT_MODEL := Enterprise
PRODUCT_MANUFACTURER := NVIDIA

ifeq ($(wildcard vendor/nvidia/tegra/core-private),vendor/nvidia/tegra/core-private)
PRODUCT_COPY_FILES += \
 vendor/nvidia/tegra/customers/nvidia-partner/enterprise/nvflash/tai_android_fastboot_microboot_emmc_full.cfg:flash.cfg
else
PRODUCT_COPY_FILES += \
   vendor/nvidia/tegra/odm/enterprise/nvflash/tai_android_fastboot_microboot_emmc_full.cfg:flash.cfg
endif

PRODUCT_COPY_FILES += \
  $(LOCAL_PATH)/ueventd.tai.rc:root/ueventd.tai.rc

ifneq ($(NO_ROOT_DEVICE),1)
 PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/init.tai.rc:root/init.tai.rc \
    $(LOCAL_PATH)/fstab.tai:root/fstab.tai
endif

PRODUCT_PACKAGES += \
        sensors.tai \
        lights.tai
#WiFi
PRODUCT_PACKAGES += \
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
		wl128x-fw-4-plt.bin \
		wl18xx-fw-mc.bin \
		wl18xx-fw.bin \
		wl1271-nvs_wl8.bin

#BT & FM packages
PRODUCT_PACKAGES += \
		uim-sysfs \
		TIInit_10.6.15.bts \
		TIInit_11.8.32.bts \
		TIInit_12.8.32.bts \
		fmc_ch8_1283.2.bts \
		fm_rx_ch8_1283.2.bts \
		fm_tx_ch8_1283.2.bts

#GPS
PRODUCT_PACKAGES += \
                agnss_connect \
                client_app \
                devproxy \
                gps.tegra \
                libassist \
                libdevproxy \
                libgnssutils \
                supl20clientd \
                Supl20service \
                Connect_Config.txt \
                dproxy.conf \
                SuplConfig.spl \
                dproxy.patch \
                dproxy_rom.patch \
                libsupl10client \
                libsupl20client \
                libsupl20comon \
                libsupl20if \
                libsupl20oasn1comn \
                libsupl20oasn1lpp \
                libsupl20oasn1rrc \
                libsupl20oasn1rrlp \
                libsupl20oasn1supl1 \
                libsupl20oasn1supl2 \
                libsupl20oasn1tia
