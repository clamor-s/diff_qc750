#!/system/bin/sh

TARGET_INI_FILE=/system/etc/firmware/ti-connectivity/TQS_S_2.6.ini
TARGET_NVS_FILE=/data/misc/wifi/wl1271-nvs.bin
BACK_NVS_FILE=/forever/misc/wifi/wl1271-nvs2.bin
NVS_SIZE=1113

TARGET_FW_MAC_FILE=/forever/misc/wifi/mac_address
WL12XX_MODULE_PATH=/system/lib/modules/compat

#if busybox not exits, end of processes.
if [ ! -f /system/bin/busybox ]; then
    echo "ERROR: Please install busybox !!"
    exit 
fi

if [ -f ${TARGET_NVS_FILE} ]; then
	CHECK1=$(calibrator get nvs_mac ${TARGET_NVS_FILE} | busybox grep 'MAC addr from NVS:' | busybox sed 's/^.*NVS://g')
	CHECK1=$(echo "$CHECK1" | busybox awk -F' ' '{ print $1 }')
	CHECK2=$(calibrator get nvs_mac ${BACK_NVS_FILE} | busybox grep 'MAC addr from NVS:' | busybox sed 's/^.*NVS://g')
	CHECK2=$(echo "$CHECK2" | busybox awk -F' ' '{ print $1 }')
	log -p e -t wifi_calibrator "CHECK1 is $CHECK1 and CHECK2 is $CHECK2"
	if [ $CHECK1 == $CHECK2 ]; then
		log -p e -t wifi_calibrator "mac ok ,exit"
		exit
	else
		log -p e -t wifi_calibrator "mac failed ,repair"
		rm ${TARGET_NVS_FILE}
	fi
fi

if [ -f ${TARGET_FW_MAC_FILE} ]; then
MAC=$(cat ${TARGET_FW_MAC_FILE} | busybox awk -F' ' '{ print $1 }')
LEN=0

#check mac address length
LEN=${#MAC}
if [ $LEN -gt 17 ]; #12bits
then
    log -p e -t wifi_calibrator "LEN=$LEN"
    log -p e -t wifi_calibrator "ERROR MAC ADDRESS!!!"
    exit
fi

log -p e -t wifi_calibrator "read mac from ${TARGET_FW_MAC_FILE}: $MAC"

M1=$(echo "$MAC" | busybox awk -F: '{ print $1 }')
M2=$(echo "$MAC" | busybox awk -F: '{ print $2 }')
M3=$(echo "$MAC" | busybox awk -F: '{ print $3 }')
M4=$(echo "$MAC" | busybox awk -F: '{ print $4 }')
M5=$(echo "$MAC" | busybox awk -F: '{ print $5 }')
M6=$(echo "$MAC" | busybox awk -F: '{ print $6 }')
else
MAC=$(getprop persist.service.bdroid.wifi)
log -p e -t wifi_calibrator "read mac from persist.service.bdroid.wifi : $MAC"
MAC=$(echo "$MAC" | busybox awk -F' ' '{ print $1 }')
LEN=0
MAC_INC=1

#check mac address length
LEN=${#MAC}
if [ $LEN -gt 17 ]; #17bits
then
    log -p e -t wifi_calibrator "LEN=$LEN"
    log -p e -t wifi_calibrator "ERROR MAC ADDRESS!!!"
    exit
fi

M1=$(echo "$MAC" | busybox awk -F: '{ print $1 }')
M2=$(echo "$MAC" | busybox awk -F: '{ print $2 }')
M3=$(echo "$MAC" | busybox awk -F: '{ print $3 }')
M4=$(echo "$MAC" | busybox awk -F: '{ print $4 }')
M5=$(echo "$MAC" | busybox awk -F: '{ print $5 }')
M6=$(echo "$MAC" | busybox awk -F: '{ print $6 }')

log -p e -t wifi_calibrator "Convert start..."
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
                        M1=$(busybox printf "%x\n" $C1)
                        if [ $C1 -le 15 ];
                        then
                            M1="0$M1"
                        fi
                        #echo "M1=$M1"
                    fi
                else
                    #D2=$C2
                    #M2=$(echo "obase=16; $D2"|bc)
                    M2=$(busybox printf "%x\n" $C2)
                    if [ $C2 -le 15 ];
                    then
                        M2="0$M2"
                    fi
                    #echo "M2=$M2"
                fi

            else
                #D3=$C3
                #M3=$(echo "obase=16; $D3"|bc)
                M3=$(busybox printf "%x\n" $C3)
                if [ $C3 -le 15 ];
                then
                    M3="0$M3"
                fi
                #echo "M3=$M3"
            fi
        else
            #D4=$C4
            #M4=$(echo "obase=16; $D4"|bc)
            M4=$(busybox printf "%x\n" $C4)
            if [ $C4 -le 15 ];
            then
                M4="0$M4"
            fi
            #echo "M4=$M4"
        fi
    else
        #D5=$C5
        #M5=$(echo "obase=16; $D5"|bc)
        M5=$(busybox printf "%x\n" $C5)
        if [ $C5 -le 15 ];
        then
            M5="0$M5"
        fi
        #echo "M5=$M5"
    fi
else
    #D6=$C6
    #M6=$(echo "obase=16; $D6"|bc)
    M6=$(busybox printf "%x\n" $C6)
    if [ $C6 -le 15 ];
    then
        M6="0$M6"
    fi
    #echo "M6=$M6"
fi
log -p e -t wifi_calibrator "Convert end !"
fi

MACADDRESS="$M1:$M2:$M3:$M4:$M5:$M6"
log -p e -t wifi_calibrator "final wifi mac : $MACADDRESS"
if [ ! -f ${TARGET_FW_MAC_FILE} ]; then
	echo $MACADDRESS > $TARGET_FW_MAC_FILE
	chmod	777 $TARGET_FW_MAC_FILE
fi

log -p e -t wifi_calibrator " ----- Calibrator start ---- "

# Calibrator process
calibrator plt autocalibrate2 wlan0 ${TARGET_INI_FILE} ${TARGET_NVS_FILE}
calibrator set nvs_mac ${TARGET_NVS_FILE} $MACADDRESS
netcfg wlan0 up
netcfg wlan0 down
#PHY=`ls /sys/class/ieee80211/`
#echo " PHY = ${PHY} "
#iw ${PHY} interface add p2p0 type managed

# Check file
if [ ! -f ${BACK_NVS_FILE} ]; then
	if [ -f ${TARGET_NVS_FILE} ]; then
		REAL_SIZE=`busybox stat ${TARGET_NVS_FILE} | busybox grep Size | busybox awk '{print $2}'`
	  if [ ${REAL_SIZE} -eq $NVS_SIZE ]; then
	  	CURMAC_TARGET=$(calibrator get nvs_mac ${TARGET_NVS_FILE} | busybox grep 'MAC addr from NVS:' | busybox sed 's/^.*NVS://g')
	  	CURMAC_TARGET=$(echo "$CURMAC_TARGET" | busybox awk -F' ' '{ print $1 }')
	  	log -p e -t wifi_calibrator "read  ${TARGET_NVS_FILE} is $CURMAC_TARGET"
	  	if [ $CURMAC_TARGET == $MACADDRESS ]; then
	  			dd if=${TARGET_NVS_FILE} of=${BACK_NVS_FILE}
	  			REAL_SIZE=`busybox stat ${BACK_NVS_FILE} | busybox grep Size | busybox awk '{print $2}'`
					echo "REAL_SIZE 2 is $REAL_SIZE"
					if [ ${REAL_SIZE} -eq $NVS_SIZE ]; then
						CURMAC_BACK=$(calibrator get nvs_mac ${BACK_NVS_FILE} | busybox grep 'MAC addr from NVS:' | busybox sed 's/^.*NVS://g')
						CURMAC_BACK=$(echo "$CURMAC_BACK" | busybox awk -F' ' '{ print $1 }')
						if [ $CURMAC_BACK == $MACADDRESS ]; then
							log -p e -t wifi_calibrator "copy backup file ok"
							chmod	777 ${BACK_NVS_FILE}
						else
							log -p e -t wifi_calibrator "copy backup file failed!"
							rm ${BACK_NVS_FILE}
						fi
					else
						log -p e -t wifi_calibrator "copy backup file failed!"
						rm ${BACK_NVS_FILE}
					fi
			fi
		fi
	fi
fi