#!/system/bin/sh

######################################
#
# Convert MAC address from BT to WLAN 
#
# NOTE: It is only fit for WG75xx
#
######################################

TARGET_FW_DIR=/system/etc/firmware/ti-connectivity
TARGET_INI_FILE=${TARGET_FW_DIR}/TQS_S_2.6.ini

TARGET_NVS_DIR=/data/misc/wifi
TARGET_NVS_FILE=${TARGET_FW_DIR}/wl1271-nvs.bin

TARGET_FW_MAC_FILE=/data/misc/wifi/mac_address.txt
WL12XX_MODULE_PATH=/system/lib/modules/compat

TARGET_FW_MAC_FILE=/data/misc/wifi/mac_address.txt

#if busybox not exits, end of processes.
if [ ! -f /system/bin/busybox ]; then
    echo "ERROR: Please install busybox !!"
    exit 
fi

#if mac_address.txt exist, remove it.
if [ -f $TARGET_FW_MAC_FILE ]; then 
    rm -r $TARGET_FW_MAC_FILE
    busybox touch $TARGET_FW_MAC_FILE
else
    busybox touch $TARGET_FW_MAC_FILE
fi

#enable BT and get bt mac
hciconfig hci0 up
echo "[Start getting BT Address]"
sleep 1
MAC=$(hciconfig hci0 | busybox grep 'BD Address'| busybox sed 's/^.*BD Address://g'| busybox sed 's/ ACL.*$//g')
#echo "[BD MAC : $MAC]"
sleep 1
MAC=$(echo "$MAC" | busybox awk -F' ' '{ print $1 }')
#echo "[BD MAC : $MAC]"
hciconfig hci0 down
LEN=0
MAC_INC=1

#check mac address length
LEN=${#MAC}
if [ $LEN -gt 17 ]; #17bits
then
    echo "LEN=$LEN"
    echo "ERROR MAC ADDRESS!!!"
    exit
fi

echo "BT MAC: $MAC"

echo "*************************************"
echo "Processing MAC for WIFI"
M1=$(echo "$MAC" | busybox awk -F: '{ print $1 }')
M2=$(echo "$MAC" | busybox awk -F: '{ print $2 }')
M3=$(echo "$MAC" | busybox awk -F: '{ print $3 }')
M4=$(echo "$MAC" | busybox awk -F: '{ print $4 }')
M5=$(echo "$MAC" | busybox awk -F: '{ print $5 }')
M6=$(echo "$MAC" | busybox awk -F: '{ print $6 }') 

echo "*************************************"


echo "Convert start..."
#************************************************************************
D6=$(busybox printf "%d\n" 0x$M6)
#D6=$(echo "ibase=16; $M6"| bc) #To convert to decimal, set ibase to 16
C6=$(( D6+$MAC_INC )) #+1
if [ $C6 -gt 255 ];
then
    echo "M5 need added 1"
    M6=00 #$(echo "obase=16; 00"|bc) #To convert to hexadecimal, set obase to 16

    #************************************************************************
    D5=$(busybox printf "%d\n" 0x$M5)
    #D5=$(echo "ibase=16; $M5"|bc) #To convert to decimal, set ibase to 16
    C5=$(( D5+$MAC_INC ))
    if [ $C5 -gt 255 ];
    then
        echo " M4 need added 1"
        M5=00 #$(echo "obase=16; 00"|bc) #To convert to hexadecimal, set obase to 16
        #echo "M5=$M5"

        #************************************************************************
        D4=$(busybox printf "%d\n" 0x$M4)	
        #D4=$(echo "ibase=16; $M4"|bc) #To convert to decimal, set ibase to 16
        C4=$(( D4+$MAC_INC ))
        if [ $C4 -gt 255 ];
        then
            echo " M3 need added 1"
            M4=00 #$(echo "obase=16; 00"|bc) #To convert to hexadecimal, set obase to 16
            #echo "M4=$M4"

            #************************************************************************
            D3=$(busybox printf "%d\n" 0x$M3)
            #D3=$(echo "ibase=16; $M3"|bc) #To convert to decimal, set ibase to 16
            C3=$(( D3+$MAC_INC ))
            if [ $C3 -gt 255 ];
            then
                echo " M2 need added 1"
                M3=00 #$(echo "obase=16; 00"|bc) #To convert to hexadecimal, set obase to 16
                #echo "M3=$M3"

                #************************************************************************
                D2=$(busybox printf "%d\n" 0x$M2)
                #D2=$(echo "ibase=16; $M2"|bc) #To convert to decimal, set ibase to 16
                C2=$(( D2+$MAC_INC ))
                if [ $C2 -gt 255 ];
                then
                    echo " M1 need added 1"
                    M2=00 #$(echo "obase=16; 00"|bc) #To convert to hexadecimal, set obase to 16
                    #echo "M2=$M2"

                    #************************************************************************
                    D1=$(busybox printf "%d\n" 0x$M1)
                    #D1=$(echo "ibase=16; $M1"|bc) #To convert to decimal, set ibase to 16
                    C1=$(( D1+$MAC_INC ))
                    if [ $C1 -gt 255 ];
                    then
                        M1=00 #$(echo "obase=16; 00"|bc) #To convert to hexadecimal, set obase to 16
                        #echo "M1=$M1"
                        #***************************END******************************************

                        MACADDRESS="$M1:$M2:$M3:$M4:$M5:$M6"
                        echo $MACADDRESS #print MACADDRESS
                        echo "********ERROR MAC ADDRESS**************"
                    else
                        #D1=$C1
                        #M1=$(echo "obase=16; $D1"|bc)
                        M1=$(busybox printf "%X\n" $C1)
                        if [ $C1 -le 15 ];
                        then
                            M1="0$M1"
                        fi
                        #echo "M1=$M1"
                    fi
                else
                    #D2=$C2
                    #M2=$(echo "obase=16; $D2"|bc)
                    M2=$(busybox printf "%X\n" $C2)
                    if [ $C2 -le 15 ];
                    then
                        M2="0$M2"
                    fi
                    #echo "M2=$M2"
                fi

            else
                #D3=$C3
                #M3=$(echo "obase=16; $D3"|bc)
                M3=$(busybox printf "%X\n" $C3)
                if [ $C3 -le 15 ];
                then
                    M3="0$M3"
                fi
                #echo "M3=$M3"
            fi
        else
            #D4=$C4
            #M4=$(echo "obase=16; $D4"|bc)
            M4=$(busybox printf "%X\n" $C4)
            if [ $C4 -le 15 ];
            then
                M4="0$M4"
            fi
            #echo "M4=$M4"
        fi
    else
        #D5=$C5
        #M5=$(echo "obase=16; $D5"|bc)
        M5=$(busybox printf "%X\n" $C5)
        if [ $C5 -le 15 ];
        then
            M5="0$M5"
        fi
        #echo "M5=$M5"
    fi
else
    #D6=$C6
    #M6=$(echo "obase=16; $D6"|bc)
    M6=$(busybox printf "%X\n" $C6)
    if [ $C6 -le 15 ];
    then
        M6="0$M6"
    fi
    #echo "M6=$M6"
fi
echo "Convert end !"

MACADDRESS="$M1:$M2:$M3:$M4:$M5:$M6"
echo "WIFI MAC: $MACADDRESS"
echo $MACADDRESS > $TARGET_FW_MAC_FILE

echo " ----- Calirator start ---- "

# Disblae wlan interface
# netcfg wlan0 down

# Remove wl12xx driver
#rmmod ${WL12XX_MODULE_PATH}/wl12xx_sdio.ko

# Remove old nvs file
rm ${TARGET_NVS_FILE}

# Calibrator process
calibrator plt autocalibrate2 wlan0 ${TARGET_INI_FILE} ${TARGET_NVS_FILE}

echo " ----- Calibrator end ---- "

# Check file whether exist then set mac addres to newset nvs file
if [ -f ${TARGET_NVS_FILE} ]; then                                                                                                    
    calibrator set nvs_mac ${TARGET_NVS_FILE} $MACADDRESS
    echo " Calibrator completed !!"
else
	echo " Calibrator failed !! "
	exit 1
fi

# Re-load wl12xx driver
insmod ${WL12XX_MODULE_PATH}/wl12xx_sdio.ko
