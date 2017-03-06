#!/system/bin/sh

TARGET_NVS_FILE=/data/misc/wifi/wl1271-nvs.bin
BACK_NVS_FILE=/forever/misc/wifi/wl1271-nvs2.bin
CYBTT10_MACADDRESS_PATH=/forever/mac/wifi/macaddress
TARGET_FW_MAC_FILE=/forever/misc/wifi/mac_address
NVS_SIZE=1113

#if busybox not exits, end of processes.
if [ ! -f /system/bin/busybox ]; then
    echo "ERROR: Please install busybox !!"
    exit 
fi

if [ ! -f $CYBTT10_MACADDRESS_PATH ]; then
    echo "ERROR: no $CYBTT10_MACADDRESS_PATH !!"
    exit 
fi

MAC=$(cat ${CYBTT10_MACADDRESS_PATH} | busybox awk -F' ' '{ print $1 }')
LEN=0
MAC_INC=1

#check mac address length
LEN=${#MAC}
if [ $LEN -gt 12 ]; #12bits
then
    echo "LEN=$LEN"
    echo "ERROR MAC ADDRESS!!!"
    exit
fi

echo "READ MAC: $MAC"

echo "*************************************"
M1=$(echo "$MAC" | busybox awk '{ print substr($1,1,2) }')
M2=$(echo "$MAC" | busybox awk '{ print substr($1,3,2) }')
M3=$(echo "$MAC" | busybox awk '{ print substr($1,5,2) }')
M4=$(echo "$MAC" | busybox awk '{ print substr($1,7,2) }')
M5=$(echo "$MAC" | busybox awk '{ print substr($1,9,2) }')
M6=$(echo "$MAC" | busybox awk '{ print substr($1,11,2) }')
echo "*************************************"

MACADDRESS="$M1:$M2:$M3:$M4:$M5:$M6"
echo "WIFI MAC: $MACADDRESS"
echo $MACADDRESS > $TARGET_FW_MAC_FILE

calibrator set nvs_mac ${TARGET_NVS_FILE} $MACADDRESS
REAL_SIZE=`busybox stat ${TARGET_NVS_FILE} | busybox grep Size | busybox awk '{print $2}'`
echo "REAL_SIZE is $REAL_SIZE"
if [ ${REAL_SIZE} -eq $NVS_SIZE ]; then
	CURMAC=$(calibrator get nvs_mac ${TARGET_NVS_FILE} | busybox grep 'MAC addr from NVS:' | busybox sed 's/^.*NVS://g')
	CURMAC=$(echo "$CURMAC" | busybox awk -F' ' '{ print $1 }')
	echo "CURMAC IS $CURMAC"

	if [ $CURMAC == $MACADDRESS ]; then
    echo "cur mac == file mac"
		dd if=${TARGET_NVS_FILE} of=${BACK_NVS_FILE}
		REAL_SIZE=`busybox stat ${BACK_NVS_FILE} | busybox grep Size | busybox awk '{print $2}'`
		echo "REAL_SIZE 2 is $REAL_SIZE"
		if [ ${REAL_SIZE} -eq $NVS_SIZE ]; then
			CURMAC=$(calibrator get nvs_mac ${BACK_NVS_FILE} | busybox grep 'MAC addr from NVS:' | busybox sed 's/^.*NVS://g')
			CURMAC=$(echo "$CURMAC" | busybox awk -F' ' '{ print $1 }')
			if [ $CURMAC == $MACADDRESS ]; then
				echo "copy backup file ok"
			else
				echo "copy backup file failed!"
				rm ${BACK_NVS_FILE}
			fi
		else
			echo "copy backup file failed!"
			rm ${BACK_NVS_FILE}
		fi
	else
    echo "cur mac != file mac"
	fi
fi