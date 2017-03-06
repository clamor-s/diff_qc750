#!/system/bin/sh
if [ -z $(getprop ro.boot.commchip_id) ]; then
	setprop wifi.commchip_id 0
	echo "commchip_id is not set, so setting it to 4330"
else
	echo "setting user configured value of WiFi chipset"
	setprop wifi.commchip_id $(getprop ro.boot.commchip_id)
fi
