#!/system/bin/sh

echo "$(getprop persist.keenhi.country)"
iw reg set $(getprop persist.keenhi.country)