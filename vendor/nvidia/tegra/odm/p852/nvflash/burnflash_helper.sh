TargetType="p852"
SysMem1GB=0
BoardKernArgs="usbcore.old_scheme_first=1"
BoardOptions="b:f:ls:k:p:u"
PartitionLayout=
LinuxBoot=
FuseType="fused"
WorkaroundFilesPath=${PWD}
DevUart=/dev/ttyS0
IsResetNeeded="no"
Disp="tftlcd"
UserConfigFile=
ConfigBootMedium=0
ValidSkus=( 1 3 5 8 9 13 23 )
SkuValid=0

BoardParseOptions()
{

    case $Option in
        f) if [ $OPTARG = "fused" ] || [ $OPTARG = "unfused" ]; then
               FuseType=$OPTARG
           else
               echo "Invalid option for -f"
               Usage
               AbnormalTermination
           fi ;;
        b) if [ $OPTARG = "nor" ] || [ $OPTARG = "nand" ] || [ $OPTARG = "emmc" ] ; then
               BootMedium=$OPTARG
	       ConfigBootMedium=1
           else
               echo "Invalid option for -b"
               Usage
               AbnormalTermination
           fi ;;
        l) if [ $FuseType = "fused" ] ; then
               echo "The -l option is available only for unfused boards."
               Usage
               AbnormalTermination
           else
               LinuxBoot=1
           fi ;;
        s) if [ $FuseType = "fused" ] ; then
               echo "The -s option is available only for unfused boards."
               Usage
               AbnormalTermination
           else
               DevUart=$OPTARG
               if [ ! -e $DevUart ]; then
                   echo "Invalid option for -s"
                   exit -1
               fi
           fi ;;
        u) UseUboot=1
           ;;
        k) ConfigFile=$OPTARG
           if [ ! -e $ConfigFile ]; then
               echo "Config file $ConfigFile does not exist"
               exit -1
           fi
           UserConfigFile=1
           ;;
        p) PartitionLayout=$OPTARG
           ;;
        *) echo "Invalid option $Option"
           Usage
           AbnormalTermination
           ;;
    esac
}

#functions needed by burnflash.sh

BoardHelpHeader()
{
    echo "Usage : `basename $0` [ -h ]  -b <BOOTMEDIUM> <-n <NFSPORT> | -r <ROOTDEVICE> > [ options ]"
}

BoardHelpOptions()
{
    echo "-b : Selects bootmedium. Supported types: emmc, nand and nor."
    echo "-u : Flash the U-Boot bootloader"
    echo "-S : Selects SKU id. For example, it can be 11, 13, 23 etc."
    echo "-k : Provide the nvflash config file to be used. Default value is quickboot_snor_linux.cfg for nor boot"
    echo "     and quickboot_nand_linux.cfg for nand boot."
    echo -n "-p : Provide the partition Layout for the device in the mtdpart format. "
    echo "Partition start and size should be block size aligned. Valid only for nand and nor"
    echo "     example: mtdparts:<DRIVER_NAME>:size@start(PART_NAME)"
    echo "              \"mtdparts=tegra_nand:1024K@26752K(env),496000K@27776K(userspace) \""
    echo "              \"mtdparts=tegra_nor:384K@36992K(env),28160K@37376K(userspace) \""
    echo ""
    echo "Supported <NFSPORT>    : usb0 : Ethernet connector J35 and J36"
    echo "                       : eth0 : USB-Ethernet adapter"
    echo "                       Supported USB-Ethernet adapters: D-Link DUB-E100, Linksys USB200M ver.2.1"
    echo "                       note: kernel command line parameter \"ehci3=usb\" is required for enabling"
    echo "                       usb-hub on J40 connector"
    echo "                       : usb1 : Ethernet connector J59"
    echo "                       note: kernel command line parameter \"ehci3=eth\" is required for enabling usb1"
    echo "                       NOTE: In linux, the interface name for usbnet may be usb* or eth* based on the"
    echo "                       mac address. If bit 2 of the first octet is zero then we get eth*, else usb*"

    if [ -f "internal_setup.sh" ] ; then
        echo ""
        echo "Internal flags (for NVIDIA use only):"
        echo "-f : Selects fuse type. Options: fused, unfused. Default is fused"
        echo "-l : Optional Linux boot, unfused boards only"
        echo "-s : Optional serial port on the host (default /dev/ttyS0), unfused boards only"
    fi
}

BoardValidateParams()
{
    if [ $BootMedium = "emmc" ] ; then
        if [ -n "${PartitionLayout}" ]; then
            echo "Partition Layout passing is only supported for Nand and Nor"
            AbnormalTermination
        fi
    fi

    # if SKU file is specified, extract sku id from the filename
    if [ -n "${ZFile}" ]; then
        SkuId=`echo $ZFile|cut -d '-' -f 3`
    fi

    if [ $SkuId -eq 0 ]; then
        echo "Missing -S option (mandatory)"
        Usage
        AbnormalTermination
    fi

    for Sku in ${ValidSkus[@]}
    do
        if [ $SkuId -eq $Sku ]; then
            SkuValid=1
        fi
    done

    if [ $SkuValid -eq 0 ]; then
        echo "Invalid SKU id"
        AbnormalTermination
    fi

    # Determine the system memory from SKU number
    if [ $SkuId -eq 23 ] || [ $SkuId -eq 13 ]; then
        SysMem1GB=1
        BootMedium="nand"
    fi

    # Set BootMedium based on SkuId if BootMedium is not configured
    if [ $SkuId -eq 1 ] || [ $SkuId -eq 8 ] || [ $SkuId -eq 9 ]; then
        if [ $ConfigBootMedium -eq 0 ]; then
            BootMedium="nor"
        fi
    fi

    if [ ${SysMem1GB} -eq 1 ] ; then
        if [ $SkuId -eq 23 ]; then
            let "ODMData = 0x3b0d0105" # UART-C (lower J16)
        else
            let "ODMData = 0x3b0c0105" # UART-A (lower J20)
#           let "ODMData = 0x300c8105" # UART-B (upper J16)
        fi
    else
        if [ $SkuId -eq 5 ]; then
            let "ODMData = 0xd0d0105" # UART-C (lower J16)
            BootMedium="nand"
        else
            let "ODMData = 0xd0c0105" # UART-A (lower J20)
#           let "ODMData = 0xc8105" # UART-B (upper J16)
        fi
    fi

    if [ $SkuId -eq 1 ] || [ $SkuId -eq 8 ]; then
        let "ODMData = 0x3b0d8105" # UART-D (upper J20)
    elif [ $SkuId -eq 9 ]; then
        let "ODMData = 0x2d0d8105" # 80MB carveout, 512MB ram, UART-D
    fi

    if [ "$BootMedium" = "emmc" ] ; then
        BCT=${NvFlashDir}/p852_12MHz_K4T1G084QE-HCF8_333MHz_1GB_emmc_THGBM1G6D4EBAI4.bct
        if [ -z $ConfigFile ]; then
            if [ $Quickboot -eq 0 ] ; then
                ConfigFile=${NvFlashDir}/fastboot_emmc_linux.cfg
                PartitionLayout=""
            else
                ConfigFile=${NvFlashDir}/quickboot_emmc_linux.cfg
                PartitionLayout=""
            fi
        fi
    fi
    if [ "$BootMedium" = "nand" ] ; then
        if [ ${SysMem1GB} -eq 1 ] ; then
            BCT=${NvFlashDir}/p852_12MHz_H5PS1G83EFR-S5I_333MHz_1GB_nand_MT29F4G08ABADA.bct
        else
            BCT=${NvFlashDir}/p852_12MHz_H5PS1G83EFR-S5I_333MHz_512MB_nand_MT29F4G08ABADA.bct
        fi
        if [ -z $ConfigFile ]; then
            if [ $Quickboot -eq 0 ] ; then
                ConfigFile=${NvFlashDir}/fastboot_nand_linux.cfg
                if [ -z ${PartitionLayout} ]; then
                    PartitionLayout="mtdparts=tegra_nand:1024K@15488K(env),507264K@16512K(userspace) "
                fi
            else
                ConfigFile=${NvFlashDir}/quickboot_nand_linux.cfg
                if [ -z ${PartitionLayout} ]; then
                    PartitionLayout="mtdparts=tegra_nand:1024K@26752K(env),496000K@27776K(userspace) "
                fi
            fi
        fi
    fi
    if [ "$BootMedium" = "nor" ] ; then
        if [ $SkuId -eq 9 ]; then
            BCT=${NvFlashDir}/p852_12MHz_H5PS1G83EFR-S5I_333MHz_512MB_nor_S29GL512N.bct
        else
            BCT=${NvFlashDir}/p852_12MHz_H5PS1G83EFR-S5I_333MHz_1GB_nor_S29GL512N.bct
        fi
        if [ -z $ConfigFile ]; then
            if [ $Quickboot -eq 0 ] ; then
                ConfigFile=${NvFlashDir}/fastboot_snor_linux.cfg
                if [ -z ${PartitionLayout} ]; then
                    PartitionLayout="mtdparts=tegra_nor:384K@17664K(env),47488K@18048K(userspace) "
                fi
            else
                ConfigFile=${NvFlashDir}/quickboot_snor_linux.cfg
                if [ -z ${PartitionLayout} ]; then
                    PartitionLayout="mtdparts=tegra_nor:384K@36992K(env),28160K@37376K(userspace) "
                fi
            fi
        fi
    fi
    BoardKernArgs="${BoardKernArgs} ${PartitionLayout}"
}

SendWorkaroundFiles()
{
    stty -F $DevUart 57600
    echo "Connect the serial cable between UART1 on the base board and Host PC."
    echo "UART1 it the lower connector on J16."
    echo "Press Enter to continue."
    read
    if [ "$4" == "yes" ] ; then
        echo "Now reset the board (press and release Reset Switch S5)."
    fi
    echo "You should see 'NV Boot AP20 0002.0001' message on the serial console."
    echo "Keep resetting the board until you get this message properly, then"
    echo "press Enter to continue."
    read

    cat ${WorkaroundFilesPath}/$1 > ${DevUart}
    echo "Sent $1."
    echo "You should see the message 'Fail' on the serial console."
    echo "If you did not see this message please rerun the script."
    echo ""
    echo "press Enter to continue."
    read

    cat ${WorkaroundFilesPath}/$1 > ${DevUart}
    echo "Sent $1."
    echo "You should see the message 'Fail' on the serial console."
    echo "If you did not see this message please rerun the script."
    echo ""
    echo "press Enter to continue."
    read

    cat ${WorkaroundFilesPath}/$2 > ${DevUart}
    echo "Sent $2, Workaround file to boot out of $3."
    echo "You should see the message 'Boot' on the serial console."
    echo "If you did not see this message please rerun the script."
    echo ""
    echo "press Enter to continue."
    read
}

# called before nvflash is invoked
BoardPreflashSteps()
{
    echo "If board is not ON, power it by moving switch S6 to ON position,"
    echo "then press the power button S4 for at least 2 seconds, then release it."
    echo "Press Enter to continue"
    read
    if [ "$FuseType" = "fused" ] || ([ "$FuseType" = "unfused" ] && [ "$LinuxBoot" = 0 ]); then
        echo "Install \"Force Recovery\" jumper (J42) , then press and release"
        echo "the Reset Switch (S5)."
        echo "Press Enter to continue"
        read
    fi
    if [ "$FuseType" = "unfused" ] && [ "$LinuxBoot" = 1 ]; then
        echo "Remove \"Force Recovery\" jumper (J42) , then press and release"
        echo "the Reset Switch (S5)."
        echo "Press Enter to continue"
        read
   fi

    #################### Boot Linux on an unfused board ########################
    if [ "$LinuxBoot" = 1 ]; then
        if [ $FuseType != "unfused" ]; then
            echo "Linux Boot is only for unfused boards."
            exit 1;
        fi
        WARFile1=bypass.uart.workaround
        case $BootMedium in
            emmc) WARFile2=bypass.uart.packaged.jump_to_irom.ap20_nv_sdmmc_1 ;;
        esac
        IsResetNeeded="yes"
        SendWorkaroundFiles ${WARFile1} ${WARFile2} ${BootMedium} ${IsResetNeeded}
        exit 0
    fi

    if [ "$FuseType" = "unfused" ] ; then
        WARFile1=bypass.uart.workaround
        case $BootMedium in
            emmc) WARFile2=bypass.uart.packaged.jump_to_irom.ap20_nv_sdmmc_1_rcm_via_pmc ;;
        esac
        IsResetNeeded="yes"
        SendWorkaroundFiles ${WARFile1} ${WARFile2} ${BootMedium} ${IsResetNeeded}
    fi

}


# called immediately after nvflash is done
BoardPostNvFlashSteps()
{
    if [ "$FuseType" = "unfused" ] && [ "$LinuxBoot" = 1 ]; then
        echo "Connect the serial cable between UART2 on the base board and Host PC."
        echo "UART2 it the upper connector on J16."
    fi
}

# called after the kernel was flashed with fastboot
BoardPostKernelFlashSteps()
{
    if [ "$FuseType" = "fused" ]; then
        echo "To boot, remove \"Force Recovery\" jumper (J42) and reset the board."
    elif [ "$FuseType" = "unfused" ]; then
        echo "To boot, rerun this script with the same arguments but add"
        echo "-l to the end of the commandline."
    fi
}

# called if uboot was flashed
BoardUseUboot()
{
    BoardPostKernelFlashSteps
    echo "u-boot will pickup kernel image over tftp for booting"
}

